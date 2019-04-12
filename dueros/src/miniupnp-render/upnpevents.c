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
#include "upnpevents.h"
#include "httppath.h"
#include "upnpglobalvars.h"
#include "upnpsoap.h"
#include "upnpdescgen.h"
#include "utilities.h"
#include "log.h"
#include <errno.h>
#include "heap_monitor.h"

/* stuctures definitions */
struct Subscriber {
    LIST_ENTRY(Subscriber) entries;
    struct UpnpEventNotify* notify;
    uint32_t timeout;
    uint32_t seq;
    enum SubscriberServiceEnum service;
    char uuid[42];
    char callback[];
};

struct UpnpEventNotify {
    int s;  /* socket */
    enum { ECreated = 1,
           EConnecting,
           ESending,
           EWaitingForResponse,
           EFinished,
           EError
         } state;
    struct Subscriber* sub;
    char* buffer;
    int buffersize;
    int tosend;
    int sent;
    const char* path;
    char addrstr[16];
    char portstr[8];
    LIST_ENTRY(UpnpEventNotify) entries;
};

/* prototypes */
static void
upnp_event_create_notify(struct Subscriber* sub);

/* Subscriber list */
LIST_HEAD(listhead, Subscriber) subscriberlist = { NULL };

/* notify list */
LIST_HEAD(listheadnotif, UpnpEventNotify) notifylist = { NULL };

/* seq for uuid generation */
static unsigned short s_seq = 0;

/* create a new subscriber */
static struct Subscriber*
new_subscriber(const char* eventurl, const char* callback, int callbacklen) {
    struct Subscriber* tmp;

    if (!eventurl || !callback || !callbacklen)
        return NULL;

    tmp = CALLOC(1, sizeof(struct Subscriber) + callbacklen + 1, UPNP);

    if (strcmp(eventurl, CONNECTIONMGR_EVENTURL) == 0) {
        tmp->service = EConnectionManager;
    } else if (strcmp(eventurl, RENDERINGCONTROL_EVENTURL) == 0) {
        tmp->service = ERenderingControl;
    } else if (strcmp(eventurl, AVTRANSPORT_EVENTURL) == 0) {
        tmp->service = EAVTransport;
    } else {
        FREE(tmp);
        return NULL;
    }

    memcpy(tmp->callback, callback, callbacklen);
    tmp->callback[callbacklen] = '\0';

    /* make a dummy uuid */
    strncpyt(tmp->uuid, uuidvalue, sizeof(tmp->uuid));
    s_seq++;
    unsigned int time = internal_time();
	unsigned char tmpuuid[16] = {0};
    //DUER_LOGD("UUID=%s, time=%d.", tmp->uuid, time);
    sscanf(tmp->uuid, "uuid:%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", &tmpuuid[0],
            &tmpuuid[1], &tmpuuid[2], &tmpuuid[3], &tmpuuid[4], &tmpuuid[5], &tmpuuid[6], &tmpuuid[7],
            &tmpuuid[8], &tmpuuid[9], &tmpuuid[10], &tmpuuid[11], &tmpuuid[12], &tmpuuid[13], &tmpuuid[14],
            &tmpuuid[15]);
    tmpuuid[3] = (unsigned char)time;
    tmpuuid[2] = (unsigned char)(time >> 8);
    tmpuuid[1] = (unsigned char)(time >> 16);
    tmpuuid[0] = (unsigned char)(time >> 24);
    tmpuuid[5] = (unsigned char)s_seq;
    tmpuuid[4] = (unsigned char)(s_seq >> 8);
    //DUER_LOGD("UUID_LEN=%d", UUID_LEN);
    snprintf(tmp->uuid, UUID_LEN, "uuid:%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", tmpuuid[0],
            tmpuuid[1], tmpuuid[2], tmpuuid[3], tmpuuid[4], tmpuuid[5], tmpuuid[6], tmpuuid[7],
            tmpuuid[8], tmpuuid[9], tmpuuid[10], tmpuuid[11], tmpuuid[12], tmpuuid[13], tmpuuid[14],
            tmpuuid[15]);

	return tmp;
}

