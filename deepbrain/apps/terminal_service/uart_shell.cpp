/*
 * Copyright 2017-2018 deepbrain.ai, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://deepbrain.ai/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <stdio.h>
#include <string.h>
#include "mbed.h"
#include <app_config.h>
#include "uart_shell.h"
#include "memory_interface.h"
#include <task_thread_interface.h>
#include <debug_log_interface.h>
#include "rda_wdt_api.h"
#include "device_params_manage.h"
#include "wifi_manage.h"
//#include "wechat_amr_data.h"
#include "dcl_mpush_push_msg.h"
#include "events.h"

#define LOG_TAG "uart_shell"

Serial g_serial(UART0_TX, UART0_RX);

static ShellCommand *scp = NULL;

static char *parse_arguments(char *str, char **saveptr) {
    char *p;

    if (str != NULL)
        *saveptr = str;

    p = *saveptr;
    if (!p) {
        return NULL;
    }

    /* Skipping white space.*/
    p += strspn(p, " \t");

    if (*p == '"') {
        /* If an argument starts with a double quote then its delimiter is another
           quote.*/
        p++;
        *saveptr = strpbrk(p, "\"");
    }
    else {
        /* The delimiter is white space.*/
        *saveptr = strpbrk(p, " \t");
    }

    /* Replacing the delimiter with a zero.*/
    if (*saveptr != NULL) {
        *(*saveptr)++ = '\0';
    }

    return *p != '\0' ? p : NULL;
}

static void list_commands(const ShellCommand *scp) {
    ShellCommand *_scp = (ShellCommand *)scp;

	if (_scp == NULL)
	{
		return;
	}
	
    while (_scp->sc_name != NULL) {
        g_serial.printf("%s \r\n", _scp->sc_name);
        _scp++;
    }
}

static bool cmdexec(char *name, int argc, char *argv[]) {
    ShellCommand *_scp = scp;

	if (_scp == NULL)
	{
		return false;
	}
	
    while (_scp->sc_name != NULL) {
        if (strcmp(_scp->sc_name, name) == 0) {
            _scp->sc_function(argc, argv);
            return false;
        }
        _scp++;
    }
    return true;
}

static bool shellGetLine(char *line, unsigned size) {
    char *p = line;
    char c;
    char tx[3];
	
    while (true) {
		c = g_serial.getc();

        if ((c == 8) || (c == 127)) {  // backspace or del
            if (p != line) {
                tx[0] = c;
                tx[1] = 0x20;
                tx[2] = c;
                g_serial.putc(tx[0]);
				g_serial.putc(tx[1]);
				g_serial.putc(tx[2]);

                p--;
            }
            continue;
        }
        if (c == '\n' || c == '\r') {
            tx[0] = c;
            g_serial.putc(tx[0]);
            *p = 0;
            return false;
        }

        if (c < 0x20)
            continue;
        if (p < line + size - 1) {
            tx[0] = c;
            g_serial.putc(tx[0]);
            *p++ = (char)c;
        }
    }
}

static void shell_task(void)
{
    int n;

    char *lp, *cmd, *tokp, line[SHELL_MAX_LINE_LENGTH];
    char *args[SHELL_MAX_ARGUMENTS + 1];

    while (true) {
        g_serial.printf(">\r\n");

        if (shellGetLine(line, sizeof(line))) {

        }

        lp = parse_arguments(line, &tokp);
        cmd = lp;
        n = 0;
        while ((lp = parse_arguments(NULL, &tokp)) != NULL) {
            if (n >= SHELL_MAX_ARGUMENTS) {
                g_serial.printf("too many arguments"SHELL_NEWLINE_STR);
                cmd = NULL;
                break;
            }
            args[n++] = lp;
        }
        args[n] = NULL;
        if (cmd != NULL) {
            if (cmdexec(cmd, n, args)) {
                g_serial.printf("%s", cmd);
                g_serial.printf("?"SHELL_NEWLINE_STR);
                list_commands(scp);
            }
        }

    }
}

static void sys_reboot(int argc, char *argv[])
{
	rda_sys_softreset();
}

static void sys_config(int argc, char *argv[])
{
	print_device_params();
}

static void sys_config_reset(int argc, char *argv[])
{
	init_default_params();
}

static void mem_info(int argc, char *argv[])
{
	memory_info();
}

static void wifi_airkiss(int argc, char *argv[])
{
	DEBUG_LOGI(LOG_TAG, "wifi_airkiss start");
	wifi_manage_start_airkiss();
}

static void key_magic_voice(int argc, char *argv[])
{
	duer::event_trigger(duer::EVT_KEY_MAGIC_VOICE_PRESS);
}

static void key_mchat(int argc, char *argv[])
{
	duer::event_trigger(duer::EVT_KEY_MCHAT_PRESS);
}

static void key_wifi(int argc, char *argv[])
{
	duer::event_trigger(duer::EVT_RESET_WIFI);
}

static void key_talk(int argc, char *argv[])
{
	duer::event_trigger(duer::EVT_KEY_REC_PRESS);
}

const ShellCommand g_shell_command[] = {
    //system
    {"------system-------", NULL},
    {"reboot", sys_reboot},
    {"config", sys_config},
    {"config_reset", sys_config_reset},
	{"------memory-------", NULL},
    {"mem", mem_info},
	{"------wifi-------", NULL},
    {"airkiss", wifi_airkiss},
    {"------keyboard-------", NULL},
    {"magic-voice", key_magic_voice},
    {"mchat", key_mchat},
    {"talk", key_talk},
    {"wifi", key_wifi},
    {NULL, NULL}
};

bool uart_shell_create(int32_t task_priority)
{
    scp = (ShellCommand *)&g_shell_command;

	g_serial.baud(921600);
    g_serial.format(8, SerialBase::None, 1);

	if (task_thread_create(shell_task, 
			"shell_task", 
			APP_NAME_SHELL_STACK_SIZE, 
			NULL, 
			task_priority))
	{
		DEBUG_LOGI(LOG_TAG, "uart_shell_create success");
		return true;
	}
	else
	{
		DEBUG_LOGE(LOG_TAG, "task_thread_create shell_task failed");
		return false;
	}
}

void uart_shell_delete(void)
{
	
}

