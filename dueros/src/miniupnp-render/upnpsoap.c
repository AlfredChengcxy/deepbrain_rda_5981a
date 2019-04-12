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

#include "upnpsoap.h"
#include "upnpglobalvars.h"
#include "utilities.h"
#include "upnphttp.h"
#include "upnpevents.h"
#include "upnpreplyparse.h"
#include "log.h"
#include "heap_monitor.h"
#include "baidu_media_manager_adapter.h"
#include "baidu_dlna_media_cb.h"

#define CONNMGR_TYPE    "urn:schemas-upnp-org:service:ConnectionManager:1"
#define CONTROL_TYPE    "urn:schemas-upnp-org:service:RenderingControl:1"
#define TRANSPORT_TYPE  "urn:schemas-upnp-org:service:AVTransport:1"

#define UPNP_EVENT_XML_NS "urn:schemas-upnp-org:event-1-0"
#define TRANSPORT_EVENT_XML_NS "urn:schemas-upnp-org:metadata-1-0/AVT/"
#define CONTROL_EVENT_XML_NS "urn:schemas-upnp-org:metadata-1-0/RCS/"

static char* transport_states[] = {
    "STOPPED",
    "PLAYING",
    "PAUSED_PLAYBACK"
};

static char* transport_status[] = {
    "OK",
    "ERROR_OCCURRED"
};

static int current_mute = 0;
static int current_volume = 100;

static char* current_uri = NULL;
static char* current_uri_decoded = NULL;
static char* current_uri_metadata = NULL;
static enum TransportState state = STOPPED;
static enum TransportStatus status = OK;

static char current_time[] = "00:00:00";
static char duration_time[] = "00:00:00.000";
static char seek_time[] = "00:00:00";

static int register_once = 0;

static char* internal_strdup(const char* str) {
    size_t len;
    char* copy;

    len = strlen(str) + 1;

    if (!(copy = (char*)MALLOC(len, UPNP))) {
        return 0;
    }
    memcpy(copy, str, len);

    return copy;
}

static char* strrpl(char* in, char* out, int outlen, const char* src,
                    char* dst) {
    char* p = in;
    unsigned int len = outlen - 1;

    if ((NULL == src) || (NULL == dst) || (NULL == in) || (NULL == out)) {
        return NULL;
    }

    if ((strcmp(in, "") == 0) || (strcmp(src, "") == 0)) {
        return NULL;
    }

    if (outlen <= 0) {
        return NULL;
    }

    while ((*p != '\0') && (len > 0)) {
        if (strncmp(p, src, strlen(src)) != 0) {
            int n = strlen(out);

            out[n] = *p;
            out[n + 1] = '\0';

            p++;
            len--;
        } else {
            //strcat_s(out, outlen, dst);
            strcat(out, dst);
            p += strlen(src);
            len -= strlen(dst);
        }
    }

    return out;
}

static void get_duration_time(char* metadata, char* duration_time) {
    if (!metadata)
        return;

    char* str = NULL;

    if ((str = strstr(metadata, "duration=")) != NULL) {
        if (sscanf(str, "duration=\"%[^\"]", duration_time) == 1) {
        } else if (sscanf(str, "duration=&quot;%[^&quot;]", duration_time) == 1) {
        } else {
            DUER_LOGW("parse duration time error!");
            memcpy(duration_time, "00:00:00.000", strlen("00:00:00.000") + 1);
        }
    }

    DUER_LOGD("duration_time=%s", duration_time);
}

static void get_current_time(char* seek_time, char* current_time) {
    int hour, min, sec;
    unsigned int seconds, duration;

    if (sscanf(seek_time, "%d:%d:%d", &hour, &min, &sec) != 3) {
        DUER_LOGW("seek_time is invalid!");
        hour = min = sec = 0;
    }

    seconds = timer_read();
    seconds += hour * 3600 + min * 60 + sec;

    if (sscanf(duration_time, "%d:%d:%d.", &hour, &min, &sec) != 3) {
        DUER_LOGW("duration_time is invalid!");
        duration = 0;
    } else {
        duration = hour * 3600 + min * 60 + sec;
    }

    if (duration != 0 && seconds > duration) {
        seconds = duration;
    }

    hour = seconds / 3600;
    min = (seconds % 3600) / 60;
    sec = seconds % 3600 % 60;

    DUER_LOGD("seconds=%d, hour=%d, min=%d, sec=%d", seconds, hour, min, sec);
    snprintf(current_time, strlen(current_time) + 1, "%02d:%02d:%02d", hour, min,
             sec);
}

