
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

import api.log.msec.org.AccessLog;
import api.monitor.msec.org.AccessMonitor;
import com.google.protobuf.Message;
import com.google.protobuf.MessageLite;
import com.google.protobuf.Parser;
import org.apache.log4j.Logger;
import org.apache.log4j.PropertyConfigurator;
import org.ini4j.Ini;
import org.ini4j.Profile;
import org.msec.net.NettyClient;
import org.msec.net.NettyServer;

import java.io.File;
import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;


public final class ServiceFactory {
    private static Logger log = Logger.getLogger(ServiceFactory.class.getName());

    public static final int SRPC_VERSION = 10000;
    public static final int DEFAULT_PORT = 7963;
    public static final String SRPC_CONF_PATH = "etc/config.ini";
    public static final String LOG4J_CONF_PATH = "etc/log4j.properties";
    public static final String MONITOR_AGENT_MMAP_PATH = "/msec/agent/monitor/monitor.mmap";

    public static long uniqueSequence = 0;
    public static synchronized long generateSequence() {
        if (uniqueSequence == 0) {
            uniqueSequence = new Random(System.currentTimeMillis()).nextLong();
            uniqueSequence = uniqueSequence & 0x7fffffff;
            if (uniqueSequence == 0)
                ++uniqueSequence;
        }
        return uniqueSequence++;
    }

    public static String getVersionString(int version) {
        int major = version / 10000;
        int minor = (version % 10000) / 100;
        int patch = version % 100;
        return Integer.toString(major) + "." + minor + "." + patch;
    }

    protected static String moduleName = null;
    protected static String listenIP = null;
    protected static int listenPort = DEFAULT_PORT;
    protected static String listenType = "tcp";
    protected static boolean initSucc = false;
    public static synchronized  void initModule(String moduleName, int version)  {
        PropertyConfigurator.configure(LOG4J_CONF_PATH);

        try {
            if (version != SRPC_VERSION) {
                throw new Exception("SRPC version not compatible: " + getVersionString(version) +
                        ", " + getVersionString(SRPC_VERSION));
            }

            log.info("SRPC version: " + getVersionString(SRPC_VERSION));
            if (ServiceFactory.moduleName == null) {
                ServiceFactory.moduleName = moduleName;
                AccessLog.initLog(SRPC_CONF_PATH);
                AccessMonitor.initialize(MONITOR_AGENT_MMAP_PATH);
                AccessMonitor.initServiceName(moduleName);

            }

            parseListenConf();
            initSucc = true;
        } catch (Exception e) {
            log.error("init module failed. ", e);
        }
    }

    public static String getModuleName() {
        if (moduleName != null && !moduleName.isEmpty())
            return ServiceFactory.moduleName;
        else
            return "Noname-Module";
    }

    public static void parseListenConf() throws Exception {
        String listenConf = getConfig("SRPC", "listen");
        if (listenConf != null) {
            int pos = listenConf.indexOf(' ');
            if (pos >= 0)
                listenConf = listenConf.substring(0, pos);

            pos = listenConf.indexOf(':');
            if (pos > 0) {
                listenIP = listenConf.substring(0, pos);
                listenConf = listenConf.substring(pos+1);
            }

            pos = listenConf.indexOf('/');
            String listenPortStr = listenConf;
            if (pos > 0) {
                listenPortStr = listenConf.substring(0, pos);
                listenType = listenConf.substring(pos+1);
                if (listenType.compareToIgnoreCase("tcp") != 0
                        && listenType.compareToIgnoreCase("udp") != 0)
                {
                    throw new Exception("Invalid listen type: " + listenType);
                }
            }

            try {
                listenPort = Integer.valueOf(listenPortStr);
            } catch (NumberFormatException ex) {
                throw new Exception("Invalid listen port: " + listenPortStr);
            }

            if (listenIP != null && !listenIP.isEmpty()) {
                Enumeration interfaces = NetworkInterface.getNetworkInterfaces();
                while (interfaces.hasMoreElements()) {
                    NetworkInterface intf = (NetworkInterface) interfaces.nextElement();
                    if (intf.getName().compareToIgnoreCase(listenIP) != 0)
                        continue;
                    // Enumerate InetAddresses of this network interface
                    Enumeration addresses = intf.getInetAddresses();
                    while (addresses.hasMoreElements()) {
                        InetAddress address = (InetAddress) addresses.nextElement();
                        listenIP = address.getHostAddress();
                        break;
                    }
                }
            }
        }
    }
    public static String getConfig(String section, String key) {
        Ini ini = new Ini();
        try {
            ini.load(new File(SRPC_CONF_PATH));
            Profile.Section sec = ini.get(section);
            if (sec != null) {
                return sec.get(key);
            }
        } catch (IOException e) {
            log.error("get config " + section + ":" + key + " failed.", e);
        }
        return null;
    }

