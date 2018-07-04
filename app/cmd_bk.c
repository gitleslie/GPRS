#include "cmd.h"
#include "stdio.h"
#include "sys_services.h"
#include "string.h"
#include "ctype.h"
#include "debug.h"
#include "app.h"
#include "sys_ext.h"
#include "mqttcloud.h"
#include "MQTTPacket.h"
#include "lite-log.h"
#include "TCP.h"
#include "hardware.h"
#include "http.h"

#define CMD_MAX_LENGTH 1024

typedef enum
{
	CMD_STATE_START,
	CMD_STATE_RECV_A,
	CMD_STATE_RECV_T,
	CMD_STATE_RECV_EXT
}CMD_STATE_T;


INT32 cmd_list(const INT8 *str);
INT32 cmd_sendmsg(const INT8 *str);
void  call_cmd(const INT8 *buff,UINT16 len);
INT32 cmd_memalloc(const INT8 *str);
INT32 cmd_fileopen(const INT8 *str);
INT32 cmd_fileread(const INT8 *str);
INT32 cmd_gb2UI(const INT8 *str);
INT32 cmd_filewrite(const INT8 *str);
INT32 cmd_gpiocfg(const INT8 *str);
INT32 cmd_gpioset(const INT8 *str);
INT32 cmd_gpioget(const INT8 *str);
INT32 cmd_lpgctrl(const INT8 *str);
INT32 cmd_fileseek(const INT8 *str);
INT32 cmd_fileclose(const INT8 *str);
INT32 cmd_pinirq(const INT8 *str);
INT32 cmd_ver(const INT8 *str);
INT32 cmd_pdp(const INT8 *str);
INT32 cmd_tcp(const INT8 *str);
INT32 cmd_tcpsend(const INT8 *str);
INT32 cmd_udp(const INT8 *str);
INT32 cmd_udpsend(const INT8 *str);
INT32 cmd_at(const INT8 *str);
INT32 cmd_fileclear(const INT8 *str);
INT32 cmd_aliconn(const INT8 *str);
INT32 cmd_ali_sub(const INT8 *str);
INT32 cmd_ali_pub(const INT8 *str);
INT32 cmd_ota(const INT8 *str);
INT32 cmd_csq(const INT8 *str);
INT32 cmd_imei(const INT8 *str);
INT32 cmd_imsi(const INT8 *str);
INT32 cmd_iccid(const INT8 *str);
INT32 cmd_start(const INT8 *str);
INT32 cmd_stop(const INT8 *str);
INT32 cmd_ioset(const INT8 *str);
INT32 cmd_openlog(const INT8 *str);
INT32 cmd_closelog(const INT8 *str);
void cmd_hdw_test(void);
INT32 cmdcmp(INT32 i,const UINT8 *b,UINT16 b_len);
extern INT32	mqttSocketId;


CMD_T cmd_table[] = 
{
	{"AT",cmd_list},
	{"+SENDMSG",cmd_sendmsg},
	{"+MALLOC",cmd_memalloc},
	{"+FILEOPEN",cmd_fileopen},
	{"+FILEREAD",cmd_fileread},
	{"+FILEWRITE",cmd_filewrite},
	{"+FILESEEK",cmd_fileseek},
	{"+FILECLOSE",cmd_fileclose},	
	{"+GB2UI",cmd_gb2UI},
    {"+gpiocfg",cmd_gpiocfg},
    {"+gpioset",cmd_gpioset},
    {"+gpioget",cmd_gpioget},
    {"+lpgctrl",cmd_lpgctrl},
	{"+PINIRQ",cmd_pinirq},
	{"+pdp",cmd_pdp},
	{"+tcp",cmd_tcp},
    {"+tcpsend",cmd_tcpsend},
    {"+udp",cmd_udp},
    {"+udpsend",cmd_udpsend},
	{"+VER",cmd_ver}, 
	{"+at",cmd_at},
	{"+fileclear",cmd_fileclear},
	{"+aliconn",cmd_aliconn},
	{"+alisub",cmd_ali_sub},
	{"+alipub",cmd_ali_pub},
    {"+ota",cmd_ota},
    {"+csq",cmd_csq},
    {"+imei",cmd_imei},
    {"+imsi",cmd_imsi},
    {"+iccid",cmd_iccid},
    {"+ioset",cmd_ioset},
    {"+start",cmd_start},
    {"+stop",cmd_stop},
    {"+openlog",cmd_openlog},
    {"+closelog",cmd_closelog},




//	{"AT+TEST",cmd_hdw_test}
};

/*udp*/
INT32 cmd_udp_sock = -1;

INT32 cmd_udp(const INT8 *str)
{
    GAPP_TCPIP_ADDR_T tcpip_addr;
    INT32  ret,op;
    UINT16 port;

    ret = sget_response(str,"dd",&op,&port);

    DB(DB3,"!!");
    
    if(2 == ret && 1 == op)
    {
        if(0 <= cmd_udp_sock)
        {
            return -1;
        }

        cmd_udp_sock = sys_sock_create(GAPP_IPPROTO_UDP);

        if(GAPP_RET_OK > ret)
        {
            return ret;
        }

        tcpip_addr.sin_addr.addr = 0;
        tcpip_addr.sin_port = htons(port);

        ret = sys_sock_bind(cmd_udp_sock,&tcpip_addr);

        return ret;
    }
    else if(1 == ret && 0 == op)
    {
        ret = sys_sock_close(cmd_udp_sock);
        DB(DB3,"sock close %d",ret);
    }
    else
    {
        return -1;
    }
    
    return 0;    
}

