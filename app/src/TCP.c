#include "sys_callback.h"
#include "sys_services.h"
#include "sys_ext.h"
#include "string.h"
#include "cmd.h"
#include "app.h"
#include "debug.h"
#include "cmddef.h"
#include "manage.h"
#include "stdio.h"
#include "TCP.h"
#include "hardware.h"



/*
    app_Stateinit=0,
    app_ATcommandTest=1,
	app_CSQCheck=2,
	app_connectNetwork=3,
	app_waitProc=4,
	app_getapn = 5,
*/
void TCPlistenProc(void)
{
	log_info("sys_getState()=%d", sys_getState());
	switch(sys_getState())
	{
		case app_Stateinit:
		{
			ght_initParam();
			ght_atCommand();
			ght_get_firmware();
			sys_setState(app_CSQCheck);
		}
		/*case app_Stateinit:
		{
			ght_initParam();
			sys_setState(app_ATcommandTest);
		}*/
			break;
		case app_ATcommandTest:
		{
			ATcommandTest();
		}
			break;
		case app_CSQCheck:
		{
			ght_CSQcheck();
		}
			break;
		case app_getapn:
		{
			getapn();
		}
		case app_connectNetwork:
		{

			TCP_connect();
		}
			break;
		case app_waitProc:
		{

		}
			break;
		default:
			break;
	}
}

void ght_initParam(void)
{
	memset(&ght_appState,0,sizeof(ght_appState));
	memset(&gDataSOC,0,sizeof(gDataSOC));
	memset(&gServersIP,0,sizeof(gServersIP));
	gDataSOC.SocID=0xff;

}

void ATcommandTest(void)
{
	switch (at_getState())
	{
		case AT_COMMAND_AT:
		{
			ght_atCommand();
			at_setState(AT_COMMAND_VER);
		}
			break;
		case AT_COMMAND_VER:
		{
			ght_get_firmware();
			sys_setState(app_CSQCheck);
		}
			break;
		default:
		{
			at_setState(AT_COMMAND_AT);
		}
			break;
	}
}

void ght_atCommand(void)
{
	sys_at_send("at\r",3);
}

INT32 ght_get_firmware(void)
{
	INT32 ret;

	ret = sys_get(GAPP_OPT_SYS_VERSION_ID,&ght_appState.firmware,sizeof(ght_appState.firmware));
	log_info("firmware version:%s",ght_appState.firmware.sys_ver);
	return ret;
}

void ght_CSQcheck(void)
{
	//INT32 ret=0;
	INT32 csq;
	INT32 cgreg_state=0, creg_state=0;

	cgreg_state=AT_GetGRegister();
	log_info("cgreg_state == %d",cgreg_state);
	creg_state=AT_GetRegister();
	log_info("creg_state == %d",creg_state);
	if (creg_state<0)
	{
		log_info("Sys:creg 0");
	}
	else
	{
		log_info("Sys:creg %d",cgreg_state);
	}
	if (cgreg_state<0)
	{
		log_info("Sys:cgreg 0");
	}
	else
	{
		log_info("Sys:cgreg %d",cgreg_state);
	}
	csq = AT_GetSignal();
	if (csq<0)
	{
		log_info("Sys:csq 0,return");
		return;
	}
	else
	{
		log_info("Sys:csq %d",csq);
	}
	//GPRS
	if (((cgreg_state!=1) && (cgreg_state!=5))/*||(!gServersIP.sin_addr.addr)*/)
	{
		ght_appState.nogprs_Time++;
		if (ght_appState.nogprs_Time>60)
		{
			log_info("Because ght_appState.nogprs_Time>60 sys_softReset");
			sys_taskSend(TASK_ONE,T1_SOFT_RESET,0,0,0);
			//sys_softReset();
		}
		log_info("Sys:No GPRS!");
		sys_setState(app_CSQCheck);
		return ;
	}
	else
	{
		sys_setState(app_connectNetwork);
	}
}

