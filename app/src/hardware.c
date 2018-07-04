#include <mqttcloud.h>
#include "hardware.h"
#include "http.h"
#include "json2.h"


int g_device_working=0;
char ota_url[128] ={0};
char ota_md5[128] ={0};

int circle_time = 0;
int circle_remainder = 0;

hdw_topic hdw_topic_para;//存放三个topic结构体，data topic  request topic  response topic


extern  MQTT_KEY_PARAM_T mqtt_key_param;
//extern UINT32 SecondCnt;


void hdw_watchdog_enable()
{
	sys_watchdog_enable(FEED_WATCHDOG_TIME);
}

void hdw_watchdog_feed()
{
	sys_watchdog_feed();
}

void hdw_watchdog_disable()
{
	sys_watchdog_disable();
}

void hdw_gpio_init()
{
	GPIO_CONFIG_0;
	GPIO_CONFIG_5;
	GPIO_RESET_0;
	GPIO_SET_5;
	log_info("22 pin reset, 20 pin set");
}

/**
  *  @author Leslie
  *  @Optimize Motor Working Time   
  *  @return 
  */
INT32 temp_time;
void  circle_times(void)                                             
{
    // if(g_device_working ==0 )
    //     return ;
    while(circle_time--)
    {
        log_info("circle_time ======================================================================================================================================================================= %d",circle_time);
        sys_timer_new(5 * 60 * 1000,circle_times,NULL);
        return ;
    }
    circle_time =  0;
    if(circle_remainder != 0)
    {
        log_info("circle_remainder ================================================================================================================================================================== %d",circle_remainder);
        sys_timer_new(circle_remainder * 1000,hdw_motor_timer_hanlder,NULL);
        circle_remainder = 0;
        return ;
    }
    /*if((cricle_time === 0) && (circle_remainder == 0))
        return ;*/
    sys_timer_new(100,hdw_motor_timer_hanlder,NULL);
    return  ;   
}


int hdw_startDevice(int sec)
{
    circle_time = sec/300;
    circle_remainder = sec%300;

    log_info("startDevice: %d second", sec);
    /*device_work_begin_time = SecondCnt;
    device_work_end_time = SecondCnt + sec;
    log_info("SecondCnt=%d, device_work_end_time=%d", SecondCnt, device_work_end_time);
    */
    HDW_MOTOR_ON;
    GPIO_RESET_5;
    g_device_working = 1;
    log_info("Motor Start To Work, g_device_working == %d",g_device_working);
    circle_times();
    //修改定时时间 sys_timer_new定时范围1ms~10mins
    /**/
    //sys_timer_new(sec * 1000,hdw_motor_timer_hanlder,NULL);
    /*if((circle_time <= 1) && (circle_remainder == 0))  
    {
        sys_timer_new(circle_remainder * 60 * 1000, hdw_motor_timer_hanlder,NULL);
    }
    else
    {
        if(circle_remainder = 0)
        {
            
        }
    }*/
}

void gapp3_device_on_hanlder(void *arg)
{
    log_info("gapp3_device_on_hanlder");
    /*send a msg to Task 3 to handle timer expire event*/
    sys_taskSend(TASK_THREE,T3_MOTOR_HANDLE_IND,0,0,0);
}


//从文件系统读取参数
int ReadDevParam(void)
{
	UINT32 opt;
	INT32 fd1,fd2,ret;
	INT32 file_len, file_len2;
	//DEVICE_PARAM_T TempBuff;
	//DEVICE_PARAM_T TempDefPara;

	//Device_Param_default_set();              //先设置成默认值
	opt = FS_O_RDWR | FS_O_CREAT;
	fd1 = sys_file_open(HDW_DEV_INIT_FILE, opt); //
	DB(DB1,"devpara open %d",fd1);
	if(0 <= fd1)
	{
		file_len = sys_file_read(fd1,&mqtt_key_param,sizeof(mqtt_key_param));//文件名字长度不能大于12
		DB(DB1,"devpara read %d",file_len);
        log_info("mqtt_key_param   %s %s %s",mqtt_key_param.productkey,mqtt_key_param.devname,mqtt_key_param.devsecret);
		sys_file_close(fd1);    //每次读取完之后最好关闭文件
		if(file_len == sizeof(mqtt_key_param))
		{
			/*if(Device_Param.version != TempBuff.version) //保存的版本号发送改变，则删除保存的文件
			{
				ret = sys_file_delete("devpara.txt");
				DB(DB1,"delete devpara ret= %d",ret);
			}
			else
				Device_Param = TempBuff;
			 */
			DB(DB1,"devpara read %d",file_len);
		}
	} 
    else
    {
        log_info("not find key file");
    }

	return 0;
}