INT32 cmd_udpsend(const INT8 *str)
{
    INT32 ret;
    UINT16 port;
    GAPP_TCPIP_ADDR_T tcpip_addr;
    UINT8 remote[100] = {0};
    UINT8 data[1024] = {0};
    
    ret = sget_response(str,"100sd1024s",remote,&port,data);

    DB(DB3,"%s %d %s",remote,port,data);

    if(3 == ret && 0 <= cmd_udp_sock)
    {
        ret = sys_getHostByName(remote,&tcpip_addr.sin_addr);
        if(GAPP_RET_OK != ret)
        {
            return -1;
        }

        tcpip_addr.sin_port = htons(port);
        ret = sys_sock_send2(cmd_udp_sock,data,strlen(data),&tcpip_addr);
        DB(DB3,"send %d",ret);

        return 0;
    }

    return -1;
}


/*tcp client*/
INT32 cmd_tcp_sock = -1;

INT32 cmd_tcp(const INT8 *str)
{
    UINT8 *p;
    GAPP_TCPIP_ADDR_T tcpip_addr;
    INT32  ret,op;
    UINT16 port;
    INT8   string[100] = {0};

    ret = sget_response(str,"d100sd",&op,string,&port);

    DB(DB3,"!!");
    
    if(3 == ret && 1 == op)
    {
        if(0 <= cmd_tcp_sock)
        {
            return -1;
        }
        
        ret = sys_getHostByName(string,&tcpip_addr.sin_addr);
        p = (UINT8*)&tcpip_addr.sin_addr;

        DB(DB2,"ret %d %d.%d.%d.%d",ret,p[0],p[1],p[2],p[3]);

        if(GAPP_RET_OK != ret)
        {
            return -1;
        }

        cmd_tcp_sock = sys_sock_create(GAPP_IPPROTO_TCP);

        if(GAPP_RET_OK > ret)
        {
            return ret;
        }

        tcpip_addr.sin_port = htons(port);
        ret = sys_sock_connect(cmd_tcp_sock,&tcpip_addr);

        return ret;
    }
    else if(1 == ret && 0 == op)
    {
        ret = sys_sock_close(cmd_tcp_sock);
        ret = sys_sock_close(mqttSocketId);
        DB(DB3,"sock close %d",ret);
    }
    else
    {
        return -1;
    }
    
    return 0;
}

INT32 cmd_tcpsend(const INT8 *str)
{
    INT32 ret;
    UINT8 data[1024] = {0};

    ret = sget_response(str,"1024s",data);

    DB(DB3,"%d %s",ret,data);

    if(1 == ret && 0 <= cmd_tcp_sock)
    {
        ret = sys_sock_send(cmd_tcp_sock,data,strlen(data));
        DB(DB3,"send %d",ret);

        return 0;
    }

    return -1;
}

INT32 cmd_pdp(const INT8 *str)
{
    UINT8 *p;
    UINT32 ip;
    INT32 ret;
    INT32 op;
    INT8 string[256] = {0};
    
    ret = sget_response(str,"d256s",&op,string);

    DB(DB2,"ret %d %d %s",ret,op,string);

    if(2 == ret && 1 == op)
    {
        ip = 0;
        ret = sys_PDPActive(string,NULL,NULL,&ip);
        p = (UINT8 *)&ip;
        
        DB(DB2,"ret %d %d.%d.%d.%d",ret,p[0],p[1],p[2],p[3]);

        return ret;
    }
    else if(1 == ret && 0 == op)
    {
        ret = sys_PDPRelease();
        DB(DB2,"ret %d",ret);
        return ret;
    }

    return -1;
}

void irq0(void)
{
    DB(DB2,"irq !!",0);
}

void irq1(void)
{
    DB(DB2,"irq !!",0);
}

INT32 cmd_lpgctrl(const INT8 *str)
{
    INT32 ret;
    INT32 op = 0;

    ret = sget_response(str,"d",&op);

    if(1 == ret)
    {
        GAPP_OPT_LPG_CONTROL_T o;
        if(op)
        {
            o.op = TRUE;
        }
        else
        {
            o.op = FALSE;
        }

        ret = sys_set(GAPP_OPT_LPG_CONTROL_ID,&o,sizeof(o));

        DB(DB2,"ret %d",ret);

        if(!ret)
        {
            return 0;
        }
    }

    return -1;  
}

INT32 cmd_ver(const INT8 *str)
{
    INT32 ret;
    GAPP_OPT_SYS_VERSION_T ver;

    ret = sys_get(GAPP_OPT_SYS_VERSION_ID,&ver,sizeof(ver));

    //DB(DB2,"ret %d [%s] [0x%x]",ret,ver.sys_ver,ver.api_ver);
    sys_debug(DB2, "+ok=[%s] [%s]\n", ver.sys_ver, VERSION);

    return ret;
}

INT32 cmd_at(const INT8 *str)
{
    UINT16 len;
    INT32 ret;
    UINT8 data[1024] = {0};

    ret = sget_response(str,"1023s",data);

    if(1 == ret)
    {
        len = strlen(data);
        data[len++] = '\r';
        ret = sys_at_send(data,len);

        if(ret == len)
        {
            return 0;
        }

        return -1;
    }

    return -1;
}

