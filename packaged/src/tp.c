/* -*- coding: utf-8 -*-
 ###############################################################################
 # Author: vigneras
 # Created on: May 21, 2013
 # Contributors:
 ###############################################################################
 # Copyright (C) 2012  Bull S. A. S.  -  All rights reserved
 # Bull, Rue Jean Jaures, B.P.68, 78340, Les Clayes-sous-Bois
 # This is not Free or Open Source software.
 # Please contact Bull S. A. S. for details about its license.
 ###############################################################################
 */

#include <unistd.h>
#include <stddef.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

#include "bxi/base/mem.h"
#include "bxi/base/str.h"
#include "bxi/base/err.h"
#include "bxi/base/log.h"
#include "bxi/util/misc.h"
#include "bxi/util/tp.h"


// *********************************************************************************
// ********************************** Defines **************************************
// *********************************************************************************
#define DIM_MAX 127

struct bxitp_context_s {
    char * line;
    uint32_t line_nb;
    bxitp_topo_cbs_p cbs;
    void * data;
};

// *********************************************************************************
// ********************************** Types ****************************************
// *********************************************************************************

// *********************************************************************************
// **************************** Static function declaration ************************
// *********************************************************************************
static void _call_cb_err(const bxitp_context_p ctx, const bxitp_error_e errcode,
                         const char * const fmt, ...);

static bxitp_error_e _parse_array(const bxitp_context_p context, const char * start,
                                  char * end, uint8_t * const result, uint8_t * const dim);

static bxitp_error_e parse_tuple(bxitp_context_p const context, const char * const start,
                                 uint8_t * const result, uint8_t * const dim);

static bxitp_error_e parse_pgft_param(bxitp_context_p const context,
                                      const char * const param_type,
                                      uint8_t * const result, uint8_t * const dim);
static uint8_t * copy_array(const uint8_t * const src, const uint8_t dim);
static int find_value(const uint8_t dim, const uint8_t array[dim], const uint8_t value);
static bxitp_error_e parse_agnostic(bxitp_context_p const context, bxitp_topo_p const topo);
static bxitp_error_e parse_pgft(bxitp_context_p const context, bxitp_topo_p const topo);
static bxitp_error_e parse_hyperx(const bxitp_context_p context, bxitp_topo_p topo);
static bxitp_error_e parse_torus(const bxitp_context_p context, bxitp_topo_p topo);
static bxitp_error_e parse_string(const bxitp_context_p context, const char ** start,
                                  const char ** result);
static bxitp_error_e parse_asic(const bxitp_context_p context, bxitp_asic_p asic);
static bxitp_error_e parse_port_rank(const bxitp_context_p context, const char * start,
                                     const char ** endptr, uint8_t * result);
static bxitp_error_e parse_key_value(const bxitp_context_p context, const char ** start,
                                     const char * prefix, const char ** result);
static bxitp_error_e parse_port(const bxitp_context_p context, bxitp_port_p port);
static void free_pgft(bxitp_pgft_p pgft);
static bxitp_error_e get_next_line(bxitp_context_p const context, FILE * const file);
static bxitp_error_e parse_line(bxitp_context_p context);

// *********************************************************************************
// ********************************** Global Variables *****************************
// *********************************************************************************

SET_LOGGER(BXITP_LOGGER, "bxiutil.tp");

// *********************************************************************************
// ********************************** Implementation   *****************************
// *********************************************************************************

bxitp_context_p bxitp_new(bxitp_topo_cbs_p cbs, void * data){

    if (cbs == NULL || cbs->topo_cb == NULL
            || cbs->asic_cb == NULL
            || cbs->port_cb == NULL
            || cbs->err_cb == NULL
            || cbs->eof_cb == NULL) {

        errno = EINVAL;
        return NULL ;
    }
    bxitp_context_p context = bximem_calloc(sizeof(*context));
    assert(context != NULL);
    context->cbs = cbs;
    context->data = data;
    return context;
}

void bxitp_free_context(bxitp_context_p context) {
    BXIFREE(context);
}

