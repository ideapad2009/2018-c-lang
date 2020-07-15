/*This is tone file*/

const char* tone_uri[] = {
   "flsh:///0_C.story_01.mp3",
   "flsh:///1_C.story_02.mp3",
   "flsh:///2_C.story_03.mp3",
   "flsh:///3_C.story_04.mp3",
   "flsh:///4_C.story_05.mp3",
   "flsh:///5_C.story_06.mp3",
   "flsh:///6_C.story_07.mp3",
   "flsh:///7_C.story_08.mp3",
   "flsh:///8_C.story_09.mp3",
   "flsh:///9_C.story_10.mp3",
   "flsh:///10_C.story_11.mp3",
   "flsh:///11_C.story_12.mp3",
   "flsh:///12_C.story_13.mp3",
   "flsh:///13_C.story_14.mp3",
   "flsh:///14_C.story_15.mp3",
   "flsh:///15_C.story_16.mp3",
   "flsh:///16_C.story_17.mp3",
   "flsh:///17_C.story_18.mp3",
   "flsh:///18_C.story_19.mp3",
};

int getToneUriMaxIndex()
{
    return sizeof(tone_uri) / sizeof(char*) - 1;
}

char *getToneUriByIndex(int index)
{
	return tone_uri[index];
}