INT32 cmd_pinirq(const INT8 *str)
{
    INT32 ret;
    UINT8 pin;
    UINT16 len;
    GAPP_OPT_PIN_CFG_T cfg;

    cfg.debounce = TRUE;
    cfg.falling  = FALSE;
    cfg.rising   = TRUE;
    cfg.level    = FALSE;
    len = sizeof(GAPP_OPT_PIN_CFG_T);
    
    ret = sget_response(str,"d",&pin);

    if(1 != ret)
    {
        return -1;
    }

    ret = -1;
    
    if(pin)
    {
        cfg.cb = irq0;
        ret = sys_set(GAPP_OPT_PIN41_IRQ_ID,&cfg,len);
    }
    else
    {
        cfg.cb = irq1;
        ret = sys_set(GAPP_OPT_PIN27_IRQ_ID,&cfg,len);
    }

    DB(DB2,"sys set ret %d",ret);

    return ret;
}

INT32 cmd_fileclose(const INT8 *str)
{
    INT32 ret;
    INT32 fd;
    
    
    ret = sget_response(str,"d",&fd);

    if(1 == ret)
    {
        ret = sys_file_close(fd);
        DB(DB2,"%d ret %d ",fd,ret); 

        if(!ret)
        {
            return 0;
        }
    }

    return -1;
}

INT32 cmd_fileclear(const INT8 *str)
{
    INT32 ret;

    ret = sys_file_clear();

    DB(DB2,"%d",ret);

    return 0;
}

extern int lunch_f;
extern MQTT_OP_T mqtt_op;
extern MQTT_OP_PARAM_T mqtt_op_param;
MQTT_KEY_PARAM_T mqtt_key_param={{0},{0},{0}};

INT32 cmd_aliconn(const INT8 *str)
{
	INT32 ret;

	/*
	ret = sget_response(str,"64s64s128s",mqtt_dev_param.productkey,mqtt_dev_param.devname,mqtt_dev_param.devsecret);//return the parameter_count 
	if(ret !=3 ){
		log_err("Device Params error!!check it!");
		return ret;
		}
	*/

    mqtt_op = MQTT_OP_CONNECT;
	return 0;
}
/*

void cmd_mqtt_key_param_init()
{
#if defined(CMD)
	strcpy(mqtt_key_param.productkey, PRODUCT_KEY);
	strcpy(mqtt_key_param.devname, DEVICE_NAME);
	strcpy(mqtt_key_param.devsecret, DEVICE_SECRET);
#else	//log_err("cmd_aliconnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn\n");
	strcpy(mqtt_key_param.productkey, hdw_dev_init_param[0]);
	strcpy(mqtt_key_param.devname, hdw_dev_init_param[1]);
	strcpy(mqtt_key_param.devsecret, hdw_dev_init_param[2]);
#endif
		mqtt_op = MQTT_OP_CONNECT;
}
*/


INT32 cmd_ali_sub(const INT8 *str)
{
	INT32 ret;
    INT8 str_subtopic[128]={0};

	ret = sget_response(str,"128s",str_subtopic);//return the parameter_count 
	if(ret !=1 ){
		log_err("Sub Params error!!check it!");
		return ret;
		}

    mqtt_op = MQTT_OP_SUB;
	sprintf(mqtt_op_param.topic_sub, "%s", str_subtopic);
	return 0;
}

INT32 cmd_ali_pub(const INT8 *str)
{
    INT32 ret;
    INT8 str_pubtopic[128]={0};
    INT8 str_msg[256] = {0};
    
    ret = sget_response(str,"128s256s",str_pubtopic,str_msg);//return the parameter_count 
	if(ret !=2 ){
		log_err("Pub Params error!!check it!");
		return ret;
		}
    mqtt_op = MQTT_OP_PUB;
	sprintf(mqtt_op_param.topic_pub,"%s",str_pubtopic);
	sprintf(mqtt_op_param.msg_pub,"%s",str_msg);
	//DB(DB2,"str_topic:%s , str_msg:%s ",mqtt_op_param.topic_pub,mqtt_op_param.msg_pub);
	return 0;
	
}

INT32 cmd_ota(const INT8 *str)
{
    INT32 ret;
    char url[256] = "http://47.95.10.254/examples/1.txt";

    char *host_file = NULL;
    char *host_addr = NULL;
    int port = 0;

    http_gethost_info(url, &host_addr, &host_file, &port);
    log_info("host_addr is: %s\n ", host_addr);
    log_info("host_file is: %s\n ", host_file);
    log_info("port is: %d\n ", port);
    if (host_file == NULL || host_addr == NULL) {
        log_info("host_file==null");
        return -1;
    }


    //strcpy(gRunParam.UDServer, "47.95.10.254");
    //gRunParam.UDPort = 80;
    //strcpy(gRunParam.UDURL, "examples/1.zip");


    strcpy(gRunParam.UDServer, host_addr);
    gRunParam.UDPort = port;
    strcpy(gRunParam.UDURL, host_file);

    setUDSocketStatus(TCP_IDLE);
    gRunParam.UpdateStartLink = 1;
    //soft_update_use_http();
    return 0;
}

