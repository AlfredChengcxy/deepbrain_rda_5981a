// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Li Chang (changli@baidu.com)
//
// Description: Media callback for DLNA

#include "baidu_dlna_media_cb.h"
#include <stdio.h>
#include "baidu_media_manager.h"
#include "baidu_media_play_listener.h"
#include "lightduer_log.h"
#include "upnpsoap.h"
#include "upnpevents.h"

class DLNAListener : public duer::MediaPlayerListener {
public:
    virtual int on_start(int flags);
    virtual int on_stop(int flags);
    virtual int on_finish(int flags);
};

int DLNAListener::on_start(int flags) {
    DUER_LOGI("### DLNAListener::on_start, flag=%d.", flags);
    //set_transport_state(PLAYING);
    return 0;
}

int DLNAListener::on_stop(int flags) {
    DUER_LOGI("### DLNAListener::on_stop, flag=%d.", flags);
    if (get_transport_state() == PLAYING) {
        set_transport_state(PAUSED_PLAYBACK);
        upnp_event_var_change_notify(EAVTransport);
    }
    return 0;
}

int DLNAListener::on_finish(int flags) {
    DUER_LOGI("### DLNAListener::on_finish, flag=%d.", flags);
    //set_transport_state(STOPPED);
    //upnp_event_var_change_notify(EAVTransport);
    return 0;
}

static DLNAListener s_listener;

void register_dlna_listener() {
    duer::MediaManager::instance().register_listener(&s_listener);
}

void unregister_dlna_listener() {
    duer::MediaManager::instance().unregister_listener(&s_listener);
}

