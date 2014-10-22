/* -*- coding: utf-8 -*-
 ###############################################################################
 # Author: Jean-Noel Quintin <jean-noel.quintin@bull.net>
 # Created on: Nov 29, 2013
 # Contributors:
 ###############################################################################
 # Copyright (C) 2012  Bull S. A. S.  -  All rights reserved
 # Bull, Rue Jean Jaures, B.P.68, 78340, Les Clayes-sous-Bois
 # This is not Free or Open Source software.
 # Please contact Bull S. A. S. for details about its license.
 ###############################################################################
 */

#ifndef BXIMAP_H_
#define BXIMAP_H_

#include <stddef.h>
#include <bxi/base/err.h>



/**
 * @file    map.h
 * @brief   Independent tasks parallel execution framework
 *
 * ### Overview
 * This module implements a framework for the parallel execution of independent tasks
 */

// *********************************************************************************
// ********************************** Define  **************************************
// *********************************************************************************

// *********************************************************************************
// ********************************** Types   **************************************
// *********************************************************************************
/**
 * The bximap context abstract data type
 */
typedef struct bximap_ctx_s_t * bximap_ctx_p;

// *********************************************************************************
// ********************************** Global Variables *****************************
// *********************************************************************************
/**
 * The error returned when the module has not been initialized.
 */
extern const bxierr_p BXIMAP_NOT_INITIALIZED;


/**
 * The error returned when the module has already been initialized.
 */
extern const bxierr_p BXIMAP_INITIALIZE;

/**
 * The error returned when a NULL context is provided as an argument.
 */
extern const bxierr_p BXIMAP_NO_CONTEXT;

/**
 * The error returned when the module is already running.
 */
extern const bxierr_p BXIMAP_RUNNING;

/**
 * The error returned when some arguments are invalid.
 */
extern const bxierr_p BXIMAP_ARG_ERROR;

// *********************************************************************************
// ********************************** Interface ************************************
// *********************************************************************************


/**
 * Initialize the bximap library.
 *
 *
 * if nb_threads == NULL or *nb_threads is equal to 0
 *      the BXIMAP_NB_THREADS environment variable is used
 * if BXIMAP_NB_THREADS isn't defined or is equal to 0
 *          the number of physical processors is used.
 *
 * If nb_threads != NULL && *nb_threads == 0, the number of threads used
 * is written back into *nb_threads.
 *
 * This function should be called before bximap_task
 *
 * @param nb_threads a pointer on the number of threads to use (can be NULL)
 * @return BXIERR_OK on error, anything else on error
 *
 */
bxierr_p bximap_init(size_t * nb_threads);

/**
 * Clean all resources allocated by the library
 *
 * @return BXIERR_OK on error, anything else on error
 */
bxierr_p bximap_finalize();

/**
 * Create a new mapping
 * Map the iteration from start to end over the threads
 * each task has grain iterations to do
 * if the number of iterations isn't proportional to the grain:
 *      if the number of tasks is proportional to the number of threads
 *          the remaining work is split between the task
 *      else
 *          one task is added with the remaining work.
 *  if gain is equal to 0 the optimal size is used
 *
 * @param start the first iteration to start the computation with
 * @param end the last iteration to end the computation with
 * @param granularity the number of iteration each thread must do
 * @param func the function to use by each thread
 * @param usr_data the data to pass to the function
 * @param ctx_p a pointer on the created context
 *
 * @return BXIERR_OK on success, anything else on error
 */
bxierr_p bximap_new(size_t start,
                    size_t end,
                    size_t granularity,
                    bxierr_p (*func)(size_t start, size_t end, size_t thread, void *usr_data),
                    void * usr_data,
                    bximap_ctx_p * ctx_p);

/**
 * Release the given bximap context.
 *
 * @param ctx_p a pointer on the bximap context to destroy
 */
bxierr_p bximap_destroy(bximap_ctx_p *ctx_p);

/**
 * Execute the work describe by the context over the initialized threads
 *
 *
 *  WARNING bximap_init_threads should be called before
 *
 *  @param context the bximap context to use
 */
bxierr_p bximap_execute(bximap_ctx_p context);

/**
 * Return the error and the number of error
 *
 * @param context the bximap context to use
 * @param n a pointer on the number of errors
 * @param err_p a pointer on an array of errors.
 *
 * @return BXIERR_OK on success, anything else on error.
 */
bxierr_p bximap_get_error(bximap_ctx_p context, size_t *n, bxierr_p **err_p);

#endif /* BXILOOP_H_ */
