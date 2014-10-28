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

#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

#include "bxi/base/log.h"
#include "bxi/util/misc.h"

#include "bxi/util/rng.h"

// *********************************************************************************
// ********************************** Defines **************************************
// *********************************************************************************
#define RANDOM_GEN_FILE "/dev/urandom"

// *********************************************************************************
// ********************************** Types ****************************************
// *********************************************************************************

struct bxirng_s {
    uint32_t seed;
};

// *********************************************************************************
// **************************** Static function declaration ************************
// *********************************************************************************
static void _init_rng(bxirng_p self, uint32_t seed);
static void _rng_key_maker();
static void _destroy_key();
// *********************************************************************************
// ********************************** Global Variables *****************************
// *********************************************************************************
SET_LOGGER(BXIRNG_LOGGER, "bxiutil.rng");

static pthread_key_t RND_KEY;
static pthread_once_t RND_KEY_ONCE = PTHREAD_ONCE_INIT;


// *********************************************************************************
// ********************************** Implementation   *****************************
// *********************************************************************************

uint32_t bxirng_new_seed() {
    errno = 0;
    int fd = open(RANDOM_GEN_FILE, O_RDONLY);
    if (-1 == fd) {
        BXIEXIT(EX_OSFILE,
                bxierr_errno("Can't open '%s'", RANDOM_GEN_FILE),
                BXIRNG_LOGGER, BXILOG_CRITICAL);
    }
    uint32_t seed;
    errno = 0;
    TRACE(BXIRNG_LOGGER, "Reading %zu bytes from '%s'", sizeof(seed), RANDOM_GEN_FILE);
    ssize_t result = read(fd, &seed, sizeof(seed));
    if (0 > result) {
        BXIEXIT(EX_IOERR,
                bxierr_errno("Can't read %zu bytes from '%s'", sizeof(seed), RANDOM_GEN_FILE),
                BXIRNG_LOGGER, BXILOG_CRITICAL);
    }
    int rc = close(fd);
    if (-1 == rc) {
        WARNING(BXIRNG_LOGGER, "Can't close() '%s'", RANDOM_GEN_FILE);
    }
    return seed;
}



/*
 * Return a new random number generator.
 */
bxirng_p bxirng_new(uint32_t seed) {
    bxirng_p self = bximem_calloc(sizeof(*self));
    _init_rng(self, seed);

    return self;
}

/*
 * Free all allocated ressources and nullify the given pointer.
 */
void bxirng_destroy(bxirng_p * rng_p) {
    BXIFREE(*rng_p);
}

/*
 * Thread safe random generator. Return a number in the interval
 * [start, end]. Note that 'start' and 'end' are included.
 */
uint32_t bxirng_nextint(bxirng_p self, uint32_t start, uint32_t end) {
    BXIASSERT(BXIRNG_LOGGER, start < end);
    uint32_t n = end - start;
    uint32_t x;
    // Prevent the modulo bias
    do {
        x = (uint32_t) rand_r(&self->seed);
    } while(x >= RAND_MAX - n && x >= n);

    return (uint32_t) (x % n + start);
}

uint32_t bxirng_nextint_tsd(uint32_t start, uint32_t end) {
    pthread_once(&RND_KEY_ONCE, _rng_key_maker);
    bxirng_p rng = pthread_getspecific(RND_KEY);
    if (rng == NULL) {
        rng = bxirng_new(bxirng_new_seed());
        int rc = pthread_setspecific(RND_KEY, rng);
        if (0 != rc) {
            BXIEXIT(EX_OSERR,
                    bxierr_fromidx(rc, NULL, "Calling pthread_setspecific() failed"),
                    BXIRNG_LOGGER, BXILOG_CRITICAL);
        }
        TRACE(BXIRNG_LOGGER, "Allocation of a new TSD rng: %p", rng);
    }
    return bxirng_nextint(rng, start, end);
}

void bxirng_new_rngs(uint32_t seed, size_t n, bxirng_s ** rngs_p) {
    BXIASSERT(BXIRNG_LOGGER, NULL != rngs_p && 0 < n);
    if (NULL == *rngs_p) {
        *rngs_p = bximem_calloc(n * sizeof(**rngs_p));
    }
    bxirng_p rng = bxirng_new(seed);
    bxirng_s * rngs = *rngs_p;
    for (size_t i = 0; i < n; i++) {
        uint32_t seed = bxirng_nextint(rng, 0, RAND_MAX);
        _init_rng(rngs + i, seed);
    }
    bxirng_destroy(&rng);
}

// *********************************************************************************
// ********************************** Static Functions  ****************************
// *********************************************************************************

void _init_rng(bxirng_p self, uint32_t seed) {
    TRACE(BXIRNG_LOGGER, "Setting seed: %u for rng: %p", seed, self);
    self->seed = seed;
}

void _destroy_key() {
    bxirng_p rng = (bxirng_p) pthread_getspecific(RND_KEY);
    bxirng_destroy(&rng);
    pthread_key_delete(RND_KEY);
}

void _rng_key_maker() {
    int rc = pthread_key_create(&RND_KEY, free);
    if (rc != 0) {
        BXIEXIT(EX_OSERR,
                bxierr_fromidx(rc, NULL, "Can't call pthread_create()"),
                BXIRNG_LOGGER, BXILOG_CRITICAL);
    }
    errno = 0;
    rc = atexit(_destroy_key);
    if (rc != 0) {
        BXIEXIT(EX_OSERR,
                bxierr_fromidx(rc, NULL,"Can't call atexit()"),
                BXIRNG_LOGGER, BXILOG_CRITICAL);

    }
}
