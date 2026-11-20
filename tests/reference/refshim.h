/*
 * refshim.h - force-included (-include) when compiling the ORIGINAL
 * Pro-Wizard 1.70a sources as the reference for differential testing.
 *
 * The original reads outside its buffers (before and after) and uses
 * uninitialised memory in places, so its output on such inputs depends on
 * whatever memory surrounds them. The port defines those bytes as zero. To
 * compare like with like, every allocation of the reference is zero-filled
 * and surrounded by 1 MiB of zeroed slack on each side (and the reference is
 * compiled with -ftrivial-auto-var-init=zero). Nothing is freed: one
 * converter frees a buffer twice, which aborts on some C libraries.
 */
#ifndef PW_REFSHIM_H
#define PW_REFSHIM_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PW_REF_SLACK ((size_t)1 << 20)

static void *pw_ref_alloc(size_t n)
{
    char *p = (char *)calloc(n + 2 * PW_REF_SLACK, 1);
    return p ? p + PW_REF_SLACK : NULL;
}

#define malloc(n) pw_ref_alloc(n)
#define free(p) ((void)(p))
#endif
