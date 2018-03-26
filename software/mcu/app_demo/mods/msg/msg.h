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


#ifndef __MSG_H_
#define __MSG_H_

/**
 * @defgroup MODS_NSS_MSG msg: Message Handler
 * @ingroup MODS_NSS
 * The message handler is a module which provides a command - response message mechanism to help an upper layer to
 * communicate with an external device. It is purely a software implementation without any direct dependency on hardware
 * or transport layer. Using the message handler module, the upper layer can easily add or remove commands and
 * responses, both at compile time and at runtime.
 * At all times, the upper layer retains full control on what is received, what is parsed, what is generated and what is
 * sent.
 *
 * @par Definitions
 *  A few definitions make the explanations below more clear:
 *  <dl><dt>Command</dt><dd>A sequence of bytes following a @ref msg_anchor_protocol "Protocol" that is given to the message
 *      handler module that forms an instruction to perform an action.</dd>
 *  <dt>Response</dt><dd>A sequence of bytes following a @ref msg_anchor_protocol "Protocol" that is generated to partially
 *     or fully complete the action(s) requested by a previously received command and that is to be sent out via any transport
 *     layer.</dd>
 *  <dt>Message</dt><dd>A command or a response.</dd>
 *  <dt>Command generator</dt><dd>A part of the upper layer that receives messages via SPI, NFC, or generates them
 *     itself.</dd>
 *  <dt>Command handler</dt><dd>A function, either part of the message handler module or the upper layer, that
 *     interprets the content enclosed in a specific command, takes the appropriate actions and generates the corresponding
 *     responses.</dd>
 *  <dt>Response handler</dt><dd>A part of the upper layer that receives responses via a designated callback, and that
 *     is responsible for sending out the generated responses to the originator of the corresponding command.</dd>
 *  <dt>Immediate response</dt><dd>The message handler module will ensure that for every command it receives, at least
 *     one response is generated and returned to the upper layer @ref msg_anchor_exceptions_no_response "(*)". This
 *     first guaranteed response will become available in a synchronous way, i.e. before the call has ended, and will
 *     be made available to the upper layer in the same CPU context as where the command has been given. This means
 *     that in case a specific command handler does not create a response, the message handler module will create one
 *     on that specific command handler's behalf. This first guaranteed response is called the immediate response.
 *     Additional responses may or may not be generated immediately or later - this is command and application
 *     specific.</dd>
 *  <dt>Response generator</dt><dd>Either the message module itself, or an application defined message handler function.
 *     The specific message handler may at any time reject generating any response; in that case, the message module
 *     itself will ensure at least one response is generated per received command.</dd>
 *  @anchor msg_anchor_direction
 *  <dt>Directionality</dt><dd> Each message requires the presence of a directionality field as part of the
 *     @ref msg_anchor_protocol "Protocol".</dd></dl>
 *
 * @par Overview
 *  The message handler module does not run in its own thread and will not react to interrupts: it has to be explicitly
 *  triggered to do something.
 *
 *  When the upper layer delivers a sequence of bytes to the message handler module, i.e. the upper layer acts as a
 *  <em>Command generator</em>, the message handler module tries to interpret this as a command. Each sequence of bytes
 *  is supposed to constitute one command.
 *
 *  The message handler module maintains a list of known commands and tries to match the incoming command with this
 *  list. If a match is found, i.e. there is a <em>Command handler</em> for this specific command, the corresponding
 *  handler is called. At that point either the handler generates at least one response, i.e. it is also the response
 *  generator, after which the message handler module considers the command handling complete; or no response is
 *  generated, after which the message handler module will generate an immediate response itself, i.e. the message
 *  handler module will take over the role of <em>Response generator</em>.
 *
 * @anchor msg_anchor_protocol
 * @par Protocol
 *  The message handler module uses the following communication protocol for commands and responses. The first byte
 *  shall indicate the message id. The second byte shall indicate the direction of transfer as mentioned below:
 *      - value of 0x0 for commands coming into message mod
 *      - value of 0x1 for responses going out from message mod
 *      .
 *  This is followed by the optional payload specific for each command/response. Details of the commands and
 *  responses are captured in the following sections.
 *  @note The direction byte is used to differentiate a command/response pair that have the exact same id and
 *   payload. However, it is not enforced by the module that the host sets the correct value for this.
 *
 * @par Commands
 *  A command can be any sequence of bytes, that adheres to the @ref msg_anchor_protocol "Protocol".
 *
 * @par Handlers
 *  The message handler module has a default list of commands and corresponding command handlers - which can be enabled
 *  and disabled at compile time - see @ref msg_anchor_handlers "MSG_APP_HANDLERS". The upper layer can at compile time
 *  add handlers for specific commands, or it can make use of a catch-all command handler to dynamically change the way
 *  it handles commands at runtime - see @ref msg_anchor_catchall_handler "MSG_CATCHALL_HANDLER".
 *
 *  Apart from some minimal rules, no predefined formats exist, leaving the application with full freedom how to
 *  implement and handle its own specific commands.
 *
 *  A handler can be any function, that adheres to these restrictions:
 *      - The prototype must match #pMsg_CmdHandler_t
 *      .
 *
 * @par Responses
 *  Responses are typically generated in a command handler. Generated responses are given to the message handler module,
 *  which then attempts to have them sent by calling a designated callback. That callback can either immediately send
 *  the response over the transport layer of its choosing back to the originator of the command; or reject the response.
 *  When rejected, the message handler module will store the response in its internal buffer; a special command is
 *  available to retrieve them at a later time.
 *
 * @note The device(s), layer(s) or block(s) that creates commands and handles responses can be considered to be the
 *  counter-part of the message handler module. The limitations imposed by the message handler module and the precise
 *  format of each command and response must be known by respected by that counter-part. Changes made to this on one
 *  side must be made as well on the other side.
 *
 * @anchor msg_anchor_exceptions_no_response
 * @note All commands generate one immediate response, safe for the #MSG_ID_RESET command.
 *  All commands may subsequently generate zero, one or multiple additional responses at a later time.
 *
 * @note Some commands are asynchronous in nature: in that case a response still must be immediately returned, denoting
 *  only the acceptance of the command, and subsequent responses provide the requested result.
 *
 * @par Sequence diagrams
 *  The following example sequence diagrams provide a quick insight in the different interactions.
 *  <dl><dt>Basic example</dt><dd>
 *      A very basic example, where each call returns success:
 *
 *      @msc
 *      hscale="1.6", wordwraparcs="true";
 *      C [label="response handler"],
 *           A [label="command generator"],
 *                D [label="message handler module"],
 *                     B [label="command handler/\nresponse generator"];
 *      |||;
 *           A => D         [label="[A, B, C, ... Z]\nsequence of bytes", URL="#Msg_HandleCommand"];
 *                D => B    [label="[msgId, dir, C, ... Z]\ncommand", URL="#pMsg_CmdHandler_t"];
 *                     B->B [label="Generate response"];
 *                D <= B    [label="[msgId, dir, c, ... z]\nresponse", URL="#Msg_AddResponse"];
 *      C <=      D         [label="[a, b, c, ... z]\nsequence of bytes", URL="#pMsg_ResponseCb_t"];
 *      @endmsc
 *
 *  </dd><dt>Holding back a response</dt><dd>
 *      In case the response handler is at first 'incapable of handling the response', i.e. of sending out the
 *      response immediately, the response can be retrieved at a later more convenient time:
 *
 *      @msc
 *      hscale="1.6", wordwraparcs="true";
 *      C [label="response handler"],
 *           A [label="command generator"],
 *                D [label="message handler module"],
 *                     B [label="command handler/\nresponse generator"];
 *      |||;
 *           A => D         [label="[A, B, C, ... Z]\nsequence of bytes", URL="@ref Msg_HandleCommand"];
 *                D => B    [label="[msgId, dir, C, ... Z]\ncommand", URL="@ref pMsg_CmdHandler_t"];
 *                     B->B [label="Generate response"];
 *                D <= B    [label="[msgId, dir, c, ... z]\nresponse", URL="@ref Msg_AddResponse"];
 *      C <=      D         [label="[a, b, c, ... z]\nsequence of bytes", URL="@ref pMsg_ResponseCb_t"];
 *      C >>      D         [label="returns false"];
 *                D->D      [label="response stored"];
 *      ...;
 *      ...;
 *      ... [label="...as time passes by..."];
 *      ...;
 *      ...;
 *
 *           A => D         [label="[MSG_ID_GETRESPONSE]\ncommand", URL="@ref Msg_HandleCommand"];
 *                D->D      [label="pop response"];
 *      C <=      D         [label="[a, b, c, ... z]\nsequence of bytes", URL="@ref Msg_AddResponse"];
 *      @endmsc
 *
 *  </dd><dt>Multiple responses per command</dt><dd>
 *      Some commands take time to complete, or cause the generation of multiple responses:
 *
 *      @msc
 *      hscale="1.6", wordwraparcs="true";
 *      C [label="response handler"],
 *           A [label="command generator"],
 *                D [label="message handler module"],
 *                     B [label="command handler/\nresponse generator"];
 *      |||;
 *           A => D         [label="[A, B, C, ... Z]\nsequence of bytes", URL="@ref Msg_HandleCommand"];
 *                D => B    [label="[msgId, dir, C, ... Z]\ncommand", URL="@ref pMsg_CmdHandler_t"];
 *                     B->B [label="Generate response"];
 *                D <= B    [label="[msgId, dir, c, ... z]\nimmediate response", URL="@ref Msg_AddResponse"];
 *      C <=      D         [label="[a, b, c, ... z]\nsequence of bytes", URL="@ref pMsg_ResponseCb_t"];
 *
 *      ...;
 *      ...;
 *      ... [label="...as time passes by..."];
 *      ...;
 *      ...;
 *
 *                     B->B [label="Generate response"];
 *                D <= B    [label="[msgId, dir, c2, ... z2]\nresponse", URL="@ref Msg_AddResponse"];
 *      C <=      D         [label="[a2, b2, c2, ... z2]\nsequence of bytes", URL="@ref pMsg_ResponseCb_t"];
 *
 *      ...;
 *      ...;
 *      ... [label="...as time passes by..."];
 *      ...;
 *      ...;
 *
 *                     B->B [label="Generate response"];
 *                D <= B    [label="[msgId, dir, c3, ... z3]\nresponse", URL="@ref Msg_AddResponse"];
 *      C <=      D         [label="[a3, b3, c3, ... z3]\nsequence of bytes", URL="@ref pMsg_ResponseCb_t"];
 *      @endmsc
 *
 *  </dd><dt>Including the transport layer</dt><dd>
 *      In practice, the role of command generator and response handler may both be taken up by 'the application'.
 *      The sequence diagram below now also an SPI block, to make clear it does not come into direct contact with the
 *      message handler module.
 *
 *      @msc
 *      hscale="1.6", wordwraparcs="true";
 *      E [label="SPI driver"],
 *           AC [label="application"],
 *                 D [label="message handler module"],
 *                      B [label="command handler"];
 *      |||;
 *      E => AC              [label="[A, B, C, ... Z]\nsequence of bytes"];
 *           AC => D         [label="[A, B, C, ... Z]\nsequence of bytes", URL="@ref Msg_HandleCommand"];
 *                 D => B    [label="[msgId, dir, C, ... Z]\ncommand", URL="@ref pMsg_CmdHandler_t"];
 *                      B->B [label="Generate response"];
 *                 D <= B    [label="[msgId, dir, c, ... z]\nresponse", URL="@ref Msg_AddResponse"];
 *           AC <= D         [label="[a, b, c, ... z]\nsequence of bytes", URL="@ref pMsg_ResponseCb_t"];
 *      E <= AC              [label="[A, B, C, ... Z]\nsequence of bytes"];
 *      @endmsc
 *
 *      @note The message handler module only expects full commands: it is either up to the upper layer or up
 *          to the specific command handler functions to guard against missing or superfluous bytes.
 *
 *      As can be seen, it is the application - @b your code - that sits between the communication channel and the
 *      message handling. This can be as thin as a simple pass-through: then the only benefit is a functionality
 *      isolation at the cost of a small overhead. @n
 *      But the application can do more: it can encapsulate the responses, i.e. adding a CRC over
 *      an unreliable communication channel, or creating an NDEF message to send the response as a MIME type NDEFt2t
 *      record; and likewise it may need to validate the incoming commands by checking the CRC, or extracting every
 *      record from an NDEF multi-record message. This also provides a good debugging point, as the entire
 *      command/response flow can be easily logged or printed out over a debug line.
 *
 *  </dd></dl>
 *
 * @par Internal messages / custom messages
 *  A number of commands and corresponding responses have been predefined by the message handler module. They
 *  can be found in the enumeration #MSG_ID_T. By default only one of them is compiled in to save code space:
 *  #MSG_ID_GETVERSION.
 *  Each command has a corresponding command structure @c MSG_CMD_xxxx_T that describes the fields which are
 *  packed into the command payload, and one or possibly two response structures @c MSG_RESPONSE_xxxx_T that describe
 *  which fields to expect in return, as part of the response payload.
 *  By default, there exist no custom messages. You can add them by properly defining both #MSG_APP_HANDLERS_COUNT and
 *  @ref msg_anchor_handlers "MSG_APP_HANDLERS", and/or by defining
 *  @ref msg_anchor_response_discarded_cb "MSG_RESPONSE_DISCARDED_CB". There is no obligation to create structures for
 *  each custom command and corresponding responses: all is well as long as the custom handlers generate the expected
 *  responses.
 *
 * @par Diversity
 *  This module supports a number of diversity flags. Check the
 *  @ref MSG_DFT_MODS_NSS "Message handler module diversity settings" for a full list and their implications.
 *
 * @par How to use the module
 *  Most likely there will be at least a few application specific commands and responses. A first step would then be to
 *  create a list of message id's and functions that act as the command handlers / response generators for these message
 *  id's and to store them in an array of type #MSG_CMD_HANDLER_T.
 *
 *  Each such function is to use #Msg_AddResponse reply to a received command. It is recommended also create a (packed)
 *  command structure and response structure for each message id, as this aids in explicitly describing the message
 *  specific parameters.
 *
 *  Also check the other diversity flags and enable the required functionality. Possibly you'll have to implement a few
 *  callback functions if you want to make use of all the functionality. Couple your 'extensions' using the diversity
 *  flags.
 *
 *  After initializing the message handler module with #Msg_Init and #Msg_SetResponseCb, you just feed it commands via
 *  #Msg_HandleCommand. The whole chain provided by the message handler module and your implemented and coupled
 *  extensions will deliver the responses to the response callback you set.
 *
 * @par Example
 *  Below a minimalistic code-like example is given. This does not cover the full functionality the message handler
 *  module offers, but aids in clarifying how all the pieces are tied together. In the example, one application specific
 *  message is added, then via the transport layer a command with that new id is received. The newly created command
 *  handler generates a response, which is then sent out again.
 *
 * @par Handle function
 *  @snippet msg_mod_example_1.c msg_mod_handle
 *
 * @par Command Handler
 *  @snippet msg_mod_example_1.c msg_mod_cmd_handler
 *  @note Id 0x77 is considered for this example.
 *
 * @par Response callback 'ResponseCb' function
 *  @snippet msg_mod_example_1.c msg_mod_responseCb
 *
 * @par Msg Mod Init
 *  @snippet msg_mod_example_1.c msg_mod_init
 *
 * @par Data receive function
 *  @snippet msg_mod_example_1.c msg_mod_rxdata
 *
 * @{
 */