//把参数保存到文件，目前参数只传1，如果要保存其他文件可以增加参数的值
void WriteDevParamToFlash(UINT8 type)
{
	UINT32 opt;
	INT32 fd;
	INT32 len;
    log_info("Begin To Write ALI's Element To key.txt");
	if(1 == type)
	{
		opt = FS_O_RDWR | FS_O_CREAT;
    
		fd = sys_file_open(HDW_DEV_INIT_FILE, opt);
       

		len = sys_file_write(fd, &mqtt_key_param, sizeof(mqtt_key_param));
      
		if(len != sizeof(mqtt_key_param))
		{
			// DB(DB1,"WriteDevParamToFlash error %d",len);
            log_info("WriteDevParamToFlash error %d",len);
			sys_file_close(fd);
			return;
		}
	}
	else
	{

	}
  
	INT32 ret = sys_file_flush(fd, 10000); //写入到flash,最长10秒返回
	log_info("sys_file_flush ret = %d", ret);
	sys_file_close(fd);
	return;
}



void hdw_dev_initial(hdw_operation data)//设备初始化
{
	int i;
	char read[100];
	char *token;  
	char delim[] = "#";  
	//hdw_data_read_file(read, HDW_DEV_INIT_FILE);

 
#if 1
	ReadDevParam();
#else
    strcpy(mqtt_key_param.productkey, PRODUCT_KEY);
	strcpy(mqtt_key_param.devname, DEVICE_NAME);
	strcpy(mqtt_key_param.devsecret, DEVICE_SECRET);
#endif

	if(strlen(mqtt_key_param.devsecret) > 1 )
	{
		hdw_start_up_flg = 1;
		log_debug("mqtt_key_param.devsecret = %s", mqtt_key_param.devsecret );
	}
	else
	{
		hdw_start_up_flg = 0;
	}
	hdw_gpio_init();//io 初始化
	hdw_watchdog_enable();//看么狗初始化
	if(hdw_start_up_flg == 1)
	{
		sprintf(hdw_topic_para.data, "/%s/%s/data", mqtt_key_param.productkey , mqtt_key_param.devname);		//生产data topic
		sprintf(hdw_topic_para.request, "/sys/%s/%s/rrpc/request/+", mqtt_key_param.productkey , mqtt_key_param.devname);//生产 request topic
        log_debug("topic request :%s\n\r", hdw_topic_para.request);
		
		sprintf(hdw_topic_para.response, "/sys/%s/%s/rrpc/response/", mqtt_key_param.productkey , mqtt_key_param.devname);// 生产response topic
        log_debug("topic request :%s\n\r", hdw_topic_para.response);

		//hdw_mqtt_key_param_init();
		// cmd_aliconn(NULL);

	}
    log_info("Read Flag Ok And Start Time == %d", sys_getSysTick() / 16384 -1);
}


/**
  *  @author Leslie
  *  @Read ALi Params And Init GPIO   
  *  @flag 0,TCP OR MQTT Mode
  *  @flag 1,MQTT FROM TCP Mode
  *  @return NULL
  */
