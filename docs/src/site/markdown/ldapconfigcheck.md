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
This page describes the **```ldapconfigcheck```** tool, which validates the syntactic correctness of a Trafodion authentication configuration file. Trafodion does not need to be running to run the tool.

# Considerations
If the configuration filename is not specified, the tool will look for a file using environment variables. Those environment variables and the search order are:

1. TRAFAUTH_CONFIGFILE

    A fully qualified name is expected.
2. TRAFAUTH_CONFIGDIR

    Filename **```.traf_authentication_config/```** is appended to the specified directory
3. MY_SQROOT

    **```/sql/scripts/.traf_authentication_config```** is appended to the value of **```MY_SQROOT```**.
    
# Errors
One of the following is output when the tool is run. Only the first error encountered is reported.

Code             | Text
-----------------|--------------------------------------------------------------------------------
**```0```**      | File *filename* is valid.
**```1```**      | File *filename* not found.
**```2```**      | File: *filename*<br /><br />Invalid attribute name on line *line-number*.
**```3```**      | File: *filename*<br /><br />Missing required value on line *line-number*.
**```4```**      | File: *filename*<br /><br />Value out of range on line *line-number*.
**```5```**      | File: *filename*<br /><br />Open of traf_authentication_config file failed.
**```6```**      | File: *filename*<br /><br />Read of traf_authentication_config file failed.
**```7```**      | No file provided. Either specify a file parameter or verify environment variables.
**```8```**      | TLS was requested in at least one section, but TLS_CACERTFilename was not provided.
**```9```**      | Missing host name in at least one section.<br /><br />Each LDAP connection configuration section must provide at least one host name.
**```10```**     | Missing unique identifier in at least one section.<br /><br />Each LDAP connection configuration section must provide at least one unique identifier. 
**```11```**     | At least one LDAP connection configuration section must be specified.
**```12```**     | Internal error parsing . traf_authentication_config.

