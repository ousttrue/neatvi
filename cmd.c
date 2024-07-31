#include "platform.h"
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "vi.h"





int cmd_exec(char *cmd)
{
	cmd_pipe(cmd, NULL, 0);
	return 0;
}
