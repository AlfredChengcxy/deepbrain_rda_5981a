// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Chang Li (changli@baidu.com)
//
// Description: utilities for miniupnprender.

#include "utilities.h"
#include <stdio.h>
#include <string.h>
#include "lightduer_timestamp.h"
#include "heap_monitor.h"
#include "log.h"

char* strcasestr(const char* src, const char* find) {
    if (NULL == src || NULL == find) {
        return NULL;
    }
    char* cp = (char*)src;
    char* s1 = NULL, *s2 = NULL;
    while (*cp) {
        s1 = cp;
        s2 = (char*)find;
        while (*s2 && *s1 && !(tolower(*s1) - tolower(*s2))) {
            s1++;
            s2++;
        }
        if (!(*s2)) {
            return cp;
        }
        cp++;
    }
    return NULL;
}

char* strcasestrc(const char* s, const char* p, const char t) {
    char* endptr = NULL;
    if(s == NULL || p == NULL){
        return NULL;
    }
    size_t slen = 0, plen = 0;
    endptr = strchr(s, t);
    if (!endptr) {
        return strcasestr(s, p);
    }
    plen = strlen(p);
    slen = endptr - s;
    while (slen >= plen) {
        if (*s == *p && strncasecmp(s + 1, p + 1, plen - 1) == 0) {
            return (char*)s;
        }
        s++;
        slen--;
    }
    return NULL;
}

unsigned int internal_time() {
    unsigned int ms = duer_timestamp();
    return ms / 1000;
}

unsigned int gettimeofday(struct timeval* tm, void* timezone) {
    unsigned int ms = duer_timestamp();

    tm->tv_sec = ms / 1000;
    tm->tv_usec = (ms % 1000) * 1000;

    return ms;
}

char* strcat_char(char* str, int* len, int* tmplen, char c){
    if(NULL == str || NULL == len || NULL == tmplen){
        DUER_LOGW("str or len or tmplen ptr is null");
        return NULL;
    }
    char* p = NULL;
    if(*tmplen <= (*len + 1)){
        *tmplen += 256;
        p = (char*)REALLOC(str, *tmplen, UPNP);
        if(!p){
            *tmplen -= 256;
            DUER_LOGW("realloc fail");
            return str;
        }else{
            str = p;
        }
    }
    str[*len] = c;
    (*len)++;
    return str;
}

char* strcat_str(char* str, int* len, int* tmplen, const char *s2){
    if(NULL == str || NULL == len || NULL == tmplen || NULL == s2){
        DUER_LOGW("str or len or tmplen or s2 ptr is null");
        return str;
    }
    char* p = NULL;
    int s2len = strlen(s2);
    if(*tmplen <= (*len + s2len)){
        if(s2len < 256){
            *tmplen += 256;
        }else{
            *tmplen += s2len +1;
        }
        p = (char*)REALLOC(str, *tmplen, UPNP);
        if(!p){
            if(s2len < 256){
                *tmplen -= 256;
            }else{
                *tmplen -= s2len + 1;
            }
            DUER_LOGW("realloc fail");
            return str;
        }else{
            str = p;
        }

    }

    memcpy(str + *len, s2, s2len + 1);
    *len += s2len;
    return str;
}

