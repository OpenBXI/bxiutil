# -*- coding: utf-8 -*-
###############################################################################
# Author: Sébastien Miquée <sebastien.miquee@atos.net>
# Contributors:
###############################################################################
# Copyright (C) 2014 - 2015  Bull S. A. S.  -  All rights reserved
# Bull, Rue Jean Jaures, B.P.68, 78340, Les Clayes-sous-Bois
# This is not Free or Open Source software.
# Please contact Bull S. A. S. for details about its license.
###############################################################################


"""Simple NodeSet tools module"""


###############################################################################
def generate_minus(value):
    """Numbers generation from a 'start-end' notation."""
    ret = list()
    try:
        start, end = value.split('-')
        inc = 1

        try:
            end, inc = end.split('/')
        except ValueError:
            pass

        start = int(start)
        end = int(end)
        inc = int(inc)

        if start > end:
            tmp = start
            start = end
            end = tmp

        ret = range(start, end + 1, inc)
    except ValueError:
        try:
            ret.append(int(value))
        except ValueError:
            pass

    return ret


###############################################################################
def generate_complex(value):
    """Numbers generation from a 'prefix[start - end]suffix' notation."""
    ret = list()
    try:
        fix, var = value.split('[')
        var, end = var.rsplit(']')

        variables = list(str(elt) for elt in expander(var))

        ret = list(int(fix + elt + end) for elt in variables)
    except ValueError:
        pass
    return ret


###############################################################################
def generate(value):
    """Select the good numbers generator."""
    if '[' in value:
        if ']' not in value:
            return list()
        return generate_complex(value)
    if '-' in value:
        return generate_minus(value)
    try:
        ret = list()
        ret.append(int(value))
        return ret
    except ValueError:
        pass
    return list()


###############################################################################
def parse(line):
    """Parse the line."""
    buff = ''
    elts = line.split(',')
    for elt in elts:
        buff += elt
        if '[' not in buff:
            yield buff
            buff = ""
        else:
            if ']' in buff:
                yield buff
                buff = ''
            else:
                buff += ','


###############################################################################
def expander(line):
    """Expand the given nodeset line and return the full list."""
    final = list()
    for elt in parse(line):
        final.extend(generate(elt.strip()))

    final = list(set(final))
    final.sort(key=int)
    return final


###############################################################################
def sets_split(line):
    """Split different sets from the command line"""

    sets = list()
    tmp_sets = line.split("],")

    for i in xrange(len(tmp_sets)):
        value = tmp_sets[i]
        if i != len(tmp_sets) - 1:
            value += ']'

        try:
            int(tmp_sets[i + 1])
            value += tmp_sets[i + 1]
            i += 1
        except ValueError:
            pass
        except IndexError:
            pass

        sets.append(value)

    return sets


###############################################################################
def global_expander(line):
    """Expand the given nodeset line and return the full list."""

    result = list()

    sets = sets_split(line)

    for value in sets:
        prefix = ""
        suffix = ""
        var = ""
        zeros = 0
        try:
            prefix, var = value.split('[', 1)
            var, suffix = var.rsplit(']', 1)
        except ValueError:
            return [value]

        inter = expander(var)
        if var[0] == '0':
            zeros = len(var.split(',')[0].split('-')[0].split('[')[0])

        result.extend(["%s%s%s" % (prefix,
                                   ("%d" % elt).zfill(zeros),
                                   suffix)
                       for elt in inter])

    return result


###############################################################################
def reducer(line):
    """Create the nodeset list from a number list."""
    # To be implemented ...
    return list()
    # First, expand it to be sure to have a good basis
    # Then, reduce it ...


###############################################################################
def global_reducer(line):
    """Create the nodeset list from a number list."""
    return ["'Reduce' function not implemented for now!"]


###############################################################################
def compact(l):
    return compact_to_str(find_sets(l))


def find_sets(l):
    glob = list()
    f = list()
    for elt in l:
        try:
            if f[-1] + 1 == elt:
                f.append(elt)
            else:
                glob.append(f)
                f = list()
        except IndexError:
            pass
        if len(f) == 0:
            f.append(elt)
    glob.append(f)
    return glob


def compact_to_str(glob):
    return ",".join([list_to_str(l) for l in glob])


def list_to_str(l):
    if len(l) > 2:
        return "%d-%d" % (l[0], l[-1])
    else:
        return ",".join([str(x) for x in l])
