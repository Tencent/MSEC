
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


package org.msec.rpc;


public class RpcRequest {

    public enum SerializeMode {
        SERIALIZE_MODE_PROTOBUF,
        SERIALIZE_MODE_HTTP
    }
    private long   seq;
    private String  httpCgiName;
    private String  serviceName;  //服务名
    private String  methodName;   //方法名
    private Class<?> parameterType;  //参数类型
    private Object  parameter;         //具体参数
    private long    sendTime;
    private long   flowid;       //flow id
    private String  fromModule;
    private SerializeMode  serializeMode;
    private Exception exception;   //异常信息

    public long getSeq() {
        return seq;
    }

    public void setSeq(long seq) {
        this.seq = seq;
    }

    public String getHttpCgiName() {
        return httpCgiName;
    }

    public void setHttpCgiName(String httpCgiName) {
        this.httpCgiName = httpCgiName;
    }

    public String getServiceName() {
        return serviceName;
    }

    public void setServiceName(String serviceName) {
        this.serviceName = serviceName;
    }

    public String getMethodName() {
        return methodName;
    }

    public void setMethodName(String methodName) {
        this.methodName = methodName;
    }

    public Class<?> getParameterType() {    return parameterType;   }

    public void setParameterType(Class<?> parameterType) {  this.parameterType = parameterType; }

    public Object getParameter() {  return parameter;   }

    public void setParameter(Object parameter) {    this.parameter = parameter; }

    public long getSendTime() {     return sendTime;    }

    public void setSendTime(long sendTime) {    this.sendTime = sendTime;   }

    public long getFlowid() {  return flowid;  }

    public void setFlowid(long flowid) {  this.flowid = flowid;  }

    public String getFromModule() { return fromModule; }

    public void setFromModule(String fromModule) { this.fromModule = fromModule; }

    public SerializeMode getSerializeMode() {
        return serializeMode;
    }

    public void setSerializeMode(SerializeMode serializeMode) {
        this.serializeMode = serializeMode;
    }

    public Exception getException() {
        return exception;
    }

    public void setException(Exception exception) {
        this.exception = exception;
    }

    @Override
    public String toString() {
        return "RpcRequest{" +
                "seq=" + seq +
                ", httpCgiName='" + httpCgiName + '\'' +
                ", serviceName='" + serviceName + '\'' +
                ", methodName='" + methodName + '\'' +
                ", parameterType=" + parameterType +
                ", parameter=" + parameter +
                ", sendTime=" + sendTime +
                ", flowid=" + flowid +
                ", fromModule='" + fromModule + '\'' +
                ", serializeMode=" + serializeMode +
                ", exception=" + exception +
                '}';
    }
}
