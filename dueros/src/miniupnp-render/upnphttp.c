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

#include "upnphttp.h"
#include "upnpsoap.h"
#include "httppath.h"
#include "log.h"
#include "upnpglobalvars.h"
#include "utilities.h"
#include "upnpdescgen.h"
#include "heap_monitor.h"

enum EventType {
    E_INVALID,
    E_SUBSCRIBE,
    E_RENEW
};

struct upnphttp*
new_upnphttp(int s) {
    struct upnphttp* ret;
    if (s < 0) {
        return NULL;
    }
    ret = (struct upnphttp*)MALLOC(sizeof(struct upnphttp), UPNP);
    if (ret == NULL) {
        return NULL;
    }
    memset(ret, 0, sizeof(struct upnphttp));
    ret->socket = s;
    return ret;
}

void
close_socket_upnphttp(struct upnphttp* h) {
    if (close(h->socket) < 0) {
        DUER_LOGE("close_socket_upnphttp: close(%d): failed\n", h->socket);
    }
    h->socket = -1;
    h->state = 100;
}

void
delete_upnphttp(struct upnphttp* h) {
    if (h) {
        if (h->socket >= 0) {
            close_socket_upnphttp(h);
        }
        FREE(h->req_buf);
        FREE(h->res_buf);
        FREE(h);
    }
}

/* parse HttpHeaders of the REQUEST */
static void
parse_http_headers(struct upnphttp* h) {
    int client = 0;
    char* line;
    char* colon;
    char* p;
    int n;
    line = h->req_buf;
    /* TODO : check if req_buf, contentoff are ok */
    while (line < (h->req_buf + h->req_contentoff)) {
        colon = strchr(line, ':');
        if (colon) {
            if (strncasecmp(line, "Content-Length", 14) == 0) {
                p = colon;
                while (*p && (*p < '0' || *p > '9')) {
                    p++;
                }
                h->req_contentlen = atoi(p);
                if (h->req_contentlen < 0) {
                    DUER_LOGW("Invalid Content-Length %d", h->req_contentlen);
                    h->req_contentlen = 0;
                }
            } else if (strncasecmp(line, "SOAPAction", 10) == 0) {
                p = colon;
                n = 0;
                while (*p == ':' || *p == ' ' || *p == '\t') {
                    p++;
                }
                while (p[n] >= ' ') {
                    n++;
                }
                if (n >= 2 &&
                        ((p[0] == '"' && p[n - 1] == '"') ||
                         (p[0] == '\'' && p[n - 1] == '\''))) {
                    p++;
                    n -= 2;
                }
                h->req_soapAction = p;
                h->req_soapActionLen = n;
            } else if (strncasecmp(line, "Callback", 8) == 0) {
                p = colon;
                while (*p && *p != '<' && *p != '\r') {
                    p++;
                }
                n = 0;
                while (p[n] && p[n] != '>' && p[n] != '\r') {
                    n++;
                }
                h->req_Callback = p + 1;
                h->req_CallbackLen = MAX(0, n - 1);
            } else if (strncasecmp(line, "SID", 3) == 0) {
                //zqiu: fix bug for test 4.0.5
                //Skip extra headers like "SIDHEADER: xxxxxx xxx"
                for (p = line + 3; p < colon; p++) {
                    if (!isspace(*p)) {
                        p = NULL; //unexpected header
                        break;
                    }
                }
                if (p) {
                    p = colon + 1;
                    while (isspace(*p)) {
                        p++;
                    }
                    n = 0;
                    while (p[n] && !isspace(p[n])) {
                        n++;
                    }
                    h->req_SID = p;
                    h->req_SIDLen = n;
                }
            } else if (strncasecmp(line, "NT", 2) == 0) {
                p = colon + 1;
                while (isspace(*p)) {
                    p++;
                }
                n = 0;
                while (p[n] && !isspace(p[n])) {
                    n++;
                }
                h->req_NT = p;
                h->req_NTLen = n;
            }
            /* Timeout: Seconds-nnnn */
            /* TIMEOUT
            Recommended. Requested duration until subscription expires,
            either number of seconds or infinite. Recommendation
            by a UPnP Forum working committee. Defined by UPnP vendor.
            Consists of the keyword "Second-" followed (without an
            intervening space) by either an integer or the keyword "infinite". */
            else if (strncasecmp(line, "Timeout", 7) == 0) {
                p = colon + 1;
                while (isspace(*p)) {
                    p++;
                }
                if (strncasecmp(p, "Second-", 7) == 0) {
                    h->req_Timeout = atoi(p + 7);
                } else if (strncasecmp(p, "infinite", 8) == 0) {
                    h->req_Timeout = 1800;
                } else {
                    h->req_Timeout = 0;
                }
            }
            // Range: bytes=xxx-yyy
            else if (strncasecmp(line, "Range", 5) == 0) {
                p = colon + 1;
                while (isspace(*p)) {
                    p++;
                }
                if (strncasecmp(p, "bytes=", 6) == 0) {
                    h->reqflags |= FLAG_RANGE;
                    h->req_RangeStart = strtoll(p + 6, &colon, 10);
                    h->req_RangeEnd = colon ? atoll(colon + 1) : 0;
                    DUER_LOGD("Range Start-End: %lld - %lld\n",
                              (long long)h->req_RangeStart,
                              h->req_RangeEnd ? (long long)h->req_RangeEnd : -1);
                }
            } else if (strncasecmp(line, "Host", 4) == 0) {
                h->reqflags |= FLAG_HOST;
                //TODO
            } else if (strncasecmp(line, "User-Agent", 10) == 0) {
                //TODO
            } else if (strncasecmp(line, "Transfer-Encoding", 17) == 0) {
                p = colon + 1;
                while (isspace(*p)) {
                    p++;
                }
                if (strncasecmp(p, "chunked", 7) == 0) {
                    h->reqflags |= FLAG_CHUNKED;
                }
            } else if (strncasecmp(line, "Accept-Language", 15) == 0) {
                h->reqflags |= FLAG_LANGUAGE;
            }
        }
        line = strstr(line, "\r\n");
        if (!line) {
            return;
        }
        line += 2;
    }
    if (h->reqflags & FLAG_CHUNKED) {
        char* endptr;
        h->req_chunklen = -1;
        if (h->req_buflen <= h->req_contentoff) {
            return;
        }
        while ((line < (h->req_buf + h->req_buflen)) &&
                (h->req_chunklen = strtol(line, &endptr, 16)) &&
                (endptr != line)) {
            endptr = strstr(endptr, "\r\n");
            if (!endptr) {
                return;
            }
            line = endptr + h->req_chunklen + 2;
        }
        if (endptr == line) {
            h->req_chunklen = -1;
            return;
        }
    }
}