INT32 cmd_csq(const INT8 *str)
{
    INT32 ret;

    int csq = AT_GetSignal();
    //log_info("%d",csq);
    sys_debug(DB2, "+ok=%d\n", csq);
    return 0;
}


INT32 cmd_imei(const INT8 *str)
{
    char IMEI[16];
    memset(IMEI, 0, 16);

    int ret = AT_GetSerialNumber(IMEI);
    if (0 != ret) {
        sys_debug(DB2, "+ok=query error\n");
        return RET_FAILED;
    } else {
        //log_info("%s", IMEI);
        sys_debug(DB2, "+ok=%s\n", IMEI);
    }
    return RET_SUCCESS;
}

INT32 cmd_imsi(const INT8 *str)
{
    char IMSI[16];
    memset(IMSI, 0, 16);
    int ret = AT_GetIMSI(IMSI);
    if (0 != ret) {
        sys_debug(DB2, "+ok=query error\n", ret);
        //return RET_FAILED;
    } else {
        //log_info("%s", IMSI);
        sys_debug(DB2, "+ok=%s\n", IMSI);
    }
    return RET_SUCCESS;
}

INT32 cmd_iccid(const INT8 *str)
{
    char ICCID[21];
    memset(ICCID, 0, 21);
    int ret = AT_GetICCID(ICCID);
    if (0 != ret) {
        sys_debug(DB2, "+ok=query error\n", ret);
        //return RET_FAILED;
    } else {
        //log_info("%s", IMSI);
        sys_debug(DB2, "+ok=%s\n", ICCID);
    }
    return RET_SUCCESS;
}

INT32 cmd_start(const INT8 *str)
{
    INT32 ret;
    INT32 time;
    ret = sget_response(str,"d",&time);

    if(1 != ret) {
        time = 5 ;
    }

    //hdw_startDevice(time);
    HDW_MOTOR_ON;
    GPIO_RESET_5;
    //log_info("22 pin set, 20 pin reset");
    sys_debug(DB2, "PASS\n");

    return 0;
}

INT32 cmd_stop(const INT8 *str)
{
    //GPIO_CONFIG_0;
    //GPIO_CONFIG_5;
    GPIO_RESET_0;
    GPIO_SET_5;
    log_info("22 pin reset, 20 pin set");

    return 0;
}
INT32 cmd_ioset(const INT8 *str)
{
    int ret1, ret2, ret3, ret4;
    //HDW_MOTOR_ON;
    ret1 = sys_gpio_set(GAPP_IO_0,1);
    //GPIO_RESET_5;
    ret2 = sys_gpio_set(GAPP_IO_5,0);
    //log_info("22 pin set, 20 pin reset");

    //GPIO_RESET_0;
    ret3 = sys_gpio_set(GAPP_IO_0,0);
    //GPIO_SET_5;
    ret4 = sys_gpio_set(GAPP_IO_5,1);

    //log_info("22 pin reset, 20 pin set");
    if( (ret1==0) && (ret2==0) && (ret3==0) && (ret4==0) )
        sys_debug(DB2, "+ok=PASS\n");
    else
        sys_debug(DB2, "+ok=FAIL\n");

    return RET_SUCCESS;
}

INT32 cmd_closelog(const INT8 *str)
{
    IOT_SetLogLevel(IOT_LOG_EMERG);
    return 0;
    //IOT_CloseLog();
}

INT32 cmd_openlog(const INT8 *str)
{
    INT32 ret;
    INT32 level;
    ret = sget_response(str,"d",&level);

    if(1 != ret) {
        level = IOT_LOG_INFO ;
    }

    IOT_SetLogLevel(level);
    return 0;
    //IOT_OpenLog("mqtt");
}

INT32 cmd_gpiocfg(const INT8 *str)
{
    INT32 ret;
    INT32 id,cfg;

    ret = sget_response(str,"dd",&id,&cfg);

    if(2 == ret)
    {
        ret = sys_gpio_cfg(id,cfg);
        DB(DB2,"sys gpio cfg %d : id %d cfg %d",ret,id,cfg);
        if(!ret)
        {
            return 0;
        }
    }

    return -1;
}

INT32 cmd_gpioset(const INT8 *str)
{
    INT32 ret;
    INT32 id,level;

    level = 0;
    id = 100;
    
    ret = sget_response(str,"dd",&id,&level);

    if(2 == ret)
    {
        ret = sys_gpio_set(id,level);
        DB(DB2,"sys gpio set %d : id %d cfg %d",ret,id,level);
        if(!ret)
        {
            return 0;
        }
    }    

    return -1;
}

INT32 cmd_gpioget(const INT8 *str)
{
    INT32 ret;
    INT32 id;
    UINT8 level;

    id = 0;
    
    ret = sget_response(str,"d",&id);

    if(1 == ret)
    {
        ret = sys_gpio_get(id,&level);
        DB(DB2,"sys gpio get %d : id %d cfg %d",ret,id,level);
        if(!ret)
        {
            return 0;
        }
    }        

    return -1;
}


INT32 cmd_gb2UI(const INT8 *str)
{
    INT32 ret;
    UINT32 gb,ui;

    gb = 0;
    ui = 0;
    
    ret = sget_response(str,"4s",&gb);

    if(1 == ret)
    {
        ret = sys_GB2UNI((UINT16)gb,(UINT16*)&ui);

        DB(DB2,"%d : %x to %x",ret,gb,ui);
        if(0 == ret)
        {
            return 0;
        }
    }

    return -1;
}



