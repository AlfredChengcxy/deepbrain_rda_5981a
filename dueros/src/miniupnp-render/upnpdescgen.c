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

#include "upnpdescgen.h"
#include <stdio.h>
#include <string.h>
#include "heap_monitor.h"
#include "httppath.h"
#include "log.h"
#include "upnpglobalvars.h"
#include "utilities.h"

#define INITHELPER(i, n) ((char *)((n<<16)|i))
#define EVENTED 1<<7

static const char xmlver[] =
    "<?xml version=\"1.0\"?>\r\n";

static const char root_service[] =
    "scpd xmlns=\"urn:schemas-upnp-org:service-1-0\"";

static const char root_device[] =
    "root xmlns=\"urn:schemas-upnp-org:device-1-0\"";

static const char* const upnptypes[] = {
    "string",
    "boolean",
    "ui2",
    "ui4",
    "i4",
    "uri",
    "int",
    "bin.base64"
};

static const char* const upnpdefaultvalues[] = {
    0,
    "Unconfigured"
};

static const char* const upnpallowedvalueranges[] = {
    0,
    "0",    /*1*/
    "100",
    "1",
    0,      /*4*/
    "0",    /*5*/
    "4294967295",
    "1",
    0,      /*8*/
    "0",    /*9*/
    "4294967295",
    0
};

static const char* const upnpallowedvalues[] = {
    0,          /* 0 */
    "DSL",          /* 1 */
    "POTS",
    "Cable",
    "Ethernet",
    0,
    "Up",           /* 6 */
    "Down",
    "Initializing",
    "Unavailable",
    0,
    "TCP",          /* 11 */
    "UDP",
    0,
    "Unconfigured",     /* 14 */
    "IP_Routed",
    "IP_Bridged",
    0,
    "Unconfigured",     /* 18 */
    "Connecting",
    "Connected",
    "PendingDisconnect",
    "Disconnecting",
    "Disconnected",
    0,
    "ERROR_NONE",       /* 25 */
    0,
    "OK",           /* 27 */
    "ContentFormatMismatch",
    "InsufficientBandwidth",
    "UnreliableChannel",
    "Unknown",
    0,
    "Input",        /* 33 */
    "Output",
    0,
    "BrowseMetadata",   /* 36 */
    "BrowseDirectChildren",
    0,
    "COMPLETED",        /* 39 */
    "ERROR",
    "IN_PROGRESS",
    "STOPPED",
    0,
    RESOURCE_PROTOCOL_INFO_VALUES,      /* 44 */
    0,
    "0",            /* 46 */
    0,
    " ",         /* 48 */
    0,
    "STOPPED",          /*50*/
    "PLAYING",
    "PAUSED_PLAYBACK",
    0,
    "OK", /*54*/
    "ERROR_OCCURRED",
    0,
    "1",  /*57*/
    0,
    "Master", /*59*/
    "LF",
    "RF",
    0,
    "REL_TIME", /*63*/
    0
};

/* root Description of the UPnP Device */
static const struct XMLElt rootDesc[] = {
    {root_device, INITHELPER(1, 3)},
    {"specVersion", INITHELPER(4, 2)},
    {"/URLBase", url_base},
    {"device", INITHELPER(6, (10))},
    {"/major", "1"},
    {"/minor", "0"},
    {"/deviceType", "urn:schemas-upnp-org:device:MediaRenderer:1"},
    {"/friendlyName", friendly_name},   /* required */
    {"/manufacturer", "baidu"},     /* required */
    {"/manufacturerURL", "www.baidu.com"},  /* optional */
    {"/modelDescription", "minirenderer1.0.0"}, /* recommended */
    {"/modelName", "minirenderer"}, /* required */
    {"/modelNumber", "1.0.0"},
    {"/modelURL", "www.baidu.com"},

    {"/UDN", uuidvalue},    /* required */
    {"serviceList", INITHELPER((16), 3)},

    {"service", INITHELPER((19), 5)},
    {"service", INITHELPER((24), 5)},
    {"service", INITHELPER((29), 5)},

    {"/serviceType", "urn:schemas-upnp-org:service:AVTransport:1"},
    {"/serviceId", "urn:upnp-org:serviceId:AVTransport"},
    {"/controlURL", AVTRANSPORT_CONTROLURL},
    {"/eventSubURL", AVTRANSPORT_EVENTURL},
    {"/SCPDURL", AVTRANSPORT_PATH},

    {"/serviceType", "urn:schemas-upnp-org:service:ConnectionManager:1"},
    {"/serviceId", "urn:upnp-org:serviceId:ConnectionManager"},
    {"/controlURL", CONNECTIONMGR_CONTROLURL},
    {"/eventSubURL", CONNECTIONMGR_EVENTURL},
    {"/SCPDURL", CONNECTIONMGR_PATH},

    {"/serviceType", "urn:schemas-upnp-org:service:RenderingControl:1"},
    {"/serviceId", "urn:upnp-org:serviceId:RenderingControl"},
    {"/controlURL", RENDERINGCONTROL_CONTROLURL},
    {"/eventSubURL", RENDERINGCONTROL_EVENTURL},
    {"/SCPDURL", RENDERINGCONTROL_PATH}

};

