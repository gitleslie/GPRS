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



#include <string.h>

#include "iot_import.h"
#include "utils_net.h"
#include "lite-log.h"

extern  int32	mqttSocketId;
/*** TCP connection ***/
int read_tcp(utils_network_pt pNetwork, char *buffer, uint32_t len, uint32_t timeout_ms)
{
    return HAL_TCP_Read(pNetwork->handle, buffer, len, timeout_ms);
}


static int write_tcp(utils_network_pt pNetwork, const char *buffer, uint32_t len, uint32_t timeout_ms)
{
    return HAL_TCP_Write(pNetwork->handle, buffer, len, timeout_ms);
}

static int disconnect_tcp(utils_network_pt pNetwork)
{
    if (0 == pNetwork->handle) {
        return -1;
    }

    HAL_TCP_Destroy(pNetwork->handle);
    pNetwork->handle = 0;
    return 0;
}


static int connect_tcp(utils_network_pt pNetwork)
{
    if (NULL == pNetwork) {
        log_err("network is null");
        return 1;
    }
#if 0
    pNetwork->handle = HAL_TCP_Establish(mqttSocketId,pNetwork->pHostAddress, pNetwork->port);
#else
	log_err("network host: %s",pNetwork->pHostAddress);
	pNetwork->sockeid_num = (uint32_t)HAL_TCP_Establish(pNetwork->pHostAddress, pNetwork->port);
	pNetwork->handle = &(pNetwork->sockeid_num);

#endif
    if (0 == pNetwork->handle) {
        return -1;
    }

    return 0;
}


/*** SSL connection ***/
#ifndef IOTX_WITHOUT_TLS
static int read_ssl(utils_network_pt pNetwork, char *buffer, uint32_t len, uint32_t timeout_ms)
{
    if (NULL == pNetwork) {
        log_err("network is null");
        return -1;
    }

    return HAL_SSL_Read((uintptr_t)pNetwork->handle, buffer, len, timeout_ms);
}

static int write_ssl(utils_network_pt pNetwork, const char *buffer, uint32_t len, uint32_t timeout_ms)
{
    if (NULL == pNetwork) {
        log_err("network is null");
        return -1;
    }

    return HAL_SSL_Write((uintptr_t)pNetwork->handle, buffer, len, timeout_ms);
}

static int disconnect_ssl(utils_network_pt pNetwork)
{
    if (NULL == pNetwork) {
        log_err("network is null");
        return -1;
    }

    HAL_SSL_Destroy((uintptr_t)pNetwork->handle);
    pNetwork->handle = 0;

    return 0;
}

static int connect_ssl(utils_network_pt pNetwork)
{
    if (NULL == pNetwork) {
        log_err("network is null");
        return 1;
    }

    if (0 != (pNetwork->handle = (int *)HAL_SSL_Establish(
                                     pNetwork->pHostAddress,
                                     pNetwork->port,
                                     pNetwork->ca_crt,
                                     pNetwork->ca_crt_len + 1))) {
        return 0;
    } else {
        //TODO SHOLUD not remove this handle space
        // The space will be freed by calling disconnect_ssl()
        //utils_memory_free((void *)pNetwork->handle);
        return -1;
    }
}
#endif  /* #ifndef IOTX_WITHOUT_TLS */


/****** network interface ******/

int utils_net_read(utils_network_pt pNetwork, char *buffer, uint32_t len, uint32_t timeout_ms)
{
    if (NULL == pNetwork->ca_crt) { //TCP connection
        return read_tcp(pNetwork, buffer, len, timeout_ms);
#ifndef IOTX_WITHOUT_TLS
    } else { //SSL connection
        return read_ssl(pNetwork, buffer, len, timeout_ms);
#endif
    }
    //log_debug("read ok");
    return 0;
}


int utils_net_write(utils_network_pt pNetwork, const char *buffer, uint32_t len, uint32_t timeout_ms)
{
    if (NULL == pNetwork->ca_crt) { //TCP connection
        return write_tcp(pNetwork, buffer, len, timeout_ms);
#ifndef IOTX_WITHOUT_TLS
    } else { //SSL connection
        return write_ssl(pNetwork, buffer, len, timeout_ms);
#endif
    }
    //log_debug("write ok");
    return 0;
}


int iotx_net_disconnect(utils_network_pt pNetwork)
{
    if (NULL == pNetwork->ca_crt) { //TCP connection
        return disconnect_tcp(pNetwork);
#ifndef IOTX_WITHOUT_TLS
    } else { //SSL connection
        return disconnect_ssl(pNetwork);
#endif
    }
    return 0;
}


int iotx_net_connect(utils_network_pt pNetwork)
{
    if (NULL == pNetwork->ca_crt) { //TCP connection
        return connect_tcp(pNetwork);
#ifndef IOTX_WITHOUT_TLS
    } else { //SSL connection
        return connect_ssl(pNetwork);
#endif
    }
    //return 0;
}

/*初始化阿里连接
 *
 *
 *
 **/
int iotx_net_init(utils_network_pt pNetwork, const char *host, uint16_t port, const char *ca_crt)
{
    if (!pNetwork || !host) {
        log_err("parameter error! pNetwork=%p, host = %p", pNetwork, host);
        return -1;
    }
    pNetwork->pHostAddress = host;
    pNetwork->port = port;
    pNetwork->ca_crt = ca_crt;

    if (NULL == ca_crt) {
        pNetwork->ca_crt_len = 0;
    } else {
        pNetwork->ca_crt_len = strlen(ca_crt);
    }
    //修改阿里连接接口，取消ssl连接
    pNetwork->handle = 0;
    pNetwork->read = read_tcp;//utils_net_read;
    pNetwork->write = write_tcp;//utils_net_write;
    pNetwork->disconnect = disconnect_tcp;//iotx_net_disconnect;
    pNetwork->connect = connect_tcp;//iotx_net_connect;
    log_debug("iotx_net_connect addr:%x , connect_tcp addr:%x ", iotx_net_connect,connect_tcp);
    

    return 0;
}
