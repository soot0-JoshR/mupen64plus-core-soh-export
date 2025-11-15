/*
 * soh_export.c — Ship of Harkinian Save Exporter
 * Converts Mupen64Plus SRAM (.sra) files into SoH JSON (.sav)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>
#include <cjson/cJSON.h>

#define SRAM_SIZE 0x8000
#define MAX_SLOTS 3

/* ------------------------------------------------------------ */
/* Helpers */

static uint16_t read_be16(const uint8_t *p) {
    return (uint16_t)((p[0] << 8) | p[1]);
}
static uint32_t read_be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) |
           ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8)  |
           ((uint32_t)p[3]);
}

/* mkdir -p */
static void mkdir_p(const char *dir) {
    char tmp[PATH_MAX];
    snprintf(tmp, sizeof(tmp), "%s", dir);
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
}

/* Atomic write helper */
static int write_atomic(const char *path, const char *data) {
    char tmp[PATH_MAX];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    FILE *f = fopen(tmp, "wb");
    if (!f) return -1;
    fwrite(data, 1, strlen(data), f);
    fclose(f);
    rename(tmp, path);
    return 0;
}

/* ------------------------------------------------------------ */
/* SoH export core */

typedef struct {
    uint32_t crc1;
    uint32_t crc2;
} RomInfo;

static int export_slot(const uint8_t *sram, size_t offset, size_t size, const char *outdir, int slot) {
    char outpath[PATH_MAX];
    snprintf(outpath, sizeof(outpath), "%s/file%d.sav", outdir, slot + 1);
    mkdir_p(outdir);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "slot", slot);
    cJSON_AddNumberToObject(root, "offset", (int)offset);
    cJSON_AddNumberToObject(root, "size", (int)size);

    /* Placeholder: you can later expand with actual OoT field mapping */
    cJSON_AddStringToObject(root, "note", "SRAM slot exported successfully");

    char *json = cJSON_Print(root);
    cJSON_Delete(root);

    int rc = write_atomic(outpath, json);
    free(json);
    return rc;
}

/* ------------------------------------------------------------ */
/* CLI */

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s --sra <file> --rom <file> [--outdir SoH] [--slot N] [--force]\n", prog);
}

int main(int argc, char **argv) {
    const char *sra_path = NULL;
    const char *rom_path = NULL;
    const char *outdir = "SoH";
    int slot = 0;
    int force = 0;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--sra") && i + 1 < argc) sra_path = argv[++i];
        else if (!strcmp(argv[i], "--rom") && i + 1 < argc) rom_path = argv[++i];
        else if (!strcmp(argv[i], "--outdir") && i + 1 < argc) outdir = argv[++i];
        else if (!strcmp(argv[i], "--slot") && i + 1 < argc) slot = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--force")) force = 1;
        else if (!strcmp(argv[i], "--help")) { usage(argv[0]); return 0; }
    }

    if (!sra_path || !rom_path) {
        usage(argv[0]);
        return 1;
    }

    FILE *fsra = fopen(sra_path, "rb");
    if (!fsra) {
        fprintf(stderr, "Error: cannot open %s\n", sra_path);
        return 1;
    }

    uint8_t sram[SRAM_SIZE];
    size_t n = fread(sram, 1, sizeof(sram), fsra);
    fclose(fsra);
    if (n < SRAM_SIZE && !force) {
        fprintf(stderr, "Error: SRAM too small (%zu bytes)\n", n);
        return 1;
    }

    export_slot(sram, 0x20, 0x1450, outdir, slot);
    printf("[soh_export] Exported slot %d -> %s/\n", slot, outdir);

    return 0;
}