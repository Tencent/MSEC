
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

import org.jboss.netty.util.HashedWheelTimer;
import org.jboss.netty.util.Timeout;
import org.jboss.netty.util.Timer;
import org.jboss.netty.util.TimerTask;

import java.util.concurrent.TimeUnit;

public class ServiceUtils {
    protected  static final Timer timer = new HashedWheelTimer();

    public static class RepeatedTimerTask implements TimerTask {
        protected Timer     timer;
        protected TimerTask innerTask;
        protected long     interval;
        protected TimeUnit unit;
        public RepeatedTimerTask(Timer timer, TimerTask innerTask, long interval, TimeUnit unit)  {
            this.timer = timer;
            this.innerTask = innerTask;
            this.interval = interval;
            this.unit = unit;
        }

        public void run(Timeout timeout) throws Exception {
            innerTask.run(timeout);
            timer.newTimeout(this, interval, unit);
        }
    }

    public static void loop(TimerTask  timerTask, long intervalInMs) {
        timer.newTimeout(new RepeatedTimerTask(timer, timerTask, intervalInMs, TimeUnit.MILLISECONDS),
                intervalInMs, TimeUnit.MILLISECONDS);
    }
}
