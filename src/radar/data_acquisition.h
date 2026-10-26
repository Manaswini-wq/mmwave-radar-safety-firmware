/**
 * @file data_acquisition.h
 * @brief High-Speed Data Acquisition Interface
 * @details DMA-based ADC data capture with ping-pong buffering and
 *          E2E CRC protection for the AWR2944 radar data path.
 * @copyright Copyright (c) 2024
 */
#ifndef DATA_ACQUISITION_H
#define DATA_ACQUISITION_H

#include "platform_types.h"
#include "safety/crc_e2e.h"

#define DATA_ACQ_MAX_SAMPLES   1024U
#define DATA_ACQ_NUM_RX_CH     4U
#define DATA_ACQ_BUFFER_SIZE   (DATA_ACQ_MAX_SAMPLES * DATA_ACQ_NUM_RX_CH * 4U)

typedef struct {
    uint32      bufferIdx;
    uint32      *pData;
    uint32      dataSize;
    E2E_Header_t e2eHeader;
    boolean     ready;
} DataAcq_Buffer_t;

Status_t         DataAcq_Init(void);
DataAcq_Buffer_t *DataAcq_GetReadyBuffer(void);
Status_t         DataAcq_ValidateCRC(const DataAcq_Buffer_t *pBuffer);
void             DataAcq_QueueForTransfer(DataAcq_Buffer_t *pBuffer);
void             DataAcq_SwapBuffers(void);
void             DataAcq_NotifyHost(void);

#endif /* DATA_ACQUISITION_H */
