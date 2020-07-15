
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char* http_get_body(const char *str_body)
{
	char* str_seprate = "\r\n\r\n";
	char* pBody = strstr(str_body, str_seprate);
	if (NULL == pBody)
	{
		return NULL;
	}
	pBody = pBody + strlen(str_seprate);
	if (*pBody == '\0')
	{
		return NULL;
	}

	return pBody;
}

//http error code
int http_get_error_code(const char *str_body)
{
	if (str_body == NULL)
	{
		return -1;
	}
	
	if (strlen(str_body) > 12 
		&& (str_body + 9) == strstr(str_body, "200"))
	{
		return 200;
	}

	return -1;
}

int http_get_content_length(const char *str_body)
{
	char length[16] = {0};
	
	if (str_body == NULL)
	{
		return -1;
	}

	char *start = strstr(str_body, "Content-Length:");
	if (start == NULL)
	{
		return -1;	
	}
	
	char *end = strstr(start, "\r\n");
	if (end == NULL)
	{
		return -1;
	}

	start = start + strlen("Content-Length:");
	int len = end - start;
	if (len > 0 && len < sizeof(length) - 1)
	{
		int i = 0;
		int n  =0;
		for (i=0; i<len; i++)
		{
			char ch = *(start+i);
			if (ch >= '0' && ch <= '9')
			{
				length[n] = ch;
				n++;
			}
		}

		return atoi(length);
	}
	else
	{
		return -1;
	}
}

