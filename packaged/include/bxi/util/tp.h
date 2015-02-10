/* -*- coding: utf-8 -*-
 ###############################################################################
 # Author: Pierre Vigneras <pierre.vigneras@bull.net>
 # Created on: May 15, 2013
 # Contributors:
 ###############################################################################
 # Copyright (C) 2012  Bull S. A. S.  -  All rights reserved
 # Bull, Rue Jean Jaures, B.P.68, 78340, Les Clayes-sous-Bois
 # This is not Free or Open Source software.
 # Please contact Bull S. A. S. for details about its license.
 ###############################################################################
 */

/*
 * BXI Topology Parser.
 *
 * This module implements a parser engine, line-based, for a topology file format such as
 * the following:

 PGFT downlinks=[2] uplinks=[1] interlinks=[3]
 ASIC 48 "/bxi/divio-7"          (0, 0)                                  # "divio-0.0
 [0]     "/bxi/lutetia-1"[0]     bw="Undefined"     lat="Undefined"   nid=2                  # Lutetia "bullx-0.0/0"
 [1]     "/bxi/lutetia-4"[0]     bw="Undefined"     lat="Undefined"   nid=3                  # Lutetia "bullx-0.0/1"
 ASIC 48 "/bxi/divio-64"         (0, 1)                                  # "divio-0.1
 [0]     "/bxi/lutetia-58"[0]    bw="Undefined"     lat="Undefined"   nid=0                  # Lutetia "bullx-0.1/0"
 [1]     "/bxi/lutetia-61"[0]    bw="Undefined"     lat="Undefined"   nid=1                  # Lutetia "bullx-0.1/1"
 [2]     "/bxi/divio-115"[3]     bw="Undefined"     lat="Undefined"   nid=-1                      # (1, 0) "divio-1.0"
 ASIC 48 "/bxi/divio-115"                (1, 0)                                  # "divio-1.0
 [0]     "/bxi/divio-7"[2]       bw="Undefined"     lat="Undefined"   nid=-1                      # (0, 0) "divio-0.0"

 In such a file, a line can be:

 - a topology specification (the first line):
 <TOPOLOGY_FAMILY> <TOPOLOGY_FAMILY_SPECIFIC_PARAMETERS>

 - an ASIC specification:
 ASIC <radix> "<id>" (<topo_addr>)

 - an ASIC port specification:
 [<rank>] "<other_end_asic_id>"[<other_end_rank>] bw=<bandwitdth> lat=<latency> nid=<nid>

 When the engine parses a line it just calls callback handlers, one for each line type,
 that has been registered by the client using bxitp_new().

 The client is therefore responsible for the storing of data (creating a graph for example)
 and the handling of topological error (such as a port connected to a not-specified port
 rank for example.

 On error during the parsing, a specific callback handler is called.

 Each callback handler can also gives back an error code to notify the parser that is should
 abort its parsing due to client-specific missing requirements.
 */

#ifndef BXITP_H_
#define BXITP_H_

#ifndef BXICFFI
#include <stdint.h>
#include <stdio.h>
#endif

#define BXITP_UNDEFINED_NID -1

typedef enum {
    BXITP_OK = 0, BXITP_ABORT = 1, BXITP_INTERNAL_ERROR = 2, BXITP_EXTERNAL_ERROR = 3,
    BXITP_EOF = 4, BXITP_CALLBACK_ERROR = 100, BXITP_BAD_FORMAT_ERROR = 200,
    BXITP_UNSUPPORTED = 201,
} bxitp_error_e;

// The context that holds parser internal data.
typedef struct bxitp_context_s * bxitp_context_p;

typedef enum {
    BXITP_PGFT = 0, BXITP_TORUS = 1, BXITP_HYPERX = 2, BXITP_AGNOSTIC = 3,
} bxitp_topo_family_e;

// Structure that holds the information found in a Topology line such as
// Torus arities=[2, 3] interlinks=2
struct bxitp_topo_s {
    bxitp_topo_family_e family;
    // Abstract parameters for each topology.
    // e.g: can be: bxitp_pgft_s, bxitp_hyperx_s, bxitp_torus_s
    void * family_specific_data;
};

struct bxitp_pgft_s {
    uint8_t dim; // Number of levels
    uint8_t *downlinks, *uplinks, *interlinks; // arrays of size dim.
};

// Structure that holds the information found in an ASIC line such as
// ASIC 48 "/bxi/divio-235"      (1, 1)             # "divio-1.1
struct bxitp_asic_s {
    // ASIC Radix
    uint8_t radix;
    // ASIC Identifier
    char * id;
    // ASIC Topological Address
    uint8_t addr_dim;
    uint8_t * addr_tuple;
};

// Structure that holds the information found in an ASIC line such as
// [7]     "/bxi/divio-121"[7]     bw=Undefined     lat=Undefined   nid=NotApplicable
struct bxitp_port_s {
    uint8_t rank;
    // Other end asic id
    char * oe_asic_id;
    // Other end port rank
    uint8_t oe_rank;
    // Port/Link Bandwidth (in Bytes/s)
    char * bw;
    // Port/Link Latency (in nanoseconds)
    char * lat;
    // Port NID if applicable
    int nid;
};