static int get_seek_len(char* duration_time, char* seek_time) {
    unsigned int total_len, recv_len;
    int hour, min, sec;
    int total_sec, seek_sec;
    unsigned int seek_len = 0;

    get_download_progress_adapter(&total_len, &recv_len);

    if (total_len == 0) {
        return 0;
    }

    if (sscanf(duration_time, "%d:%d:%d.", &hour, &min, &sec) != 3) {
        DUER_LOGW("duration_time is invalid!");
        return 0;
    } else {
        total_sec = hour * 3600 + min * 60 + sec;
    }

    if (sscanf(seek_time, "%d:%d:%d", &hour, &min, &sec) != 3) {
        DUER_LOGW("seek_time is invalid!");
        return 0;
    } else {
        seek_sec = hour * 3600 + min * 60 + sec;
    }

    seek_len = ((unsigned long)seek_sec * total_len) / total_sec;
    DUER_LOGD("total_len=%d, total_sec=%d, seek_sec=%d, seek_len=%d", total_len,
              total_sec,
              seek_sec, seek_len);

    return seek_len;
}

static void build_send_and_close_soap_resp(struct upnphttp* h,
                               const char* body, int bodylen) {
    static const char beforebody[] =
            //"<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n"
            "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
            "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
            "<s:Body>";
    const char afterbody[] =
            "</s:Body>"
            "</s:Envelope>\r\n";

    if (!body || bodylen < 0) {
        send500(h);
        return;
    }

    build_header_upnphttp(h, 200, "OK",  sizeof(beforebody) - 1
                          + sizeof(afterbody) - 1 + bodylen);
    memcpy(h->res_buf + h->res_buflen, beforebody, sizeof(beforebody) - 1);
    h->res_buflen += sizeof(beforebody) - 1;
    memcpy(h->res_buf + h->res_buflen, body, bodylen);
    h->res_buflen += bodylen;
    memcpy(h->res_buf + h->res_buflen, afterbody, sizeof(afterbody) - 1);
    h->res_buflen += sizeof(afterbody) - 1;
    send_resp_upnphttp(h);
    close_socket_upnphttp(h);
}

static void get_mute(struct upnphttp* h, const char* action) {
    static const char resp[] =
            "<u:%sResponse "
            "xmlns:u=\"%s\">"
            "<CurrentMute>"
            "%d"
            "</CurrentMute>"
            "</u:%sResponse>";
    char body[sizeof(resp) + 80];
    int bodylen;

    bodylen = snprintf(body, sizeof(body), resp, action, CONTROL_TYPE, current_mute,
                       action);
    build_send_and_close_soap_resp(h, body, bodylen);
}

static void set_mute(struct upnphttp* h, const char* action) {
    static const char resp[] =
            "<u:%sResponse "
            "xmlns:u=\"%s\">"
            "</u:%sResponse>";
    char body[sizeof(resp) + 80];
    int bodylen;

    struct NameValueParserData data;
    const char* id;
    const char* channel;
    const char* mute;
    parse_name_value(h->req_buf + h->req_contentoff, h->req_contentlen, &data,
                     XML_STORE_EMPTY_FL);
    id = get_value_from_name_value_list(&data, "InstanceID");
    channel = get_value_from_name_value_list(&data, "Channel");
    mute = get_value_from_name_value_list(&data, "DesiredMute");
    clear_name_value_list(&data);

    if (id && channel && mute) {
        DUER_LOGD("InstanceID=%s, Channel=%s, DesiredMute=%s.", id, channel, mute);
        current_mute = atoi(mute);
        if (current_mute) {
            current_volume = 0;
            set_volume_adapter(current_volume);
        }
    } else {
        DUER_LOGI("id or channel or mute is incorrect.");
        return;
    }

    bodylen = snprintf(body, sizeof(body), resp, action, CONTROL_TYPE, action);
    build_send_and_close_soap_resp(h, body, bodylen);
}

