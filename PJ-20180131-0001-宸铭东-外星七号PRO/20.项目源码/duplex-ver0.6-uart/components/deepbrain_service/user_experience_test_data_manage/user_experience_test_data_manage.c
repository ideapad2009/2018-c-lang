#include <stdio.h>
#include "user_experience_test_data_manage.h"

USER_EXPERIENCE_TEST_DATE_T g_need_print_date = {1, 0};

//获取语音识别所需的时间和语音识别返回的结果
void get_asr_cost_time_and_result(int asr_time, char *asr_result)
{
	g_need_print_date.asr_time = asr_time;
	snprintf(g_need_print_date.asr_result, sizeof(g_need_print_date.asr_result), "%s", asr_result);
}

/*//获取语义处理所需的时间和语义处理返回的结果
void get_nlp_cost_time_and_result(int nlp_time, char *nlp_result)
{
	g_need_print_date.nlp_time = nlp_time;
	snprintf(g_need_print_date.nlp_result, sizeof(g_need_print_date.nlp_result), "%s", nlp_result);
}
*/

//获取语义处理所需的时间
void get_nlp_cost_time(int nlp_time)
{
	g_need_print_date.nlp_time = nlp_time;
}

//获取语义处理返回的结果
void get_nlp_result(char *nlp_result)
{
	snprintf(g_need_print_date.nlp_result, sizeof(g_need_print_date.nlp_result), "%s", nlp_result);
}

//从语音识别到语义处理 流程完全正常情况下的打印，即正常数据
int print_date()
{
	printf("\n\n\n");
	for(int i = 0; i < 20; i++)
	{
		printf("*");
	}

	printf("\n\n\n");
	printf("**请求编号：第【%d】次\n\n**语音识别时间：用了【%d】毫秒\n\n**语音识别结果：【%s】\n\n**语义返回时间：用了【%d】毫秒\n\n**语义返回结果：【%s】\n\n\n",
		g_need_print_date.count++, 
		g_need_print_date.asr_time, 
		g_need_print_date.asr_result, 
		g_need_print_date.nlp_time, 
		g_need_print_date.nlp_result);

	for(int i = 0; i < 20; i++)
	{
		printf("*");
	}
	printf("\n\n\n");
	return 0;
}

//语音识别不出情况下的打印
int asr_date_err_print()
{
	printf("\n\n\n");
	for(int i = 0; i < 20; i++)
	{
		printf("*");
	}

	printf("\n\n\n");
	printf("**请求编号：第【%d】次\n\n**语音识别时间：用了【%d】毫秒\n\n**语音识别结果：【语音识别失败了！】\n\n**语义返回时间：【语音识别失败了！】\n\n**语义返回结果：【语音识别失败了！】\n\n\n",
		g_need_print_date.count++, 
		g_need_print_date.asr_time);

	for(int i = 0; i < 20; i++)
	{
		printf("*");
	}
	printf("\n\n\n");
	return 0;
}

//语义处理失败情况下的打印
int nlp_date_err_print()
{
	printf("\n\n\n");
	for(int i = 0; i < 20; i++)
	{
		printf("*");
	}

	printf("\n\n\n");
	printf("**请求编号：第【%d】次\n\n**语音识别时间：用了【%d】毫秒\n\n**语音识别结果：【%s】\n\n**语义返回时间：用了【%d】毫秒\n\n**语义返回结果：【语义处理失败了！】\n\n\n",
		g_need_print_date.count++, 
		g_need_print_date.asr_time, 
		g_need_print_date.asr_result, 
		g_need_print_date.nlp_time);

	for(int i = 0; i < 20; i++)
	{
		printf("*");
	}
	printf("\n\n\n");
	return 0;
}

//语义返回结果为空时的打印
int nlp_date_null_print()
{
	printf("\n\n\n");
	for(int i = 0; i < 20; i++)
	{
		printf("*");
	}

	printf("\n\n\n");
	printf("**请求编号：第【%d】次\n\n**语音识别时间：用了【%d】毫秒\n\n**语音识别结果：【%s】\n\n**语义返回时间：用了【%d】毫秒\n\n**语义返回结果：【语义返回无任何数据！】\n\n\n",
		g_need_print_date.count++, 
		g_need_print_date.asr_time, 
		g_need_print_date.asr_result, 
		g_need_print_date.nlp_time);
	
	for(int i = 0; i < 20; i++)
	{
		printf("*");
	}
	printf("\n\n\n");
	return 0;
}

//无法进行语义处理时的打印
int nlp_process_err_print()
{
	printf("\n\n\n");
	for(int i = 0; i < 20; i++)
	{
		printf("*");
	}

	printf("\n\n\n");
	printf("**请求编号：第【%d】次\n\n**语音识别时间：用了【%d】毫秒\n\n**语音识别结果：【%s】\n\n**语义返回时间：【无法进行语义处理！】\n\n**语义返回结果：【无法进行语义处理！】\n\n\n",
		g_need_print_date.count++, 
		g_need_print_date.asr_time, 
		g_need_print_date.asr_result);

	for(int i = 0; i < 20; i++)
	{
		printf("*");
	}
	printf("\n\n\n");
	return 0;
}