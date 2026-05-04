/**
 * @file shell_port.h
 * @author Letter (NevermindZZT@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2019-02-22
 * 
 * @copyright (c) 2019 Letter
 * 
 */

#ifndef __SHELL_PORT_H__
#define __SHELL_PORT_H__

#include "shell.h"
#define debugSerial huart3

extern Shell shell;

#define Debug 1

#if Debug
#define debugShellPrintf(shell, fmt, ...) shellPrint(shell, fmt, ##__VA_ARGS__)

#else

#define debugShellPrintf(Shell *shell, const char *fmt, ...)


#endif



void userShellInit(void);
#endif