void bxitp_free_topo(bxitp_topo_p topo) {
    if (topo != NULL ) {
        switch (topo->family) {
        case BXITP_PGFT:
            if (topo->family_specific_data != NULL ) {
                free_pgft((bxitp_pgft_p) topo->family_specific_data);
            }
            break;
        default:
            if (topo->family_specific_data != NULL ) {
                BXIFREE(topo->family_specific_data);
            }
            break;
        }
    }
    BXIFREE(topo);
}

void bxitp_free_asic(bxitp_asic_p asic) {
    if (asic != NULL ) {
        BXIFREE(asic->addr_tuple);
        BXIFREE(asic->id);
    }
    BXIFREE(asic);
}

void bxitp_free_port(bxitp_port_p port) {
    if (port != NULL ) {
        BXIFREE(port->oe_asic_id);
        BXIFREE(port->bw);
        BXIFREE(port->lat);
    }
    BXIFREE(port);
}

bxitp_error_e bxitp_parse(const bxitp_context_p context, FILE * file) {
    if (context == NULL || file == NULL ) {
        errno = EINVAL;
        return BXITP_ABORT;
    }
    bxitp_error_e rc;
    context->line = NULL;
    context->line_nb = 0;
    while (true) {
        // The context holds the next line.
        rc = get_next_line(context, file);
        if (rc != BXITP_OK) break;
        // The context holds the line
        rc = parse_line(context);
        BXIFREE(context->line);
        if (rc != BXITP_OK) break;
    }
    if (rc == BXITP_EOF) {
        rc = context->cbs->eof_cb(context->data, context->line_nb);
    }
    return rc;
}


/**************************************************************************************
 **************************************************************************************
 ******************************* INTERNAL (static) FUNCTIONS **************************
 **************************************************************************************
 **************************************************************************************
 */

/*
 * Call the error callback
 * In: context, error code and printf style format with their arguments.
 *
 */
void _call_cb_err(const bxitp_context_p ctx, const bxitp_error_e errcode,
                         const char * const fmt, ...) {
    va_list argp;
    va_start(argp, fmt);

    char * msg;
    size_t len = bxistr_vnew(&msg, fmt, argp);
    va_end(argp);
    assert(len > 0);
    const bxitp_error_e rc = ctx->cbs->err_cb(errcode, msg, ctx->data, ctx->line,
                                              ctx->line_nb);
    if (rc != 0) {
        BXIEXIT(EX_SOFTWARE,
                bxierr_gen("Callback returned an error (%d) at line %u, exiting.", rc,
                           ctx->line_nb),
                BXITP_LOGGER, BXILOG_CRITICAL);
    }
    BXIFREE(msg);
}

/*
 * Create an array from a string description of the following format: (xx, yy, zz)
 * In: the start and end pointers, a pointer on the result and a pointer on the number of
 * entries found.
 * Out: BXITP_OK or any other error.
 *
 * Note: the given string is modified. The given result should be large enough to
 * hold all dimensions
 * Pre-condition: *start == '(', *end == ')'
 * Post-condition: dim < DIM_MAX
 */
bxitp_error_e _parse_array(const bxitp_context_p context, const char * start,
                          char * end, uint8_t * const result, uint8_t * const dim) {
    assert(start != NULL && end != NULL);
    assert((*start == '(' && *end == ')') || (*start == '[' && *end == ']'));
    assert(result != NULL && dim != NULL);
    // Next character should be the first dimension
    start++;
    // Change last ')' into a ',': (xx, yy, zz) -> xx, yy, zz,
    *end = ',';
    end++;
    // We have reached the end of the array, or the end of the line or
    // the end of the string
    if (*end != ' ' && *end != '\n' && *end != '\0') {
        _call_cb_err(context, BXITP_BAD_FORMAT_ERROR,
                    "Expecting a space after ')' or ']' in %s but read a '%c'",
                    context->line, *end);
        return BXITP_ABORT;
    }
    *end = '\0';
    int tmp_dim = 0;
    for (int i = 0;; i++) {
        // We have reached the end of the string
        if (start >= end) break;
        const char * const comma = strchr(start, ',');
        if (comma == NULL ) {
            _call_cb_err(context, BXITP_BAD_FORMAT_ERROR, "Character ',' expected in %s",
                        context->line);
            return BXITP_ABORT;
        }
        char *endptr;
        errno = 0;
        unsigned long val = strtoul(start, &endptr, 10);

        /* Check for various possible errors */
        if (errno == ERANGE && val == ULONG_MAX) {
            _call_cb_err(context, BXITP_BAD_FORMAT_ERROR,
                        "Can't read specified value in %s", context->line);
            return BXITP_ABORT;
        }
        if (errno == EINVAL && val != 0) {
            _call_cb_err(context, BXITP_BAD_FORMAT_ERROR,
                         "Can't read specified value in %s", context->line);
            return BXITP_ABORT;
        }
        if ((val == 0) && (start == endptr)) {
            // No digit found: [] -> dimension == 0
            *dim = 0;
            result[i] = 0;
            return BXITP_OK;
        }
        /* If we got here, strtoul() successfully parsed a number */
        if (val > UINT8_MAX) {
            _call_cb_err(context, BXITP_BAD_FORMAT_ERROR, "Value too large %lu > %d in %s",
                         val, UINT8_MAX, context->line);
            return BXITP_ABORT;
        }
        result[i] = (uint8_t) val;
        // Next character
        start = comma + 1;
        tmp_dim++;
    }
    *dim = (uint8_t) tmp_dim;
    return BXITP_OK;
}

