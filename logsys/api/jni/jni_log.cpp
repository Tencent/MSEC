
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
// Created by bison on 16-5-19.
//
#include "jni_log.h"
#include "logsys_api.h"

using namespace msec;

JNIEXPORT jboolean JNICALL Java_api_log_msec_org_AccessLog_init
        (JNIEnv * env, jobject me, jstring cfgFile)
{
    const char * configFile = env->GetStringUTFChars(cfgFile , NULL);

    LogsysApi * api = LogsysApi::GetInstance();
    jboolean  result ;
    if (api->Init(configFile) == 0)
    {
        result = true;
    }
    else
    {
        result = false;
    }



    env->ReleaseStringUTFChars(cfgFile, configFile);
    return result;

}


JNIEXPORT void JNICALL Java_api_log_msec_org_AccessLog_setHeader
(JNIEnv * env, jobject me, jstring key, jstring value)
{
    const char * c_key = env->GetStringUTFChars(key , NULL);
    const char * c_value = env->GetStringUTFChars(value, NULL);

    //printf("setHeader:%s->%s\n", c_key, c_value);

    LogsysApi * api = LogsysApi::GetInstance();
    if (api == NULL)
    {
        //printf("failed to GetInstance()\n");
        env->ReleaseStringUTFChars(key, c_key);
        env->ReleaseStringUTFChars(value, c_value);
        return;
    }

    api->LogSetHeader(c_key, c_value);

    env->ReleaseStringUTFChars(key, c_key);
    env->ReleaseStringUTFChars(value, c_value);
}


JNIEXPORT void JNICALL Java_api_log_msec_org_AccessLog_log
(JNIEnv *env , jobject me, jint level , jstring body)
{
    const char * c_body = env->GetStringUTFChars(body , NULL);

    //printf("log:body=%s, level=%d\n", c_body, level);


    LogsysApi * api = LogsysApi::GetInstance();
    if (api == NULL)
    {
        //printf("failed to GetInstance()\n");
        env->ReleaseStringUTFChars(body, c_body);
        return;
    }

    int ret = api->Log(level, c_body);
    if (ret != 0) 
    {
        printf("logsys api err ret=%d\n", ret);
    }

    env->ReleaseStringUTFChars(body, c_body);

}
