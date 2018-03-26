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


/* -------------------------------------------------------------------------
 * Include files
 * ------------------------------------------------------------------------- */
#include <string.h>
#include "chip.h"
#include "msg/msg.h"

/* -------------------------------------------------------------------------
 * Types & defines
 * ------------------------------------------------------------------------- */
/** Directionality byte value for commands coming into message mod */
#define MSG_DIRECTION_INCOMING 0x0
/** Directionality byte value for responses going out from message mod */
#define MSG_DIRECTION_OUTGOING 0x1
/** Size of message header in bytes for message Id and directionality byte */
#define MSG_HEADER_SIZE 2

/** Special value for length used in the response buffer. @see spResponseBuffer */
#define RESPONSE_SIZE_SKIP_TO_END 0xFF

/* -------------------------------------------------------------------------
 * Private function prototypes
 * ------------------------------------------------------------------------- */

#if MSG_ENABLE_GETRESPONSE
static uint32_t GetResponseHandler(uint8_t msgId, int payloadLen, const uint8_t* pPayload);
#endif
#if MSG_ENABLE_GETVERSION
static uint32_t GetVersionHandler(uint8_t msgId, int payloadLen, const uint8_t* pPayload);
#endif
#if MSG_ENABLE_RESET
static uint32_t ResetHandler(uint8_t msgId, int payloadLen, const uint8_t* pPayload);
#endif
#if MSG_ENABLE_READREGISTER
static uint32_t ReadRegisterHandler(uint8_t msgId, int payloadLen, const uint8_t* pPayload);
#endif
#if MSG_ENABLE_WRITEREGISTER
static uint32_t WriteRegisterHandler(uint8_t msgId, int payloadLen, const uint8_t* pPayload);
#endif
#if MSG_ENABLE_READMEMORY
static uint32_t ReadMemoryHandler(uint8_t msgId, int payloadLen, const uint8_t* pPayload);
#endif
#if MSG_ENABLE_WRITEMEMORY
static uint32_t WriteMemoryHandler(uint8_t msgId, int payloadLen, const uint8_t* pPayload);
#endif
#if MSG_ENABLE_PREPAREDEBUG
static uint32_t PrepareDebugHandler(uint8_t msgId, int payloadLen, const uint8_t* pPayload);
#endif
#if MSG_ENABLE_GETUID
static uint32_t GetUidHandler(uint8_t msgId, int payloadLen, const uint8_t* pPayload);
#endif

static uint32_t DispatchCommand(uint8_t msgId, int payloadLen, const uint8_t* pPayload,
                                const MSG_CMD_HANDLER_T * handler, int handlerCount);

/* -------------------------------------------------------------------------
 * Private variables
 * ------------------------------------------------------------------------- */

static pMsg_ResponseCb_t sResponseCb;

/**
 * All command handlers must return #MSG_OK and must call #Msg_AddResponse themselves.
 */
static MSG_CMD_HANDLER_T sCmdHandler[] = {
#if MSG_ENABLE_GETRESPONSE
    {MSG_ID_GETRESPONSE, GetResponseHandler},
#endif
#if MSG_ENABLE_GETVERSION
    {MSG_ID_GETVERSION, GetVersionHandler},
#endif
#if MSG_ENABLE_RESET
    {MSG_ID_RESET, ResetHandler},
#endif
#if MSG_ENABLE_READREGISTER
    {MSG_ID_READREGISTER, ReadRegisterHandler},
#endif
#if MSG_ENABLE_WRITEREGISTER
    {MSG_ID_WRITEREGISTER, WriteRegisterHandler},
#endif
#if MSG_ENABLE_READMEMORY
    {MSG_ID_READMEMORY, ReadMemoryHandler},
#endif
#if MSG_ENABLE_WRITEMEMORY
    {MSG_ID_WRITEMEMORY, WriteMemoryHandler},
#endif
#if MSG_ENABLE_PREPAREDEBUG
    {MSG_ID_PREPAREDEBUG, PrepareDebugHandler},
#endif
#if MSG_ENABLE_GETUID
    {MSG_ID_GETUID, GetUidHandler},
#endif
};

#if MSG_RESPONSE_BUFFER_SIZE
/**
 * The buffer is used to store responses.
 *  - @c spOldestResponse points to the oldest response: one byte indicating the size of the response, followed by that
 *      many response bytes. When the size is 0, no responses are available.
 *  - @c spNextResponse points to the first free byte where the next response can be stored.
 *  - When no responses are available, @c spNextResponse equals @c spOldestResponse. And vice-versa, when
 *      @c spNextResponse equals @c spOldestResponse, the buffer is completely empty.
 *  - The buffer is used cyclically. When the buffer is full, the oldest response is automatically discarded.
 *  - Responses are always stored in one contiguous block of memory. When the size byte equals
 *      #RESPONSE_BUFFER_SKIP_TO_END the next response can be found at @c spResponseBuffer[0].
 *  - When any public function is left, @c *spNextResponse is always 0.
 * @{
 */
