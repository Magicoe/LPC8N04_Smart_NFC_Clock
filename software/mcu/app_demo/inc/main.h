/*
 * Copyright (c), NXP Semiconductors
 * (C)NXP B.V. 2014-2018
 * All rights are reserved. Reproduction in whole or in part is prohibited without
 * the written consent of the copyright owner. NXP reserves the right to make
 * changes without notice at any time. NXP makes no warranty, expressed, implied or
 * statutory, including but not limited to any implied warranty of merchantability
 * or fitness for any particular purpose, or that the use will not infringe any
 * third party patent, copyright or trademark. NXP must not be liable for any loss
 * or damage arising from its use.
 */

#ifndef __MAIN_H__
#define __MAIN_H__

/* -------------------------------------------------------------------------
 * defines
 * ------------------------------------------------------------------------- */

/**
 * Just assign a reasonable number. It must accommodate for all the overhead that comes with the complete ndef
 * message, plus it must be a multiple of 4.
 */
#define MAX_COMMAND_MESSAGE_SIZE    512

/**
 * Allow the - assumed - corresponding host to issue a first command in the given time window.
 * A few seconds also allows for easier grabbing hold of a debug session.
 */
#define FIRST_HOST_TIMEOUT          100

/**
 * We're assuming the host will continually exchange commands and responses. If after some timeout no command
 * has been received, we shut down abort communication. When a field is still present, we will automatically
 * wake up again from Deep Power Down and refresh the NFC shared memory with a new initial message.
 * This way we prevent a possible hang-up when both sides are waiting indefinitely for an NDEF message to be
 * written by the other side.
 * No need to set the timeout too strict: set it reasonably long enough to never hamper any execution flow,
 * while still being short enough to re-enable communication from a user perspective.
 */
#define HOST_TIMEOUT                100

/**
 * From what was observed, the power off/power on/(re-)select sequence takes place in the order of a few 100ms
 * at most. Waiting a full second seems more than enough to also take small changes in physical placement into
 * account.
 */
#define LAST_HOST_TIMEOUT           100

/**
 * No need to set the watchdog timeout too strict: set it long enough to never hamper any execution flow,
 * while still being short enough to re-enable communication from a user perspective.
 * Just ensure it is higher than #HOST_TIMEOUT.
 */
#define WATCHDOG_TIMEOUT            (HOST_TIMEOUT + 5)

/**
 * The number of watchdog ticks per second is equal to SFRO freq / clock divider / 4.
 * See @ref WWDTClockingAndTimeout_anchor.
 */
#define WATCHDOG_TICKS(clockDivider, timeout) (8*1000*1000 / 254 / 4 * (timeout))


#define EE_HEADER_SIZE              (4U)
#define EE_PAGE_SIZE                (64U)

// System alive timing when powered by external, minutes
#define WAKEUP_MINS                 (2)

#endif
