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


#ifndef __MSG_DFT_H_
#define __MSG_DFT_H_

/**
 * @defgroup MSG_DFT_MODS_NSS Diversity Settings
 * @ingroup MODS_NSS_MSG
 * The message handler module allows the application to enable or disable certain behavior through the use diversity
 * flags in the form of defines. Sensible defaults are chosen; to deviate, simply define the relevant flags and message
 * ids in your application before including the message handler module.
 *
 * @par Flags
 *  - Flags to enable application specific commands and responses:
 *      - @ref msg_anchor_handlers "MSG_APP_HANDLERS",
 *      - #MSG_APP_HANDLERS_COUNT and
 *      - @ref msg_anchor_catchall_handler "MSG_CATCHALL_HANDLER".
 *      .
 *  .
 *
 * @par Special message ids
 *  - #MSG_ID_GETVERSION is a special message id, as it is always enabled (see #MSG_ENABLE_GETVERSION). @n
 *      There are two related flags for this id:
 *      - #SW_MAJOR_VERSION and
 *      - #SW_MINOR_VERSION.
 *      .
 *  - #MSG_ID_GETRESPONSE is a special message id, as it is automatically enabled when you expand the functionality of
 *      the message handler module to use a buffer, i.e. by defining both
 *      - @ref msg_anchor_response_buffer "MSG_RESPONSE_BUFFER" and
 *      - #MSG_RESPONSE_BUFFER_SIZE,
 *      .
 *  .
 *
 * @par Additional hooks to control the flow
 *  - @ref msg_anchor_command_accept_cb "MSG_COMMAND_ACCEPT_CB",
 *  - @ref msg_anchor_response_discarded_cb "MSG_RESPONSE_DISCARDED_CB".
 *  .
 *
 * @par Other message ids
 *  All other predefined message id's can be enabled or remain disabled without additional consequences:
 *      - #MSG_ENABLE_RESET
 *      - #MSG_ENABLE_READREGISTER
 *      - #MSG_ENABLE_WRITEREGISTER
 *      - #MSG_ENABLE_READMEMORY
 *      - #MSG_ENABLE_WRITEMEMORY
 *      - #MSG_ENABLE_PREPAREDEBUG
 *      - #MSG_ENABLE_GETUID
 *      .
 *
 * @par Example
 *  To override each and every diversity flag that the message handler module offers, create defines that are known to
 *  the precompiler before including the message handler module.
 *  The recommended way to do this is to add the code below to @c app_sel.h
 *  @code
 *      #define MSG_APP_HANDLERS_COUNT 15
 *      #define MSG_APP_HANDLERS <name of MSG_CMD_HANDLER_T array>
 *      #define MSG_CATCHALL_HANDLER <name of pMsg_CmdHandler_t function>
 *      #define SW_MAJOR_VERSION 9
 *      #define SW_MINOR_VERSION 2
 *      #define MSG_RESPONSE_BUFFER_SIZE 65
 *      #define MSG_RESPONSE_BUFFER <name of uint8_t buffer>
 *      #define MSG_COMMAND_ACCEPT_CB <name of pMsg_AcceptCommandCb_t function>
 *      #define MSG_RESPONSE_DISCARDED_CB <name of pMsg_ResponseCb_t function>
 *      #define MSG_ENABLE_RESET 1
 *      #define MSG_ENABLE_READREGISTER 1
 *      #define MSG_ENABLE_WRITEREGISTER 1
 *      #define MSG_ENABLE_READMEMORY 1
 *      #define MSG_ENABLE_WRITEMEMORY 1
 *  @endcode
 *
 * @{
 */

