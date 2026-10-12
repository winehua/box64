/* OHOS_PATCH_DISABLE_MALLOCHOOK -- production passthrough.
 *
 * box64 原版 mallochook 用一组内部内存池 + spin lock 接管所有
 * malloc/free, 在 OHOS musl 上于 ctor 阶段死循环, 故整体替换为
 * 直通 libc 的实现: 所有 box_* 函数直接转给 libc.
 *
 * 已知行为差异 (长期): 不再用强符号覆盖 malloc/free -- guest 自带
 * tcmalloc 类分配器不会被拦截, box_malloc_usable_size 恒返回 0.
 */
#include <limits.h>
#include <stdlib.h>
#include <string.h>

extern void *memalign(size_t, size_t);

/* ---------- 基础 box_* 分配族 ---------- */
void *box_malloc(size_t s)                  { return malloc(s); }
void  box_free(void *p)                     { free(p); }
void *box_calloc(size_t n, size_t s)        { return calloc(n, s); }
void *box_realloc(void *p, size_t s)        { return realloc(p, s); }
void *box_memalign(size_t a, size_t s)      { return memalign(a, s); }
char *box_strdup(const char *s)             { return s ? strdup(s) : NULL; }
char *box_strndup(const char *s, size_t n)  { return s ? strndup(s, n) : NULL; }

/* 部分 box64 内部入口的最小桩 */
void   box_free_internal(void *p)              { free(p); }
size_t box_malloc_usable_size(void *p)         { (void)p; return 0; }

/* ---------- box64 启停 hook (直通实现: 全部空操作) ---------- */
/* 原版在此安装/卸载全局 malloc 拦截器, 直通实现为 no-op. */
void init_malloc_hook(void)  {}
void startMallocHook(void)   {}
void endMallocHook(void)     {}

/* checkHookedSymbols: 原版用于扫描刚加载的 ELF 的 dynsym 表,
 * 把命中我们感兴趣的符号 (malloc/free 等) 替换成 box_* 实现.
 * 直通实现不再 hook 全局 malloc, 此函数为 no-op.
 * 第二参数原型一般是 elfheader_t* / SymbolMap*, 用 void* 兼容.
 */
void checkHookedSymbols(void *symbols, void *h)
{
    (void)symbols; (void)h;
}

/* box_realpath: 路径解析 + 失败时退化为原始路径,
 * 保证非 NULL 返回 (box64 多处调用站点不检查 NULL). */
char *box_realpath(const char *path, char *resolved)
{
    if (!path) return NULL;

    char buf[PATH_MAX];
    char *target = resolved ? resolved : buf;
    char *r = realpath(path, target);

    if (r) {
        return resolved ? r : strdup(buf);
    }

    /* realpath 失败 (文件不存在 / 权限 / 等) -- 退化为原始字符串.
     * 不要返回 NULL, box64 上层不做空指针检查. */
    if (resolved) {
        size_t n = strlen(path);
        if (n >= PATH_MAX) n = PATH_MAX - 1;
        memcpy(resolved, path, n);
        resolved[n] = '\0';
        return resolved;
    }
    return strdup(path);
}

/* 不再用强符号覆盖系统 malloc/free, 让 musl 自己处理 */
