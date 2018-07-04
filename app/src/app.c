#include "sys_callback.h"
#include "sys_services.h"
#include "sys_ext.h"
#include "string.h"
#include "cmd.h"
#include "app.h"
#include "debug.h"
#include "cmddef.h"
#include "manage.h"
#include "nwsetup.h"
#include "mqttcloud.h"
#include "iot_import.h"
#include "iot_export.h"
#include "mqtt_client.h"
#include "lite-log.h"
#include "hardware.h"
#include "TCP.h"
// #include <http.h>
#include <app.h>
//#include "cJSON.h"
#include "LPG.h"

typedef unsigned short    u16_t;
UINT32 SecondCnt = 0;
UINT8 GprsAttched = 0;
UINT8 GprsActived = 0;
INT32 DataRcvSocket = -2;
INT32	mqttSocketId=-1;
// UINT8 err_flag = 0;
int lunch_f=0;
iotx_mqtt_param_t mqtt_params;
void *m_pclient;
MQTT_OP_T mqtt_op=MQTT_OP_IDLE;
MQTT_OP_PARAM_T mqtt_op_param;
MQTT_STATE_T m_state=MQTT_STATE_DISCONNECT;
UINT8 ping_flag=0;

hdw_operation hdw;

extern char hdw_imei[16];
extern char hdw_imsi[16];
extern int g_device_working;
extern char ota_url[128];
extern char ota_md5[128];

LPG_SYS_SIGNAL_STAT  LPG_stat;
//extern u16_t CRC_Calculate( u16_t Length,unsigned char *Address ); 

UINT8 ResendCnt = 0;             /*数据发送失败时重发的次数，超过一定次数关掉socket重连*/
RUN_PARAM_T gRunParam =
        {
                .GprsStatus = 0,
                .TcpStartLink = 1,
                .UpdateStartLink = 0,
                .LBSStartLink = 0,
                .GSMRegisted = 0,
                .GprsRegisted = 0,
                .TcpStatus = 0,
                .tcp_sock = -1,
                .UDFile = -1,
                .UDPort = 80,
                .jd = {0},
                .wd = {0}
        };
DEVICE_PARAM_T Device_Param;

static UINT8 app_init(GAPP_TASK_T **tl);
void  uart_in(INT32 uid,UINT8 *data,UINT16 len);
void  at_in(UINT8 *rsp,UINT16 rsplen);
void  sig_in(GAPP_SIGNAL_ID_T sig,va_list arg);
void  at_handle(INT8 *data,UINT16 data_len);
char * getSigStr(GAPP_SIGNAL_ID_T sig);

char imsi[16];
char imei [16];
int tick =9;
int csq=28;
char version[20];
unsigned char send_data[100];

SYS_CALLBACK_T sys_callback = 
{
	app_init,/*init function*/
	uart_in,/*uart input callback function*/
	at_in,/*at rsp callback function*/
	sig_in,/*system signal callback function*/
};

UINT32 TASK_ONE,TASK_TWO,TASK_THREE,TASK_FOUR,TASK_FIVE;

void  uart_in(INT32 uid,UINT8 *data,UINT16 len)
{
	UINT8 *d = (UINT8*)sys_malloc(len);

	if(d)
	{
		//log_info("111111111111111");
		memcpy(d,data,len);
		/*send uart data to TASK 1 */
		if(GAPP_RET_OK != sys_taskSend(TASK_ONE,T1_UART_DATA_RECV_IND,(UINT32)uid,(UINT32)d,(UINT32)len))
		{
		    sys_free(d);
		}
	}
}

void  at_in(UINT8 *rsp,UINT16 rsplen)
{
	UINT8 *d = (UINT8*)sys_malloc(rsplen+1);
	if(d)
	{
		memcpy(d,rsp,rsplen);
		d[rsplen] = '\0';
		//log_info("receive at order :%s", rsp);
		/*send data to TASK 2 , AT command respond*/
		if(GAPP_RET_OK != sys_taskSend(TASK_TWO,T2_EVENT_AT_RSP_IND,(UINT32)rsplen,(UINT32)d,0))
		{
		    sys_free(d);
		}
	}	
}

