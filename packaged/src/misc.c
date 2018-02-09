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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <limits.h>
#include <execinfo.h>
#include <pthread.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <libgen.h>
#include <sysexits.h>


#ifdef __linux__
#include <syscall.h>
#include <sys/syscall.h>
#include <sys/signalfd.h>
#endif

#include "bxi/base/str.h"
#include "bxi/base/log.h"
#include "bxi/util/misc.h"

// *********************************************************************************
// ********************************** Defines **************************************
// *********************************************************************************

#define VECTOR_INIT_SIZE 32



// *********************************************************************************
// ********************************** Types ****************************************
// *********************************************************************************

// *********************************************************************************
// **************************** Static function declaration ************************
// *********************************************************************************

static bxierr_p _create_writable_file(const char * filename, size_t size, int *fd);

// *********************************************************************************
// ********************************** Global Variables *****************************
// *********************************************************************************
SET_LOGGER(BXIMISC_LOGGER, BXILOG_LIB_PREFIX "bxiutil.misc");
// *********************************************************************************
// ********************************** Implementation   *****************************
// *********************************************************************************

bxierr_p bximisc_get_filename(FILE * const stream, char ** filename) {
    BXIASSERT(BXIMISC_LOGGER, NULL != stream && NULL != filename);
    errno = 0;
    const int fd = fileno(stream);
    if (-1 == fd) return bxierr_errno("Bad stream: %x", stream);
    char * path = bxistr_new("/proc/self/fd/%d", fd);

    *filename = bximisc_readlink(path);

    BXIFREE(path);
    return BXIERR_OK;
}

char * bximisc_readlink(const char * const linkname) {
    struct stat sb;
    errno = 0;
    if (lstat(linkname, &sb) == -1) {
        BXIEXIT(EX_IOERR,
                bxierr_errno("Calling lstat(%s) failed", linkname),
                BXIMISC_LOGGER, BXILOG_CRITICAL);
    }
    size_t old_size = 0;
    size_t n = (size_t) (sb.st_size + 1);
    char *targetname = NULL;
    while (true) {
        targetname = bximem_realloc(targetname, old_size, n);
        errno = 0;
        ssize_t r = readlink(linkname, targetname, n);
        if (r < 0) {
            BXIFREE(targetname);
            if (errno != EINVAL) {
                return NULL;
            }
            // EINVAL means the linkname is actually not a link: return the original name
            return strdup(linkname);
        }
        if ((size_t) r < n) {
            targetname[r] = '\0';
            break;
        }
        FINE(BXIMISC_LOGGER, "Wrong size in lstat() of '%s'", linkname);
        old_size = n;
        n *= 2;
    }
    return targetname;
}

char * bximisc_abs_readlink(const char * const linkname) {
    BXIASSERT(BXIMISC_LOGGER, NULL != linkname);

    char * link = bximisc_readlink(linkname);
    if (NULL == link) return link;
    if ('/' == link[0]) return link;

    // Relative path returned, try to compute the path from the linkname given.
    char * found = strrchr(linkname, '/');
    if (NULL == found) return link;

    size_t dirname_len = (size_t) (found - linkname);
    size_t link_len = strlen(link);
    size_t len = dirname_len + ARRAYLEN("/") + link_len + 1; // NUL terminating byte
    char * result = bximem_calloc(len * sizeof(*result));
    memcpy(result, linkname, dirname_len);
    result[dirname_len] = '/';
    memcpy(result + dirname_len + 1, link, link_len);

    return result;
}

