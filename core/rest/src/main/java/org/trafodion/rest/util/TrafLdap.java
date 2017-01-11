package org.trafodion.rest.util;

import java.util.Hashtable;

import javax.naming.Context;
import javax.naming.NamingException;

import javax.naming.ldap.InitialLdapContext;
import javax.naming.ldap.LdapContext;

public class TrafLdap {

    private String ip = "10.10.10.3";
    private String port = "389";
    private String basedn = "cn=users,cn=accounts,dc=esgyncn,dc=local";
    private String username="trafodion";
    private String password="traf123";

    public String getUsername() {
        return username;
    }

    public void setUsername(String username) {
        this.username = username;
    }

    public String getPassword() {
        return password;
    }

    public void setPassword(String password) {
        this.password = password;
    }

    public String getIp() {
        return ip;
    }

    public void setIp(String ip) {
        this.ip = ip;
    }

    public String getPort() {
        return port;
    }

    public void setPort(String port) {
        this.port = port;
    }

    public String getBasedn() {
        return basedn;
    }

    public void setBasedn(String basedn) {
        this.basedn = basedn;
    }

    public boolean validateUser(String userName, String passwd) {

        String userdn = "uid=" + userName + "," + basedn;
        String url = "ldap://" + ip + ":" + port + "/" + basedn;

        Hashtable<String, String> env = new Hashtable<String, String>();

        LdapContext ldapContext = null;

        env.put(Context.SECURITY_PRINCIPAL, userdn);

        env.put(Context.SECURITY_CREDENTIALS, passwd);

        env.put(Context.PROVIDER_URL, url);

        env.put(Context.INITIAL_CONTEXT_FACTORY, "com.sun.jndi.ldap.LdapCtxFactory");

        env.put(Context.SECURITY_AUTHENTICATION, "simple");
        try {
            ldapContext = new InitialLdapContext(env, null);
            ldapContext.close();
            return true;

        } catch (NamingException e) {
             e.printStackTrace();
             //System.out.println("---  failure ----");
            return false;
        }
    }

    public static void main(String[] args) {
    TrafLdap t = new TrafLdap();
        System.out.println(t.validateUser("trafodion", "traf1234"));
        return;
    }
}