void hdw_dev_init(hdw_operation data,UINT8 flag)
{
    int i;
    char read[100];
    char *token;
    char delim[] = "#";
    if(flag == 0)
    {    
        ReadDevParam();
        if(strlen(mqtt_key_param.devsecret) > 1 )
        {
            hdw_start_up_flg = 1;
            log_debug("mqtt_key_param.devsecret = %s", mqtt_key_param.devsecret );
        }
        else
        {
            hdw_start_up_flg = 0;
        }
        hdw_gpio_init();
        hdw_watchdog_enable();
        if(hdw_start_up_flg == 1)
        {
            sprintf(hdw_topic_para.data, "/%s/%s/data", mqtt_key_param.productkey , mqtt_key_param.devname);        //生产data topic
            sprintf(hdw_topic_para.request, "/sys/%s/%s/rrpc/request/+", mqtt_key_param.productkey , mqtt_key_param.devname);//生产 request topic
            log_debug("topic request :%s\n\r", hdw_topic_para.request);
            
            sprintf(hdw_topic_para.response, "/sys/%s/%s/rrpc/response/", mqtt_key_param.productkey , mqtt_key_param.devname);// 生产response topic
            log_debug("topic request :%s\n\r", hdw_topic_para.response);

            //hdw_mqtt_key_param_init();
            cmd_aliconn(NULL);

        }

    }
    else if(flag == 1)
    {
        ReadDevParam();
        log_debug("mqtt_key_param.devsecret = %s", mqtt_key_param.devsecret );
        sprintf(hdw_topic_para.data, "/%s/%s/data", mqtt_key_param.productkey , mqtt_key_param.devname);        //生产data topic
        sprintf(hdw_topic_para.request, "/sys/%s/%s/rrpc/request/+", mqtt_key_param.productkey , mqtt_key_param.devname);//生产 request topic
        log_debug("topic request :%s\n\r", hdw_topic_para.request);
        
        sprintf(hdw_topic_para.response, "/sys/%s/%s/rrpc/response/", mqtt_key_param.productkey , mqtt_key_param.devname);// 生产response topic
        log_debug("topic request :%s\n\r", hdw_topic_para.response);

        //hdw_mqtt_key_param_init();
        cmd_aliconn(NULL);

    }
}



/*****************************************************************************

*********************************定时器*********************************************

***********************************************************************************/
void hdw_motor_timer_hanlder(void *arg)//电机定时关断定时器
{
    if(g_device_working == 0)
    {
        log_info("Device Has Stopped ,Stop Send \"stop\" To Server");
        return ;
    }
        
    circle_time = 0;
    circle_remainder = 0;
    //sys_timer_free(temp_time);
    log_info("hdw_motor_timer_hanlder");
    sys_taskSend(TASK_THREE, T3_MOTOR_HANDLE_IND, 0, 0, 0); //跳转到按摩抱枕关闭事件
    if(hdw_start_up_flg == 1)
	    sys_taskSend(TASK_FIVE, T5_HDW_SEND_DATA_TOSERVER, 0, 0, 0);//跳转到发送stop json 字符串事件
    //g_device_working = 0;
	log_info("hdw time is over");
}

void hdw_dev_init_timer_hanlder(void *arg)//设备获取参数时的定时器，没2秒扫描1次tcp 事件
{
    //log_info("");
    sys_taskSend(TASK_THREE,T3_CSQ_TIMER_IND,0,0,0); 
	hdw_watchdog_feed();
}

//*************************uart 硬件测试*****************************************
void hdw_test_self(UINT8 *data)//AT 指令集 硬件检测
{
	char IMSI[16];
	char imei[16];
	unsigned char *test_order = "AT+TEST\r\n";
	if((memcmp((UINT8*)test_order, (UINT8*)data, 7)) == 0)
	{
		AT_GetIMSI(IMSI);
		AT_GetSerialNumber(imei);
		int csq = AT_GetSignal();
		log_info("CSQ = %d", csq);
        log_info("IMSI = %s", IMSI);
        log_info("IMEI = %s", imei);
		hdw_startDevice(5);
		//sys_file_clear();// 设备运转好以后 删除
		//DB(DB2,"hardware is delete");
		//sys_taskSend(TASK_THREE, T3_MOTOR_HANDLE_IND, 0, 0, 0); //关电机

        //sys_get(GAPP_OPT_APP_UPDATA_ID, )

	}
	else
	{
		//log_info("if you want to cheak up the hard ,putin: AT+TEST\n ");
	}
}
extern char IMSI[16];

//发送数据处理*****************************************
void hdw_send_register_data(char *send_data)// 按摩抱枕注册设备信息时，想TCP服务器发送  tag  json串。
{
	char imei[16];
    char iccid[21];
	AT_GetSerialNumber(imei);
    AT_GetICCID(iccid);

	int csq = AT_GetSignal();

	UINT32 tick = sys_getSysTick() / 16384;

    sprintf(send_data, "{\"type\":\"reg\",\"imei\":\"%s\",\"iccid\":\"%s\",\"csq\":%d,\"tick\":%d,\"version\":\"%s\",\"protocol\":\"MQTT\"}\r\n",
			imei, iccid, csq, tick, VERSION);
}

