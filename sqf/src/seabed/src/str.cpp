//------------------------------------------------------------------
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2006-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>

#include "verslib.h"

VERS_LIB(libsbstr)

#define XX_CVP(pp_p) (void *) pp_p
#define XX_CL(pv_l) (long) pv_l

#define XX_STRN
#define XX_WCS

#define XX_MARK(pv_ab) { \
    char        la_buf[100]; \
    static bool lv_once = true; \
    static bool lv_fatal = true; \
    sprintf(la_buf, "pstacksq %d | grep -v '^#[01] '", getpid()); \
    system(la_buf); \
    if (lv_once) { \
        lv_once = false; \
        char *lp_p = getenv("OVERLAP_NONFATAL"); \
        if ((lp_p != NULL) && (atoi(lp_p) != 0)) \
            lv_fatal = false; \
    } \
    if (pv_ab && lv_fatal) abort(); \
}

static bool gv_debug = false;

#ifdef XX_STRN
static size_t xx_strnlen(const char *pp_src, size_t pv_len) {
    size_t lv_len;

    for (lv_len = 0; lv_len < pv_len && pp_src[lv_len]; lv_len++)
        ;
    return lv_len;
}
#endif // XX_STRN

#ifdef XX_WCS
static size_t xx_wcsnlen(const wchar_t *pp_src, size_t pv_len) {
    size_t lv_len;

    for (lv_len = 0; lv_len < pv_len && pp_src[lv_len]; lv_len++)
        ;
    return lv_len;
}
#endif // XX_WCS