void  sig_in(GAPP_SIGNAL_ID_T sig,va_list arg)
{
    INT32 ret;

    //char *p = getSigStr(sig);
	//log_info("sig: %d, %s",sig, p);
    //sys_free(p);
    // log_info("sig: %d",sig);
    /*
    GAPP_SIG_PDP_RELEASE_IND == 0,
    GAPP_SIG_SOCK_CONNECT_RSP ==1,
    GAPP_SIG_SOCK_TCPXON_IND ==2,
    GAPP_SIG_SOCK_CLOSE_IND ==3,
    GAPP_SIG_SOCK_ERROR_IND ==4,
    GAPP_SIG_SOCK_ACCEPT_IND ==5,
    GAPP_SIG_SOCK_CLOSE_WITH_FIN_RSP ==6,
    GAPP_SIG_SOCK_SEND_RSP ==7,
    GAPP_SIG_SOCK_CLOSE_RSP ==8,
    GAPP_SIG_SOCK_DATA_RECV_IND ==9.
    */
	switch(sig)
	{
	    case GAPP_SIG_SOCK_CONNECT_RSP:/*tcp connect success event 1*/
	    {
	        UINT32 sock;
	        /*send msg to TASK 1 , we handle socket event there*/
	        sock = va_arg(arg,UINT32);
	        ret = sys_taskSend(TASK_ONE,T1_SOCK_CONNECT_RSP,sock,0,0);
	        va_end(arg);
	    }
	    break;
	    case GAPP_SIG_SOCK_ERROR_IND:/*socket error 4*/
	    {
	        UINT32 sock;
	        sock = va_arg(arg,UINT32);
	        /*send msg to TASK 1 , we handle socket event there*/
	        ret = sys_taskSend(TASK_ONE,T1_SOCK_ERROR_IND,sock,0,0);
	        va_end(arg);	        
	    }
	    break;
	    case GAPP_SIG_SOCK_DATA_RECV_IND:/*peer data recving 9*/
	    {
	        UINT32 sock,len;
	        sock = va_arg(arg,UINT32);
	        len  = va_arg(arg,UINT32);
         //    log_info("GAPP_SIG_SOCK_DATA_RECV_IND, sock: %d",sock);
         //    log_info("task sleep");
	        // log_info("ret == %d",ret = sys_taskSend(TASK_ONE,T1_SOCK_DATA_RECV_IND,sock,len,0));
	        // log_info("Send SOCK TO TASK_ONE");
	        //sys_taskSend(TASK_ONE,T1_SOCK_DATA_RECV_IND,sock,len,0);
	        sys_taskSend(TASK_ONE,T1_SOCK_DATA_RECV_IND,sock,len,0);
	        va_end(arg);	        
	    }
	    break;
	    case GAPP_SIG_SOCK_CLOSE_IND:/*tcp peer close , maybe recv FIN 3*/
	    {
	        UINT32 sock;
	        sock = va_arg(arg,UINT32);
            log_info("GAPP_SIG_SOCK_CLOSE_IND, sock: %d",sock);
	         /*send msg to TASK 1 , we handle socket event there*/
	        ret = sys_taskSend(TASK_ONE,T1_SOCK_CLOSE_IND,sock,0,0);
	        va_end(arg);	    
	    }
	    break;
	    case GAPP_SIG_SOCK_CLOSE_RSP:/*close respond 8*/
	    {
	        UINT32 sock;
	        sock = va_arg(arg,UINT32);
	         /*send msg to TASK 1 , we handle socket event there*/
	        ret = sys_taskSend(TASK_ONE,T1_SOCK_CLOSE_RSP,sock,0,0);
	        va_end(arg);	    
	    }
	    break;

	    default:
	    break;
	}
}

void gapp1_sencond_timer_handler(void *arg)
{
	//log_info("Begin GAPP1_SENCOND_TIMER_HANDLER");
	sys_taskSend(TASK_ONE,T1_SECOND_HANDLE_IND ,NULL,NULL,NULL);
	sys_watchdog_feed();	
	//log_info("=================================================================================feed watchdog ok");
}
void gapp1_ping_hanlder(void *arg)
{
	log_info("Begin GAPP1_PING_HANDLER");
    sys_taskSend(TASK_THREE, T3_MQTT_PING_IND, 0, 0, 0); 
}

void gapp1_softreset_hanlder(void *arg)
{
	log_info("Begin GAPP1_SOFTRESET_HANDLER");
    sys_taskSend(TASK_ONE, T1_SOFT_RESET, 0, 0, 0);
}