/* very minimalistic 400 error message */
static void
send400(struct upnphttp* h) {
    static const char body400[] =
        "<HTML><HEAD><TITLE>400 Bad Request</TITLE></HEAD>"
        "<BODY><H1>Bad Request</H1>The request is invalid"
        " for this HTTP version.</BODY></HTML>\r\n";
    h->respflags = FLAG_HTML;
    build_resp_upnphttp2(h, 400, "Bad Request",
                         body400, sizeof(body400) - 1);
    send_resp_upnphttp(h);
    close_socket_upnphttp(h);
}

/* very minimalistic 404 error message */
static void
send404(struct upnphttp* h) {
    static const char body404[] =
        "<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD>"
        "<BODY><H1>Not Found</H1>The requested URL was not found"
        " on this server.</BODY></HTML>\r\n";
    h->respflags = FLAG_HTML;
    build_resp_upnphttp2(h, 404, "Not Found",
                         body404, sizeof(body404) - 1);
    send_resp_upnphttp(h);
    close_socket_upnphttp(h);
}

/* very minimalistic 500 error message */
void
send500(struct upnphttp* h) {
    static const char body500[] =
        "<HTML><HEAD><TITLE>500 Internal Server Error</TITLE></HEAD>"
        "<BODY><H1>Internal Server Error</H1>Server encountered "
        "and Internal Error.</BODY></HTML>\r\n";
    h->respflags = FLAG_HTML;
    build_resp_upnphttp2(h, 500, "Internal Server Errror",
                         body500, sizeof(body500) - 1);
    send_resp_upnphttp(h);
    close_socket_upnphttp(h);
}

