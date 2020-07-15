
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ctypes.h"
#include "device_api.h"

#define LOG_FILE_PATH "/sdcard/log.txt"

void debug_log_2_file(char *_str_content)
{
	FILE *log_file = NULL;

	log_file = fopen(LOG_FILE_PATH, "at+");
	if (log_file == NULL)
	{
		printf("fopen %s fail\n", LOG_FILE_PATH);
		goto debug_log_2_file_error;
	}

	if (fwrite(_str_content, strlen(_str_content), 1, log_file) <= 0)
	{
		printf("fwrite %s fail\n", _str_content);
		goto debug_log_2_file_error;
	}

	fflush(log_file);

debug_log_2_file_error:
	if (log_file != NULL)
	{
		fclose(log_file);
		log_file = NULL;
	}

	return;
}

