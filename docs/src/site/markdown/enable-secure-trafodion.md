<!--
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at
 
      http://www.apache.org/licenses/LICENSE-2.0
 
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the 
  License.
-->
This page describes how to enable Trafodion security.

# Introduction
If you do not enable security in Trafodion, a client interface to Trafodion may request a username and password, but Trafodion ignores the user name and password entered in the client interface, and the session runs as the database **```root```** user, **```DB__ROOT```**, without restrictions. If you want to restrict users, restrict access to certain users only, or restrict access to an object or operation, then you must enable security, which enforces authentication and authorization. You can enable security during installation by answering the installer's prompts or after installation by running the **```traf_authentication_setup```** script, which enables both authentication and authorization. For more information, see [Authentication Setup Script](#Authentication_Setup_Script) below.

Trafodion does not manage user names and passwords internally but does support authentication via directory servers that support the OpenLDAP protocol, also known as LDAP servers. You can configure the LDAP servers during installation by answering the installer's prompts, or you can configure the LDAP servers manually after installation. For more information, please refer to [Configuring LDAP Servers](Configuring_LDAP_Servers) below.

Once authentication and authorization are enabled, Trafodion allows users to be registered in the database and allows privileges on objects to be granted to users and roles (which are granted to users). Trafodion also supports component-level (or system-level) privileges, such as MANAGE_USERS, which can be granted to users and roles. Refer to [Manage Users](#Manage_Users) below.

# Configuring LDAP Servers
To specify the LDAP server(s) to be used for authentication, you need to configure the text file **```.traf_authentication_config```**, located (by default) in **```$MY_SQROOT/sql/scripts```**. This file is a flat file, organized as a series of attribute/value pairs. Details on all the attributes and values accepted in the authentication configuration file and how to configure alternate locations can be found in [.traf_authentication_config](traf_authentication_config.html).

A sample template file is located in **```$MY_SQROOT/sql/scripts/traf_authentication_config```**.

Attributes and values in the authentication configuration file are separated with a colon immediately following the attribute name. In general white space is ignored, but spaces may be relevant in some values. Attribute names are always case insensitive. Multiple instances of an attribute are specified by repeating the attribute name and providing the new value. For attributes with only one instance, if the attribute is repeated, the last value provided is used.

    Attribute1: valueA
    Attribute2: valueB
    Attribute1: valueC

If **```Attribute1```** has only one instance, **```valueC```** is used, otherwise, **```valueA```** and **```valueC```** are both added to the list of values for **```Attribute1```**.

Attributes are grouped into sections; this is for future enhancements. Attributes are declared in the **```LOCAL```** section, unless otherwise specified.

<table><tr><td><strong>Note</strong><br />Section names, attribute names, and the general layout of the authentication configuration file are subject to change in future versions of Trafodion and backward compatibility is not guaranteed.</td></tr></table>

Specification of your directory server(s) requires at a minimum:

<!-- This table is too complex to do in markdown -->
<table>
  <tr>
    <th width="15%">Setting</th>
    <th width="55%">Description</th>
    <th width="30%">Example</th>
  </tr>
  <tr>
    <td><strong><code>LDAP Host Name(s)</code></strong></td>
    <td>One or more names of hosts that support the OpenLDAP protocol must be specified. Trafodion will attempt to connect to all provided host names during the authentication process. The set of user names and passwords should be identical on all hosts to avoid unpredictable results. The attribute name is <strong><code>LDAPHostName</code></strong></td>
    <td><pre>LDAPHostName: ldap.company.com</pre></td>
  </tr>
  <tr>
    <td><strong><code>LDAP Port Number</code></strong></td>
    <td>Port number of the LDAP server. Typically this is 389 for servers using no encryption or TLS, and 636 for servers using SSL. The attribute name is <strong><code>LDAPPort</code></strong>.</td>
    <td><pre>LDAPPort: 389</pre></td>
  </tr>
  <tr>
    <td><strong><code>LDAP Unique Identifier</code></strong></td>
    <td>Attribute(s) used by the directory server that uniquely identifies the user name. You may provide one or more unique identifier specifiers.</td>
    <td><pre>UniqueIdentifier: uid=,ou=users,dc=com</pre></td>
  </tr>  
  <tr>
    <td><strong><code>Encryption Level</code></strong></td>
    <td>A numeric value indicating the encryption scheme used by your LDAP server. Values are:
      <ul>
        <li>0: Encryption not used</li>
        <li>1: SSL</li>
        <li>2: TLS</li>
      </ul>
    </td>
    <td>
      <pre>LDAPSSL: 2</pre>
      <p>If your LDAP server uses TLS you must specify a file containing the certificate used to encrypt the password. By default the Trafodion software looks for this file in <strong><code>$MY_SQROOT/cacerts</code></strong>, but you may specify a fully qualified filename, or set the environment variable <strong><code>CACERTS_DIR</code></strong> to another directory. To specify the file containing the certificate, you set the value of the attribute <strong><code>TLS_CACERTFilename</code></strong>, located in the Defaults section.</p>
      <p><strong>Example</strong></p>
      <pre>
TLS_CACERTFilename: mycert.pem 
TLS_CACertFilename: /usr/etc/cert.pem</pre>
    </td>
  </tr>  
  <tr>
    <td><strong><code>Search username and password</code></strong></td>
    <td>Some LDAP servers require a known user name and password to search the directory of user names. If your environment has that requirement, provide these “search” values.</td>
    <td>
      <pre>
LDAPSearchDN: lookup@company.com
LDAPSearchPwd: Lookup123</pre>
    </td>
  </tr>
</table>  

There are additional optional attributes that can be used to customize Trafodion authentication. As mentioned earlier, they are described in [.traf_authentication_config](traf_authentication_config.html).

You can test the authentication configuration file for syntactic errors using the **```ldapconfigcheck```** tool. If you have loaded the Trafodion environment (**```sqenv.sh```**), then the tool will automatically check the file at **```$MY_SQROOT/sql/scripts/.traf_authentication_config```**. If not, you can specify the file to be checked.

**Example**

    ldapconfigcheck –file myconfigfile
    File myconfigfile is valid.
    
If an error is found, the line number with the error is displayed along with the error. Please refer to [ldapconfigcheck](ldapconfigcheck.html) for more information.

<table><tr><td><strong>Note</strong><br />The authentication configuration file needs to be propagated to all nodes, but there is a script that will do that for you described later. For now, you can test your changes on the local node.</td></tr></table>

You can test the LDAP connection using the utility **```ldapcheck```**. To use this utility the Trafodion environment must be loaded (**```sqenv.sh```**), but the Trafodion instance does not need to be running. To test the connection only, you can specify any user name, and a name lookup will be performed using the attributes in **```.traf_authentication_config```**.

    ldapcheck --username=fakename@company.com
    User fakename@company.com not found

If **```ldapcheck```** reports either that the user was found or the user was not found, the connection was successful. However, if an error is reported, either the configuration file is not setup correctly, or there is a problem either with your LDAP server or the connection to the server. You can get additional error detail by including the **```--verbose```** option. Please refer to [ldapcheck](ldapcheck.html) for more information.

If you supply a password, **```ldapcheck```** will attempt to authenticate the specified **```username```** and **```password```**. The example below shows the password for illustrative purposes, but to avoid typing the password on the command line, leave the password blank (**```--password=```**) and the utility will prompt for the password with no echo.

    ldapcheck --username=realuser@company.com –-password=StrongPassword
    Authentication successful
    
# Generate a Trafodion Certificate
Trafodion clients such as **```trafci```** will encrypt the password before sending it to Trafodion. A self-signed certificate is used to encrypt the password. The certificate and key should be generated when the **```sqgen```** script is invoked. By default, the files **```server.key```** and **```server.crt```** will be located in **```$HOME/sqcert```**. If those files are not present, since Trafodion clients will not send unencrypted passwords, you will need to manually generate those files. To do so, run the script **```sqcertgen```** located in **```$MY_SQROOT/sql/scripts```**. The script runs **```openssl```** to generate the certificate and key.

To run openssl manually, follow the example:

    openssl req -x509 -nodes -days 365 -subj '/C=US/ST=California/L=PaloAlto/CN=host.domain.com/O=Some Company/OU=Service Connection' 
    - newkey rsa:2048 -keyout server.key -out server.crt

Option                                    | Description
------------------------------------------|-----------------------
**```-x509```**                           | Generate a self-signed certificate.
**```-days <validity of certificate>```** | Make the certificate valid for the days specified.
**```-newkey rsa:<bytes>```**             | Generate a new private key of type RSA of length 1024 or 2048 bytes.
**```-subj <certificateinfo>```**         | Specify the information that will be incorporated in the certificate. Each instance in a cluster should have a unique common name(**```CN```**).
**```-keyout <filename>```**              |  Write the newly generated RSA private key to the file specified.
**```-nodes```**                          | It is an optional parameter that specifies NOT to encrypt the private key. If you encrypt the private key, then you must enter the password every time the private key is used by an application.
**```-out <filename>```**                 | Write the self-signed certificate to the specified file.

Both the public (**```server.crt```**) and private (**```server.key```**) files should be placed in the directory **```$HOME/sqcert```**. If you do not want to use the **```HOME```** directory or if you want to use different names for the private and/or public key files, then Trafodion supports environment variables to specific the alternate locations or names.

* Trafodion first checks the environment variables **```SQCERT_PRIVKEY```** and **```SQCERT_PUBKEY```**. If they are set, Trafodion uses the fully qualified filename value of the environment variable.

    You can specify either one filename environment variable or both.

* If at least one filename environment variable is not set, Trafodion checks the value of the environment variable **```SQCERT_DIR```**. If set, the default filename **```server.key```** or **```server.crt```** is appended to the value of the environment variable **```SQCERT_DIR```**.
* If the filename environment variable is not set and the directory environment variable is not set, Trafodion uses the default location (**```$HOME/sqcert```**) and the default filename.

# Authentication Setup Script
The final step to enable security is to change the value of the environment variable **```TRAFODION_ENABLE_AUTHENTICATION```**from **```NO```** to **```YES```** and turn on authorization. This is achieved by invoking the **```traf_authentication_setup```** script, which is located in **```$MY_SQROOT/sql/scripts```**.

**Usage**

Usage: traf_authentication_setup [options]

```
Options:
    --file <loc>  Optional location of OpenLDAP configuration file
    --help        Prints this message
    --off         Disables authentication and authorization                               
    --on          Enables authentication and authorization
    --setup       Enables authentication                             
    --status      Returns status of authentication enablement
```

<!-- In HTML to control the column widths; markdown table doesn't handle Option correctly. -->
<table>
  <tr>
    <th width="10%">Option</th>
    <th width="90%">Description</th>
  </tr>
  <tr>
    <td><strong><code>--file</code></strong></td>
    <td>If specified, then <strong><code>filename</code></strong> is copied to <strong><code>$MY_SQROOT/</code></strong>. Users working in their own private environment can refer to a site-specific configuration file from a central location.</td>
  </tr>
  <tr>
    <td><strong><code>--on</code></strong></td>
    <td>
      <p><strong><code>traf_authentication_setup</code></strong> invokes <a href="ldapconfigcheck.html">ldapconfigcheck</a>) to verify the configuration file is syntactically correct. It also invokes <a href="ldapcheck.html">ldapcheck</a> to verify that a connection can be made to an LDAP server.</p>
      <p>If both checks pass, the script sets the environment variable <strong><code>TRAFODION_ENABLE_AUTHENTICATION</code></strong> to <strong><code>YES</code></strong> in the file <strong><code>$MY_SQROOT/sqenvcom.sh</code></strong>, and propagates <strong><code>sqenvcom.sh</code></strong> and <strong><code>.traf_authentication_config</code></strong> to all nodes in the cluster.</p>
      <p>The last step is to enable authorization by creating privilege-related metadata tables and set up default permissions with a call to the database. The list of privilege-related metadata tables, users, roles, and component privileges are logged in <strong><code>$MY_SQROOT/logs/authEnable.log</code></strong>.</p>
      <p>Specifying <strong><code>--on</code></strong> requires that a valid <strong><code>.traf_authentication_config</code></strong> file exists and the Trafodion metadata initialized.</p>
    </td>
  </tr>
  <tr>
    <td><strong><code>--off</code></strong></td>
    <td>
      <p>If specified, then <strong><code>traf_authentication_setup</code></strong> sets the environment variable <strong><code>TRAFODION_ENABLE_AUTHENTICATION</code></strong> to <strong><code>NO</code></strong> in <strong><code>$MY_SQROOT/sqenvcom.sh</code></strong> and propagates the file to all the nodes in the cluster.</p>
      <p>The last step is to disable authorization by removing any privilege-related metadata and permissions with a call to the database. The results of this operation is logged in <strong><code>$MY_SQROOT/logs/authEnable.log</code></strong>.</p>
    </td>
  </tr>
  <tr>
    <td><strong><code>--setup</code></strong></td>
    <td>Use this option if the Trafodion metadata has not been initialized. This option enables authentication but does not call the database to create privilege-related metadata tables. Later, when Trafodion metadata is initialized, privilege-related metadata tables and default permissions are automatically created.
    </td>
  </tr>
  <tr>
    <td><strong><code>--status</code></strong></td>
    <td>Reports the value of the environment variable <strong><code>TRAFODION_ENABLE_AUTHENTICATION</code></strong> in <strong><code>$MY_SQROOT/sqenvcom.sh</code></strong> on the current node and reports the status of security features in the database.
    </td>
  </tr>