INT32 cmd_fileread(const INT8 *str)
{
    INT32 fd,ret;
    UINT8 buff[130];
    ret = sget_response(str,"d",&fd);

    if(1 == ret)
    {
        ret = sys_file_read(fd,buff,128);
        if(0 < ret)
        {
            buff[ret] = '\0';
            DB(DB2,"%s",buff);
            return 0;
        }
    }

    return -1;
}

INT32 cmd_fileseek(const INT8 *str)
{
    INT32 fd,ret,offset,from;
    UINT8 opt;

    offset = 0;
    from = 0;
    
    ret = sget_response(str,"ddd",&fd,&offset,&from);

    if(3 == ret)
    {
        switch(from)
        {
            case 0:
            {
                opt = FS_SEEK_SET;
            }
            break;
            case 1:
            {
                opt = FS_SEEK_CUR;
            }
            break;
            case 2:
            {
                opt = FS_SEEK_END;
            }
            break;
            default:
            return -1;
        }
        
        ret = sys_file_seek(fd,offset,opt);
        DB(DB2,"seek %d from %d offset %d",ret,opt,from);

        if(0 <= ret)
        {
            return 0;
        }
    }

    return -1;
}


INT32 cmd_filewrite(const INT8 *str)
{
    INT32 fd,ret,len;
    UINT8 buff[1025] = {0};
    
    ret = sget_response(str,"ds",&fd,buff);

    if(2 == ret)
    {
        len = strlen(buff);
        ret = sys_file_write(fd,buff,len);
        DB(DB2,"%d %d write %d",fd,len,ret);
        if(0 < ret)
        {
            return 0;
        }
    }

    return -1;
}

INT32 cmd_fileopen(const INT8 *str)
{
    INT32 ret;
    UINT32 opt;
    INT8  name[13];

    ret = sget_response(str,"sd",name,&opt);

    if(2 == ret)
    {
        switch(opt)
        {
            case 0:
            {
                opt = FS_O_RDONLY;
            }
            break;
            case 1:
            {
                opt = FS_O_WRONLY;
            }
            break;
            case 2:
            {
                opt = FS_O_RDWR;
            }
            break;
            case 3:
            {
                opt = FS_O_RDWR | FS_O_CREAT;
            }
            break;
            case 4:
            {
                opt = FS_O_RDWR | FS_O_CREAT | FS_O_EXCL;
            }
            break;
            case 5:
            {
                opt = FS_O_RDWR | FS_O_CREAT | FS_O_TRUNC;
            }
            break;
            case 6:
            {
                opt = FS_O_RDWR | FS_O_CREAT | FS_O_APPEND;
            }
            break;
            default:
            return -1;
        }
        
        ret = sys_file_open(name,opt);

        DB(DB2,"open %d",ret);

        if(0 <= ret)
        {
            return 0;
        } 
    }

    return -1;
}

/*list all of commands*/
INT32 cmd_list(const INT8 *str)
{
	UINT16 i;
	
	for(i = 0;i < sizeof(cmd_table)/sizeof(CMD_T);i++)
	{
		sys_uart_output(0,cmd_table[i].cmd,(UINT16)strlen(cmd_table[i].cmd));
		sys_uart_output(0,"\r\n",2);
	}

	return 0;
}

/*alloc memory function
 * AT+MALLOC=<size>,<n> 
*/
INT32 cmd_memalloc(const INT8 *str)
{
	INT32 size,n,i,ret;
	UINT8 *p;
	
	size = 0;
	n = 0;

	ret = sget_response(str,"dd",&size,&n);

    if(2 == ret)
    {
    	for(i = 0;i < n;i++)
    	{
    		p = sys_malloc(size);	
    		DB(DB2,"malloc a block %x size of %d bytes !! %s",p,size,p ? "success":"fail");
    		sys_taskSleep(10);
    	}

    	return 0;
	}
	else
	{
	    return -1;
	}
}

/*send a msg to a task
 * AT++SENDMSG=<tid>,<msg id>,<n1>,<n2>,<n3> 
*/
INT32 cmd_sendmsg(const INT8 *str)
{
	UINT32 id,n1,n2,n3;
	INT32 tid,n;
	
	n = sget_response(str,"ddddd",&tid,&id,&n1,&n2,&n3);
	//eeeeeeeeeeeeeeeelog_err("cmd sendmsggggggggggggggggggggggggggggggggggggggg\n");
	DB(DB1,"[cmd_sendmsg] n[%d] tid[%d] id[%d] n1[%d] n2[%d] n3[%d]",n,tid,id,n1,n2,n3);

	switch(tid)
	{
		case 1:
		{
			tid = TASK_ONE;
		}
		break;
		case 2:
		{
			tid = TASK_TWO;
		}
		break;
		case 3:
		{
			tid = TASK_THREE;
		}
		break;
		case 4:
		{
			tid = TASK_FOUR;
		}
		break;
		case 5:
		{
			tid = TASK_FIVE;
		}
		break;
		default:
		return -1;
	}
	
	sys_taskSend(tid,id,n1,n2,n3);

	return 0;
}


struct 
{
	CMD_STATE_T state;
	UINT16      cmd_len;
	INT8 cmd[CMD_MAX_LENGTH];
}cmd_parser = {CMD_STATE_START,0,{0}};

