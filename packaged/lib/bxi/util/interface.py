# -*- coding: utf-8 -*-
###############################################################################
# Author: Pierre Imbaud
# Contributors:
###############################################################################
# Copyright (C) 2014 - 2015  Bull S. A. S.  -  All rights reserved
# Bull, Rue Jean Jaures, B.P.68, 78340, Les Clayes-sous-Bois
# This is not Free or Open Source software.
# Please contact Bull S. A. S. for details about its license.
###############################################################################


""" determine interface to reach a given target

    use the ip route command to determine which interface, among
    current host interfaces, is used reach a given target
"""

import subprocess as sp
import socket
import re

def interface(target):
    
    """ use the ip route command to determine which interface, among
        current host interfaces, is used reach a given target

        @param target: host to reach, ip or dns name
        @return interface ip address
    """
    ip = socket.gethostbyname(target)
    command = ("ip route get %s" %ip).split()
    out = sp.check_output(command)
    lines = out.split('\n')
    l0 = lines[0]
    reg = re.compile('(^[0-9.]+) via ([0-9.]+).*src ([0-9.]+)')
    match = reg.match(l0)
    assert match
    interface = match.groups()[-1]
    print match.groups()
    return interface
    