bxitp_error_e parse_tuple(bxitp_context_p const context, const char * const start,
                          uint8_t * const result, uint8_t * const dim) {
    if (*start != '(' && *start != '[') {
        _call_cb_err(context, BXITP_BAD_FORMAT_ERROR, "Expecting '(' or '[' in %s",
                    context->line);
        return BXITP_ABORT;
    }
    char * end = strchr(start, *start == '(' ? ')' : ']');
    if (end == NULL ) {
        _call_cb_err(context, BXITP_BAD_FORMAT_ERROR, "Expecting ')' or ']' in %s",
                     context->line);
        return BXITP_ABORT;
    }
    // Special case for unidimension: (3,)
    if (*(end - 1) == ',') {
        *end = ' ';
        end--;
        *end = *start == '(' ? ')' : ']';
    }
    const bxitp_error_e rc = _parse_array(context, start, end, result, dim);
    return rc;
}

// Parse the line and fills the given array for the given param_type.
// Line is expected to be of the following form:
// PGFT downlinks=(18, 18) uplinks=(18, 9) interlinks=(1, 2)
// 'line' points to somewhere inside the string
// The 'result' will contain (18, 9) if 'param_type'=="uplinks".
// In that case, '*dim'==2.
// param_type can be "downlinks", "uplinks" or "interlinks".
// Returns the character after the one that was parsed (that is after the ']')
bxitp_error_e parse_pgft_param(bxitp_context_p const context,
                               const char * const param_type,
                               uint8_t * const result, uint8_t * const dim) {
    // Use a working copy
    char * const working_copy = bxistr_new("%s", context->line);
    // Searching for "'param_type'="
    // Starting from line
    char * const param_start = strstr(working_copy, param_type);
    if (param_start == NULL ) {
        BXIFREE(working_copy);
        _call_cb_err(context, BXITP_BAD_FORMAT_ERROR, "Can't find %s in line %s.",
                    param_type, context->line);
        return BXITP_ABORT;
    }
    char * const last = strchr(param_start, '=');
    if (last == NULL ) {
        BXIFREE(working_copy);
        _call_cb_err(context, BXITP_BAD_FORMAT_ERROR, "Expecting '=' after %s in line %s",
                    param_type, context->line);
        return BXITP_ABORT;
    }
    char * const start = last + 1;

    const bxitp_error_e rc = parse_tuple(context, start, result, dim);
    BXIFREE(working_copy);
    return rc;
}

/*
 * Return a new copy of the given array with the given number of dimension.
 * In: array src and number of dimensions
 * Out: the copy
 *
 * Note: the returned copy should be freed using free() by the caller.
 */
uint8_t * copy_array(const uint8_t * const src, const uint8_t dim) {
    const size_t bytes_nb = dim * sizeof(*src);
    uint8_t * const result = malloc(bytes_nb);
    assert(result != NULL);
    // Use memmove() as a memcpy(), memmove is less prone to bug!
    return memmove(result, src, bytes_nb);
}

