/*
 * Copyright (c) 2014-2016 Alibaba Group. All rights reserved.
 * License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iot_export_mqtt.h>
#include "sys_services.h"

#include "mqttcloud.h"
#include "mqtt_client.h"
#include "hardware.h"

extern MQTT_OP_T mqtt_op;
extern iotx_mqtt_param_t mqtt_params;
extern void *m_pclient;
extern MQTT_OP_PARAM_T mqtt_op_param;
extern UINT32 SecondCnt;
extern MQTT_KEY_PARAM_T mqtt_key_param;
extern MQTT_STATE_T m_state;
extern UINT8 GprsActived;
extern UINT8 GprsAttched;
extern UINT8 ping_flag;
extern INT32 mqttSocketId;

MQTT_PROCESS_ABNORMAL_PARAM m_process_dis_param = {0};

void event_handle(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
{

    uintptr_t packet_id = (uintptr_t)msg->msg;
    iotx_mqtt_topic_info_pt topic_info = (iotx_mqtt_topic_info_pt)msg->msg;
    char e_topic[128] = {0}; //MQTT_MAX_TOPIC_LEN =64;
    unsigned char e_payload[256] = {0};
    int ret;
    log_info("Event Hande Process And Event Type == %d ", msg->event_type);
    switch (msg->event_type)
    {
    case IOTX_MQTT_EVENT_UNDEF:
        log_info("undefined event occur.");
        break;

    case IOTX_MQTT_EVENT_DISCONNECT:
        /*log_info("MQTT disconnect.");
            ret = sys_sock_close(mqttSocketId);
            log_info("begin to");
			ret = sys_PDPRelease();
            log_info("sys_PDPRelease ret %d",ret);
			ping_flag = 0;
			GprsAttched = 0;
			GprsActived = 0;
			mqtt_op = MQTT_STATE_RECONNECT;*/
        //ret = sys_sock_close(mqttSocketId);
        log_info("End The Mqtt");
        ret = sys_sock_close(mqttSocketId);
        ping_flag = 0;
        GprsAttched = 0;
        GprsActived = 0;
        mqtt_op = MQTT_STATE_RECONNECT;
        sys_taskSend(TASK_ONE, Feed_Watch_dog, 0, 0, 0);
        break;

    case IOTX_MQTT_EVENT_RECONNECT:
        log_info("MQTT reconnect."); //添加断网处理
        break;

    case IOTX_MQTT_EVENT_SUBCRIBE_SUCCESS:
        log_info("subscribe success, packet-id=%u", (unsigned int)packet_id);
        // sub_topic_mark_flag++;// 每个topic data 注册成功后  +1
        // switch(sub_topic_mark_flag)
        // {
        // 	case 1:// resopnse topic 成功
        // 	{
        // 		sub_rrpc_topic_suss_flag = 1;
        // 		sys_taskSend(TASK_FIVE,  T5_HDW_REGISTER_TOPIC_FIRSTDATA, 0, 0, 0);
        // 	}
        // 		break;
        // 	case 2:// data topic成功
        // 	{
        // 		sub_data_topic_suss_flag= 1;
        // 		sys_taskSend(TASK_FIVE,   T5_HDW_REGISTER_TOPIC_FIRSTDATA, 0, 0, 0);
        // 	}
        // 		break;
        // 	default:
        // 		break;
        // }
        // sub_rrpc_topic_suss_flag = 1;
        sub_data_topic_suss_flag = 1;
        sys_taskSend(TASK_FIVE, T5_HDW_REGISTER_TOPIC_FIRSTDATA, 0, 0, 0);
        break;

    case IOTX_MQTT_EVENT_SUBCRIBE_TIMEOUT:
        log_err("subscribe wait ack timeout, packet-id=%u", (unsigned int)packet_id);
        // switch(sub_topic_mark_flag)   //topic 注册超时，查询是那个topic 没注册成功
        // {
        // 	case 0:// response  topic 没注册成功
        // 	{
        // 		sub_rrpc_topic_fail_flag = 1 ;
        // 		sys_taskSend(TASK_FIVE,  T5_HDW_HANDLE_ABNORMAL , 0, 0, 0);
        // 	}
        // 	break;
        // 	case 1:// data  topic 没注册成功
        // 	{
        // 		sub_data_topic_fail_flag  = 1 ;
        // 		packet_id = 0;
        // 		sys_taskSend(TASK_FIVE,  T5_HDW_HANDLE_ABNORMAL , 0, 0, 0);
        // 	}
        // 	break;
        // 	default:
        // 		break;
        // }
        sub_data_topic_fail_flag = 1;
        packet_id = 0;
        sys_taskSend(TASK_FIVE, T5_HDW_HANDLE_ABNORMAL, 0, 0, 0);
        break;

    case IOTX_MQTT_EVENT_SUBCRIBE_NACK:
        log_err("subscribe nack, packet-id=%u", (unsigned int)packet_id);
        switch (sub_topic_mark_flag) //topic 注册错误，查询是那个topic 没注册成功
        {
        case 0:
        {
            sub_rrpc_topic_fail_flag = 1;
            sys_taskSend(TASK_FIVE, T5_HDW_HANDLE_ABNORMAL, 0, 0, 0);
        }
        break;
        case 1:
        {
            sub_data_topic_fail_flag = 1;
            packet_id = 0;
            sys_taskSend(TASK_FIVE, T5_HDW_HANDLE_ABNORMAL, 0, 0, 0);
        }
        break;
        default:
            break;
        }
        break;

    case IOTX_MQTT_EVENT_UNSUBCRIBE_SUCCESS:
        log_info("unsubscribe success, packet-id=%u", (unsigned int)packet_id);
        break;

    case IOTX_MQTT_EVENT_UNSUBCRIBE_TIMEOUT:
        log_err("unsubscribe timeout, packet-id=%u", (unsigned int)packet_id);
        break;

    case IOTX_MQTT_EVENT_UNSUBCRIBE_NACK:
        log_err("unsubscribe nack, packet-id=%u", (unsigned int)packet_id);
        break;

    case IOTX_MQTT_EVENT_PUBLISH_SUCCESS:
        log_info("publish success, packet-id=%u", (unsigned int)packet_id);
        break;

    case IOTX_MQTT_EVENT_PUBLISH_TIMEOUT:
        log_err("publish timeout, packet-id=%u", (unsigned int)packet_id);
        break;

    case IOTX_MQTT_EVENT_PUBLISH_NACK:
        log_err("publish nack, packet-id=%u", (unsigned int)packet_id);
        break;

    case IOTX_MQTT_EVENT_PUBLISH_RECVEIVED:
        log_warning("_topic message arrived but without any related handle_");
        snprintf(e_topic, topic_info->topic_len + 1, "%s", topic_info->ptopic);
        log_warning("Topic: '%s' (Length: %d)", e_topic, topic_info->topic_len);
        snprintf(e_payload, topic_info->payload_len + 1, "%s", topic_info->payload);

        // EXAMPLE_TRACE("topic message arrived but without any related handle: topic=%.*s, topic_msg=%.*s",
        //              topic_info->topic_len,
        //              topic_info->ptopic,
        //              topic_info->payload_len,
        //              topic_info->payload);
        break;

    default:
        log_warning("Should NOT arrive here.");
        break;
    }
}

