/* -*- coding: utf-8 -*-
 ###############################################################################
 # Author: Pierre Vigneras <pierre.vigneras@bull.net>
 # Created on: May 24, 2013
 # Contributors:
 ###############################################################################
 # Copyright (C) 2013  Bull S. A. S.  -  All rights reserved
 # Bull, Rue Jean Jaures, B.P.68, 78340, Les Clayes-sous-Bois
 # This is not Free or Open Source software.
 # Please contact Bull S. A. S. for details about its license.
 ###############################################################################
 */

#ifndef BXIVECTOR_H_
#define BXIVECTOR_H_


#include "bxi/base/log.h"


/**
 * @file    vector.h
 * @brief   Growable arrays implementation -- aka vectors
 *
 * ### Overview
 * This module implements growable arrays -- aka vectors.
 */

// *********************************************************************************
// ********************************** Defines **************************************
// *********************************************************************************

/**
 * Error during apply(). err->data is a bxierr_p[]
 */
#define BXIVECTOR_APPLY_ERR 1

// *********************************************************************************
// ********************************** Types   **************************************
// *********************************************************************************

/**
 * The `vector` abtract data type.
 */
typedef struct bxivector_s_t *bxivector_p;

// *********************************************************************************
// ********************************** Global Variables *****************************
// *********************************************************************************

// *********************************************************************************
// ********************************** Interface ************************************
// *********************************************************************************

/**
 * Return a new vector initialized with the given array of elements
 *
 * @param n the number of elements of the given array
 * @param elems the array of elements the vector should be initialized with
 * @return a new vector initialized with the given array of elements
 *
 */
bxivector_p bxivector_new(size_t n, void * elems[n]);

/**
 * Destroy the given vector that is apply the provided function on all remaining
 * elements, free the underlying data structure and nullify the vector pointer.
 *
 * @param self_p a pointer on a vector
 * @param a function called on each element of the vector for releasing resources
 */
void bxivector_destroy(bxivector_p *self_p, void (*free_element)(void**));

/**
 * Return the number of element inside the vector
 *
 * @param self a vector
 * @return the number of element inside the vector
 */
size_t bxivector_get_size(bxivector_p self);

/**
 * Return the n^th element of the array
 *
 * @param self a vector*
 * @return the n^th element of the array
 */
void * bxivector_get_elem(bxivector_p self, size_t n);

/**
 * Add an element to the end of the vector
 *
 * @param self a vector
 * @param elem the element
 */
void bxivector_push(bxivector_p self, void * elem);

/**
 * Remove the last element and return it
 *
 * @param self a vector
 * @return the last element
 */
void * bxivector_pop(bxivector_p self);

/**
 * Return the array containing the elements
 *
 * To be kept the array should be copied or the bximisc_vector_p freed manually
 * WARNING: Every bximisc_vector... call change the array returned.
 *
 * @param self a vector
 */
void ** bxivector_get_array(bxivector_p self);

/**
 * Apply the provided function on all remaining elements
 *
 * @param self a vector
 * @param func a function
 * @param data some data given as the last parameter of `func` for each element
 *        in the given `vector`
 * @return BXIERR_OK, bxierr(code=BXIVECTOR_APPLY_ERR) on error.
 * @see bxierr_p
 *
 */
bxierr_p bxivector_apply(bxivector_p self,
                         bxierr_p (*func)(void* elem, void* data),
                         void* data) ;

#endif /* BXIMISC_H_ */