char * bximisc_tuple_str(const size_t n,
                         const uint8_t * const tuple,
                         const uint8_t endmark,
                         const char prefix,
                         const char sep,
                         const char suffix) {
    // 2 digits * n tuples + 1 * sep * n tuples + prefix + suffix + '\0'
    size_t size = n * 2 + n + 2 + 1;

    char * p = bximem_calloc(size);
    if (p == NULL ) return NULL ;
    if (n == 0) {
        *p = prefix;
        *(p + 1) = suffix;
        *(p + 2) = '\0';
        return (p);
    }
    char * next = p;
    size_t i = 0;
    size_t written = 0;
    for (; i < n &&tuple[i] != endmark; i++) {
        char fmta[10]; // Allocate enough data for the format
        char * fmt = fmta;
        if (i == 0) {
            // First item: e.g: [03,
            fmt[0] = prefix;
            fmt[1] = '%';
            fmt[2] = '0';
            fmt[3] = '2';
            fmt[4] = 'u';
            // Special case for unidimension: e.g: [03]
            fmt[5] = (char)((i == n - 1 || tuple[i + 1] == endmark) ? suffix : sep);
            fmt[6] = '\0';
            if (prefix == '\0') fmt++; // Do not include the first character if it is '\0'
        } else {
            // Middle item: e.g: 02,
            fmt[0] = '%';
            fmt[1] = '0';
            fmt[2] = '2';
            fmt[3] = 'u';
            // Special case: last item: e.g: 01]
            fmt[4] = (char)((i == n - 1 || tuple[i + 1] == endmark) ? suffix : sep);
            fmt[5] = '\0';
        }
        size_t remaining = size - written;
        int m = snprintf(next, remaining, fmt, tuple[i]);
        /* If that worked, continue. */
        if (m > -1 && m < (int) remaining) {
            // We added m characters
            written += (size_t) m;
            next = p + written;
            continue;
        }

        /* Else try again with more space. */
        if (m > -1) {
            //    size = (size_t) m + 1u; /* gibc 2.1: exactly what is needed */
            //} else {
            size *= 2; /* glibc 2.0: twice the old size */
    }

    char * np = (char *) realloc(p, (size_t) size);
    if (!np) {
        free(p);
        return (NULL);
    }
    p = np;
    next = p + written;
    written = 0;
    }
    return (p);
}

bxierr_p bximisc_str_tuple(const char * start, char * end,
                           const char prefix, const char sep, const char suffix,
                           uint8_t * const dim,
                           uint8_t * const result) {

    BXIASSERT(BXIMISC_LOGGER, start != NULL && end != NULL);
    BXIASSERT(BXIMISC_LOGGER, result != NULL && dim != NULL);

    long int len = end-start+2;
    BXIASSERT(BXIMISC_LOGGER, len > 1);
    char * original_str = bximem_calloc((size_t) len * sizeof(*original_str));
    memcpy(original_str, start, (size_t) len - 1);
    // original_str[len] = '\0';  // Already done by bximem_calloc()
    // Next character should be the first dimension
    if (prefix != '\0') start++;
    // Change last ')' into a ',': (xx, yy, zz) -> xx, yy, zz,
    if (suffix != '\0') *end = sep;
    end++;
    //    // We have reached the end of the array, or the end of the line or
    //    // the end of the string
    //    if (*end != ' ' && *end != '\n' && *end != '\0') {
    //        return bxierr_new(340,
    //                          original_str,
    //                          free,
    //                          NULL,
    //                          "Expecting a space after ')' or ']' in %s but read a '%c'",
    //                          original_str, *end);
    //    }
    *end = '\0';
    int tmp_dim = 0;
    for (int i = 0;; i++) {
        // We have reached the end of the string
        if (start >= end) break;
        unsigned long val;
        bxierr_p err = bximisc_strtoul(start, 10, &val);
        if (bxierr_isko(err)) {
            if (BXIMISC_NODIGITS_ERR == err->code && tmp_dim == 0) {
                *dim = 0;
                bxierr_destroy(&err);
                BXIFREE(original_str);
                return BXIERR_OK;
            }
            if (BXIMISC_REMAINING_CHAR == err->code) { // The whole string contains sep
                const char * comma = (char *) err->data;

                if (*comma != sep && *comma != suffix) {
                    return  bxierr_new(340, original_str, free, NULL, err,
                                       "Bad char '%c', expecting '%c' or '%c' in '%s'",
                                       *comma, sep, suffix, original_str);
                }
                bxierr_destroy(&err);
                // Next character
                start = comma + 1;
            } else return  bxierr_new(340, original_str, free, NULL, err,
                                      "Calling bximisc_strtoul() "
                                      "failed with string '%s'",
                                      original_str);
        } else start = end + 1; // The whole string is a digit.
        /* If we got here, strtoul() successfully parsed a number */
        if (val > UINT8_MAX) {
            return bxierr_new(340,
                              original_str, free,
                              NULL ,
                              NULL ,
                              "Value too large %lu > %d in %s",
                              val, UINT8_MAX, original_str);
        }
        result[i] = (uint8_t) val;
        tmp_dim++;
    }
    *dim = (uint8_t) tmp_dim;
    BXIFREE(original_str);
    return BXIERR_OK;
}

