#ifndef _TCP_H_
#define _TCP_H_

#define ServerIP  "pdt.ccn520.com"
#define PROTO_TCP 0
#define PROTO_PORT 5520

typedef struct { 
   
  int sys_state;
  int at_state;
  GAPP_OPT_SYS_VERSION_T firmware;
  int nogprs_Time;
  
  
} APPSTATE;

APPSTATE ght_appState;

typedef enum
{
    app_Stateinit=0,
    app_ATcommandTest=1,
	app_CSQCheck=2,
	app_connectNetwork=3,
	app_waitProc=4,
	app_getapn = 5,
		
}SYS_STATE;

typedef enum
{
    AT_COMMAND_AT=0,
	AT_COMMAND_VER=1,
	
}AT_STATE;


//socket status
typedef enum{
	PPP_DWON=0,
	PPP_UP=1,
	GHT_SOC_IDLE=2,
	GHT_SOC_SOCKET_CONNECT=3,
	GHT_SOC_SOCKET_CONNECTED=4,
	GHT_SOC_SOCKET_OK=5,
	GHT_SOC_SOCKET_CLOSE=6,
	GHT_SOC_MAX=7,
	GHT_SOC_UNFIT=8,
	GHT_SOC_PDPWait=9,
	GHT_SOC_Close=10,
	GHT_SOC_DeActivePDP=11,
	PPP_HOLD=12,
	
}GHT_SOCKET_STATE;


typedef struct{
	int five_time;
	UINT32 PDP_IP;
	int    timeConnect;
	int    pdpactive_count;
	int    connect_count;
	int    Create_sock_count;
	UINT32 Servers_type;
	UINT32 SocID;			//sock idid
	UINT32 SocStatus;		//sock status
	UINT32 DataLen;
	UINT32 SerialNum;		
//	UINT8 Sendbuff[1024];	
	UINT8 Recebuff[1400];	
}IboxDataSOCGroup;
IboxDataSOCGroup gDataSOC;
GAPP_TCPIP_ADDR_T gServersIP;


void ght_initParam(void);
void TCPlistenProc(void);
void ATcommandTest(void);
void ght_atCommand(void);
INT32 ght_get_firmware(void);
void ght_CSQcheck(void);
void TCP_connect(void);
INT32 ght_active_ppp_proc(void);
INT32 ght_deactive_ppp_proc(void);
INT32 Event_DataSockSend(UINT8 *buf,UINT16 datalen);
void Event_DataSockRecv(UINT32 n1,UINT32 n2,UINT32 n3);
INT32 ght_socket_close(void);

void sys_setState(int state);
int sys_getState(void);
void at_setState(int state);
int at_getState(void);
void getapn();

#endif