static void get_volume(struct upnphttp* h, const char* action) {
    static const char resp[] =
            "<u:%sResponse "
            "xmlns:u=\"%s\">"
            "<CurrentVolume>"
            "%d"
            "</CurrentVolume>"
            "</u:%sResponse>";
    char body[sizeof(resp) + 80];
    int bodylen;

    struct NameValueParserData data;
    const char* id, *channel;
    parse_name_value(h->req_buf + h->req_contentoff, h->req_contentlen, &data,
                     XML_STORE_EMPTY_FL);
    id = get_value_from_name_value_list(&data, "InstanceID");
    channel = get_value_from_name_value_list(&data, "Channel");
    clear_name_value_list(&data);

    if (id && channel)
        DUER_LOGD("InstanceID=%s, Channel=%s.", id, channel);

    unsigned char vol = get_volume_adapter();
    DUER_LOGD("vol=%d", vol);
    current_volume = vol; // (vol * 100) / 15;

    bodylen = snprintf(body, sizeof(body), resp, action, CONTROL_TYPE,
                       current_volume, action);
    build_send_and_close_soap_resp(h, body, bodylen);
}

static void set_volume(struct upnphttp* h, const char* action) {
    static const char resp[] =
            "<u:%sResponse "
            "xmlns:u=\"%s\">"
            "</u:%sResponse>";
    char body[sizeof(resp) + 80];
    int bodylen;

    struct NameValueParserData data;
    const char* id, *channel, *volume;
    parse_name_value(h->req_buf + h->req_contentoff, h->req_contentlen, &data,
                     XML_STORE_EMPTY_FL);
    id = get_value_from_name_value_list(&data, "InstanceID");
    channel = get_value_from_name_value_list(&data, "Channel");
    volume = get_value_from_name_value_list(&data, "DesiredVolume");
    clear_name_value_list(&data);

    if (id && channel && volume) {
        DUER_LOGD("InstanceID=%s, Channel=%s, DesiredVolume=%s.", id, channel, volume);
        current_volume = atoi(volume);
        if (current_volume) {
            current_mute = 0;
        } else {
            current_mute = 1;
        }

        //unsigned char vol = current_volume * 15 / 100;
        set_volume_adapter(current_volume);
    } else {
        DUER_LOGI("id or channel or volume is incorrect.");
        return;
    }

    bodylen = snprintf(body, sizeof(body), resp, action, CONTROL_TYPE, action);
    build_send_and_close_soap_resp(h, body, bodylen);
}

static void get_protocol_info(struct upnphttp* h, const char* action) {
    static const char resp[] =
            "<u:%sResponse "
            "xmlns:u=\"%s\">"
            "<Source></Source>"
            "<Sink>"
            RESOURCE_PROTOCOL_INFO_VALUES
            "</Sink>"
            "</u:%sResponse>";
    char body[sizeof(resp) + 100];
    int bodylen;

    bodylen = snprintf(body, sizeof(body), resp, action, CONNMGR_TYPE, action);
    build_send_and_close_soap_resp(h, body, bodylen);
}

static void set_avtransport_uri(struct upnphttp* h, const char* action) {
    static const char resp[] =
            "<u:%sResponse "
            "xmlns:u=\"%s\">"
            "</u:%sResponse>";
    char body[sizeof(resp) + 80];
    int bodylen;

    if (register_once == 0) {
        register_dlna_listener();
        register_once = 1;
    }

    struct NameValueParserData data;
    const char* id, *uri, *metadata;
    parse_name_value(h->req_buf + h->req_contentoff, h->req_contentlen, &data,
                     XML_STORE_EMPTY_FL);
    id = get_value_from_name_value_list(&data, "InstanceID");
    uri = get_value_from_name_value_list(&data, "CurrentURI");
    metadata = get_value_from_name_value_list(&data, "CurrentURIMetaData");

    if (current_uri) {
        FREE(current_uri);
        current_uri = NULL;
    }

    if (current_uri_decoded) {
        FREE(current_uri_decoded);
        current_uri_decoded = NULL;
    }

    if (current_uri_metadata) {
        FREE(current_uri_metadata);
        current_uri_metadata = NULL;
    }

    get_duration_time(metadata, duration_time);
    memcpy(seek_time, "00:00:00", strlen("00:00:00") + 1);
    memcpy(current_time, "00:00:00", strlen("00:00:00") + 1);

    current_uri = internal_strdup(uri);
    current_uri_metadata = internal_strdup(metadata);
    DUER_LOGD("id=%s, current_uri=%s, current_uri_metadata=%s", id, current_uri,
              current_uri_metadata);

    /* some url contains special characters like "&amp;" etc. */
    current_uri_decoded = internal_strdup(uri);
    memset(current_uri_decoded, 0, strlen(uri));
    strrpl(current_uri, current_uri_decoded, strlen(uri) + 1, "&amp;", "&");
    DUER_LOGD("current_uri_decoded=%s", current_uri_decoded);

    clear_name_value_list(&data);

    bodylen = snprintf(body, sizeof(body), resp, action, TRANSPORT_TYPE, action);
    build_send_and_close_soap_resp(h, body, bodylen);
}

