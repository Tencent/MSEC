
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


#include <monitor_client.h>
#include "jni_monitor.h"

JNIEXPORT jboolean JNICALL Java_api_monitor_msec_org_AccessMonitor_init
        (JNIEnv *env, jobject me, jstring cfgFile)
{
    const char * c_cfgFile = (*env)->GetStringUTFChars(env, cfgFile, NULL);
    int result = 0;
    if (Monitor_Init(c_cfgFile) == 0)
    {
        result = 1;
    }

    (*env)->ReleaseStringUTFChars(env, cfgFile, c_cfgFile);
    return result;
}

JNIEXPORT jboolean JNICALL Java_api_monitor_msec_org_AccessMonitor_add
        (JNIEnv *env , jobject me, jstring svcname, jstring attrname, jint value)
{
    const char * c_svcname = (*env)->GetStringUTFChars(env ,svcname, NULL);
    const char * c_attrname = (*env)->GetStringUTFChars(env, attrname, NULL);

    int result = 0;
    if (Monitor_Add(c_svcname, c_attrname, value) == 0)
    {
        result = 1;
    }

    (*env)->ReleaseStringUTFChars(env, svcname, c_svcname);
    (*env)->ReleaseStringUTFChars(env, attrname, c_attrname);
    return result;
}

JNIEXPORT jboolean JNICALL Java_api_monitor_msec_org_AccessMonitor_set
        (JNIEnv *env, jobject me, jstring svcname, jstring attrname, jint value)
{
    const char * c_svcname = (*env)->GetStringUTFChars(env ,svcname, NULL);
    const char * c_attrname = (*env)->GetStringUTFChars(env, attrname, NULL);

    int result = 0;
    if (Monitor_Set(c_svcname, c_attrname, value) == 0)
    {
        result = 1;
    }

    (*env)->ReleaseStringUTFChars(env, svcname, c_svcname);
    (*env)->ReleaseStringUTFChars(env, attrname, c_attrname);
    return result;
}