/*cmd proccess
*must start with "AT"*/
void cmd_proccess(const INT8 *str,UINT16 len)
{
	UINT16 i;

	for(i = 0;i < len; i++)
	{
		switch(cmd_parser.state)
		{
			case CMD_STATE_START:
			{
				if('a' == str[i] ||
				   'A' == str[i])
				{
					cmd_parser.state = CMD_STATE_RECV_A;
				}
			}
			break;
			case CMD_STATE_RECV_A:
			{
				if('t' == str[i] ||
				   'T' == str[i])
				{
					cmd_parser.state = CMD_STATE_RECV_T;
				}
				else if('a' == str[i] ||
				        'A' == str[i])
				{
					cmd_parser.state = CMD_STATE_RECV_A;
				}
				else
				{
					memset(&cmd_parser,0,sizeof(cmd_parser));
				}
			}
			break;
			case CMD_STATE_RECV_T:
			{
				if('\r' == str[i])
				{
					call_cmd("AT",2);
					memset(&cmd_parser,0,sizeof(cmd_parser));
				}
				else
				{
					cmd_parser.state = CMD_STATE_RECV_EXT;
					cmd_parser.cmd[cmd_parser.cmd_len++] = str[i];
				}
			}
			break;
			case CMD_STATE_RECV_EXT:
			{
				if('\r' == str[i])
				{
					call_cmd(cmd_parser.cmd,cmd_parser.cmd_len);
					memset(&cmd_parser,0,sizeof(cmd_parser));
				}
				else
				{
					cmd_parser.cmd[cmd_parser.cmd_len++] = str[i];
				}
			}
		}
		
	}
}


void call_cmd(const INT8 *buff,UINT16 len)
{
    INT32 ret;
	UINT16 i;
	UINT16 cmdlen;
    UINT8 str[64];

    ret = -1;
    //log_err("call cmd111111111111111111111111");
	for(i = 0;i < sizeof(cmd_table)/sizeof(CMD_T);i++)
	{
	    cmdlen = strlen(cmd_table[i].cmd);
		if(0 == cmdcmp(i,(UINT8 *)buff,len))
		{
			if(cmd_table[i].cmd_handler)
			{
				//sys_uart_output(0,"\r\n",2);
                
				ret = cmd_table[i].cmd_handler(&buff[cmdlen]);
				if(0 == ret)
				{
				    goto OK;
				}
				else
				{
				    goto ERROR;
				}
			}
			else
			{
				DB(DB1,"[call_cmd] command found but no handler !!");
				goto ERROR;
			}
			break;
		}
	}

	if(sizeof(cmd_table)/sizeof(CMD_T) == i)
	{
		DB(DB1,"[call_cmd] command not found !!");
		goto ERROR;
	}

	return;

OK:
	//sys_uart_output(0, "\r\nOK\r\n", 6);
    sys_uart_output(0, "\r\n", 2);
	return;
ERROR:
    //len = sprintf(str, "\r\AT order: %ld\r\n", ret);
	//sys_uart_output(0, str, len);

    len = sprintf(str, "+ERR=%ld\r\n\r\n", ret);
    sys_uart_output(0, str, len);
    return;
}

INT32 cmdcmp(INT32 i,const UINT8 *b,UINT16 at_len)
{
    UINT16 ati = 0;
    UINT16 cmd_len = 0;
    const UINT8 *pcmd = cmd_table[i].cmd;

    cmd_len = (UINT16)strlen(cmd_table[i].cmd);

    while(ati < at_len)
    {
        if(toupper(*pcmd) != toupper(*b))
        {
            return ati+1;
        }

        if('\0' == *pcmd && '\0' == *b)
        {
            return 0;
        }
        
        ati++;
        pcmd++;
        b++;

        if('\0' == *pcmd)
        {
            if('=' == *b)
            {
                return 0;
            }
            else if('\0' == *b)
            {
                return 0;
            }
            else
            {
                return ati+1;
            }
        }        
    }

    return ati;
}

extern UINT8 ResendCnt;
/*socket connect success*/
void cmd_sock_connect_handle(UINT32 n1,UINT32 n2,UINT32 n3)
{
    if((INT32)n1 == cmd_tcp_sock)
    {
        log_info("socket %d connect success %d %d!!",cmd_tcp_sock,(INT32)n2,(INT32)n3);
    }

    if((INT32)n1 == gRunParam.tcp_sock)
    {
        log_info("tcp socket %d connect success %d %d!!",gRunParam.tcp_sock,(INT32)n2,(INT32)n3);
        setTcpSocketStatus(TCP_CONNECT);
        ResendCnt = 0;
    }

    if((INT32)n1 == gRunParam.lbs_sock)
    {
        log_info("lbs socket %d connect success %d %d!!",gRunParam.lbs_sock,(INT32)n2,(INT32)n3);
        // ~~~~~~~todo setLbsSocketStatus(TCP_CONNECT);
    }

    if((INT32)n1 == gRunParam.update_sock)
    {
        log_info("ud socket %d connect success %d %d!!",gRunParam.update_sock,(INT32)n2,(INT32)n3);
        setUDSocketStatus(TCP_CONNECT);
    }
}

