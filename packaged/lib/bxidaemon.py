# -*- coding: utf-8 -*-
################################################################################
# Author: Sébastien Miquée <sebastien.miquee@bull.net>
# Contributor:
################################################################################
# Copyright (C) 2014  Bull S. A. S.  -  All rights reserved
# Bull, Rue Jean Jaures, B.P.68, 78340, Les Clayes-sous-Bois
# This is not Free or Open Source software.
# Please contact Bull S. A. S. for details about its license.
################################################################################

"""-= The BXI Daemon library =-"""

from configutils.configuration_parser import MultiLevelConfigParser
from configutils.configuration_parser import ConfigMultiLevel
from configutils.configuration_parser import ParsingError
from logged_posless import ArgumentParser

import zmq

import bxi.base.log as logging
import ast
import sys
import os


_LOGGER = logging.getLogger(__name__)


# -#-----------------------------------------------------------------------------------#-#
class AdditionalArgument(object):
    """class describing a new argument for the argument parser"""
    def __init__(self, group, *args, **kwargs):
        # no log should be done before the initialization of the logger
        # _LOGGER.debug("Creating a new parser argument for group '%s'", group)
        self.group = group
        self.args = args
        self.kwargs = kwargs


# -#-----------------------------------------------------------------------------------#-#
class BXIDaemon(object):
    """BXI Daemon class, directly from the Hell"""

    def __init__(self, args):
        """Initialization"""
        _LOGGER.debug("Initializing daemon")
        self.args = args
        self.config = None
        self.zctx = None
        self.zctrl = None
        self.bthread = None
        self.exit = 0
        self._logfile = args.logfile
        self._logcfg = args.logcfg

        # Reading configuration file
        self._read_config_file()

        # Resetting configuration according to the command line options
        self._reconfigure()

        # Access to options through local fields
        self._set_fields()

        # Storing the PID
        self._store_pid()

    def _clean(self, fpidpb=False):
        """Cleaning all the stuff created by the daemon"""
        # Cleaning the PID file
        if not fpidpb:
            if os.path.exists(self.pid_file):
                try:
                    os.remove(self.pid_file)
                    _LOGGER.debug('Successfully removed PID file')
                except OSError as err:
                    _LOGGER.error('Error while trying to remove PID file: %s', err)

    def _reconfigure(self):
        """Reset the configuration following the command line options"""
        # TODO: Take into account also environment variables before doing the following
        # Replacing values from the command line options
        for opt, value in self.args.__dict__.items():
            if value is not None:
                if type(value) == str and 'Default: ' not in value:
                    _LOGGER.debug("Reconfiguring from command line option: '%s'", opt)
                    self.config[opt] = self.args.__dict__[opt]

    def _set_fields(self):
        """Set the configuration options as local fields"""
        # Setting the ones in the configuration object
        for key, value in self.config.items():
            try:
                self.__setattr__(key, ast.literal_eval(value))
                _LOGGER.debug("Configuring option '%s' with value: '%s'", key, value)
            except (ValueError, SyntaxError):
                self.__setattr__(key, value)

        # Setting the ones existing only on the command line
        for key, value in self.args.__dict__.items():
            if key not in self.config.keys():
                if type(value) == str:
                    value = value.rsplit('Default: ')[-1]
                try:
                    self.__setattr__(key, ast.literal_eval(value))
                except (ValueError, SyntaxError):
                    self.__setattr__(key, value)

    def _read_config_file(self):
        """Read the configuration file"""
        # Loading configuration file
        if self.args.cfg_file != 'None':
            try:
                _LOGGER.debug("Reading configuration file: '%s'", self.args.cfg_file)
                with open(self.args.cfg_file, 'r') as f:
                    self.config = MultiLevelConfigParser(f)
                    self.config.parse()
            except IOError as err:
                _LOGGER.error('Configuration file error: %s', err)
                print('Configuration file error: %s' % err)
                sys.exit(1)
            except ParsingError as err:
                _LOGGER.error('Configuration file parsing error: %s', err)
                print('Configuration file parsing error: %s' % err)
                sys.exit(2)
        else:
            _LOGGER.warning("No configuration file specified")
            self.config = ConfigMultiLevel()

    def _store_pid(self):
        """Store the current process PID in a file"""
        # A daemon is already running
        if os.path.exists(self.pid_file):
            if not self.config['force_start']:
                _LOGGER.error('PID file already existing')
                print('The PID file already exists! It seems that another daemon is'
                      ' currently running.')
                self._clean(fpidpb=True)
                sys.exit(4)

        # That a new daemon, so writing its PID in the correct file
        try:
            _LOGGER.debug("Creating PID file")
            with open(self.pid_file, 'w') as f:
                f.write('%d\n' % os.getpid())
        except IOError as err:
            _LOGGER.error('Problem while writing the PID in the file: %s', err)
            print('Problem while writing the PID in the file: %s' % err)
            self._clean()
            sys.exit(5)

    def start(self):
        """Start the business code in a dedicated thread"""
        self._start()
        self._clean()
        sys.exit(self.exit)

    def _start(self):
        """The business code to launch"""
        raise NotImplementedError