void app1(UINT32 id,UINT32 n1,UINT32 n2,UINT32 n3)
{

    DB(DB1,"[app1] id[%u] n1[%u] n2[%u] n3[%u]",id,n1,n2,n3);
	char imei[16], imsi[16];
	// log_info("APP1 id == %d ",id);
	switch(id)
	{
		case GAPP_TASK_INITED:
		{
			TASK_ONE = n1;//save task id
			log_info("APP1 Start Task ID == %d",TASK_ONE);
			IOT_OpenLog("mqtt");
    		IOT_SetLogLevel(IOT_LOG_INFO);
            //IOT_SetLogLevel(IOT_LOG_DEBUG);
			sys_hookUart(0,1);
			log_info("Ali_MQTT_Demo, SW_Version:%s,HW_Version:%s", VERSION,Ali_MQTT_HW_VERSION);

			mqtt_ping_time = 15;
			hdw_motor_enable_flg = 1;// 上电使能按摩抱枕
			hdw_dev_initial(hdw_oper);// 设备初始化
			if(hdw_start_up_flg == 1)//判断是否初始化成功
			{
				sys_timer_new(100, gapp1_sencond_timer_handler, NULL);// 成功进入 MQTT 事件
			}
			else
			{
				sys_timer_new(500, hdw_dev_init_timer_hanlder, NULL);// 不成功 进去tcp 注册
			}
		}
		break;
		case T1_UART_DATA_RECV_IND:/*uart data recv event*/
		{
			if(0 == n1)
			{
				sys_uart_output(0,(UINT8*)n2,(UINT16)n3);/*uart echo*/
				//hdw_test_self(n2);
				cmd_proccess((INT8*)n2,(UINT16)n3);/*proccess uart data*/
				sys_free((void*)n2);/*free memory*/
			}
		}
		break;
		case T1_SOCK_CONNECT_RSP:
		{
            log_info("connect ok ~~~~~~~~~~");
			if ((INT32)n1==gDataSOC.SocID)
			{
				gDataSOC.SocStatus = GHT_SOC_SOCKET_CONNECTED;
			}
            cmd_sock_connect_handle(n1,n2,n3);/*connect success */
		}
		break;
		case T1_SOCK_ERROR_IND:
		{
		    cmd_sock_error_handle(n1,n2,n3);/*socket error , so me must close the socket*/
		}
		break;
		case T1_SOCK_DATA_RECV_IND:
		{
			// log_info("Socket Data Recv ");
			// log_info("hdw_start_up_flg ==  %d",hdw_start_up_flg);
			if (hdw_start_up_flg == 0)
				Event_DataSockRecv(n1,n2,n3);
			/*if ((INT32)n1==gDataSOC.SocID && hdw_start_up_flg == 0)
			{
				Event_DataSockRecv(n1,n2,n3);
                log_info("Event_DataSockRecv");
			}*/
			else
			{
				cmd_sock_data_recv_handle(n1,n2,n3);

			}
		}
		break;
		case T1_SOCK_CLOSE_IND:
		{
			log_info("Socket Close ");
			if ((INT32)n1==gDataSOC.SocID && hdw_start_up_flg == 0)
			{
				gDataSOC.PDP_IP=0;
				gDataSOC.SocStatus=PPP_DWON;
			}
		    cmd_sock_close_ind_handle(n1,n2,n3);/*peer close , recv FIN*/
		}
		break;
		case T1_SOCK_CLOSE_RSP:
		{
			if ((INT32)n1==gDataSOC.SocID && hdw_start_up_flg == 0)
			{
				log_info("Close Socket Response time == %d",sys_getSysTick() / 16384);
				gDataSOC.PDP_IP=0;
				gDataSOC.SocStatus=PPP_DWON;
			}
		    cmd_sock_close_rsp_handle(n1,n2,n3);/*close resp*/
		}
		break;
		case T1_SECOND_HANDLE_IND:
		{
			// log_info("Second Handle And hdw_start_up_flg == %d",hdw_start_up_flg);

			if(hdw_start_up_flg == 1)
			{
				SecondCnt++;
			   	//Network_Tick_Form_TCP();
				Network_Tick();
				if(GprsActived==1)
				{
                    gRunParam.GprsStatus = GPRS_ACTIVED;

                    if(gRunParam.UpdateStartLink == 1)
                    {
                    	//log_info("Receive Update's Dates ");
                        soft_update_use_http();
                        //log_info("Update Success");
                    }
					CloudTick();
				}
                else
                {
                	//Network_Tick();
                    gRunParam.GprsStatus = GPRS_DEACTIVE;
                    //gRunParam.GprsRegisted
                }
				sys_timer_new(HDW_MQTT_TICK_TIME , gapp1_sencond_timer_handler, NULL);// 滴答定时器，相应ping时间有HDW_MQTT_TICK_TIME ms基数计算
			}
			else
			{
				//Network_Tick();
				//添加TCP启动
			}
		}
		case T3_MQTT_PING_IND:
			//try ping?
			break;
		case Feed_Watch_dog:
		{
			sys_timer_new(HDW_MQTT_TICK_TIME , gapp1_sencond_timer_handler, NULL);// 滴答定时器，相应ping时间有HDW_MQTT_TICK_TIME ms基数计算
		}
		break;
		case T1_MQTT_START_UP:// 设备注册成功以后，进行LPG常亮指示
		{
			log_info("hardware is ready,LPG is lighting\n\n");
			LPG_init();
			LPG_TAsK();
			sys_softReset();
		}
		break;
        case T1_SOFT_RESET:
        {
            log_info("T1_SOFT_RESET,g_device_working == %d",g_device_working);
            if(g_device_working == 0)
            {
                //sys_taskSleep(3000);
                log_info("Because g_device_working == 0,sys_softReset");
                sys_taskSleep(1000);
                sys_softReset();
            }
            else
            {
            	log_info("Device Working ,Please Waiting");
            	sys_timer_new(10*1000,gapp1_softreset_hanlder,NULL);
            }
            //sys_timer_new(61*1000 , gapp1_softreset_hanlder, NULL);
            
        }
            break;
        case T1_START_MQTT_FROM_TCP:
        {
         	log_info("Start Mqtt From TCP");
         	hdw_start_up_flg = 1;
         	hdw_dev_initial(hdw_oper);
         	sys_timer_new(100, gapp1_sencond_timer_handler, NULL);
         	GprsActived = 1;
         	/*hdw_start_up_flg = 1;
         	Network_Tick_Form_TCP();
         	if(GprsActived==1)
			{
				log_info("");
                gRunParam.GprsStatus = GPRS_ACTIVED;

                if(gRunParam.UpdateStartLink == 1)
                    soft_update_use_http();

         		log_info("Start MQTT From TCP");
				CloudTick();
				}
                else
                {
                    gRunParam.GprsStatus = GPRS_DEACTIVE;
                    //gRunParam.GprsRegisted
                }*/
        }
         	/*hdw_start_up_flg = 1;
         	//tcp_socket_flag = 1;
         	GprsAttched = 1;
         	GprsActived = 1;
         	sys_timer_new(1000, gapp1_sencond_timer_handler, NULL);*/

        	break;
		default:
		{
		}
		break;
	}

}

