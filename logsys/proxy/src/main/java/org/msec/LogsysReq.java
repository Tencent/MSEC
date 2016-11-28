
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


public class LogsysReq {

    public QueryLogReq      queryLogReq;
    public ModifyFieldsReq  modifyFieldsReq;
    public GetFieldsReq     getFieldsReq;
    public CallGraphReq     callGraphReq;

    public static class QueryLogReq
    {
        protected String appName;
        protected String logLevel;
        protected List<LogField>  filterFieldList = new ArrayList<LogField>();
        protected int maxRetNum = 300;
        protected String startDate;
        protected String endDate;
        protected String startTime;
        protected String endTime;
        protected String whereCondition;

        //Setter and Getter
        public String getAppName() {
            return appName;
        }

        public void setAppName(String appName) {
            this.appName = appName;
        }

        public String getLogLevel() {
            return logLevel;
        }

        public void setLogLevel(String logLevel) {
            this.logLevel = logLevel;
        }

        public List<LogField> getFilterFieldList() {
            return filterFieldList;
        }

        public void addFilterField(LogField filterField) {
            filterFieldList.add(filterField);
        }

        public int getMaxRetNum() {
            return maxRetNum;
        }

        public void setMaxRetNum(int maxRetNum) {
            this.maxRetNum = maxRetNum;
        }

        public String getStartDate() {
            return startDate;
        }

        public void setStartDate(String startDate) {
            this.startDate = startDate;
        }

        public String getEndDate() {
            return endDate;
        }

        public void setEndDate(String endDate) {
            this.endDate = endDate;
        }

        public String getStartTime() {
            return startTime;
        }

        public void setStartTime(String startTime) {
            this.startTime = startTime;
        }

        public String getEndTime() {
            return endTime;
        }

        public void setEndTime(String endTime) {
            this.endTime = endTime;
        }

        public String getWhereCondition() {
            return whereCondition;
        }

        public void setWhereCondition(String whereCondition) {
            this.whereCondition = whereCondition;
        }

        @Override
        public String toString() {
            return "LogsysReq -- AppName: " + appName + " LogLevel: " + logLevel + " FitlerFieldList: " + filterFieldList.toString()
                    + " MaxRetNum: " + maxRetNum + " DateRange: " + startDate + " " + startTime + "--" + endDate + " " + endTime;
        }
    }

    public static class ModifyFieldsReq
    {
        protected String appName;
        protected String operator;
        protected String fieldName;
        protected String fieldType;
        protected String newFieldName;

        //Setter and Getter
        public String getAppName() {
            return appName;
        }

        public void setAppName(String appName) {
            this.appName = appName;
        }

        public String getOperator() {
            return operator;
        }

        public void setOperator(String operator) {
            this.operator = operator;
        }

        public String getFieldName() {
            return fieldName;
        }

        public void setFieldName(String fieldName) {
            this.fieldName = fieldName;
        }

        public String getFieldType() {
            return fieldType;
        }

        public void setFieldType(String fieldType) {
            this.fieldType = fieldType;
        }

        public String getNewFieldName() {
            return newFieldName;
        }

        public void setNewFieldName(String newFieldName) {
            this.newFieldName = newFieldName;
        }

        @Override
        public String toString() {
            return "ModifyFieldsReq{" +
                    "appName='" + appName + '\'' +
                    ", operator='" + operator + '\'' +
                    ", fieldName='" + fieldName + '\'' +
                    ", fieldType='" + fieldType + '\'' +
                    ", newFieldName='" + newFieldName + '\'' +
                    '}';
        }
    }


    public static class GetFieldsReq {
        protected String appName;

        public String getAppName() {
            return appName;
        }

        public void setAppName(String appName) {
            this.appName = appName;
        }

        @Override
        public String toString() {
            return "GetFieldsReq{" +  "appName='" + appName + '\'' + '}';
        }
    }

    public static class CallGraphReq {
        protected String appName;
        protected String reqId;
        protected List<LogField>  filterFieldList = new ArrayList<LogField>();
        protected String startDate;
        protected String endDate;
        protected String startTime;
        protected String endTime;

        public String getAppName() {
            return appName;
        }

        public void setAppName(String appName) {
            this.appName = appName;
        }

        public String getReqId() {
            return reqId;
        }

        public void setReqId(String reqId) {
            this.reqId = reqId;
        }

        public List<LogField> getFilterFieldList() {
            return filterFieldList;
        }

        public void addFilterField(LogField filterField) {
            filterFieldList.add(filterField);
        }

        public String getStartDate() {
            return startDate;
        }

        public void setStartDate(String startDate) {
            this.startDate = startDate;
        }

        public String getEndDate() {
            return endDate;
        }

        public void setEndDate(String endDate) {
            this.endDate = endDate;
        }

        public String getStartTime() {
            return startTime;
        }

        public void setStartTime(String startTime) {
            this.startTime = startTime;
        }

        public String getEndTime() {
            return endTime;
        }

        public void setEndTime(String endTime) {
            this.endTime = endTime;
        }

        @Override
        public String toString() {
            return "CallGraphReq{" +
                    "appName='" + appName + '\'' +
                    ", filterFieldList=" + filterFieldList +
                    ", startDate='" + startDate + '\'' +
                    ", endDate='" + endDate + '\'' +
                    ", startTime='" + startTime + '\'' +
                    ", endTime='" + endTime + '\'' +
                    '}';
        }
    }
}