UINT8 TcpConnectFaileCnt = 0;
UINT8 LBSConnectFaileCnt = 0;
UINT8 UDConnectFaileCnt = 0;
/*socket error , close socket*/
void cmd_sock_error_handle(UINT32 n1,UINT32 n2,UINT32 n3)
{
    if((INT32)n1 == gRunParam.tcp_sock)
    {
        TcpConnectFaileCnt++;
        log_err("socket %d error %d %d ,%d!!",gRunParam.tcp_sock,(INT32)n2,(INT32)n3,TcpConnectFaileCnt);
        if(TcpConnectFaileCnt >= 5)
        {
            TcpConnectFaileCnt = 0;
            log_err("TcpConnectFaileCnt >= 5 ， will sys_softReset() ");
            if(g_device_working == 0)
            {
                sys_softReset();
            }
            
            return;

        }
        setTcpSocketStatus(TCP_SOCKET_CLOSE);
    }

    if((INT32)n1 == gRunParam.lbs_sock)
    {
        LBSConnectFaileCnt++;
        //20170106
        log_err("socket %d error %d %d ,%d!!",gRunParam.lbs_sock,(INT32)n2,(INT32)n3,LBSConnectFaileCnt);
        if(LBSConnectFaileCnt >= 5)
        {
            LBSConnectFaileCnt = 0;
            log_err("LBSConnectFaileCnt >= 5 ， will sys_softReset() ");
            if(g_device_working == 0)
            {
                sys_softReset();
            }
            //sys_softReset();
            return;
        }
        // todo ~~~ setLbsSocketStatus(TCP_SOCKET_CLOSE);
    }

    if((INT32)n1 == gRunParam.update_sock)
    {
        UDConnectFaileCnt++;
        //20170106
        log_err("socket %d error %d %d ,%d!!",gRunParam.update_sock,(INT32)n2,(INT32)n3,UDConnectFaileCnt);
        if(UDConnectFaileCnt >= 5)
        {
            UDConnectFaileCnt = 0;
            log_err("UDConnectFaileCnt >= 5 ， will sys_softReset() ");
            if(g_device_working == 0)
            {
                sys_softReset();
            }
            //sys_softReset();
            return;
        }
        setUDSocketStatus(TCP_SOCKET_CLOSE);
        //20170106
        log_info("Because UDConnectFaileCnt > 5,sys_softReset");
        sys_softReset();
    }

    if((INT32)n1 == cmd_tcp_sock)
    {
        log_err("socket %d error %d %d !!",cmd_tcp_sock,(INT32)n2,(INT32)n3);
        sys_sock_close(cmd_tcp_sock);
        cmd_tcp_sock = -1;
    }else if((INT32)n1 == mqttSocketId)
    {
        log_err("socket %d error %d %d !!",mqttSocketId,(INT32)n2,(INT32)n3);
        sys_sock_close(mqttSocketId);
        mqttSocketId = -1;
		//mqtt_op = MQTT_OP_DISCONNECT;

	}
}

/*socket recv data*/
extern MQTT_OP_T mqtt_op;