#ifndef MSG_APP_HANDLERS_COUNT
    /**
     * @pre To define custom command handlers, both @c MSG_APP_HANDLERS_COUNT and
     *  @ref msg_anchor_handlers "MSG_APP_HANDLERS" must be defined.
     * @pre @c MSG_APP_HANDLERS_COUNT must be a strict positive number indicating the size of the array
     *  @ref msg_anchor_handlers "MSG_APP_HANDLERS"
     *
     * @anchor msg_anchor_handlers
     * @par #define MSG_APP_HANDLERS
     *  To define custom command handlers, both #MSG_APP_HANDLERS_COUNT and @ref msg_anchor_handlers "MSG_APP_HANDLERS"
     *  must be defined.
     *  @pre @ref msg_anchor_handlers "MSG_APP_HANDLERS" must be an array with elements of type #MSG_CMD_HANDLER_T
     */
    #define MSG_APP_HANDLERS_COUNT 0
#endif
#if !MSG_APP_HANDLERS_COUNT
    #undef MSG_APP_HANDLERS
#endif
#if MSG_APP_HANDLERS_COUNT && !defined(MSG_APP_HANDLERS)
    #error MSG_APP_HANDLERS and MSG_APP_HANDLERS_COUNT must be defined jointly.
#endif

/* ------------------------------------------------------------------------- */

/**
 * The command/response for #MSG_ID_GETVERSION is enforced and can not be disabled.
 * @see MSG_ID_GETVERSION
 */
#define MSG_ENABLE_GETVERSION 1

/**
 * Use the major version to distinguish between applications and/or application types.
 * This define is used when generating the response to #MSG_ID_GETVERSION
 */
#if !SW_MAJOR_VERSION
    #define SW_MAJOR_VERSION 0
#endif

/**
 * Use the minor version to distinguish between application updates.
 * This define is used when generating the response to #MSG_ID_GETVERSION
 */
#if !SW_MINOR_VERSION
    #define SW_MINOR_VERSION 0
#endif

/* ------------------------------------------------------------------------- */

#ifndef MSG_RESPONSE_BUFFER_SIZE
    /**
     * Responses that do not get treated immediately can be stored in an internal buffer.
     * @pre To define a buffer to be used by the message handler module, both @c MSG_RESPONSE_BUFFER_SIZE and
     *  @ref msg_anchor_response_buffer "MSG_RESPONSE_BUFFER" must be defined.
     * Define here the size of the buffer.
     * @note responses greater than this size can not be stored.
     * @note @ref msg_anchor_response_discarded_cb "MSG_RESPONSE_DISCARDED_CB" will also be used whenever a response is
     *  not immediately accepted and:
     *  - no buffer is set and.
     *  - the response size is bigger than the buffer size.
     *  - the response size is bigger than @c 255 bytes: This is a limitation fully due to the implementation how the
     *      responses are stored in the internal buffer.
     *  .
     * @note Defining @c MSG_RESPONSE_BUFFER_SIZE will automatically enable the command/response for #MSG_ID_GETRESPONSE
     * @see pMsg_ResponseCb_t
     *
     * @anchor msg_anchor_response_buffer
     * @par #define MSG_RESPONSE_BUFFER
     *  Responses that do not get treated immediately can be stored in an internal buffer. To define a buffer to be used
     *  by the message handler module, both #MSG_RESPONSE_BUFFER_SIZE and
     *  @ref msg_anchor_response_buffer "MSG_RESPONSE_BUFFER" must be defined. Define here the location of the buffer.
     *  @note There are no alignment requirements.
     *  @note The value set must match type #pMsg_ResponseCb_t
     *  @note Defining #MSG_RESPONSE_BUFFER_SIZE will automatically enable the command/response for #MSG_ID_GETRESPONSE
     */
    #define MSG_RESPONSE_BUFFER_SIZE 0
#endif
#if !MSG_RESPONSE_BUFFER_SIZE
    #undef MSG_RESPONSE_BUFFER
#endif
#if MSG_RESPONSE_BUFFER_SIZE && !defined(MSG_RESPONSE_BUFFER)
    #error MSG_RESPONSE_BUFFER and MSG_RESPONSE_BUFFER_SIZE must be defined jointly.
#endif