char hdw_ini_register_json_parser(char *src, char *data)//设备注册时，接收TCP服务器过来的json，并进行处理
{
    char *hdw_dev_init_receive[3];
    char *hdw_dev_init_param_key[3] = {"pdk", "id", "pwd"};

    for(int i = 0; i < 3; i++)
    {
        hdw_dev_init_receive[i] = LITE_json_value_of( hdw_dev_init_param_key[i], src);
    }
    log_debug("hdw_dev_init_receive = %s",hdw_dev_init_receive[0]);
    log_debug("hdw_dev_init_receive = %s",hdw_dev_init_receive[1]);
    //log_info("hdw_dev_init_receive = %s",hdw_dev_init_receive[2]);

    strcpy(mqtt_key_param.productkey,   hdw_dev_init_receive[0]);
    strcpy(mqtt_key_param.devname,   hdw_dev_init_receive[1]);
    strcpy(mqtt_key_param.devsecret,   hdw_dev_init_receive[2]);

    sprintf(data, "%s#%s#%s#", hdw_dev_init_receive[0], hdw_dev_init_receive[1], hdw_dev_init_receive[2]);
}

void hdw_send_init_data(char *send_data)// 按摩抱枕初始化时，想服务器发送init json串。
{
    int protocol = 1;
    int csq = AT_GetSignal();
    char imei[16];
    AT_GetSerialNumber(imei);
	UINT32 tick = sys_getSysTick() / 16384;

    // sprintf(send_data, "{\"type\":\"init\",\"imei\":\"%s\",\"csq\":%d,\"tick\":%d,\"version\":\"%s\",\"protocol\":\"MQTT\"}\r\n",
    //         imei, csq, tick, VERSION);
    sprintf(send_data, "i%d,%d,%d,%s\r\n", protocol, csq, tick, VERSION);
}

void hdw_send_stop_data(char *send_data)// 按摩保证停止时，向服务器发送stop  json串。
{
	sprintf(send_data, "%s", "s0");
}

void  hdw_send_ping_data(char *ping_data)
{
    sprintf(ping_data, "%s", "{\"type\":\"ping\"}");
}
/*****************************************************************************

********************************接收数据处理 分析********************************************

***********************************************************************************/

