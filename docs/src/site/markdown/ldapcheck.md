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
This page describes ldapcheck, which is a tool that is used to validate the Trafodion authentication configuration and attempt to connect to a configured LDAP server.

# Usage

```
ldapcheck  [<option>]...
<option> ::= --help|-h            display usage information
             --username=<LDAP-username>
             --password[=<password>]
             --primary            Use first configuration
             --local              Use first configuration
             --enterprise         Use first configuration
             --secondary          Use second configuration
             --remote             Use second configuration
             --cluster            Use second configuration
             --verbose            Display non-zero retry counts
                                       and LDAP errors
```

# Considerations

* Aliases for primary include enterprise and local. Aliases for secondary include cluster and remote. If no configuration is specified, primary is assumed.
* The equals sign is required when supplying a value to username or password.
* To be prompted for a password value with no echo, specify the password argument but omit the equals sign and value.
* Passwords that contain special characters may need to be escaped if the password is specified on the command line or within a script file.
* If the password keyword is not specified, only the username will be checked. The tool can therefore be used to test the LDAP configuration and connection to the configured LDAP server(s) without knowing a valid username or password.

