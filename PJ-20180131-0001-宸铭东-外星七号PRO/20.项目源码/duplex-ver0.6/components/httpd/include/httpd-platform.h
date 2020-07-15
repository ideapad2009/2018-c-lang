#ifndef HTTPD_PLATFORM_H
#define HTTPD_PLATFORM_H
#include <stdint.h>
#include "lwip/sockets.h"

typedef struct RtosConnType{
    int fd;
    int needWriteDoneNotif;
    int needsClose;
    int port;
    char ip[4];
    struct sockaddr_in server_addr;
    struct sockaddr_in remote_addr;
} RtosConnType;

typedef RtosConnType* ConnTypePtr;

#define httpd_printf(fmt, ...) do { \
    printf(fmt, ##__VA_ARGS__); \
    } while(0)


int httpdPlatSendData(ConnTypePtr conn, char *buff, int len);
void httpdPlatDisconnect(ConnTypePtr conn);
void httpdPlatDisableTimeout(ConnTypePtr conn);
void httpdPlatInit(int port, int maxConnCt);
void httpdPlatLock();
void httpdPlatUnlock();
#endif