/**
 * @anchor msg_anchor_command_accept_cb
 * @par #define MSG_COMMAND_ACCEPT_CB
 *  Adds a hook inside #Msg_HandleCommand that allows to block handling each individual command.
 *  @note The value set must match type #pMsg_AcceptCommandCb_t
 */

/**
 * This command is automatically enabled when #MSG_RESPONSE_BUFFER_SIZE is set.
 * @see @ref msg_anchor_response_buffer "MSG_RESPONSE_BUFFER"
 * @see MSG_ID_GETRESPONSE
 *
 * @anchor msg_anchor_catchall_handler
 * @par #define MSG_CATCHALL_HANDLER
 *  When the flexibility offered at compile time with @ref msg_anchor_handlers "MSG_APP_HANDLERS" and
 *  #MSG_APP_HANDLERS_COUNT is not enough, a callback can be defined where all untreated commands are directed to. The
 *  upper layer can then dynamically decide to treat certain commands or not.
 *  @note The value set must match type #pMsg_CmdHandler_t
 *  @note This must be set to the name of a function, not a pointer to a function: no dereference will be made!
 *
 * @anchor msg_anchor_response_discarded_cb
 * @par #define MSG_RESPONSE_DISCARDED_CB
 *  Responses that do not get treated immediately can be stored in an internal buffer, and they can be fetched later
 *  using #MSG_ID_GETRESPONSE. Whenever this buffer runs full, the oldest response is discarded. To get notified when
 *  a response is being discarded, set this define to the callback to be called.
 *  @note The value set must match type #pMsg_ResponseCb_t
 *  @note This must be set to the name of a function, not a pointer to a function: no dereference will be made!
 *  @note This callback will also be used when no buffer is set and whenever a response is not immediately
 *      accepted.
 */
#define MSG_ENABLE_GETRESPONSE (MSG_RESPONSE_BUFFER_SIZE > 0)
//#define MSG_CATCHALL_HANDLER
//#define MSG_RESPONSE_DISCARDED_CB

/* ------------------------------------------------------------------------- */

/**
 * Assign a non-zero value to enable the handling of the command #MSG_ID_RESET.
 */
#if !MSG_ENABLE_RESET
   #define MSG_ENABLE_RESET 0
#endif

/**
 * Assign a non-zero value to enable the handling of the command #MSG_ID_READREGISTER.
 * @warning Should be disabled in production code; may be useful for debugging and testing purposes.
 */
#if !MSG_ENABLE_READREGISTER
   #define MSG_ENABLE_READREGISTER 0
#endif

/**
 * Assign a non-zero value to enable the handling of the command #MSG_ID_WRITEREGISTER.
 * @warning Should be disabled in production code; may be useful for debugging and testing purposes.
 */
#if !MSG_ENABLE_WRITEREGISTER
    #define MSG_ENABLE_WRITEREGISTER 0
#endif

/**
 * Assign a non-zero value to enable the handling of the command #MSG_ID_READMEMORY.
 * @warning Should be disabled in production code; may be useful for debugging and testing purposes.
 */
#if !MSG_ENABLE_READMEMORY
    #define MSG_ENABLE_READMEMORY 0
#endif

/**
 * Assign a non-zero value to enable the handling of the command #MSG_ID_WRITEMEMORY.
 * @warning Should be disabled in production code; may be useful for debugging and testing purposes.
 */
#if !MSG_ENABLE_WRITEMEMORY
    #define MSG_ENABLE_WRITEMEMORY 0
#endif

/**
 * Assign a non-zero value to enable the handling of the command #MSG_ID_PREPAREDEBUG.
 * @warning Should be disabled in production code; may be useful for debugging and testing purposes.
 */
#if !MSG_ENABLE_PREPAREDEBUG
    #define MSG_ENABLE_PREPAREDEBUG 0
#endif

/**
 * Assign a non-zero value to enable the handling of the command #MSG_ID_GETUID.
 */
#if !MSG_ENABLE_GETUID
    #define MSG_ENABLE_GETUID 0
#endif

/** @} */

#endif
