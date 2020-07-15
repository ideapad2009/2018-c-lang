#include "functional_running_state_check.h"

FUNCTIONAL_TYIGGER_KEY_E g_key_state = FUNCTIONAL_TYIGGER_KEY_INITIAL_STATE;
WIFI_STATE_E g_wifi_state = WIFI_INITIAL_STATE;

//按键功能状态 set&get
void set_functional_tyigger_key_state(FUNCTIONAL_TYIGGER_KEY_E key_state)
{
	g_key_state = key_state;
}

FUNCTIONAL_TYIGGER_KEY_E get_functional_tyigger_key_state()
{
	FUNCTIONAL_TYIGGER_KEY_E key_state = g_key_state;
	return key_state;
}

//网络状态 set&get
void set_wifi_state(WIFI_STATE_E wifi_state)
{
	g_wifi_state = wifi_state;
}

WIFI_STATE_E get_wifi_state()
{
	WIFI_STATE_E wifi_state = g_wifi_state;
	return wifi_state;
}

