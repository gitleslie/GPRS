#include "lite-log.h"
#include "iot_import.h"
#include "sys_callback.h"
#include "sys_services.h"
#include "sys_ext.h"
#include "string.h"
#include "cmd.h"
#include "app.h"
#include "debug.h"
#include "cmddef.h"
#include "manage.h"



//add by Frank liu 20150414
#define INVALID_SOCKET  (-1)
#define SOCKET_ERROR    (-1)
#define SEND_UDP_DATA_TIMES 30
#define RET_FAILED -1


/** @defgroup group_platform platform
 *  @{
 */


/*********************************** mutex interface ***********************************/

/** @defgroup group_platform_mutex mutex
 *  @{
 */

/**
 * @brief Create a mutex.
 *
 * @return NULL, initialize mutex failed; not NULL, the mutex handle.
 * @see None.
 * @note None.
 */
void *HAL_MutexCreate(void)
{

	return NULL;

}



/**
 * @brief Destroy the specified mutex object, it will free related resource.
 *
 * @param [in] mutex @n The specified mutex.
 * @return None.
 * @see None.
 * @note None.
 */
void HAL_MutexDestroy(_IN_ void *mutex)
{
	// do nothing
}



/**
 * @brief Waits until the specified mutex is in the signaled state.
 *
 * @param [in] mutex @n the specified mutex.
 * @return None.
 * @see None.
 * @note None.
 */
void HAL_MutexLock(_IN_ void *mutex)
{
	//do nothing;
}



/**
 * @brief Releases ownership of the specified mutex object..
 *
 * @param [in] mutex @n the specified mutex.
 * @return None.
 * @see None.
 * @note None.
 */
void HAL_MutexUnlock(_IN_ void *mutex)
{
	//do nothing
}


/** @} */ //end of platform_mutex


/** @defgroup group_platform_memory_manage memory
 *  @{
 */

/**
 * @brief Allocates a block of size bytes of memory, returning a pointer to the beginning of the block.
 *
 * @param [in] size @n specify block size in bytes.
 * @return A pointer to the beginning of the block.
 * @see None.
 * @note Block value is indeterminate.
 */
void *HAL_Malloc(_IN_ uint32_t size)
{
	char *tmp=NULL;
	tmp = sys_malloc(size);
	return tmp;
}


/**
 * @brief Deallocate memory block
 *
 * @param[in] ptr @n Pointer to a memory block previously allocated with platform_malloc.
 * @return None.
 * @see None.
 * @note None.
 */
void HAL_Free(_IN_ void *ptr)
{
	sys_free(ptr);

}


/** @} */ //end of platform_memory_manage

/** @defgroup group_platform_other other
 *  @{
 */

/**
 * @brief Retrieves the number of milliseconds that have elapsed since the system was boot.
 *
 * @param None.
 * @return the number of milliseconds.
 * @see None.
 * @note None.
 */
uint32_t HAL_UptimeMs(void)
{
	return sys_getSysTick();

}


/**
 * @brief sleep thread itself.
 *
 * @param [in] ms @n the time interval for which execution is to be suspended, in milliseconds.
 * @return None.
 * @see None.
 * @note None.
 */
void HAL_SleepMs(_IN_ uint32_t ms)
{
	sys_taskSleep(1);
}

/**
 * @brief Writes formatted data to stream.
 *
 * @param [in] fmt: @n String that contains the text to be written, it can optionally contain embedded format specifiers
     that specifies how subsequent arguments are converted for output.
 * @param [in] ...: @n the variable argument list, for formatted and inserted in the resulting string replacing their respective specifiers.
 * @return None.
 * @see None.
 * @note None.
 */
void HAL_Printf(_IN_ const char *fmt, ...)
{
	UINT16 len;
	va_list ap;
	UINT8 db[1024];
	
	va_start(ap,fmt);
	len  = (UINT16)sys_vsnprintf(db,1024,fmt,ap);
	va_end(ap);
	sys_uart_output(0,db,len);


}
/**
 * @brief Get vendor ID of hardware module.
 *
 * @return NULL, Have NOT PID; NOT NULL, point to pid_str.
 */
