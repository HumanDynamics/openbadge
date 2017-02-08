//
// Created by Andrew Bartow on 2/5/17.
//

#ifndef OPENBADGE_REQUEST_HANDLER_H
#define OPENBADGE_REQUEST_HANDLER_H

#include "comm_protocol.h"

/**
 * Handles the given Communication Protocol request according to the communication protocol. Queues any necessary
 *   response(s). The queued response(s) can be fetched using RequestHandler_GetNextQueuedResponse.
 * The queue must be completely empty of responses before a new request can be handled. Will assert if this is not the
 *   case.
 * @param request The request to handle.
 */
void RequestHandler_HandleRequest(Request_t request);

/**
 * Fetches the next response that is currently queued, waiting to be sent back to client. Writes it into p_response.
 *
 * @param p_response Destination struct for the next queued response.
 * @return NRF_SUCCESS if the next queued response was successfully fetched.
 *         NRF_ERROR_NOT_FOUND if no response was queued.
 */
uint32_t RequestHandler_GetNextQueuedResponse(Response_t * p_response);

#endif //OPENBADGE_REQUEST_HANDLER_H
