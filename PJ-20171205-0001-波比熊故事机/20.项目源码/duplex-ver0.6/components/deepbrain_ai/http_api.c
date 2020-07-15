
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

char* http_get_chunked_body(
	char *str_dest,
	int  dest_len,
	char *str_src)
{
	int i = 0;
	char str_ch[2] = {0};
	char str_len[16] = {0};
	char str_body[256] = {0};
	int state = 0;

	bzero(str_ch, sizeof(str_ch));
	bzero(str_len, sizeof(str_len));
	bzero(str_dest, dest_len);
	for (i=0; i<strlen(str_src); i++)
	{
		char ch = str_src[i];
		str_ch[0] = ch;
		switch(state)
		{
			case 0:
			{
				if (ch == '\r')
				{
					state = 1;
				}
				else
				{
					if (ch == '0')
					{
						state = 4;
					}
					else
					{
						strcat(str_len, str_ch);
					}
				}
				break;
			}
			case 1:
			{
				if (ch == '\n')
				{
					state = 2;
				}
				else
				{
					state = 0;
					bzero(str_len, sizeof(str_len));
				}
				break;
			}
			case 2:
			{
				if (ch == '\r')
				{
					state = 3;
				}
				else
				{
					strcat(str_dest, str_ch);
				}
				break;
			}
			case 3:
			{
				state = 0;
				bzero(str_len, sizeof(str_len));
				break;
			}
			case 4:
			{
				break;
			}
			default:
				break;
		}
	}

	return str_dest;
}