//callback after the corresponding topic message recieve, set in topic subcribe.
UINT16 upacket_id = 1;
static void _demo_message_arrive(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
{   
    
    iotx_mqtt_topic_info_pt ptopic_info = (iotx_mqtt_topic_info_pt)msg->msg;
    if((upacket_id == ptopic_info->packet_id) && (ptopic_info->packet_id != 0))
        return ;
    else
        upacket_id = ptopic_info->packet_id;
    if (msg == NULL)
        return;
    char tmptopic[128] = {0}; //MQTT_MAX_TOPIC_LEN =64;
    char tmppayload[256] = {0};
    memset(tmppayload, 0, sizeof(tmppayload));
    // log_info("_demo_message_arrive receive ptopic_info->payload_id == %d",ptopic_info->packet_id);
    // print topic name and topic message
    // log_info("----%d", msg->event_type);
#if 0
    EXAMPLE_TRACE("Topic: %s (Length: %d)",
                  ptopic_info->ptopic,
                  ptopic_info->topic_len);
	
    EXAMPLE_TRACE("Payload: %s (Length: %d)",
                  ptopic_info->payload,
                  ptopic_info->payload_len);
#else
    
    snprintf(tmptopic, ptopic_info->topic_len + 1, "%s", ptopic_info->ptopic);
    // log_info("Topic: '%s' (Length: %d)",tmptopic, ptopic_info->topic_len);
    snprintf(tmppayload, ptopic_info->payload_len + 1, "%s", ptopic_info->payload);
    // log_info("Payload: '%s' (Length: %d)", tmppayload, ptopic_info->payload_len);
    hdw_data_topic_message_process(tmppayload);                // 处理data topic 发送的信息
    memset(ptopic_info, 0x0, sizeof(iotx_mqtt_topic_info_pt)); //发送后处理
#endif
    // log_info("----");
}

//char *msg_buf = NULL,*msg_readbuf = NULL;
INT32 Cloud_Param_Init(const char *productkey, const char *devname, const char *devsecret)
{
    int rc = 0;
    char *msg_buf = NULL, *msg_readbuf = NULL;
    iotx_conn_info_pt pconn_info;

    if (NULL == (msg_buf = (char *)HAL_Malloc(MSG_LEN_MAX)))
    {
        log_err("not enough memory");
        rc = -1;
        return rc;
    }

    if (NULL == (msg_readbuf = (char *)HAL_Malloc(MSG_LEN_MAX)))
    {
        log_err("not enough memory");
        rc = -1;
        return rc;
    }

    /* Device AUTH */
    //if (0 != IOT_SetupConnInfo(PRODUCT_KEY, DEVICE_NAME, DEVICE_SECRET, (void **)&pconn_info)) {
    if (0 != IOT_SetupConnInfo(productkey, devname, devsecret, (void **)&pconn_info))
    {

        log_err("AUTH request failed!");
        rc = -1;
        return rc;
    }

    /* Initialize MQTT parameter */
    memset(&mqtt_params, 0x0, sizeof(mqtt_params));

    mqtt_params.port = pconn_info->port;
    mqtt_params.host = pconn_info->host_name;
    mqtt_params.client_id = pconn_info->client_id;
    mqtt_params.username = pconn_info->username;
    mqtt_params.password = pconn_info->password;
    mqtt_params.pub_key = pconn_info->pub_key;

    mqtt_params.request_timeout_ms = 5000;
    mqtt_params.clean_session = 0;
    mqtt_params.keepalive_interval_ms = MQTT_KEEP_ALIVE_TIME;
    mqtt_params.pread_buf = msg_readbuf;
    mqtt_params.read_buf_size = MSG_LEN_MAX;
    mqtt_params.pwrite_buf = msg_buf;
    mqtt_params.write_buf_size = MSG_LEN_MAX;

    mqtt_params.handle_event.h_fp = event_handle; //mqtt event proccess,such as ota?
    mqtt_params.handle_event.pcontext = NULL;

    return rc;
}

extern hdw_topic hdw_topic_para; // 定义三个topic  data topic  request topic response  topic

void mqtt_rrpc_msg_arrive(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
{
    iotx_mqtt_topic_info_pt ptopic_info = (iotx_mqtt_topic_info_pt)msg->msg;
    iotx_mqtt_topic_info_t topic_msg;
    char msg_pub[255] = {0};
    char topic[255] = {0};
    char msg_id[255] = {0};
    char tmppayload[256] = {0};
    // print topic name and topic message
    log_info("----\n");

    snprintf(tmppayload, ptopic_info->payload_len + 1, "%s", ptopic_info->payload);
    log_info("Payload: %s ", tmppayload);
    log_info("----\n");

    hdw_rrpc_topic_message_process(tmppayload, msg_pub); // rrpc request topic 信息处理
    if (snprintf(msg_id,
                 ptopic_info->topic_len - strlen(hdw_topic_para.request) + 2,
                 "%s",
                 ptopic_info->ptopic + (strlen(hdw_topic_para.request)) - 1) > sizeof(msg_id))
    {
        log_err("snprintf error!\n");
        return;
    }
    log_info("response msg_id = %s\n", msg_id);

    if (snprintf(topic, sizeof(topic), "%s%s", hdw_topic_para.response, msg_id) > sizeof(topic))
    {
        log_err("snprintf error!\n");
        return;
    }
    log_info("response topic = %s\n", topic);
    topic_msg.qos = IOTX_MQTT_QOS0;
    topic_msg.retain = 0;
    topic_msg.dup = 0;
    topic_msg.payload = (void *)msg_pub;
    topic_msg.payload_len = strlen(msg_pub);
    if (IOT_MQTT_Publish(pclient, topic, &topic_msg) < 0)
    {
        log_err("error occur when publish!\n");
    }
}

INT32 Op_aliconn()
{
    int rc = 0;
    iotx_mqtt_topic_info_t topic_msg;
    char msg_pub[128];
    char *msg_buf = NULL, *msg_readbuf = NULL;
    char topic_pub[128] = {0};
    char msg_input[255];
    m_pclient = IOT_MQTT_Construct(&mqtt_params);
    if (NULL == m_pclient)
    {
        log_err("MQTT construct failed");
        rc = -1;
        return rc;
    }

    hdw_data_topic_register();

    //hdw_response_topic_register();// 注册 response  topic
    return 0;
}

void hdw_publish_send_init_data(void) // 打印 上电时的设备信息
{
    int rc = 0;
    iotx_mqtt_topic_info_t topic_msg;
    char msg_pub[128];
    char *msg_buf = NULL, *msg_readbuf = NULL;
    char topic_pub[128] = {0};
    char msg_input[255];

    hdw_send_init_data(msg_input);

    memset(&topic_msg, 0x0, sizeof(iotx_mqtt_topic_info_t));
    sprintf(msg_pub, "%s", msg_input);
    log_info("topic:%s,msg:%s", topic_pub, msg_pub);

    //topic_msg.qos = IOTX_MQTT_QOS1; 修改发动参数
    topic_msg.qos = IOTX_MQTT_QOS0;
    topic_msg.retain = 0;
    topic_msg.dup = 0;
    topic_msg.payload = (void *)msg_input;
    topic_msg.payload_len = strlen(msg_input);

    rc = IOT_MQTT_Publish(m_pclient, hdw_topic_para.data, &topic_msg);
    log_info("rc = IOT_MQTT_Publish() = %d", rc);

    // sys_taskSleep(1000);
    // memset(&topic_msg,0x0,sizeof(iotx_mqtt_topic_info_t));
    mqtt_op = MQTT_OP_IDLE;
}

void hdw_send_stop_order_toserver(void) // 按摩抱枕停止时，发送stop  json串
{
    int rc = 0;
    iotx_mqtt_topic_info_t topic_msg;
    char msg_pub[128];
    char *msg_buf = NULL, *msg_readbuf = NULL;
    char topic_pub[128] = {0};
    char msg_input[255];
    hdw_send_stop_data(msg_input);

    memset(&topic_msg, 0x0, sizeof(iotx_mqtt_topic_info_t));
    sprintf(msg_pub, "%s", msg_input);
    log_info("topic:%s,msg:%s", topic_pub, msg_pub);

    topic_msg.qos = IOTX_MQTT_QOS1;
    topic_msg.retain = 0;
    topic_msg.dup = 0;
    topic_msg.payload = (void *)msg_input;
    topic_msg.payload_len = strlen(msg_input);

    rc = IOT_MQTT_Publish(m_pclient, hdw_topic_para.data, &topic_msg);
    log_info("rc = IOT_MQTT_Publish() = %d", rc);
}

void hdw_response_topic_register(void) // 注册 response data topic
{
    int rc = 0;
    // log_info("hdw_response_topic_register ------------------------------------------------------------------");
    rc = IOT_MQTT_Subscribe(m_pclient, hdw_topic_para.request, IOTX_MQTT_QOS1, mqtt_rrpc_msg_arrive, NULL);
    if (rc < 0)
    {
        log_err("IOT_MQTT_Subscribe() failed, rc = %d", rc);
        IOT_MQTT_Destroy(&m_pclient);
        rc = -1;
    }
}

void hdw_data_topic_register(void) // 注册 data topic
{
    int rc = 0;
    // log_info("hdw_data_topic_register ---------------------------------------------------------------------");
    // rc = IOT_MQTT_Subscribe(m_pclient, hdw_topic_para.data , IOTX_MQTT_QOS1, _demo_message_arrive, NULL);
    rc = IOT_MQTT_Subscribe(m_pclient, hdw_topic_para.data, IOTX_MQTT_QOS0, _demo_message_arrive, NULL);
    if (rc < 0)
    {
        log_err("IOT_MQTT_Subscribe() failed, rc = %d", rc);
        IOT_MQTT_Destroy(&m_pclient);
        rc = -1;
    }
    return 0;
}

void hdw_send_ping_toserver(void)
{
    int rc = 0;
    iotx_mqtt_topic_info_t topic_msg;
    char msg_pub[128];
    char *msg_buf = NULL, *msg_readbuf = NULL;
    char topic_pub[128] = {0};
    char msg_input[255];
    hdw_send_ping_data(msg_input);

    memset(&topic_msg, 0x0, sizeof(iotx_mqtt_topic_info_t));
    sprintf(msg_pub, "%s", msg_input);
    log_info("topic:%s,msg:%s", topic_pub, msg_pub);

    topic_msg.qos = IOTX_MQTT_QOS1;
    topic_msg.retain = 0;
    topic_msg.dup = 0;
    topic_msg.payload = (void *)msg_input;
    topic_msg.payload_len = strlen(msg_input);

    rc = IOT_MQTT_Publish(m_pclient, hdw_topic_para.data, &topic_msg);
    log_info("rc = IOT_MQTT_Publish() = %d", rc);
}

/*未使用*/
INT32 Op_ali_sub(const char *topic)
{
    //    INT32 ret;
    //    static char topic_sub[128]={0};  //must set to static or global, or will miss the message topic.

    //    /* Subscribe the specific topic */
    //    if(m_pclient==NULL){
    //        return -1;
    //    }
    //    //~~~sprintf(topic_sub,"/"PRODUCT_KEY"/"DEVICE_NAME"/%s",topic );
    // sprintf(topic_sub,"%s",topic );
    //    log_info("Op_ali_sub --------------------------------------------------------------------------------");
    //    //EXAMPLE_TRACE("topicsub get:%s,len:%d",topic_sub,strlen(topic_sub));
    //    ret = IOT_MQTT_Subscribe(m_pclient, topic_sub, IOTX_MQTT_QOS0, _demo_message_arrive, mqtt_params.handle_event.pcontext);
    //    //ret = IOT_MQTT_Subscribe(m_pclient, TOPIC_DATA, IOTX_MQTT_QOS1, _demo_message_arrive, mqtt_params.handle_event.pcontext);
    //    if (ret< 0) {
    //        IOT_MQTT_Destroy(&m_pclient);
    //        sys_printf("IOT_MQTT_Subscribe() failed, ret = %d", ret);
    //        ret = -1;
    //        return ret;
    //    }

    return 0;
}

INT32 Op_ali_pub(const char *topic, const char *msg_input)
{
    int rc = 0; //, msg_len,cnt=0;
    //iotx_conn_info_pt pconn_info;
    //iotx_mqtt_param_t mqtt_params;
    iotx_mqtt_topic_info_t topic_msg;
    char msg_pub[256] = {0};
    char topic_pub[128] = {0};
    //char *msg_buf = NULL, *msg_readbuf = NULL;

    /* Initialize topic information */
    memset(&topic_msg, 0x0, sizeof(iotx_mqtt_topic_info_t));
    //strcpy(msg_pub, "message: hello! start!");

    //~~sprintf(topic_pub,"/"PRODUCT_KEY"/"DEVICE_NAME"/%s",topic );
    sprintf(topic_pub, "%s", topic);
    sprintf(msg_pub, "%s", msg_input);
    log_info("topic:%s,msg:%s", topic_pub, msg_pub);

    //topic_msg.qos = IOTX_MQTT_QOS0;
    topic_msg.qos = IOTX_MQTT_QOS1;
    topic_msg.retain = 0;
    topic_msg.dup = 0;
    topic_msg.payload = (void *)msg_input;
    topic_msg.payload_len = strlen(msg_input);
    //EXAMPLE_TRACE("topic:%s,msg22:%s", topic_pub,topic_msg.payload);

    rc = IOT_MQTT_Publish(m_pclient, topic_pub, &topic_msg);
    //rc = IOT_MQTT_Publish(m_pclient, TOPIC_DATA, &topic_msg);
    log_info("rc = IOT_MQTT_Publish() = %d", rc);

    memset(&topic_msg, 0x0, sizeof(iotx_mqtt_topic_info_t));
    return 0;
}

/**/

int mqtt_client(void)
{
}

void tryPingAndKeepAlive()
{
    ping_flag = 0;
    while ((m_state == MQTT_STATE_PROCRECV) || (m_state == MQTT_STATE_PROCPUB))
    {
        // log_info("11111111111111111111111111111111");-
    }
    m_state = MQTT_STATE_PING;
    // log_info("2222222222222222222222222222222222");
    //EXAMPLE_TRACE("IOT_MQTT_KEEPALIVE_AND_CHECK.");
    //IOT_MQTT_KEEPALIVE_AND_CHECK(m_pclient); //修改心跳连接信息IOT_MQTT_KEEPALIVE_AND_CHECK（）为IOT_MQTT_Yield();
    IOT_MQTT_Yield(m_pclient, 5000); //用在>=QoS1场合下
    // int csq = AT_GetSignal();
    // log_info("csq %d",csq);
    m_state = MQTT_STATE_CONNECT;
    ping_flag = 1;
}

int disconnect_cnt = 0;

void CloudTick()
{
    //log_info("mqtt_op ============================================================================================================ %d",mqtt_op);
    switch (mqtt_op)
    {
    case MQTT_OP_IDLE:
        if ((SecondCnt % mqtt_ping_time == 0) && (ping_flag))
        {
            gclient_state = hdw_get_mqtt_client_state(m_pclient);
            // log_info("current client state:%d",gclient_state);
            //tryPingAndKeepAlive();
            if (gclient_state == 3 || gclient_state == 4) // 掉线处理
            {
                //reconnect to cloud or setup the auto connect.
                log_info("Send To TASK_ONE Start To SofeReset");
                //sys_softReset();
                //sys_taskSend(TASK_ONE,T1_SOFT_RESET,0,0,0);
                //sys_taskSleep(3000);
                log_info("Waiting to connect..................");
                Cloud_Param_Init(mqtt_key_param.productkey, mqtt_key_param.devname, mqtt_key_param.devsecret);
                //iotx_mc_handle_reconnect(m_pclient);
                Op_aliconn();
                if (disconnect_cnt++ >= 3)
                {
                    int conn_ret = sys_PDPRelease();
                    log_err("sys_PDPRelease ret %d", conn_ret);
                    sys_taskSend(TASK_ONE, T1_SOFT_RESET, 0, 0, 0);
                    //sys_softReset();
                    disconnect_cnt = 0;
                    ping_flag = 0;
                    GprsAttched = 0;
                    GprsActived = 0;
                }
                return;
            }
            if (gclient_state == 2) //设备正常情况下进行执行心跳连接
            {
                tryPingAndKeepAlive();
            }
            if (gclient_state < 0) // 设备异常处理
            {
                log_err("gclient_state < 0， will softReset!!!!");
                sys_taskSend(TASK_ONE, T1_SOFT_RESET, 0, 0, 0);
                //sys_softReset();
            }
        }

        break;
    case MQTT_OP_CONNECT:
        Cloud_Param_Init(mqtt_key_param.productkey, mqtt_key_param.devname, mqtt_key_param.devsecret);
        Op_aliconn();
        sys_watchdog_enable(15000); // 初始化看门狗 为何又使用看门狗？？？？？
        m_state = MQTT_STATE_CONNECT;
        //mqtt_op = MQTT_OP_IDLE;
        ping_flag = 1;
        break;

    case MQTT_OP_SUB:
        log_info("MQTT_OP_SUB");
        // Op_ali_sub(mqtt_op_param.topic_sub);
        mqtt_op = MQTT_OP_IDLE;
        break;

    case MQTT_OP_PUB:
        // log_info("MQTT_OP_PUB");
        m_state = MQTT_STATE_PROCPUB;
        Op_ali_pub(mqtt_op_param.topic_pub, mqtt_op_param.msg_pub);
        m_state = MQTT_STATE_CONNECT;
        mqtt_op = MQTT_OP_IDLE;
        break;

    case MQTT_OP_RECV:
        m_state = MQTT_STATE_PROCRECV;
        //	EXAMPLE_TRACE("begin receiveeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee\n");
        // log_info("MQTT_OP_RECV");
        IOT_MQTT_Yield(m_pclient, 5000); //超时时间设置
        m_state = MQTT_STATE_CONNECT;
        mqtt_op = MQTT_OP_IDLE;
        break;
    case MQTT_OP_DISCONNECT:
    {
        mqtt_op = MQTT_OP_IDLE;
        m_state = MQTT_STATE_DISCONNECT;
        ping_flag = 0;
        break;
    }
    case MQTT_OP_RECONNETCT:
        IOT_MQTT_KEEPALIVE_AND_CHECK(m_pclient);
        break;
    default:
        log_err("wrong op");
        ping_flag = 0;
        break;
    }
#if 0
	if(mqtt_op==MQTT_OP_CONNECT){
		Cloud_Param_Init();
		Op_aliconn();
		mqtt_op = MQTT_OP_IDLE;
		}
	if(mqtt_op==MQTT_OP_SUB){
		Op_ali_sub(mqtt_op_param.topic_sub);
		mqtt_op = MQTT_OP_IDLE;
		}
	if(mqtt_op==MQTT_OP_PUB){
		Op_ali_pub(mqtt_op_param.topic_pub,mqtt_op_param.msg_pub);
		mqtt_op = MQTT_OP_IDLE;
		}
	if(mqtt_op==MQTT_OP_RECV){
		IOT_MQTT_Yield(m_pclient,500);
		mqtt_op = MQTT_OP_IDLE;
		}
#endif
}
