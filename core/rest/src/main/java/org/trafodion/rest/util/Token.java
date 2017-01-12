package org.trafodion.rest.util;

import java.util.Date;

import java.security.Key;

import javax.crypto.Cipher;

public class Token {

    public static String error_invalid_request = "invalid_request";
    public static String error_invalid_client = "invalid_client";
    public static String error_invalid_grant = "invalid_grant";
    public static String error_unauthorized_client = "unauthorized_client";
    public static String error_unsupported_grant_type = "unsupported_grant_type";
    public static String error_invalid_scope = "invalid_scope";

    public static String error = "error";
    public static String error_description = "error_description";
    public static String error_uri = "error_uri";

    public static String access_token = "access_token";
    public static String token_type = "token_type";
    public static String expires_in = "expires_in";

    public static String error_invalid_token = "error_invalid_token";

    private String passwd = "fuck-trafodion!";
    private String token = "";
    private String interval = "7200000";

    public String getInterval() {
        return interval;
    }

    public void setInterval(String interval) {
        this.interval = interval;
    }

    public String getPasswd() {
        return passwd;
    }

    public void setPasswd(String passwd) {
        this.passwd = passwd;
    }

    public String getToken() {
        this.token = encode();
        return this.token;
    }

    public void setToken(String token) {
        this.token = token;
    }

    public boolean isTimeOut() {
        if (token == null)
            return true;
        long timeout = Long.parseLong(getTimeOut(token));
        Date date = new Date();
        long timenow = date.getTime();
        return (timenow > timeout);
    }

    private String deconde(String t) {

        byte[] decryptFrom = parseHexStr2Byte(t);

        if (decryptFrom == null)
            return null;
        byte[] decryptResult = decrypt(decryptFrom, passwd);

        return new String(decryptResult);
    }

    private String encode() {

        Date date = new Date();
        long timeout = date.getTime() + Long.parseLong(interval);

        String content = String.valueOf(timeout) + "=" + "continue";

        byte[] encryptResult = encrypt(content, passwd);
        String encryptResultStr = parseByte2HexStr(encryptResult);
        return encryptResultStr;
    }

    private String getTimeOut(String t) {
        String d = deconde(t);
        if (d == null)
            return "0";
        return d.split("=")[0];
    }

    public static void main(String[] args) {
        // TODO Auto-generated method stub
        Token t = new Token();
        // t.setPasswd("12345678");
        String content = "test";
        // String password = "12345678";

        System.out.println("before:" + content);
        byte[] encryptResult = t.encrypt(content, "fuck-trafodion!");
        String encryptResultStr = t.parseByte2HexStr(encryptResult);
        System.out.println("after:" + encryptResultStr);
        System.out.println("password:" + t.passwd);

        byte[] decryptFrom = t.parseHexStr2Byte(encryptResultStr);
        byte[] decryptResult = t.decrypt(decryptFrom, t.passwd);
        System.out.println("after:" + new String(decryptResult));

        Date date = new Date();

        System.out.println(date.getTime());
        System.out.println("123=".split("=")[0]);
        System.out.println(t.getToken());
        System.out.println(t.deconde(t.token));

        System.out.println(t.isTimeOut());
    }

    private byte[] encrypt(String content, String password) {
        try {

            Key key = getKey(password.getBytes());
            Cipher encryptCipher = null;
            encryptCipher = Cipher.getInstance("DES");
            encryptCipher.init(Cipher.ENCRYPT_MODE, key);

            return encryptCipher.doFinal(content.getBytes());

        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    private byte[] decrypt(byte[] content, String password) {
        try {

            Key key = getKey(password.getBytes());

            Cipher decryptCipher = null;

            decryptCipher = Cipher.getInstance("DES");
            decryptCipher.init(Cipher.DECRYPT_MODE, key);

            return decryptCipher.doFinal(content);

        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    private String parseByte2HexStr(byte buf[]) {
        StringBuffer sb = new StringBuffer();
        for (int i = 0; i < buf.length; i++) {
            String hex = Integer.toHexString(buf[i] & 0xFF);
            if (hex.length() == 1) {
                hex = '0' + hex;
            }
            sb.append(hex.toUpperCase());
        }
        return sb.toString();
    }

    private byte[] parseHexStr2Byte(String hexStr) {
        if (hexStr.length() < 1)
            return null;
        byte[] result = new byte[hexStr.length() / 2];
        for (int i = 0; i < hexStr.length() / 2; i++) {
            int high = Integer.parseInt(hexStr.substring(i * 2, i * 2 + 1), 16);
            int low = Integer.parseInt(hexStr.substring(i * 2 + 1, i * 2 + 2), 16);
            result[i] = (byte) (high * 16 + low);
        }
        return result;
    }

    private Key getKey(byte[] arrBTmp) throws Exception {

        byte[] arrB = new byte[8];

        for (int i = 0; i < arrBTmp.length && i < arrB.length; i++) {
            arrB[i] = arrBTmp[i];
        }

        Key key = new javax.crypto.spec.SecretKeySpec(arrB, "DES");

        return key;
    }

}
