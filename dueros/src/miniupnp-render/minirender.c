/*
 * miniupnprender @ Baidu
 *
 * Portions of the code from the MiniUPnP project:
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 *
 * Copyright (c) 2006-2008, Thomas Bernard
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

#include "minirender.h"
#include "sockets.h"

#include "log.h"
#include "minissdp.h"
#include "list.h"
#include "upnphttp.h"
#include "upnpevents.h"
#include "utilities.h"
#include "upnpglobalvars.h"
#include "heap_monitor.h"

unsigned short port = 56567;
int notify_interval = 15;
unsigned char quitting = 0;

/* OpenAndConfHTTPSocket() :
 * setup the socket used to handle incoming HTTP connections. */
static int open_http_socket(unsigned short* port) {
    int s = -1;
    int i = 1;
    struct sockaddr_in listenname;
    s = socket(AF_INET, SOCK_STREAM, 0);

    if (s < 0) {
        DUER_LOGE("socket(http) failed.\n");
        return -1;
    }

    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i)) < 0) {
        DUER_LOGE("setsockopt(http, SO_REUSEADDR) failed.\n");
    }

    memset(&listenname, 0, sizeof(listenname));
    listenname.sin_family = AF_INET;
    listenname.sin_port = htons(*port);
    listenname.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(s, (struct sockaddr*)&listenname, sizeof(listenname)) < 0) {
        DUER_LOGE("bind(http) failed.\n");
        close(s);
        return -1;
    }

    if (listen(s, 5) < 0) {
        DUER_LOGE("listen(http) failed.\n");
        close(s);
        return -1;
    }

    return s;
}

void set_uuid(const char* uuid) {
    snprintf(uuidvalue, UUID_LEN, "uuid:%s", uuid);
}

void set_friendly_name(const char* name) {
    snprintf(friendly_name, FRIENDLYNAME_MAX_LEN, "%s", name);
}

void minirender_quit() {
    quitting = 1;
}

static void minirender_init(const char* mac) {
    unsigned char hex_mac[6] = {0};
    sscanf(mac, "%02x:%02x:%02x:%02x:%02x:%02x", &hex_mac[0], &hex_mac[1], &hex_mac[2], &hex_mac[3],
            &hex_mac[4], &hex_mac[5]);
    
    if(uuidvalue[0] == 0) {
        snprintf(uuidvalue, UUID_LEN, "uuid:00000000-0000-0000-0000-%02x%02x%02x%02x%02x%02x", hex_mac[0], hex_mac[1],
                hex_mac[2], hex_mac[3], hex_mac[4], hex_mac[5]);
    }

    if(friendly_name[0] == 0) {
        snprintf(friendly_name, FRIENDLYNAME_MAX_LEN, "duer_light_os-%02x%02x", hex_mac[4], hex_mac[5]);
    }

    DUER_LOGD("uuid=%s, name=%s", uuidvalue, friendly_name);
}

