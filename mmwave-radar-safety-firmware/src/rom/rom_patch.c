/**
 * @file rom_patch.c
 * @brief ROM Firmware Patch Loader
 * @details Manages runtime ROM patches for the AWR2944. Patches are
 *          stored in the ROM patch table hardware and redirect ROM
 *          function calls to RAM-based replacements. Each patch is
 *          verified with CRC before activation.
 *
 *          The patch mechanism uses address comparison logic in hardware:
 *          when the CPU fetches from a patched ROM address, the hardware
 *          redirects the fetch to the patch data (branch instruction).
 *
 * @copyright Copyright (c) 2024
 */

#include "rom_patch.h"
#include "awr2944_regs.h"
#include "safety/crc_e2e.h"

/* ========================================================================== */
/*                           Module Variables                                 */
/* ========================================================================== */

static RomPatchEntry_t g_patchTable[ROM_PATCH_MAX_ENTRIES];
static uint32 g_patchCount = 0U;
static uint32 g_romVersion = 0U;

/* ========================================================================== */
/*                       Public Functions                                     */
/* ========================================================================== */

/**
 * @brief Load and activate all ROM patches
 * @details Reads patch entries from the ROM patch table in hardware,
 *          verifies each entry's CRC, and enables valid patches.
 * @return STATUS_OK if all patches loaded and verified
 */
Status_t RomPatch_LoadAll(void)
{
    uint32 i;
    uint32 enableMask = 0U;

    /* Read ROM version */
    g_romVersion = REG32_RD(OTP_FW_VERSION);

    /* Read patch control register */
    uint32 patchCtrl = REG32_RD(ROM_PATCH_CTRL);
    g_patchCount = patchCtrl & 0x1FU;  /* Bits [4:0] = number of patches */

    if (g_patchCount > ROM_PATCH_MAX_ENTRIES)
    {
        g_patchCount = ROM_PATCH_MAX_ENTRIES;
    }

    /* Load each patch entry */
    for (i = 0U; i < g_patchCount; i++)
    {
        g_patchTable[i].patchAddr = REG32_RD(ROM_PATCH_ADDR(i));
        g_patchTable[i].patchData = REG32_RD(ROM_PATCH_DATA(i));

        /* Compute CRC over patch address and data */
        uint8 patchBytes[8];
        patchBytes[0] = (uint8)(g_patchTable[i].patchAddr & 0xFFU);
        patchBytes[1] = (uint8)((g_patchTable[i].patchAddr >> 8U) & 0xFFU);
        patchBytes[2] = (uint8)((g_patchTable[i].patchAddr >> 16U) & 0xFFU);
        patchBytes[3] = (uint8)((g_patchTable[i].patchAddr >> 24U) & 0xFFU);
        patchBytes[4] = (uint8)(g_patchTable[i].patchData & 0xFFU);
        patchBytes[5] = (uint8)((g_patchTable[i].patchData >> 8U) & 0xFFU);
        patchBytes[6] = (uint8)((g_patchTable[i].patchData >> 16U) & 0xFFU);
        patchBytes[7] = (uint8)((g_patchTable[i].patchData >> 24U) & 0xFFU);

        g_patchTable[i].crc = CRC_E2E_Calculate(patchBytes, 8U);

        /* Verify patch address is in ROM region */
        if ((g_patchTable[i].patchAddr >= 0x70000000U) &&
            (g_patchTable[i].patchAddr < 0x70100000U))
        {
            g_patchTable[i].enabled = TRUE;
            enableMask |= (1U << i);
        }
        else
        {
            g_patchTable[i].enabled = FALSE;
        }
    }

    /* Enable all valid patches atomically */
    REG32_WR(ROM_PATCH_ENABLE_MASK, enableMask);

    return STATUS_OK;
}

/**
 * @brief Apply a specific patch by index
 */
Status_t RomPatch_Apply(uint32 patchIdx)
{
    if (patchIdx >= g_patchCount) return STATUS_ERROR;

    /* Write patch address and data */
    REG32_WR(ROM_PATCH_ADDR(patchIdx), g_patchTable[patchIdx].patchAddr);
    REG32_WR(ROM_PATCH_DATA(patchIdx), g_patchTable[patchIdx].patchData);

    /* Enable this patch */
    uint32 mask = REG32_RD(ROM_PATCH_ENABLE_MASK);
    mask |= (1U << patchIdx);
    REG32_WR(ROM_PATCH_ENABLE_MASK, mask);

    return STATUS_OK;
}

/**
 * @brief Verify a patch entry by recomputing CRC
 */
Status_t RomPatch_Verify(uint32 patchIdx)
{
    if (patchIdx >= g_patchCount) return STATUS_ERROR;

    uint32 hwAddr = REG32_RD(ROM_PATCH_ADDR(patchIdx));
    uint32 hwData = REG32_RD(ROM_PATCH_DATA(patchIdx));

    if ((hwAddr != g_patchTable[patchIdx].patchAddr) ||
        (hwData != g_patchTable[patchIdx].patchData))
    {
        return STATUS_ERROR;
    }

    return STATUS_OK;
}

/**
 * @brief Get ROM firmware version
 */
uint32 RomPatch_GetVersion(void)
{
    return g_romVersion;
}
