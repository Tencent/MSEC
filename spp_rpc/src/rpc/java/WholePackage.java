
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


package srpc;

import com.google.protobuf.ByteString;
import com.google.protobuf.CodedInputStream;
import com.google.protobuf.GeneratedMessage;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;
import java.util.List;
import java.util.regex.Pattern;

/**
 * Created by Administrator on 2016/5/6.
 */
public class WholePackage {

    private long sequence;
    private String methodNameBeCalled;
    private byte[] bodyBytes;

    public long getSequence() {
        return sequence;
    }

    public void setSequence(long sequence) {
        this.sequence = sequence;
    }

    public String getMethodNameBeCalled() {
        return methodNameBeCalled;
    }

    public void setMethodNameBeCalled(String methodNameBeCalled) {
        this.methodNameBeCalled = methodNameBeCalled;
    }

    public byte[] getBodyBytes() {
        return bodyBytes;
    }

    public void setBodyBytes(byte[] bodyBytes) {
        this.bodyBytes = bodyBytes;
    }

    //����ת��Ϊ������ֽ�����
    static private byte[] int2Bytes(int i)
    {
        byte[] b = new byte[4];
        int v = 256 * 256 * 256;
        for (int j = 0; j < 3; j++) {
            b[j] = (byte)(i / v);
            i = i % v;
            v = v / 256;
        }
        b[3] = (byte)i;

        return b;
    }
//������ֽ�bufת��Ϊ����int
    static private int bytes2int(byte[] buf)
    {
        int v = 0;
        int b0 = buf[0]; if (b0 < 0) { b0 += 256;}
        int b1 = buf[1]; if (b1 < 0) { b1 += 256;}
        int b2 = buf[2]; if (b2 < 0) { b2 += 256;}
        int b3 = buf[3]; if (b3 < 0) { b3 += 256;}
        v = b0 * (256*256*256) + b1 * (256*256) + b2*256 + b3;
        return v;
    }

    private Head.CRpcHead buildHead()
    {
        Head.CRpcHead.Builder b = Head.CRpcHead.newBuilder();
        b.setSequence( sequence);
        b.setMethodName(ByteString.copyFrom(methodNameBeCalled.getBytes()));
        return b.build();
    }
    public byte[] pack() throws Exception
    {
        //���ĸ�ʽ "(head_len body_len head body)"
        Head.CRpcHead head = buildHead();
        int sz = 2 + 4 + 4 + head.getSerializedSize()+bodyBytes.length;
        byte[] result = new byte[sz];

        byte[] head_len = int2Bytes(head.getSerializedSize());
        byte[] body_len = int2Bytes(bodyBytes.length);
        byte[] head_bytes = head.toByteArray();


        int offset = 0;

        result[offset++] = 0x28; // (
        for (int i = 0; i < 4; ++i, offset++) // head_len
        {
            result[offset] = head_len[i];
        }
        for (int i = 0; i < 4; ++i, offset++) // body_len
        {
            result[offset] = body_len[i];
        }

        for (int i = 0; i < head_bytes.length; ++i, ++offset) // head
        {
            result[offset] = head_bytes[i];
        }

        for (int i = 0; i < bodyBytes.length; ++i, ++offset) // body
        {
            result[offset] = bodyBytes[i];
        }
        result[offset++] = 0x29; // )

        if (offset != sz)
        {
            throw  new Exception("length mismatch!");
        }
        return result;


    }

     public void unpack(final  byte[] raw) throws Exception
    {
        if (raw.length < 10 ||
                raw[0] != 0x28 )
        {
            throw  new Exception("raw data is invalid response.");
        }

        byte[] lenBytes = new byte[4];
        for (int i = 0; i < 4; i++) {
            lenBytes[i] = raw[1+i];
        }
        int headLen = bytes2int(lenBytes);
        for (int i = 0; i < 4; i++) {
            lenBytes[i] = raw[5+i];
        }
        int bodyLen = bytes2int(lenBytes);

        if (raw[10+headLen+bodyLen-1] != 0x29)
        {
            throw  new Exception("raw data is invalid response.");
        }

        if ( (10 + headLen + bodyLen ) > raw.length)
        {
            throw new Exception("raw data length is invalid.");
        }
        byte[] headBytes = new byte[headLen];
        bodyBytes  = new byte[bodyLen];
        for (int i = 0; i < headLen; i++) {
            headBytes[i] = raw[9+i];
        }
        for (int i = 0; i < bodyLen; i++) {
            bodyBytes[i] = raw[9+headLen+i];
        }

        Head.CRpcHead head = Head.CRpcHead.parseFrom(headBytes);

        sequence = head.getSequence();
        methodNameBeCalled = new String(head.getMethodName().toByteArray());

        return;


    }
    //用于流式传输（例如tcp）情况下，判断buf里是否获得了一个完整的package
    // 返回值：小于0表示格式非法  等于0表示没有收完整还需要继续接收  大于0表示收到了完整的报文，报文长度作为返回值
    static public int isWholePackage(byte[] buf, int offset, int len)
    {
        if (buf[offset] != 0x28)
        {
            return -1;
        }
        if (len < 10)
        {
            return -1;
        }
        byte[] lenBytes = new byte[4];
        for (int i = 0; i < 4; i++) {
            lenBytes[i] = buf[offset+1+i];
        }
        int headLen = bytes2int(lenBytes);
        if (headLen < 0) { return -1;}

        for (int i = 0; i < 4; i++) {
            lenBytes[i] = buf[offset+5+i];
        }
        int bodyLen = bytes2int(lenBytes);
        if (bodyLen < 0) { return -1;}

        if ( ( 2+4+4+headLen+bodyLen) <= len)
        {
            return ( 2+4+4+headLen+bodyLen);
        }
        else {
            return 0;
        }

    }

}
