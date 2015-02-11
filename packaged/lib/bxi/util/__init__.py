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
import bxi.base as bxibase
from bxi.util.cffi_h import C_DEF
from cffi.api import CDefError

__FFI__ = FFI()

# Including the BB Client C specification
bxibase.include_if_required(__FFI__)
__FFI__.cdef(C_DEF)
__CAPI__ = __FFI__.dlopen('libbxiutil.so')


def include_if_required(other_ffi):
    try:
        other_ffi.getctype("bxirng_p")
    except CDefError as ffie:
        other_ffi.cdef(C_DEF)


def get_ffi():
    return __FFI__

def get_api():
    return __CAPI__

