/**
 * @file crc_e2e.c
 * @brief End-to-End CRC-32 Protection Implementation
 * @details Implements CRC-32 (IEEE 802.3 polynomial) for end-to-end
 *          data protection across the radar signal processing chain.
 *
 *          Uses a 256-entry lookup table for fast computation.
 *          Supports both raw CRC calculation and E2E protocol with
 *          sequence counter for freshness checking.
 *
 *          ISO 26262 Part 6, Annex D: E2E protection
 *          IEC 61508 Table A.15: Data validation by CRC
 *
 * @copyright Copyright (c) 2024
 */

#include "crc_e2e.h"
#include "safety_cfg.h"

/* ========================================================================== */
/*                           CRC-32 Lookup Table                              */
/* ========================================================================== */

/** @brief CRC-32 table (polynomial 0x04C11DB7, reflected) */
static const uint32 g_crcTable[256] __attribute__((section(".crc_tables"))) =
{
    0x00000000U, 0x77073096U, 0xEE0E612CU, 0x990951BAU,
    0x076DC419U, 0x706AF48FU, 0xE963A535U, 0x9E6495A3U,
    0x0EDB8832U, 0x79DCB8A4U, 0xE0D5E91BU, 0x97D2D988U,
    0x09B64C2BU, 0x7EB17CBDU, 0xE7B82D09U, 0x90BF1D9FU,
    0x1DB71064U, 0x6AB020F2U, 0xF3B97148U, 0x84BE41DEU,
    0x1ADAD47DU, 0x6DDDE4EBU, 0xF4D4B551U, 0x83D385C7U,
    0x136C9856U, 0x646BA8C0U, 0xFD62F97AU, 0x8A65C9ECU,
    0x14015C4FU, 0x63066CD9U, 0xFA0F3D63U, 0x8D080DF5U,
    0x3B6E20C8U, 0x4C69105EU, 0xD56041E4U, 0xA2677172U,
    0x3C03E4D1U, 0x4B04D447U, 0xD20D85FDU, 0xA50AB56BU,
    0x35B5A8FAU, 0x42B2986CU, 0xDBBBC9D6U, 0xACBCF940U,
    0x32D86CE3U, 0x45DF5C75U, 0xDCD60DCFU, 0xABD13D59U,
    0x26D930ACU, 0x51DE003AU, 0xC8D75180U, 0xBFD06116U,
    0x21B4F6B5U, 0x56B3C423U, 0xCFBA9599U, 0xB8BDA50FU,
    0x2802B89EU, 0x5F058808U, 0xC60CD9B2U, 0xB10BE924U,
    0x2F6F7C87U, 0x58684C11U, 0xC1611DABU, 0xB6662D3DU,
    0x76DC4190U, 0x01DB7106U, 0x98D220BCU, 0xEFD5102AU,
    0x71B18589U, 0x06B6B51FU, 0x9FBFE4A5U, 0xE8B8D433U,
    0x7807C9A2U, 0x0F00F934U, 0x9609A88EU, 0xE10E9818U,
    0x7F6A0DBBU, 0x086D3D2DU, 0x91646C97U, 0xE6635C01U,
    0x6B6B51F4U, 0x1C6C6162U, 0x856530D8U, 0xF262004EU,
    0x6C0695EDU, 0x1B01A57BU, 0x8208F4C1U, 0xF50FC457U,
    0x65B0D9C6U, 0x12B7E950U, 0x8BBEB8EAU, 0xFCB9887CU,
    0x62DD1DDFU, 0x15DA2D49U, 0x8CD37CF3U, 0xFBD44C65U,
    0x4DB26158U, 0x3AB551CEU, 0xA3BC0074U, 0xD4BB30E2U,
    0x4ADFA541U, 0x3DD895D7U, 0xA4D1C46DU, 0xD3D6F4FBU,
    0x4369E96AU, 0x346ED9FCU, 0xAD678846U, 0xDA60B8D0U,
    0x44042D73U, 0x33031DE5U, 0xAA0A4C5FU, 0xDD0D7CC9U,
    0x5005713CU, 0x270241AAU, 0xBE0B1010U, 0xC90C2086U,
    0x5768B525U, 0x206F85B3U, 0xB966D409U, 0xCE61E49FU,
    0x5EDEF90EU, 0x29D9C998U, 0xB0D09822U, 0xC7D7A8B4U,
    0x59B33D17U, 0x2EB40D81U, 0xB7BD5C3BU, 0xC0BA6CADU,
    0xEDB88320U, 0x9ABFB3B6U, 0x03B6E20CU, 0x74B1D29AU,
    0xEAD54739U, 0x9DD277AFU, 0x04DB2615U, 0x73DC1683U,
    0xE3630B12U, 0x94643B84U, 0x0D6D6A3EU, 0x7A6A5AA8U,
    0xE40ECF0BU, 0x9309FF9DU, 0x0A00AE27U, 0x7D079EB1U,
    0xF00F9344U, 0x8708A3D2U, 0x1E01F268U, 0x6906C2FEU,
    0xF762575DU, 0x806567CBU, 0x196C3671U, 0x6E6B06E7U,
    0xFED41B76U, 0x89D32BE0U, 0x10DA7A5AU, 0x67DD4ACCU,
    0xF9B9DF6FU, 0x8EBEEFF9U, 0x17B7BE43U, 0x60B08ED5U,
    0xD6D6A3E8U, 0xA1D1937EU, 0x38D8C2C4U, 0x4FDFF252U,
    0xD1BB67F1U, 0xA6BC5767U, 0x3FB506DDU, 0x48B2364BU,
    0xD80D2BDAU, 0xAF0A1B4CU, 0x36034AF6U, 0x41047A60U,
    0xDF60EFC3U, 0xA8670955U, 0x31682DEFU, 0x46693B79U,
    0xCB61B38CU, 0xBC66831AU, 0x256FD2A0U, 0x5268E236U,
    0xCC0C7795U, 0xBB0B4703U, 0x220216B9U, 0x5505262FU,
    0xC5BA3BBEU, 0xB2BD0B28U, 0x2BB45A92U, 0x5CB36A04U,
    0xC2D7FFA7U, 0xB5D0CF31U, 0x2CD99E8BU, 0x5BDEAE1DU,
    0x9B64C2B0U, 0xEC63F226U, 0x756AA39CU, 0x026D930AU,
    0x9C0906A9U, 0xEB0E363FU, 0x72076785U, 0x05005713U,
    0x95BF4A82U, 0xE2B87A14U, 0x7BB12BAEU, 0x0CB61B38U,
    0x92D28E9BU, 0xE5D5BE0DU, 0x7CDCEFB7U, 0x0BDBDF21U,
    0x86D3D2D4U, 0xF1D4E242U, 0x68DDB3F8U, 0x1FDA836EU,
    0x81BE16CDU, 0xF6B9265BU, 0x6FB077E1U, 0x18B74777U,
    0x88085AE6U, 0xFF0F6B70U, 0x66063BCAU, 0x11010B5CU,
    0x8F659EFFU, 0xF862AE69U, 0x6165D6D3U, 0x16625E45U,
    0x6B04D018U, 0x1C03E08EU, 0x8504F334U, 0xF203C3A2U,
    0x650A7354U, 0x12071F62U, 0x8BBEB82CU, 0xFCBAC8BAU,
    0x6CC4DB59U, 0x1BC3EBCFU, 0x82CAEC75U, 0xF5CDDC63U,
    0x6BD0D3F2U, 0x1CD7E364U, 0x75D08ADEU, 0x02D7BA48U,
    0xBCF0E1A9U, 0xCBF7D13FU, 0x52FEB085U, 0x25F98013U,
    0xBB9D15B0U, 0xCB9A2526U, 0x529374CCU, 0x2594445AU,
    0xB52AB9CBU, 0xC22D895DU, 0x5B24D8E7U, 0x2C23E871U,
    0xB2477DD2U, 0xC5404D44U, 0x5C491CFEU, 0x2B4E2C68U
};

