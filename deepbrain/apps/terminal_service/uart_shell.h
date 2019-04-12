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

#ifndef UART_SHELL_H
#define UART_SHELL_H

typedef void (*shellcmd_t)(int argc, char *argv[]);

/**
 * @brief   Custom command entry type.
 */
typedef struct {
    const char      *sc_name;           /**< @brief Command name.       */
    shellcmd_t      sc_function;        /**< @brief Command function.   */
} ShellCommand;


#define _shell_clr_line(stream)   chprintf(stream, "\033[K")

#define SHELL_NEWLINE_STR   "\r\n"
#define SHELL_MAX_ARGUMENTS 10
#define SHELL_PROMPT_STR "\r\n>"
#define SHELL_MAX_LINE_LENGTH 512

bool uart_shell_create(int32_t task_priority);
void uart_shell_delete(void);

#endif
