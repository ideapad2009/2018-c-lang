#if abc
#define STORAGE_SIZE (FRAME_LEN * 4)

void recordTask(void *pv)
{
    char readPtr[STORAGE_SIZE] = {0};
    char buffer3[STORAGE_SIZE/2] = {0};

    //printf("free heap: %d\r\n", xPortGetFreeHeapSizeCaps(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    void *pChannel;

    int i, ret=0, count = 0;
	//ϵͳ������ֻ���ʼ��һ�Σ���ֻ��һ��
    YQSW_CreateChannel(&pChannel,"none");
    YQSW_ResetChannel(pChannel);
	
    while(1){
        i2s_read_bytes(0, readPtr, STORAGE_SIZE, portMAX_DELAY);

		//ֻȡ˫�����е�һ����������
		for (i = 0; i < STORAGE_SIZE / 2; i += 2) {
			*((short*)(buffer3 + i)) = *((short*)(readPtr + i * 2));
		}
	
		//��ȡƽ̨¼�����ݺ�¼������������Ϊ 16k ������16bit����������
    	//��ÿһ֡��¼�������͵��������洦�����������Ԥ���廽�ѻ�����ʣ��򷵻�1
		ret = YQSW_ProcessWakeup(pChannel, (int16_t*)(buffer3 + count), FRAME_LEN);
		if (ret == 1){
			int id = YQSW_GetKeywordId(pChannel);
			//�ɹ����ֺ���������
			YQSW_ResetChannel(pChannel);
			printf("### spot keyword, id = %d\n", id);                   
		}
	
    }
	
    YQSW_DestroyChannel(pChannel);	

    vTaskDelete(NULL);
}

nvs_flash_init();
xTaskCreate(&recordTask, "recordTask", 1024*8, NULL, 5, NULL);
#endif