void getapn(void)
{
	
	if (RET_FAILED == GetAPNFromCarrier()) 
	{
        log_err("Get APN error!!!check network And Sim card!!"); 
        sys_setState(app_getapn);
        return;
    } 
    log_info("GetAPNOK");
    sys_setState(app_connectNetwork);
}

void TCP_connect(void)
{
	INT32 ret=0;
	UINT8 *p=NULL;
	log_info("gDataSOC.SocStatus ====== %d",gDataSOC.SocStatus);
	
	if ((gDataSOC.SocStatus==PPP_DWON)||(gDataSOC.SocStatus==GHT_SOC_UNFIT))
	{
		log_info("Sys:CGREG state is ok!!!");
		if(gDataSOC.SocID!=0xff)
		{

			if(sys_sock_close(gDataSOC.SocID)>=0)
			{
				gDataSOC.SocID=0xff;
			}
		}

		if (gDataSOC.pdpactive_count>20)
		{
			gDataSOC.pdpactive_count=0;
			log_info("T4_PPP_DEACTIVE");
			sys_taskSend(TASK_FOUR,T4_PPP_DEACTIVE,0,0,0);
		}
		/*if (gDataSOC.pdpactive_count>2)
		{
			gDataSOC.pdpactive_count=0;
			log_info("T4_PPP_DEACTIVE");
			sys_taskSend(TASK_FOUR,T4_PPP_DEACTIVE,0,0,0);
		}*/
		else
		{
			gDataSOC.pdpactive_count++;
			log_info("T4_PPP_ACTIVE");
			sys_taskSend(TASK_FOUR,T4_PPP_ACTIVE,0,0,0);
		}
		return ;
	}
	else if(gDataSOC.SocStatus == PPP_HOLD)
	{
		log_info("Sys : Now State PPP_HOLD");
	}
	else if(gDataSOC.SocStatus==PPP_UP)
	{
		log_info("Sys:now state: PPP_UP");

		ret=sys_sock_create(gDataSOC.Servers_type);
		if (ret<0)
		{
			gDataSOC.Create_sock_count++;
			if(gDataSOC.Create_sock_count>3)
			{
				gDataSOC.Create_sock_count=0;
				// gDataSOC.SocStatus=PPP_DWON;
				sys_taskSend(TASK_FOUR,T4_PPP_DEACTIVE,0,0,0);
			}
			gDataSOC.SocID=0xff;
		}
		else
		{
			gDataSOC.SocID=ret;
			log_info("gDataSOC.SocID=%d",gDataSOC.SocID);
			gDataSOC.SocStatus=GHT_SOC_IDLE;
		}
	}

#if 1
	else if (gDataSOC.SocStatus==GHT_SOC_IDLE)
	{
		log_info("try to connect server..");

		ret = sys_getHostByName(ServerIP, &gServersIP.sin_addr);
		log_info("End sys_getHostByName And ret == %d",ret);
		gServersIP.sin_port= htons(PROTO_PORT);  //fixed port 9080

		ret=sys_sock_connect(gDataSOC.SocID, &gServersIP);
		if (ret<0)
		{
			log_info("connect server error..");
			gDataSOC.connect_count++;
			if(gDataSOC.connect_count>3)
			{
				gDataSOC.connect_count=0;
				if(gDataSOC.SocID!=0xff)
				{
					if(sys_sock_close(gDataSOC.SocID)>=0)
					{
						gDataSOC.SocID=0xff;
					}
				}
				sys_taskSend(TASK_FOUR,T4_PPP_DEACTIVE,0,0,0);
				//gDataSOC.SocStatus=PPP_DWON;
			}
			gDataSOC.SocStatus=GHT_SOC_IDLE;

		}
		else
		{
			p=(UINT8 *)&gServersIP.sin_addr;
			log_info("Sys:server Ip (%d.%d.%d.%d,%d) successfully!",p[0],p[1],p[2],p[3],(gServersIP.sin_port&0xff)*0x100+((gServersIP.sin_port>>8)&0xff));
			gDataSOC.SocStatus=GHT_SOC_SOCKET_CONNECT;
		}
	}
	else if((gDataSOC.SocStatus==GHT_SOC_SOCKET_CONNECT)&&(gDataSOC.Servers_type==PROTO_TCP))
	{
		gDataSOC.timeConnect++;
		if(gDataSOC.timeConnect==3)
		{
			gDataSOC.timeConnect=0;
			if(gDataSOC.SocID!=0xff)
			{

				if(sys_sock_close(gDataSOC.SocID)>=0)
				{
					gDataSOC.SocID=0xff;
				}
				sys_taskSend(TASK_FOUR,T4_PPP_DEACTIVE,0,0,0);
			}
		}
	}
	else if (gDataSOC.SocStatus==GHT_SOC_SOCKET_CONNECTED)
	{
		//log_info("send data to server:welcome to fibocom!!");
		//sys_taskSend(TASK_FOUR,T4_PPP_SENDDATA,0,0,0);
		//sys_sock_send(gDataSOC.NewSocID,"welcome to fibocom!!",25);
		gDataSOC.SocStatus=GHT_SOC_SOCKET_OK;
	}
	else if (gDataSOC.SocStatus==GHT_SOC_SOCKET_OK)
	{

		log_info("send data to server:welcome to fibocom!!");
		sys_taskSend(TASK_FIVE,T5_PPP_SENDDATA,0,0,0);
		gDataSOC.SocStatus=GHT_SOC_SOCKET_CONNECTED;

	}
	else if (gDataSOC.SocStatus==GHT_SOC_PDPWait)
	{

		log_info("wait for reactive pdp");
		gDataSOC.SocStatus=PPP_DWON;
	}
	else if (gDataSOC.SocStatus==GHT_SOC_Close)
	{
		ght_socket_close();
	}
	else if (gDataSOC.SocStatus==GHT_SOC_DeActivePDP)
	{
		sys_taskSend(TASK_FOUR,T4_PPP_DEACTIVE,0,0,0);
	}

#endif
	return;

}


