/*This is tone file*/

const char* tone_uri[] = {
   "flsh:///0_Bt_Reconnect.mp3",
   "flsh:///1_Wechat.mp3",
   "flsh:///2_Wakeup.mp3",
   "flsh:///3_Welcome_To_Wifi.mp3",
   "flsh:///4_New_Version_Available.mp3",
   "flsh:///5_Bt_Success.mp3",
   "flsh:///6_Freetalk.mp3",
   "flsh:///7_Upgrade_Done.mp3",
   "flsh:///8_Alarm.mp3",
   "flsh:///9_Wifi_Success.mp3",
   "flsh:///10_Under_Smartconfig.mp3",
   "flsh:///11_Out_Of_Power.mp3",
   "flsh:///12_hello.mp3",
   "flsh:///13_Please_Retry_Wifi.mp3",
   "flsh:///14_Welcome_To_Bt.mp3",
   "flsh:///15_Wifi_Time_Out.mp3",
   "flsh:///16_Wifi_Reconnect.mp3",
};

int getToneUriMaxIndex()
{
    return sizeof(tone_uri) / sizeof(char*) - 1;
}
                