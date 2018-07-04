#include <stdio.h>
#include <stdlib.h>
#include <lite-log.h>
#include "sys_callback.h"
#include "sys_services.h"
#include "sys_ext.h"
#include "string.h"
#include "cmd.h"
#include "app.h"
#include "debug.h"
#include "cmddef.h"
#include "manage.h"
#include "http.h"

UINT32 UDFileLen = 0;
UINT8 FotaStatus = 0;
extern FOTA_PARAM_T Fota_param;

extern unsigned char Plaintext[256];

extern int socketb_recv_callback(unsigned long event,char *data,unsigned long len,unsigned long buf_len);



/**
 * @brief http_gethost_info
 *
 * @Param: src  url (in)  http://dev.ccn520.com/examples/1.zip
 * @Param: web  WEB (out)  dev.ccn520.com
 * @Param: file  download filename (out)  examples/1.zip
 * @Param: port  default 80 (out)
 *
 * host_addr is: dev.ccn520.com
 * host_file is: examples/1.zip
 * port is: 80
 */
void http_gethost_info(char *src, char **web, char **file, int *port)
{
    char *pa;
    char *pb;
    int isHttps = 0;

    if (!src || strlen(src) == 0) {
        log_err("http_gethost_info parms error!\n");
        return;
    }
    *port = 0;
    if (!(*src)) {
        return;
    }
    pa = src;
    if (!strncmp(pa, "https://", strlen("https://"))) {
        pa = src + strlen("https://");
        isHttps = 1;
    }
    if (!isHttps) {
        if (!strncmp(pa, "http://", strlen("http://"))) {
            pa = src + strlen("http://");
        }
    }
    *web = pa;
    pb = strchr(pa, '/');
    if (pb) {
        *pb = 0;
        pb += 1;
        if (*pb) {
            *file = pb;
            *((*file) + strlen(pb)) = 0;
        }
    } else {
        (*web)[strlen(pa)] = 0;
    }

    pa = strchr(*web, ':');
    if (pa) {
        *pa = 0;
        *port = atoi(pa + 1);
    } else {
        if (isHttps) {
            *port = 80;//443
        } else {
            *port = 80;
        }

    }
}

int32 Http_HeadLen( uint8 *httpbuf )
{
    int8 *p_start = NULL;
    int8 *p_end =NULL;
    int32 headlen=0;
    p_start = (char *)httpbuf;
    p_end = strstr( (char *)httpbuf,kCRLFLineEnding);
    if( p_end==NULL )
    {
        DB(DB1,"Can't not find the http head!");
        return 0;
    }
    p_end=p_end+strlen(kCRLFLineEnding);
    headlen = (p_end-p_start);
    return headlen;
}
/*********************************************************************
*
*   FUNCTION       :   TO get the http bodylen
*      httpbuf     :   http receive buf
*      return      :   the http bodylen.(0-error)

*   Add by Alex lin  --2014-12-02
*
**********************************************************************/
int32 Http_BodyLen( uint8 *httpbuf )
{
    int8 *p_start = NULL;
    int8 *p_end =NULL;
    int8 bodyLenbuf[10]={0};
    int32 bodylen=0;  //Content-Length:
    p_start = strstr( (char *)httpbuf,"Content-Length: ");
    if( p_start==NULL ) return 0;
    p_start = p_start+strlen("Content-Length: ");
    p_end = strstr( p_start,kCRLFNewLine);
    if( p_end==NULL )   return 0;

    memcpy( bodyLenbuf,p_start,(p_end-p_start));
    bodylen = atoi(bodyLenbuf);
    return bodylen;
}

