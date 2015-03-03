/* -*- coding: utf-8 -*-
###############################################################################
# Author:  Quintin Jean-Noël <jean-noel.quintin@bull.net>
# Created on: 2015-02-26
# Contributors:
###############################################################################
# Copyright (C) 2015  Bull S. A. S.  -  All rights reserved
# Bull, Rue Jean Jaures, B.P.68, 78340, Les Clayes-sous-Bois
# This is not Free or Open Source software.
# Please contact Bull S. A. S. for details about its license.
###############################################################################
*/

#ifndef BXIUTIL_STRETCH_H
#define BXIUTIL_STRETCH_H


#ifndef BXICFFI
#include <stdint.h>
#include <stdlib.h>
#endif


// *********************************************************************************
// ********************************** Defines **************************************
// *********************************************************************************


#define BXISTRETCH_ARRAY_SIZE 64
#define BXISTRETCH_DEFAULT_CHUNK_SIZE 1024

// *********************************************************************************
// ********************************** Types   **************************************
// *********************************************************************************

typedef struct bxistretch_s_t *bxistretch_p;

// *********************************************************************************
// ********************************** Global Variables *****************************
// *********************************************************************************


// *********************************************************************************
// ********************************** Interface ************************************
// *********************************************************************************

/**
 * Allocate a stretchable for elements of the same size.
 *
 * If the chunk_size is 0, a default size is selected.
 *
 * @param chunk_size the overhead of allocated element
 * @param element_size sie of the element to store
 * @param element_nb number of element at the initialization
 *
 * @returns   pointer on the newly allocated stretchable array
 */
bxistretch_p bxistretch_new(uint16_t chunk_size, uint32_t  element_size, uint32_t element_nb);

/**
 * Destroy a stretchable array.
 * 
 *
 * @param stretch_p pointer on the object to be destroy
 */
void bxistretch_destroy(bxistretch_p *stretch_p);

/**
 * Return the element at the index requested.
 *
 * @param self array on which the element has been store
 * @param index index of the wanted element
 *
 * @returns  pointer on the element
 */
void * bxistretch_get(bxistretch_p self, uint32_t index);

/**
 * Allocate the memory space for all the element with an index lower and equal to the provided one.
 *
 * @param self array on which the element has to be store
 * @param index of the element wanted inside the memory
 * @returns pointer on the element at the index
 *
 */
void * bxistretch_hit(bxistretch_p self, uint32_t index);

#endif /* !STRETCH_H */
