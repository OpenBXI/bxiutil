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


import bxi.ffi as bxiffi
import bxi.base as bxibase
from bxi.util.cffi_h import C_DEF
from cffi.api import CDefError


bxiffi.add_cdef_for_type("bxirng_p", C_DEF)

__FFI__ = bxiffi.get_ffi()
__CAPI__ = __FFI__.dlopen('libbxiutil.so')


def get_capi():
    """
    Return the CFFI wrapped C library.

    @return the CFFI wrapped C library.
    """
    return __CAPI__


class Map(object):
    """
    Wrap the map C module
    """
    def __init__(self, nb_thread=0):
        """
        Initialise the threads for the task parallelization.
        @param nb_thread number of threads should be start.
        @return
        """
        nb_thread_p = __FFI__.new("size_t [1]")
        nb_thread_p[0] = nb_thread
        __CAPI__.bximap_init(nb_thread_p)

    def finalize(self):
        """
        Stop the threads
        @return
        """
        __CAPI__.bximap_finalize()

    def __del__(self):
        self.finalize()

    @staticmethod
    def Cpu_mask(mask):
        __CAPI__.bximap_set_cpumask(mask)