static void get_transport_info(struct upnphttp* h, const char* action) {
    static const char resp[] =
            "<u:%sResponse "
            "xmlns:u=\"%s\">"
            "<CurrentTransportState>"
            "%s"
            "</CurrentTransportState>"
            "<CurrentTransportStatus>"
            "%s"
            "</CurrentTransportStatus>"
            "<CurrentSpeed>1</CurrentSpeed>"
            "</u:%sResponse>";
    char body[sizeof(resp) + 105];
    int bodylen;

    struct NameValueParserData data;
    const char* id;
    parse_name_value(h->req_buf + h->req_contentoff, h->req_contentlen, &data,
                     XML_STORE_EMPTY_FL);
    id = get_value_from_name_value_list(&data, "InstanceID");

    if (id)
        DUER_LOGD("InstanceID=%s.", id);

    clear_name_value_list(&data);

    bodylen = snprintf(body, sizeof(body), resp, action, TRANSPORT_TYPE,
                       transport_states[state],
                       transport_status[status], action);
    build_send_and_close_soap_resp(h, body, bodylen);
}

static void get_position_info(struct upnphttp* h, const char* action) {
    static const char resp[] =
            "<u:%sResponse "
            "xmlns:u=\"%s\">"
            "<Track>1</Track>"
            "<TrackDuration>"
            "%s"
            "</TrackDuration>"
            "<TrackMetaData></TrackMetaData>"
            "<TrackURI>"
            "%s"
            "</TrackURI>"
            "<RelTime>"
            "%s"
            "</RelTime>"
            "<AbsTime></AbsTime>"
            "<RelCount></RelCount>"
            "<AbsCount></AbsCount>"
            "</u:%sResponse>";
    char body[sizeof(resp) + 80 + (current_uri ? strlen(current_uri) : 0)];
    int bodylen;

    get_current_time(seek_time, current_time);
    bodylen = snprintf(body, sizeof(body), resp, action, TRANSPORT_TYPE, duration_time,
                       current_uri ? current_uri : "", current_time, action);
    build_send_and_close_soap_resp(h, body, bodylen);
}

static void get_media_info(struct upnphttp* h, const char* action) {
    static const char resp[] =
            "<u:%sResponse "
            "xmlns:u=\"%s\">"
            "<NrTracks>"
            "%d"
            "</NrTracks>"
            "<MediaDuration></MediaDuration>"
            "<CurrentURI>"
            "%s"
            "</CurrentURI>"
            "<CurrentURIMetaData></CurrentURIMetaData>"
            "<NextURI></NextURI>"
            "<NextURIMetaData></NextURIMetaData>"
            "<PlayMedium></PlayMedium>"
            "<RecordMedium></RecordMedium>"
            "<WriteStatus></WriteStatus>"
            "</u:%sResponse>";
    char body[sizeof(resp) + 80 + (current_uri ? strlen(current_uri) : 0)];
    int bodylen;

    bodylen = snprintf(body, sizeof(body), resp, action, TRANSPORT_TYPE, current_uri ? 1 : 0,
                       current_uri ? current_uri : "", action);
    build_send_and_close_soap_resp(h, body, bodylen);
}

static void ts_play(struct upnphttp* h, const char* action) {
    static const char resp[] =
            "<u:%sResponse "
            "xmlns:u=\"%s\">"
            "</u:%sResponse>";
    char body[sizeof(resp) + 60];
    int bodylen;

    if (state == STOPPED) {
        DUER_LOGD("PLAY...");
        media_play_url(current_uri_decoded);
    } else if (state == PAUSED_PLAYBACK) {
        DUER_LOGD("RESUME...");
        media_pause_resume_url();
    }
    DUER_LOGD("Get media player status: %d.", get_player_status_adapter());
    state = PLAYING;
    timer_start();

    bodylen = snprintf(body, sizeof(body), resp, action, TRANSPORT_TYPE, action);
    build_send_and_close_soap_resp(h, body, bodylen);

    DUER_LOGD("### event notify: %d.", EAVTransport);
    upnp_event_var_change_notify(EAVTransport);
}

