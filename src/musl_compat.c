/*
 * musl_compat.c — glibc-private symbol stubs for OHOS musl.
 *
 * All implementations are weak so that any future OHOS NDK update which
 * adds a real symbol will silently override these.
 */
#define _GNU_SOURCE
#define BOX64_OHOS_MUSL_COMPAT 1

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern void *memalign(size_t, size_t);  /* not in <stdlib.h> on musl */

/* ------------------------------------------------------------------
 * glibc-private __libc_* malloc family
 * ------------------------------------------------------------------ */
__attribute__((weak)) void *__libc_malloc (size_t s)            { return malloc(s); }
__attribute__((weak)) void  __libc_free   (void *p)             { free(p); }
__attribute__((weak)) void *__libc_calloc (size_t n, size_t s)  { return calloc(n, s); }
__attribute__((weak)) void *__libc_realloc(void *p, size_t s)   { return realloc(p, s); }
__attribute__((weak)) void *__libc_memalign(size_t a, size_t s) { return memalign(a, s); }
__attribute__((weak)) void *__libc_valloc (size_t s)
{
    return memalign(sysconf(_SC_PAGESIZE), s);
}
__attribute__((weak)) void *__libc_pvalloc(size_t s)
{
    long pg = sysconf(_SC_PAGESIZE);
    size_t r = (s + pg - 1) & ~(pg - 1);
    return memalign(pg, r);
}

/* ------------------------------------------------------------------
 * pthread NP extensions
 * ------------------------------------------------------------------ */
__attribute__((weak))
int pthread_attr_setaffinity_np(pthread_attr_t *a, size_t s, const void *c)
{ (void)a;(void)s;(void)c; return ENOSYS; }

__attribute__((weak))
int pthread_attr_getaffinity_np(const pthread_attr_t *a, size_t s, void *c)
{ (void)a;(void)s;(void)c; return ENOSYS; }

__attribute__((weak))
int pthread_getaffinity_np(pthread_t t, size_t s, void *c)
{ (void)t;(void)s;(void)c; return ENOSYS; }

__attribute__((weak))
int pthread_setaffinity_np(pthread_t t, size_t s, const void *c)
{ (void)t;(void)s;(void)c; return ENOSYS; }

__attribute__((weak))
int pthread_getattr_default_np(pthread_attr_t *a)
{ (void)a; return ENOSYS; }

__attribute__((weak))
int pthread_setattr_default_np(pthread_attr_t *a)
{ (void)a; return ENOSYS; }

__attribute__((weak))
int pthread_mutexattr_getrobust(const pthread_mutexattr_t *a, int *r)
{ (void)a; if (r) *r = 0; return 0; }

__attribute__((weak))
int pthread_mutexattr_setrobust(pthread_mutexattr_t *a, int r)
{ (void)a;(void)r; return 0; }

__attribute__((weak))
int pthread_mutexattr_getprioceiling(const pthread_mutexattr_t *a, int *p)
{ (void)a; if (p) *p = 0; return 0; }

__attribute__((weak))
int pthread_mutexattr_setprioceiling(pthread_mutexattr_t *a, int p)
{ (void)a;(void)p; return ENOSYS; }

/* ------------------------------------------------------------------
 * dlinfo — musl doesn't ship one. Return ENOSYS.
 * ------------------------------------------------------------------ */
__attribute__((weak))
int dlinfo(void *handle, int request, void *info)
{
    (void)handle; (void)request; (void)info;
    errno = ENOSYS;
    return -1;
}

/* ------------------------------------------------------------------
 * qsort_r — thread-local trampoline to qsort
 * ------------------------------------------------------------------ */
typedef int (*qsort_r_compar_t)(const void *, const void *, void *);
static __thread qsort_r_compar_t g_qr_compar;
static __thread void            *g_qr_arg;

static int qsort_r_thunk(const void *a, const void *b)
{
    return g_qr_compar(a, b, g_qr_arg);
}

__attribute__((weak))
void qsort_r(void *base, size_t nmemb, size_t size,
             qsort_r_compar_t compar, void *arg)
{
    g_qr_compar = compar;
    g_qr_arg    = arg;
    qsort(base, nmemb, size, qsort_r_thunk);
}

/* ------------------------------------------------------------------
 * glob64 / globfree64 — on musl, off64_t == off_t, just forward.
 * ------------------------------------------------------------------ */
extern int  glob (const char *, int, int (*)(const char *, int), void *);
extern void globfree(void *);

__attribute__((weak))
int glob64(const char *pat, int flags,
           int (*errfunc)(const char *, int), void *pglob)
{
    return glob(pat, flags, errfunc, pglob);
}

__attribute__((weak))
void globfree64(void *pglob) { globfree(pglob); }

/* ------------------------------------------------------------------
 * scandirat / scandirat64
 * ------------------------------------------------------------------ */
__attribute__((weak))
int scandirat(int dirfd, const char *dirp,
              struct dirent ***namelist,
              int (*filter)(const struct dirent *),
              int (*compar)(const struct dirent **,
                            const struct dirent **))
{
    int fd = openat(dirfd, dirp, O_RDONLY | O_DIRECTORY);
    if (fd < 0) return -1;
    DIR *d = fdopendir(fd);
    if (!d) { close(fd); return -1; }

    struct dirent **list = NULL;
    int n = 0, cap = 0;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (filter && !filter(e)) continue;
        struct dirent *copy = (struct dirent *)malloc(sizeof(*e));
        if (!copy) { closedir(d); free(list); return -1; }
        memcpy(copy, e, sizeof(*e));
        if (n == cap) {
            cap = cap ? cap * 2 : 16;
            list = (struct dirent **)realloc(list, cap * sizeof(*list));
        }
        list[n++] = copy;
    }
    closedir(d);
    if (compar) {
        qsort(list, n, sizeof(*list),
              (int (*)(const void *, const void *))compar);
    }
    *namelist = list;
    return n;
}

