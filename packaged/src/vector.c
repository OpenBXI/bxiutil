/* -*- coding: utf-8 -*-
 ###############################################################################
 # Author: Jean-Noël Quintin <jean-noel.quintin@bull.net>
 # Created on: August 28, 2014
 # Contributors: Pierre Vignéras <pierre.vigneras@bull.net>
 ###############################################################################
 # Copyright (C) 2013  Bull S. A. S.  -  All rights reserved
 # Bull, Rue Jean Jaures, B.P.68, 78340, Les Clayes-sous-Bois
 # This is not Free or Open Source software.
 # Please contact Bull S. A. S. for details about its license.
 ###############################################################################
 */


#include "bxi/base/log.h"

#include "bxi/util/misc.h"
#include "bxi/util/vector.h"

// *********************************************************************************
// ********************************** Defines **************************************
// *********************************************************************************

#define VECTOR_INIT_SIZE 32

// *********************************************************************************
// ********************************** Types ****************************************
// *********************************************************************************

/*
 * Extendable array implementation as a vector
 */
typedef struct bxivector_s_t {
    size_t total_size;
    size_t used_size;
    void ** array;
} bxivector_s;


// *********************************************************************************
// **************************** Static function declaration ************************
// *********************************************************************************

// *********************************************************************************
// ********************************** Global Variables *****************************
// *********************************************************************************

SET_LOGGER(BXIVECTOR_LOGGER, "bxiutil.vector");

// *********************************************************************************
// ********************************** Implementation   *****************************
// *********************************************************************************

/*
 * Initialize the vector with some elements
 */
bxivector_p bxivector_new(const size_t n, void * elems[n]){
    bxivector_p vector = bximem_calloc(sizeof(*vector));
    vector->total_size = (n < VECTOR_INIT_SIZE) ? VECTOR_INIT_SIZE : 2 * n;
    vector->used_size = 0;
    vector->array = bximem_calloc(vector->total_size*sizeof(*vector->array));

    for (size_t i = 0; i < n; i++) bxivector_push(vector, elems[i]);

    BXIASSERT(BXIVECTOR_LOGGER, vector->used_size == n);

    DEBUG(BXIVECTOR_LOGGER, "New vector %p created: total_size=%zu, used_size=%zu",
          vector,
          vector->total_size,
          vector->used_size);

    return vector;
}

/*
 * Apply the provided function on all remaining elements
 */
bxierr_p bxivector_apply(bxivector_p vector,
                         bxierr_p (*func)(void*,void*),
                         void* const data_cb) {

    BXIASSERT(BXIVECTOR_LOGGER, NULL != vector);
    BXIASSERT(BXIVECTOR_LOGGER, NULL != func);
    size_t n = bxivector_get_size(vector);
    bxierr_p * errors = NULL;
    size_t err_nb = 0;
    for (size_t i = 0; i < n; i++) {
        bxierr_p err = func(bxivector_get_elem(vector, i), data_cb);
        if (bxierr_isko(err)) {
            // First error, allocate the errors array
            if (NULL == errors) {
                errors = bximem_calloc(n*sizeof(*errors));
            }
            errors[i] = err;
            err_nb++;
        }
    }

    return 0 == err_nb ? BXIERR_OK : bxierr_new(BXIVECTOR_APPLY_ERR,
                                                errors,
                                                free,
                                                NULL ,
                                                NULL,
                                                "Errors found in apply(): %zu/%zu",
                                                err_nb, n);
}

/*
 * Free the vector and apply the provided function on all remaining elements
 */
void bxivector_destroy(bxivector_p *vector, void (*free_element)(void**)) {
    if (NULL == vector || NULL == *vector) return;
    if (free_element != NULL){
        size_t n = bxivector_get_size(*vector);
        for (size_t i = 0; i < n; i++) {
            void * elem = bxivector_get_elem(*vector, i);
            free_element(&elem);
        }
    }
    (*vector)->used_size = 0;
    (*vector)->total_size = 0;
    DEBUG(BXIVECTOR_LOGGER, "Vector %p destroyed", vector);
    BXIFREE((*vector)->array);
    BXIFREE(*vector);

    BXIASSERT(BXIVECTOR_LOGGER, NULL == *vector);
}

/*
 * Return the number of element inside the vector
 */
size_t bxivector_get_size(bxivector_p vector) {
    BXIASSERT(BXIVECTOR_LOGGER, NULL != vector);
    return vector->used_size;
}

/*
 * Return the n^th element of the array
 */
void * bxivector_get_elem(bxivector_p vector, size_t n) {
    BXIASSERT(BXIVECTOR_LOGGER, NULL != vector && n < vector->used_size);
    return vector->array[n];
}

/*
 * Add an element to the end of the vector
 */
void bxivector_push(bxivector_p vector, void * elem) {
    BXIASSERT(BXIVECTOR_LOGGER, NULL != vector);
    BXIASSERT(BXIVECTOR_LOGGER, vector->total_size >= vector->used_size);
    BXIASSERT(BXIVECTOR_LOGGER, vector->total_size > 0);

    if (vector->total_size == vector->used_size){
        size_t old_total_size = vector->total_size;
        vector->total_size *= 2;
        vector->array = bximem_realloc(vector->array, old_total_size*sizeof(*vector->array),
                                       (size_t)vector->total_size*sizeof(*vector->array));
        DEBUG(BXIVECTOR_LOGGER,
              "Reallocation for vector %p: old total_size=%zu, new total_size=%zu",
              vector, old_total_size, vector->total_size);
    }
    vector->array[vector->used_size] = elem;
    vector->used_size++;
}

/*
 * Remove the last element
 */
void * bxivector_pop(bxivector_p vector) {
    BXIASSERT(BXIVECTOR_LOGGER, NULL != vector);
    BXIASSERT(BXIVECTOR_LOGGER, 0 < vector->used_size);

    vector->used_size--;
    void * elem = vector->array[vector->used_size];
    vector->array[vector->used_size] = NULL;
    return elem;
}


/*
 * Return the array containing the elements
 *
 * To be kept the array should be copied or the bximisc_vector_p freed manually
 */
void ** bxivector_get_array(bxivector_p vector){
    BXIASSERT(BXIVECTOR_LOGGER, NULL != vector);
    return vector->array;
}
