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

#ifndef BXIRNG_H_
#define BXIRNG_H_


// *********************************************************************************
// ********************************** Types   **************************************
// *********************************************************************************

typedef struct bxirng_s bxirng_s;
typedef struct bxirng_s * bxirng_p;

// *********************************************************************************
// ********************************** Global Variables *****************************
// *********************************************************************************

// *********************************************************************************
// ********************************** Interface ************************************
// *********************************************************************************

/*
 * Return a new random seed from the underlying operating system
 * random number generator (/dev/urandom). Note: this is slow.
 */
uint32_t bxirng_new_seed();

/*
 * Return a new random number generator initialized with the given seed.
 */
bxirng_p bxirng_new(uint32_t seed);


/*
 * Free all allocated ressources and nullify the given pointer.
 */
void bxirng_destroy(bxirng_p * rng_p);

/*
 * Return a number in the interval
 * [start, end]. Note that 'start' and 'end' are included.
 */
uint32_t bxirng_nextint(bxirng_p self, uint32_t start, uint32_t end);

/*
 * Return the given number of random bytes.
 */
void bxirng_bytes(bxirng_p self, size_t k, uint8_t bytes[k]);


/*
 * Select p distinct random numbers between 0 and p exclusive
 * Note that if n <= p, it is not possible to guarantee that
 * all p choices are distinct.
 */
//void bxirng_select(thread_ctx_p, uint8_t n, uint8_t p, uint8_t result[p]);


/*
 * Convenience function that uses thread-specific data (tsd) holding
 * a bxirng_p instance under the cover
 * to ensure thread safety. This is useful when you want to get rid of
 * an extra bxirng_p instance parameter in your function signature.
 *
 * Note however, that this incurs a call to pthread_getspecific()
 * which can be costly in some implementations.
 *
 * If you can get rid of it, do it:
 * just pass a bxirng_p instance in parameter of your functions.
 */
uint32_t bxirng_nextint_tsd(uint32_t start, uint32_t end);


/*
 * Return an array of bxirng_p instances initialized from the
 * given seed.
 *
 * This is usefull for the creation of thread specific rngs:
 *
 * bxirng_s rngs[threads_nb];
 * bxirng_news(bxirng_new_seed(), threads_nb, &rngs);
 *
 * Then the rngs array can be given to each thread.
 * And each thread can therefore do the following:
 *
 * bxirng_p rng = &rngs[thread_rank];
 *
 * Each thread therefore holds it own random generator. This is thread safe.
 * If the initial seed (given to bxirng_rngs) is given twice,
 * the whole set of pseudo random numbers generated can be reproduced.
 *
 * Note: an internal bxirng_p instance is created and destroyed at the end.
 */
void bxirng_new_rngs(uint32_t seed, size_t n, bxirng_s ** rngs_p);

#endif /* BXIMISC_H_ */
