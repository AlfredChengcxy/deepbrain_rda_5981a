/*
 * miniupnprender @ Baidu
 *
 * Portions of the code from the MiniUPnP project:
 *
 * Copyright (c) 2006-2007, Thomas Bernard
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * The name of the author may not be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef BAIDU_IOT_TINYDU_IOT_OS_SRC_MINIUPNPRENDER_UPNPSOAP_H
#define BAIDU_IOT_TINYDU_IOT_OS_SRC_MINIUPNPRENDER_UPNPSOAP_H

#include "upnphttp.h"

#ifdef __cplusplus
extern "C" {
#endif

enum TransportState {
    STOPPED,
    PLAYING,
    PAUSED_PLAYBACK
};

enum TransportStatus {
    OK,
    ERR
};

/**
 * @brief get internal tranport state.
 * @ret transport state.
 */
int get_transport_state();

/**
 * @brief set internal tranport state.
 * @param in st: transport state.
 */
void set_transport_state(enum TransportState st);

/**
 * @brief this method executes the requested Soap Action
 * @param in upnphttp: http header.
 * @param in char*: soap method name.
 * @param in in: soap method length.
 */
void execute_soap_action(struct upnphttp*, const char*, int);

/**
 * @brief event notification for connmanager service
 * @param out len: notification message length
 * @ret notification message
 */
char* connmanager_event_notify(int* len);

/**
 * @brief event notification for rendercontrol service
 * @param out len: notification message length
 * @ret notification message
 */
char* rendercontrol_event_notify(int* len);

/**
 * @brief event notification for avtransport service
 * @param out len: notification message length
 * @ret notification message
 */
char* avtransport_event_notify(int* len);

/**
 * @brief reclaim resources after minirenderer quit
 */
void reclaim_res();

#ifdef __cplusplus
}
#endif

#endif // BAIDU_IOT_TINYDU_IOT_OS_SRC_MINIUPNPRENDER_UPNPSOAP_H

