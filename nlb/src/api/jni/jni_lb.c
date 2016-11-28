
/**
 * Tencent is pleased to support the open source community by making MSEC available.
 *
 * Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
 *
 * Licensed under the GNU General Public License, Version 2.0 (the "License"); 
 * you may not use this file except in compliance with the License. You may 
 * obtain a copy of the License at
 *
 *     https://opensource.org/licenses/GPL-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the 
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions
 * and limitations under the License.
 */


//
// Created by bison on 16-5-18.
//
#include <memory.h>
#include <nlbapi.h>
#include <arpa/inet.h>
#include "jni_lb.h"

static void ip2str(uint32_t ip, char * str, int32_t len)
{
	unsigned char * p = (unsigned char*)&ip;
	
	snprintf(str, len, "%u.%u.%u.%u", p[0], p[1], p[2], p[3]);
}

static uint32_t str2ip(const char * str)
{
    struct in_addr addr;
    inet_aton(str, &addr);

    return addr.s_addr;
}



JNIEXPORT jboolean JNICALL Java_api_lb_msec_org_AccessLB_getroutebyname
        (JNIEnv * env, jobject me,
         jstring name, jbyteArray ip, jbyteArray port, jbyteArray type)
{
    const char * c_name = (*env)->GetStringUTFChars(env,  name , NULL);

    //printf("in C, search name:%s...\n", c_name);

    char result_ip[100] ;
    char result_port[100] ;
    char result_type[100] ;

    //call nlb api to get route
    struct routeid route;
    if (getroutebyname(c_name, &route))
	{
        (*env)->ReleaseStringUTFChars(env, name, c_name);
		return 0;// return false
	}

    // convert route into string type
	ip2str(route.ip, result_ip, (int32_t)sizeof(result_ip));
    sprintf(result_port, "%d", route.port);
	if (route.type == NLB_PORT_TYPE_UDP)
	{
		strcpy(result_type, "udp");
	}
	if (route.type == NLB_PORT_TYPE_TCP)
	{
		strcpy(result_type, "tcp");
	}
	if (route.type == NLB_PORT_TYPE_ALL)
	{
		strcpy(result_type, "all");
	}

    // copy byte by byte into out parameters
    jbyte * ip_bytes;
    jbyte *  port_bytes;
    jbyte * type_bytes;
    int i;

    ip_bytes = (*env)->GetByteArrayElements(env, ip, NULL);
    for (i = 0; i < strlen(result_ip); ++i)
    {
        ip_bytes[i] = result_ip[i];
    }
    (*env)->ReleaseByteArrayElements(env, ip, ip_bytes, 0);

    port_bytes = (*env)->GetByteArrayElements(env, port, NULL);
    for (i = 0; i < strlen(result_port); ++i)
    {
        port_bytes[i] = result_port[i];
    }
    (*env)->ReleaseByteArrayElements(env, port, port_bytes, 0);

    type_bytes = (*env)->GetByteArrayElements(env, type, NULL);
    for (i = 0; i < strlen(result_type); ++i)
    {
        type_bytes[i] = result_type[i];
    }
    (*env)->ReleaseByteArrayElements(env, type, type_bytes, 0);

    (*env)->ReleaseStringUTFChars(env, name, c_name);


    return 1;// return true



}

JNIEXPORT jboolean JNICALL Java_api_lb_msec_org_AccessLB_updateroute
        (JNIEnv *env, jobject me, jstring name, jstring ip, jint failed, jint cost)
{

    const char * c_name = (*env)->GetStringUTFChars(env,  name , NULL);
    const char * c_ip = (*env)->GetStringUTFChars(env,  ip, NULL);

    uint32_t iIP = str2ip(c_ip);



    //printf("updateroute(%s, %u, %d, %d)\n", c_name, iIP, failed, cost);
     int32_t result =  updateroute(c_name, iIP,  failed,cost);
    if (result)
    {
        (*env)->ReleaseStringUTFChars(env, name, c_name);
        (*env)->ReleaseStringUTFChars(env, ip, c_ip);
        return 0;
    }

    (*env)->ReleaseStringUTFChars(env, name, c_name);
    (*env)->ReleaseStringUTFChars(env, ip, c_ip);
    return 1;
}