/* very minimalistic 501 error message */
void
send501(struct upnphttp* h) {
    static const char body501[] =
        "<HTML><HEAD><TITLE>501 Not Implemented</TITLE></HEAD>"
        "<BODY><H1>Not Implemented</H1>The HTTP Method "
        "is not implemented by this server.</BODY></HTML>\r\n";
    h->respflags = FLAG_HTML;
    build_resp_upnphttp2(h, 501, "Not Implemented",
                         body501, sizeof(body501) - 1);
    send_resp_upnphttp(h);
    close_socket_upnphttp(h);
}

/* Sends the description generated by the parameter */
static void
sendXMLdesc(struct upnphttp* h, char* (f)(int*)) {
    char* desc;
    int len;
    desc = f(&len);
    if (!desc) {
        DUER_LOGW("Failed to generate XML description\n");
        send500(h);
        return;
    }
    build_resp_upnphttp(h, desc, len);
    send_resp_upnphttp(h);
    close_socket_upnphttp(h);
    FREE(desc);
}

static int
check_event(struct upnphttp* h) {
    enum EventType type;
    if (h->req_Callback) {
        if (h->req_SID || !h->req_NT) {
            build_resp_upnphttp2(h, 400, "Bad Request",
                                 "<html><body>Bad request</body></html>", 37);
            type = E_INVALID;
        } else if (strncmp(h->req_Callback, "http://", 7) != 0 ||
                   strncmp(h->req_NT, "upnp:event", h->req_NTLen) != 0) {
            /* Missing or invalid CALLBACK : 412 Precondition Failed.
             * If CALLBACK header is missing or does not contain a valid HTTP URL,
             * the publisher must respond with HTTP error 412 Precondition Failed*/
            build_resp_upnphttp2(h, 412, "Precondition Failed", 0, 0);
            type = E_INVALID;
        } else {
            type = E_SUBSCRIBE;
        }
    } else if (h->req_SID) {
        /* subscription renew */
        if (h->req_NT) {
            build_resp_upnphttp2(h, 400, "Bad Request",
                                 "<html><body>Bad request</body></html>", 37);
            type = E_INVALID;
        } else {
            type = E_RENEW;
        }
    } else {
        build_resp_upnphttp2(h, 412, "Precondition Failed", 0, 0);
        type = E_INVALID;
    }
    return type;
}

static void
process_httpsubscribe_upnphttp(struct upnphttp* h, const char* path) {
    const char* sid;
    enum EventType type;
    DUER_LOGD("ProcessHTTPSubscribe %s\n", path);
    DUER_LOGD("Callback '%.*s' Timeout=%d\n",
              h->req_CallbackLen, h->req_Callback, h->req_Timeout);
    DUER_LOGD("SID '%.*s'\n", h->req_SIDLen, h->req_SID);
    type = check_event(h);
    if (type == E_SUBSCRIBE) {
        /* - add to the subscriber list
         * - respond HTTP/x.x 200 OK
         * - Send the initial event message */
        /* Server:, SID:; Timeout: Second-(xx|infinite) */
        sid = upnpevents_add_subscriber(path, h->req_Callback,
                                        h->req_CallbackLen, h->req_Timeout);
        h->respflags = FLAG_TIMEOUT;
        if (sid) {
            DUER_LOGD("generated sid=%s\n", sid);
            h->respflags |= FLAG_SID;
            h->req_SID = sid;
            h->req_SIDLen = strlen(sid);
        }
        build_resp_upnphttp(h, 0, 0);
    } else if (type == E_RENEW) {
        if (renew_subscription(h->req_SID, h->req_SIDLen, h->req_Timeout) < 0) {
            /* Invalid SID
               412 Precondition Failed. If a SID does not correspond to a known,
               un-expired subscription, the publisher must respond
               with HTTP error 412 Precondition Failed. */
            build_resp_upnphttp2(h, 412, "Precondition Failed", 0, 0);
        } else {
            /* A DLNA yydevice must enforce a 5 minute timeout */
            h->respflags = FLAG_TIMEOUT;
            h->req_Timeout = 300;
            h->respflags |= FLAG_SID;
            build_resp_upnphttp(h, 0, 0);
        }
    }
    send_resp_upnphttp(h);
    close_socket_upnphttp(h);
}

