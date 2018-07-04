/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *******************************************************************************/

#include "MQTTPacket.h"
#include "string.h"
#include "debug.h"
#include "sys_services.h"
#include "stdio.h"

#define min(a, b) ((a < b) ? 1 : 0)

/**
  * Deserializes the supplied (wire) buffer into publish data
  * @param dup returned integer - the MQTT dup flag
  * @param qos returned integer - the MQTT QoS value
  * @param retained returned integer - the MQTT retained flag
  * @param packetid returned integer - the MQTT packet identifier
  * @param topicName returned MQTTString - the MQTT topic in the publish
  * @param payload returned byte buffer - the MQTT publish payload
  * @param payloadlen returned integer - the length of the MQTT payload
  * @param buf the raw buffer data, of the correct length determined by the remaining length field
  * @param buflen the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success
  */
int MQTTDeserialize_publish(unsigned char* dup, unsigned char* qos, unsigned char* retained, unsigned short* packetid, MQTTString* topicName,
		unsigned char** payload, int* payloadlen, unsigned char* buf, int buflen)
{
	MQTTHeader header = {0};
	unsigned char* curdata = buf;
	unsigned char* enddata = NULL;
	int rc = 0;
	int mylen = 0;
	/*
	//for  buf printf
	unsigned char buf_prf[256]={0};
	int i=0;

	for(i=0;i<32;i++){
		sprintf(buf_prf+strlen(buf_prf),"%02x",buf[i]);
		}
	DB(DB2,"buf:%s",buf_prf);
	*/
	header.byte = readChar(&curdata);
	if (header.bits.type != PUBLISH)
		goto exit;
	*dup = header.bits.dup;
	*qos = header.bits.qos;
	*retained = header.bits.retain;
	curdata += (rc = MQTTPacket_decodeBuf(curdata, &mylen)); /* read remaining length */
	//mylen = *curdata;// 1 byte for length.
	//curdata+=1;
	enddata = curdata + mylen;

	if (!readMQTTLenString(topicName, &curdata, enddata) ||
		enddata - curdata < 0) /* do we have enough data to read the protocol version byte? */
		goto exit;
	if (header.bits.qos>0)//(*qos > 0)
		*packetid = readInt(&curdata);

	*payloadlen = enddata - curdata;
	*payload = curdata;
	rc = 1;
exit:
	return rc;
}



/**
  * Deserializes the supplied (wire) buffer into an ack
  * @param packettype returned integer - the MQTT packet type
  * @param dup returned integer - the MQTT dup flag
  * @param packetid returned integer - the MQTT packet identifier
  * @param buf the raw buffer data, of the correct length determined by the remaining length field
  * @param buflen the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success, 0 is failure
  */
int MQTTDeserialize_ack(unsigned char* packettype, unsigned char* dup, unsigned short* packetid, unsigned char* buf, int buflen)
{
	MQTTHeader header = {0};
	unsigned char* curdata = buf;
	unsigned char* enddata = NULL;
	int rc = 0;
	int mylen;

	header.byte = readChar(&curdata);
	*dup = header.bits.dup;
	*packettype = header.bits.type;

	curdata += (rc = MQTTPacket_decodeBuf(curdata, &mylen)); /* read remaining length */
	enddata = curdata + mylen;

	if (enddata - curdata < 2)
		goto exit;
	*packetid = readInt(&curdata);

	rc = 1;
exit:
	return rc;
}