/*INT32 ght_active_ppp_proc(void)
{
	INT32 ret= 0;
	log_info("TCP Begin PDPActive");
	ret = sys_PDPActive((INT8 *)"cmnet" , NULL, NULL, &(gDataSOC.PDP_IP));
	log_info("End TCP PDPActive And ret == %d",ret);
}
*/

INT32 ght_active_ppp_proc(void)
{
	INT32 ret=0;
	U8 *Ibox_ip;
	static ac_err_cnt = 0;

	if (gDataSOC.SocStatus!=PPP_UP)
	{
		log_info("TCP Begin PDPActive");
		ret = sys_PDPActive((INT8 *)"cmnet" , NULL, NULL, &(gDataSOC.PDP_IP));
		log_info("End TCP PDPActive And ret == %d",ret);
	}
	if((ret==0)&&(gDataSOC.PDP_IP))
	{
		ac_err_cnt = 0;
		gDataSOC.SocStatus=PPP_UP;
		Ibox_ip = (UINT8 *)&(gDataSOC.PDP_IP);
		log_info("Sys:Active_ppp OK!");
		log_info("Sys:(host IP) %d.%d.%d.%d",Ibox_ip[0],Ibox_ip[1],Ibox_ip[2],Ibox_ip[3]);
	}
	else
	{
		log_info("Sys:Active_ppp error");

		if (gDataSOC.SocStatus!=GHT_SOC_UNFIT)
		{
			gDataSOC.SocStatus=PPP_DWON;
		}
		gDataSOC.PDP_IP=0;
		ac_err_cnt++;
		if(ac_err_cnt > 30)
		{
			ac_err_cnt = 0;
			sys_softReset();
		}
	}
	return ret;
}

