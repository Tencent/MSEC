
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


package org.msec;

import java.util.ArrayList;
import java.util.List;


public class LogsysRsp {

    public QueryLogRsp queryLogRsp;
    public ModifyFieldsRsp modifyFieldsRsp;
    public GetFieldsRsp getFieldsRsp;
    public CallGraphRsp callGraphRsp;

    public static class CallPair
    {
        protected  String from;
        protected  String to;
        protected  String rpcname;

        public String getFrom() {
            return from;
        }

        public void setFrom(String from) {
            this.from = from;
        }

        public String getTo() {
            return to;
        }

        public void setTo(String to) {
            this.to = to;
        }

        public String getRpcname() {
            return rpcname;
        }

        public void setRpcname(String rpcname) {
            this.rpcname = rpcname;
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) return true;
            if (o == null || getClass() != o.getClass()) return false;

            CallPair callPair = (CallPair) o;

            if (!from.equals(callPair.from)) return false;
            if (!to.equals(callPair.to)) return false;
            return rpcname != null ? rpcname.equals(callPair.rpcname) : callPair.rpcname == null;

        }

        @Override
        public int hashCode() {
            int result = from.hashCode();
            result = 31 * result + to.hashCode();
            result = 31 * result + (rpcname != null ? rpcname.hashCode() : 0);
            return result;
        }
    }

    public static class QueryLogRsp
    {
        protected int ret;
        protected String errmsg;
        protected int lines;
        protected String querySql;
        protected List<String> heads = new ArrayList<String>();
        protected List<Object> records = new ArrayList<Object>();

        public int getRet() {
            return ret;
        }

        public void setRet(int ret) {
            this.ret = ret;
        }

        public String getErrmsg() {
            return errmsg;
        }

        public void setErrmsg(String errmsg) {
            this.errmsg = errmsg;
        }

        public int getLines() {
            return lines;
        }

        public void setLines(int lines) {
            this.lines = lines;
        }

        public String getQuerySql() {
            return querySql;
        }

        public void setQuerySql(String querySql) {
            this.querySql = querySql;
        }

        public List<String> getHeads() {  return heads; }

        public void setHeads(List<String> heads) {  this.heads = heads; }

        public List<Object> getRecords() {
            return records;
        }

        public void setRecords(List<Object> records) {
            this.records = records;
        }

        @Override
        public String toString() {
            return "QueryLogRsp{" +
                    "ret=" + ret +
                    ", errmsg='" + errmsg + '\'' +
                    ", lines=" + lines +
                    ", querySql='" + querySql + '\'' +
                    ", records='" + records + '\'' +
                    '}';
        }
    }

    public static class ModifyFieldsRsp
    {
        protected int ret;
        protected String errmsg;

        public int getRet() {
            return ret;
        }

        public void setRet(int ret) {
            this.ret = ret;
        }

        public String getErrmsg() {
            return errmsg;
        }

        public void setErrmsg(String errmsg) {
            this.errmsg = errmsg;
        }

        @Override
        public String toString() {
            return "ModifyFieldsRsp{" +
                    "ret=" + ret +
                    ", errmsg='" + errmsg + '\'' +
                    '}';
        }
    }

    public static class GetFieldsRsp
    {
        protected int ret;
        protected String errmsg;
        protected List<LogField> fieldsInfo = new ArrayList<LogField>();

        public int getRet() {
            return ret;
        }

        public void setRet(int ret) {
            this.ret = ret;
        }

        public String getErrmsg() {
            return errmsg;
        }

        public void setErrmsg(String errmsg) {
            this.errmsg = errmsg;
        }

        public List<LogField> getFieldsInfo() {
            return fieldsInfo;
        }

        public void setFieldsInfo(List<LogField> fieldsInfo) {
            this.fieldsInfo = fieldsInfo;
        }

        @Override
        public String toString() {
            return "GetFieldsRsp{" +
                    "ret=" + ret +
                    ", errmsg='" + errmsg + '\'' +
                    ", fieldsInfo=" + fieldsInfo +
                    '}';
        }
    }

    public static class CallGraphRsp
    {
        protected int ret;
        protected String errmsg;
        protected String graph;

        public int getRet() {
            return ret;
        }

        public void setRet(int ret) {
            this.ret = ret;
        }

        public String getErrmsg() {
            return errmsg;
        }

        public void setErrmsg(String errmsg) {
            this.errmsg = errmsg;
        }

        public String getGraph() {
            return graph;
        }

        public void setGraph(String graph) {
            this.graph = graph;
        }

        @Override
        public String toString() {
            return "CallGraphRsp{" +
                    "ret=" + ret +
                    ", errmsg='" + errmsg + '\'' +
                    ", graph='" + graph + '\'' +
                    '}';
        }
    }
}