void hdw_data_topic_message_process(const char *msg) //取消json格式 
{
    UINT16 ntime = 0;
    UINT16 ucnt = 1;
    char productkey[64] = {0};
    char devname[64] = {0};
    char devsecret[128] = {0};

    if(msg == NULL || strlen(msg) == 0)
    {
        return ;
    }
    switch(msg[0])
    {
    case '1':
        if (strlen(msg) < 2)
        {
            log_info("length is error , please resend");
            return;
        }
        log_info("1");
        if(g_device_working == 1)
        {
            log_info("Device Has Already Started ,Please Wait Device Stop");
            break;
        }
        ntime = atoi(&msg[1]);
        if (ntime > 0)
        {

            hdw_startDevice(ntime);
            Op_ali_pub(hdw_topic_para.data, "s1");
        }
        else
        {
            Op_ali_pub(hdw_topic_para.data, "e1");
        }
        break;
    case 0x32:
        log_info("2");
        // strcat(mqtt_key_param.productkey,productkey);
        // sprintf(mqtt_key_param.productkey, "%s", productkey);
        // sprintf(mqtt_key_param.devname, "%s", devname);
        // sprintf(mqtt_key_param.devsecret, "%s", devsecret);
        memset(&mqtt_key_param,0x0,sizeof(mqtt_key_param));
        char *temp_msg = strtok(&msg[1],",");
        strcat(mqtt_key_param.productkey,temp_msg);
        while(temp_msg)
        {
            ucnt++;
            log_info("%s ",temp_msg);
            temp_msg = strtok(NULL,",");
            if(ucnt==2)
                strcat(mqtt_key_param.devname,temp_msg);
            if(ucnt==3)
                strcat(mqtt_key_param.devsecret,temp_msg);
        }
        // log_info("%s,%s",mqtt_key_param.productkey, productkey);
        // log_info("%s,%s",mqtt_key_param.devname, devname);
        // log_info("%s,%s",mqtt_key_param.devsecret,  devsecret);
        WriteDevParamToFlash(1);
        // ReadDevParam();
        Op_ali_pub(hdw_topic_para.data,"o2");
        sys_taskSleep(1000);
        sys_softReset();
        break;
    case 0x33:
        log_info("3");
        Op_ali_pub(hdw_topic_para.data,"o3");
        sys_taskSleep(1000);
        sys_softReset();
        break;
    case 0x34:
        log_info("4");
        hdw_motor_timer_hanlder(NULL);
        // Op_ali_pub(hdw_topic_para.data,"o4");
        break;
    case 0x35:
        log_info("5");
        char *temp_msg_ota = strtok(&msg[1], ",");
        strcat(ota_url, temp_msg_ota);
        while (temp_msg_ota)
        {
            ucnt++;
            log_info("%s ", temp_msg_ota);
            temp_msg_ota = strtok(NULL, ",");
            if (ucnt == 2)
                strcat(ota_md5, temp_msg_ota);
            break;
        }
        if (ota_url != NULL && strlen(ota_url) > 0)
        {
            log_info("begin ota ~~~~~~~~~~~, url=%s  md5=%s", ota_url, ota_md5);

            sys_taskSend(TASK_THREE, T3_OTA_IND, 0, 0, 0);
        }
        break;
        0x31;
    case 0x36:
        log_info("6");
        break;
    case 0x37:
        log_info("7");
        break;
        // default：
        // break;
    }

}
/*
void hdw_data_topic_message_process(const char *msg)// data  topic 数据处理，协议待添加
{
    // log_info("begin to hdw_data_topic_message_process");
    if( msg == NULL || strlen(msg) == 0 )
    {
        log_err(" ~~ msg is null ......");
        return ;
    }

    char *key = "type";
    char *key1 = "time";
    char *key2 = "pdk";
    char msg_type[20];
    char msg_time[20];
    char msg_send[40];
    char msg_pwd[40];
    char heart[10];
    memset(msg_type, 0, sizeof(msg_type));
    memset(msg_send,0,sizeof(msg_type));
    memset(ota_url, 0, sizeof(ota_url));
    memset(ota_md5, 0, sizeof(ota_md5));
    memset(heart, 0, sizeof(heart));
    memset(msg_time,0x0,sizeof(msg_time));
    memset(msg_pwd,0x0,sizeof(msg_pwd));

    json_get_value(msg, key, msg_type);
    json_get_value(msg, key1,msg_time);
    json_get_value(msg,key2,msg_pwd);

    if( msg_type == NULL || strlen(msg_type) == 0 )
    {
        log_err(" ~~ msg_type is null ......");
        return ;
    }

    log_debug("msg type=%s", msg_type);


    if(strcmp(msg_type, "begin") ==0 )
    {
        log_info("process type begin ");

        json_get_value(msg, "heart", heart);
        log_info("heart=%s", heart);
        if(heart != NULL && strlen(heart)>0 )
        {
            mqtt_ping_time = atoi(heart);
            log_info(" mqtt_ping_time : %d\n", mqtt_ping_time);
        }

        json_get_value(msg, "ota", ota_url);
        json_get_value(msg, "md5", ota_md5);
        if(ota_url != NULL && strlen(ota_url)>0 )
        {
            log_info("begin ota ~~~~~~~~~~~, url=%s  md5=%s", ota_url, ota_md5);

            sys_taskSend(TASK_THREE,T3_OTA_IND,0,0,0);
        }log_info("process type begin ");

        json_get_value(msg, "heart", heart);
        log_info("heart=%s", heart);
        if(heart != NULL && strlen(heart)>0 )
        {
            mqtt_ping_time = atoi(heart);
            log_info(" mqtt_ping_time : %d\n", mqtt_ping_time);
        }

        json_get_value(msg, "ota", ota_url);
        json_get_value(msg, "md5", ota_md5);
        if(ota_url != NULL && strlen(ota_url)>0 )
        {
            log_info("begin ota ~~~~~~~~~~~, url=%s  md5=%s", ota_url, ota_md5);

            sys_taskSend(TASK_THREE,T3_OTA_IND,0,0,0);
        }

    } else if ( strcmp(msg_type, "ota") ==0 ) {
        json_get_value(msg, "url", ota_url);
        json_get_value(msg,"md5", ota_md5);
        if(ota_url != NULL && strlen(ota_url)>0 )
        {
            log_info("begin ota ~~~~~~~~~~~, url=%s  md5=%s", ota_url, ota_md5);
            sys_taskSend(TASK_THREE,T3_OTA_IND,0,0,0);
        }
    } else if ( strcmp(msg_type, "ping") ==0 ) {

        //static int v = 0;
        char msg_id[10] = {0};
        char pong[100];
        //int msg_id;
        json_get_value(msg, "id", msg_id);
        if(msg_id != NULL && strlen(msg_id)>0 )
        {
            sprintf( pong, "{\"type\":\"pong\",\"id\":\"%s\"}", msg_id);
            Op_ali_pub( hdw_topic_para.data, pong);
        } else
        {
            Op_ali_pub( hdw_topic_para.data, "{\"type\":\"pong\"}");
        }

    } else if ( strcmp(msg_type, "start") == 0) {

        //Op_ali_pub( hdw_topic_para.data, "{\"type\":\"starting\"}");
        if( msg_time == NULL || strlen(msg_time) == 0 )
        {
            // log_info(" ~~ msg_time is null ......");
            return ;
        }
        char *time  = LITE_json_value_of("time", msg);
        if(time != NULL || strlen(time)>0)
        {
            int nTime = atoi(time);
            if(nTime > 0)
            {

                hdw_startDevice(nTime);
                Op_ali_pub( hdw_topic_para.data, "{\"type\":\"start\"}");
            }
            else
            {
                Op_ali_pub( hdw_topic_para.data, "{\"type\":\"start\",\"time\":\"error\"}");
            }
        }
        else
        {
            log_info("time error");
        }
    } else if ( strcmp(msg_type, "key") == 0) {

        if( msg_pwd == NULL || strlen(msg_pwd) == 0 )
        {
                // log_err(" ~~ msg_type is null ......");
                return ;
        }

        char productkey[64]={0};
        char devname[64]={0};
        char devsecret[128] = {0};
        json_get_value(msg, "pdk", productkey);
        json_get_value(msg, "id", devname);
        json_get_value(msg, "pwd", devsecret);

        sprintf(mqtt_key_param.productkey,"%s",productkey);
        sprintf(mqtt_key_param.devname,"%s",devname);
        sprintf(mqtt_key_param.devsecret,"%s",devsecret);

        WriteDevParamToFlash(1);
        Op_ali_pub(hdw_topic_para.data,"{\"type\":\"key\"}");

    } else if ( strcmp(msg_type, "reset") == 0) {
        Op_ali_pub( hdw_topic_para.data, "{\"type\":\"reset\"}");
        sys_taskSleep(1000);
        sys_softReset();
    }
    else if( strcmp(msg_type,"end") == 0)
    {
        log_info("mqtt end");
        hdw_motor_timer_hanlder(NULL);
        // sys_taskSend(TASK_THREE, T3_MOTOR_HANDLE_IND, 0, 0, 0);
        // hdw_send_stop_order_toserver();
        // Op_ali_pub( hdw_topic_para.data, "{\"type\":\"end\"}");
    }
}
*/