//int main(int argc, char* argv[])
int upnp_test(const char* host_ip, const char* host_mac) {
    /*
     get host_ip();
     get host_mac();
     set_uuid();
     set_ssdp_interval();
    */

    int i = 0;
    int ret = 0;
    int ssdp_notify_fd = -1;
    int ssdp_recv_fd = -1;	
    int httpl_fd = -1;
    int max_fd = -1;
    fd_set readset;	/* for select() */
    fd_set writeset;
    struct timeval timeout, timeofday, lastnotifytime = {0, 0};
	
    LIST_HEAD(httplisthead, upnphttp) upnphttphead;
    struct upnphttp* e = 0;
    struct upnphttp* next;
    LIST_INIT(&upnphttphead);

    i = strlen(host_ip);
    memcpy(hostip, host_ip, i);
    hostip[i] = '\0';
    snprintf(url_base, 40, "http://%s:%d/", hostip, port);

    minirender_init(host_mac);
	
    ssdp_notify_fd = open_ssdp_notify_socket();
    if (ssdp_notify_fd == -1) {
        DUER_LOGE("open_ssdp_notify_socket failed.\n");
        return -1;
    }

    ssdp_recv_fd = open_and_conf_ssdp_receive_socket();
    if (ssdp_recv_fd < 0) {
        DUER_LOGE("create receive socket is failed");
        return -1;
    }
    
    httpl_fd = open_http_socket(&port);
    if (httpl_fd < 0) {
        DUER_LOGE("open_http_socket failed.\n");
        return -1;
    }

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    while (!quitting) {
        /* Check if we need to send SSDP NOTIFY messages and do it if
         * needed */
        if (gettimeofday(&timeofday, 0) < 0) {
            DUER_LOGW("gettimeofday failed.\n");
            //timeout.tv_sec = notify_interval;
            //timeout.tv_usec = 0;
        } else {
            /* the comparison is not very precise but who cares ? */
            if (timeofday.tv_sec >= (lastnotifytime.tv_sec + notify_interval)) {
                DUER_LOGD("Sending SSDP notifies, hostip=%s.\n", hostip);
                send_ssdp_notifies(ssdp_notify_fd, hostip, port, 60);
                memcpy(&lastnotifytime, &timeofday, sizeof(struct timeval));
                //timeout.tv_sec = notify_interval;
                //timeout.tv_usec = 0;
            } else {
                /*timeout.tv_sec = lastnotifytime.tv_sec + notify_interval
                                 - timeofday.tv_sec;

                if (timeofday.tv_usec > lastnotifytime.tv_usec) {
                    timeout.tv_usec = 1000000 + lastnotifytime.tv_usec
                                      - timeofday.tv_usec;
                    timeout.tv_sec--;
                } else
                    timeout.tv_usec = lastnotifytime.tv_usec - timeofday.tv_usec;*/
            }
        }

        /* select open sockets (SSDP, HTTP listen, and all HTTP soap sockets) */
        FD_ZERO(&readset);

        if (ssdp_recv_fd >= 0) {
            FD_SET(ssdp_recv_fd, &readset);
			max_fd = MAX(max_fd, ssdp_recv_fd);
        }
        
        if (httpl_fd >= 0) {
            FD_SET(httpl_fd, &readset);
            max_fd = MAX(max_fd, httpl_fd);
        }		

        i = 0;	/* active HTTP connections count */

        for (e = upnphttphead.lh_first; e != NULL; e = e->entries.le_next) {
            if ((e->socket >= 0) && (e->state <= 2)) {
                FD_SET(e->socket, &readset);
                max_fd = MAX(max_fd, e->socket);
                i++;
            }
        }

        FD_ZERO(&writeset);		
		upnpevents_select_fds(&readset, &writeset, &max_fd);

        ret = select(max_fd + 1, &readset, &writeset, NULL, &timeout);
        DUER_LOGV("select ret=%d.\n", ret);
        if (ret < 0) {
            DUER_LOGE("select failed!! ret=%d.\n", ret);
            continue;
        }

		upnpevents_process_fds(&readset, &writeset);

        /* process ssdp request */
        if (ssdp_recv_fd >= 0 && FD_ISSET(ssdp_recv_fd, &readset)) {
            process_ssdp_request(ssdp_recv_fd, hostip, port);
        }

        /* process active HTTP connections */
        for (e = upnphttphead.lh_first; e != NULL; e = e->entries.le_next) {
            DUER_LOGD("upnphttp, state=%d", e->state);
            if ((e->socket >= 0) && (e->state <= 2) && (FD_ISSET(e->socket, &readset)))
                process_upnphttp(e);
        }

        /* process incoming HTTP connections */
        if (httpl_fd >= 0 && FD_ISSET(httpl_fd, &readset)) {
            int shttp;
            socklen_t clientnamelen;
            struct sockaddr_in clientname;
            clientnamelen = sizeof(struct sockaddr_in);
            shttp = accept(httpl_fd, (struct sockaddr*)&clientname, &clientnamelen);

            if (shttp < 0) {
                DUER_LOGE("accept(http) failed: shttp=%d\n", shttp);
            } else {
                struct upnphttp* tmp = 0;
                DUER_LOGD("HTTP connection from %s:%d\n",
                         inet_ntoa(clientname.sin_addr),
                         ntohs(clientname.sin_port));
                /*if (fcntl(shttp, F_SETFL, O_NONBLOCK) < 0) {
                	DPRINTF(E_ERROR, L_GENERAL, "fcntl F_SETFL, O_NONBLOCK\n");
                }*/
                /* Create a new upnphttp object and add it to
                 * the active upnphttp object list */
                tmp = new_upnphttp(shttp);

                if (tmp) {
                    tmp->clientaddr = clientname.sin_addr;
                    LIST_INSERT_HEAD(&upnphttphead, tmp, entries);
                } else {
                    DUER_LOGW("New_upnphttp() failed\n");
                    close(shttp);
                }
            }
        }

        /* delete finished HTTP connections */
        for (e = upnphttphead.lh_first; e != NULL; e = next) {
            next = e->entries.le_next;

            if (e->state >= 100) {
                LIST_REMOVE(e, entries);
                delete_upnphttp(e);
            }
        }

#ifdef HEAP_MONITOR
        show_heap_info();
#endif
    }

    DUER_LOGI("miniupnprender quit!");

    /* close out open sockets */
    while (upnphttphead.lh_first != NULL) {
        e = upnphttphead.lh_first;
        LIST_REMOVE(e, entries);
        delete_upnphttp(e);
    }

    if (httpl_fd >= 0) {
        close(httpl_fd);
    }

    if (ssdp_recv_fd >=0 ) {
        close(ssdp_recv_fd);
    }

    if (ssdp_notify_fd >= 0) {
        close(ssdp_notify_fd);
    }

    upnpevents_remove_subscribers();

    reclaim_res();

    return ret;
}
