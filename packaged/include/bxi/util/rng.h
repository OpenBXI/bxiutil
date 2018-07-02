/* -*- coding: utf-8 -*-
 ###############################################################################
 # Author: Pierre Vigneras <pierre.vigneras@bull.net>
 # Created on: May 24, 2013
 # Contributors:
 ###############################################################################
 # Copyright (C) 2018 Bull S.A.S.  -  All rights reserved
 # Bull, Rue Jean Jaures, B.P. 68, 78340 Les Clayes-sous-Bois
 # This is not Free or Open Source software.
 # Please contact Bull S. A. S. for details about its license.
 ###############################################################################
 */

#ifndef BXIRNG_H_
#define BXIRNG_H_


/**
 * @file    rng.h
 * @brief   Random Number Generator Module
 *
 * ### Overview
 * This module implements a thread-safe pseudo-random number generator with the
 * following features:
 *
 * - easy to use: see `bxirng_nextint_tsd()`
 * - efficient: use `bxirng_new_seed()`, `bxirng_new()`, `bxirng_nextint()`
 *              and `bxirng_destroy()`
 * - reproducible: see `bxirng_new_rngs()`
 *
 * Note: Though the implementation uses a modulo, the well-known
 * [modulo bias](https://www.google.fr/search?q=random+modulo+bias)
 * is completely removed using an
 * [efficient trick](https://stackoverflow.com/questions/10984974/why-do-people-say-there-is-modulo-bias-when-using-a-random-number-generator)
 * that in the worst case incurs
 * a double call to the underlying rand() function.
 *
 */
// *********************************************************************************
// ********************************** Types   **************************************
// *********************************************************************************

/**
 * An alias on the underlying data-structure.
 */
typedef struct bxirng_s bxirng_s;

/**
 * The pseudo-random number generator abstract data type.
 */
typedef struct bxirng_s * bxirng_p;

// *********************************************************************************
// ********************************** Global Variables *****************************
// *********************************************************************************

// *********************************************************************************
// ********************************** Interface ************************************
// *********************************************************************************

/**
 * Return a new random seed from the underlying operating system
 * random number generator (`/dev/urandom`).
 *
 * Note: this is slow, use it for initialization purpose.
 *
 * @return a new seed
 *
 */
uint32_t bxirng_new_seed();

/**
 * Return a new random number generator initialized with the given seed.
 *
 * @param seed the seed to use internally.
 * @return a new instance
 */
bxirng_p bxirng_new(uint32_t seed);


/**
 * Free all allocated resources and nullify the given pointer.
 *
 * @param self_p a pointer on the instance to destroy
 */
void bxirng_destroy(bxirng_p * self_p);

/**
 * Return a number in the interval
 * [`start`, `end`].
 *
 * Note that `start` and `end` are included.
 *
 * @param self the instance to use
 * @param start the start interval
 * @param end the end interval
 *
 * @return a number in the interval [`start`, `end`[
 *
 * @see bxirng_nextint_tsd()
 */
uint32_t bxirng_nextint(bxirng_p self, uint32_t start, uint32_t end);

/**
 * Return the given number of random bytes.
 *
 * \unimplemented this function is not implemented yet
 *
 * @param self the instance to use
 * @param k the number of bytes
 * @param bytes the array to fill
 */
void bxirng_bytes(bxirng_p self, size_t k, uint8_t *bytes);


/**
 * Select p distinct random numbers between 0 and n exclusive.
 *
 * Note that if n <= p, it is not possible to guarantee that
 * all p choices are distinct.
 *
 * \unimplemented this function is not implemented yet
 *
 * @param self the instance to use
 * @param n the maximum value for the random number chosen
 * @param p the number of random value to select
 * @param result the array of value to fill
 */
void bxirng_select(bxirng_p self, uint8_t n, uint8_t p, uint8_t *result);


/**
 * Convenience (slow) function for getting a random number in the
 * interval [`start`, `end`].
 *
 * Note: this function uses a thread-specific data (tsd) holding
 * a bxirng_p instance under the cover to ensure thread safety.
 *
 * This is useful when you want to get rid of
 * an extra bxirng_p instance parameter in your function signature.
 *
 * Note however, that this incurs a call to pthread_getspecific()
 * which can be costly in some implementations.
 *
 * If you can get rid of it, do it:
 * just pass a bxirng_p instance in parameter of your functions.
 *
 * @param start the beginning of the interval
 * @param end the end of the interval
 *
 * @return a pseudo-random number in the interval [`start`, `end`[
 *
 * @see bxirng_nextint()
 */
uint32_t bxirng_nextint_tsd(uint32_t start, uint32_t end);


/**
 * Return an array of `bxirng_p` instances initialized from the
 * given `seed`.
 *
 * This is useful for the creation of thread specific rngs as shown in the
 * following example:
 *
 *      bxirng_s rngs[threads_nb];
 *      bxirng_news(bxirng_new_seed(), threads_nb, &rngs);
 *
 * The rngs array can be given to each thread.
 * And each thread can therefore do the following:
 *
 *      bxirng_p rng = &rngs[thread_rank];
 *
 * Each thread therefore holds it own random generator. This is thread safe.
 *
 * If the initial seed (given to `bxirng_new_rngs()`) is given twice,
 * the whole set of pseudo random numbers generated can be reproduced.
 * This is useful to implement reproducibility in a multi-threaded application
 * that use pseudo-random in its threads.
 *
 * Note: an internal bxirng_p instance is created and destroyed at the end.
 *
 * @param seed the initial seed to use
 * @param n the number of pseudo-random generators to produce
 * @param rngs_p a pointer on the array of pseudo-random number generators
 *               that should be filled with the result
 */
void bxirng_new_rngs(uint32_t seed, size_t n, bxirng_s ** rngs_p);

#endif /* BXIMISC_H_ */