void hdw_rrpc_topic_message_process(char *msg, char *send_msg)//response  topic 数据处理，
{
    if( msg == NULL || strlen(msg) == 0 )
    {
        log_err(" ~~ msg is null ......");
        return ;
    }

    char *key = "type";
    char *msg_type;
    msg_type = LITE_json_value_of(key, msg);
    if( msg_type == NULL || strlen(msg_type) == 0 )
    {
        log_err(" ~~ msg_type is null ......");
        return ;
    }
    log_debug("msg type=%s", msg_type);

    if(strcmp(msg_type, "start") == 0) {
        char *time  = LITE_json_value_of("time", msg);
        if(time != NULL || strlen(time)>0)
        {
            int nTime = atoi(time);
            if(nTime > 0)
            {
                hdw_startDevice(nTime);
            }
        }

    } else if(strcmp(msg_type, "end") == 0) {
        //hdw_stopDevice();
        log_info("rrpc end");
        sys_taskSend(TASK_THREE, T3_MOTOR_HANDLE_IND, 0, 0, 0); //关电机
    } else if(strcmp(msg_type, "reset") == 0) {
        sys_softReset();//服务器重启还需要考虑电机的运行状态吗？？
    } else if(strcmp(msg_type, "ping") == 0) {

        //hdw_send_ping_toserver();
    }

    sprintf(send_msg, "%s ok", msg_type);



}



