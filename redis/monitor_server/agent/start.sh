
#
# Tencent is pleased to support the open source community by making MSEC available.
#
# Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
#
# Licensed under the GNU General Public License, Version 2.0 (the "License"); 
# you may not use this file except in compliance with the License. You may 
# obtain a copy of the License at
#
#     https://opensource.org/licenses/GPL-2.0
#
# Unless required by applicable law or agreed to in writing, software distributed under the 
# License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
# either express or implied. See the License for the specific language governing permissions
# and limitations under the License.
#



killall -9 monitor_agent
./monitor_agent monitor.conf >>monitor.log
sleep 1

check=`ps -ef | grep monitor_agent | grep -v grep | wc -l`
if [ $check -eq 0 ]; then
        echo "Monitor agent fails to start, see monitor.log (below) for more details:"
        tail monitor.log
else
        echo "Monitor agent started"
fi

if (( $(crontab -l | grep "sysmon.py" | wc -l) == 0 )); then
    crontab -l > /tmp/crontab.sysmon
    echo '' >> /tmp/crontab.sysmon
    echo "## added system monitor report"  >> /tmp/crontab.sysmon
    echo "* * * * * /msec/agent/monitor/sysmon.py >>/msec/agent/monitor/sysmon.log 2>&1 &" >>/tmp/crontab.sysmon
    crontab /tmp/crontab.sysmon
    crontab -l
fi