/* ========================================================================== */
/*                           Module Variables                                 */
/* ========================================================================== */

/** @brief E2E sequence counter */
static uint32 g_sequenceCounter = 0U;

/* ========================================================================== */
/*                       Public Functions                                     */
/* ========================================================================== */

/**
 * @brief Initialize E2E CRC module
 */
void CRC_E2E_Init(void)
{
    g_sequenceCounter = 0U;
}

/**
 * @brief Calculate CRC-32 over a data buffer
 * @param pData Pointer to data
 * @param length Data length in bytes
 * @return CRC-32 value
 */
uint32 CRC_E2E_Calculate(const uint8 *pData, uint32 length)
{
    uint32 crc = E2E_CRC_INIT_VALUE;
    uint32 i;

    if (pData == NULL_PTR)
    {
        return 0U;
    }

    for (i = 0U; i < length; i++)
    {
        uint8 tableIdx = (uint8)((crc ^ pData[i]) & 0xFFU);
        crc = (crc >> 8U) ^ g_crcTable[tableIdx];
    }

    return crc ^ E2E_CRC_XOR_OUT;
}

/**
 * @brief Verify CRC-32 of a data buffer
 * @param pData Pointer to data
 * @param length Data length in bytes
 * @param expectedCrc Expected CRC-32 value
 * @return STATUS_OK if CRC matches
 */