/* For ConnectionManager */
static const struct Argument GetProtocolInfoArgs[] = {
    {"Source", 2, 0},
    {"Sink", 2, 1},
    {NULL, 0, 0}
};

static const struct Argument GetCurrentConnectionIDsArgs[] = {
    {"ConnectionIDs", 2, 2},
    {NULL, 0, 0}
};

static const struct Argument GetCurrentConnectionInfoArgs[] = {
    {"ConnectionID", 1, 7},
    {"RcsID", 2, 9},
    {"AVTransportID", 2, 8},
    {"ProtocolInfo", 2, 6},
    {"PeerConnectionManager", 2, 4},
    {"PeerConnectionID", 2, 7},
    {"Direction", 2, 5},
    {"Status", 2, 3},
    {NULL, 0, 0}
};

static const struct Action ConnectionManagerActions[] = {
    {"GetProtocolInfo", GetProtocolInfoArgs}, /* R */
    {"GetCurrentConnectionIDs", GetCurrentConnectionIDsArgs}, /* R */
    {"GetCurrentConnectionInfo", GetCurrentConnectionInfoArgs}, /* R */
    {0, 0}
};

static const struct StateVar ConnectionManagerVars[] = {
    {"SourceProtocolInfo", 0 | EVENTED, 0, 0, 44}, /* required */
    {"SinkProtocolInfo", 0 | EVENTED, 0, 0, 48}, /* required */
    {"CurrentConnectionIDs", 0 | EVENTED, 0, 0, 46}, /* required */
    {"A_ARG_TYPE_ConnectionStatus", 0, 0, 27}, /* required */
    {"A_ARG_TYPE_ConnectionManager", 0, 0}, /* required */
    {"A_ARG_TYPE_Direction", 0, 0, 33}, /* required */
    {"A_ARG_TYPE_ProtocolInfo", 0, 0}, /* required */
    {"A_ARG_TYPE_ConnectionID", 4, 0}, /* required */
    {"A_ARG_TYPE_AVTransportID", 4, 0}, /* required */
    {"A_ARG_TYPE_RcsID", 4, 0}, /* required */
    {0, 0}
};

/*For AVTransport*/
static const struct Argument SetAVTransportURIArgs[] = {
    {"InstanceID", 1, 0},
    {"CurrentURI", 1, 1},
    {"CurrentURIMetaData", 1, 2},
    {NULL, 0, 0}
};

static const struct Argument GetTransportInfoArgs[] = {
    {"InstanceID", 1, 0},
    {"CurrentTransportState", 2, 3},
    {"CurrentTransportStatus", 2, 4},
    {"CurrentSpeed", 2, 5},
    {NULL, 0, 0}
};

static const struct Argument GetPositionInfoArgs[] = {
    {"InstanceID", 1, 0},
    {"Track", 2, 6},
    {"TrackDuration", 2, 7},
    {"TrackMetaData", 2, 8},
    {"TrackURI", 2, 9},
    {"RelTime", 2, 10},
    {"AbsTime", 2, 11},
    {"RelCount", 2, 12},
    {"AbsCount", 2, 13},
    {NULL, 0, 0}
};

static const struct Argument GetMediaInfoArgs[] = {
    {"InstanceID", 1, 0},
    {"NrTracks", 2, 15},
    {"MediaDuration", 2, 16},
    {"CurrentURI", 2, 1},
    {"CurrentURIMetaData", 2, 2},
    {"NextURI", 2, 17},
    {"NextURIMetaData", 2, 18},
    {"PlayMedium", 2, 19},
    {"RecordMedium", 2, 20},
    {"WriteStatus", 2, 21},
    {NULL, 0, 0}
};

static const struct Argument PlayArgs[] = {
    {"InstanceID", 1, 0},
    {"Speed", 1, 14},
    {NULL, 0, 0}
};

static const struct Argument PauseArgs[] = {
    {"InstanceID", 1, 0},
    {NULL, 0, 0}
};

static const struct Argument StopArgs[] = {
    {"InstanceID", 1, 0},
    {NULL, 0, 0}
};

