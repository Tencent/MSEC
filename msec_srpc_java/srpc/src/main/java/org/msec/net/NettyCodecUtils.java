
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



package org.msec.net;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import org.jboss.netty.channel.ChannelHandlerContext;

public final class NettyCodecUtils {

	private NettyCodecUtils() {
	}

	@SuppressWarnings("unchecked")
	public static void setAttachment(ChannelHandlerContext ctx, int key, Object value) {
		Map<Integer, Object> attachments = (Map<Integer, Object>) ctx.getAttachment();
		if (attachments == null) {
			attachments = createAttachment(ctx);
		}
		attachments.put(key, value);
	}

	@SuppressWarnings("unchecked")
	public static Object getAttachment(ChannelHandlerContext ctx, int key) {
		Map<Integer, Object> attachments = (Map<Integer, Object>) ctx.getAttachment();
		if (attachments == null) {
			//attachments = createAttachment(ctx);
			return null;
		}
		return attachments.remove(key);
	}

	@SuppressWarnings("unchecked")
	private static Map<Integer, Object> createAttachment(ChannelHandlerContext ctx) {
		synchronized (ctx) {
			Map<Integer, Object> attachments = (Map<Integer, Object>) ctx.getAttachment();
			if (attachments == null) {
				attachments = new ConcurrentHashMap<Integer, Object>();
				ctx.setAttachment(attachments);
			}
			return attachments;
		}
	}

	public static long generateColorId(String methodName) {
		long colorId = 0;
		for (int i=0; i<methodName.length(); ++i) {
			colorId += (long)(methodName.charAt(i)) * 107;
		}
		return colorId;
	}

	public static void int8_to_buf(byte buf[], int pos, int value) {
		buf[pos + 0] = (byte) (value >> 0);
	}

	public static void int16_to_buf(byte buf[], int pos, int value) {
		buf[pos + 1] = (byte) (value >> 0);
		buf[pos + 0] = (byte) (value >> 8);
	}

	public static void int32_to_buf(byte buf[], int pos, int value) {
		buf[pos + 3] = (byte) (value >> 0);
		buf[pos + 2] = (byte) (value >> 8);
		buf[pos + 1] = (byte) (value >> 16);
		buf[pos + 0] = (byte) (value >> 24);
	}

	public static void int64_to_buf(byte buf[], int pos, long value) {
		buf[pos + 7] = (byte) (value >> 0);
		buf[pos + 6] = (byte) (value >> 8);
		buf[pos + 5] = (byte) (value >> 16);
		buf[pos + 4] = (byte) (value >> 24);
		buf[pos + 3] = (byte) (value >> 32);
		buf[pos + 2] = (byte) (value >> 40);
		buf[pos + 1] = (byte) (value >> 48);
		buf[pos + 0] = (byte) (value >> 56);
	}

	public static void int64_to_buf32(byte buf[], int pos, long value) {
		buf[pos + 3] = (byte) (value >> 0);
		buf[pos + 2] = (byte) (value >> 8);
		buf[pos + 1] = (byte) (value >> 16);
		buf[pos + 0] = (byte) (value >> 24);
	}

	public static int buf_to_int8(byte buf[], int pos) {
		return buf[pos] & 0xff;
	}

	public static int buf_to_int16(byte buf[], int pos) {
		return ((buf[pos] << 8) & 0xff00) + ((buf[pos + 1] << 0) & 0xff);
	}

	public static int buf_to_int32(byte buf[], int pos) {
		return ((buf[pos] << 24) & 0xff000000)
				+ ((buf[pos + 1] << 16) & 0xff0000)
				+ ((buf[pos + 2] << 8) & 0xff00) + ((buf[pos + 3] << 0) & 0xff);
	}

	public static long buf_to_int64(byte buf[], int pos) {
		long ret = 0;

		ret += ((((long) buf[pos] << 56)) & 0xff00000000000000L);
		ret += ((((long) buf[pos + 1] << 48)) & 0xff000000000000L);
		ret += ((((long) buf[pos + 2] << 40)) & 0xff0000000000L);
		ret += ((((long) buf[pos + 3] << 32)) & 0xff00000000L);
		ret += ((((long) buf[pos + 4] << 24)) & 0xff000000L);
		ret += (((long) buf[pos + 5] << 16) & 0xff0000L);
		ret += (((long) buf[pos + 6] << 8) & 0xff00L);
		ret += (((long) buf[pos + 7] << 0) & 0xffL);

		return ret;
	}

	public static int getRand() {
		return (int) (Math.random() * Integer.MAX_VALUE);
	}
}
