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

/**
 * @file    stretch.h
 * @brief   Extensible array of data structures.
 *
 * This struct has been developed to have an Extensible array
 * but the address element won't changed.
 * The aim is to keep pointer on elements and never changed
 * this even if elements are added.
 *
 * This data structure is formed of one meta array on chunk.
 * This meta array is an internal structure.
 * When an extention is required some chunks are added
 * the meta array is reallocated.
 * However existing chunk aren't changed.
 *
 * Allocation of a stretch array:
 * @snippet bxistretch.c INIT STRETCH
 *
 * Element access:
 * @snippet bxistretch.c GET STRETCH
 *
 * Extention of array:
 * @snippet bxistretch.c HIT STRETCH
 *
 * Liberate the array:
 * @snippet bxistretch.c FREE STRETCH
 *
 */

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
 * @param element_size size of the element to store
 * @param element_nb number of element at the initialization
 *
 * @returns   pointer on the newly allocated stretchable array
 */
bxistretch_p bxistretch_new(size_t chunk_size,
                            size_t element_size,
                            size_t element_nb);

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
void * bxistretch_get(bxistretch_p self, size_t index);

/**
 * Allocate the memory space for all the element with an index lower
 * and equal to the provided one.
 *
 * @param self array on which the element has to be store
 * @param index of the element wanted inside the memory
 * @returns pointer on the element at the index
 *
 */
void * bxistretch_hit(bxistretch_p self, size_t index);

/**
 * @example bxistretch.c
 * An example on how to use the module stretch.
 */

#endif /* !STRETCH_H */
