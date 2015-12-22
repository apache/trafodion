#!/bin/bash
# @@@ START COPYRIGHT @@@
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
# @@@ END COPYRIGHT @@@

SERVER_JAR=${MY_SQROOT}/export/lib/spj_mgmt.jar
CI=sqlci
CATALOG_NAME=TRAFODION
CIS_SCHEMA="_SPJ_"


function dropAndCreateSchema {
    echo "Creating Schema for SPJ_MGMT"
    ${CI} << sqlciEOF

      cqd CAT_IGNORE_ALREADY_EXISTS_ERROR 'on';
      cqd CAT_IGNORE_DOES_NOT_EXIST_ERROR 'on';

      DROP SCHEMA $CATALOG_NAME.$CIS_SCHEMA CASCADE;
      CREATE SCHEMA $CATALOG_NAME.$CIS_SCHEMA;

      cqd CAT_IGNORE_ALREADY_EXISTS_ERROR 'off';
      cqd CAT_IGNORE_DOES_NOT_EXIST_ERROR 'off';

      exit;
sqlciEOF
}

function createProcedures {

    echo "Creating Procedures in  schema '$CATALOG_NAME.$CIS_SCHEMA' "
    ${CI} << procEOF

      cqd CAT_IGNORE_ALREADY_EXISTS_ERROR 'on';
      cqd CAT_IGNORE_DOES_NOT_EXIST_ERROR 'on';

      -- Creating Procedures 
      set schema $CATALOG_NAME.$CIS_SCHEMA; 

      DROP LIBRARY SPJMGMT CASCADE;
      CREATE LIBRARY SPJMGMT FILE '${SERVER_JAR}';
   
      CREATE PROCEDURE HELP (
      INOUT COMMANDNAME VARCHAR(2560) CHARACTER SET ISO88591)
      EXTERNAL NAME 'org.trafodion.spjmgmt.FileMgmt.help (java.lang.String[])'
      EXTERNAL SECURITY DEFINER
      LIBRARY SPJMGMT
      LANGUAGE JAVA
      PARAMETER STYLE JAVA
      READS SQL DATA
      ;
      GRANT EXECUTE ON PROCEDURE HELP TO PUBLIC;
      
      CREATE PROCEDURE PUT (
      IN FILEDATA VARCHAR(102400) CHARACTER SET ISO88591,
      IN FILENAME VARCHAR(256) CHARACTER SET ISO88591,
      IN CREATEFLAG INTEGER)
      EXTERNAL NAME 'org.trafodion.spjmgmt.FileMgmt.put(java.lang.String,java.lang.String,int)'
      EXTERNAL SECURITY DEFINER
      LIBRARY SPJMGMT
      LANGUAGE JAVA
      PARAMETER STYLE JAVA
      READS SQL DATA
      ;
      GRANT EXECUTE ON PROCEDURE PUT TO PUBLIC;
      
      CREATE PROCEDURE LS (
      IN FILENAME VARCHAR(256) CHARACTER SET ISO88591,
      OUT FILENAMES VARCHAR(10240) CHARACTER SET ISO88591)
      EXTERNAL NAME 'org.trafodion.spjmgmt.FileMgmt.ls(java.lang.String,java.lang.String[])'
      EXTERNAL SECURITY DEFINER
      LIBRARY SPJMGMT
      LANGUAGE JAVA
      PARAMETER STYLE JAVA
      READS SQL DATA
      ;
      GRANT EXECUTE ON PROCEDURE LS TO PUBLIC;
      
      CREATE PROCEDURE LSALL (
      OUT FILENAMES VARCHAR(10240) CHARACTER SET ISO88591)
      EXTERNAL NAME 'org.trafodion.spjmgmt.FileMgmt.lsAll(java.lang.String[])'
      EXTERNAL SECURITY DEFINER
      LIBRARY SPJMGMT
      LANGUAGE JAVA
      PARAMETER STYLE JAVA
      READS SQL DATA
      ;
      GRANT EXECUTE ON PROCEDURE LSALL TO PUBLIC;
      
      CREATE PROCEDURE RM (
      IN FILENAME VARCHAR(256) CHARACTER SET ISO88591)
      EXTERNAL NAME 'org.trafodion.spjmgmt.FileMgmt.rm(java.lang.String)'
      EXTERNAL SECURITY DEFINER
      LIBRARY SPJMGMT
      LANGUAGE JAVA
      PARAMETER STYLE JAVA
      READS SQL DATA
      ;
      GRANT EXECUTE ON PROCEDURE RM TO PUBLIC;
      
      CREATE PROCEDURE RMREX (
      IN FILENAME VARCHAR(256) CHARACTER SET ISO88591,
      OUT FILENAMES VARCHAR(10240) CHARACTER SET ISO88591)
      EXTERNAL NAME 'org.trafodion.spjmgmt.FileMgmt.rmRex(java.lang.String, java.lang.String[])'
      EXTERNAL SECURITY DEFINER
      LIBRARY SPJMGMT
      LANGUAGE JAVA
      PARAMETER STYLE JAVA
      READS SQL DATA
      ;
      GRANT EXECUTE ON PROCEDURE RMREX TO PUBLIC;

      CREATE PROCEDURE GETFILE (
      IN FILENAME VARCHAR(256) CHARACTER SET UTF8,
      IN OFFSET INTEGER,
      OUT FILEDATA VARCHAR(51200) CHARACTER SET UTF8,
      OUT DATALENGTH LARGEINT)
      EXTERNAL NAME 'org.trafodion.spjmgmt.FileMgmt.get (java.lang.String,int,java.lang.String[],long[])'
      EXTERNAL SECURITY DEFINER
      LIBRARY SPJMGMT
      LANGUAGE JAVA
      PARAMETER STYLE JAVA
      READS SQL DATA
      ;
      GRANT EXECUTE ON PROCEDURE GETFILE TO PUBLIC;
      
      cqd CAT_IGNORE_ALREADY_EXISTS_ERROR 'off';
      cqd CAT_IGNORE_DOES_NOT_EXIST_ERROR 'off';

      exit;
procEOF
    
}

dropAndCreateSchema
createProcedures
echo "Successfully completed installation of SPJs."