static const struct Argument SeekArgs[] = {
    {"InstanceID", 1, 0},
    {"Unit", 1, 22},
    {"Target", 1, 23},
    {NULL, 0, 0}
};

static const struct Action AVTransportActions[] = {
    {"SetAVTransportURI", SetAVTransportURIArgs},
    {"GetTransportInfo", GetTransportInfoArgs},
    {"GetPositionInfo", GetPositionInfoArgs},
    {"GetMediaInfo", GetMediaInfoArgs},
    {"Play", PlayArgs},
    {"Pause", PauseArgs},
    {"Stop", StopArgs},
    {"Seek", SeekArgs},
    {0, 0}
};

static const struct StateVar AVTransportVars[] = {
    {"A_ARG_TYPE_InstanceID", 3, 0, 0, 0},
    {"AVTransportURI", 0, 0, 0, 0},
    {"AVTransportURIMetaData", 0, 0, 0, 0}, /*2*/
    {"TransportState", 0, 0, 50, 0},
    {"TransportStatus", 0, 0, 54, 0},
    {"TransportPlaySpeed", 0, 0, 57, 0}, /*5*/
    {"CurrentTrack", 3, 0, 0, 0, 5},   /*6*/
    {"CurrentTrackDuration", 0, 0, 0, 0},
    {"CurrentTrackMetaData", 0, 0, 0, 0},
    {"CurrentTrackURI", 0, 0, 0, 0},
    {"RelativeTimePosition", 0, 0, 0, 0}, /*10*/
    {"AbsoluteTimePosition", 0, 0, 0, 0},
    {"RelativeCounterPosition", 0, 0, 0, 0},
    {"AbsoluteCounterPosition", 0, 0, 0, 0}, /*13*/
    {"TransportPlaySpeed", 0, 0, 57, 0},
    {"NumberOfTracks", 3, 0, 0, 0, 9}, /*15*/
    {"CurrentMediaDuration", 0, 0, 0, 0},
    {"NextAVTransportURI", 0, 0, 0, 0},
    {"NextAVTransportURIMetaData", 0, 0, 0, 0}, /*18*/
    {"PlaybackStorageMedium", 0, 0, 0, 0},
    {"RecordStorageMedium", 0, 0, 0, 0},
    {"RecordMediumWriteStatus", 0, 0, 0, 0},
    {"A_ARG_TYPE_SeekMode", 0, 0, 63, 0}, /*22*/
    {"A_ARG_TYPE_SeekTarget", 0, 0, 0, 0},
    {"LastChange", 0 | EVENTED, 0, 0, 0},
    {0, 0}
};

/* For RenderingControl */
static const struct Argument GetMuteArgs[] = {
    {"InstanceID", 1, 0},
    {"Channel", 1, 1},
    {"CurrentMute", 2, 2},
    {NULL, 0, 0}
};

static const struct Argument SetMuteArgs[] = {
    {"InstanceID", 1, 0},
    {"Channel", 1, 1},
    {"DesiredMute", 1, 2},
    {NULL, 0, 0}
};

static const struct Argument GetVolumeArgs[] = {
    {"InstanceID", 1, 0},
    {"Channel", 1, 1},
    {"CurrentVolume", 2, 3},
    {NULL, 0, 0}
};

static const struct Argument SetVolumeArgs[] = {
    {"InstanceID", 1, 0},
    {"Channel", 1, 1},
    {"DesiredVolume", 1, 3},
    {NULL, 0, 0}
};

static const struct Action RenderingControlActions[] = {
    {"GetMute", GetMuteArgs},
    {"SetMute", SetMuteArgs},
    {"GetVolume", GetVolumeArgs},
    {"SetVolume", SetVolumeArgs},
    {0, 0}
};

static const struct StateVar RenderingControlVars[] = {
    {"A_ARG_TYPE_InstanceID", 3, 0, 0, 0},
    {"A_ARG_TYPE_Channel", 0, 0, 59, 0},
    {"Mute", 1, 0, 0, 0 }, /**2*/
    {"Volume", 2, 0, 0, 0, 1},
    {"LastChange", 0 | EVENTED, 0, 0, 0},
    {0, 0}
};

static const struct ServiceDesc scpdConnectionManager = { ConnectionManagerActions, ConnectionManagerVars };

static const struct ServiceDesc scpdAVTransport = {AVTransportActions, AVTransportVars};

static const struct ServiceDesc scpdRenderingControl = {RenderingControlActions, RenderingControlVars};