void app2(UINT32 id,UINT32 n1,UINT32 n2,UINT32 n3)
{
    DB(DB1,"[app2] id[%u] n1[%u] n2[%u] n3[%u]",id,n1,n2,n3);
	switch(id)
	{
		case GAPP_TASK_INITED:
		{
			TASK_TWO = n1;//save task id
		}
		break;
			
		case T2_EVENT_AT_RSP_IND:
		{
				at_handle((INT8*)n2,(UINT16)n1);/*handle at respond*/
                sys_free((void*)n2);
		}
		break;
		case T2_EVENT_UART_SEND_IND:
		{

		}
		break;		
		default:
		{
		}
		break;
	}	
}

void gapp3_ota_hanlder(void *arg)
{
    sys_taskSend(TASK_THREE, T3_OTA_IND, 0, 0, 0);
}

void app3(UINT32 id,UINT32 n1,UINT32 n2,UINT32 n3)
{
    DB(DB1,"[app3] id[%u] n1[%u] n2[%u] n3[%u]",id,n1,n2,n3);
	switch(id)
	{
		case GAPP_TASK_INITED:
		{
			TASK_THREE = n1;//save task id
            init_sending_command(NULL);/*init*/
            GPRS_InitSet();/*configure */
			/*if (AT_GetSerialNumber(hdw_imei) == 0)
			{
				log_info("hdw_imzi : %s",hdw_imei);
			}
			else
			{
				log_info("get imei wrong");
			}*/
		}
		break;
		case T3_CSQ_TIMER_IND:
		{
			if(hdw_start_up_flg == 0 )// 判断注册不成功，开始进行TCP链接 进行注册
			{
				
				TCPlistenProc();
				//sys_timer_new(2000,hdw_dev_init_timer_hanlder,NULL);
				sys_timer_new(1000,hdw_dev_init_timer_hanlder,NULL);
			}
		}
		break;
		case T3_MOTOR_HANDLE_IND:
		{
            log_info("HDW_MOTOR_OFF");
			HDW_MOTOR_OFF;
			GPIO_SET_5 ;
			g_device_working = 0;
            //hdw_send_stop_order_toserver();
		}
		break;
        case T3_OTA_IND:
        {
            log_info("T3_OTA_IND");
            // if( (SecondCnt>60) &&  (g_device_working==0) &&  (gRunParam.UpdateStartLink != 1) )
			//if( g_device_working ==0 && gRunParam.UpdateStartLink !=1 ) //修改升級時間，取消前60+61等待時間
			if(gRunParam.UpdateStartLink !=1)//修改工作中升級
            {
                char *host_file = NULL;
                char *host_addr = NULL;
                int port = 0;

                http_gethost_info(ota_url, &host_addr, &host_file, &port);
                log_info("host_addr is: %s\n ", host_addr);
                log_info("host_file is: %s\n ", host_file);
                log_info("port is: %d\n ", port);
                if (host_file == NULL || host_addr == NULL) {
                    log_info("host_file==null");
                    return ;
                }

                strcpy(gRunParam.UDServer, host_addr);
                gRunParam.UDPort = port;
                strcpy(gRunParam.UDURL, host_file);

                setUDSocketStatus(TCP_IDLE);
                gRunParam.UpdateStartLink = 1;
            } else
            {
                log_info("T3_OTA_IND, waiting ota...");
                // sys_timer_new(61*1000 , gapp3_ota_hanlder, NULL);
				sys_timer_new(20*1000,gapp3_ota_hanlder,NULL);
            }

        }
            break;
		default:
		{
		}
		break;
	}	
}