/* creates a new subscriber and adds it to the subscriber list
 * also initiate 1st notify */
const char*
upnpevents_add_subscriber(const char* eventurl, const char* callback, int callbacklen, 
                          int timeout) {
    struct Subscriber* tmp;
    int found = 0;
    enum SubscriberServiceEnum service;

    if (strcmp(eventurl, CONNECTIONMGR_EVENTURL) == 0) {
        service = EConnectionManager;
    } else if (strcmp(eventurl, RENDERINGCONTROL_EVENTURL) == 0) {
        service = ERenderingControl;
    } else if (strcmp(eventurl, AVTRANSPORT_EVENTURL) == 0) {
        service = EAVTransport;
    } else {
        return NULL;
    }

    for (tmp = subscriberlist.lh_first; tmp != NULL; tmp = tmp->entries.le_next) {
        DUER_LOGD("service=%d, callback=%s, sid=%s", tmp->service, tmp->callback, tmp->uuid);
        if (tmp->service==service && memcmp(tmp->callback, callback, callbacklen)==0) {
            found = 1;
            break;
        }
    }
    if (found) {
        DUER_LOGI("Duplicated subscription.");
        return tmp->uuid;
    }

    DUER_LOGD("addSubscriber(%s, %.*s, %d)\n",
              eventurl, callbacklen, callback, timeout);
    tmp = new_subscriber(eventurl, callback, callbacklen);

    if (!tmp)
        return NULL;

    if (timeout)
        tmp->timeout = internal_time() + timeout;

    LIST_INSERT_HEAD(&subscriberlist, tmp, entries);
    //upnp_event_create_notify(tmp);
    return tmp->uuid;
}

/* renew a subscription (update the timeout) */
int
renew_subscription(const char* sid, int sidlen, int timeout) {
    struct Subscriber* sub;

    for (sub = subscriberlist.lh_first; sub != NULL; sub = sub->entries.le_next) {
        if (memcmp(sid, sub->uuid, 41) == 0) {
            sub->timeout = (timeout ? internal_time() + timeout : 0);
            return 0;
        }
    }

    return -1;
}

int
upnpevents_remove_subscriber(const char* sid, int sidlen) {
    struct Subscriber* sub;

    if (!sid)
        return -1;

    DUER_LOGD("remove_subscriber(%.*s)\n",
              sidlen, sid);

    for (sub = subscriberlist.lh_first; sub != NULL; sub = sub->entries.le_next) {
        if (memcmp(sid, sub->uuid, 41) == 0) {
            if (sub->notify) {
                sub->notify->sub = NULL;
            }

            LIST_REMOVE(sub, entries);
            FREE(sub);
            return 0;
        }
    }

    return -1;
}

void
upnpevents_remove_subscribers(void) {
    struct Subscriber* sub;

    for (sub = subscriberlist.lh_first; sub != NULL;
            sub = subscriberlist.lh_first) {
        upnpevents_remove_subscriber(sub->uuid, sizeof(sub->uuid));
    }
}

void
upnp_event_var_change_notify(enum SubscriberServiceEnum service) {
    struct Subscriber* sub;

    for (sub = subscriberlist.lh_first; sub != NULL; sub = sub->entries.le_next) {
        if (sub->service == service && sub->notify == NULL)
            upnp_event_create_notify(sub);
    }
}

/* create and add the notify object to the list */
static void
upnp_event_create_notify(struct Subscriber* sub) {
    struct UpnpEventNotify* obj;
    int flags;
    obj = CALLOC(1, sizeof(struct UpnpEventNotify), UPNP);

    if (!obj) {
        DUER_LOGE("%s: calloc(): %s\n", "upnp_event_create_notify", strerror(errno));
        return;
    }

    obj->sub = sub;
    obj->state = ECreated;
    obj->s = socket(PF_INET, SOCK_STREAM, 0);

    if (obj->s < 0) {
        DUER_LOGE("%s: socket(): %s\n", "upnp_event_create_notify", strerror(errno));
        goto error;
    }

    if ((flags = fcntl(obj->s, F_GETFL, 0)) < 0) {
        DUER_LOGE("%s: fcntl(..F_GETFL..): %s\n",
                  "upnp_event_create_notify", strerror(errno));
        goto error;
    }

    if (fcntl(obj->s, F_SETFL, flags | O_NONBLOCK) < 0) {
        DUER_LOGE("%s: fcntl(..F_SETFL..): %s\n",
                  "upnp_event_create_notify", strerror(errno));
        goto error;
    }

    if (sub)
        sub->notify = obj;

    LIST_INSERT_HEAD(&notifylist, obj, entries);
    return;
error:

    if (obj->s >= 0)
        close(obj->s);

    FREE(obj);
}