static void ts_pause(struct upnphttp* h, const char* action) {
    static const char resp[] =
            "<u:%sResponse "
            "xmlns:u=\"%s\">"
            "</u:%sResponse>";
    char body[sizeof(resp) + 60];
    int bodylen;

    media_pause_resume_url();
    state = PAUSED_PLAYBACK;
    timer_stop();

    bodylen = snprintf(body, sizeof(body), resp, action, TRANSPORT_TYPE, action);
    build_send_and_close_soap_resp(h, body, bodylen);

    /* TODO: get_current_time here for the time being, remove it after add
     * var_change_notify(service, func_cb) in future. */
    get_current_time(seek_time, current_time);
    DUER_LOGD("### event notify: %d.", EAVTransport);
    upnp_event_var_change_notify(EAVTransport);
}

static void ts_stop(struct upnphttp* h, const char* action) {
    static const char resp[] =
            "<u:%sResponse "
            "xmlns:u=\"%s\">"
            "</u:%sResponse>";
    char body[sizeof(resp) + 60];
    int bodylen;

    if (state != STOPPED) {
        media_stop_url(current_uri_decoded);
    }

    state = STOPPED;
    timer_stop();
    timer_reset();

    bodylen = snprintf(body, sizeof(body), resp, action, TRANSPORT_TYPE, action);
    build_send_and_close_soap_resp(h, body, bodylen);

    DUER_LOGD("### event notify: %d.", EAVTransport);
    upnp_event_var_change_notify(EAVTransport);
}

/*
<InstanceID>0</InstanceID>
<Unit>REL_TIME</Unit>
<Target>0:02:19</Target>
*/
static void ts_seek(struct upnphttp* h, const char* action) {
    static const char resp[] =
            "<u:%sResponse "
            "xmlns:u=\"%s\">"
            "</u:%sResponse>";
    char body[sizeof(resp) + 60];
    int bodylen = 0;

    int seeklen = 0;
    struct NameValueParserData data;
    const char* target;

    parse_name_value(h->req_buf + h->req_contentoff, h->req_contentlen, &data,
                     XML_STORE_EMPTY_FL);
    target = get_value_from_name_value_list(&data, "Target");
    if (target) {
        DUER_LOGD("Target=%s.", target);
        memcpy(seek_time, target, strlen(target) + 1);
        seeklen = get_seek_len(duration_time, seek_time);
    }
    clear_name_value_list(&data);

    media_seek(seeklen);

    timer_stop();
    timer_reset();
    if (state == PLAYING) {
        timer_start();
    }
    /* Depends on current media player seek implementation, that is,
     * PAUSED->SEEK(stop-adjust-play) */
    if (state == PAUSED_PLAYBACK) {
        state = PLAYING;
    }

    get_current_time(seek_time, current_time);
    DUER_LOGD("### event notify: %d.", EAVTransport);
    upnp_event_var_change_notify(EAVTransport);

    bodylen = snprintf(body, sizeof(body), resp, action, TRANSPORT_TYPE, action);
    build_send_and_close_soap_resp(h, body, bodylen);
}


/* Standard Errors:
 *
 * errorCode errorDescription Description
 * --------	---------------- -----------
 * 401 		Invalid Action 	No action by that name at this service.
 * 402 		Invalid Args 	Could be any of the following: not enough in args,
 * 							too many in args, no in arg by that name,
 * 							one or more in args are of the wrong data type.
 * 403 		Out of Sync 	Out of synchronization.
 * 501 		Action Failed 	May be returned in current state of service
 * 							prevents invoking that action.
 * 600-699 	TBD 			Common action errors. Defined by UPnP Forum
 * 							Technical Committee.
 * 700-799 	TBD 			Action-specific errors for standard actions.
 * 							Defined by UPnP Forum working committee.
 * 800-899 	TBD 			Action-specific errors for non-standard actions.
 * 							Defined by UPnP vendor.
*/
static void soap_error(struct upnphttp* h, int err_code, const char* err_desc) {
    static const char resp[] =
            "<s:Envelope "
            "xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
            "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
            "<s:Body>"
            "<s:Fault>"
            "<faultcode>s:Client</faultcode>"
            "<faultstring>UPnPError</faultstring>"
            "<detail>"
            "<UPnPError xmlns=\"urn:schemas-upnp-org:control-1-0\">"
            "<errorCode>%d</errorCode>"
            "<errorDescription>%s</errorDescription>"
            "</UPnPError>"
            "</detail>"
            "</s:Fault>"
            "</s:Body>"
            "</s:Envelope>";
    char body[2048];
    int bodylen;

    DUER_LOGD("Returning UPnPError %d: %s\n", err_code, err_desc);
    bodylen = snprintf(body, sizeof(body), resp, err_code, err_desc);
    build_resp_upnphttp2(h, 500, "Internal Server Error", body, bodylen);
    send_resp_upnphttp(h);
    close_socket_upnphttp(h);
}

