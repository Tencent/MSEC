
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


package $(PKG_REPLACE);

import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.RpcController;
import com.google.protobuf.ServiceException;
import org.msec.rpc.ServiceFactory;

public class ServiceImpl implements $(INTERFACE_LIST_REPLACE) {

    public static void main(String[] args) throws Exception {
        ServiceFactory.initModule("$(MODULE_REPLACE)", ServiceFactory.SRPC_VERSION);
    $(SERVICE_REGISTER_BEGIN)
        ServiceFactory.addService("$(PKG_REPLACE).$(SERVICE_REPLACE)", $(PROTO_REPLACE).$(SERVICE_REPLACE).BlockingInterface.class, new ServiceImpl());
    $(SERVICE_REGISTER_END)
        ServiceFactory.runService();
    }

    $(METHOD_IMPL_BEGIN)
    public $(PROTO_REPLACE).$(RESPONSE_REPLACE) $(METHOD_REPLACE)(RpcController controller, $(PROTO_REPLACE).$(REQUEST_REPLACE) request) throws ServiceException {
	//Add your code here
        return null;
    }
    $(METHOD_IMPL_END)
		
}