    protected static Map<String, Map<String, List<ServiceMethodEntry>>>   serviceMethodMap = new ConcurrentHashMap<String, Map<String, List<ServiceMethodEntry>>>();
    public static final class ServiceMethodEntry {
        private Object   service;
        private Method   method;
        private Class<?>  paramType;
        private Class<?>  returnType;
        private Parser    paramTypeParser;
        private Parser    returnTypeParser;
        private Message.Builder  paramTypeBuilder;

        public ServiceMethodEntry(Object service, Method method) throws NoSuchMethodException, InvocationTargetException, IllegalAccessException {
            this.service = service;
            this.method = method;
            this.paramType = method.getParameterTypes()[1];
            this.returnType = method.getReturnType();
            this.paramTypeParser = ((MessageLite)this.paramType.getMethod("getDefaultInstance").invoke(null)).getParserForType();
            this.returnTypeParser = ((MessageLite)this.returnType.getMethod("getDefaultInstance").invoke(null)).getParserForType();
            this.paramTypeBuilder = (Message.Builder)this.paramType.getMethod("newBuilder").invoke(null);
        }
        public Object invoke(Object argument) throws InvocationTargetException, IllegalAccessException {
            return method.invoke(service, null, argument);
        }

        public Object getService() {    return service; }
        public Method getMethod() {     return method;  }

        public Class<?> getParamType() {    return paramType;   }
        public Class<?> getReturnType() {   return returnType;  }

        public Parser getParamTypeParser() {    return paramTypeParser; }
        public Parser getReturnTypeParser() {   return returnTypeParser;}
        public Message.Builder getParamTypeBuilder() {  return paramTypeBuilder;    }

        @Override
        public String toString() {
            return "ServiceMethodEntry{" +  "service=" + service +  ", method=" + method +  ", paramType=" + paramType +
                    ", returnType=" + returnType + '}';
        }
    }

    public static Set<String> ignoreMethodNames = new HashSet<String>();

    static {
        Method[] methods1 = Object.class.getMethods();
        for (Method oneMethod: methods1) {
            ignoreMethodNames.add(oneMethod.getName());
        }

        Method[] methods2 = Class.class.getMethods();
        for (Method oneMethod: methods2) {
            ignoreMethodNames.add(oneMethod.getName());
        }
        ignoreMethodNames.add("main");
    }

    public static Map<String, Map<String, List<ServiceMethodEntry>>>  getServiceMethodMap() {
        return serviceMethodMap;
    }

    public static void runService() {
        try {
            if (!initSucc)  return;

            NettyServer server;
            if (listenIP == null) {
                server = new NettyServer(listenPort);
            } else {
                server = new NettyServer(listenIP, listenPort);
            }
            server.start();

            System.in.read();
        } catch (Exception e) {
            log.error("run service failed. ", e);
        }
    }

    public static <T> void addService(Class<T> serviceClass, T serviceObj) {
        addService(serviceClass.getCanonicalName(), serviceClass, serviceObj);
    }

    public static <T> void addService(String serviceKeyName, Class<T> serviceClass, T serviceObj) {
        addService(serviceKeyName, serviceClass, serviceObj, DEFAULT_PORT);
    }