typedef struct bxitp_topo_s * bxitp_topo_p;
typedef struct bxitp_pgft_s * bxitp_pgft_p;
typedef struct bxitp_asic_s * bxitp_asic_p;
typedef struct bxitp_port_s * bxitp_port_p;


/*
 * Callbacks called each time a new BXI topology is discovered during parsing.
 */

typedef struct bxitp_topo_cbs_s_t{
    /*
     * This function is called for each line containing a topology description.
     *
     * In: the parsed topology, your data, the corresponding line and line number.
     *
     * Out: if this callback returns BXITP_OK, the parser will continue parsing the file.
     * Otherwise, the parser will stop and the bxitp_parse() function will return an
     * error code.
     *
     * Note: the passed topology will have to be freed using bxitp_free_topo().
     */
    bxitp_error_e (*topo_cb)(bxitp_topo_p topo, void * data, const char * line,
                                     uint32_t line_nb);


    /*
     * This function is called for each line containing an asic description.
     *
     * In: the parsed asic, your data, the corresponding line and line number.
     *
     * Out: if this callback returns BXITP_OK, the parser will continue parsing the file.
     * Otherwise, the parser will stop and the bxitp_parse() function will return an
     * error code.
     *
     * Note: the passed asic will have to be freed using bxitp_free_asic().
     * Note: the parser make no assumption nor relationship between an asic and a topology.
     * It is the caller responsibility to check whether the given asic is relevant or not
     * according to its internal state. The 'data' pointer can be used to get that internal
     * status: it can point to a data structure holding various informations such as the last
     * topology currently parsed.
     */
    bxitp_error_e (*asic_cb)(bxitp_asic_p asic, void * data, const char * line,
                                     uint32_t line_nb);

    /*
     * This function is called for each line containing a port description.
     *
     * In: the parsed port, your data, the corresponding line and line number.
     *
     * Out: if this callback returns BXITP_OK, the parser will continue parsing the file.
     * Otherwise, the parser will stop and the bxitp_parse() function will return an
     * error code.
     *
     * Note: the passed port will have to be freed using bxitp_free_port().
     * Note: the parser make no assumption nor relationship between a port and an asic.
     * It is the caller responsibility to check whether the given port is relevant or not
     * according to its internal state. The 'data' pointer can be used to get that internal
     * status: it can point to a data structure holding various informations such as the last
     * asic currently parsed and link the passed port to that asic if required.
     */
    bxitp_error_e (*port_cb)(bxitp_port_p port, void * data, const char * line,
                                     uint32_t line_nb);

    /*
     * When an error occurs during the parsing, this callback is called.
     * In: a bxiparser specific error code as defined in this header file,
     *     the corresponding error message, and the data as given in the bxitp_new function.
     * Out: 0 if OK, anything else means KO and will produce a error message in the parser
     *      engine itself.
     *
     * WARNING: the given errmsg will be freed immediatly after this callback returned. Therefore
     * if this error message is to be used at a later stage, it should be copied.
     */
    bxitp_error_e (*err_cb)(bxitp_error_e error, const char * errmsg, void * data,
                                    const char * line, uint32_t line_nb);

    /*
     * When the end of file is reached while parsing, this callback is called.
     * In: The data as given in the bxitp_new function and the total line numbers.
     *
     * Out: 0 if OK, anything else means KO. This code will be returned by the
     * bxitp_parser() function (unless error occurred before).
     */
    bxitp_error_e (*eof_cb)(void * data, uint32_t line_nb);
} bxitp_topo_cbs_s;

typedef bxitp_topo_cbs_s* bxitp_topo_cbs_p;

/*
 * Return a new context for the parsing of BXI topologies.
 * In: callbacks function, and some data that will be passed to callback
 *     functions each time one is called.
 * Out: a BXI topology parser context, or NULL if an error occured.
 *
 * Note: the returned context should be freed by the caller using bxitp_free()
 */
bxitp_context_p bxitp_new(bxitp_topo_cbs_p cbs, void * data);

/*
 * Free the given context and all their resources;
 */
void bxitp_free_context(bxitp_context_p context);

/*
 * Free the given topology and all their resources
 */
void bxitp_free_topo(bxitp_topo_p topo);

/*
 * Free the given asic and all their resources
 */
void bxitp_free_asic(bxitp_asic_p asic);

/*
 * Free the given asic and all their resources
 */
void bxitp_free_port(bxitp_port_p port);

/*
 * Parse the given file.
 * In: a context created by bxitp_new and the file to parse
 * Out: a BXI error code.
 *
 * Note: All objects created by this function are passed to callbacks.
 * The caller is responsible therefore to free them using the different bxitp_free_*()
 * functions.
 */
bxitp_error_e bxitp_parse(bxitp_context_p context, FILE* file);

#endif /* BXITP_H_ */