static uint8_t * spResponseBuffer;
static uint8_t * spOldestResponse;
static uint8_t * spNextResponse;
/** @} */
#endif

/* -------------------------------------------------------------------------
 * Private functions
 * ------------------------------------------------------------------------- */

#if MSG_ENABLE_GETRESPONSE
/** @see MSG_ID_GETRESPONSE */
static uint32_t GetResponseHandler(uint8_t msgId, int payloadLen, const uint8_t* pPayload)
{
    (void)payloadLen; /* suppress [-Wunused-parameter]: no argument is expected, but if present redundantly, just ignore. */
    (void)pPayload; /* suppress [-Wunused-parameter]: no argument is expected, but if present redundantly, just ignore. */

    if (*spOldestResponse == RESPONSE_SIZE_SKIP_TO_END) {
        spOldestResponse = spResponseBuffer;
    }
    if (*spOldestResponse == 0) {
        MSG_RESPONSE_RESULTONLY_T response;
        response.result = MSG_ERR_NO_RESPONSE;
        Msg_AddResponse(msgId, sizeof(response), (uint8_t*)&response);
    }
    else {
        uint8_t msgIdStored = *(spOldestResponse + 1);
        int payloadLenStored = *spOldestResponse - MSG_HEADER_SIZE;
        uint8_t* pPayloadStored = spOldestResponse + 1 + MSG_HEADER_SIZE;
        Msg_AddResponse(msgIdStored, payloadLenStored, pPayloadStored);
        spOldestResponse += 1 + *spOldestResponse;
    }
    return MSG_OK;
}
#endif

#if MSG_ENABLE_GETVERSION
/** @see MSG_ID_GETVERSION */
static uint32_t GetVersionHandler(uint8_t msgId, int payloadLen, const uint8_t* pPayload)
{
    MSG_RESPONSE_GETVERSION_T response;
    (void)payloadLen; /* suppress [-Wunused-parameter]: no argument is expected, but if present redundantly, just ignore. */
    (void)pPayload; /* suppress [-Wunused-parameter]: no argument is expected, but if present redundantly, just ignore. */

    response.reserved2 = 0;
    response.apiMajorVersion = MSG_API_MAJOR_VERSION;
    response.apiMinorVersion = MSG_API_MINOR_VERSION;
    response.swMajorVersion = SW_MAJOR_VERSION;
    response.swMinorVersion = SW_MINOR_VERSION;
    response.reserved7 = 0;

    Msg_AddResponse(msgId, sizeof(response), (uint8_t*)&response);
    return MSG_OK;
}
#endif

#if MSG_ENABLE_RESET
/** @see MSG_ID_RESET */
static uint32_t ResetHandler(uint8_t msgId, int payloadLen, const uint8_t* pPayload)
{
    ASSERT(msgId == MSG_ID_RESET);
    (void)payloadLen; /* suppress [-Wunused-parameter]: no argument is expected, but if present redundantly, just ignore. */
    (void)pPayload; /* suppress [-Wunused-parameter]: no response is sent back, so no bytes require being copied. */

    MSG_RESPONSE_RESULTONLY_T response;
    response.result = MSG_OK;
    Msg_AddResponse(msgId, sizeof(response), (uint8_t*)&response);
    Chip_Clock_System_BusyWait_ms(500);

    NVIC_SystemReset();
    return MSG_OK;
}
#endif

#if MSG_ENABLE_READREGISTER
/** @see MSG_ID_READREGISTER */
static uint32_t ReadRegisterHandler(uint8_t msgId, int payloadLen, const uint8_t* pPayload)
{
    const MSG_CMD_READREGISTER_T * command = (const MSG_CMD_READREGISTER_T *)pPayload;
    MSG_RESPONSE_READREGISTER_T response;

    if (payloadLen == sizeof(MSG_CMD_READREGISTER_T)) {
        response.data = *(uint32_t *)command->address;
        response.result = MSG_OK;
    }
    else {
        response.data = 0;
        response.result = MSG_ERR_INVALID_COMMAND_SIZE;
    }

    Msg_AddResponse(msgId, sizeof(response), (uint8_t*)&response);
    return MSG_OK;
}
#endif