/*
 * Returns the index 'i' such that array[i] == value or -1 if not was found.
 */
int find_value(const uint8_t dim, const uint8_t array[dim], const uint8_t value) {
    for (int i = 0; i < dim; i++) {
        if (array[i] == value) {
            return i;
        }
    }
    return -1;
}

/*
 * Parse the given FILE as a AGNOSTIC.
 * In: context, topology, line and the file
 * Out: the given topology is filled with the parsed informations.
 *
 * Return: a BXI error code
 */
bxitp_error_e parse_agnostic(bxitp_context_p const context, bxitp_topo_p const topo) {
    assert(topo->family == BXITP_AGNOSTIC);
    topo->family_specific_data = NULL;
    // Invoke the callback
    bxitp_error_e rc = context->cbs->topo_cb(topo, context->data, context->line, context->line_nb);
    return rc;
}

/*
 * Parse the given FILE as a PGFT.
 * In: context, topology, line and the file
 * Out: the given topology is filled with the parsed informations.
 *
 * Return: a BXI error code
 */
bxitp_error_e parse_pgft(bxitp_context_p const context, bxitp_topo_p const topo) {
    assert(topo->family == BXITP_PGFT);
    uint8_t ddim, udim, idim;
    uint8_t downlinks[DIM_MAX], uplinks[DIM_MAX], interlinks[DIM_MAX];
    bxitp_error_e rc = BXITP_OK;
    rc = parse_pgft_param(context, "downlinks", downlinks, &ddim);
    if (rc != BXITP_OK) return rc;
    rc = parse_pgft_param(context, "uplinks", uplinks, &udim);
    if (rc != BXITP_OK) return rc;
    rc = parse_pgft_param(context, "interlinks", interlinks, &idim);
    if (rc != BXITP_OK) return rc;
    if (ddim != udim || ddim != idim || udim != idim) {
        _call_cb_err(context, BXITP_BAD_FORMAT_ERROR, "Dimensions of downlinks (%d), "
                    "uplinks(%d) and interlinks(%d) should be equals",
                    ddim, udim, idim);
        return BXITP_ABORT;
    }
    // Check for zero in each array
    int index = find_value(ddim, downlinks, 0);
    if (index != -1) {
        _call_cb_err(context, BXITP_BAD_FORMAT_ERROR, "Invalid value: downlinks[%d]=%d",
                    index, downlinks[index]);
        return BXITP_ABORT;
    }
    index = find_value(ddim, uplinks, 0);
    if (index != -1) {
        _call_cb_err(context, BXITP_BAD_FORMAT_ERROR, "Invalid value: uplinks[%d]=%d",
                    index, uplinks[index]);
        return BXITP_ABORT;
    }
    index = find_value(ddim, interlinks, 0);
    if (index != -1) {
        _call_cb_err(context, BXITP_BAD_FORMAT_ERROR, "Invalid value: interlinks[%d]=%d",
                    index, interlinks[index]);
        return BXITP_ABORT;
    }
    // Everything is fine, copy array from the stack to the heap.
    bxitp_pgft_p const pgft = bximem_calloc(sizeof(*pgft));
    assert(pgft != NULL);
    pgft->dim = ddim;
    pgft->downlinks = copy_array(downlinks, pgft->dim);
    pgft->uplinks = copy_array(uplinks, pgft->dim);
    pgft->interlinks = copy_array(interlinks, pgft->dim);
    topo->family_specific_data = pgft;
    // Invoke the callback
    rc = context->cbs->topo_cb(topo, context->data, context->line, context->line_nb);
    return rc;
}

bxitp_error_e parse_hyperx(const bxitp_context_p context, bxitp_topo_p topo) {
    assert(topo->family == BXITP_HYPERX);
    _call_cb_err(context, BXITP_UNSUPPORTED, "HyperX topology not supported yet");
    return BXITP_UNSUPPORTED;
}

bxitp_error_e parse_torus(const bxitp_context_p context, bxitp_topo_p topo) {
    assert(topo->family == BXITP_TORUS);
    _call_cb_err(context, BXITP_UNSUPPORTED, "Torus topology not supported yet");
    return BXITP_UNSUPPORTED;
}

