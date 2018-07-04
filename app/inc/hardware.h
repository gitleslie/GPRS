#ifndef _HARDWARE_H_
#define _HARDWARE_H_

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
#include "lite-utils_internal.h"
#include "json_parser.h"

#define CMD 0// 测试时使用
#define HDW_MQTT_TICK_TIME 1000   //MQTT 循环查询时间
#define HDW_DEV_INIT_FILE "key.txt"   //设备注册数据存储文件
// #define VERSION "11"                      //软件版本号
#define VERSION "10"
#define FEED_WATCHDOG_TIME 40000   //看门狗喂狗时间

#define GPIO_CONFIG_0  (sys_gpio_cfg(GAPP_IO_0, GAPP_GPIO_OUT))//PIN22,只能作为输出
#define GPIO_SET_0     (sys_gpio_set(GAPP_IO_0,1)) //~~~~ 18-2-26 从1改成0
#define GPIO_RESET_0   (sys_gpio_set(GAPP_IO_0,0)) //~~~~ 18-2-26 从0改成1

#define GPIO_CONFIG_1  (sys_gpio_cfg(GAPP_IO_1, GAPP_GPIO_OUT))//PIN17,只能作为输出
#define GPIO_SET_1     (sys_gpio_set(GAPP_IO_1,1))
#define GPIO_RESET_1   (sys_gpio_set(GAPP_IO_1,0)) 

#define GPIO_CONFIG_5  (sys_gpio_cfg(GAPP_IO_5, GAPP_GPIO_OUT))//PIN20,只能作为输出
#define GPIO_SET_5     (sys_gpio_set(GAPP_IO_5,1))
#define GPIO_RESET_5   (sys_gpio_set(GAPP_IO_5,0)) 

#define HDW_MOTOR_ON GPIO_SET_0  //按摩抱枕电机启动
#define HDW_MOTOR_OFF GPIO_RESET_0  //按摩抱枕电机停止

unsigned char hdw_start_up_flg;   //设备启动检测标识，如果从注册文件中读取到设备数据，置1，设备连接MQTT 启动，否则置0，设备从TCP 启动

unsigned char sub_topic_mark_flag;//发送 第一次注册数据成功后，置1.
unsigned char sub_rrpc_topic_suss_flag; //设备连接MQTT后，发送rrpc request topic注册，注册成功，则置1，开始发送data topic 注册，
unsigned char sub_data_topic_suss_flag ;//在发送data topic 并且注册成功后，置1，开始发送第一次通电时，注册的数据，
unsigned char sub_rrpc_topic_fail_flag;//response topic 没注册成功，置1，重新发送 rrpc request topic
unsigned char sub_data_topic_fail_flag ;//data topic 没注册成功 置1，重新发送data topic

int mqtt_ping_time;// MQTT 发送心跳包的基础时间
// bool bend_flag = 1;

unsigned char hdw_motor_enable_flg;//上位机启用 禁止标志位，开始初始化时，置1，使能电机。

typedef struct {
    char data[100]; //data topic
    char request[100];//request topic
	char response[100]; //response topioc 
}hdw_topic;//



/* Handle structure of subscribed topic */
typedef struct 
{

}hdw_operation;

hdw_operation hdw_oper;//初始化参数  待添加


void hdw_watchdog_enable();//使能看门狗函数

void hdw_watchdog_feed();//喂狗函数

void hdw_watchdog_disable();//禁止看门狗函数

void hdw_gpio_init();//gpio 初始化函数，初始化电机控制IO(PIN22)， IO20

int ReadDevParam(void);
void WriteDevParamToFlash(UINT8 type);

void hdw_dev_start_from_tcp(hdw_operation data); //MQTT 通过TCP连接设备初始化

//void hdw_dev_init_from_tcp_to_mqtt(hdw_operation data); //

void hdw_dev_initial(hdw_operation data);//设备初始化，从flash中读取将PRODUCT_KEY   DEVICE_NAME   DEVICE_SECRET
//////////////////////////////////////////////////////////

void hdw_motor_timer_hanlder(void *arg);//电机运行时间 定时器

void hdw_dev_init_timer_hanlder(void *arg);   //链接TCP服务器 轮询定时器

void hdw_get_init_param_hanlder(void *arg); //链接MQTT 轮询定时器
/////////////////////////////////////////////////////////////////

void hdw_test_self(UINT8 *data);// 设备自检  指令为AT+TEST

/////////////////////////////////////////////////

void hdw_send_register_data(char *send_data);// 与TCP 服务器建立好链接以后，发送IMEI等注册信息

void hdw_send_init_data(char *send_data); //与MQTT链接以后，发送上电时的初始化信息

void hdw_send_stop_data(char *send_data);// 发送按摩抱枕停止信息

void hdw_send_ping_data(char *send_data);//发送按摩抱枕ping信息

/////////////////////////////////////////////////////////////////

void hdw_data_topic_register( void);// 发送 data topic 注册函数

void hdw_response_topic_register( void);//发送 response topic 注册函数

void hdw_publish_send_init_data( void);//发送 上电时的注册信息

void hdw_send_stop_order_toserver( void);//发送按摩抱枕停止时，stop 信息

///////////////////////////////////////////////////////////////////////

iotx_mc_state_t hdw_get_mqtt_client_state(iotx_mc_client_t *pClient);//获取当前MQTT的链接状态

/////////////////////////////////////////////////////////////////
void hdw_data_topic_message_process(const char *msg);//从data topic 获取的数据后处理函数

void hdw_rrpc_topic_message_process(char *msg, char *send_data);//从rrpc topic 获取数据后处理函数

#endif
