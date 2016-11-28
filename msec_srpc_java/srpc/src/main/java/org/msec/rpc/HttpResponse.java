
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

import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;


public class HttpResponse {

    private String  statusCode;
    private Map<String, List<String>> headers = new HashMap<String, List<String>>();
    private String contentType;
    private String body;

    public Map<String, List<String>> getHeaders() {
        return headers;
    }

    public void setHeaders(Map<String, List<String>> headers) {
        this.headers = headers;
    }

    public String getStatusCode() {
        return statusCode;
    }

    public void setStatusCode(String statusCode) {
        this.statusCode = statusCode;
    }

    public String  getContentType() {
        return contentType;
    }

    public void setContentType(String contentType) {
        this.contentType = contentType;
    }

    public String getBody() {
        return body;
    }

    public void setBody(String body) {
        this.body = body;
    }

    public String  write() {
        StringBuilder  builder = new StringBuilder();
        builder.append("HTTP/1.1 ");
        builder.append(statusCode + "\r\n");

        builder.append("Server: msec srpc server\r\n");
        builder.append("Date: " + new Date() + "\r\n");
        builder.append("Content-Type: " + contentType + "\r\n");
        builder.append("Content-Length: " + body.length() + "\r\n\r\n");

        builder.append(body);
        return builder.toString();
    }

    @Override
    public String toString() {
        return "responseCode = " + statusCode + ", body=" + body + ", headers = " + headers;
    }
}

