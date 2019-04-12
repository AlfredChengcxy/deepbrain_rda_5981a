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

#include "minissdp.h"
#include "sockets.h"
#include "httppath.h"
#include "upnpglobalvars.h"
#include "log.h"
#include "utilities.h"

// SSDP IP and Port
#define SSDP_PORT (1900)
#define SSDP_MCAST_ADDR ("239.255.255.250")

static const char* const known_service_types[] = {
    uuidvalue,
    "upnp:rootdevice",
    "urn:schemas-upnp-org:device:MediaRenderer:1",
    "urn:schemas-upnp-org:service:AVTransport:1",
    "urn:schemas-upnp-org:service:ConnectionManager:1",
    "urn:schemas-upnp-org:service:RenderingControl:1",
    0
};

int open_ssdp_notify_socket() {
    int fd = -1;
    unsigned char loopchar = 0;
    unsigned char ttl = 2;
    int bcast = 1;
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        DUER_LOGE("Create socket failed!\n");
        return -1;
    }
    if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, (char*)&loopchar, sizeof(loopchar)) < 0) {
        DUER_LOGE("setsockopt IP_MULTICAST_LOOP failed!\n");
        close(fd);
        return -1;
    }
    if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&ttl, sizeof(ttl)) < 0) {
        DUER_LOGE("setsockopt IP_MULTICAST_TTL failed!\n");
    }
    if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &bcast, sizeof(bcast)) < 0) {
        DUER_LOGE("setsockopt SO_BROADCAST failed!\n");
        close(fd);
        return -1;
    }
    return fd;
}

void send_ssdp_notifies(int fd, const char* hostip, unsigned short httpport,
                        unsigned int lifetime) {
    int i = 0;
    int len = 0;
    int ret = -1;
    char buf[512] = {0};
    struct sockaddr_in sockname;
    sockname.sin_family = AF_INET;
    sockname.sin_port = htons(SSDP_PORT);
    sockname.sin_addr.s_addr = inet_addr(SSDP_MCAST_ADDR);
    while (known_service_types[i]) {
        len = snprintf(buf, sizeof(buf),
                       "NOTIFY * HTTP/1.1\r\n"
                       "HOST:%s:%d\r\n"
                       "CACHE-CONTROL:max-age=%u\r\n"
                       "LOCATION:http://%s:%d" ROOTDESC_PATH"\r\n"
                       "SERVER: " MINIRENDER_SERVER_STRING "\r\n"
                       "NT:%s\r\n"
                       "USN:%s%s%s\r\n"
                       "NTS:ssdp:alive\r\n"
                       "\r\n",
                       SSDP_MCAST_ADDR, SSDP_PORT,
                       lifetime,
                       hostip, httpport,
                       known_service_types[i],
                       uuidvalue,
                       (i > 0 ? "::" : ""),
                       (i > 0 ? known_service_types[i] : ""));
        if (len < 0) {
            DUER_LOGE("snprintf error.\n");
        } else if (len >= sizeof(buf)) {
            DUER_LOGE("truncated output, len=%d.\n", len);
            len = sizeof(buf);
        }
        ret = sendto(fd, buf, len, 0, (struct sockaddr*)&sockname, sizeof(sockname));
        if (ret < 0) {
            DUER_LOGE("sendto err = %d.\n", ret);
        }
        DUER_LOGD("### ret = %d\n", ret);
        i++;
    }
}

int add_multicast_membership(int s) {
    if (s < 0) {
        DUER_LOGE("socket_id is negative");
        return -1;
    }

    int ret = 0;
    struct ip_mreq imr; /* Ip multicast membership */
    /* setting up imr structure */
    memset(&imr, '\0', sizeof(imr));
    imr.imr_multiaddr.s_addr = inet_addr(SSDP_MCAST_ADDR);
    imr.imr_interface.s_addr = inet_addr(hostip);
    /* Setting the socket options will guarantee, tha we will only receive
     * multicast traffic on a specific Interface.
     * In addition the kernel is instructed to send an igmp message (choose
     * mcast group) on the specific interface/subnet.
     */
    ret = setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*)&imr, sizeof(imr));
    if (ret < 0) {
        DUER_LOGE("setsockopt(udp, IP_ADD_MEMBERSHIP): host_ip:%s", hostip);
    }
    return ret;
}

