/**
 * @file ecc_handler.h
 * @brief ECC (Error Correction Code) Handler Interface
 * @details SEC (Single Error Correction) / DED (Double Error Detection)
 *          handler for TCMA, TCMB, L2RAM, and VIM RAM.
 * @copyright Copyright (c) 2024
 */
#ifndef ECC_HANDLER_H
#define ECC_HANDLER_H

#include "platform_types.h"

Status_t     ECC_Init(void);
DiagResult_t ECC_SelfTest(void);
DiagResult_t ECC_GetDiagStatus(void);
void         ECC_SEC_ISR(void);
void         ECC_DED_ISR(void);
uint32       ECC_GetSECCount(void);
uint32       ECC_GetDEDCount(void);
Status_t     ECC_InjectError(uint32 memRegion, boolean doubleBit);

#endif /* ECC_HANDLER_H */