char *HAL_GetPartnerID(char pid_str[])
{
	return NULL;

}

/** @} */ //end of group_platform_other

/**
 * @brief Establish a TCP connection.
 *
 * @param [in] host: @n Specify the hostname(IP) of the TCP server
 * @param [in] port: @n Specify the TCP port of TCP server
 *
 * @return 0, fail; > 0, success, the value is handle of this TCP connection.
 */
extern INT32 mqttSocketId;
INT32 HAL_TCP_Establish(const char *host, uint16_t port)
{
	int32 ret=0;
	int32 tempSocketId=0;
	ret = strlen( host );
	GAPP_TCPIP_ADDR_T Msocket_address;
//huihonglei modify
	//if( iSocketId > 0 )
	if( mqttSocketId >= 0 )
	{
		log_warning(DB2, "Cloud socket need to close SocketID:[%d]", mqttSocketId );
		//huihonglei modify
		//close( iSocketId );
		sys_sock_close(mqttSocketId);
		mqttSocketId = INVALID_SOCKET;
		sys_taskSleep(500);
	}

//huihonglei modify
	//if( (iSocketId = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))<=0)
	if((mqttSocketId = sys_sock_create(GAPP_IPPROTO_TCP))<0)
	{
		log_err(" Cloud socket init fail");
		return NULL;
	}
	if(mqttSocketId>=3)
	{
        log_err("mqttSocketId>=3, will reset...");
		sys_softReset();
	}
	tempSocketId = mqttSocketId;
	log_info("New cloud socketID [%d]",mqttSocketId);
//huihonglei modify
	//iSocketId = GAgent_connect( iSocketId, port, p_szServerIPAddr,flag );

	log_info("do connect ip:%s port=%d",host,port );
	ret = sys_getHostByName((INT8 *)host,&Msocket_address.sin_addr);
	log_info("End sys_getHostByName");
	Msocket_address.sin_port = htons(port);
	ret = sys_sock_connect(mqttSocketId,&Msocket_address);

	if ( ret < 0 )
	{
	//huihonglei modify
		//close( tempSocketId );
		sys_sock_close(tempSocketId);
		mqttSocketId=INVALID_SOCKET;
		log_err("Cloud socket connect fail with:%d", ret);
		return NULL;
	}
	return mqttSocketId;

}

/**
 * @brief Destroy the specific TCP connection.
 *
 * @param [in] fd: @n Specify the TCP connection by handle.
 *
 * @return < 0, fail; 0, success.
 */
int32_t HAL_TCP_Destroy(uintptr_t fd)
{
	sys_sock_close(*fd);
	return 0;

}


/**
 * @brief Write data into the specific TCP connection.
 *        The API will return immediately if @len be written into the specific TCP connection.
 *
 * @param [in] fd @n A descriptor identifying a connection.
 * @param [in] buf @n A pointer to a buffer containing the data to be transmitted.
 * @param [in] len @n The length, in bytes, of the data pointed to by the @buf parameter.
 * @param [in] timeout_ms @n Specify the timeout value in millisecond. In other words, the API block @timeout_ms millisecond maximumly.
 * @return
   @verbatim
        < 0 : TCP connection error occur..
          0 : No any data be write into the TCP connection in @timeout_ms timeout period.
   (0, len] : The total number of bytes be written in @timeout_ms timeout period.
   @endverbatim
 * @see None.
 */
int32_t HAL_TCP_Write(uintptr_t fd, const char *buf, uint32_t len, uint32_t timeout_ms)
{
	int32_t sen_len=0;
	sen_len = sys_sock_send((INT32)*fd ,(INT8 *)buf, len);
	//log_info("write ok");
	//DB(DB2,"packet send length= %d\r\n",sen_len);
	if(sen_len<0)
	{
		return -1;
		}
	return sen_len;

}