int send_ssdp_response(int soket_id, const char* host_ip, int port, struct sockaddr_in sockname,
                       const char* st) {
    if (soket_id < 0 || host_ip == NULL || port < 0 || st == NULL) {
        return -1;
    }
    int send_len = 0, send_ret = 0;
    char buf[512];
    send_len = snprintf(buf, sizeof(buf), "HTTP/1.1 200 OK\r\n"
                        "CACHE-CONTROL: max-age=%u\r\n"
                        "EXT:\r\n"
                        "LOCATION: http://%s:%u" ROOTDESC_PATH "\r\n"
                        "SERVER: " MINIRENDER_SERVER_STRING "\r\n"
                        "ST: %s\r\n"
                        "USN: %s::%s\r\n"
                        "\r\n",
                        100,
                        host_ip, port,
                        st,
                        uuidvalue,
                        st);
    DUER_LOGI("Sending M-SEARCH response to %s:%d ST: %s\n buffer = %s\n",
              inet_ntoa(sockname.sin_addr), ntohs(sockname.sin_port),
              st, buf);
    send_ret = sendto(soket_id, buf, send_len, 0, (struct sockaddr*)&sockname,
                      sizeof(struct sockaddr_in) );
    DUER_LOGV("sendto::udp send len= %d\n", send_ret);
    if (send_ret < 0) {
        DUER_LOGE("sendto(udp) fail");
    }
    return send_ret;
}

int ssdp_send_response_to_cp(const char* local_ip, int local_server_port,
                             struct sockaddr_in sendername, char* st) {
    if (local_ip == NULL || local_server_port < 0) {
        return -1;
    }
    int soket_id_send = socket(PF_INET, SOCK_DGRAM, 0);
    int ov = 0;
    if (soket_id_send < 0) {
        DUER_LOGE("create send socket to send data fail %d", soket_id_send);
        return -1;
    }
    if (setsockopt(soket_id_send, SOL_SOCKET, SO_REUSEADDR, &ov, sizeof(ov)) < 0) {
        DUER_LOGE("setsockopt(udp, SO_REUSEADDR) fail");
        return -1;
    }

    struct sockaddr_in socket_name_new;
    memset(&socket_name_new, 0, sizeof(struct sockaddr_in));
    socket_name_new.sin_family = AF_INET;
    socket_name_new.sin_port = htons(0);
    socket_name_new.sin_addr.s_addr = htonl(0);

    if (bind(soket_id_send, (struct sockaddr*)&socket_name_new, sizeof(struct sockaddr_in)) < 0) {
        DUER_LOGE("bind socket_id fail");
        close(soket_id_send);
        return -1;
    }

    int ret = send_ssdp_response(soket_id_send, local_ip, local_server_port,
                                 sendername, st);
    close(soket_id_send);
    return ret;
}

int ssdp_process_m_search_for_media_renderer(const char* buf, int len, char* st_buf) {
    int ret = -1;
    char* st = NULL, *mx = NULL, *man = NULL, *mx_end = NULL;
    char* buffer = (char*)buf;
    int st_len = 0, mx_len = 0, mx_val = 0, index = 0, man_len = 0;
    for (; index < len; ++index) {
        if (buffer[index] == '*') {
            break;
        }
    }
    if (strcasestrc(buffer + index, "HTTP/1.1", '\r') == NULL) {
        return ret;
    }
    while (index < len) {
        while ((index < len) && (buffer[index] != '\r' || buffer[index + 1] != '\n')) {
            index++;
        }
        index += 2;
        if (strncasecmp(buffer + index, "ST:", strlen("ST:")) == 0) {
            st = buffer + index + strlen("ST:");
            st_len = 0;
            while (*st == ' ' || *st == '\t') {
                st++;
            }
            while (st[st_len] != '\r' && st[st_len] != '\n') {
                st_len++;
            }
        } else if (strncasecmp(buffer + index, "MX:", strlen("MX:")) == 0) {
            mx = buffer + index + strlen("MX:");
            mx_len = 0;
            while (*mx == ' ' || *mx == '\t') {
                mx++;
            }
            while (mx[mx_len] != '\r' && mx[mx_len] != '\n') {
                mx_len++;
            }
            mx_val = strtol(mx, &mx_end, 10);
        } else if (strncasecmp(buffer + index, "MAN:", strlen("MAN:")) == 0) {
            man = buffer + index + strlen("MAN:");
            man_len = 0;
            while (*man == ' ' || *man == '\t') {
                man++;
            }
            while (man[man_len] != '\r' && man[man_len] != '\n') {
                man_len++;
            }
        }
    }
    if (!man || (strncmp(man, "\"ssdp:discover\"", strlen("\"ssdp:discover\"")) != 0)) {
        DUER_LOGW("Ignoring invalid SSDP M-SEARCH from [bad %s header '%.*s']", "MAN", man_len, man);
        ret = -1;
    } else if (!mx || mx == mx_end || mx_val < 0) {
        DUER_LOGW("Ignoring invalid SSDP M-SEARCH from [bad %s header '%.*s']", "MX", mx_len, mx);
        ret = -1;
    } else if (st && (st_len > 0)) {
        
        DUER_LOGI("SSDP M-SEARCH ====== ST: %.*s, MX: %.*s, MAN: %.*s\n",
                  st_len, st, mx_len, mx, man_len, man);
        int length = strlen("urn:schemas-upnp-org:device:MediaRenderer:1");
        int length_for_ssdp_all = strlen("ssdp:all");
        int length_for_rootdevice = strlen("upnp:rootdevice");

        if (length <=  st_len
                && (memcmp(st, "urn:schemas-upnp-org:device:MediaRenderer:1", length) == 0)) {
            memcpy(st_buf, "urn:schemas-upnp-org:device:MediaRenderer:1", length);
            st_buf[length] = 0;
            ret = 0;
        } else if (length_for_ssdp_all <=  st_len
                   && (memcmp(st, "ssdp:all", length_for_ssdp_all) == 0)) {
            memcpy(st_buf, "urn:schemas-upnp-org:device:MediaRenderer:1", length);
            st_buf[length] = 0;
            ret = 0;
        } else if (length_for_rootdevice <=  st_len
                   && (memcmp(st, "upnp:rootdevice", length_for_rootdevice) == 0)) {
            memcpy(st_buf, "upnp:rootdevice", length_for_rootdevice);
            st_buf[length_for_rootdevice] = 0;
            ret = 0;
        } else {
            DUER_LOGD("Ignoring SSDP M-SEARCH not MediaRenderer\n");
            ret = -1;
        }
    } else {
        DUER_LOGI("Invalid SSDP M-SEARCH from");
        ret = -1;
    }
    return ret;
}


