#ifndef __ARRAY_MUSIC_H__
#define __ARRAY_MUSIC_H__
 #include <stdio.h>
#include <string.h>

typedef struct
{
    int audio_len;
    char *audio_data;
    char *audio_name;
} FLASH_MUSIC_DATA_T;

typedef enum
{
    FLASH_MUSIC_A_01_PROMPT_THE_CONNECTION_NETWORK,
    FLASH_MUSIC_A_02_PROMPT_NETWORK_CONNECTION_SUCCESS,
    FLASH_MUSIC_A_03_PROMPT_NETWORK_CONNECTION_FAILURE,
    FLASH_MUSIC_A_04_PROMPT_NETWORKING_TIMEOUT,
    FLASH_MUSIC_A_05_PROMPT_FIRST_CONNECT_THE_NETWORK,
    FLASH_MUSIC_A_06_PROMPT_NETWORK_DISCONNECTION,
    FLASH_MUSIC_A_07_SAY_HELLO,
    FLASH_MUSIC_B_01_I_AM_BOBY,
    FLASH_MUSIC_B_02_ADDED_TO_THE_COLLECTION_LIST,
    FLASH_MUSIC_B_03_CAN_NOT_BE_COLLECTED_AT_PRESENT,
    FLASH_MUSIC_B_04_THE_COLLECTION_LIST_IS_EMPTY,
    FLASH_MUSIC_B_05_GET_INTO_SLEEP_15_MINUTES_LATER,
    FLASH_MUSIC_B_06_GET_INTO_SLEEP_30_MINUTES_LATER,
    FLASH_MUSIC_B_07_GET_INTO_SLEEP_45_MINUTES_LATER,
    FLASH_MUSIC_B_08_SLEEP_MODE_HAS_BEEN_CLOSED,
    FLASH_MUSIC_B_09_NO_AUDIO_CAN_BE_PLAYED,
    FLASH_MUSIC_B_10_CHILD_LOCK_HAS_BEEN_OPENED,
    FLASH_MUSIC_B_11_CHILD_LOCK_HAS_BEEN_CLOSED,
    FLASH_MUSIC_B_12_SD_CARD_HAS_BEEN_INSERTED,
    FLASH_MUSIC_B_13_SD_CARD_HAS_BEEN_REMOVED,
    FLASH_MUSIC_B_14_UPGRADE_SUCCESSFUL,
    FLASH_MUSIC_B_15_UPGRADE_FAIL,
    FLASH_MUSIC_B_16_PLEASE_CHARGE_THE_BATTERY,
    FLASH_MUSIC_B_17_ADD_TO_THE_COLLECTION_LIST_FAIL,
    FLASH_MUSIC_B_18_SHUTDOWN_PROMPTING,
    FLASH_MUSIC_B_19_DEVICE_ACTIVATE_FAIL,
    FLASH_MUSIC_B_20_DEVICE_ACTIVATE_OK,
    FLASH_MUSIC_B_21_ERROR_PLEASE_TRY_AGAIN_LATER,
    FLASH_MUSIC_B_22_I_CAN_NOT_DO_THIS,
    FLASH_MUSIC_B_23_NEW_VERSION_AVAILABLE,
    FLASH_MUSIC_B_24_NOT_HEAR_CLEARLY_PLEASE_REPEAT,
    FLASH_MUSIC_B_25_ERROR_PROMPT_TAKE_A_BREAK_AND_GO_ON,
    FLASH_MUSIC_B_26_NETWORK_ERROR,
    FLASH_MUSIC_B_27_PLEASE_CLOSE_THE_CHILD_LOCK_FIRST,
    FLASH_MUSIC_B_28_SHUT_DOWN_CONTINUOUS_CHAT,
    FLASH_MUSIC_B_29_COLLECTION_LIST_HAS_ALREADY_HAD_THIS,
    FLASH_MUSIC_D_BAIKE,
    FLASH_MUSIC_D_BENDI,
    FLASH_MUSIC_D_ERGE,
    FLASH_MUSIC_D_GUOXUE,
    FLASH_MUSIC_D_GUSHI,
    FLASH_MUSIC_D_KETANG,
    FLASH_MUSIC_D_QINZI,
    FLASH_MUSIC_D_XIAZAI,
    FLASH_MUSIC_D_YINGYU,
    FLASH_MUSIC_D_YINYUE,
    FLASH_MUSIC_E_01_KEY_PRESS,
    FLASH_MUSIC_E_02_RECEIVE_MASSAGE_TONE,
    FLASH_MUSIC_E_03_UPGRADING_MUSIC,
    FLASH_MUSIC_E_04_CHAT_SEND_TONE,
    FLASH_MUSIC_CHAT_SHORT_AUDIO,
    FLASH_MUSIC_DYY_NO_MUSIC_PLAY,
    FLASH_MUSIC_DYY_VOL_MAX,
    FLASH_MUSIC_DYY_VOL_MIN,
    FLASH_MUSIC_DYY_VOLUME_ALREADY_HIGHEST,
    FLASH_MUSIC_DYY_VOLUME_ALREADY_LOWEST,
    FLASH_MUSIC_DYY_VOLUME_CHANGE_OK,
    FLASH_MUSIC_DYY_VOLUME_DOWN,
    FLASH_MUSIC_DYY_VOLUME_UP,
    FLASH_MUSIC_MESSAGE_ALERT,
    FLASH_MUSIC_MSG_SEND,
    FLASH_MUSIC_MUSIC_NOT_FOUND,
    FLASH_MUSIC_MUSIC_PAUSE,
    FLASH_MUSIC_MUSIC_PLAY,
    FLASH_MUSIC_NEXT_SONG,
    FLASH_MUSIC_NO_MUSIC,
    FLASH_MUSIC_NO_TF_CARD,
    FLASH_MUSIC_PREV_SONG,
    FLASH_MUSIC_SEND_FAIL,
    FLASH_MUSIC_SEND_SUCCESS,
    FLASH_MUSIC_WAIT_FOR_SD_SCAN,
    FLASH_MUSIC_WECHAT_SHORT_AUDIO,
}FLASH_MUSIC_INDEX_T;


char *get_flash_music_data(int index);

char *get_flash_music_name(int index);

char *get_flash_music_name_ex(int index);

int get_flash_music_size(int index);

int get_flash_music_max_subscript();

#endif /*__ARRAY_MUSIC_H__*/