Status_t CRC_E2E_Verify(const uint8 *pData, uint32 length, uint32 expectedCrc)
{
    uint32 computedCrc;

    if (pData == NULL_PTR)
    {
        return STATUS_ERROR;
    }

    computedCrc = CRC_E2E_Calculate(pData, length);

    if (computedCrc == expectedCrc)
    {
        return STATUS_OK;
    }

    return STATUS_ERROR;
}

/**
 * @brief Protect a data packet with E2E header (CRC + sequence counter)
 * @param pHeader Pointer to E2E header to fill
 * @param pData Pointer to payload data
 * @param length Payload length in bytes
 * @return STATUS_OK on success
 */
Status_t CRC_E2E_Protect(E2E_Header_t *pHeader, const uint8 *pData, uint32 length)
{
    if ((pHeader == NULL_PTR) || (pData == NULL_PTR))
    {
        return STATUS_ERROR;
    }

    /* Set sequence counter for freshness */
    pHeader->sequenceCounter = g_sequenceCounter;
    pHeader->dataLength = length;

    /* Compute CRC over header fields + payload */
    uint32 crc = E2E_CRC_INIT_VALUE;
    uint32 i;

    /* CRC over sequence counter */
    const uint8 *pSeq = (const uint8 *)&pHeader->sequenceCounter;
    for (i = 0U; i < sizeof(uint32); i++)
    {
        uint8 tableIdx = (uint8)((crc ^ pSeq[i]) & 0xFFU);
        crc = (crc >> 8U) ^ g_crcTable[tableIdx];
    }

    /* CRC over data length */
    const uint8 *pLen = (const uint8 *)&pHeader->dataLength;
    for (i = 0U; i < sizeof(uint32); i++)
    {
        uint8 tableIdx = (uint8)((crc ^ pLen[i]) & 0xFFU);
        crc = (crc >> 8U) ^ g_crcTable[tableIdx];
    }

    /* CRC over payload */
    for (i = 0U; i < length; i++)
    {
        uint8 tableIdx = (uint8)((crc ^ pData[i]) & 0xFFU);
        crc = (crc >> 8U) ^ g_crcTable[tableIdx];
    }

    pHeader->crc = crc ^ E2E_CRC_XOR_OUT;

    /* Increment sequence counter with wrap */
    g_sequenceCounter++;
    if (g_sequenceCounter > E2E_SEQUENCE_COUNTER_MAX)
    {
        g_sequenceCounter = 0U;
    }

    return STATUS_OK;
}

/**
 * @brief Verify E2E protected data packet
 * @param pHeader Pointer to E2E header
 * @param pData Pointer to payload
 * @param length Expected payload length
 * @return STATUS_OK if CRC and length match
 */
Status_t CRC_E2E_Check(const E2E_Header_t *pHeader, const uint8 *pData,
                        uint32 length)
{
    uint32 crc;
    uint32 i;

    if ((pHeader == NULL_PTR) || (pData == NULL_PTR))
    {
        return STATUS_ERROR;
    }

    /* Verify data length */
    if (pHeader->dataLength != length)
    {
        return STATUS_ERROR;
    }

    /* Recompute CRC */
    crc = E2E_CRC_INIT_VALUE;

    const uint8 *pSeq = (const uint8 *)&pHeader->sequenceCounter;
    for (i = 0U; i < sizeof(uint32); i++)
    {
        uint8 tableIdx = (uint8)((crc ^ pSeq[i]) & 0xFFU);
        crc = (crc >> 8U) ^ g_crcTable[tableIdx];
    }

    const uint8 *pLen = (const uint8 *)&pHeader->dataLength;
    for (i = 0U; i < sizeof(uint32); i++)
    {
        uint8 tableIdx = (uint8)((crc ^ pLen[i]) & 0xFFU);
        crc = (crc >> 8U) ^ g_crcTable[tableIdx];
    }

    for (i = 0U; i < length; i++)
    {
        uint8 tableIdx = (uint8)((crc ^ pData[i]) & 0xFFU);
        crc = (crc >> 8U) ^ g_crcTable[tableIdx];
    }

    crc ^= E2E_CRC_XOR_OUT;

    if (crc != pHeader->crc)
    {
        return STATUS_ERROR;
    }

    return STATUS_OK;
}
