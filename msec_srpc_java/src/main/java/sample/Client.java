
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

import org.msec.rpc.ServiceFactory;

public class Client {
    public static void main(String[] args)  {
        $(PROTO_REPLACE).$(REQUEST_REPLACE).Builder requestBuilder = $(PROTO_REPLACE).$(REQUEST_REPLACE).newBuilder();
         
        //Add your code here: build your request
        

        $(PROTO_REPLACE).$(REQUEST_REPLACE)  request = requestBuilder.build();

        try {
            $(PROTO_REPLACE).$(RESPONSE_REPLACE) response = ($(PROTO_REPLACE).$(RESPONSE_REPLACE)) ServiceFactory.callMethod("$(MODULE_REPLACE)", "$(PKG_REPLACE).$(SERVICE_REPLACE).$(METHOD_REPLACE)",
                    request, $(PROTO_REPLACE).$(RESPONSE_REPLACE).getDefaultInstance(), 3000);

            System.out.println("Request:\n" + request + "Response:\n" + response);

        } catch (Exception ex) {
            System.out.println("Exception occurs: " + ex);
            ex.printStackTrace();
        }
    }
}

