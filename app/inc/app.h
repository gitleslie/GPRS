#ifndef _APP_H_
#define _APP_H_

#define Ali_MQTT_SW_VERSION "00000002_T1"
#define Ali_MQTT_HW_VERSION "00000001"
#define TEST "AT+TEST"

#define SEND_BUFF_LEN 1024
#define RECV_BUFF_LEN 2880
#define INPUT_GPS_LEN 250
#define UD_FILE_NAME "/sysapp/appchk.bin"
//#define UD_FILE_NAME     "ota.bin"
//每包数据的最大长度
#define SAVE_BUFF_LEN 265

typedef enum
{
    T1_EVENT_CAN_NOT_USE = GAPP_TASK_INITED,
    T1_UART_DATA_RECV_IND = 1, // uart data receive
    T1_SOCK_CONNECT_RSP = 2,
    T1_SOCK_ERROR_IND = 3,
    T1_SOCK_DATA_RECV_IND = 4,
    T1_SOCK_CLOSE_IND = 5,
    T1_SOCK_CLOSE_RSP = 6,
    T1_SECOND_HANDLE_IND = 7,
    T3_MQTT_PING_IND = 8,
    T1_MQTT_START_UP = 9,
    T1_SOFT_RESET = 10,
    T1_START_MQTT_FROM_TCP = 11,
    Feed_Watch_dog = 12,
} APP_TASK1_EVENT_T;

typedef enum
{
    T2_EVENT_CAN_NOT_USE = GAPP_TASK_INITED,
    T2_EVENT_AT_RSP_IND = 1,    // AT RESPONE
    T2_EVENT_UART_SEND_IND = 2, // AT RESPONE
} APP_TASK2_EVENT_T;

typedef enum
{
    T3_EVENT_CAN_NOT_USE = GAPP_TASK_INITED,
    T3_CSQ_TIMER_IND = 1,
    T3_MOTOR_HANDLE_IND = 2,
    T3_OTA_IND = 3,
} APP_TASK3_EVENT_T;

typedef enum
{
    T4_EVENT_CAN_NOT_USE = GAPP_TASK_INITED,
    T4_PPP_ACTIVE = 1,
    T4_PPP_DEACTIVE = 2,
    T4_PPP_SENDDATA = 3,
    T4_UART_DATA_RECV_IND = 4,
} APP_TASK4_EVENT_T;

typedef enum
{
    T5_EVENT_CAN_NOT_USE = GAPP_TASK_INITED,
    T5_PPP_SENDDATA = 1,
    T5_HDW_REGISTER_TOPIC_FIRSTDATA = 2,
    T5_HDW_SEND_DATA_TOSERVER = 3,
    T5_HDW_HANDLE_ABNORMAL = 4,
} APP_TASK5_EVENT_T;

typedef enum
{
    CMD_MODE = 0,
    DATA_MODE
} WORK_MODE_T;

typedef enum
{
    NW_UNREGISTER,
    NW_REGISTERED,
    GPRS_ACTIVED,
    GPRS_DEACTIVE,
    GPRS_DEACTIVED
} WORK_STATUS_T;

typedef enum
{
    TCP_IDLE,
    TCP_CONNECTTING,
    TCP_CONNECT,
    TCP_WAIT_ACK,
    TCP_SOCKET_CLOSE,
    TCP_SOCKET_CLOSING,
    TCP_SOCKET_CLOSED
} TCP_STATUS_T;

typedef enum
{
    SEND_STATUS_NONE,
    SEND_STATUS_LOGIN,
    SEND_STATUS_ALARM,
    SEND_STATUS_REPORT,
    SEND_STATUS_OLDGPS_REPORT,
    SEND_STATUS_HEARTBEAT,
    SEND_STATUS_UNREGISTER,
    SEND_STATUS_PARAMACK,
    SEND_STATUS_DEFAULTPARAMACK,
    SEND_STATUS_ACK,
    SEND_STATUS_LOCATION_ACK
} SEND_STATUS_T;

typedef enum
{
    DATA_TYPE_REGISTER,
    DATA_TYPE_AUTH,
    DATA_TYPE_REPORT,
    DATA_TYPE_OLDGPS_REPORT,
    DATA_TYPE_HEARTBEAT,
    DATA_TYPE_UNREGISTER,
    DATA_TYPE_PARAMACK,
    DATA_TYPE_DEFAULTPARAMACK,
    DATA_TYPE_ACK,
    DATA_TYPE_LOCATION_ACK
} DATA_TYPE_T;

typedef enum
{
    SYS_CONNECT_HTTP_SERVER,
    SYS_CONNECT_AUTH_SERVER,
    SYS_CONNECT_DATA_SERVER
} SYS_STATUS_T;