/*built-in function*/
char* gen_xml(char* str, int* len, int* tmplen, const struct XMLElt* p) {
    if (NULL == str || NULL == len || NULL == tmplen || NULL == p) {
        DUER_LOGW("in put str ptr is null or len ptr is null or tmplen or p is null");
        return NULL;
    }
    unsigned int current_node_index = 0, current_node_sub_index = 1, k = 0;
    int top = -1;
    const char* eltname = NULL, *s = NULL;
    char c;
    char element[64] = {0};
    struct {
        unsigned short current_node_index;
        unsigned short current_node_sub_index;
        const char* eltname;
    } pile[16]; /*stack*/
    for (;;) {
        eltname = p[current_node_index].eltname;
        if (NULL == eltname) {
            DUER_LOGW("eltname is null");
            return str;
        }
        if (eltname[0] == '/') {
            str = strcat_char(str, len, tmplen, '<');
            str = strcat_str(str, len, tmplen, eltname + 1);
            str = strcat_char(str, len, tmplen, '>');
            str = strcat_str(str, len, tmplen, p[current_node_index].data);
            str = strcat_char(str, len, tmplen, '<');
            sscanf(eltname, "%s", element);
            str = strcat_str(str, len, tmplen, element);
            str = strcat_char(str, len, tmplen, '>');
            for (;;) {
                if (top < 0) {
                    return str;
                }
                current_node_index = ++(pile[top].current_node_index);
                current_node_sub_index = pile[top].current_node_sub_index;
                if (current_node_index == current_node_sub_index) {
                    str = strcat_char(str, len, tmplen, '<');
                    str = strcat_char(str, len, tmplen, '/');
                    s = pile[top].eltname;
                    for (c = *s; c > ' '; c = *(++s)) {
                        str = strcat_char(str, len, tmplen, c);
                    }
                    str = strcat_char(str, len, tmplen, '>');
                    top--;
                } else {
                    //pass do nothing
                    break;
                }
            }
        } else {
            str = strcat_char(str, len, tmplen, '<');
            str = strcat_str(str, len, tmplen, eltname);
            str = strcat_char(str, len, tmplen, '>');
            k = current_node_index;
            current_node_index = (unsigned long)p[k].data & 0xffff;
            current_node_sub_index = current_node_index + ((unsigned long)p[k].data >> 16);
            top++;
            pile[top].current_node_index = current_node_index;
            pile[top].current_node_sub_index = current_node_sub_index;
            pile[top].eltname = eltname;
        }
    }
}

