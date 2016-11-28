
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

import java.util.concurrent.TimeUnit;

import api.monitor.msec.org.AccessMonitor;
import org.apache.log4j.Logger;
import org.msec.net.NettyClient;

public class CallbackFuture implements Callback {
	private static Logger log = Logger.getLogger(CallbackFuture.class.getName());

	protected RpcRequest  request;
	protected NettyClient client;
	protected RpcResponse response;
	private boolean done = false;
	private boolean success = false;

	public CallbackFuture(RpcRequest request, NettyClient client) {
		this.request = request;
		this.client = client;
	}

	public void OnResponse(RpcResponse response) {
		synchronized (this) {
			this.response = response;
			this.done = true;
			this.success = true;
			this.notifyAll();
		}
	}

	public void callback(RpcResponse response) {
		this.response = response;
	}

	public RpcResponse getResponse()  {
		return getResponse(Long.MAX_VALUE);
	}

	public RpcResponse getResponse(long timeout, TimeUnit unit)  {
		return getResponse(unit.toMillis(timeout));
	}

	public RpcResponse getResponse(long timeoutMillis) throws  RequestTimeoutException {
		synchronized (this) {
			long start = System.currentTimeMillis();
			while (!this.done) {
				long timeoutMillis_ = timeoutMillis - (System.currentTimeMillis() - start);
				if (timeoutMillis_ <= 0) {
					StringBuilder sb = new StringBuilder();
					sb.append("timeout for request to ").append(request.getServiceName())
							.append("(").append(request.getMethodName()).append(")")
							.append(" addr: ").append(client.getHost()).append(":").append(client.getPort())
							.append("\r\nrequest:").append(request.getParameter());
					AccessMonitor.add("frm.rpc " + request.getServiceName() + "/" + request.getMethodName() + " timeout");
					RequestTimeoutException e = new RequestTimeoutException(sb.toString());
					throw e;
				} else {
					try {
						this.wait(timeoutMillis_);
					} catch (Exception ex){
					}
				}
			}

			AccessMonitor.add("frm.rpc " + request.getServiceName() + "/" + request.getMethodName() + " got response");
			return this.response;
		}
	}

	public boolean isDone() {
		return this.done;
	}

	public NettyClient getClient() {
		return this.client;
	}
}