extern "C" {
    // mbstowcs NOT implemented - unlikely overlap

    void *memccpy(void *pp_dst, const void *pp_src, int pv_c, size_t pv_len) throw() {
        char        *lp_dst;
        char        *lp_p;
        char        *lp_src;
        static bool  lv_fatal = false;
        size_t       lv_inx;
        static bool  lv_once = true;
        static bool  lv_warn = true;

        if (lv_once) {
            lv_once = false;
            lp_p = getenv("MEMCCPY_NOWARN");
            if ((lp_p != NULL) && (atoi(lp_p) != 0))
                lv_warn = false;
            lp_p = getenv("MEMCCPY_FATAL");
            if ((lp_p != NULL) && (atoi(lp_p) != 0))
                lv_fatal = true;
        }

        if (gv_debug)
            printf("memccpy(%p, %p, %d, %ld)\n",
                   pp_dst, pp_src, pv_c, XX_CL(pv_len));

        lp_dst = (char *) pp_dst;
        lp_src = (char *) pp_src;
        if (lp_src < lp_dst && lp_dst < lp_src + pv_len) {
            // overlap case 1
            if (lv_warn) {
                fprintf(stderr, "memccpy (%s:%d) overlap-1 src<dst && dst<src+len %p < %p && %p < %p + 0x%lx\n",
                       __FILE__, __LINE__,
                       lp_src, lp_dst, lp_dst, lp_src, XX_CL(pv_len));
                XX_MARK(lv_fatal);
            }
        }
        if (lp_dst < lp_src && lp_src < lp_dst + pv_len) {
            // overlap case 2
            if (lv_warn) {
                fprintf(stderr, "memccpy (%s:%d) overlap-2 dst<src && src<dst+len %p < %p && %p < %p + 0x%lx\n",
                       __FILE__, __LINE__,
                       lp_dst, lp_src, lp_src, lp_dst, XX_CL(pv_len));
                XX_MARK(lv_fatal);
            }
        }

        // doit memccpy
        for (lv_inx = 0; lv_inx < pv_len; lv_inx++) {
            lp_dst[lv_inx] = lp_src[lv_inx];
            if (lp_src[lv_inx] == pv_c)
                return &lp_dst[lv_inx + 1];
        }
        return NULL;
    }

    void *memcpy(void *pp_dst, const void *pp_src, size_t pv_len) throw() {
        char        *lp_dst;
        char        *lp_p;
        char        *lp_src;
        static bool  lv_fatal = false;
        size_t       lv_inx;
        static bool  lv_once = true;
        static bool  lv_warn = true;

        if (lv_once) {
            lv_once = false;
            lp_p = getenv("MEMCPY_NOWARN");
            if ((lp_p != NULL) && (atoi(lp_p) != 0))
                lv_warn = false;
            lp_p = getenv("MEMCPY_FATAL");
            if ((lp_p != NULL) && (atoi(lp_p) != 0))
                lv_fatal = true;
        }

#if 0
        if (gv_debug)
            printf("memcpy(%p, %p, %ld)\n", pp_dst, pp_src, XX_CL(pv_len));
#endif

        lp_dst = (char *) pp_dst;
        lp_src = (char *) pp_src;
        if (lp_src < lp_dst && lp_dst < lp_src + pv_len) {
            // overlap case 1
            if (lv_warn) {
                fprintf(stderr, "memcpy (%s:%d) overlap-1 src<dst && dst<src+len %p < %p && %p < %p + 0x%lx\n",
                       __FILE__, __LINE__,
                       lp_src, lp_dst, lp_dst, lp_src, XX_CL(pv_len));
                XX_MARK(lv_fatal);
            }
        }
        if (lp_dst < lp_src && lp_src < lp_dst + pv_len) {
            // overlap case 2
            if (lv_warn) {
                fprintf(stderr, "memcpy (%s:%d) overlap-2 dst<src && src<dst+len %p < %p && %p < %p + 0x%lx\n",
                       __FILE__, __LINE__,
                       lp_dst, lp_src, lp_src, lp_dst, XX_CL(pv_len));
                XX_MARK(lv_fatal);
            }
        }

        // doit memcpy
        for (lv_inx = 0; lv_inx < pv_len; lv_inx++)
            lp_dst[lv_inx] = lp_src[lv_inx];
        return lp_dst;
    }

    char *strcpy(char *pp_dst, const char *pp_src) throw() {
        size_t lv_inx;
        size_t lv_len;

        lv_len = strlen(pp_src) + 1;

#if 0
        if (gv_debug)
            printf("strcpy(%p, %p) [srclen=%ld]\n",
                   pp_dst, pp_src, XX_CL(lv_len));
#endif

        if (pp_src < pp_dst && pp_dst < pp_src + lv_len) {
            // overlap case 1
            fprintf(stderr, "strcpy (%s:%d) overlap-1 src<dst && dst<src+len %p < %p && %p < %p + 0x%lx\n",
                   __FILE__, __LINE__,
                   pp_src, pp_dst, pp_dst, pp_src, XX_CL(lv_len));
            XX_MARK(true);
        }
        if (pp_dst < pp_src && pp_src < pp_dst + lv_len) {
            // overlap case 2
            fprintf(stderr, "strcpy (%s:%d) overlap-2 dst<src && src<dst+len %p < %p && %p < %p + 0x%lx\n",
                   __FILE__, __LINE__,
                   pp_dst, pp_src, pp_src, pp_dst, XX_CL(lv_len));
            XX_MARK(true);
        }

        // doit strcpy
        for (lv_inx = 0; lv_inx < lv_len; lv_inx++)
            pp_dst[lv_inx] = pp_src[lv_inx];
        return pp_dst;
    }

    char *strcat(char *pp_dst, const char *pp_src) throw() {
        size_t lv_inx;
        size_t lv_lendst;
        size_t lv_lensrc;

        lv_lendst = strlen(pp_dst);
        lv_lensrc = strlen(pp_src) + 1;

        if (gv_debug)
            printf("strcat(%p, %p) [dstlen=%ld, srclen=%ld]\n",
                   pp_dst, pp_src, XX_CL(lv_lendst), XX_CL(lv_lensrc));

        if (pp_src < pp_dst + lv_lendst && pp_dst + lv_lendst < pp_src + lv_lensrc) {
            fprintf(stderr, "strcat (%s:%d) overlap src<dst+dstlen && dst+dstlen<src+srclen %p < %p + 0x%lx && %p + 0x%lx < %p + 0x%lx\n",
                   __FILE__, __LINE__,
                   pp_src, pp_dst, lv_lendst,
                   pp_dst, XX_CL(lv_lendst), pp_src, XX_CL(lv_lensrc));
            XX_MARK(true);
        }

        // doit strcat
        for (lv_inx = 0; lv_inx < lv_lensrc; lv_inx++)
            pp_dst[lv_lendst + lv_inx] = pp_src[lv_inx];
        return pp_dst;
    }

    // strftime NOT implemented - unlikely overlap

#ifdef XX_STRN
    char *strncat(char *pp_dst, const char *pp_src, size_t pv_len) throw() {
        size_t lv_inx;
        size_t lv_lendst;
        size_t lv_lensrc;

        if (gv_debug)
            printf("strncat(%p, %p, %ld)\n",
                   pp_dst, pp_src, XX_CL(pv_len));

        if (pv_len <= 0)
            return pp_dst; // done

        lv_lendst = strlen(pp_dst);
        lv_lensrc = xx_strnlen(pp_src, pv_len);

        if (gv_debug)
            printf("strncat(%p, %p) [dstlen=%ld, srclen=%ld]\n",
                   pp_dst, pp_src, XX_CL(lv_lendst), XX_CL(lv_lensrc));

        if (pp_src < pp_dst + lv_lendst &&
            pp_dst + lv_lendst < pp_src + lv_lensrc + 1) {
            // overlap case 1
            fprintf(stderr, "strncat (%s:%d) overlap-1 src<dst+dstlen && dst+dstlen<src+srclen+1 %p < %p + 0x%lx && %p + 0x%lx < %p + 0x%lx\n",
                   __FILE__, __LINE__,
                   pp_src, pp_dst, lv_lendst,
                   pp_dst, XX_CL(lv_lendst), pp_src, XX_CL(lv_lensrc));
            XX_MARK(true);
        }
        if (pp_dst < pp_src + lv_lensrc &&
            pp_src + lv_lensrc < pp_dst + lv_lendst + 1) {
            // overlap case 1
            fprintf(stderr, "strncat (%s:%d) overlap-1 dst<src+srclen && src+srclen<dst+dstlen+1 %p < %p + 0x%lx && %p + 0x%lx < %p + 0x%lx\n",
                   __FILE__, __LINE__,
                   pp_dst, pp_src, lv_lensrc,
                   pp_src, XX_CL(lv_lensrc), pp_dst, XX_CL(lv_lendst));
            XX_MARK(true);
        }

        // doit strncat
        for (lv_inx = 0; lv_inx < lv_lensrc; lv_inx++)
            pp_dst[lv_lendst + lv_inx] = pp_src[lv_inx];
        pp_dst[lv_lendst + lv_lensrc] = 0;
        return pp_dst;
    }

    char *strncpy(char *pp_dst, const char *pp_src, size_t pv_len) throw() {
        size_t lv_inx;
        size_t lv_lensrc;

        lv_lensrc = xx_strnlen(pp_src, pv_len);

#if 0
        if (gv_debug)
            printf("strncpy(%p, %p, %ld) [srcnlen=%ld]\n",
                   pp_dst, pp_src, XX_CL(pv_len), XX_CL(lv_len));
#endif

        if (pv_len > 0 &&
            pp_src < pp_dst &&
            pp_dst < pp_src + lv_lensrc) {
            // overlap case 1
            fprintf(stderr, "strncpy (%s:%d) overlap-1 src<dst && dst<src+srclen %p < %p && %p < %p + 0x%lx\n",
                   __FILE__, __LINE__, pp_src, pp_dst, pp_dst, pp_src, XX_CL(lv_lensrc));
            XX_MARK(true);
        }
        if (pv_len > 0 &&
            pp_dst < pp_src &&
            pp_src < pp_dst + pv_len) {
            // overlap case 2
            fprintf(stderr, "strncpy (%s:%d) overlap-2 dst<src && src<dst+len %p < %p && %p < %p + 0x%lx\n",
                   __FILE__, __LINE__, pp_dst, pp_src, pp_src, pp_dst, XX_CL(pv_len));
            XX_MARK(true);
        }

        // doit strncpy
        for (lv_inx = 0; lv_inx < pv_len && pp_src[lv_inx]; lv_inx++)
            pp_dst[lv_inx] = pp_src[lv_inx];

        for (; lv_inx < pv_len; lv_inx++)
            pp_dst[lv_inx] = 0;

        return pp_dst;
    }
#endif // XX_STRN

    // strxfrm NOT implemented - not called by SQ

    // swab NOT implemented - not called by SQ

#ifdef XX_WCS
    wchar_t *wcscat(wchar_t *pp_dst, const wchar_t *pp_src) throw() {
        size_t lv_inx;
        size_t lv_lendst;
        size_t lv_lensrc;

        lv_lendst = wcslen(pp_dst);
        lv_lensrc = wcslen(pp_src) + 1;

        if (gv_debug)
            printf("wcscat(%p, %p) [dstlen=%ld, srclen=%ld]\n",
                   XX_CVP(pp_dst), XX_CVP(pp_src),
                   XX_CL(lv_lendst), XX_CL(lv_lensrc));

        if (pp_src < pp_dst + lv_lendst &&
            pp_dst + lv_lendst < pp_src + lv_lensrc) {
            fprintf(stderr, "wcscat (%s:%d) overlap src<dst+dstlen && dst+dstlen<src+srclen %p < %p + 0x%lx && %p + 0x%lx < %p + 0x%lx\n",
                   __FILE__, __LINE__,
                   XX_CVP(pp_src), XX_CVP(pp_dst), lv_lendst,
                   XX_CVP(pp_dst), XX_CL(lv_lendst),
                   XX_CVP(pp_src), XX_CL(lv_lensrc));
            XX_MARK(true);
        }

        // doit wcscat
        for (lv_inx = 0; lv_inx < lv_lensrc; lv_inx++)
            pp_dst[lv_lendst + lv_inx] = pp_src[lv_inx];
        return pp_dst;
    }

    wchar_t *wcscpy(wchar_t *pp_dst, const wchar_t *pp_src) throw() {
        size_t lv_inx;
        size_t lv_len;

        lv_len = wcslen(pp_src) + 1;

        if (gv_debug)
            printf("wcscpy(%p, %p) [srclen=%ld]\n",
                   XX_CVP(pp_dst), XX_CVP(pp_src), XX_CL(lv_len));

        if (pp_src < pp_dst && pp_dst < pp_src + lv_len) {
            // overlap case 1
            fprintf(stderr, "wcscpy (%s:%d) overlap-1 src<dst && dst<src+len %p < %p && %p < %p + 0x%lx\n",
                   __FILE__, __LINE__,
                   XX_CVP(pp_src), XX_CVP(pp_dst),
                   XX_CVP(pp_dst), XX_CVP(pp_src), XX_CL(lv_len));
            XX_MARK(true);
        }
        if (pp_dst < pp_src && pp_src < pp_dst + lv_len) {
            // overlap case 2
            fprintf(stderr, "wcscpy (%s:%d) overlap-2 dst<src && src<dst+len %p < %p && %p < %p + 0x%lx\n",
                   __FILE__, __LINE__,
                   XX_CVP(pp_dst), XX_CVP(pp_src),
                   XX_CVP(pp_src), XX_CVP(pp_dst), XX_CL(lv_len));
            XX_MARK(true);
        }

        // doit wcscpy
        for (lv_inx = 0; lv_inx < lv_len; lv_inx++)
            pp_dst[lv_inx] = pp_src[lv_inx];
        return pp_dst;
    }

    // wcsftime NOT implemented - unlikely overlap

    wchar_t *wcsncat(wchar_t *pp_dst, const wchar_t *pp_src, size_t pv_len) throw() {
        size_t lv_inx;
        size_t lv_lendst;
        size_t lv_lensrc;

        if (gv_debug)
            printf("wcsncat(%p, %p, %ld)\n",
                   XX_CVP(pp_dst), XX_CVP(pp_src), XX_CL(pv_len));

        if (pv_len <= 0)
            return pp_dst; // done

        lv_lendst = wcslen(pp_dst);
        lv_lensrc = xx_wcsnlen(pp_src, pv_len);

        if (gv_debug)
            printf("wcsncat(%p, %p) [dstlen=%ld, srclen=%ld]\n",
                   XX_CVP(pp_dst), XX_CVP(pp_src), XX_CL(lv_lendst), XX_CL(lv_lensrc));

        if (pp_src < pp_dst + lv_lendst &&
            pp_dst + lv_lendst < pp_src + lv_lensrc + 1) {
            // overlap case 1
            fprintf(stderr, "wcsncat (%s:%d) overlap-1 src<dst+dstlen && dst+dstlen<src+srclen+1 %p < %p + 0x%lx && %p + 0x%lx < %p + 0x%lx\n",
                   __FILE__, __LINE__,
                   XX_CVP(pp_src), XX_CVP(pp_dst), lv_lendst,
                   XX_CVP(pp_dst), XX_CL(lv_lendst),
                   XX_CVP(pp_src), XX_CL(lv_lensrc));
            XX_MARK(true);
        }
        if (pp_dst < pp_src + lv_lensrc &&
            pp_src + lv_lensrc < pp_dst + lv_lendst + 1) {
            // overlap case 1
            fprintf(stderr, "wcsncat (%s:%d) overlap-1 dst<src+srclen && src+srclen<dst+dstlen+1 %p < %p + 0x%lx && %p + 0x%lx < %p + 0x%lx\n",
                   __FILE__, __LINE__,
                   XX_CVP(pp_dst), XX_CVP(pp_src), lv_lensrc,
                   XX_CVP(pp_src), XX_CL(lv_lensrc),
                   XX_CVP(pp_dst), XX_CL(lv_lendst));
            XX_MARK(true);
        }

        // doit wcsncat
        for (lv_inx = 0; lv_inx < lv_lensrc; lv_inx++)
            pp_dst[lv_lendst + lv_inx] = pp_src[lv_inx];
        pp_dst[lv_lendst + lv_lensrc] = 0;
        return pp_dst;
    }

    wchar_t *wcsncpy(wchar_t *pp_dst, const wchar_t *pp_src, size_t pv_len) throw() {
        size_t lv_inx;
        size_t lv_len;

        lv_len = xx_wcsnlen(pp_src, pv_len);

        if (gv_debug)
            printf("wcsncpy(%p, %p, %ld) [srcnlen=%ld]\n",
                   XX_CVP(pp_dst), XX_CVP(pp_src), XX_CL(pv_len), XX_CL(lv_len));

        if (lv_len > 0 && pp_src < pp_dst && pp_dst < pp_src + lv_len) {
            // overlap case 1
            fprintf(stderr, "wcsncpy (%s:%d) overlap-1 src<dst && dst<src+len %p < %p && %p < %p + 0x%lx\n",
                   __FILE__, __LINE__,
                   XX_CVP(pp_src), XX_CVP(pp_dst),
                   XX_CVP(pp_dst), XX_CVP(pp_src), XX_CL(lv_len));
            XX_MARK(true);
        }
        if (lv_len > 0 && pp_dst < pp_src && pp_src < pp_dst + lv_len) {
            // overlap case 2
            fprintf(stderr, "wcsncpy (%s:%d) overlap-2 dst<src && src<dst+len %p < %p && %p < %p + 0x%lx\n",
                   __FILE__, __LINE__,
                   XX_CVP(pp_dst), XX_CVP(pp_src),
                   XX_CVP(pp_src), XX_CVP(pp_dst), XX_CL(lv_len));
            XX_MARK(true);
        }

        // doit wcsncpy
        for (lv_inx = 0; lv_inx < lv_len && pp_src[lv_inx]; lv_inx++)
            pp_dst[lv_inx] = pp_src[lv_inx];

        for (; lv_inx < lv_len; lv_inx++)
            pp_dst[lv_inx] = 0;

        return pp_dst;
    }
#endif // XX_WCS

    // wcstombs NOT implemented - unlikely overlap

    // wcsxfrm NOT implemented - unlikely overlap?

}
