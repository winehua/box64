#include "core.h"

#include <sys/syscall.h>
int main(int argc, const char **argv, char **env) {
    syscall(__NR_prctl, 0x6a6974, 0, 0);

    x64emu_t* emu = NULL;
    elfheader_t* elf_header = NULL;
    if (initialize(argc, argv, env, &emu, &elf_header, 1)) {
        return -1;
    }

    return emulate(emu, elf_header);
}
