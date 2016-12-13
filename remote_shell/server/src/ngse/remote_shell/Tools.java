package ngse.remote_shell;

import org.bouncycastle.openssl.PEMReader;
import org.bouncycastle.openssl.PEMWriter;

import javax.crypto.Cipher;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.security.*;
import java.util.Date;

/**
 * Created by Administrator on 2016/2/7.
 */
public class Tools {

    static public String currentSeconds()
    {
        long seconds = new Date().getTime() / 1000;
        return String.format("%-32d", seconds);
    }

    static public PrivateKey loadPrivKeyFromFile(String filename)
    {
        Security.addProvider(new org.bouncycastle.jce.provider.BouncyCastleProvider());
        try {
            FileReader fr = new FileReader(filename);
            PEMReader pr = new PEMReader(fr);
            KeyPair key = (KeyPair)(pr.readObject());
            pr.close();
            fr.close();
            return key.getPrivate();
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return null;
        }
    }
    static public PublicKey loadPubKeyFromFile(String filename)
    {
        Security.addProvider(new org.bouncycastle.jce.provider.BouncyCastleProvider());
        try {
            FileReader fr = new FileReader(filename);
            PEMReader pr = new PEMReader(fr);
            PublicKey key = (PublicKey)(pr.readObject());
            pr.close();
            fr.close();
            return key;
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return null;
        }
    }
    static public KeyPair newRSAKeyAndSave()
    {
        Security.addProvider(new org.bouncycastle.jce.provider.BouncyCastleProvider());

        if (new File("./priv.txt").exists() ||
                new File("./pub.txt").exists())
        {
            System.out.println("priv.txt/pub.txt exists, move away at first please.");
            return null;
        }

        try {
            byte[] input = "abc".getBytes();
            Cipher cipher = Cipher.getInstance("RSA/None/PKCS1Padding", "BC");
            SecureRandom random = new SecureRandom();
            KeyPairGenerator generator = KeyPairGenerator.getInstance("RSA", "BC");

            generator.initialize(1024, random);

            KeyPair pair = generator.generateKeyPair();
            Key pubKey = pair.getPublic();
            Key privKey = pair.getPrivate();

            FileWriter fw = new FileWriter("./priv.txt");
            PEMWriter pw = new PEMWriter(fw);
            pw.writeObject(privKey);
            pw.close();
            fw.close();

            FileWriter fw2 = new FileWriter("./pub.txt");
            PEMWriter pw2 = new PEMWriter(fw2);
            pw2.writeObject(pubKey);
            pw2.close();
            fw2.close();

            return pair;

        }
        catch ( Exception e)
        {
            e.printStackTrace();
            return null;
        }
    }

    static public byte[] encryptWithPriKey(byte[] buf, PrivateKey p)
    {
        Security.addProvider(new org.bouncycastle.jce.provider.BouncyCastleProvider());

        try {
            Cipher cipher = Cipher.getInstance("RSA/None/PKCS1Padding", "BC");
            cipher.init(Cipher.ENCRYPT_MODE, p);
            return  cipher.doFinal(buf);
        }
        catch ( Exception e)
        {
            e.printStackTrace();
            return new byte[0];
        }
    }

    static public byte[] decryptWithPriKey(byte[] buf, PublicKey p)
    {
        Security.addProvider(new org.bouncycastle.jce.provider.BouncyCastleProvider());

        try {
            SecureRandom random = new SecureRandom();
            Cipher cipher = Cipher.getInstance("RSA/None/PKCS1Padding", "BC");

            cipher.init(Cipher.DECRYPT_MODE, p, random);
            return cipher.doFinal(buf);


        }
        catch ( Exception e)
        {
            e.printStackTrace();
            return null;
        }
    }

    static public String toHexString(byte[] b)
    {
        int i;
        StringBuffer sb = new StringBuffer();
        char[] chars = {'0', '1','2','3', '4','5','6','7','8','9','a','b','c','d','e','f'};
        for (i = 0; i < b.length; ++i)
        {
            int bb = b[i];
            if (bb < 0) { bb += 256;}
            int index;
            index = bb>>4;
            sb.append(chars[index]);
            index = bb & 0x0f;
            sb.append(chars[index]);
        }
        return sb.toString();
    }
    static private  int hexChr2Int(char c)
    {
        char[] chars = {'0', '1','2','3', '4','5','6','7','8','9','a','b','c','d','e','f'};
        int i;
        for (i = 0; i < chars.length; ++i)
        {
            if (chars[i] == c)
            {
                return i;
            }
        }
        return 16;
    }
    static public byte[] fromHexString(String s)
    {
        int i;
        if ((s.length() % 2) != 0)
        {
            return new byte[0];
        }
        int len = s.length() / 2;
        byte[] b = new byte[len];


        for (i = 0; i < b.length; ++i)
        {
            int v1 = hexChr2Int(s.charAt(2*i));
            int v2 = hexChr2Int(s.charAt(2*i+1));
            if (v1 > 15 || v2 > 15) { return new byte[0];}
            b[i] = (byte)(v1*16+v2);
        }
        return b;
    }


    static public String getLengthField(int len)
    {
        StringBuffer sb = new StringBuffer();
        sb.append(new Integer(len).toString());
        while (sb.length() < 10)
        {
            sb.append(" ");
        }
        return sb.toString();
    }
}