typedef struct SaveData_T
{
    UINT8 SaveDataBuff[SAVE_BUFF_LEN];
    UINT16 SaveDataLen;
    struct SaveData_T *next;
} SaveData_T;

typedef struct
{
    UINT8 SendDataBuff[SEND_BUFF_LEN];
    UINT16 SendDataLen;
} SendData_T;

typedef struct
{
    UINT8 RecvDataBuff[RECV_BUFF_LEN + 1]; //保存接收到的数据
    UINT16 RecvDataLen;                    //接收到的数据长度
} RecvData_T;

typedef struct
{
    INT8 *mccmnc;
    INT32 *apn;
} PLMN_T;

typedef struct
{
    char apn[32];       //保存接入点名称
    char name[32];      //保存接入点用户名
    char pw[32];        //保存接入点密码
    char ip[32];        //数据服务器IP地址
    char http_ip[32];   //HTTP服务器IP地址
    char http_url[256]; //认证服务器地址在HTTP服务器上的路径
    char auth_ip[32];   //认证服务器地址
    char LBS_ip[32];    //LBS服务器地址
    //char SencondApn[32];
    //char SencondName[32];
    //char SencondPw[32];
    //char SencondIp[32];
    char phNum[6];                 //保存本机号码
    DWORD port;                    //数据服务器端口号
    DWORD http_port;               //http服务器端口号
    DWORD auth_port;               //认证服务器端口号
    DWORD LBS_port;                //LBS服务器端口号
    DWORD UdpPort;                 //UDP连接端口号
    unsigned char ServerRegisterd; //标记是否已进行认证
    char lbsKey[128];              //保存LBS需要的key
    BYTE PowerLowValue;
    BYTE TempListenFlag;
    DWORD HeartBeatTime;  //心跳时间
    DWORD TcpResendNum;   //重发次数
    DWORD ReportInterval; //未使用
    //UINT8 ResetNum[22];
    //UINT8 ListenNum[4][MAX_PHONE_NUMBER];    //监听号码
    //UINT8 PreroNum[4][MAX_PHONE_NUMBER];    //特权号码
    //UINT8 TempListenNum[MAX_PHONE_NUMBER];
    DWORD version; //增加参数或修改参数默认值时需要修改此变量
} DEVICE_PARAM_T;

typedef struct
{
    UINT8 FotaStatus;
    char version[7]; //
} FOTA_PARAM_T;

typedef struct
{
    SYS_STATUS_T Sys_status;
    UINT8 GSMRegisted;  /*GSM已注册*/
    UINT8 GprsRegisted; /*GPRS已注册*/
    UINT8 GprsStatus;   /*当前联网的状态*/
    INT32 tcp_sock;     /*连接数据 服务器socket*/
    INT32 lbs_sock;     /*连接LBS 服务器socket*/
    INT32 update_sock;  /*http远程升级socket*/
    UINT8 login;
    UINT8 TcpStartLink; /*TCP需要建立连接*/
    UINT8 TcpStatus;    /*连接状态机*/
    UINT8 LBSStartLink; //LBS需要建立连接时置1
    UINT8 LBSStatus;
    UINT8 UpdateStartLink;
    UINT8 UpdateConnStatus; /*升级连接状态机*/
    UINT8 UDURL[128];
    UINT8 UDServer[32];
    UINT16 UDPort;
    UINT8 mnc;
    WORD mcc;
    WORD GsmLac;    /*地区编号*/
    WORD GsmCellID; /*小区ID*/
    UINT8 jd[16];   /*经度*/
    UINT8 wd[16];   /*纬度*/
    INT32 UDFile;
} RUN_PARAM_T;

typedef enum
{
    GPS_VALID,         //GPS定位成功
    GPS_INVALID,       //GPS掉线
    SYS_SLEEP,         //通知MCU休眠
    PARAM_SAVE,        //把默认参数保存到MCU
    PARAM_GET,         //从MCU获取默认参数
    PARAM_SET_OK,      //短信设置参数成功,通知MCU闪灯
    SERVER_CONNECT,    //连接服务器成功
    SERVER_DISCONNECT, //服务器断开连接
    GET_VOLTAGE        //获取电压值
} MCU_DATA_T;

extern UINT32 TASK_ONE, TASK_TWO, TASK_THREE, TASK_FOUR, TASK_FIVE;
extern RUN_PARAM_T gRunParam;
extern char IMSI[16];
extern char IMEI[16];
extern DEVICE_PARAM_T Device_Param;

void gapp1_sencond_timer_handler(void *arg);
#endif