#if MSG_ENABLE_WRITEREGISTER
/** @see MSG_ID_WRITEREGISTER */
static uint32_t WriteRegisterHandler(uint8_t msgId, int payloadLen, const uint8_t* pPayload)
{
    const MSG_CMD_WRITEREGISTER_T * command = (const MSG_CMD_WRITEREGISTER_T *)pPayload;
    MSG_RESPONSE_RESULTONLY_T response;

    if (payloadLen == sizeof(MSG_CMD_WRITEREGISTER_T)) {
        *(uint32_t *)command->address = command->data;
        response.result = MSG_OK;
    }
    else {
        response.result = MSG_ERR_INVALID_COMMAND_SIZE;
    }

    response.result = MSG_OK;
    Msg_AddResponse(msgId, sizeof(response), (uint8_t*)&response);
    return MSG_OK;
}
#endif

#if MSG_ENABLE_READMEMORY
/** @see MSG_ID_READMEMORY */
static uint32_t ReadMemoryHandler(uint8_t msgId, int payloadLen, const uint8_t* pPayload)
{
    const MSG_CMD_READMEMORY_T * command = (const MSG_CMD_READMEMORY_T *)pPayload;
    MSG_RESPONSE_READMEMORY_T response;
    response.length = 0;

    if (payloadLen == sizeof(MSG_CMD_READMEMORY_T)) {
        if (command->length <= sizeof(response.data)) {
            response.length = command->length;
            memcpy(response.data, (uint8_t *)command->address, response.length);
            response.result = MSG_OK;
        }
        else {
            response.result = MSG_ERR_INVALID_PARAMETER;
        }
    }
    else {
        response.result = MSG_ERR_INVALID_COMMAND_SIZE;
    }

    Msg_AddResponse(msgId, sizeof(response), (uint8_t*)&response);
    return MSG_OK;
}
#endif

#if MSG_ENABLE_WRITEMEMORY
/** @see MSG_ID_WRITEMEMORY */
static uint32_t WriteMemoryHandler(uint8_t msgId, int payloadLen, const uint8_t* pPayload)
{
    const MSG_CMD_WRITEMEMORY_T * command = (const MSG_CMD_WRITEMEMORY_T *)pPayload;
    MSG_RESPONSE_RESULTONLY_T response;

    if (payloadLen == sizeof(MSG_CMD_WRITEMEMORY_T)) {
        if (command->length <= sizeof(command->data)) {
            memcpy((uint8_t *)command->address, command->data, command->length);
            response.result = MSG_OK;
        }
        else {
            response.result = MSG_ERR_INVALID_PARAMETER;
        }
    }
    else {
        response.result = MSG_ERR_INVALID_COMMAND_SIZE;
    }

    Msg_AddResponse(msgId, sizeof(response), (uint8_t*)&response);
    return MSG_OK;
}
#endif

#if MSG_ENABLE_PREPAREDEBUG
/** @see MSG_ID_PREPAREDEBUG */
static uint32_t PrepareDebugHandler(uint8_t msgId, int payloadLen, const uint8_t* pPayload)
{
    MSG_RESPONSE_RESULTONLY_T response;
    response.result = MSG_OK;
    __disable_irq();

    volatile bool waitForDebuggerConnection = true;
    ASSERT(msgId == MSG_ID_PREPAREDEBUG);
    (void)payloadLen; /* suppress [-Wunused-parameter]: no argument is expected, but if present redundantly, just ignore. */
    (void)pPayload; /* suppress [-Wunused-parameter]: no argument is expected, but if present redundantly, just ignore. */

    Chip_IOCON_Init(NSS_IOCON);
    Chip_GPIO_Init(NSS_GPIO);

    Chip_GPIO_SetPinDIRInput(NSS_GPIO,0,10);
    Chip_GPIO_SetPinDIRInput(NSS_GPIO,0,11);

    Chip_IOCON_SetPinConfig(NSS_IOCON, IOCON_PIO0_10, IOCON_FUNC_2);
    Chip_IOCON_SetPinConfig(NSS_IOCON, IOCON_PIO0_11, IOCON_FUNC_2);

    Msg_AddResponse(msgId, sizeof(response), (uint8_t*)&response);
    while(waitForDebuggerConnection) {
        ;
    }
    __enable_irq();

    /* IOCON and GPIO DeInit not performed as application may be using this. */

    return MSG_OK;
}
#endif

