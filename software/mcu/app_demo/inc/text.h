/*
 * Copyright (c), NXP Semiconductors
 * (C)NXP B.V. 2014-2017
 * All rights are reserved. Reproduction in whole or in part is prohibited without
 * the written consent of the copyright owner. NXP reserves the right to make
 * changes without notice at any time. NXP makes no warranty, expressed, implied or
 * statutory, including but not limited to any implied warranty of merchantability
 * or fitness for any particular purpose, or that the use will not infringe any
 * third party patent, copyright or trademark. NXP must not be liable for any loss
 * or damage arising from its use.
 */


#ifndef TEXT_H_
#define TEXT_H_

#include <stdbool.h>

/* -------------------------------------------------------------------------------- */

#define TEXT_STATUS_LENGTH 107

const char * Text_GetState(void);

void Text_SetState(int temperature, bool valid, int minimum, int maximum);

#endif
