typedef struct 
{
    int audio_len;
    char *audio_data;
}FLASH_MUSIC_DATA_T;

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
    FLASH_MUSIC_DYY_NO_MUSIC_PLAY,
    FLASH_MUSIC_MUSIC_PAUSE,
    FLASH_MUSIC_MUSIC_PLAY,
    FLASH_MUSIC_NO_TF_CARD,
    FLASH_MUSIC_WAIT_FOR_SD_SCAN,
}FLASH_MUSIC_INDEX_T;

extern const char flash_music_a_01_prompt_the_connection_network[];
extern const char flash_music_a_02_prompt_network_connection_success[];
extern const char flash_music_a_03_prompt_network_connection_failure[];
extern const char flash_music_a_04_prompt_networking_timeout[];
extern const char flash_music_a_05_prompt_first_connect_the_network[];
extern const char flash_music_a_06_prompt_network_disconnection[];
extern const char flash_music_a_07_say_hello[];
extern const char flash_music_b_01_i_am_boby[];
extern const char flash_music_b_02_added_to_the_collection_list[];
extern const char flash_music_b_03_can_not_be_collected_at_present[];
extern const char flash_music_b_04_the_collection_list_is_empty[];
extern const char flash_music_b_05_get_into_sleep_15_minutes_later[];
extern const char flash_music_b_06_get_into_sleep_30_minutes_later[];
extern const char flash_music_b_07_get_into_sleep_45_minutes_later[];
extern const char flash_music_b_08_sleep_mode_has_been_closed[];
extern const char flash_music_b_09_no_audio_can_be_played[];
extern const char flash_music_b_10_child_lock_has_been_opened[];
extern const char flash_music_b_11_child_lock_has_been_closed[];
extern const char flash_music_b_12_sd_card_has_been_inserted[];
extern const char flash_music_b_13_sd_card_has_been_removed[];
extern const char flash_music_b_14_upgrade_successful[];
extern const char flash_music_b_15_upgrade_fail[];
extern const char flash_music_b_16_please_charge_the_battery[];
extern const char flash_music_b_17_add_to_the_collection_list_fail[];
extern const char flash_music_b_18_shutdown_prompting[];
extern const char flash_music_b_19_device_activate_fail[];
extern const char flash_music_b_20_device_activate_ok[];
extern const char flash_music_b_21_error_please_try_again_later[];
extern const char flash_music_b_22_i_can_not_do_this[];
extern const char flash_music_b_23_new_version_available[];
extern const char flash_music_b_24_not_hear_clearly_please_repeat[];
extern const char flash_music_b_25_error_prompt_take_a_break_and_go_on[];
extern const char flash_music_b_26_network_error[];
extern const char flash_music_b_27_please_close_the_child_lock_first[];
extern const char flash_music_b_28_shut_down_continuous_chat[];
extern const char flash_music_b_29_collection_list_has_already_had_this[];
extern const char flash_music_d_baike[];
extern const char flash_music_d_bendi[];
extern const char flash_music_d_erge[];
extern const char flash_music_d_guoxue[];
extern const char flash_music_d_gushi[];
extern const char flash_music_d_ketang[];
extern const char flash_music_d_qinzi[];
extern const char flash_music_d_xiazai[];
extern const char flash_music_d_yingyu[];
extern const char flash_music_d_yinyue[];
extern const char flash_music_e_01_key_press[];
extern const char flash_music_e_02_receive_massage_tone[];
extern const char flash_music_e_03_upgrading_music[];
extern const char flash_music_dyy_no_music_play[];
extern const char flash_music_music_pause[];
extern const char flash_music_music_play[];
extern const char flash_music_no_tf_card[];
extern const char flash_music_wait_for_sd_scan[];

char *get_flash_music_data(FLASH_MUSIC_INDEX_T index);

int get_flash_music_size(FLASH_MUSIC_INDEX_T index);