#if MSG_ENABLE_GETUID
/** @see MSG_ID_GETUID */
static uint32_t GetUidHandler(uint8_t msgId, int payloadLen, const uint8_t* pPayload)
{
    MSG_RESPONSE_GETUID_T response;
    ASSERT(msgId == MSG_ID_GETUID);

    (void)msgId; /* suppress [-Wunused-parameter] */
    (void)payloadLen; /* suppress [-Wunused-parameter]: no argument is expected, but if present redundantly, just ignore. */
    (void)pPayload; /* suppress [-Wunused-parameter]: no argument is expected, but if present redundantly, just ignore. */

    Chip_IAP_ReadUID(response.uid);

    Msg_AddResponse(msgId, sizeof(response), (uint8_t*)&response);
    return MSG_OK;
}
#endif

/* ------------------------------------------------------------------------- */

static uint32_t DispatchCommand(uint8_t msgId, int payloadLen, const uint8_t* pPayload,
                                const MSG_CMD_HANDLER_T * handler, int handlerCount)
{
    uint32_t result = MSG_ERR_UNKNOWN_COMMAND;
    uint8_t n;
    ASSERT((handlerCount == 0) || (handler != NULL));
    for (n = 0; n < handlerCount; n++) {
        if ((handler[n].id == msgId) && (handler[n].handler != NULL)) {
            result = handler[n].handler(msgId, payloadLen, pPayload);
            break;
        }
    }
    return result;
}

/* -------------------------------------------------------------------------
 * Exported functions
 * ------------------------------------------------------------------------- */

void Msg_Init(void)
{
#if MSG_RESPONSE_BUFFER_SIZE
    extern uint8_t MSG_RESPONSE_BUFFER[MSG_RESPONSE_BUFFER_SIZE];
    spResponseBuffer = MSG_RESPONSE_BUFFER;
    spOldestResponse = MSG_RESPONSE_BUFFER;
    spNextResponse = MSG_RESPONSE_BUFFER;
    *spNextResponse = 0;
#endif
}

void Msg_SetResponseCb(pMsg_ResponseCb_t cb)
{
    sResponseCb = cb;
}

void Msg_AddResponse(uint8_t msgId, int payloadLen, const uint8_t* pPayload)
{
    ASSERT(payloadLen > 0);
    ASSERT(pPayload != NULL);

    /* Formatted message formation. */
    uint8_t formattedMsg[payloadLen + MSG_HEADER_SIZE];
    memcpy(formattedMsg + MSG_HEADER_SIZE, pPayload, (size_t)payloadLen);
    formattedMsg[0] = msgId;
    formattedMsg[1] = MSG_DIRECTION_OUTGOING;
    int responseLength = payloadLen + MSG_HEADER_SIZE; /* Update response length for header. */

    if ((sResponseCb != NULL) && sResponseCb(responseLength, formattedMsg)) {
        /* Response has been accepted. Nothing to be stored. */
    }
#if MSG_RESPONSE_BUFFER_SIZE
    else if ((responseLength > MSG_RESPONSE_BUFFER_SIZE) || (responseLength > 255)) {
        /* Response has not been accepted but it is too big to be stored. */
    #if defined(MSG_RESPONSE_DISCARDED_CB)
        /* Send out this new response _now_, then discard it unconditionally. */
        extern bool MSG_RESPONSE_DISCARDED_CB(int responseLength, const uint8_t* pResponseData);
        (void)MSG_RESPONSE_DISCARDED_CB(responseLength, formattedMsg);
    #endif
    }
    else { /* Response must be stored so it can be fetched later. */
        int skipCount;
        int rolloverCount;

        /* Check if the response can be stored in the buffer without splitting.
         * If not, we skip the remainder of the buffer, thereby increasing the required space.
         */
        skipCount = 0;
        if (spNextResponse + 1 + responseLength > spResponseBuffer + MSG_RESPONSE_BUFFER_SIZE) {
            skipCount = MSG_RESPONSE_BUFFER_SIZE + spResponseBuffer - spNextResponse;
        }

        /* Determine what to add to spOldestResponse to have comparisons as if the buffer was linear. */
        rolloverCount = 0;
        if (spOldestResponse <= spNextResponse) {
            rolloverCount = MSG_RESPONSE_BUFFER_SIZE;
        }

        /* Check if one or more oldest responses must be discarded. */
        while (spNextResponse + 1 + responseLength + skipCount >= spOldestResponse + rolloverCount) {
            if (*spOldestResponse == RESPONSE_SIZE_SKIP_TO_END) {
                /* Discard a dummy response that was added because the next response didn't fit in the remaining space
                 * in the buffer. No need to inform anyone.
                 */
                spOldestResponse = spResponseBuffer;
                rolloverCount = MSG_RESPONSE_BUFFER_SIZE;
            }
            else {
    #if defined(MSG_RESPONSE_DISCARDED_CB)
                /* Send out the oldest response _now_, then discard it unconditionally. */
                int length = *spOldestResponse;
                uint8_t* data = spOldestResponse + 1;
                extern bool MSG_RESPONSE_DISCARDED_CB(int responseLength, const uint8_t* pResponseData);
                (void)MSG_RESPONSE_DISCARDED_CB(length, data);
    #endif
                spOldestResponse += 1 + *spOldestResponse;
                if (spOldestResponse >= spResponseBuffer + MSG_RESPONSE_BUFFER_SIZE) {
                    ASSERT(spOldestResponse == spResponseBuffer + MSG_RESPONSE_BUFFER_SIZE); /* A failure indicates a buffer overflow. */
                    spOldestResponse -= MSG_RESPONSE_BUFFER_SIZE;
                    rolloverCount = MSG_RESPONSE_BUFFER_SIZE;
                }
            }
        }

        /* Store the marker to skip the remainder of the buffer, if needed. */
        if (skipCount > 0) {
            *spNextResponse = RESPONSE_SIZE_SKIP_TO_END;
            spNextResponse = spResponseBuffer;
        }

        /* Store the new response. */
        *spNextResponse = (uint8_t)responseLength; /* Guaranteed to fit in one byte. */
        memcpy(spNextResponse + 1, formattedMsg, (size_t)responseLength);
        spNextResponse += 1 + responseLength;
        if (spNextResponse >= spResponseBuffer + MSG_RESPONSE_BUFFER_SIZE) {
            ASSERT(spNextResponse == spResponseBuffer + MSG_RESPONSE_BUFFER_SIZE); /* A failure indicates a buffer overflow. */
            spNextResponse -= MSG_RESPONSE_BUFFER_SIZE;
        }
        *spNextResponse = 0;
    }
#elif defined(MSG_RESPONSE_DISCARDED_CB)
    else { /* Send out this new response _now_, then discard it unconditionally. */
        extern bool MSG_RESPONSE_DISCARDED_CB(int responseLength, const uint8_t* pResponseData);
        (void)MSG_RESPONSE_DISCARDED_CB(responseLength, formattedMsg);
    }
#endif
}