static void
process_httpunsubscribe_upnphttp(struct upnphttp* h, const char* path) {
    enum EventType type;
    DUER_LOGD("ProcessHTTPUnSubscribe %s\n", path);
    DUER_LOGD("SID '%.*s'\n", h->req_SIDLen, h->req_SID);
    /* Remove from the list */
    type = check_event(h);
    if (type != E_INVALID) {
        if (upnpevents_remove_subscriber(h->req_SID, h->req_SIDLen) < 0) {
            build_resp_upnphttp2(h, 412, "Precondition Failed", 0, 0);
        } else {
            build_resp_upnphttp(h, 0, 0);
        }
    }
    send_resp_upnphttp(h);
    close_socket_upnphttp(h);
}

/* process_httppost_upnphttp()
 * executes the SOAP query if it is possible */
static void
process_httppost_upnphttp(struct upnphttp* h) {
    if ((h->req_buflen - h->req_contentoff) >= h->req_contentlen) {
        if (h->req_soapAction) {
            /* we can process the request */
            DUER_LOGD("SOAPAction: %.*s\n", h->req_soapActionLen, h->req_soapAction);
            execute_soap_action(h,
                                h->req_soapAction,
                                h->req_soapActionLen);
        } else {
            static const char err400str[] =
                "<html><body>Bad request</body></html>";
            DUER_LOGW("No SOAPAction in HTTP headers\n");
            h->respflags = FLAG_HTML;
            build_resp_upnphttp2(h, 400, "Bad Request",
                                 err400str, sizeof(err400str) - 1);
            send_resp_upnphttp(h);
            close_socket_upnphttp(h);
        }
    } else {
        /* waiting for remaining data */
        h->state = 1;
        DUER_LOGW("waiting for remaining data.\n");
    }
}

/* Parse and process Http Query
 * called once all the HTTP headers have been received. */
static void
process_httpquery_upnphttp(struct upnphttp* h) {
    char HttpCommand[16];
    char HttpUrl[512];
    char* HttpVer;
    char* p;
    int i;
    p = h->req_buf;
    if (!p) {
        return;
    }
    for (i = 0; i < 15 && *p && *p != ' ' && *p != '\r'; i++) {
        HttpCommand[i] = *(p++);
    }
    HttpCommand[i] = '\0';
    while (*p == ' ') {
        p++;
    }
    for (i = 0; i < 511 && *p && *p != ' ' && *p != '\r'; i++) {
        HttpUrl[i] = *(p++);
    }
    HttpUrl[i] = '\0';
    while (*p == ' ') {
        p++;
    }
    HttpVer = h->HttpVer;
    for (i = 0; i < 15 && *p && *p != '\r'; i++) {
        HttpVer[i] = *(p++);
    }
    HttpVer[i] = '\0';
    parse_http_headers(h);
    /* see if we need to wait for remaining data */
    if ((h->reqflags & FLAG_CHUNKED)) {
        if (h->req_chunklen == -1) {
            send400(h);
            return;
        }
        if (h->req_chunklen) {
            h->state = 2;
            return;
        }
        char* chunkstart, *chunk, *endptr, *endbuf;
        chunk = endbuf = chunkstart = h->req_buf + h->req_contentoff;
        while ((h->req_chunklen = strtol(chunk, &endptr, 16)) && (endptr != chunk)) {
            endptr = strstr(endptr, "\r\n");
            if (!endptr) {
                send400(h);
                return;
            }
            endptr += 2;
            memmove(endbuf, endptr, h->req_chunklen);
            endbuf += h->req_chunklen;
            chunk = endptr + h->req_chunklen;
        }
        h->req_contentlen = endbuf - chunkstart;
        h->req_buflen = endbuf - h->req_buf;
        h->state = 100;
    }
    DUER_LOGD("HTTP REQUEST: %.*s\n", h->req_buflen, h->req_buf);
    if (strcmp("POST", HttpCommand) == 0) {
        h->req_command = EPOST;
        process_httppost_upnphttp(h);
    } else if (strcmp("GET", HttpCommand) == 0) {
        if (((strcmp(h->HttpVer, "HTTP/1.1") == 0) && !(h->reqflags & FLAG_HOST))
                || (h->reqflags & FLAG_INVALID_REQ)) {
            DUER_LOGW("Invalid request, responding ERROR 400.  (No Host specified in HTTP headers?)\n");
            send400(h);
            return;
        }
        h->req_command = EGET;
        if (strcmp(ROOTDESC_PATH, HttpUrl) == 0) {
            DUER_LOGD("HTTPGET: %s, send response.\n", ROOTDESC_PATH);
            sendXMLdesc(h, gen_root_desc);
        } else if (strcmp(CONNECTIONMGR_PATH, HttpUrl) == 0) {
            DUER_LOGD("HTTPGET: %s, send response.\n", CONNECTIONMGR_PATH);
            sendXMLdesc(h, gen_connection_manager_scpd);
        } else if (strcmp(AVTRANSPORT_PATH, HttpUrl) == 0) {
            DUER_LOGD("HTTPGET: %s, send response.\n", AVTRANSPORT_PATH);
            sendXMLdesc(h, gen_av_transport_scpd);
        } else if (strcmp(RENDERINGCONTROL_PATH, HttpUrl) == 0) {
            DUER_LOGD("HTTPGET: %s, send response.\n", RENDERINGCONTROL_PATH);
            sendXMLdesc(h, gen_rendering_control_scpd);
        } else {
            DUER_LOGW("%s not found, responding ERROR 404\n", HttpUrl);
            send404(h);
        }
    } else if (strcmp("SUBSCRIBE", HttpCommand) == 0) {
        h->req_command = ESUBSCRIBE;
        process_httpsubscribe_upnphttp(h, HttpUrl);
    } else if (strcmp("UNSUBSCRIBE", HttpCommand) == 0) {
        h->req_command = EUNSUBSCRIBE;
        process_httpunsubscribe_upnphttp(h, HttpUrl);
    } else {
        DUER_LOGW("Unsupported HTTP Command %s\n", HttpCommand);
        send501(h);
    }
}

