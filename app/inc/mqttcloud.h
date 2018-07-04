
#ifndef  MQTTCLOUD_H_
#define  MQTTCLOUD_H_


#include "iot_import.h"
#include "iot_export.h"

#include "debug.h"

#define MQTT_PING_TIME          20   //  滴答定时器，相应ping时间有HDW_MQTT_TICK_TIME  ms基数计算
#define MQTT_KEEP_ALIVE_TIME    30000   //mSecond

typedef struct
{
	UINT8 disconnect_cnt;

}MQTT_PROCESS_ABNORMAL_PARAM;

typedef struct
{
	char productkey[64];
	char devname[64];
	char devsecret[128];
}MQTT_KEY_PARAM_T;

typedef enum
{
	MQTT_STATE_DISCONNECT = 0,
	MQTT_STATE_CONNECT =1,
	MQTT_STATE_RECONNECT = 2,
	MQTT_STATE_PROCRECV = 3,
	MQTT_STATE_PROCPUB = 4,
	MQTT_STATE_PING = 5,
}MQTT_STATE_T;

typedef struct
{
	char topic_pub[128];
	char topic_sub[128];
	char msg_pub[256];
}MQTT_OP_PARAM_T;

typedef enum
{
	MQTT_OP_DISCONNECT = 0,
	MQTT_OP_CONNECT = 1,// uart data receive
	MQTT_OP_SUB = 2,
	MQTT_OP_PUB = 3,
	MQTT_OP_IDLE = 4,
	MQTT_OP_RECV = 5,
	MQTT_OP_RECONNETCT = 6,
	MQTT_OP_DDD = 7,
}MQTT_OP_T;


#if defined(MQTT_ID2_AUTH) && defined(TEST_ID2_DAILY)
    //#define PRODUCT_KEY             "1uQLHnaQJ6v"
    //#define DEVICE_NAME             "ccn1"
    //#define DEVICE_SECRET           "a3NDObhVh4pIOK1dFRcvONaiauPzKxpb"
#else
#define PRODUCT_KEY             "8P50RNCZVHc"
#define DEVICE_NAME             "864797031235217"
#define DEVICE_SECRET           "pq6sFdUROGyiPEwek9qzxyLhYlq0SHrL"
//#define PRODUCT_KEY             "1uQLHnaQJ6v"
//#define DEVICE_NAME             "ccn1"
//#define DEVICE_SECRET           "a3NDObhVh4pIOK1dFRcvONaiauPzKxpb"
#endif


// These are pre-defined topics
#define TOPIC_UPDATE            "/"PRODUCT_KEY"/"DEVICE_NAME"/update"
#define TOPIC_ERROR             "/"PRODUCT_KEY"/"DEVICE_NAME"/update/error"
#define TOPIC_GET               "/"PRODUCT_KEY"/"DEVICE_NAME"/get"
#define TOPIC_DATA              "/"PRODUCT_KEY"/"DEVICE_NAME"/data"  

#define MSG_LEN_MAX             (1024*2)

#define EXAMPLE_TRACE(fmt, args...)  \
    do { \
        sys_printf("%s|%03d :: ", __func__, __LINE__); \
        sys_printf(fmt, ##args); \
        sys_printf("%s", "\r\n"); \
    } while(0)

void event_handle(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg);
INT32 Cloud_Param_Init();
INT32 Op_aliconn();
INT32 Op_ali_sub(const char *topic);
INT32 Op_ali_pub(const char *topic, const char *msg_input);
void CloudTick();
void tryPingAndKeepAlive();
#endif

