# -*- coding: utf-8 -*-

"""
@authors Pierre Vignéras <pierre.vigneras@bull.net>
@copyright 2015  Bull S.A.S.  -  All rights reserved.\n
           This is not Free or Open Source software.\n
           Please contact Bull SAS for details about its license.\n
           Bull - Rue Jean Jaurès - B.P. 68 - 78340 Les Clayes-sous-Bois
@namespace bxi.util Python BXI Utilitaries

"""
from __future__ import print_function
import re
import math
import sys
from pkgutil import extend_path
import bxi.base.log as bxilog


# Try to find other BXI packages in other folders
__path__ = extend_path(__path__, __name__)

from bxi.util.cffi_h import ffi

_LOGGER = bxilog.getLogger(bxilog.LIB_PREFIX + 'bxiutil')


__FFI__ = ffi
__CAPI__ = __FFI__.dlopen('libbxiutil.so')


# Used by smartdisplay
FILL_EMPTY_ENTRY = u'!#$_'
TRUNCATION_REF = u"..{%s}"
TRUNCATION_MAX_SIZE = len(TRUNCATION_REF)
REMOVE_UNSPECIFIED_COLUMNS = -1

# Use by replace_if_none
NONE_VALUE = unicode(None)


def get_ffi():
    """
    Return the ffi object used by this module to interact with the C backend.

    @return the ffi object used by this module to interact with the C backend.
    """
    return __FFI__


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

    @staticmethod
    def finalize():
        """
        Stop the threads
        @return
        """
        __CAPI__.bximap_finalize()

    def __del__(self):
        self.finalize()

    @staticmethod
    def cpu_mask(mask):
        """
        Map the threads on some cpus
        """
        __CAPI__.bximap_set_cpumask(mask)


# clm_sequencer
# Code below taken from http://code.activestate.com/recipes/267662/ (r7)
# and modified to our own needs.
def indent(rows,
           hasheader=False, headerchar=u'-',
           delim=u' | ',
           justify_functions=None,
           separateRows=False,
           prefix=u'', postfix=u'',
           max_widths=None,
           filler_char=u'_',
           output=sys.stdout):
    '''Indents a table by column.
    @param[in] rows A sequence of sequences of items, one sequence per row.
    @param[in] hasheader True if the first row consists of the columns' names.
    @param[in] headerchar Character to be used for the row separator line
        (if hasheader==True or separateRows==True).
    @param[in] delim The column delimiter.
    @param[in] justify_functions Determines how are data justified in each column.
        Valid values are function of the form f(str,width)->str such as
        unicode.ljust, unicode.center and unicode.rjust. Default is unicode.ljust.
    @param[in] separateRows True if rows are to be separated by a line
        of 'headerchar's.
    @param[in] prefix A string prepended to each printed row.
    @param[in] postfix A string appended to each printed row.
    @param[in] max_widths Determines the maximum width for each column.
        Words are wrapped to the specified maximum width if greater than 0.
        Wrapping is not done at all when max_width is set to None.
        This is the default.
    @param[in] filler_char a row entry that is FILL_EMPTY_ENTRY will be filled by
        the specified filler character up to the maximum width for
        the related column.
    @param[inout] output a file like object the output will be written to
    @return
    '''
    if not rows:
        return

    if justify_functions is None:
        justify_functions = [unicode.ljust] * len(rows[0])
    _LOGGER.lowest("Justify: %s", justify_functions)
    if max_widths is None:
        max_widths = [0] * len(rows[0])
    _LOGGER.lowest("max_widths: %s", max_widths)

    def i2str(item, maxwidth):
        '''
        Trasform the given row item into a final string.
        @param item
        @param maxwidth
        @return
        @todo to be documented
        '''
        if item is FILL_EMPTY_ENTRY:
            return filler_char * maxwidth
        return item

    # closure for breaking logical rows to physical, using wrapfunc
    def rowWrapper(row):
        '''
        @param row
        @return
        @todo to be documented
        '''
        newrows = [wrap_onspace_strict(item, width).split('\n')
                   for (item, width) in zip(row, max_widths)]
        _LOGGER.lowest("NewRows: %s", newrows)
        if len(newrows) <= 1:
            return newrows
        return [[substr or '' for substr in item] for item in map(None, *newrows)]

    # break each logical row into one or more physical ones
    logicalRows = [rowWrapper(row) for row in rows]
    _LOGGER.lowest("logicalRows: %s", logicalRows)
    # Fetch the list of physical rows
    physicalRows = logicalRows[0]
    for lrow in logicalRows[1:]:
        physicalRows += lrow
    _LOGGER.lowest("physicalRows: %s", physicalRows)
    # columns of physical rows
    if len(physicalRows) == 0:
        return ''
    columns = map(None, *physicalRows)
    _LOGGER.lowest("columns: %s", columns)
    # get the maximum of each column by the string length of its items
    maxWidths = [max([len(item) for item in column]) for column in columns]
    _LOGGER.lowest("MaxWidths: %s", maxWidths)
    rowSeparator = headerchar * (len(prefix) + len(postfix) + sum(maxWidths) +
                                 len(delim) * (len(maxWidths) - 1))

    if separateRows:
        print(rowSeparator, file=output)
    # for physicalRows in logicalRows:
    for row in physicalRows:
        _LOGGER.lowest("row: %s", row)
        row2 = []
        for r in row:
            row2.append(to_unicode(r))
        line = [justify(i2str(item, width),
                        width) for (item,
                                    justify,
                                    width) in zip(row2,
                                                  justify_functions,
                                                  maxWidths)]
        line_uni = [to_unicode(elt) for elt in line]
        line_uni = prefix + delim.join(line_uni) + postfix
        line_uni = to_str_from_unicode(line_uni)

        print(line_uni,
              file=output)
        if separateRows or hasheader:
            print(rowSeparator, file=output)
            hasheader = False