static bxitp_error_e parse_string(const bxitp_context_p context, const char ** start,
                                  const char ** result) {
    char * id_start = strchr(*start, '"');
    char * id_end = NULL;
    if (id_start != NULL ) id_end = strchr(id_start + 1, '"');
    if (id_start == NULL || id_end == NULL || id_end == id_start + 1) {
        _call_cb_err(context, BXITP_BAD_FORMAT_ERROR,
                    "Double quote expected around a string in %s", context->line);
        return BXITP_ABORT;
    }
    assert(id_start < id_end);
    id_start++;
    size_t n = (size_t) (id_end - id_start);
    *result = strndup(id_start, n);
    assert(*result != NULL);
    *start = id_end;
    return BXITP_OK;
}

/*
 * Parse an ASIC line
 * In: context, an asic structure and the line
 * Out: the given asic structure is filled with the line informations
 *
 * Return: a bxi error code
 *
 * Note: an ASIC line has the following form:
 *
 *  ASIC 48 "/bxi/divio-115"  (1, 0)
 *
 */
bxitp_error_e parse_asic(const bxitp_context_p context, bxitp_asic_p asic) {
    assert(asic != NULL);
    // Work on a copy
    char * working_copy = strdup(context->line);
    assert(working_copy != NULL);
    // Starts from 5th character, that is the space in 'ASIC '
    char * nptr = working_copy + 5;
    const char * endptr = nptr;

    errno = 0;
    long int radix = strtol(nptr, (char **) &endptr, 0);
    if (errno == ERANGE) {
        BXIFREE(working_copy);
        _call_cb_err(context, BXITP_BAD_FORMAT_ERROR, strerror(errno));
        return BXITP_ABORT;
    }
    if (nptr == endptr) {
        BXIFREE(working_copy);
        _call_cb_err(context, BXITP_BAD_FORMAT_ERROR,
                    "No digits found for radix specification in %s", context->line);
        return BXITP_ABORT;
    }
    if (*endptr != ' ') {
        char badchar = *endptr;
        BXIFREE(working_copy);
        _call_cb_err(context, BXITP_BAD_FORMAT_ERROR,
                    "Bad digit '%c' for radix specification in %s", badchar,
                    context->line);
        return BXITP_ABORT;
    }
    if (radix < 0 || radix > 255) {
        _call_cb_err(context, BXITP_BAD_FORMAT_ERROR,
                    "Radix should be between 0 and 255 in %s", context->line);
        BXIFREE(working_copy);
        return BXITP_ABORT;
    }
    asic->radix = (uint8_t) radix;
    const char * id;
    bxitp_error_e rc = parse_string(context, &endptr, &id);
    if (rc != BXITP_OK) {
        BXIFREE(working_copy);
        return rc;
    }
    asic->id = (char *) id;
    char * addr_start = strchr(endptr + 1, '(');
    char * addr_end = strchr(endptr + 1, ')');
    if (addr_start == NULL || addr_end == NULL ) {
        BXIFREE(working_copy);
        _call_cb_err(context, BXITP_BAD_FORMAT_ERROR,
                    "Missing parenthesis for topology address in %s", context->line);
        return BXITP_ABORT;
    }
    uint8_t addr_dim;
    uint8_t addr_tuple[DIM_MAX];
    rc = parse_tuple(context, addr_start, addr_tuple, &addr_dim);
    if (rc != BXITP_OK) {
        BXIFREE(working_copy);
        return rc;
    }
    asic->addr_dim = addr_dim;
    asic->addr_tuple = copy_array(addr_tuple, addr_dim);
    rc = context->cbs->asic_cb(asic, context->data, context->line, context->line_nb);
    BXIFREE(working_copy);
    return rc;
}