/*----------------------------------------------------------------------------*\
 * This code has been taken from: http://www.csbruce.com/~csbruce/software/crc32.c
 */
uint32_t bximisc_crc32(uint32_t inCrc32, const void *buf, size_t bufLen) {
    static const uint32_t crcTable[256] =
    { 0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
        0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
        0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
        0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
        0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
        0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
        0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
        0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
        0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
        0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
        0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
        0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
        0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
        0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
        0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
        0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
        0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
        0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
        0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
        0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
        0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
        0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
        0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
        0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
        0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
        0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
        0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
        0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
        0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
        0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
        0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
        0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
        0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
        0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
        0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
        0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
        0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
        0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
        0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
        0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
        0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693,
        0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
        0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D };
    uint32_t crc32;
    unsigned char *byteBuf;
    size_t i;

    /** accumulate crc32 for buffer **/
    crc32 = inCrc32 ^ 0xFFFFFFFF;
    byteBuf = (unsigned char*) buf;
    for (i = 0; i < bufLen; i++) {
        crc32 = (crc32 >> 8u) ^ crcTable[(crc32 ^ byteBuf[i]) & 0xFFu];
    }
    return (crc32 ^ 0xFFFFFFFF);
}

char * bximisc_get_ip(char * hostname) {
    struct addrinfo hints, *info, *p;
    char tmp[255]; // Should be in this scope for hostname to point towards it.

    if (NULL == hostname || 0 == strcmp(hostname, "localhost")) {
        tmp[254] = '\0';
        errno = 0;
        int rc = gethostname(tmp, 254);
        if (-1 == rc) {
            BXIEXIT(EX_NOHOST,
                    bxierr_errno("Can't get host name from %s", hostname),
                    BXIMISC_LOGGER, BXILOG_CRITICAL);
        }
        hostname = tmp;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; /* IPV4 only */
    hints.ai_socktype = SOCK_STREAM; /* TCP */
    hints.ai_flags = AI_CANONNAME;

    int rc = getaddrinfo(hostname, NULL, &hints, &info);
    if (0 != rc) {
        BXIEXIT(EX_UNAVAILABLE,
                bxierr_gen("Function getaddrinfo() failed: %s",
                           gai_strerror(rc)),
                BXIMISC_LOGGER, BXILOG_CRITICAL);
    }

    if (NULL == info) {
        BXIEXIT(EX_UNAVAILABLE,
                bxierr_gen("Function getaddrinfo() returned NULL!"),
                BXIMISC_LOGGER, BXILOG_CRITICAL);
    }
    struct sockaddr_in * addr = (struct sockaddr_in*) info->ai_addr;
    char * result = strdup(inet_ntoa(addr->sin_addr));

    // Since we asked for AI_CANONNAME, the first element is the official name
    // this is the one we return.
    for(p = info->ai_next; p != NULL; p = p->ai_next) {
        struct sockaddr_in * addr = (struct sockaddr_in*) p->ai_addr;
        WARNING(BXIMISC_LOGGER, "Ignored address: %s %s", p->ai_canonname,
                inet_ntoa(addr->sin_addr));
    }
    freeaddrinfo(info);
    return (result);
}

bxierr_p bximisc_strtoul(const char * const str,
                         const int base,
                         unsigned long * const result) {
    errno = 0;
    char * endptr;
    *result = strtoul(str, &endptr, base);
    if (0 != errno) return bxierr_errno("Error while parsing number: '%s'", str);
    if (endptr == str) return bxierr_new(BXIMISC_NODIGITS_ERR,
                                         strdup(str),
                                         free,
                                         NULL ,
                                         NULL,
                                         "No digit found in '%s'",
                                         str);
    if (*endptr != '\0') {
        return bxierr_new(BXIMISC_REMAINING_CHAR,
                          endptr,
                          NULL,
                          NULL ,
                          NULL,
                          "Some non digits characters remain in string '%s'", str);
    }
    return BXIERR_OK;
}

bxierr_p bximisc_strtol(const char * const str, const int base, long * result) {
    errno = 0;
    char * endptr;
    *result = strtol(str, &endptr, base);
    if (0 != errno) return bxierr_errno("Error while parsing number: '%s'", str);
    if (endptr == str) return bxierr_new(BXIMISC_NODIGITS_ERR,
                                         strdup(str),
                                         free,
                                         NULL ,
                                         NULL,
                                         "No digit found in '%s'",
                                         str);
    if (*endptr != '\0') {
        return bxierr_new(BXIMISC_REMAINING_CHAR,
                          endptr,
                          NULL,
                          NULL ,
                          NULL,
                          "Some non digits characters remain in string '%s'", str);
    }
    return BXIERR_OK;
}

bxierr_p bximisc_strtoi(const char * const str, const int base, int * result) {
    long longresult;
    bxierr_p err = bximisc_strtol(str, base, &longresult);
    if (bxierr_isko(err)) return err;
    if (longresult < INT_MIN || longresult > INT_MAX) {
        return bxierr_new(BXIMISC_INVALID_CAST_ERR,
                          free, NULL, NULL, NULL,
                          "Converted string '%s' could not be cast "
                          "from long to int",
                          str);
    }
    *result = (int)longresult;
    return BXIERR_OK;
}


#define UNKNOWN_LAST UINT64_MAX
char * bximisc_bitarray_str(const char * const bitarray, const uint64_t n,
                            const char *prefix,
                            const char *separator,
                            const char *suffix) {

    assert(bitarray != NULL && n < UNKNOWN_LAST);
    // Create the line for printing
    char * line = NULL;
    size_t line_len = 0;
    FILE* fd = open_memstream(&line, &line_len);

    fprintf(fd, "%s", prefix);

    bool hole = true; // A hole, is a contiguous zone of cleared bits
    bool first = true;
    uint64_t last = UNKNOWN_LAST; // Last known bit in a series of set bit
    for (uint64_t i = 0; i < n; i++) {
        if (BITTEST(bitarray, i)) { // The bit i is set, print it
            if (hole) { // Only if we had a hole, that is previous bit set was not i-1
                if (last == UNKNOWN_LAST) { // Print a space if last bit set is unknown
                    if (!first) { // and i is not the first element
                        fprintf(fd, "%s", separator);
                    } else first = !first;
                }
                fprintf(fd, "%lu", (unsigned long)i); // Print the bit
                hole = false;   // we were in a hole, but this is no
                // more the case since i bit i set
            } else last = i; // do not display contiguous bit set, but record the last
        } else { // The bit is unset
            if (last != UNKNOWN_LAST) { // displays the last if known
                fprintf(fd, "-%lu", (unsigned long)last);
                last = UNKNOWN_LAST;
            }
            hole = true; // a cleared bit defines a hole
        }
    }
    if (last != UNKNOWN_LAST && !hole) fprintf(fd, "-%lu", (unsigned long)last);
    fprintf(fd, "%s", suffix);
    fclose(fd);
    return line;
}

void bximisc_stats(size_t n, uint32_t *data, bximisc_stats_s * stats_p) {
    assert(NULL != stats_p);
    uint32_t tmp = 0;
    stats_p->min = UINT32_MAX;
    stats_p->max = 0;
    for (size_t i = 0; i < n; ++i) {
        tmp += data[i];
        stats_p->min = BXIMISC_MIN(stats_p->min, data[i]);
        stats_p->max = BXIMISC_MAX(stats_p->max, data[i]);
    }
    stats_p->mean = (double) tmp / (double) n;
    double tmp2 = 0;
    for (size_t i = 0; i < n; ++i) {
        tmp2 += ((double) data[i]-stats_p->mean)*((double) data[i]-stats_p->mean);
    }
    stats_p->stddev = sqrt((double) tmp2 / (double)n);
}



bxierr_p bximisc_file_size(const char *filename, size_t * size) {
    BXIASSERT(BXIMISC_LOGGER, size != NULL);
    struct stat st;
    errno = 0;

    if (stat(filename, &st) == 0) {
        *size = (size_t)st.st_size;
        return BXIERR_OK;
    }

    bxierr_p err = bxierr_errno("An error occured while getting stat of file %s.", filename);
    return err;
}

bxierr_p bximisc_mkdir(const char* const foldername) {
    BXIASSERT(BXIMISC_LOGGER, NULL != foldername);
    TRACE(BXIMISC_LOGGER, "Creating %s", foldername);
    struct stat buf;
    errno = 0;
    int rc = mkdir(foldername, S_IRWXU | S_IRGRP | S_IROTH);
    if (rc == -1) {
        if (errno == EEXIST) {
            TRACE(BXIMISC_LOGGER, "Path %s already exists, checking that it is a directory.",
                  foldername);
            errno = 0;
            rc = stat(foldername, &buf);
            if (rc == -1) {
                bxierr_p bxierr = bxierr_errno("Calling stat() failed %s", foldername);
                return bxierr;
            }
        } else {
          bxierr_p bxierr = bxierr_errno("Can't create %s", foldername);
          return bxierr;

        }
    }
    return BXIERR_OK;
}

bxierr_p bximisc_mkdirs(const char* const foldername) {
    BXIASSERT(BXIMISC_LOGGER, NULL != foldername);
    struct stat buf;
    errno = 0;
    long rc = stat(foldername, &buf);
    if (rc == -1) {
        if (errno != ENOENT) { // All other errors are critical
          bxierr_p bxierr = bxierr_errno("Calling stat() failed on %s", foldername);
          return bxierr;
        }

        // foldername does not exist
    }
    if (rc == 0) {
        if (!S_ISDIR(buf.st_mode)) {
          bxierr_p bxierr = bxierr_errno("Can't create '%s': name already exists and "
                                          "is not a directory. Aborting.",
                                          foldername);
          return bxierr;

        } else {
            return BXIERR_OK;
        }
    }

    char * subdir = strdup(foldername);
    char * dir = dirname(subdir);
    if(strcmp(dir, "/") != 0 && strcmp(dir, ".") != 0){
        bxierr_p err = bximisc_mkdirs(dir);
        if (err != BXIERR_OK) {
          return err;
        }
    }
    BXIFREE(subdir);
    return bximisc_mkdir(foldername);
}

bxierr_p bximisc_file_map(const char * filename,
                          size_t size,
                          bool load,
                          bool link_onfile,
                          int MMAP_PROT,
                          char ** addr){
    BXIASSERT(BXIMISC_LOGGER, addr != NULL);

    int file = -1;
    errno = 0;
    char * init_file_addr = NULL;
    bxierr_p err = BXIERR_OK, err2;
    if (!link_onfile){
        init_file_addr = mmap(NULL, size, MMAP_PROT,
                              MAP_PRIVATE | MAP_ANONYMOUS, file, 0);
    } else {
        BXIASSERT(BXIMISC_LOGGER, NULL != filename);
        if (load) {
            if (filename == NULL) {
                return bxierr_gen("Can't load a file without its name");
            }
            errno = 0;
            file = open(filename, O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
            if (file == -1) return bxierr_errno("Can't open %s", filename);
            errno = 0;
            init_file_addr = mmap(NULL, size, MMAP_PROT,
                                  MAP_PRIVATE, file, 0);
        } else {
            err2 = _create_writable_file(filename, size, &file);
            BXIERR_CHAIN(err, err2);

            if (bxierr_isko(err)) return err;
            errno = 0;
            init_file_addr = mmap(NULL, size, MMAP_PROT,
                                  MAP_SHARED, file, 0);

            if (-1 == fchmod(file, S_IRUSR | S_IRGRP | S_IROTH)) {
                err2 = bxierr_errno("An error occured while changing access mode of %s.", filename);
                BXIERR_CHAIN(err, err2);
            }

        }
        if (-1 == close(file)) {
            err2 = bxierr_errno("An error occured while closing %s.", filename);
            BXIERR_CHAIN(err, err2);
            int * file_p = bximem_calloc(sizeof(*file_p));
            *file_p = file;
            err2 = bxierr_new(BXIMISC_FILE_CLOSE_ERROR, file_p, free, NULL, NULL,
                              "An error occured while closing %s.", filename);
            BXIERR_CHAIN(err, err2);
            if(init_file_addr != NULL && -1 == munmap(init_file_addr, size)) {
                bxierr_p err2 = bxierr_errno("An error occured while unmapping %s.",
                                             filename);
                BXIERR_CHAIN(err, err2);
            }
            return err;
        }
    }

    if (MAP_FAILED == init_file_addr || NULL == init_file_addr) {
        return bxierr_errno("An error occured while mapping %s.", filename);
    }

    *addr = init_file_addr;
    return BXIERR_OK;
}

bxierr_p bximisc_mkdtemp(char * tmp_name, char ** res) {
    BXIASSERT(BXIMISC_LOGGER, res != NULL);
    char * tmpdir = getenv("TMPDIR");
    if (tmpdir == NULL) {
        tmpdir = "/tmp" ;
    }
    char * prefix="bxi";
    if (tmp_name != NULL) {
        prefix = tmp_name;
    }
    char * full_tmp_name = bxistr_new("%s/%s-XXXXXX", tmpdir, prefix);
    *res = mkdtemp(full_tmp_name);
    if (*res == NULL) {
        bxierr_p err = bxierr_errno("mkdtemp can't handle the string %s", full_tmp_name);
        BXIFREE(full_tmp_name);
        return err;
    }
    return BXIERR_OK;
}

bxierr_p bximisc_mkstemp(char * tmp_name, char ** res, int *fd) {
    BXIASSERT(BXIMISC_LOGGER, res != NULL);
    char * prefix="bxi";
    if (tmp_name != NULL) {
        prefix = tmp_name;
    }
    char * tmpdir = getenv("TMPDIR");
    if (tmpdir == NULL) {
        tmpdir = "/tmp" ;
    }
    char * full_tmp_name = bxistr_new("%s/%s-XXXXXX", tmpdir, prefix);
    int rc = mkstemp(full_tmp_name);
    if (rc == -1) {
        bxierr_p err = bxierr_errno("mkstemp can't handle the string %s", full_tmp_name);
        BXIFREE(full_tmp_name);
        return err;
    }
    if (fd != NULL) {
        *fd = rc;
    } else {
        rc = close(rc);
        if (rc != 0) {
            bxierr_p err = bxierr_errno("close error on file descriptor %d for file %s",
                                        fd, full_tmp_name);
            BXIFREE(full_tmp_name);
            return err;
        }
    }
    *res = full_tmp_name;
    return BXIERR_OK;
}

// *********************************************************************************
// ********************************** Static Functions  ****************************
// *********************************************************************************

bxierr_p _create_writable_file(const char * filename, size_t size, int *fd) {
    BXIASSERT(BXIMISC_LOGGER, fd != NULL);
    errno = 0;
    *fd = open(filename, O_CREAT|O_RDWR|O_EXCL,  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (*fd == -1){
        return bxierr_errno("Can't open %s", filename);
    }

    off_t rc = lseek(*fd, (off_t)size-1, SEEK_SET);
    if (rc == -1){
        bxierr_p err = bxierr_errno("Can't lseek %s for size %zu", filename, size);
        rc = close(*fd);
        if (rc == -1){
            bxierr_p err2 = bxierr_errno("Can't fclose file descriptor %d for file %s",
                                         *fd, filename);
            BXIERR_CHAIN(err, err2);
        }
        return err;

    }

    ssize_t rc_w = write(*fd, "", 1);
    if (rc_w == -1){
        bxierr_p err = bxierr_errno("Can't write %s", filename);
        rc = close(*fd);
        if (rc == -1){
            bxierr_p err2 = bxierr_errno("Can't fclose file descriptor %d for file %s",
                                         *fd, filename);
            BXIERR_CHAIN(err, err2);
        }
        return err;
    }
    return BXIERR_OK;
}