# written by Mike Brown
# http://aspn.activestate.com/ASPN/Cookbook/Python/Recipe/148061
def wrap_onspace(text, width):
    '''
    A word-wrap function that preserves existing line breaks
    and most spaces in the text. Expects that existing line
    breaks are posix newlines (\n).
    @param text
    @param width
    @return
    @todo to be documented
    '''
    return reduce(lambda line, word, width=width: '%s%s%s' %
                  (line,
                   ' \n'[(len(line[line.rfind('\n') + 1:]) +
                          len(word.split('\n', 1)[0]) >= width)],
                   word),
                  text.split(' '))


def wrap_onspace_strict(text, width):
    '''
    Similar to wrap_onspace, but enforces the width constraint:
    words longer than width are split.
    @param text
    @param width
    @return
    @todo to be documented
    '''
    if text is None:
        text = str(None)
    if width == 0:
        return text
    wordregex = re.compile(r'\S{' + str(width) + r',}')
    return wrap_onspace(wordregex.sub(lambda m: wrap_always(m.group(), width),
                                      text),
                        width)


def wrap_always(text, width):
    '''
    A simple word-wrap function that wraps text on exactly width characters.
    It doesn't split the text in words.
    @param text
    @param width
    @return
    @todo to be documented
    '''
    return '\n'.join([text[width * i:width * (i + 1)]
                      for i in xrange(int(math.ceil(1. * len(text) / width)))])


def smart_display(header, data,
                  hsep=u'=', vsep=u' | ',
                  justify=None,
                  columns_max=None,
                  filler_char=u'-',
                  output=sys.stdout):
    '''
    Display an array so each columns are well aligned.

    @param header: the list of column header that should be displayed

    @param data: a list of lines that should be displayed. A line is a
        list of strings. If one element in the line is the FILL_EMPTY_ENTRY
        constant, the 'filler_char' string will fill up the corresponding column.

    @param hsep: the horizontal separator (just after the header)
    @param vsep: the vertical separator (between columns)

    @param justify: a list of alignement justifiers, one for each column.
        Values in the list should be a function f(s,w)->s such as
        unicode.rjust, unicode.ljust and unicode.center. Default is str.ljust.

    @param columns_max: a {column_header: max} dictionary that should be used
        for the display of the related column. If max is 0, it means
        that the column will not be displayed at all. If max is greater
        than 0, then max characters will be used for the display of the column.
        When a string is greater than the 'max', it is wrapped.
        When max=REMOVE_UNSPECIFIED_COLUMNS, then
        only header from columns_max will be displayed.
    @param filler_char
    @param output the file-like object the output must be written to
    @return
    @todo to be documented

    *Warning*: Unicode strings are required.
    '''

    assert header is not None
    assert data is not None
    assert hsep is not None
    assert vsep is not None
    assert len(header) > 0
    if justify is not None:
        assert len(justify) == len(header)
    else:
        justify = [unicode.ljust] * len(header)
    if columns_max is None:
        columns_max = dict()

    def remove_columns(header, columns_max, data):
        '''
        Remove columns with a maximum character number set to 0 and
        columns that are not present in columns_max if one element in
        columns_max maps to REMOVE_UNSPECIFIED_COLUMNS
        @param header
        @param columns_max
        @param data
        @return
        @todo to be documented
        '''
        # We use a copy because we modify the list during the iteration
        given_header = header[:]
        # If we found a columns_max set to REMOVE_UNSPECIFIED_COLUMNS
        # we scan the header table and we set unspecified column_max
        # to 0 so they will be removed in the next step
        for cmax in columns_max.copy():
            if columns_max[cmax] == REMOVE_UNSPECIFIED_COLUMNS:
                for head in given_header:
                    if head not in columns_max:
                        columns_max[head] = 0

        for cmax in columns_max.copy():
            if columns_max[cmax] == REMOVE_UNSPECIFIED_COLUMNS:
                del columns_max[cmax]

        i = 0
        for head in given_header:
            if head in columns_max and columns_max[head] == 0:
                del header[i]
                for line in data:
                    del line[i]
            else:
                # Next column is j+1 only if a removal has not been made.
                i += 1

    remove_columns(header, columns_max, data)
    col_nb = len(header)
    line_nb = len(data)
    max_widths = []
    for head_line in header:
        max_widths.append(columns_max.get(head_line, 0))

    indent([header] + data, hasheader=True,
           headerchar=hsep, delim=vsep,
           separateRows=False,
           max_widths=max_widths,
           filler_char=filler_char,
           justify_functions=justify,
           output=output)


def to_unicode(value, encoding='utf-8'):
    '''
    Returns a unicode object made from the value of the given string.
    @param value
    @param encoding
    @return
    @todo to be documented
    '''
    if isinstance(value, unicode):
        return value
    elif isinstance(value, basestring):
        try:
            value = unicode(value, encoding)
        except UnicodeDecodeError:
            value = value.decode('utf-8', 'replace')
    return value


def to_str_from_unicode(value, encoding='utf-8', should_be_uni=True):
    '''
    Returns a string encoded from the given unicode object
    @param value
    @param encoding
    @param should_be_uni
    @return
    @todo to be documented
    '''
    if isinstance(value, unicode):
        value = value.encode(encoding)
        if not should_be_uni:
            _LOGGER.warning("%s: is unicode-typed, should be a string" % value)
    elif isinstance(value, basestring):
        if should_be_uni:
            _LOGGER.warning("%s: is a string, should be unicode-typed" % value)
    return value
