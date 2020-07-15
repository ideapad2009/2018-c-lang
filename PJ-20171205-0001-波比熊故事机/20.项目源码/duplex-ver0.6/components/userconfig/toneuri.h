#ifndef __TONEURI_H__
#define __TONEURI_H__

extern const char* tone_uri[];

typedef enum {
    TONE_TYPE_STORY_01,
    TONE_TYPE_STORY_02,
    TONE_TYPE_STORY_03,
    TONE_TYPE_STORY_04,
    TONE_TYPE_STORY_05,
    TONE_TYPE_STORY_06,
    TONE_TYPE_STORY_07,
    TONE_TYPE_STORY_08,
    TONE_TYPE_STORY_09,
    TONE_TYPE_STORY_10,
    TONE_TYPE_STORY_11,
    TONE_TYPE_STORY_12,
    TONE_TYPE_STORY_13,
    TONE_TYPE_STORY_14,
    TONE_TYPE_STORY_15,
    TONE_TYPE_STORY_16,
    TONE_TYPE_STORY_17,
    TONE_TYPE_STORY_18,
    TONE_TYPE_STORY_19,
} ToneType;

int getToneUriMaxIndex();
char *getToneUriByIndex(int index);

#endif /* __TONEURI_H__ */