def create_daemon_parser(*args, **kwargs):
    """Create and return an argument parser for BXI Daemon"""

    if 'description' not in kwargs:
        kwargs['description'] = 'BXI Daemon'

    if 'conf_file' in kwargs:
        default_conf_file = 'Default: %s' % kwargs.pop('conf_file')
    else:
        default_conf_file = 'Default: /etc/bxi/global.conf'

    if 'pid_file' in kwargs:
        default_pid_file = 'Default: %s' % kwargs.pop('pid_file')
    else:
        default_pid_file = 'Default: /run/bxi/daemon.pid'

    additional_args = kwargs.pop('additional_args', list())

    dameon_parser = ArgumentParser(*args, **kwargs)

    # Common options
    gbxi = dameon_parser.add_argument_group('BXI Common options')
    gbxi.add_argument('-c', '--config-file',
                      action='store',
                      dest='cfg_file',
                      metavar='FILE',
                      type=str,
                    default=default_conf_file,
                      help="Daemon configuration file [%(default)s]")

    gbxi.add_argument('-p', '--pid-file',
                      action='store',
                      dest='pid_file',
                      metavar='FILE',
                      type=str,
                    default=default_pid_file,
                      help="Daemon PID file [%(default)s]")

    gbxi.add_argument('-f', '--force-start',
                      action='store_true',
                      dest='force_start',
                      help="Force daemon to start")

    gbxi.add_argument('--bxidir',
                      action='store',
                      type=str,
                      metavar='DIR',
                      dest='bxidir',
                      help="Folder containing both the topology description and its values")

    gbxi.add_argument('--bb-url',
                      action='store',
                      type=str,
                      metavar='URL',
                      dest='bb_url',
                      help="Full url to connect to the BXI Backbone")

    gbxi.add_argument('--name',
                      action='store',
                      type=str,
                      metavar='STR',
                      dest='name',
                      help="Name of the daemon (identification in the Backbone)")

    gbxi.add_argument('--subtrees',
                      action='store',
                      type=str,
                      metavar='"/sub/tree" | "/sub/tree0, /sub/tree1"',
                      dest='subtrees',
                      help="List of the subtrees to subscribe to the Backbone")

    gbxi.add_argument('--serialize',
                      action='store',
                      type=str,
                      metavar='FCT',
                      dest='serialize',
                      help="Function to do the serialization before a send to the Backbone")

    gbxi.add_argument('--deserialize',
                      action='store',
                      type=str,
                      metavar='FCT',
                      dest='deserialize',
                      help="Function to do the deserialization when receiving a message from the Backbone")

    for new_arg in additional_args:
        if new_arg.group == 'bxi':
            gbxi.add_argument(*new_arg.args, **new_arg.kwargs)

    return dameon_parser


# -#-----------------------------------------------------------------------------------#-#
class BXIDaemonZMQ(BXIDaemon):
    """BXI Daemon class, directly from the Hell with a ZMQ interface"""

    def __init__(self, args):
        """Initialization"""
        super(BXIDaemonZMQ, self).__init__(args)

        # Opening the control zocket
        self._open_control_zocket()

    def _clean(self, fpidpb=False):
        """Cleaning all the stuff created by the daemon"""
        # Cleaning the control zocket
        if self.zctrl is not None:
            self.zctrl.close(1000)
            self.zctx.destroy(1000)

        super(BXIDaemonZMQ, self)._clean(fpidpb)

    def _open_control_zocket(self):
        """Create and configure the control zocket"""
        if 'ctrl_zocket_url' in self.config and len(self.config['ctrl_zocket_url']) > 0:
            _LOGGER.debug("Configuring the control zocket")
            try:
                self.zctx = zmq.Context()
                self.zctrl = self.zctx.socket(zmq.REQ)
                _LOGGER.debug("Opening control zocket at url: '%s'",
                              self.config['ctrl_zocket_url'])
                self.zctrl.bind(self.config['ctrl_zocket_url'])
            except zmq.ZMQError as err:
                _LOGGER.critical('Problem while creating the Control Zocket: %s', err)
                print('Problem while creating the Control Zocket: %s' % err)
                sys.exit(3)


def create_controlled_daemon_parser(*args, **kwargs):
    """Create and return an argument parser for ZMQ Controlled BXI Daemon"""

    ctrl_args = AdditionalArgument('bxi', '-z', '--ctrl-zocket-url',
                                   action='store',
                                   dest='ctrl_zocket_url',
                                   metavar='URL',
                                   type=str,
                                   help="Daemon control zocket URL")

    if 'additional_args' not in kwargs:
        kwargs['additional_args'] = list()
    kwargs['additional_args'].append(ctrl_args)

    return create_daemon_parser(*args, **kwargs)