bxitp_error_e parse_port_rank(const bxitp_context_p context, const char * start,
                              const char ** endptr, uint8_t * result) {
    if (*start != '[') {
        _call_cb_err(context, BXITP_BAD_FORMAT_ERROR,
                    "Opening bracket '[' expected for port rank specification in %s",
                    context->line);
        return BXITP_ABORT;
    }
    const char * end = strchr(start + 1, ']');
    if (end == NULL ) {
        _call_cb_err(context, BXITP_BAD_FORMAT_ERROR,
                    "Closing bracket ']' expected for port rank specification in %s",
                    context->line);
        return BXITP_ABORT;
    }

    *endptr = end;
    errno = 0;
    long int rank = strtol(start + 1, (char **) endptr, 0);
    if (errno == ERANGE) {
        _call_cb_err(context, BXITP_BAD_FORMAT_ERROR, strerror(errno));
        return BXITP_ABORT;
    }
    if (*endptr != end) {
        _call_cb_err(context, BXITP_BAD_FORMAT_ERROR,
                    "Invalid character '%c' for port rank specification in %s", **endptr,
                    context->line);
        return BXITP_ABORT;
    }
    if (rank < 0) {
        _call_cb_err(context, BXITP_BAD_FORMAT_ERROR, "Port rank can't be negative in %s",
                    context->line);
        return BXITP_ABORT;
    }
    if (rank > UINT8_MAX) {
        _call_cb_err(context, BXITP_BAD_FORMAT_ERROR,
                    "Port rank can't be greater than %d in %s", UINT8_MAX, context->line);
        return BXITP_ABORT;
    }
    *result = (uint8_t) rank;
    return BXITP_OK;
}

bxitp_error_e parse_key_value(const bxitp_context_p context, const char ** start,
                              const char * prefix, const char ** result) {
    char * found = strstr(*start, prefix);
    if (found == NULL ) {
        _call_cb_err(context, BXITP_BAD_FORMAT_ERROR, "Can't find %s\"...\" in %s", prefix,
                    context->line);
        return BXITP_ABORT;
    }
    *start = found;
    return parse_string(context, start, result);
}

bxitp_error_e parse_port(const bxitp_context_p context, bxitp_port_p port) {
    assert(port != NULL);
    const char * endptr;
    uint8_t rank;
    bxitp_error_e rc = parse_port_rank(context, context->line, &endptr, &rank);
    if (rc != BXITP_OK) return rc;
    port->rank = rank;
    const char * id;
    rc = parse_string(context, &endptr, &id);
    if (rc != BXITP_OK) return rc;
    port->oe_asic_id = (char *) id;
    rc = parse_port_rank(context, endptr + 1, &endptr, &port->oe_rank);
    if (rc != BXITP_OK) return rc;
    rc = parse_key_value(context, &endptr, "bw=", (const char **) &port->bw);
    if (rc != BXITP_OK) return rc;
    rc = parse_key_value(context, &endptr, "lat=", (const char **) &port->lat);
    if (rc != BXITP_OK) return rc;
    char * s_nid = strstr(endptr, "nid=");
    if (s_nid == NULL ) {
        _call_cb_err(context, BXITP_BAD_FORMAT_ERROR, "Can't find nid=... in %s",
                    context->line);
        return BXITP_ABORT;
    }
    char * start_nid = s_nid + 4;
    endptr = start_nid;
    errno = 0;
    long int nid = strtol(start_nid, (char **) &endptr, 0);
    if (errno == ERANGE) {
        _call_cb_err(context, BXITP_BAD_FORMAT_ERROR, strerror(errno));
        return BXITP_ABORT;
    }
    if (endptr == start_nid) {
        _call_cb_err(context, BXITP_BAD_FORMAT_ERROR,
                    "No digits found for port nid specification in %s", context->line);
        return BXITP_ABORT;
    }
    if (*endptr != '\0' && *endptr != ' ') {
        _call_cb_err(context, BXITP_BAD_FORMAT_ERROR,
                    "Invalid character '%c' for port nid specification in %s", *endptr,
                    context->line);
        return BXITP_ABORT;
    }
    if ((nid < 0 || nid > UINT16_MAX)&& nid != BXITP_UNDEFINED_NID) {
        _call_cb_err(context, BXITP_BAD_FORMAT_ERROR,
                    "Bad value for port nid, should be [0..%d] or %d in %s", UINT16_MAX,
                    BXITP_UNDEFINED_NID, context->line);
        return BXITP_ABORT;
    }
    port->nid = (int) nid;
    return context->cbs->port_cb(port, context->data, context->line, context->line_nb);
}

void free_pgft(bxitp_pgft_p pgft) {
    BXIFREE(pgft->downlinks);
    BXIFREE(pgft->uplinks);
    BXIFREE(pgft->interlinks);
    BXIFREE(pgft);
}