void
process_upnphttp(struct upnphttp* h) {
    char buf[2048];
    int n;
    if (!h) {
        return;
    }
    switch (h->state) {
    case 0:
        n = recv(h->socket, buf, 2048, 0);
        if (n < 0) {
            DUER_LOGE("recv (state0) failed: n=%d\n", n);
            h->state = 100;
        } else if (n == 0) {
            DUER_LOGW("HTTP Connection closed unexpectedly\n");
            h->state = 100;
        } else {
            int new_req_buflen;
            const char* endheaders;
            /* if 1st arg of realloc() is null,
             * realloc behaves the same as malloc() */
            new_req_buflen = n + h->req_buflen + 1;
            DUER_LOGD("new_req_buflen = %d.\n", new_req_buflen);
            if (new_req_buflen >= 1024 * 1024) {
                DUER_LOGW("Receive headers too large (received %d bytes)\n", new_req_buflen);
                h->state = 100;
                break;
            }
            h->req_buf = (char*)REALLOC(h->req_buf, new_req_buflen, UPNP);
            if (!h->req_buf) {
                DUER_LOGE("(state=%d) realloc failed.\n", h->state);
                h->state = 100;
                break;
            }
            memcpy(h->req_buf + h->req_buflen, buf, n);
            h->req_buflen += n;
            h->req_buf[h->req_buflen] = '\0';
            /* search for the string "\r\n\r\n" */
            endheaders = strstr(h->req_buf, "\r\n\r\n");
            if (endheaders) {
                h->req_contentoff = endheaders - h->req_buf + 4;
                h->req_contentlen = h->req_buflen - h->req_contentoff;
                process_httpquery_upnphttp(h);
            }
        }
        break;
    case 1:
    case 2:
        n = recv(h->socket, buf, sizeof(buf), 0);
        if (n < 0) {
            DUER_LOGE("recv (state%d): n=%d\n", h->state, n);
            h->state = 100;
        } else if (n == 0) {
            DUER_LOGW("HTTP Connection closed unexpectedly\n");
            h->state = 100;
        } else {
            buf[sizeof(buf) - 1] = '\0';
            /*fwrite(buf, 1, n, stdout);*/  /* debug */
            h->req_buf = (char*)REALLOC(h->req_buf, n + h->req_buflen, UPNP);
            if (!h->req_buf) {
                DUER_LOGE("(state=%d) realloc failed.\n", h->state);
                h->state = 100;
                break;
            }
            memcpy(h->req_buf + h->req_buflen, buf, n);
            h->req_buflen += n;
            if ((h->req_buflen - h->req_contentoff) >= h->req_contentlen) {
                /* Need the struct to point to the realloc'd memory locations */
                if (h->state == 1) {
                    parse_http_headers(h);
                    process_httppost_upnphttp(h);
                } else if (h->state == 2) {
                    process_httpquery_upnphttp(h);
                }
            }
        }
        break;
    default:
        DUER_LOGE("Unexpected state: %d\n", h->state);
    }
}

