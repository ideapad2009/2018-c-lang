#
# Component Makefile
#
# This Makefile should, at the very least, just include $(SDK_PATH)/make/component.mk. By default,
# this will take the sources in this directory, compile them and link them into
# lib(subdirectory_name).a in the build directory. This behaviour is entirely configurable,
# please read the SDK documents if you need to do this.
#
COMPONENT_ADD_INCLUDEDIRS := . ./power_manage ./ota_service ./player_middleware ./flash_music ./flash_config_manage ./keyboard_manage ./led_ctrl ./nlp_service ./wechat_service ./bind_device ./free_talk ./auto_play ./sd_music_manage ./flash_music_manage ./geling_edu ./amrwb_encode ./memo_service ./user_experience_test_data_manage ./asr_service ./translate_manage ./keyword_wakeup ./keyboard_event

COMPONENT_SRCDIRS :=  . ./power_manage ./ota_service ./player_middleware ./flash_music ./flash_config_manage ./keyboard_manage ./led_ctrl ./nlp_service ./wechat_service ./bind_device ./free_talk ./auto_play ./sd_music_manage ./flash_music_manage ./geling_edu ./amrwb_encode ./memo_service ./user_experience_test_data_manage ./asr_service ./translate_manage ./keyword_wakeup ./keyboard_event

