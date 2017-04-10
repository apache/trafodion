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
This page describes the settings in the **```.traf_authentication_config```** file, which is the configuration file related to [Enabling Security Features in Trafodion](enable-secure-trafodion.html).

# File Location
By default the Trafodion authentication configuration file is located at **```$TRAF_HOME/sql/scripts/.traf_authentication_config```**. If you want to store the configuration file in a different location and/or use a different filename, then Trafodion supports environment variables to specify the alternate location/name.

Trafodion firsts checks the environment variable **```TRAFAUTH_CONFIGFILE```**. If set, the value is used as the fully qualified Trafodion authentication configuration file.

If the environment variable is not set, then Trafodion next checks the variable **```TRAFAUTH_CONFIGDIR```**. If set, the value is prepended to **```.traf_authentication_config```** and used as the Trafodion authentication file.

If neither is set, Trafodion defaults to **```$TRAF_HOME/sql/scripts/.traf_authentication_config```**.

# Template

    # To use authentication in Trafodion, this file must be configured
    # as described below and placed in $TRAF_HOME/sql/scripts and be named
    # .traf_authentication_config.  You must also enable authentication by
    # running the script traf_authentication_setup in $TRAF_HOME/sql/scripts.
    #
    # NOTE: the format of this configuration file is expected to change in the 
    # next release of Trafodion.  Backward compatibility is not guaranteed.
    #
    SECTION: Defaults
      DefaultSectionName: local
      RefreshTime: 1800
      TLS_CACERTFilename:
    SECTION: local 
    # If one or more of the LDAPHostName values is a load balancing host, list
    # the name(s) here, one name: value pair for each host.
      LoadBalanceHostName: 

    # One or more identically configured hosts must be specified here,  
    # one name: value pair for each host.
      LDAPHostName:
    
    # Default is port 389, change if using 636 or any other port
      LDAPPort:389
    
    # Must specify one or more unique identifiers, one name: value pair for each
      UniqueIdentifier:
    
    # If the configured LDAP server requires a username and password to 
    # to perform name lookup, provide those here.  
      LDAPSearchDN:
      LDAPSearchPwd:
     
    # If configured LDAP server requires TLS(1) or SSL (2), update this value
      LDAPSSL:0
    
    # Default timeout values in seconds
      LDAPNetworkTimeout: 30 
      LDAPTimeout: 30 
      LDAPTimeLimit: 30
     
    # Default values for retry logic algorithm
      RetryCount: 5 
      RetryDelay: 2 
      PreserveConnection: No
      ExcludeBadHosts: Yes  
      MaxExcludeListSize: 3

# Configuration Attributes

Attribute Name             | Purpose                             | Example Value             | Notes
---------------------------|-------------------------------------|---------------------------|---------------------------------------
**```LDAPHostName```**     | Host name of the local LDAP server. | **```ldap.master.com```** | If more than one **```LDAPHostName```** entry is provided, Trafodion will attempt to connect with each LDAP server before returning an authentication error. Also see the description related to **```RetryCount```** and **```RetryDelay```** entries.
**```LDAPPort```**         | Port number of the local LDAP server. | **```345```** | Must be numeric value. Related to **```LDAPSSL```** entry. Standard port numbers for OpenLDAP are as follows: <ul><li>Non-secure: 389</li><li>SSL: 636</li><li>TLS: 389</li></ul>
**```LDAPSearchDN```** | If a search user is needed, the search user distinguished name is specified here. | **```cn=aaabbb, dc=demo, dc=net```** | If anonymous search is allowed on the local server, this attribute does not need to be specified or can be specified with no value (blank). To date, anonymous search is the normal approach used.
**```LDAPSearchPWD```** | Password for the **```LDAPSearchDN```** value. See that entry for details. | **```welcome```** | None.
**```LDAPSSL```** | A numeric value specifying whether the local LDAP server interface is unencrypted or TLS or SSL. Legal values are 0 for unencrypted, 1 for SSL, and 2 for TLS. For SSL/TLS, see the section below on Encryption Support. | **```0```** | None.
**```UniqueIdentifier```** | The directory attribute that contains the user's unique identifier. | **```uid=,ou=Users,dc=demo,
dc=net```** | To account for the multiple forms of **```DN```** supported by a given LDAP server, specify the **```UniqueIdentifier```** parameter multiple times with different values. During a search, each **```UniqueIdentifier```** is tried in the order it is listed in the configuration file. 
**```LDAPNetworkTimeout```** | Specifies the timeout (in seconds) after which the next **```LDAPHostName```** entry will be tried, in case of no response for a connection request. This parameter is similar to **```NETWORK_TIMEOUT```** in **```ldap_conf(5)```**. Default value is 30 seconds. | **```20```** | The value must be a positive number or -1. Setting this to -1 results in an infinite timeout.
**```LDAPTimelimit```** | Specifies the time to wait when performing a search on the LDAP server for the user name. The number must be a positive integer. This parameter is similar to **```TIMELIMIT```** in **```ldap_conf(5)```**. Default value is 30 seconds. | **```15```** | The server may still apply a lower server-side limit on the duration of a search operation.
**```LDAPTimeout```** | Specifies a timeout (in seconds) after which calls to synchronous LDAP APIs will abort if no response is received. This parameter is similar to **```TIMEOUT```** in **```ldap_conf(5)```**. Default value is 30 seconds. | **```15```** | The value must be a positive number or -1. Setting this to -1 results in an infinite timeout.
**```RetryCount```** | Number of attempts to establish a successful LDAP connection. Default is 5 retries before returning an error. | **```10```** | When a failed operation is retried, it will be attempted with each configured LDAP server, until the operation is successful or the number of configured retries is exceeded.
**```RetryDelay```** | Specifies the number of seconds to delay between retries. Default value is 2 seconds. See description of **```RetryCount```**. | **```1```** | None.
**```PreserveConnection```** | Specifies whether the connection to LDAP server will be maintained (YES) or closed (NO) once the operation finishes. Default value is NO. | **```YES```** | None.
**```RefreshTime```** | Specifies the number of seconds that must have elapsed before the configuration file is reread. Default is 1800 (30 minutes). | **```3600```** | If set to zero, the configuration file is never read. The connectivity servers must be restarted for changes to take effect if this value is zero. This attribute is not specific to either configuration and must be defined in the DEFAULTS section.
**```TLS_CACERTFilename```** | Specifies the location of the certificate file for the LDAP server(s). Filename can either be fully qualified or relative to **```$CACERTS_DIR```**. | **```cert.pem```** | This attribute applies to both configurations. If a configuration does not require a certificate, this attribute is ignored. This attribute must be defined in the DEFAULTS section.
**```DefaultSectionName```** | Specifies the configuration type that will be assigned to a user by the **```REGISTER USER```** command if no authentication type is specified. In the initial Trafodion release, only one configuration is supported. | **```LOCAL```** | This attribute must be defined in the **```DEFAULTS```** section. If the **```DefaultSectionName```** attribute is specified, a section by that name (or equivalent) must be defined in **```.traf_ldapconfig```**. Legal values are **```LOCAL```** and **```ENTERPRISE```**. This syntax is likely to change.

Return to [Enabling Security Features in Trafodion](enable-secure-trafodion.html).