char* gen_service_desc(int* len, const struct ServiceDesc* s) {
    if (NULL == len || NULL == s) {
        DUER_LOGW("in put len ptr is null or s ptr is null");
        return NULL;
    }
    int i, j;
    const struct Action* acts;
    const struct StateVar* vars;
    const struct Argument* args;
    const char* p;
    char* str;
    int tmplen;
    tmplen = 2048;
    str = (char*)MALLOC(tmplen, UPNP);
    if (str == NULL) {
        DUER_LOGW("malloc fail");
        return NULL;
    }
    /*strcpy(str, xmlver); */
    *len = strlen(xmlver);
    memcpy(str, xmlver, *len + 1);
    acts = s->action_list;
    vars = s->service_state_table;
    str = strcat_char(str, len, &tmplen, '<');
    str = strcat_str(str, len, &tmplen, root_service);
    str = strcat_char(str, len, &tmplen, '>');
    str = strcat_str(str, len, &tmplen,
                     "<specVersion><major>1</major><minor>0</minor></specVersion>");
    i = 0;
    str = strcat_str(str, len, &tmplen, "<actionList>");
    while (acts[i].name) {
        str = strcat_str(str, len, &tmplen, "<action><name>");
        str = strcat_str(str, len, &tmplen, acts[i].name);
        str = strcat_str(str, len, &tmplen, "</name>");
        /* argument List */
        args = acts[i].args;
        if (args) {
            str = strcat_str(str, len, &tmplen, "<argumentList>");
            j = 0;
            while (args[j].dir) {
                str = strcat_str(str, len, &tmplen, "<argument><name>");
                p = vars[args[j].related_var].name;
                str = strcat_str(str, len, &tmplen, (args[j].name ? args[j].name : p));
                str = strcat_str(str, len, &tmplen, "</name><direction>");
                str = strcat_str(str, len, &tmplen, args[j].dir == 1 ? "in" : "out");
                str = strcat_str(str, len, &tmplen,
                                 "</direction><relatedStateVariable>");
                str = strcat_str(str, len, &tmplen, p);
                str = strcat_str(str, len, &tmplen,
                                 "</relatedStateVariable></argument>");
                j++;
            }
            str = strcat_str(str, len, &tmplen, "</argumentList>");
        }
        str = strcat_str(str, len, &tmplen, "</action>");
        /*str = strcat_char(str, len, &tmplen, '\n'); // TEMP ! */
        i++;
    }
    str = strcat_str(str, len, &tmplen, "</actionList><serviceStateTable>");
    i = 0;
    while (vars[i].name) {
        str = strcat_str(str, len, &tmplen,
                         "<stateVariable sendEvents=\"");
        str = strcat_str(str, len, &tmplen, (vars[i].itype & EVENTED) ? "yes" : "no");
        str = strcat_str(str, len, &tmplen, "\"><name>");
        str = strcat_str(str, len, &tmplen, vars[i].name);
        str = strcat_str(str, len, &tmplen, "</name><dataType>");
        str = strcat_str(str, len, &tmplen, upnptypes[vars[i].itype & 0x0f]);
        str = strcat_str(str, len, &tmplen, "</dataType>");
        if (vars[i].iallowedlist) {
            str = strcat_str(str, len, &tmplen, "<allowedValueList>");
            for (j = vars[i].iallowedlist; upnpallowedvalues[j]; j++) {
                str = strcat_str(str, len, &tmplen, "<allowedValue>");
                str = strcat_str(str, len, &tmplen, upnpallowedvalues[j]);
                str = strcat_str(str, len, &tmplen, "</allowedValue>");
            }
            str = strcat_str(str, len, &tmplen, "</allowedValueList>");
        }
        /*if(vars[i].defaultValue) */
        if (vars[i].idefault) {
            str = strcat_str(str, len, &tmplen, "<defaultValue>");
            /*str = strcat_str(str, len, &tmplen, vars[i].defaultValue); */
            str = strcat_str(str, len, &tmplen, upnpdefaultvalues[vars[i].idefault]);
            str = strcat_str(str, len, &tmplen, "</defaultValue>");
        }
        if (vars[i].iallowedvaluerange) {
            str = strcat_str(str, len, &tmplen, "<allowedValueRange>");
            int index = 0;
            for (j = vars[i].iallowedvaluerange; upnpallowedvalueranges[j]; j++, index++) {
                if (index == 0) {
                    str = strcat_str(str, len, &tmplen, "<minimum>");
                    str = strcat_str(str, len, &tmplen, upnpallowedvalueranges[j]);
                    str = strcat_str(str, len, &tmplen, "</minimum>");
                } else if (index == 1) {
                    str = strcat_str(str, len, &tmplen, "<maximum>");
                    str = strcat_str(str, len, &tmplen, upnpallowedvalueranges[j]);
                    str = strcat_str(str, len, &tmplen, "</maximum>");
                } else if (index == 2) {
                    str = strcat_str(str, len, &tmplen, "<step>");
                    str = strcat_str(str, len, &tmplen, upnpallowedvalueranges[j]);
                    str = strcat_str(str, len, &tmplen, "</step>");
                } else {
                    //pass do nothing
                }
            }
            str = strcat_str(str, len, &tmplen, "</allowedValueRange>");
        }
        str = strcat_str(str, len, &tmplen, "</stateVariable>");
        /*str = strcat_char(str, len, &tmplen, '\n'); // TEMP ! */
        i++;
    }
    str = strcat_str(str, len, &tmplen, "</serviceStateTable></scpd>");
    str[*len] = '\0';
    return str;
}

/*external function*/
char* gen_root_desc(int* len) {
    if (NULL == len) {
        DUER_LOGW("input len ptr is null");
        return NULL;
    }
    char* str;
    int tmplen = 2048;
    str = (char*)MALLOC(tmplen, UPNP);
    if (NULL == str) {
        DUER_LOGW("malloc fail");
        return NULL;
    }
    *len = strlen(xmlver);
    memcpy(str, xmlver, *len + 1);
    str = gen_xml(str, len, &tmplen, rootDesc);
    str[*len] = '\0';
    DUER_LOGI("root_desc = %s len = %d\n", str, *len);
    return str;
}

char* gen_connection_manager_scpd(int* len) {
    if (NULL == len) {
        DUER_LOGW("input len ptr is null");
        return NULL;
    }
    return gen_service_desc(len, &scpdConnectionManager);
}

char* gen_av_transport_scpd(int* len) {
    if (NULL == len) {
        DUER_LOGW("input len ptr is null");
        return NULL;
    }
    return gen_service_desc(len, &scpdAVTransport);
}

char* gen_rendering_control_scpd(int* len) {
    if (NULL == len) {
        DUER_LOGW("input len ptr is null");
        return NULL;
    }
    return gen_service_desc(len, &scpdRenderingControl);
}
