
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

import java.io.BufferedReader;
import java.io.IOException;
import java.io.StringReader;
import java.util.Hashtable;


public class HttpRequestParser {

    private String _requestLine;
    private String _method;
    private String _uri;
    private String _cgi_path;
    private Hashtable<String, String> _requestHeaders;
    private Hashtable<String, String> _queryStrings;
    private StringBuffer _messagetBody;

    public HttpRequestParser() {
        _requestHeaders = new Hashtable<String, String>();
        _queryStrings = new Hashtable<String, String>();
        _messagetBody = new StringBuffer();
    }

    /**
     * Parse HTTP request.
     */
    public void parseRequest(String request) throws IOException, HttpFormatException {
        BufferedReader reader = new BufferedReader(new StringReader(request));

        setRequestLine(reader.readLine()); // Request-Line
        parseRequestLine();

        String header = reader.readLine();
        while (header.length() > 0) {
            appendHeaderParameter(header);
            header = reader.readLine();
        }

        String bodyLine = reader.readLine();
        while (bodyLine != null) {
            appendMessageBody(bodyLine);
            bodyLine = reader.readLine();
        }
    }

    public String getRequestLine() {
        return _requestLine;
    }

    public String getMethod() {
        return _method;
    }

    public String getURI() {
        return _uri;
    }

    public String getCgiPath()  { return _cgi_path; }

    public void parseRequestLine() throws HttpFormatException {
        int pos = _requestLine.indexOf(' ');
        if (pos < 0) {
            throw new HttpFormatException("Invalid Request-Line: " + _requestLine);
        }

        _method = _requestLine.substring(0, pos);
        int pos2 = pos;
        while (pos2 < _requestLine.length() && _requestLine.charAt(pos2) == ' ') pos2++;
        if (pos2 >=  _requestLine.length()) {
            throw new HttpFormatException("Invalid Request-Line: " + _requestLine);
        }

        int pos3 = _requestLine.indexOf(' ', pos2);
        if (pos3 < 0) {
            throw new HttpFormatException("Invalid Request-Line: " + _requestLine);
        }
        _uri = _requestLine.substring(pos2, pos3);
        parseQueryString();
    }

    public void parseQueryString() {
        int pos = _uri.indexOf('?');
        if (pos < 0) {
            _cgi_path = _uri;
            return;
        }

        _cgi_path = _uri.substring(0, pos);
        int nextPos = pos;
        do {
            nextPos = _uri.indexOf('&', pos+1);
            String part;
            if (nextPos >= 0) {
                part = _uri.substring(pos + 1, nextPos);
            } else {
                part = _uri.substring(pos + 1);
            }
            int idx = part.indexOf('=');
            if (idx >= 0) {
                _queryStrings.put(part.substring(0, idx), part.substring(idx+1));
            } else {
                _queryStrings.put(part.substring(0, idx), "");
            }

            pos = nextPos;
        } while (nextPos >= 0);
    }

    public String getQueryString(String key) {
        if (!_queryStrings.containsKey(key))
            return null;

        return _queryStrings.get(key);
    }

    private void setRequestLine(String requestLine) throws HttpFormatException {
        if (requestLine == null || requestLine.length() == 0) {
            throw new HttpFormatException("Invalid Request-Line: " + requestLine);
        }
        _requestLine = requestLine;
    }

    private void appendHeaderParameter(String header) throws HttpFormatException {
        int idx = header.indexOf(":");
        if (idx == -1) {
            throw new HttpFormatException("Invalid Header Parameter: " + header);
        }
        _requestHeaders.put(header.substring(0, idx), header.substring(idx + 1, header.length()));
    }

    /**
     * The message-body (if any) of an HTTP message is used to carry the
     * entity-body associated with the request or response. The message-body
     * differs from the entity-body only when a transfer-coding has been
     * applied, as indicated by the Transfer-Encoding header field (section
     * 14.41).
     * @return String with message-body
     */
    public String getMessageBody() {
        return _messagetBody.toString();
    }

    private void appendMessageBody(String bodyLine) {
        _messagetBody.append(bodyLine).append("\r\n");
    }

    /**
     * For list of available headers refer to sections: 4.5, 5.3, 7.1 of RFC 2616
     * @param headerName Name of header
     * @return String with the value of the header or null if not found.
     */
    public String getHeaderParam(String headerName){
        return _requestHeaders.get(headerName);
    }
}
