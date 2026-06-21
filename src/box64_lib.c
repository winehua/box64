/**
 * box64_lib.c — Box64 shared library entry point
 *
 * 问题: ARM64 Pad 无 execve，Box64 不能作为独立可执行文件运行。
 * 解决: 编译为 box64.so，由 wine_child NCP 子进程 dlopen 加载后调用
 * box64_hmos_main()，在同一宿主进程内模拟执行 x86_64 Wine ELF。
 *
 * @param argc  Box64 argv: ["box64"(标记), "/path/to/wine"(x86_64 ELF), guest_arg0, ...]
 * @param argv  如上
 * @param env   宿主环境变量 (environ)
 * @return      0=成功, -1=初始化失败
 */
#include "core.h"

__attribute__((visibility("default")))
int box64_hmos_main(int argc, const char **argv, char **env)
{
    x64emu_t* emu = NULL;
    elfheader_t* elf_header = NULL;

    if (initialize(argc, argv, env, &emu, &elf_header, 0))
        return -1;

    return emulate(emu, elf_header);
}