int open_and_conf_ssdp_receive_socket(void) {
    int soket_id;
    int i = 1;
    struct sockaddr_in sockname;
    soket_id = socket(PF_INET, SOCK_DGRAM, 0);
    if (soket_id < 0) {
        DUER_LOGE("socket(upd) fail");
        return -1;
    }
    if (setsockopt(soket_id, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i)) < 0) {
        DUER_LOGE("setsockopt(udp, SO_REUSEADDR) fail");
    }
    memset(&sockname, 0, sizeof(struct sockaddr_in));
    sockname.sin_family = AF_INET;
    sockname.sin_port = htons(SSDP_PORT);
    sockname.sin_addr.s_addr = inet_addr(SSDP_MCAST_ADDR);
    if (bind(soket_id, (struct sockaddr*)&sockname, sizeof(struct sockaddr_in)) < 0) {
        DUER_LOGE("bind(udp) fail");
        close(soket_id);
        return -1;
    }
    add_multicast_membership(soket_id);
    return soket_id;
}

int process_ssdp_request(int socket_id, const char* host_ip, int local_server_port) {
    int ret = -1;
    if (socket_id < 0 || host_ip == NULL) {
        DUER_LOGE("socket id and host_ip is null");
        return ret;
    }
    int read_len = 0;
    char bufr[1024];
    char st[80] = {0};
    struct sockaddr_in sendername;
    socklen_t len_r = sizeof(struct sockaddr_in);
    read_len = recvfrom(socket_id, bufr, sizeof(bufr) - 1, 0, (struct sockaddr*)&sendername, &len_r);
    if (read_len < 0) {
        DUER_LOGE("recvfrom(udp) fail len = %d", read_len);
        return ret;
    }
    if (read_len < 2) {
        DUER_LOGE("recvfrom(udp) len = %d is less 2", read_len);
        return ret;
    }
    bufr[read_len] = '\0';
    read_len -= 2;
    if (ntohs(sendername.sin_port) == ntohs(SSDP_PORT)) {
        ret = -1;
        DUER_LOGE("broadcast port is = %d ", SSDP_PORT);
        return ret;
    }
    if (memcmp(bufr, "M-SEARCH", strlen("M-SEARCH")) == 0) {
        DUER_LOGD("receive data from %s:%d: data = %s", inet_ntoa(sendername.sin_addr),
                  ntohs(sendername.sin_port), bufr);
        ret = ssdp_process_m_search_for_media_renderer(bufr, read_len, st);
        if (ret >= 0) {
            ret = ssdp_send_response_to_cp(host_ip, local_server_port, sendername, st);
            if (ret < 0) {
                DUER_LOGE("send data to : %s:%d is fail", inet_ntoa(sendername.sin_addr),
                          ntohs(sendername.sin_port));
            }
        } else {
            DUER_LOGI("is not MediaRenderer m_serch from : %s:%d", inet_ntoa(sendername.sin_addr),
                      ntohs(sendername.sin_port));
        }
    } else {
        DUER_LOGD("other udp broadcast");
        ret = -1;
    }
    return ret;
}
