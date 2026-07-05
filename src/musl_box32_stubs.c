/* OHOS_PATCH_BOX32_LINK_STUBS
 *
 * Link-time stubs for the BOX32 build on HarmonyOS musl.
 *
 * box32_malloc/calloc/realloc/free/strdup/memalign/malloc_usable_size
 * are now implemented in musl_compat.c (patch 28, low-4GB heap).
 *
 * Remaining stubs: glibc-only functions not available on OHOS musl.
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <netdb.h>
#include <errno.h>
#include <resolv.h>
#include <ucontext.h>

/* ---- glibc-only libc functions ------------------------------ */

int getprotobyname_r(const char* name,
                     struct protoent* result_buf,
                     char* buf, size_t buflen,
                     struct protoent** result)
{
    (void)result_buf; (void)buf; (void)buflen;
    struct protoent* p = getprotobyname(name);
    if (!p) { *result = NULL; return ENOENT; }
    *result = p;
    return 0;
}

int getprotobynumber_r(int proto,
                       struct protoent* result_buf,
                       char* buf, size_t buflen,
                       struct protoent** result)
{
    (void)result_buf; (void)buf; (void)buflen;
    struct protoent* p = getprotobynumber(proto);
    if (!p) { *result = NULL; return ENOENT; }
    *result = p;
    return 0;
}

struct __res_state;
struct __res_state* __res_state(void) {
    static struct __res_state s;
    return &s;
}

void __chk_fail(void) {
    fprintf(stderr, "FORTIFY_SOURCE check failed (stub)\n");
    abort();
}

/* ucontext: called by wrappedexpat XML coroutine path */
int getcontext(ucontext_t* ucp) {
    (void)ucp;
    fprintf(stderr, "getcontext: not implemented on OHOS musl\n");
    return -1;
}
int makecontext(ucontext_t* ucp, void (*func)(), int argc, ...) {
    (void)ucp; (void)func; (void)argc;
    fprintf(stderr, "makecontext: not implemented on OHOS musl\n");
    return -1;
}
int swapcontext(ucontext_t* oucp, const ucontext_t* ucp) {
    (void)oucp; (void)ucp;
    fprintf(stderr, "swapcontext: not implemented on OHOS musl\n");
    return -1;
}