int32 Http_GET( const int8 *host,const int8 *url )
{
    int8 getBuf[400] = {0};
    int32 totalLen=0;
    int32 ret=0;

    memset( getBuf,0,400 );

    snprintf( getBuf,400,"%s /%s %s%s%s %s%s%s%s%s%s%s%s",
              "GET",url,"HTTP/1.1",kCRLFNewLine,
              "Host:",host,kCRLFNewLine,
              "User-Agent: Fibo-Module",kCRLFNewLine,
              "Accept: text/html,*/*,",kCRLFNewLine,
              "Connect"
                      "ion: Close",kCRLFLineEnding);
    totalLen =strlen( getBuf );

    ret = sys_sock_send(gRunParam.tcp_sock,getBuf,totalLen);

    DB(DB1,"Sent provision:\n %s\n", getBuf);

    if(ret<=0 )
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

int32 UD_Http_GET( const int8 *host,const int8 *url )
{
    int8 getBuf[400] = {0};
    int32 totalLen=0;
    int32 ret=0;

    memset( getBuf,0,400 );

    snprintf( getBuf,400,"%s /%s %s%s%s %s%s%s%s%s%s%s%s",
              "GET",url,"HTTP/1.1",kCRLFNewLine,
              "Host:",host,kCRLFNewLine,
              "User-Agent: Fibo-Module",kCRLFNewLine,
              "Accept: text/html,*/*,",kCRLFNewLine,
              "Connection: Close",kCRLFLineEnding);
    totalLen =strlen( getBuf );

    log_info(" ~~~ will send %d, \n %s", totalLen, getBuf);
    ret = sys_sock_send(gRunParam.update_sock,getBuf,totalLen);

    DB(DB1,"http ud: %s\n", getBuf);

    if(ret<=0 )
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

void setUDSocketStatus(TCP_STATUS_T Status)
{
    gRunParam.UpdateConnStatus = Status;
}

extern UINT8 UDConnectFaileCnt;
void soft_update_use_http(void)
{
    UINT32 ip;
    INT32 ret;
    static int ActFailCnt = 0;
    static int ConnectFailCnt = 0;
    UINT8 *p;
    GAPP_TCPIP_ADDR_T tcpip_addr;
    static int n;
    static UINT32 second = 0;
    char temp_ip[32] = {0};
    DWORD temp_port = 0;

    second++;
    //if((second%5) == 0)
        log_info("tcp_connect_process status=%d",gRunParam.UpdateConnStatus);


    switch(gRunParam.UpdateConnStatus)
    {
        case TCP_IDLE:
            if((gRunParam.UpdateStartLink)&&(gRunParam.GprsStatus == GPRS_ACTIVED))    //激活状态下才进行连接
            {

                memcpy(temp_ip, gRunParam.UDServer, 32);
                temp_port = gRunParam.UDPort;
                log_info("Begin sys_getHostByName");
                ret = sys_getHostByName(temp_ip,&tcpip_addr.sin_addr); //("218.108.52.230",&tcpip_addr.sin_addr);//通过域名获取IP
                log_info("End sys_getHostByName");
                p = (UINT8*)&tcpip_addr.sin_addr;

                log_info("ud ret %d %d.%d.%d.%d",ret,p[0],p[1],p[2],p[3]);
                log_info("ud port %d",temp_port);

                if(GAPP_RET_OK != ret)
                {
                    ConnectFailCnt++;
                    if(ConnectFailCnt >= 5)
                    {
                        log_err("ConnectFailCnt >= 5， will reset!");
                        sys_taskSend(TASK_ONE,T1_SOFT_RESET,0,0,0);
                        //sys_softReset();   //连续5次连接失败复位模块
                        return;
                    }
                    return;
                }

                gRunParam.update_sock = sys_sock_create(GAPP_IPPROTO_TCP);
                tcpip_addr.sin_port = htons(temp_port);//htons(50040);//
                ret = sys_sock_connect(gRunParam.update_sock,&tcpip_addr);

                log_info("))))) gRunParam.update_sock = %d, sys_sock_connect ret=%d", gRunParam.update_sock, ret);
                if(GAPP_RET_OK > ret)
                {
                    ConnectFailCnt++;
                    if(ConnectFailCnt >= 5)
                    {
                        log_err("ConnectFailCnt >= 5， will reset1!");
                        sys_taskSend(TASK_ONE,T1_SOFT_RESET,0,0,0);
                        //sys_softReset();
                        return;
                    }
                    return;
                }
                gRunParam.UpdateConnStatus = TCP_CONNECTTING;
            }
            break;
        case TCP_CONNECTTING:     //等待连接成功的消息GAPP_SIG_SOCK_CONNECT_RSP
            if(n++ < 180)
                return;
            else
            {
                ConnectFailCnt++;
                if(ConnectFailCnt >= 5)
                {
                    ConnectFailCnt = 0;
                    log_err("ConnectFailCnt >= 5， will reset2!");
                    sys_taskSend(TASK_ONE,T1_SOFT_RESET,0,0,0);
                    //sys_softReset();
                    return;
                }
                gRunParam.UpdateConnStatus = TCP_SOCKET_CLOSE;
                n = 0;
                //20170106
                log_err("ConnectFailCnt >= 5， will reset3!");
                sys_taskSend(TASK_ONE,T1_SOFT_RESET,0,0,0);
                //sys_softReset();
            }
            break;
        case TCP_CONNECT:
            n = 0;
            ConnectFailCnt = 0;
            UDConnectFaileCnt = 0;
            log_info(" TCP_CONNECT, call UD_Http_Get");
            ret = UD_Http_GET(gRunParam.UDServer,gRunParam.UDURL);//发送数据
            if(ret < 0)
            {
                gRunParam.UpdateConnStatus = TCP_SOCKET_CLOSE;
            }
            else if(0 == ret)
            {
                gRunParam.UpdateConnStatus = TCP_WAIT_ACK;
            }
            else if(1 == ret) //不需要再发送数据
            {
                //gRunParam.TcpStatus = TCP_SOCKET_CLOSE;
                //gRunParam.TcpStartLink = 0;
            }
            break;
        case TCP_WAIT_ACK:    //等待发送成功的消息GAPP_SIG_SOCK_SEND_RSP
            if(n++ >= 120)
            {
                log_info("TCP_WAIT_ACK n>10");
                n = 0;

                gRunParam.UpdateConnStatus = TCP_SOCKET_CLOSE;
                //数据接收超时，reset
                sys_taskSend(TASK_ONE,T1_SOFT_RESET,0,0,0);
                //Fota_param.FotaStatus = FOTA_STATUS_UPDATE_FAILE;
                //todo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Fota_param.FotaStatus = FOTA_STATUS_IDLE;
                //todo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~WriteFotaStatusToFlash();
                
            }
            break;
        case TCP_SOCKET_CLOSE:
            gRunParam.UpdateStartLink = 0;
            if(gRunParam.update_sock != -1)
            {
                //20170106
                //sys_sock_close(gRunParam.tcp_sock);
                sys_sock_close(gRunParam.update_sock);
                gRunParam.UpdateConnStatus = TCP_SOCKET_CLOSING;
            }
            else
            {
                gRunParam.UpdateConnStatus = TCP_SOCKET_CLOSED;
            }
            break;
        case TCP_SOCKET_CLOSING:
            if(n++ < 180)
                return;
            else
            {
                gRunParam.UpdateConnStatus = TCP_SOCKET_CLOSED;
                n = 0;
            }
            break;
        case TCP_SOCKET_CLOSED:
            gRunParam.UpdateStartLink = 0;
            n = 0;
            //SendInfoToMcu(SERVER_DISCONNECT);
            if((0 == gRunParam.TcpStartLink)&&(0 == gRunParam.LBSStartLink)&&(0 == gRunParam.UpdateStartLink))  //如果没有需要连接的需要，则去激活PDP
            {

                gRunParam.GprsStatus = GPRS_DEACTIVE;
            }
            else
            {
                gRunParam.GprsStatus = GPRS_ACTIVED;
            }
            gRunParam.UpdateConnStatus = TCP_IDLE;
            break;
        default:
            gRunParam.UpdateConnStatus = TCP_IDLE;
            break;
    }
}

INT32 ParseUDHttpData(UINT8 *RecvBuf, INT32 len)
{
    INT32 WroteLength;
    static INT32 WroteLengthAll;
    static UINT8 HeadNotRcvAll = 0;
    UINT8* p1;
    UINT8* p2;

    UINT8 tmp[256]={0};
    strncpy(tmp, RecvBuf, 256);
    log_debug("ud receive %d",len);
    p1 = RecvBuf;

    if(0 == UDFileLen)
    {
        WroteLengthAll = 0;
        UDFileLen = Http_BodyLen(RecvBuf); //要下载的文件总长度
        log_info("UDFileLen=%d",UDFileLen);
        if(UDFileLen > 0)
        {
            p2 = strstr(RecvBuf, "\r\n\r\n");
            if(NULL == p2)
            {
                log_info("HeadNotRcvAll = 1");
                HeadNotRcvAll = 1;
                return 0;
            }
            else
            {
                HeadNotRcvAll = 0;
                RecvBuf = p2+4;
                len = len - (p2+4-p1);
                log_info( "remain len=%d",len);
            }
        }
        else
        {
            return 0;
        }
    }
    else if(1 == HeadNotRcvAll)
    {
        p2 = strstr(RecvBuf, "\r\n\r\n");
        if(NULL == p2)
        {
            HeadNotRcvAll = 1;
            return 0;
        }
        else
        {
            HeadNotRcvAll = 0;
            RecvBuf = p2+4;
            len = len - (p2+4-p1);
            log_info( "remain2 len=%d",len);
        }
    }


    //wqrz~~~~~~ 打开存储本地的升级文件Leslie
    if(gRunParam.UDFile < 0)
    {
        UINT32 opt = FS_O_RDWR | FS_O_CREAT | FS_O_TRUNC;
        gRunParam.UDFile = sys_file_open(UD_FILE_NAME, opt);
        log_info("sys_file_open return %d", gRunParam.UDFile);
    }


    if(gRunParam.UDFile >= 0)
    {
        log_info( "ud file write");
        WroteLength = sys_file_write(gRunParam.UDFile, RecvBuf, len);
        log_info("len=%d,wrotelen=%d",len,WroteLength);
        if(WroteLength != len)
            return -1;

        WroteLengthAll += WroteLength;
        log_info( "ud file write OK %d", WroteLengthAll);
        if(WroteLengthAll >= UDFileLen)
        {
            while(0 != sys_file_flush(gRunParam.UDFile, 10000))
            {
                log_info( "wait for flush all data...");
            }
            return 1;
        }
    }

    return 0;
}