void app4(UINT32 id,UINT32 n1,UINT32 n2,UINT32 n3)
{
	switch(id)
	{
		case GAPP_TASK_INITED:
		{
			TASK_FOUR = n1;//save task id
		}
		break;
		case T4_PPP_ACTIVE:
		{
			ght_active_ppp_proc();
		}
		break;
		case T4_PPP_DEACTIVE:
		{
			ght_deactive_ppp_proc();
		}
		break;
		case T4_PPP_SENDDATA:
		{
			//sys_sock_send(gDataSOC.SocID,"welcome to fibocom!!",25);
		}
		break;
		case T4_UART_DATA_RECV_IND:/*uart data recv event*/
		{   
			if(0 == n1)
			{
				sys_uart_output(0,(UINT8*)n2,(UINT16)n3);/*uart echo*/
				cmd_proccess((INT8*)n2,(UINT16)n3);/*proccess uart data*/
				sys_free((void*)n2);/*free memory*/	
			}
		}
		break;
		default:
		{
		}
		break;
	}	
}
///char *s = "{\"OnLineCount\":\"57\",\"imsi\":\"112741294\",\"errorCode\":\"200\",\"errorInfo\":\"Success\"}";
extern err_flag ;
void app5(UINT32 id,UINT32 n1,UINT32 n2,UINT32 n3)
{
	
	switch(id)
	{
		case GAPP_TASK_INITED:
		{
			TASK_FIVE = n1;//save task id
		}
		break;
		case T5_PPP_SENDDATA:
		{
			if(hdw_start_up_flg == 0)// 
			{
				hdw_send_register_data(send_data);// 向TCP  服务器发送注册设备信息 IMEI
				log_info("jsonnnnnnnnnnnn =%s",send_data);
				sys_sock_send(gDataSOC.SocID, send_data, 200);
			}	
		}
		break;
		case T5_HDW_REGISTER_TOPIC_FIRSTDATA:
		{
			if (sub_rrpc_topic_suss_flag == 1)
			{
				// log_info("1111--------------------------------------------------------------------------");
				sub_rrpc_topic_suss_flag = 0;
				hdw_data_topic_register();// 发送topic  data 注册信息
			}
			if (sub_data_topic_suss_flag == 1)// 发送上电初始化 注册信息
			{
				// log_info("2222--------------------------------------------------------------------------");
				sub_data_topic_suss_flag = 0;
				hdw_publish_send_init_data();
			}	
		}
		break;
		case T5_HDW_SEND_DATA_TOSERVER:
			{
				hdw_send_stop_order_toserver();// 按摩抱枕停止，发送stop  json 串
				//log_info("send motor stop to mqtt and err_flag == %d",err_flag);
				if(err_flag == 1)
					sys_taskSend(TASK_ONE,T1_SOFT_RESET,0,0,0);
				// sys_taskSleep(2000);
			}
			break;
		case T5_HDW_HANDLE_ABNORMAL:
			if (sub_rrpc_topic_fail_flag == 1 )
			{
				sub_rrpc_topic_fail_flag = 0;
				//hdw_response_topic_register();// response topic 没注册成功，重新注册
			}
			if (sub_data_topic_fail_flag == 1)// data topic 没注册成功，重新注册
			{
				sub_data_topic_fail_flag = 0;
				hdw_data_topic_register();
			}
		break;
		default:
		{
		}
		break;
	}	
}