/*
 * Get the next line that is not a comment in the given file.
 */
bxitp_error_e get_next_line(bxitp_context_p const context, FILE * const file) {
    assert(context != NULL && file != NULL);
    context->line = NULL;
    char * start = NULL;
    size_t n = 0;
    while (context->line == NULL ) {
        ssize_t read = getline(&context->line, &n, (FILE*) file);
        if (read == -1) {
            if (ferror((FILE *) file)) {
                _call_cb_err(context, BXITP_EXTERNAL_ERROR, "Reading a line failed.");
                return BXITP_ABORT;
            }
            if (feof((FILE *) file)) {
                BXIFREE(context->line);
                return BXITP_EOF;
            }
        }
        context->line_nb++;
        assert(context->line != NULL);
        start = context->line;
        // Remove beginning whitespaces
        while (*start == ' ' || *start == '\t') {
            start++;
        }
        // Remove comments:
        char * comment = strchr(start, '#');
        if (comment != NULL ) {
            // Replace the '#' character by the end of string: '\0'
            *comment = '\0';
        }
        // Skip empty lines and commented out lines
        if (*start == '\n' || *start == '\0') {
            if (feof((FILE *) file)) {
                BXIFREE(context->line);
                return BXITP_EOF;
            }
            // Reinitialize line to NULL so we will read the next line in the buffer.
            BXIFREE(context->line);
            context->line = NULL;
        }
    }
    // Remove trailing '\n'
    for (char * current = start; *current != '\0'; current++) {
        if (*current == '\t') {
            *current = ' ';
        }
        if (*current == '\n') {
            *current = '\0';
        }
    }
    if (start != context->line) {
        char * result = strdup(start);
        BXIFREE(context->line);
        context->line = result;
    }
    return BXITP_OK;
}

/*
 * Parse the given line.
 */
bxitp_error_e parse_line(bxitp_context_p context) {
    const char * last = strchr(context->line, ' ');
    if (last == NULL ) {
        _call_cb_err(context, BXITP_BAD_FORMAT_ERROR, "Expecting space ' ' after token.");
        return BXITP_ABORT;
    }
    size_t length = (size_t) (last - context->line);
    bxitp_error_e rc;
    if (strncasecmp(context->line, "PGFT", length) == 0) {
        bxitp_topo_p topo = bximem_calloc(sizeof(*topo));
        assert(topo != NULL);
        topo->family = BXITP_PGFT;
        rc = parse_pgft(context, topo);
        if (rc != BXITP_OK) bxitp_free_topo(topo);
    } else if (strncasecmp(context->line, "AGNOSTIC", length) == 0) {
        bxitp_topo_p topo = bximem_calloc(sizeof(*topo));
        assert(topo != NULL);
        topo->family = BXITP_AGNOSTIC;
        rc = parse_agnostic(context, topo);
        if (rc != BXITP_OK) bxitp_free_topo(topo);
    } else if (strncasecmp(context->line, "HYPERX", length) == 0) {
        bxitp_topo_p topo = bximem_calloc(sizeof(*topo));
        assert(topo != NULL);
        topo->family = BXITP_HYPERX;
        rc = parse_hyperx(context, topo);
        if (rc != BXITP_OK) bxitp_free_topo(topo);
    } else if (strncasecmp(context->line, "TORUS", length) == 0) {
        bxitp_topo_p topo = bximem_calloc(sizeof(*topo));
        assert(topo != NULL);
        topo->family = BXITP_TORUS;
        rc = parse_torus(context, topo);
        if (rc != BXITP_OK) bxitp_free_topo(topo);
    } else if (strncasecmp(context->line, "ASIC", length) == 0) {
        bxitp_asic_p asic = bximem_calloc(sizeof(*asic));
        assert(asic != NULL);
        rc = parse_asic(context, asic);
        if (rc != BXITP_OK) bxitp_free_asic(asic);
    } else {
        // We have a port specification
        bxitp_port_p port = bximem_calloc(sizeof(*port));
        assert(port != NULL);
        rc = parse_port(context, port);
        if (rc != BXITP_OK) bxitp_free_port(port);
    }
    return rc;
}