/**
 * @brief Read data from the specific TCP connection with timeout parameter.
 *        The API will return immediately if @len be received from the specific TCP connection.
 *
 * @param [in] fd @n A descriptor identifying a TCP connection.
 * @param [in] buf @n A pointer to a buffer to receive incoming data.
 * @param [in] len @n The length, in bytes, of the data pointed to by the @buf parameter.
 * @param [in] timeout_ms @n Specify the timeout value in millisecond. In other words, the API block @timeout_ms millisecond maximumly.
 * @return
   @verbatim
         -2 : TCP connection error occur.
         -1 : TCP connection be closed by remote server.
          0 : No any data be received in @timeout_ms timeout period.
   (0, len] : The total number of bytes be received in @timeout_ms timeout period.
   @endverbatim
 * @see None.
 */
int32_t HAL_TCP_Read(uintptr_t fd, char *buf, uint32_t len, uint32_t timeout_ms)
{
	//INT32 sys_sock_recv(INT32 sock,UINT8 *buff,UINT16 len);
	INT32 recv_len=0;
	recv_len = sys_sock_recv(*fd, buf, len);
	// log_info("Receive Buf == %s, And  BufLen == %d",buf,recv_len);
	//log_info("read ok");
	return recv_len;
}
/**
 * @brief Establish a SSL connection.
 *
 * @param [in] host: @n Specify the hostname(IP) of the SSL server
 * @param [in] port: @n Specify the SSL port of SSL server
 * @param [in] ca_crt @n Specify the root certificate which is PEM format.
 * @param [in] ca_crt_len @n Length of root certificate, in bytes.
 * @return SSL handle.
 * @see None.
 * @note None.
 */
uintptr_t HAL_SSL_Establish(
            const char *host,
            uint16_t port,
            const char *ca_crt,
            size_t ca_crt_len)
{
	return NULL;


}


/**
 * @brief Destroy the specific SSL connection.
 *
 * @param[in] handle: @n Handle of the specific connection.
 *
 * @return < 0, fail; 0, success.
 */
int32_t HAL_SSL_Destroy(uintptr_t handle)
{
	return 0;
	
}	


/**
 * @brief Write data into the specific SSL connection.
 *        The API will return immediately if @len be written into the specific SSL connection.
 *
 * @param [in] fd @n A descriptor identifying a connection.
 * @param [in] buf @n A pointer to a buffer containing the data to be transmitted.
 * @param [in] len @n The length, in bytes, of the data pointed to by the @buf parameter.
 * @param [in] timeout_ms @n Specify the timeout value in millisecond. In other words, the API block @timeout_ms millisecond maximumly.
 * @return
   @verbatim
        < 0 : SSL connection error occur..
          0 : No any data be write into the SSL connection in @timeout_ms timeout period.
   (0, len] : The total number of bytes be written in @timeout_ms timeout period.
   @endverbatim
 * @see None.
 */
int32_t HAL_SSL_Write(uintptr_t handle, const char *buf, int len, int timeout_ms)
{
	return 0;
}


/**
 * @brief Read data from the specific SSL connection with timeout parameter.
 *        The API will return immediately if @len be received from the specific SSL connection.
 *
 * @param [in] fd @n A descriptor identifying a SSL connection.
 * @param [in] buf @n A pointer to a buffer to receive incoming data.
 * @param [in] len @n The length, in bytes, of the data pointed to by the @buf parameter.
 * @param [in] timeout_ms @n Specify the timeout value in millisecond. In other words, the API block @timeout_ms millisecond maximumly.
 * @return
   @verbatim
         -2 : SSL connection error occur.
         -1 : SSL connection be closed by remote server.
          0 : No any data be received in @timeout_ms timeout period.
   (0, len] : The total number of bytes be received in @timeout_ms timeout period.
   @endverbatim
 * @see None.
 */
int32_t HAL_SSL_Read(uintptr_t handle, char *buf, int len, int timeout_ms)
{
	return 0;
}

