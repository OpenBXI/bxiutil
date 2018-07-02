#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
@author Jean-Noel Quintin <<jean-noel.quintin@atos.net>>
@copyright 2018 Bull S.A.S.  -  All rights reserved.\n
           This is not Free or Open Source software.\n
           Please contact Bull SAS for details about its license.\n
           Bull - Rue Jean Jaures - B.P. 68 - 78340 Les Clayes-sous-Bois
@namespace bxi.base.builder cffi builder

"""

from cffi import FFI
from bxi.base.cffi_builder import ffibuilder as base_ffibuilder

import bxi.util_cffi_def as cdef
libname = 'bxiutil'
modulename = 'bxi.util.cffi_h'

ffibuilder = FFI()
ffibuilder.include(base_ffibuilder)
ffibuilder.set_source(modulename,
                      None,
                      libraries=[libname])   # or a list of libraries to link with
ffibuilder.cdef(cdef.C_DEF)


if __name__ == "__main__":
    ffibuilder.compile(verbose=True)
