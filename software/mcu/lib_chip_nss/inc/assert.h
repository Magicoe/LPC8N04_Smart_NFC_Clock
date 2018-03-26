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


#ifndef __ASSERT_H_
#define __ASSERT_H_

/**
 * @addtogroup CHIP_NSS
 * @{
 */

/**
 * If condition @c expr is false, the debugger halts (BKPT instruction), otherwise does nothing.
 * @param expr : the condition to check
 * @note If the macro @c DEBUG is not defined, the #ASSERT macro generates no code.
 * @note If the expression @c expr has side-effects, the program behaves differently depending on whether @c DEBUG is
 *  defined.
 */
#ifdef DEBUG
    #define ASSERT(expr) do { if (expr) {} else { __asm__("BKPT"); } } while (0)
#else
    /* Just to prevent compiler warning for unused variables */
    #define ASSERT(expr)
#endif

/** @} */

#endif /* __ASSERT_H_ */
