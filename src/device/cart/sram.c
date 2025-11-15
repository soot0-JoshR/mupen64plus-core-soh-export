/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - sram.c                                                  *
 *   Mupen64Plus homepage: https://mupen64plus.org/                        *
 *   Copyright (C) 2014 Bobby Smiles                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "sram.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "backends/api/storage_backend.h"
#include "device/memory/memory.h"
#include "device/rdram/rdram.h"

#define SRAM_ADDR_MASK UINT32_C(0x0000ffff)

#ifdef ENABLE_SOH_EXPORT
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>
#include <stdio.h>

/* Asynchronous exporter launch */
static void soh_export_sram_async(const char* rom_path, const char* sra_path)
{
    char exe_path[PATH_MAX];
    const char* custom_path = getenv("SOH_EXPORT_PATH");

    /* Prefer explicit user override */
    if (custom_path && access(custom_path, X_OK) == 0) {
        strncpy(exe_path, custom_path, sizeof(exe_path));
    }
    /* Standard dev-tree path */
    else if (access("tools/soh_export", X_OK) == 0) {
        strncpy(exe_path, "tools/soh_export", sizeof(exe_path));
    }
    /* Local dir fallback */
    else if (access("./soh_export", X_OK) == 0) {
        strncpy(exe_path, "./soh_export", sizeof(exe_path));
    }
    /* CWD-based fallback */
    else {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd))) {
            snprintf(exe_path, sizeof(exe_path), "%s/../tools/soh_export", cwd);
        } else {
            strncpy(exe_path, "tools/soh_export", sizeof(exe_path));
        }
    }

    /* Spawn exporter */
    pid_t pid = fork();
    if (pid == 0) {
        char *const argv[] = {
            (char*)exe_path,
            "--sra", (char*)sra_path,
            "--rom", (char*)rom_path,
            "--outdir", "SoH",
            "--slot", "0",
            "--force",
            NULL
        };
        execvp(argv[0], argv);
        _exit(127);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, WNOHANG);
#ifdef DEBUG_SAVES
        DebugMessage(M64MSG_INFO, "SoH export launched: %s", exe_path);
#endif
    }
}
#endif /* ENABLE_SOH_EXPORT */

void format_sram(uint8_t* mem)
{
    memset(mem, 0xff, SRAM_SIZE);
}

void init_sram(struct sram* sram,
               void* storage, const struct storage_backend_interface* istorage, struct rdram* rdram)
{
    sram->storage = storage;
    sram->istorage = istorage;
    sram->rdram = rdram;
}

unsigned int sram_dma_read(void* opaque, const uint8_t* dram, uint32_t dram_addr, uint32_t cart_addr, uint32_t length)
{
    size_t i;
    struct sram* sram = (struct sram*)opaque;
    uint8_t* mem = sram->istorage->data(sram->storage);

    cart_addr &= SRAM_ADDR_MASK;

    for (i = 0; i < length && (cart_addr+i) < SRAM_SIZE; ++i) {
        mem[(cart_addr+i)^S8] = dram[(dram_addr+i)^S8];
    }

    sram->istorage->save(sram->storage, cart_addr, length);

    return /* length / 8 */0x1000;
}

unsigned int sram_dma_write(void* opaque, uint8_t* dram, uint32_t dram_addr, uint32_t cart_addr, uint32_t length)
{
    size_t i;
    struct sram* sram = (struct sram*)opaque;
    const uint8_t* mem = sram->istorage->data(sram->storage);

    cart_addr &= SRAM_ADDR_MASK;

    for (i = 0; i < length && (dram_addr+i) < sram->rdram->dram_size; ++i) {
        dram[(dram_addr+i)^S8] = mem[(cart_addr+i)^S8];
    }

    return /* length / 8 */0x1000;
}

void read_sram(void* opaque, uint32_t address, uint32_t* value)
{
    struct sram* sram = (struct sram*)opaque;
    const uint8_t* mem = sram->istorage->data(sram->storage);

    address &= SRAM_ADDR_MASK;

    *value = *(uint32_t*)(mem + address);
}

void write_sram(void* opaque, uint32_t address, uint32_t value, uint32_t mask)
{
    struct sram* sram = (struct sram*)opaque;
    uint8_t* mem = sram->istorage->data(sram->storage);

    address &= SRAM_ADDR_MASK;

    masked_write((uint32_t*)(mem + address), value, mask);

    sram->istorage->save(sram->storage, address, sizeof(value));

#ifdef ENABLE_SOH_EXPORT
    /* After SRAM write → run exporter */
    const char* rom_path = sram->istorage->get_rom_path ?
                           sram->istorage->get_rom_path(sram->storage) :
                           "UNKNOWN_ROM.z64";

    const char* sra_path = sram->istorage->get_save_filename ?
                           sram->istorage->get_save_filename(sram->storage) :
                           "UNKNOWN.sra";

    soh_export_sram_async(rom_path, sra_path);
#endif
}
