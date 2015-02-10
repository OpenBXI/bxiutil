#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
@authors Pierre Vignéras <pierre.vigneras@bull.net>
@copyright 2013  Bull S.A.S.  -  All rights reserved.\n
           This is not Free or Open Source software.\n
           Please contact Bull SAS for details about its license.\n
           Bull - Rue Jean Jaurès - B.P. 68 - 78340 Les Clayes-sous-Bois
@namespace bxi.base Python BXI Base module

"""
# Try to find other BXI packages in other folders
from pkgutil import extend_path
__path__ = extend_path(__path__, __name__)

from cffi import FFI
from bxi.util.cffi_h import C_DEF as LOG_DEF

__ffi__ = FFI()
__ffi__.cdef(LOG_DEF)
__api__ = __ffi__.dlopen('libbxiutil.so')

def get_ffi():
    return __ffi__

def get_api():
    return __api__