INT32 ght_deactive_ppp_proc(void)
{
	INT32 ret=0;
	log_info("Begin sys_PDPRelease ");
	ret = sys_PDPRelease();
	log_info("End  sys_PDPReleaseAnd ret == %d",ret);
	if (0==ret)
	{
		//if (gDataSOC.SocStatus!=GHT_SOC_UNFIT)
		//gDataSOC.SocStatus=PPP_DWON;
		//sys_setState(app_CSQCheck);
		gDataSOC.PDP_IP=0;
		gDataSOC.SocStatus=GHT_SOC_PDPWait;
	}
	else
	{
		//if (gDataSOC.SocStatus!=GHT_SOC_UNFIT)
		//{
		//gDataSOC.SocStatus=PPP_DWON;
		//}
		gDataSOC.PDP_IP=0;
		gDataSOC.SocStatus=GHT_SOC_PDPWait;

		//sys_setState(app_CSQCheck);
	}
	return ret;
}

/* @Change SocStatus 
 *
 */

INT32 ght_socket_close(void)
{

	if (gDataSOC.SocID!=0xff)
	{
		int ret;
		ret=sys_sock_close(gDataSOC.SocID);
		if(ret<0)
		{
			log_info("sys_sock_close(gDataSOC.SocID) error");
			gDataSOC.SocStatus=GHT_SOC_Close;
		}
		else
		{
			log_info("sys_sock_close(gDataSOC.SocID) ok %d",gDataSOC.SocID);
			gDataSOC.SocID=0xff;
			//gDataSOC.SocStatus=PPP_UP;修改状态
			gDataSOC.SocStatus = PPP_HOLD;

		}

	}
	return 0;
}
INT32 Event_DataSockSend(UINT8 *buf,UINT16 datalen)
{
	INT32 ret=0;
	ret=sys_sock_send(gDataSOC.SocID,buf,datalen);
	if (ret>=0)
	{
		return ret;
	}
	else
	{
		log_info("Sys:send data error!!!ret=%d",ret);
		return ret;
	}
}

unsigned char flg;
extern UINT32 SecondCnt;

void Event_DataSockRecv(UINT32 n1,UINT32 n2,UINT32 n3)
{
	//log_info("")
	INT32 ret;
	char *key = "SvrIP";
	char json[100];
	if ((INT32)n1==gDataSOC.SocID)
	{
		//log_info("Sys:RecvDatasocket %d read %d %d !!",gDataSOC.SocID,(INT32)n2,(INT32)n3);
		memset(gDataSOC.Recebuff, 0, sizeof(gDataSOC.Recebuff));
		ret = sys_sock_recv(gDataSOC.SocID, gDataSOC.Recebuff, 1300);
		if (ret < 0)
		{
			return;
		}
		else
		{
			log_debug("json receive from server :%s\n", gDataSOC.Recebuff);
			if (strlen(gDataSOC.Recebuff) >11)
			{
				log_info("");
				log_info("Handle ALI's Elements Start Time  %d\n",sys_getSysTick() / 16384);
				ght_socket_close();
				hdw_start_up_flg =1;
				//gDataSOC.SocStatus = GHT_SOC_Close;
				hdw_ini_register_json_parser(gDataSOC.Recebuff, json);// 分析从tcp 服务器获取的 json 设备信息
				//hdw_data_write_file(json, HDW_DEV_INIT_FILE);// 将设备信息整理存储到flash
				WriteDevParamToFlash(1);
				log_info("WriteDevParamToFlash will reset!");
				//ght_socket_close();
				//tcp_flag = 1;
				cmd_aliconn(NULL);
				UINT32 MQTT_FROM_TCP_TIME = sys_getSysTick() / 16384;;
				log_info("");
				log_info("MQTT_FROM_TCP_TIME == %d\n\r",MQTT_FROM_TCP_TIME);
				hdw_start_up_flg =1;
				//ght_deactive_ppp_proc();
				sys_taskSend(TASK_ONE,T1_START_MQTT_FROM_TCP,0,0,0);
				//sys_taskSleep(3000);
				//sys_softReset();

			}
		}
	}
}


//sys state
void sys_setState(int state)
{
	ght_appState.sys_state=state;
}
int sys_getState(void)
{
	return ght_appState.sys_state;
}

//at state
void at_setState(int state)
{
	ght_appState.at_state=state;
}
int at_getState(void)
{
	return ght_appState.at_state;
}
