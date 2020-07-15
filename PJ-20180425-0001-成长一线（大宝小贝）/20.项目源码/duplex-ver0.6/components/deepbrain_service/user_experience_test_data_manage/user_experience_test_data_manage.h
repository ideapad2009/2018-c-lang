#ifndef __USER_EXPERIENCE_TEST_DATE_MANAGE__H
#define __USER_EXPERIENCE_TEST_DATE_MANAGE__H

typedef struct
{
	int count;					//请求次数
	int asr_time;				//语音识别所用时间
	int nlp_time;				//语义处理所用时间
	char asr_result[512];		//语音识别返回结果
	char nlp_result[2048];		//语义处理返回结果
}USER_EXPERIENCE_TEST_DATE_T;

//获取语音识别所需的时间和语音识别返回的结果
void get_asr_cost_time_and_result(int asr_time, char *asr_result);

//获取语义处理所需的时间和语义处理返回的结果
//void get_nlp_cost_time_and_result(int nlp_time, char *nlp_result);

//获取语义处理所需的时间
void get_nlp_cost_time(int nlp_time);

//获取语义处理返回的结果
void get_nlp_result(char *nlp_result);

//从语音识别到语义处理 流程完全正常情况下的打印，即正常数据
int print_date();

//语音识别不出情况下的打印
int asr_date_err_print();

//语义处理失败情况下的打印
int nlp_date_err_print();

//语义返回结果为空时的打印
int nlp_date_null_print();

//无法进行语义处理时的打印
int nlp_process_err_print();

#endif