void cmd_sock_data_recv_handle(UINT32 n1,UINT32 n2,UINT32 n3)
{
	//DB(DB3,"cmd_sock_data_recv_handle,sockid:%d",(INT32)n1);
#if 0
    INT32 ret;
    UINT8 buff[1400] = {0};
    
    if((INT32)n1 == cmd_tcp_sock)
    {
        DB(DB3,"socket %d read %d %d !!",cmd_tcp_sock,(INT32)n2,(INT32)n3);
        ret = sys_sock_recv(cmd_tcp_sock,buff,1300);
        DB(DB3,"data len %d : %s",ret,buff);
    }
    else if((INT32)n1 == cmd_udp_sock)
    {
        GAPP_TCPIP_ADDR_T addr;
        UINT8 *pip = (UINT8*)&addr.sin_addr.addr;
        ret = sys_sock_recvFrom(cmd_udp_sock,buff,1300,&addr);
        DB(DB3,"recv %d data from [%d.%d.%d.%d:%d] : %s",ret,pip[0],pip[1],pip[2],pip[3],ntohs(addr.sin_port),buff);
    }
#endif
    INT32 ret = 0, ret2 = 0;
    UINT8 buff[1440] = {0};
   // log_debug("this is recivinggggggggggggggggggg\n");
	if((INT32)n1 == mqttSocketId)
    {
    	//DB(DB3,"socketid(%d), readlen(%d).",(INT32)n1,(INT32)n2);
        //ret = sys_sock_recv(mqttSocketId,buff,n2);
		mqtt_op = MQTT_OP_RECV;

	} else if((INT32)n1 == gRunParam.tcp_sock)
    {
        /*DB(DB3,"socket %d read %d %d !!",gRunParam.tcp_sock,(INT32)n2,(INT32)n3);
        ret = sys_sock_recv(gRunParam.tcp_sock,buff,n2);
        DB(DB3,"data len %d ,%s",ret,buff);
        if(ret > 0)
        {
            ParseRecvData(buff,ret);
        }
        if((ret < n2)&&(ret>0))
        {
            sys_taskSend(TASK_ONE,T1_SOCK_DATA_RECV_IND,gRunParam.tcp_sock,1440,0);
        }*/
    }
    else if((INT32)n1 == gRunParam.lbs_sock)
    {
        /*
        DB(DB3,"socket %d read %d %d !!",gRunParam.lbs_sock,(INT32)n2,(INT32)n3);
        ret = sys_sock_recv(gRunParam.lbs_sock,buff,n2);
        DB(DB3,"data len %d : %s",ret,buff);
        if(ret > 0)
        {
            ret2 = ParseLBSData(buff,ret);
            if(0 == ret2)
            {
                gRunParam.LBSStatus = TCP_SOCKET_CLOSE;
            }
        }
        if((ret < n2)&&(ret>0))
        {
            sys_taskSend(TASK_ONE,T1_SOCK_DATA_RECV_IND,gRunParam.lbs_sock,1440,0);
        }
        */
    }
    else if((INT32)n1 == gRunParam.update_sock)
    {
        log_info("ud socket %d read %d %d !!",gRunParam.update_sock,(INT32)n2,(INT32)n3);
        ret = sys_sock_recv(gRunParam.update_sock,buff,1024);
        log_info("data len %d ......................................",ret);
        if(ret > 0)
        {
            ret2 = ParseUDHttpData(buff,ret);
            log_info("ParseUDHttpData return %d", ret2);
            if(-1 == ret2)
            {
                setUDSocketStatus(TCP_SOCKET_CLOSE);
                log_info("write file faile...");
                sys_file_close(gRunParam.UDFile);
                sys_file_delete(UD_FILE_NAME);
                gRunParam.UDFile = -1;
            }
            else if(1 == ret2)
            {
                setUDSocketStatus(TCP_SOCKET_CLOSE);
                log_info("wrote all ok...");
                sys_file_close(gRunParam.UDFile);
                //sys_taskSend(TASK_ONE,T1_SOFT_RESET,0,0,0);
                sys_taskSleep(3000);
                sys_softReset();
            }
        }
        if((ret < n2)&&(ret>0))
        {
            sys_taskSend(TASK_ONE,T1_SOCK_DATA_RECV_IND,gRunParam.update_sock,1440,0);
        }
    }
    else if((INT32)n1 == cmd_tcp_sock)
    {
        log_info("socket %d read %d %d !!",cmd_tcp_sock,(INT32)n2,(INT32)n3);
        ret = sys_sock_recv(cmd_tcp_sock,buff,1300);
        log_info("data len %d : %s",ret,buff);
    }
    else if((INT32)n1 == cmd_udp_sock)
    {
        GAPP_TCPIP_ADDR_T addr;
        UINT8 *pip = (UINT8*)&addr.sin_addr.addr;
        ret = sys_sock_recvFrom(cmd_udp_sock,buff,1300,&addr);
        log_info("recv %d data from [%d.%d.%d.%d:%d] : %s",ret,pip[0],pip[1],pip[2],pip[3],ntohs(addr.sin_port),buff);
    }
}

/*peer send a FIN in tcp*/
void cmd_sock_close_ind_handle(UINT32 n1,UINT32 n2,UINT32 n3)
{
    INT32 ret;
    
    if((INT32)n1 == cmd_tcp_sock)
    {
        ret = sys_sock_close(cmd_tcp_sock);
        log_info("ret %d",ret);
    }else if((INT32)n1 == mqttSocketId)
    {
        log_info("socket %d close ind!!",mqttSocketId);
        sys_sock_close(mqttSocketId);
		//mqtt_op = MQTT_OP_DISCONNECT;

	}

    if((INT32)n1 == gRunParam.tcp_sock)
    {
        ret = sys_sock_close(gRunParam.tcp_sock);
        gRunParam.tcp_sock = -1;
        setTcpSocketStatus(TCP_SOCKET_CLOSED);
        log_info("tcp_sock close_ind ret=%d",ret);
    }

    if((INT32)n1 == gRunParam.lbs_sock)
    {
        ret = sys_sock_close(gRunParam.lbs_sock);
        gRunParam.lbs_sock = -1;
        //todo ~~~~setLbsSocketStatus(TCP_SOCKET_CLOSED);
        log_info("lbs_sock close_ind ret=%d",ret);
    }

    if((INT32)n1 == gRunParam.update_sock)
    {
        ret = sys_sock_close(gRunParam.update_sock);
        gRunParam.update_sock = -1;
        setUDSocketStatus(TCP_SOCKET_CLOSED);
        log_info("update_sock close_ind ret=%d",ret);
    }
}
/*close a socket success*/
void cmd_sock_close_rsp_handle(UINT32 n1,UINT32 n2,UINT32 n3)
{    
    if((INT32)n1 == cmd_tcp_sock)
    {
        cmd_tcp_sock = -1;
    }
	else if((INT32)n1 == mqttSocketId)
    {
		mqttSocketId = -1;
		//mqtt_op = MQTT_OP_DISCONNECT;

	}

    if((INT32)n1 == gRunParam.tcp_sock)
    {
        gRunParam.tcp_sock = -1;
        setTcpSocketStatus(TCP_SOCKET_CLOSED);
        log_info("tcp_sock close_rsp");
    }

    if((INT32)n1 == gRunParam.lbs_sock)
    {
        gRunParam.lbs_sock = -1;
        //todo ~~~~  setLbsSocketStatus(TCP_SOCKET_CLOSED);
        log_info("lbs_sock close_rsp");
    }


    if((INT32)n1 == gRunParam.update_sock)
    {
        gRunParam.update_sock = -1;
        setUDSocketStatus(TCP_SOCKET_CLOSED);
        log_info("update_sock close_rsp");
    }
}

void cmd_hdw_test(void)
{
	//hdw_test_self();
}