    public static <T> void addService(String serviceKeyName, Class<T> serviceClass, T serviceObj, int port) {
        Map<String, List<ServiceMethodEntry>>  methodMap = serviceMethodMap.get(serviceKeyName);
        if (methodMap == null) {
            methodMap = new ConcurrentHashMap<String, List<ServiceMethodEntry>>();
            serviceMethodMap.put(serviceKeyName, methodMap);
        }

        List<ServiceMethodEntry>  entries;
        Method[]  allMethods = serviceObj.getClass().getMethods();
        String paramTypeName = null;
        String returnTypeName = null;
        for (Method oneMethod: allMethods) {
            if (!ignoreMethodNames.contains(oneMethod.getName())) {
                oneMethod.setAccessible(true);

                entries = methodMap.get(oneMethod.getName());
                if (entries == null) {
                    entries = new ArrayList<ServiceMethodEntry>();
                    methodMap.put(oneMethod.getName(), entries);
                }

                try {
                    paramTypeName = oneMethod.getParameterTypes()[1].getName();
                    returnTypeName = oneMethod.getReturnType().getName();
                    Class.forName(paramTypeName);
                    Class.forName(returnTypeName);

                    entries.add(new ServiceMethodEntry(serviceObj, oneMethod));
                } catch (Exception e) {
                    log.error("Load class failed: " + paramTypeName + " " + returnTypeName, e);
                }
            }
        }
    }

    public static String underscoresToCamelCase(String input, boolean cap_next_letter) {
        String result = "";
        for (int i = 0; i < input.length(); i++) {
            if ('a' <= input.charAt(i) && input.charAt(i) <= 'z') {
                if (cap_next_letter) {
                    result += Character.toUpperCase(input.charAt(i));
                } else {
                    result += input.charAt(i);
                }
                cap_next_letter = false;
            } else if ('A' <= input.charAt(i) && input.charAt(i) <= 'Z') {
                if (i == 0 && !cap_next_letter) {
                    result += Character.toLowerCase(input.charAt(i));
                } else {
                    result += input.charAt(i);
                }
                cap_next_letter = false;
            } else if ('0' <= input.charAt(i) && input.charAt(i) <= '9') {
                result += input.charAt(i);
                cap_next_letter = true;
            } else {
                cap_next_letter = true;
            }
        }
        return result;
    }

    public static ServiceMethodEntry getServiceMethodEntry(String serviceKeyName, String methodName) {
        Map<String, List<ServiceMethodEntry>>  methodMap = serviceMethodMap.get(serviceKeyName);
        if (methodMap == null) {
            return null;
        }

        List<ServiceMethodEntry>  entries = methodMap.get(methodName);
        if (entries == null) {
            entries = methodMap.get(underscoresToCamelCase(methodName, false));
            if (entries == null)
                return null;
        }

        return entries.get(0);
    }

    public static MessageLite callMethod(String moduleName, String serviceMethodName, MessageLite request, MessageLite responseInstance, int timeoutMillis) throws Exception {
        MessageLite ret = null;
        NettyClient client = null;
        //System.out.println("GetClient begin: " + System.currentTimeMillis());
        try {
            client = ClientManager.getClient(moduleName, true);
        } catch (Exception e) {
            log.error("get route for [" + moduleName + "] failed.", e);
            return ret;
        }

        //System.out.println("AfterConnect: " + System.currentTimeMillis());
        int pos = serviceMethodName.lastIndexOf('.');
        if (pos == -1 || pos == 0 || pos == serviceMethodName.length() - 1) {
            throw new Exception("Invalid serviceMethodName. Must be in format like *.*");
        }
        String serviceName  = serviceMethodName.substring(0, pos);
        String methodName = serviceMethodName.substring(pos + 1);
        RpcRequest rpcRequest = new RpcRequest();
        rpcRequest.setSeq(generateSequence());
        rpcRequest.setServiceName(serviceName);
        rpcRequest.setMethodName(methodName);
        rpcRequest.setParameter(request);

        RpcContext  context = (RpcContext)RequestProcessor.getThreadContext("session");
        if (context != null) {
            rpcRequest.setFlowid(context.getRequest().getFlowid());
        }
        RpcResponse rpcResponse = client.sendRequestAndWaitResponse(rpcRequest, timeoutMillis);

        if (rpcResponse != null) {
            if (rpcResponse.getErrno() == 0) {
                ret = responseInstance.getParserForType().parseFrom((byte[]) rpcResponse.getResultObj());
                return ret;
            } else if (rpcResponse.getError() != null){
                throw rpcResponse.getError();
            } else {
                throw new Exception("Rpc failed: errno = " + rpcResponse.getErrno());
            }
        }
        return ret;
    }