</table>

**Example**

```
INFO: Start of security (authentication and authorization) script Wed Mar 25 15:12:50 PDT 2xxx.

INFO:  *** Trafodion security (authentication and authorization) status *** 
   Authentication is ENABLED
   Authorization (grant/revoke) is ENABLED

INFO: End of security (authorization and authentication) script Wed Mar 25 15:12:54 PDT 2xxx.
```

<table><tr><td><strong>IMPORTANT</strong><br />Any time the environment file (<strong><code>sqenvcom.sh</code></strong>) is changed (and propagated to all nodes), Database Connectivity Services (DCS) must be restarted to pick up the new value. If the configuration file is changed, it will be re-read in 30 minutes (by default), but you can have changes take effect immediately by restarting DCS.</td></tr></table>

To restart DCS, run the scripts **```stop-dcs.sh```** and **```start-dcs.sh```**, located in **```$MY_SQROOT/dcs-<x>.<y>.<z>/bin```**.

# Manage Users
Users are registered in the Trafodion database and are used to enforce authorization. If security is disabled, any user can register any user at any time. However, once security is enabled, user administration is considered a secure operation, and registration of users is restricted to **```DB__ROOT```** or any user granted the **```MANAGE_USERS```** component privilege. To initially register a user, connect to Trafodion with the external user mapped to **```DB__ROOT```**(also known as the Trafodion ID).

When security is enabled, the **```DB__ROOT```** user is registered as the **```TRAFODION```** external user name. It is recommended that the **```DB__ROOT```** user be mapped to the external user name that will be used to connect for root operations. To do this, start a **```sqlci```** session and perform the **```ALTER USER``*** command, for example:

    ALTER USER DB__ROOT SET EXTERNAL NAME trafodion_rootuser_in_ldap;

To learn more about how to register users, grant object and component privileges, and manage users and roles, please see the [Trafodion SQL Reference Manual](docs/Trafodion_SQL_Reference_Manual.pdf).