/**
 * @file rom_patch.h
 * @brief ROM Firmware Patch Loader Interface
 * @copyright Copyright (c) 2024
 */
#ifndef ROM_PATCH_H
#define ROM_PATCH_H

#include "platform_types.h"

#define ROM_PATCH_MAX_ENTRIES  32U

typedef struct {
    uint32 patchAddr;
    uint32 patchData;
    uint32 crc;
    boolean enabled;
} RomPatchEntry_t;

Status_t RomPatch_LoadAll(void);
Status_t RomPatch_Apply(uint32 patchIdx);
Status_t RomPatch_Verify(uint32 patchIdx);
uint32   RomPatch_GetVersion(void);

#endif /* ROM_PATCH_H */