/* -------------------------------------------------------------------------
 * Include files
 * ------------------------------------------------------------------------- */

#include <stdint.h>
#include <stdbool.h>
#include "msg_dft.h"
#include "msg_cmd.h"
#include "msg_response.h"

/* -------------------------------------------------------------------------
 * Types and defines
 * ------------------------------------------------------------------------- */

/**
 * Supported messages.
 * @note Each message comprises of a command and a response. For each command and response a corresponding struct
 *  exists which explains the parameters, their sizes and their use.
 */
typedef enum MSG_ID {

    /**
     * @c 0x01 @n
     * The message handler module holds a queue of responses. Each time a response is created which cannot be sent back
     * immediately (in response to a command), it is queued. It's the task of the host to use this command to retrieve
     * the queued responses.
     * @note When the host doesn't retrieve the responses fast enough, the buffer gets filled up. When a new response
     *  can not be stored any more, the oldest responses are discarded until sufficient room is available to store the
     *  newest response. The upper layer can get notified when this happens via the diversity setting
     *  @ref msg_anchor_response_discarded_cb "MSG_RESPONSE_DISCARDED_CB".
     * @param Header : Sequence of bytes as per the @ref msg_anchor_protocol "Protocol".
     * @param Payload : @b No payload.
     * @return The response is returned immediately.
     *  The type of the response is determined by the first byte and is equal to the message id which triggered the
     *  generation of this response.
     * @note synchronous command
     * @note For this command to become available, define both @ref msg_anchor_response_buffer "MSG_RESPONSE_BUFFER" and
     *  #MSG_RESPONSE_BUFFER_SIZE
     */
    MSG_ID_GETRESPONSE = 0x01,

    /**
     * @c 0x02 @n
     * This message id allows the host to determine the version of the firmware.
     * @param Header : Sequence of bytes as per the @ref msg_anchor_protocol "Protocol".
     * @param Payload : @b No payload.
     * @return MSG_RESPONSE_GETVERSION_T
     * @note synchronous command
     * @note This command is always available.
     */
    MSG_ID_GETVERSION = 0x02,

    /**
     * @c 0x03 @n
     * Generate a reset of the digital part of the system.
     * @param Header : Sequence of bytes as per the @ref msg_anchor_protocol "Protocol".
     * @param Payload : @b No payload.
     * @return MSG_RESPONSE_RESULTONLY_T.
     * @note synchronous command
     * @note For this command to become available, define #MSG_ENABLE_RESET
     */
    MSG_ID_RESET = 0x03,

    /**
     * @c 0x04 @n
     * Read and return the value from the selected ARM register address.
     * @param Header : Sequence of bytes as per the @ref msg_anchor_protocol "Protocol".
     * @param Payload : #MSG_CMD_READREGISTER_T
     * @return MSG_RESPONSE_READREGISTER_T
     * @note synchronous command
     * @note For this command to become available, define #MSG_ENABLE_READREGISTER
     */
    MSG_ID_READREGISTER = 0x04,

    /**
     * @c 0x05 @n
     * Write the data supplied by the host to the selected ARM register address.
     * @param Header : Sequence of bytes as per the @ref msg_anchor_protocol "Protocol".
     * @param Payload : #MSG_CMD_WRITEREGISTER_T
     * @return MSG_RESPONSE_RESULTONLY_T.
     * @note synchronous command
     * @note For this command to become available, define #MSG_ENABLE_WRITEREGISTER
     */
    MSG_ID_WRITEREGISTER = 0x05,

    /**
     * @c 0x06 @n
     * Read and return the value from the selected address.
     * @param Header : Sequence of bytes as per the @ref msg_anchor_protocol "Protocol".
     * @param Payload : #MSG_CMD_READMEMORY_T
     * @return MSG_RESPONSE_READMEMORY_T.
     * @note synchronous command
     * @note For this command to become available, define #MSG_ENABLE_READMEMORY
     * @note The address range may be part of SRAM, Flash or EEPROM.
     * @warning It is assumed the address region is accessible and can be read. This is @b not checked for.
     *  e.g. This implies it is the application's responsibility to ensure the EEPROM is initialized
     *  when reading EEPROM data, and that it is the caller's responsibility to ensure the range can be mapped to valid
     *  addresses.
     */
    MSG_ID_READMEMORY = 0x06,

    /**
     * @c 0x07 @n
     * Write the data supplied by the host to the selected ARM memory address.
     * @param Header : Sequence of bytes as per the @ref msg_anchor_protocol "Protocol".
     * @param Payload : #MSG_CMD_WRITEMEMORY_T
     * @return MSG_RESPONSE_RESULTONLY_T.
     * @note synchronous command
     * @note For this command to become available, define #MSG_ENABLE_WRITEMEMORY
     * @warning Only SRAM can be written to. It is assumed the address region which is referred to is fully part of
     *  SRAM. This is @b not checked for.
     */
    MSG_ID_WRITEMEMORY = 0x07,

    /**
     * @c 0x08 @n
     * Configures and enables the SWD lines, then waits in an endless while loop. This allows a developer to attach
     * a debugger to the running device.
     * @param Header : Sequence of bytes as per the @ref msg_anchor_protocol "Protocol".
     * @param Payload : @b No payload.
     * @return MSG_RESPONSE_RESULTONLY_T.
     * @note synchronous command
     * @note When a debugging session has been established, set the boolean @c waitForDebuggerConnection to @c false
     *  to return from the command handler.
     * @note For this command to become available, define #MSG_ENABLE_PREPAREDEBUG
     */
    MSG_ID_PREPAREDEBUG = 0x08,

    /**
     * @c 0x09 @n
     * Retrieve the unique identifier of the IC. This is the device serial number guaranteed unique among all NHS31xx
     * ICs.
     * @note This is @b not equal to the NFC ID, which is a randomly assigned number.
     * @param Header : Sequence of bytes as per the @ref msg_anchor_protocol "Protocol".
     * @param Payload : @b No payload.
     * @return MSG_RESPONSE_GETUID_T
     * @note synchronous command
     * @note For this command to become available, define #MSG_ENABLE_GETUID.
     */
    MSG_ID_GETUID = 0x09,

    /**
     * @c 0x3F @n
     * This message id does not encompass a command or a response. It is used to signify the highest id that is reserved
     * for use by the message handler itself. All application specific commands - which are to be handled by setting a
     * specific handler - must use id's greater than this value.
     * @note Only to be used as an offset for your own application specific commands and responses.
     * @see Cmd_SetApplicationHandler
     */
    MSG_ID_LASTRESERVED = 0x3F

} MSG_ID_T;