void setTcpSocketStatus(TCP_STATUS_T Status)
{
    gRunParam.TcpStatus = Status;
}

GAPP_TASK_T app_task[] =
{
	{
		"cmd task",/*task name*/
		GAPP_THREAD_PRIO_0,/*priority*/
		4096,/*stack size*/
		app1/*msg handle function*/
	},
	{
		"at task",/*task name*/
		GAPP_THREAD_PRIO_1,/*priority*/
		4096,/*stack size*/
		app2/*msg handle function*/
	},
	{
		"net task",/*task name*/
		GAPP_THREAD_PRIO_2,/*priority*/
		4096,/*stack size*/
		app3/*msg handle function*/
	},
	{
		"app4",/*task name*/
		GAPP_THREAD_PRIO_3,/*priority*/
		4096,/*stack size*/
		app4/*msg handle function*/
	},
	{
		"app5",/*task name*/
		GAPP_THREAD_PRIO_4,/*priority*/
		4096,/*stack size*/
		app5/*msg handle function*/
	}
};

static UINT8 app_init(GAPP_TASK_T **tl)
{
	*tl = app_task;/*so we can got the task list at the modem side*/
	return sizeof(app_task)/sizeof(GAPP_TASK_T);/*must return task list size*/
}

char * getSigStr(GAPP_SIGNAL_ID_T sig)
{
    char *s = sys_malloc(40);
    switch(sig)
    {
        case GAPP_SIG_PDP_RELEASE_IND:
        {
            s = "GAPP_SIG_PDP_RELEASE_IND";
        }
            break;
        case GAPP_SIG_SOCK_CONNECT_RSP:
        {
            s = "GAPP_SIG_SOCK_CONNECT_RSP";
        }
            break;
        case GAPP_SIG_SOCK_TCPXON_IND:
        {
            s = "GAPP_SIG_SOCK_TCPXON_IND";
        }
            break;
        case GAPP_SIG_SOCK_CLOSE_IND:
        {
            s = "GAPP_SIG_SOCK_CLOSE_IND";
        }
            break;
        case GAPP_SIG_SOCK_ERROR_IND:
        {
            s = "GAPP_SIG_SOCK_ERROR_IND";
        }
            break;
        case GAPP_SIG_SOCK_ACCEPT_IND:
        {
            s = "GAPP_SIG_SOCK_ACCEPT_IND";
        }
            break;
        case GAPP_SIG_SOCK_CLOSE_WITH_FIN_RSP:
        {
            s = "GAPP_SIG_SOCK_CLOSE_WITH_FIN_RSP";
        }
            break;
        case GAPP_SIG_SOCK_SEND_RSP:
        {
            s = "GAPP_SIG_SOCK_SEND_RSP";
        }
            break;
        case GAPP_SIG_SOCK_CLOSE_RSP:
        {
            s = "GAPP_SIG_SOCK_CLOSE_RSP";
        }
            break;
        case GAPP_SIG_SOCK_DATA_RECV_IND:
        {
            s = "GAPP_SIG_SOCK_DATA_RECV_IND";
        }
            break;
        default:
            break;
    }
    return s;

}