    //������ʹ�ó����ӣ�������ʵ��SEQ���ƣ��Է�����
    public static byte[] callMethod(String moduleName, String serviceMethodName,
                                    CustomPackageCodec packageCodec, int timeoutMillis) throws Exception {
        byte[] ret = null;
        NettyClient client = null;
        try {
            client = ClientManager.getClient(moduleName, true);
        } catch (Exception e) {
            log.error("get route for [" + moduleName + "] failed.", e);
            return ret;
        }

        int pos = serviceMethodName.lastIndexOf('.');
        if (pos == -1 || pos == 0 || pos == serviceMethodName.length() - 1) {
            throw new Exception("Invalid serviceMethodName. Must be in format like *.*");
        }
        String serviceName  = serviceMethodName.substring(0, pos);
        String methodName = serviceMethodName.substring(pos + 1);
        RpcRequest rpcRequest = new RpcRequest();
        rpcRequest.setSeq(generateSequence());
        rpcRequest.setServiceName(serviceName);
        rpcRequest.setMethodName(methodName);
        rpcRequest.setParameter(null);
        RpcResponse rpcResponse = client.sendRequestAndWaitResponse(packageCodec, rpcRequest, timeoutMillis);
        if (rpcResponse != null) {
            if (rpcResponse.getErrno() == 0) {
                ret = (byte[]) rpcResponse.getResultObj();
                return ret;
            } else if (rpcResponse.getError() != null){
                throw rpcResponse.getError();
            } else {
                throw new Exception("Rpc failed: errno = " + rpcResponse.getErrno());
            }
        }
        return ret;
    }

    //������ʹ�ö�����
    public static byte[] callMethod(String moduleName, String serviceMethodName,
                                    CustomPackageLengthChecker lengthChecker, byte[] requestData, int timeoutMillis) throws Exception {
        byte[] ret = null;
        NettyClient client = null;

        try {
            client = ClientManager.getClient(moduleName, false);
        } catch (Exception e) {
            log.error("get route for [" + moduleName + "] failed.", e);
            return ret;
        }

        //System.out.println("AfterConnect: " + System.currentTimeMillis());
        int pos = serviceMethodName.lastIndexOf('.');
        if (pos == -1 || pos == 0 || pos == serviceMethodName.length() - 1) {
            throw new Exception("Invalid serviceMethodName. Must be in format like *.*");
        }
        String serviceName  = serviceMethodName.substring(0, pos);
        String methodName = serviceMethodName.substring(pos + 1);
        RpcRequest rpcRequest = new RpcRequest();
        rpcRequest.setSeq(generateSequence());
        rpcRequest.setServiceName(serviceName);
        rpcRequest.setMethodName(methodName);
        rpcRequest.setParameter(null);
        RpcResponse rpcResponse = client.sendRequestAndWaitResponse(lengthChecker, requestData, rpcRequest, timeoutMillis);
        //System.out.println("WaitFinished: " + System.currentTimeMillis());
        client.close();
        if (rpcResponse != null) {
            if (rpcResponse.getErrno() == 0) {
                ret = (byte[]) rpcResponse.getResultObj();
                return ret;
            } else if (rpcResponse.getError() != null){
                throw rpcResponse.getError();
            } else {
                throw new Exception("Rpc failed: errno = " + rpcResponse.getErrno());
            }
        }
        return ret;
    }
}