/* -------------------------------------------------------------------------
 * Exported function prototypes
 * ------------------------------------------------------------------------- */

/** @cond !MSG_PROTOCOL_DOC */

/**
 * Initializes the module.
 * @pre This must be the first function called of this module.
 * @note May be called multiple times. Whenever called, the response buffer will be cleared.
 */
void Msg_Init(void);

/**
 * Register a callback function that will be called whenever an immediate response is available.
 * @param cb : The function to call that will send the response to the host. May not be @c NULL.
 *  @pre This must be set before calling #Msg_HandleCommand.
 * @note The given function will be called under the same context as the previous call to #Msg_AddResponse, or the
 *  previous call to #Msg_HandleCommand with id #MSG_ID_GETRESPONSE
 */
void Msg_SetResponseCb(pMsg_ResponseCb_t cb);

/**
 * Try to send the response back to the upper layer. If this fails and a buffer has been made available via the
 * diversities @ref msg_anchor_response_buffer "MSG_RESPONSE_BUFFER" and #MSG_RESPONSE_BUFFER_SIZE, store the response
 * so it can be given to the upper layer at a later time.
 * @note If a function has been assigned to @ref msg_anchor_response_discarded_cb "MSG_RESPONSE_DISCARDED_CB", and no
 *  response buffer is available, this function will have been called before this function exits in case the upper layer
 *  refuses the response. That callback will also have been called when pushing this response into the response buffer
 *  causes the oldest response(s) to be popped out.
 * @param msgId : Holds the id of the message
 * @param payloadLen : Size in bytes of the response
 * @param pPayload : May not be @c NULL. Points to @c payloadLen number of bytes, which forms the complete response.
 */
void Msg_AddResponse(uint8_t msgId, int payloadLen, const uint8_t* pPayload);

/**
 * To be called each time a command has been received via any communication channel.
 * @param cmdLength : The size in bytes in @c pCmdData
 *  @pre @c cmdLength >= 2
 * @param pCmdData : Pointer to the array containing the raw command bytes as specified
 *  by @ref msg_anchor_protocol "Protocol".
 *  @pre data retention of the bytes contained in @c pCmdData must be guaranteed until the function returns.
 * @post The callback function as registered in the call to #Msg_SetResponseCb will have been called.
 * @post Multiple consecutive calls to that same callback function may be issued after this call has ended.
 */
void Msg_HandleCommand(int cmdLength, const uint8_t* pCmdData);

/** @endcond */

#endif /** @} */
