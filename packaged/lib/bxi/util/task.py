# -*- coding: utf-8 -*-

"""
Module to allow using ClusterShell to dispatch commands using gateways

@authors Alain Cady <<alain.cady@atos.net>>
@authors Sébastien Miquée <<sebastien.miquee@atos.net>>
@copyright 2014 - 2016  Bull S.A.S.  -  All rights reserved.\n
           This is not Free or Open Source software.\n
           Please contact Bull SAS for details about its license.\n
           Bull - Rue Jean Jaurès - B.P. 68 - 78340 Les Clayes-sous-Bois
"""


import ClusterShell.Task
import ClusterShell.Event

import bxi.base.log as bxilog


_LOGGER = bxilog.get_logger(bxilog.LIB_PREFIX + "util.task")


class MyHandler(ClusterShell.Event.EventHandler):
    """
    Task events processing
    """

    def ev_start(self, worker):
        """
        Called to indicate that a worker has just started.

        @param[in] worker The worker that encounters an event

        @return None
        """
        _LOGGER.trace('worker started')

    def ev_read(self, worker):
        """
        Called to indicate that a worker has data to read.

        @param[in] worker The worker that encounters an event

        @return None
        """
        _LOGGER.debug("%s: %s", worker.current_node.encode("ascii"),
                      worker.current_msg.encode("ascii"))

    def ev_error(self, worker):
        """
        Called to indicate that a worker has error to read (on stderr).

        @param[in] worker The worker that encounters an event

        @return None
        """
        _LOGGER.error("%s: %s", worker.current_node.encode("ascii"),
                      worker.current_errmsg.encode("ascii"))

    def ev_written(self, worker):
        """
        Called to indicate that writing has been done.

        @param[in] worker The worker that encounters an event

        @return None
        """
        _LOGGER.debug("Writing done")

    def ev_hup(self, worker):
        """
        Called to indicate that a worker's connection has been closed.

        @param[in] worker The worker that encounters an event

        @return None
        """
        _LOGGER.trace("Connection to '%s' closed with %d",
                      worker.current_node, worker.current_rc)

    def ev_timeout(self, worker):
        """
        Called to indicate that a worker has timed out (worker timeout only).

        @param[in] worker The worker that encounters an event

        @return None
        """
        _LOGGER.critical('%s Timeout reached!', worker.current_node)

    def ev_close(self, worker):
        """
        Called to indicate that a worker has just finished
           (it may already have failed on timeout).

        @param[in] worker The worker that encounters an event

        @return None
        """
        _LOGGER.trace("Worker done")

    def ev_msg(self, port, msg):
        """Handle port message.

        @param[in] port The port that encounters an event
        @param[in] msg The event message

        @return None
        """
        _LOGGER.debug("%s msg: %s", port, msg)

    def ev_timer(self, timer):
        """Handle firing timer.

        @param[in] timer The timer that has expired

        @return None
        """
        _LOGGER.debug('Time expired')


def task_debug(task, msg):
    """
    Function used to do debugging

    @param[in] task The Task object to work on
    @param[in] msg The debug message

    @return None
    """
    _LOGGER.fine(msg)


def tasked(cmd, timeout=5, topo="", nodes="", no_remote=False):
    """
    Function that requests the execution of a command on multiple nodes in parallel.

    @param[in] cmd The command to execute (can either be a string or a list of strings)
    @param[in] timeout The timeout for the execution on remote nodes (default 5s)
    @param[in] topo The path to the 'clush' topology file
    @param[in] nodes The nodes on which executing the command (nodeset notation)
    @param[in] no_remote Boolean indicating to not execute the command on final nodes,
                         but on their parent

    @return The execution code: 0 if ok, 1 else
    """
    _LOGGER.debug("Initializing clush task object")
    handler = MyHandler()
    task = ClusterShell.Task.task_self()
    cmd = " ".join(cmd)

    _LOGGER.debug("Configuring clush debugging function to use bxilog")
    task.set_info("debug", True)

    tree = False
    remote = not no_remote

    if len(topo) > 0:
        _LOGGER.debug("Reding the clush topology file: '%s'", topo)
        task.load_topology(topo)
        tree = True

    if len(nodes) == 0:
        nodes = "localhost"

    _LOGGER.debug("Scheduling the task: cmd: '%s', nodes: '%s',"
                  " timeout: '%d', tree: '%s', remote: '%s'",
                  cmd, nodes, timeout, tree, remote)
    task.shell(cmd,
               nodes=nodes, handler=handler, timeout=timeout,
               tree=tree, remote=remote)

    _LOGGER.debug("Requesting task to start")
    task.resume()

    _LOGGER.debug("Waiting for task completion")
    ClusterShell.Task.task_wait()

    _LOGGER.debug("Task completed")
    rc = 0
    if task.max_retcode():
        for ret, nodes in task.iter_retcodes():
            if ret != 0:
                rc = 1
                for node in nodes:
                    _LOGGER.warning("%s terminated with %s: %s",
                                    node, ret, task.node_buffer(node))

    if task.num_timeout():
        _LOGGER.warning("%d timeout reached: %s", task.num_timeout(),
                        ", ".join(task.iter_keys_timeout()))
        rc = 1

    return rc


if "__main__" == __name__:
    import posless as argparse

    PARSER = argparse.ArgumentParser()
    PARSER.add_argument("--timeout", "-t", default=5, type=int)
    PARSER.add_argument("--loglevel", "-l", default=":debug,bxi:output", type=str)
    PARSER.add_argument("cmd", default="uname -r", type=str)
    PARSER.add_argument("--topo", type=str, default="")
    PARSER.add_argument("--nodes", "-w", type=str, default="")
    PARSER.add_argument("--no-remote", action="store_true", default=False)
    ARGS = PARSER.parse_args()

    bxilog.basicConfig(cfg_items=ARGS.loglevel)
    del ARGS.loglevel

    tasked(**vars(ARGS))
