#include "user_cmd.h"
#include "main.h"
#include "shell.h"
#include "shell_cfg.h"
#include "string.h"
#include "stdio.h"
#include "stdarg.h"
#include "shell_ext.h"


void nowtimeCmd(int argc, char *argv[])
{
	shellPrint(shellGetCurrent(), "nowtime: %d\r\n",HAL_GetTick());
}
SHELL_EXPORT_CMD(
SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
nowtime, nowtimeCmd, test command);