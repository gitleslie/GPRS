/******************************************************************************
  Copyright (C), 2004, 深圳广宇通信科技有限公司
  文件名:      sms.h

  作者: shumin      版本: 1.0        日期: 2004.06.14

  描述:    AT命令处理 -- 短消息处理AT命令实现
******************************************************************************/
#ifndef  HTTP_H
#define  HTTP_H

//#define HTTP_SERVER         "203.195.189.64"
//#define HTTP_URL               "/DOWN/auth_address.txt"
//#include <limits.h>

#define kCRLFNewLine     "\r\n"
#define kCRLFLineEnding  "\r\n\r\n"

int32 Http_HeadLen( uint8 *httpbuf );

int32 Http_BodyLen( uint8 *httpbuf );

void http_gethost_info(char *src, char **web, char **file, int *port);

//int32 GetDataServerAddr(UINT8* databuff, UINT16 len, UINT8* ip, DWORD *port);

//int32 HttpGetAuthServerAddr(UINT8* databuff, UINT16 len, UINT8* ip, DWORD *port);

int32 Http_GET( const int8 *host,const int8 *url );

void soft_update_use_http(void);

typedef enum
{
    FOTA_STATUS_IDLE,
    FOTA_STATUS_UPDATING,
    FOTA_STATUS_UPDATE_SUCCESS,
    FOTA_STATUS_UPDATE_FAILE
}FOTA_STATUS;

#endif