void Msg_HandleCommand(int cmdLength, const uint8_t* pCmdData)
{
    if ((cmdLength < MSG_HEADER_SIZE) || (pCmdData == NULL)) {
        ASSERT(false);
    }
    else {
        uint32_t result = MSG_ERR_UNKNOWN_COMMAND;
        uint8_t msgId = pCmdData[0];

#if defined(MSG_COMMAND_ACCEPT_CB)
        extern bool MSG_COMMAND_ACCEPT_CB(uint8_t msgId, int payloadLen, const uint8_t* pPayload);
        if (MSG_COMMAND_ACCEPT_CB(msgId, cmdLength - MSG_HEADER_SIZE, pCmdData + MSG_HEADER_SIZE)) {
#endif

        if (msgId <= MSG_ID_LASTRESERVED) {
            result = DispatchCommand(msgId, cmdLength - MSG_HEADER_SIZE, pCmdData + MSG_HEADER_SIZE,
                                     sCmdHandler, sizeof(sCmdHandler) / sizeof(sCmdHandler[0]));
        }
#if (MSG_APP_HANDLERS_COUNT)
        else {
            extern MSG_CMD_HANDLER_T MSG_APP_HANDLERS[MSG_APP_HANDLERS_COUNT];
            result = DispatchCommand(msgId, cmdLength - MSG_HEADER_SIZE, pCmdData + MSG_HEADER_SIZE,
                                     MSG_APP_HANDLERS, MSG_APP_HANDLERS_COUNT);
        }
#endif
#if defined(MSG_CATCHALL_HANDLER)
        if (result == MSG_ERR_UNKNOWN_COMMAND) {
            extern uint32_t MSG_CATCHALL_HANDLER(uint8_t msgId, int payloadLen, const uint8_t* pPayload);
            result = MSG_CATCHALL_HANDLER(msgId, cmdLength - MSG_HEADER_SIZE, pCmdData + MSG_HEADER_SIZE);
        }
#endif

#if defined(MSG_COMMAND_ACCEPT_CB)
        }
        else {
            result = MSG_ERR_INVALID_PRECONDITION;
        }
#endif
        if (result != MSG_OK) {
            MSG_RESPONSE_RESULTONLY_T response;
            int responseLength;
            response.result = result;
            responseLength = sizeof(MSG_RESPONSE_RESULTONLY_T);
            Msg_AddResponse(msgId, responseLength, (uint8_t*)&response);
        }
    }
}
