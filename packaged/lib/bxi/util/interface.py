# -*- coding: utf-8 -*-
###############################################################################
# Author: Pierre Imbaud
# Contributors:
###############################################################################
# Copyright (C) 2018 Bull S.A.S.  -  All rights reserved
# Bull, Rue Jean Jaures, B.P. 68, 78340 Les Clayes-sous-Bois
# This is not Free or Open Source software.
# Please contact Bull S. A. S. for details about its license.
###############################################################################


""" determine interface to reach a given target

    use the ip route command to determine which interface, among
    current host interfaces, is used to reach a given target

    The original purpose is to know which "trapDestination" one
    should provide to a bxi divio.
"""

import subprocess as sp
import socket
import re

import bxi.base.err as bxierr


def interface(target):

    """ use the ip route command to determine which interface, among
        current host interfaces, is used to reach a given target

        @param target: host to reach, ip or dns name
        @return interface ip address
    """
    ip = socket.gethostbyname(target)
    command_string = ("ip route get %s" % ip)
    command = command_string.split()
    out = sp.check_output(command)
    lines = out.split('\n')
    l0 = lines[0]
    reg = re.compile('.*src ([0-9.]+)')
    match = reg.match(l0)
    if not match:
        template = "command '%s'\nproduced '%s'\nthat does not match regex '%s'"
        msg = template % (command_string, l0, reg.pattern)
        raise bxierr.BXIError(msg)
    assert match
    interface = match.groups()[-1]
    return interface