__attribute__((weak))
int scandirat64(int dirfd, const char *dirp,
                struct dirent ***namelist,
                int (*filter)(const struct dirent *),
                int (*compar)(const struct dirent **,
                              const struct dirent **))
{
    return scandirat(dirfd, dirp, namelist, filter, compar);
}

/* ------------------------------------------------------------------
 * obstack_printf / obstack_vprintf — musl-obstack 不提供
 * ------------------------------------------------------------------ */
struct obstack;
extern void _obstack_grow_box(struct obstack *o, const void *data, size_t n);
/* (real implementation below uses obstack_grow which is a macro; include the
 *  obstack header for it.) */
#include "obstack.h"

__attribute__((weak))
int obstack_vprintf(struct obstack *obs, const char *fmt, va_list ap)
{
    char small[2048];
    va_list ap2;
    va_copy(ap2, ap);
    int n = vsnprintf(small, sizeof(small), fmt, ap);
    if (n >= 0 && (size_t)n < sizeof(small)) {
        obstack_grow(obs, small, n);
    } else if (n >= 0) {
        char *big = (char *)malloc(n + 1);
        if (big) {
            vsnprintf(big, n + 1, fmt, ap2);
            obstack_grow(obs, big, n);
            free(big);
        }
    }
    va_end(ap2);
    return n;
}

__attribute__((weak))
int obstack_printf(struct obstack *obs, const char *fmt, ...)
{
    va_list ap;
    int n;
    va_start(ap, fmt);
    n = obstack_vprintf(obs, fmt, ap);
    va_end(ap);
    return n;
}

/* ------------------------------------------------------------------
 * __ctype_b_loc / __ctype_tolower_loc / __ctype_toupper_loc
 * Allocate 384-entry tables; return pointer offset by +128 so callers
 * can index in [-128, 255] like glibc does.
 * ------------------------------------------------------------------ */
#define BOX_ISupper  0x0100
#define BOX_ISlower  0x0200
#define BOX_ISalpha  0x0400
#define BOX_ISdigit  0x0800
#define BOX_ISxdigit 0x1000
#define BOX_ISspace  0x2000
#define BOX_ISprint  0x4000
#define BOX_ISgraph  0x8000
#define BOX_ISblank  0x0001
#define BOX_IScntrl  0x0002
#define BOX_ISpunct  0x0004
#define BOX_ISalnum  0x0008

static unsigned short box_ctype_b      [384];
static int            box_ctype_tolower[384];
static int            box_ctype_toupper[384];

static const unsigned short *box_ctype_b_ptr       = box_ctype_b       + 128;
static const int            *box_ctype_tolower_ptr = box_ctype_tolower + 128;
static const int            *box_ctype_toupper_ptr = box_ctype_toupper + 128;

__attribute__((constructor(102)))
static void box_init_ctype_tables(void)
{
    for (int c = 0; c < 256; c++) {
        unsigned short f = 0;
        if (c == ' ' || c == '\t')      f |= BOX_ISblank;
        if (c >= 0x09 && c <= 0x0D)     f |= BOX_ISspace;
        if (c == ' ')                   f |= BOX_ISspace;
        if (c < 0x20 || c == 0x7F)      f |= BOX_IScntrl;
        if (c >= 'A' && c <= 'Z')       f |= BOX_ISupper | BOX_ISalpha | BOX_ISalnum | BOX_ISprint | BOX_ISgraph;
        if (c >= 'a' && c <= 'z')       f |= BOX_ISlower | BOX_ISalpha | BOX_ISalnum | BOX_ISprint | BOX_ISgraph;
        if (c >= '0' && c <= '9')       f |= BOX_ISdigit | BOX_ISalnum | BOX_ISxdigit | BOX_ISprint | BOX_ISgraph;
        if ((c >= 'a' && c <= 'f') ||
            (c >= 'A' && c <= 'F'))     f |= BOX_ISxdigit;
        if (c >= 0x21 && c <= 0x7E && !(f & BOX_ISalnum))
            f |= BOX_ISpunct | BOX_ISprint | BOX_ISgraph;
        if (c == ' ')                   f |= BOX_ISprint;

        box_ctype_b      [c + 128] = f;
        box_ctype_tolower[c + 128] = (c >= 'A' && c <= 'Z') ? (c + 32) : c;
        box_ctype_toupper[c + 128] = (c >= 'a' && c <= 'z') ? (c - 32) : c;
    }
}

__attribute__((weak))
const unsigned short **__ctype_b_loc(void)
{
    return (const unsigned short **)&box_ctype_b_ptr;
}

__attribute__((weak))
const int **__ctype_tolower_loc(void)
{
    return (const int **)&box_ctype_tolower_ptr;
}

__attribute__((weak))
const int **__ctype_toupper_loc(void)
{
    return (const int **)&box_ctype_toupper_ptr;
}

/* ------------------------------------------------------------------
 * Misc math helpers some box64 code may reference
 * ------------------------------------------------------------------ */
__attribute__((weak)) int isnanf (float x) { return __builtin_isnan(x); }
__attribute__((weak)) int isinff (float x) { return __builtin_isinf(x); }
__attribute__((weak)) int finitef(float x) { return __builtin_isfinite(x); }

__attribute__((weak)) double      exp10 (double x)      { return pow (10.0,  x); }
__attribute__((weak)) float       exp10f(float x)       { return powf(10.0f, x); }
__attribute__((weak)) long double exp10l(long double x) { return powl(10.0L, x); }