static const struct {
    const char* method_name;
    void (*method_impl)(struct upnphttp*, const char*);
}
soap_methods[] = {
    // RenderingControl
    {"GetMute", get_mute},
    {"SetMute", set_mute},
    {"GetVolume", get_volume},
    {"SetVolume", set_volume},

    // ConnectionManager
    {"GetProtocolInfo", get_protocol_info},

    // AVTransport
    {"SetAVTransportURI", set_avtransport_uri},
    {"GetTransportInfo", get_transport_info},
    {"GetPositionInfo", get_position_info},
    {"GetMediaInfo", get_media_info},
    {"Play", ts_play},
    {"Pause", ts_pause},
    {"Stop", ts_stop},
    {"Seek", ts_seek},
    { 0, 0 }
};

void execute_soap_action(struct upnphttp* h, const char* action, int n) {
    char* p;
    p = strchr(action, '#');

    if (p) {
        int i = 0;
        int len;
        int methodlen;
        char* p2;
        p++;
        p2 = strchr(p, '"');

        if (p2)
            methodlen = p2 - p;
        else
            methodlen = n - (p - action);

        DUER_LOGD("SoapMethod: %.*s\n", methodlen, p);

        while (soap_methods[i].method_name) {
            len = strlen(soap_methods[i].method_name);

            if (strncmp(p, soap_methods[i].method_name, len) == 0) {
                soap_methods[i].method_impl(h, soap_methods[i].method_name);
                return;
            }

            i++;
        }

        DUER_LOGW("SoapMethod: Unknown: %.*s\n", methodlen, p);
    }

    soap_error(h, 401, "Invalid Action");
}

char* avtransport_event_notify(int* len) {
    static const char buf[] =
            "<e:propertyset xmlns:e=\"%s\">"
            "<e:property>"
            "<LastChange>&lt;?xml version=\"1.0\"?&gt;"
            "&lt;Event xmlns=\"%s\"&gt;"
            "&lt;InstanceID val=\"0\"&gt;"
            "&lt;TransportState val=\"%s\"/&gt;"
            "&lt;CurrentTransportActions val=\"%s\"/&gt;"
            "&lt;RelativeTimePosition val=\"%s\"/&gt;"
            "&lt;/InstanceID&gt;"
            "&lt;/Event&gt;"
            "</LastChange>"
            "</e:property>"
            "</e:propertyset>";

    int l = sizeof(buf) + 120;
    char* xml = (char*)MALLOC(l, UPNP);
    if (xml == NULL) {
        DUER_LOGE("malloc failed!");
        return NULL;
    }

    memset(xml, 0, l);
    *len = snprintf(xml, l, buf, UPNP_EVENT_XML_NS, TRANSPORT_EVENT_XML_NS,
                    transport_states[state],
                    state == STOPPED ? "PLAY,SEEK" : state == PLAYING ? "PAUSE,STOP,SEEK" :
                    "PLAY,STOP,SEEK", current_time);
    DUER_LOGD("len=%d, strlen=%d, xml=%s", *len, strlen(xml), xml);
	
    return xml;
}

int get_transport_state() {
    return state;
}

void set_transport_state(enum TransportState st) {
    DUER_LOGD("Set state:%s", transport_states[st]);
    state = st;
    if (state == PAUSED_PLAYBACK || state == STOPPED) {
        timer_stop();
    }
}

void reclaim_res() {
    if (current_uri) {
        FREE(current_uri);
        current_uri = NULL;
    }

    if (current_uri_decoded) {
        FREE(current_uri_decoded);
        current_uri_decoded = NULL;
    }

    if (current_uri_metadata) {
        FREE(current_uri_metadata);
        current_uri_metadata = NULL;
    }
}