static void
upnp_event_notify_connect(struct UpnpEventNotify* obj) {
    int i;
    const char* p;
    unsigned short port;
    struct sockaddr_in addr;

    if (!obj)
        return;

    memset(&addr, 0, sizeof(addr));
    i = 0;

    if (obj->sub == NULL) {
        obj->state = EError;
        return;
    }

    p = obj->sub->callback;
    p += 7;	/* http:// */

    while (*p != '/' && *p != ':' && i < (sizeof(obj->addrstr) - 1))
        obj->addrstr[i++] = *(p++);

    obj->addrstr[i] = '\0';

    if (*p == ':') {
        obj->portstr[0] = *p;
        i = 1;
        p++;
        port = (unsigned short)atoi(p);

        while (*p != '/' && *p != '\0') {
            if (i < 7) obj->portstr[i++] = *p;

            p++;
        }

        obj->portstr[i] = 0;
    } else {
        port = 80;
        obj->portstr[0] = '\0';
    }

    if (*p)
        obj->path = p;
    else
        obj->path = "/";

    addr.sin_family = AF_INET;
    inet_aton(obj->addrstr, &addr.sin_addr);
    addr.sin_port = htons(port);
    DUER_LOGD("%s: '%s' %hu '%s'\n", "upnp_event_notify_connect",
              obj->addrstr, port, obj->path);
    obj->state = EConnecting;

    if (connect(obj->s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        if (errno != EINPROGRESS && errno != EWOULDBLOCK) {
            DUER_LOGE("%s: connect(): %s\n", "upnp_event_notify_connect", strerror(errno));
            obj->state = EError;
        }
    }
}

static void upnp_event_prepare(struct UpnpEventNotify* obj) {
    static const char notifymsg[] =
            "NOTIFY %s HTTP/1.1\r\n"
            "Host: %s%s\r\n"
            "Content-Type: text/xml; charset=\"utf-8\"\r\n"
            "Content-Length: %d\r\n"
            "NT: upnp:event\r\n"
            "NTS: upnp:propchange\r\n"
            "SID: %s\r\n"
            "SEQ: %u\r\n"
            "Connection: close\r\n"
            "Cache-Control: no-cache\r\n"
            "\r\n"
            "%.*s\r\n";
    char* xml;
    int l;

    if (obj->sub == NULL) {
        obj->state = EError;
        return;
    }

    switch (obj->sub->service) {
    case EConnectionManager:
        //xml = connmanager_event_notify(&l);
        break;

    case ERenderingControl:
        //xml = rendercontrol_event_notify(&l);
        break;

    case EAVTransport:
        xml = avtransport_event_notify(&l);
        break;

    default:
        xml = NULL;
        l = 0;
    }

    obj->tosend = asprintf(&(obj->buffer), notifymsg, obj->path,
                           obj->addrstr, obj->portstr, l + 2,
                           obj->sub->uuid, obj->sub->seq,
                           l, xml);
    obj->buffersize = obj->tosend;
    FREE(xml);
    DUER_LOGD("Sending UPnP Event response:\n%s\n", obj->buffer);
    obj->state = ESending;
}

static void upnp_event_send(struct UpnpEventNotify* obj) {
    int i;
    DUER_LOGD("Sending UPnP Event:\n%s", obj->buffer + obj->sent);

    while (obj->sent < obj->tosend) {
        i = send(obj->s, obj->buffer + obj->sent, obj->tosend - obj->sent, 0);

        if (i < 0) {
            DUER_LOGW("%s: send(): %s\n", "upnp_event_send", strerror(errno));
            obj->state = EError;
            return;
        }

        obj->sent += i;
    }

    if (obj->sent == obj->tosend)
        obj->state = EWaitingForResponse;
}

static void upnp_event_recv(struct UpnpEventNotify* obj) {
    int n;
    n = recv(obj->s, obj->buffer, obj->buffersize, 0);

    if (n < 0) {
        DUER_LOGE("%s: recv(): %s\n", "upnp_event_recv", strerror(errno));
        obj->state = EError;
        return;
    }

    DUER_LOGD("%s: (%dbytes) %.*s\n", "upnp_event_recv",
              n, n, obj->buffer);
    obj->state = EFinished;

    if (obj->sub) {
        obj->sub->seq++;

        if (!obj->sub->seq)
            obj->sub->seq++;
    }
}

static void
upnp_event_process_notify(struct UpnpEventNotify* obj) {
    switch (obj->state) {
    case EConnecting:
        /* now connected or failed to connect */
        upnp_event_prepare(obj);
        upnp_event_send(obj);
        break;

    case ESending:
        upnp_event_send(obj);
        break;

    case EWaitingForResponse:
        upnp_event_recv(obj);
        break;

    case EFinished:
        close(obj->s);
        obj->s = -1;
        break;

    default:
        DUER_LOGE("upnp_event_process_notify: unknown state\n");
    }
}

void upnpevents_select_fds(fd_set* readset, fd_set* writeset, int* max_fd) {
    struct UpnpEventNotify* obj;

    for (obj = notifylist.lh_first; obj != NULL; obj = obj->entries.le_next) {
        DUER_LOGD("upnpevents_select_fds: %p %d %d\n",
                  obj, obj->state, obj->s);

        if (obj->s >= 0) {
            switch (obj->state) {
            case ECreated:
                upnp_event_notify_connect(obj);

                if (obj->state != EConnecting)
                    break;

            case EConnecting:
            case ESending:
                FD_SET(obj->s, writeset);

                if (obj->s > *max_fd)
                    *max_fd = obj->s;

                break;

            case EWaitingForResponse:
                FD_SET(obj->s, readset);

                if (obj->s > *max_fd)
                    *max_fd = obj->s;

                break;

            default:
                break;
            }
        }
    }
}

void upnpevents_process_fds(fd_set* readset, fd_set* writeset) {
    struct UpnpEventNotify* obj;
    struct UpnpEventNotify* next;
    struct Subscriber* sub;
    struct Subscriber* subnext;
    uint32_t curtime;

    for (obj = notifylist.lh_first; obj != NULL; obj = obj->entries.le_next) {
        DUER_LOGD("%s: %p %d %d %d %d\n",
                  "upnpevents_process_fds", obj, obj->state, obj->s,
                  FD_ISSET(obj->s, readset), FD_ISSET(obj->s, writeset));

        if (obj->s >= 0) {
            if (FD_ISSET(obj->s, readset) || FD_ISSET(obj->s, writeset))
                upnp_event_process_notify(obj);
        }
    }

    obj = notifylist.lh_first;

    while (obj != NULL) {
        next = obj->entries.le_next;

        if (obj->state == EError || obj->state == EFinished) {
            if (obj->s >= 0) {
                close(obj->s);
            }

            if (obj->sub)
                obj->sub->notify = NULL;

#if 0 /* Just let it time out instead of explicitly removing the subscriber */

            /* remove also the subscriber from the list if there was an error */
            if (obj->state == EError && obj->sub) {
                LIST_REMOVE(obj->sub, entries);
                FREE(obj->sub);
            }

#endif
            FREE(obj->buffer);
            LIST_REMOVE(obj, entries);
            FREE(obj);
        }

        obj = next;
    }

    /* remove timeouted subscribers */
    curtime = internal_time();

    for (sub = subscriberlist.lh_first; sub != NULL;) {
        subnext = sub->entries.le_next;

        if (sub->timeout && curtime > sub->timeout && sub->notify == NULL) {
            DUER_LOGI("Subscriber timeout, remove it. {%s}", sub->callback);
            LIST_REMOVE(sub, entries);
            FREE(sub);
        }

        sub = subnext;
    }
}