/* with response code and response message
 * also allocate enough memory */

void
build_header_upnphttp(struct upnphttp* h, int respcode,
                      const char* respmsg,
                      int bodylen) {
    static const char httpresphead[] =
        "%s %d %s\r\n"
        "Content-Type: %s\r\n"
        "Connection: close\r\n"
        "Content-Length: %d\r\n"
        "Server: " MINIRENDER_SERVER_STRING "\r\n";
    //time_t curtime = time(NULL);
    //char date[30];
    int templen;
    struct string_s res;
    if (!h->res_buf) {
        templen = sizeof(httpresphead) + 256 + bodylen;
        h->res_buf = (char*)MALLOC(templen, UPNP);
        h->res_buf_alloclen = templen;
    }
    res.data = h->res_buf;
    res.size = h->res_buf_alloclen;
    res.off = 0;
    strcatf(&res, httpresphead, "HTTP/1.1",
            respcode, respmsg,
            (h->respflags & FLAG_HTML) ? "text/html" : "text/xml; charset=\"utf-8\"",
            bodylen);
    /* Additional headers */
    if (h->respflags & FLAG_TIMEOUT) {
        strcatf(&res, "Timeout: Second-");
        if (h->req_Timeout) {
            strcatf(&res, "%d\r\n", h->req_Timeout);
        } else {
            strcatf(&res, "300\r\n");
        }
    }
    if (h->respflags & FLAG_SID) {
        strcatf(&res, "SID: %.*s\r\n", h->req_SIDLen, h->req_SID);
    }
    if (h->reqflags & FLAG_LANGUAGE) {
        strcatf(&res, "Content-Language: en\r\n");
    }
    //strftime(date, 30,"%a, %d %b %Y %H:%M:%S GMT" , gmtime(&curtime));
    //strcatf(&res, "Date: %s\r\n", date);
    strcatf(&res, "EXT:\r\n");
    strcatf(&res, "\r\n");
    h->res_buflen = res.off;
    if (h->res_buf_alloclen < (h->res_buflen + bodylen)) {
        h->res_buf = (char*)REALLOC(h->res_buf, (h->res_buflen + bodylen), UPNP);
        h->res_buf_alloclen = h->res_buflen + bodylen;
    }
}

void
build_resp_upnphttp2(struct upnphttp* h, int respcode,
                     const char* respmsg,
                     const char* body, int bodylen) {
    build_header_upnphttp(h, respcode, respmsg, bodylen);
    if (body) {
        memcpy(h->res_buf + h->res_buflen, body, bodylen);
    }
    h->res_buflen += bodylen;
}

/* responding 200 OK ! */
void
build_resp_upnphttp(struct upnphttp* h, const char* body, int bodylen) {
    build_resp_upnphttp2(h, 200, "OK", body, bodylen);
}

void
send_resp_upnphttp(struct upnphttp* h) {
    int n;
    DUER_LOGD("HTTP RESPONSE: %.*s\n", h->res_buflen, h->res_buf);
    n = send(h->socket, h->res_buf, h->res_buflen, 0);
    if (n < 0) {
        DUER_LOGE("send(res_buf) n=%d\n", n);
    } else if (n < h->res_buflen) {
        /* TODO : handle correctly this case */
        DUER_LOGW("send(res_buf): %d bytes sent (out of %d)\n",
                  n, h->res_buflen);
    }
}

