#ifndef __MPUSH_MESSAGE_H__
#define __MPUSH_MESSAGE_H__

#include <stdio.h>
#include "ctypes.h"
#include "mbedtls/pk.h"

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

#define BUFFER_SIZE_8 		8
#define BUFFER_SIZE_16 		16
#define BUFFER_SIZE_32 		32
#define BUFFER_SIZE_64 		64
#define BUFFER_SIZE_128 	128
#define BUFFER_SIZE_1024 	1024

#define MPUSH_MAX_HEART_BEAT 30
#define MPUSH_MIN_HEART_BEAT 10

#define MPUSH_HEARTBEAT_BYTE -33
#define MPUSH_HEARTBEAT_BYTE_REPLY 223

typedef enum 
{
    MPUSH_MSG_CMD_HEARTBEAT 	= 1,
    MPUSH_MSG_CMD_HANDSHAKE 	= 2,
    MPUSH_MSG_CMD_LOGIN 		= 3,
    MPUSH_MSG_CMD_LOGOUT 		= 4,
    MPUSH_MSG_CMD_BIND 			= 5,
    MPUSH_MSG_CMD_UNBIND 		= 6,
    MPUSH_MSG_CMD_FAST_CONNECT 	= 7,
    MPUSH_MSG_CMD_PAUSE 		= 8,
    MPUSH_MSG_CMD_RESUME 		= 9,
    MPUSH_MSG_CMD_ERROR 		= 10,
    MPUSH_MSG_CMD_OK 			= 11,
    MPUSH_MSG_CMD_HTTP_PROXY	= 12,
    MPUSH_MSG_CMD_KICK			= 13,
    MPUSH_MSG_CMD_GATEWAY_KICK	= 14,
    MPUSH_MSG_CMD_PUSH			= 15,
    MPUSH_MSG_CMD_GATEWAY_PUSH	= 16,
    MPUSH_MSG_CMD_NOTIFICATION	= 17,
    MPUSH_MSG_CMD_GATEWAY_NOTIFICATION = 18,
    MPUSH_MSG_CMD_CHAT			= 19,
    MPUSH_MSG_CMD_GATEWAY_CHAT	= 20,
    MPUSH_MSG_CMD_GROUP			= 21,
    MPUSH_MSG_CMD_GATEWAY_GROUP	= 22,
    MPUSH_MSG_CMD_ACK			= 23,
    MPUSH_MSG_CMD_UNKNOWN		= -1,
}MPUSH_MSG_CMD_T;

typedef enum
{
	MPUSH_MSG_FLAG_CRYPTO 	= 0x01,//packet包启用加密
   	MPUSH_MSG_FLAG_COMPRESS = 0x02,//packet包启用压缩
   	MPUSH_MSG_FLAG_BIZ_ACK 	= 0x04,
   	MPUSH_MSG_FLAG_AUTO_ACK = 0x08,
}MPUSH_MSG_FLAG_T;

typedef enum
{
	MPUSH_SEND_MSG_TYPE_NONE = 0,
	MPUSH_SEND_MSG_TYPE_TEXT,
	MPUSH_SEND_MSG_TYPE_FILE,
	MPUSH_SEND_MSG_TYPE_LINK,
}MPUSH_SEND_MSG_TYPE_T;


#pragma pack(1)

typedef struct
{
	uint8_t str_device_id[BUFFER_SIZE_32];
	uint8_t str_os_name[BUFFER_SIZE_8];
	uint8_t str_os_version[BUFFER_SIZE_8];
	uint8_t str_client_version[BUFFER_SIZE_8];
}MPUSH_MSG_CONFIG_T;

typedef struct	//total lenght 13 bytes
{
	uint32_t	length;//body length
	uint8_t 	cmd; //命令
    uint16_t 	cc; //校验码 暂时没有用到
    uint8_t 	flags; //特性，如是否加密，是否压缩等
    uint32_t 	sessionId; // 会话id
    uint8_t 	lrc; // 校验，纵向冗余校验。只校验header
}MPUSH_MSG_HEADER_T;

typedef struct
{
	uint16_t 	str_device_id_len;
	uint8_t 	str_device_id[BUFFER_SIZE_32];
	uint16_t 	str_os_name_len;
	uint8_t 	str_os_name[BUFFER_SIZE_8];
	uint16_t 	str_os_version_len;
	uint8_t 	str_os_version[BUFFER_SIZE_8];
	uint16_t 	str_client_version_len;
	uint8_t 	str_client_version[BUFFER_SIZE_8];
	uint16_t 	str_iv_len;
	uint8_t 	str_iv[BUFFER_SIZE_16];
	uint16_t 	str_client_key_len;
	uint8_t 	str_client_key[BUFFER_SIZE_16];
	uint32_t 	minHeartbeat;
	uint32_t 	maxHeartbeat;
	uint64_t 	timestamp;
}MPUSH_MSG_HANDSHAKE_T;

typedef struct
{
	uint8_t str_user_id[BUFFER_SIZE_256];
	uint8_t str_alias[BUFFER_SIZE_32];
	uint8_t str_tags[BUFFER_SIZE_32];
}MPUSH_MSG_BIND_T;

typedef struct
{
	int msg_type;
	char msg_content[256];
}MPUSH_CLIENT_MSG_INFO_T;

#pragma pack(pop)

void mpush_make_bind_user_msg(
	char *_out_buf,
	size_t *_out_len,
	MPUSH_MSG_CONFIG_T *_config,
	mbedtls_rsa_context *_rsa);
void mpush_make_handshake_msg(
	char *_out_buf,
	size_t *_out_len,
	MPUSH_MSG_CONFIG_T *_config,
	mbedtls_rsa_context *_rsa);
void print_msg_head(MPUSH_MSG_HEADER_T *_head);
void convert_msg_head_to_net(MPUSH_MSG_HEADER_T *_head);
void convert_msg_head_to_host(MPUSH_MSG_HEADER_T *_head);
int mpush_msg_decode(
	MPUSH_MSG_HEADER_T *_head, char*_body,
	uint8_t *_out, size_t _out_len);
void mpush_make_heartbeat_msg(char *_out_buf, size_t *_out_len);
void mpush_make_ack_msg(char *_out_buf, size_t *_out_len, uint32_t _session_id);


#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif /* __MPUSH_MESSAGE_H__ */

