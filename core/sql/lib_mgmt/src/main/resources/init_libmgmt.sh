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

SERVER_JAR=${TRAF_HOME}/export/lib/lib_mgmt.jar
CI=sqlci
CATALOG_NAME=TRAFODION
LIB_SCHEMA="DB__LIBMGR"
DB__LIBMGRROLE=DB_LIBMGRROLE
LIB_NAME=DB__LIBMGRNAME

function dropAndCreateSchema {
    echo "Creating Schema for ${LIB_SCHEMA}"
    ${CI} << sqlciEOF

      cqd CAT_IGNORE_ALREADY_EXISTS_ERROR 'on';
      cqd CAT_IGNORE_DOES_NOT_EXIST_ERROR 'on';

      DROP SCHEMA IF EXISTS $CATALOG_NAME.$LIB_SCHEMA CASCADE;
      CREATE SCHEMA $CATALOG_NAME.$LIB_SCHEMA;
      set schema $CATALOG_NAME.$LIB_SCHEMA;
      DROP LIBRARY ${LIB_NAME} CASCADE;
      CREATE LIBRARY ${LIB_NAME} FILE '${SERVER_JAR}';
      CREATE ROLE ${DB__LIBMGRROLE};

      cqd CAT_IGNORE_ALREADY_EXISTS_ERROR 'off';
      cqd CAT_IGNORE_DOES_NOT_EXIST_ERROR 'off';

      exit;
sqlciEOF
}

function createProcedures {

    echo "Creating Procedures in  schema '$CATALOG_NAME.$LIB_SCHEMA' "
    ${CI} << procEOF

      cqd CAT_IGNORE_ALREADY_EXISTS_ERROR 'on';
      cqd CAT_IGNORE_DOES_NOT_EXIST_ERROR 'on';

      -- Creating Procedures 
      set schema $CATALOG_NAME.$LIB_SCHEMA;
   
      CREATE PROCEDURE HELP (
      INOUT COMMANDNAME VARCHAR(2560) CHARACTER SET ISO88591)
      EXTERNAL NAME 'org.trafodion.libmgmt.FileMgmt.help (java.lang.String[])'
      EXTERNAL SECURITY DEFINER
      LIBRARY ${LIB_NAME}
      LANGUAGE JAVA
      PARAMETER STYLE JAVA
      READS SQL DATA
      ;
      GRANT EXECUTE ON PROCEDURE HELP TO ${DB__LIBMGRROLE};

      CREATE PROCEDURE ADDLIB (
      IN LIBNAME VARCHAR(1024) CHARACTER SET UTF8,
      IN FILENAME VARCHAR(1024) CHARACTER SET UTF8,
      IN HOSTNAME VARCHAR(1024) CHARACTER SET UTF8,
      IN LOCALFILE VARCHAR(1024) CHARACTER SET UTF8)
      EXTERNAL NAME 'org.trafodion.libmgmt.FileMgmt.addLib (java.lang.String,java.lang.String,java.lang.String,java.lang.String)'
      LIBRARY ${LIB_NAME}
      EXTERNAL SECURITY DEFINER
      LANGUAGE JAVA
      PARAMETER STYLE JAVA
      CONTAINS SQL
      NO TRANSACTION REQUIRED
      ;
      GRANT EXECUTE ON PROCEDURE ADDLIB TO ${DB__LIBMGRROLE};

      CREATE PROCEDURE DROPLIB (
      IN LIBNAME VARCHAR(1024) CHARACTER SET UTF8,
      IN MODETYPE VARCHAR(1024) CHARACTER SET ISO88591)
      EXTERNAL NAME 'org.trafodion.libmgmt.FileMgmt.dropLib (java.lang.String,java.lang.String)'
      EXTERNAL SECURITY DEFINER
      LIBRARY ${LIB_NAME}
      LANGUAGE JAVA
      PARAMETER STYLE JAVA
      CONTAINS SQL
      NO TRANSACTION REQUIRED
      ;
      GRANT EXECUTE ON PROCEDURE DROPLIB TO ${DB__LIBMGRROLE};

      CREATE PROCEDURE ALTERLIB (
      IN LIBNAME VARCHAR(1024) CHARACTER SET UTF8,
      IN FILENAME VARCHAR(1024) CHARACTER SET UTF8,
      IN HOSTNAME VARCHAR(1024) CHARACTER SET UTF8,
      IN LOCALFILE VARCHAR(1024) CHARACTER SET UTF8)
      EXTERNAL NAME 'org.trafodion.libmgmt.FileMgmt.alterLib (java.lang.String,java.lang.String,java.lang.String,java.lang.String)'
      EXTERNAL SECURITY DEFINER
      LIBRARY ${LIB_NAME}
      LANGUAGE JAVA
      PARAMETER STYLE JAVA
      CONTAINS SQL
      NO TRANSACTION REQUIRED
      ;
      GRANT EXECUTE ON PROCEDURE ALTERLIB TO ${DB__LIBMGRROLE};

      CREATE PROCEDURE PUT (
      IN FILEDATA VARCHAR(102400) CHARACTER SET ISO88591,
      IN FILENAME VARCHAR(256) CHARACTER SET UTF8,
      IN CREATEFLAG INTEGER,
      IN FILEOVERWRITE INTEGER)
      EXTERNAL NAME 'org.trafodion.libmgmt.FileMgmt.put(java.lang.String,java.lang.String,int,int)'
      EXTERNAL SECURITY DEFINER
      LIBRARY ${LIB_NAME}
      LANGUAGE JAVA
      PARAMETER STYLE JAVA
      READS SQL DATA
      NO TRANSACTION REQUIRED
      ;
      GRANT EXECUTE ON PROCEDURE PUT TO ${DB__LIBMGRROLE};
      
      CREATE PROCEDURE LS (
      IN FILENAME VARCHAR(256) CHARACTER SET UTF8,
      OUT FILENAMES VARCHAR(10240) CHARACTER SET ISO88591)
      EXTERNAL NAME 'org.trafodion.libmgmt.FileMgmt.ls(java.lang.String,java.lang.String[])'
      EXTERNAL SECURITY DEFINER
      LIBRARY ${LIB_NAME}
      LANGUAGE JAVA
      PARAMETER STYLE JAVA
      READS SQL DATA
      NO TRANSACTION REQUIRED
      ;
      GRANT EXECUTE ON PROCEDURE LS TO ${DB__LIBMGRROLE};
      
      CREATE PROCEDURE LSALL (
      OUT FILENAMES VARCHAR(10240) CHARACTER SET ISO88591)
      EXTERNAL NAME 'org.trafodion.libmgmt.FileMgmt.lsAll(java.lang.String[])'
      EXTERNAL SECURITY DEFINER
      LIBRARY ${LIB_NAME}
      LANGUAGE JAVA
      PARAMETER STYLE JAVA
      READS SQL DATA
      NO TRANSACTION REQUIRED
      ;
      GRANT EXECUTE ON PROCEDURE LSALL TO ${DB__LIBMGRROLE};
      
      CREATE PROCEDURE RM (
      IN FILENAME VARCHAR(256) CHARACTER SET UTF8)
      EXTERNAL NAME 'org.trafodion.libmgmt.FileMgmt.rm(java.lang.String)'
      EXTERNAL SECURITY DEFINER
      LIBRARY ${LIB_NAME}
      LANGUAGE JAVA
      PARAMETER STYLE JAVA
      READS SQL DATA
      NO TRANSACTION REQUIRED
      ;
      GRANT EXECUTE ON PROCEDURE RM TO ${DB__LIBMGRROLE};
      
      CREATE PROCEDURE RMREX (
      IN FILENAME VARCHAR(256) CHARACTER SET UTF8,
      OUT FILENAMES VARCHAR(10240) CHARACTER SET ISO88591)
      EXTERNAL NAME 'org.trafodion.libmgmt.FileMgmt.rmRex(java.lang.String, java.lang.String[])'
      EXTERNAL SECURITY DEFINER
      LIBRARY ${LIB_NAME}
      LANGUAGE JAVA
      PARAMETER STYLE JAVA
      READS SQL DATA
      NO TRANSACTION REQUIRED
      ;
      GRANT EXECUTE ON PROCEDURE RMREX TO ${DB__LIBMGRROLE};

      CREATE PROCEDURE GETFILE (
      IN FILENAME VARCHAR(256) CHARACTER SET UTF8,
      IN OFFSET INTEGER,
      OUT FILEDATA VARCHAR(12800) CHARACTER SET ISO88591,
      OUT DATALENGTH LARGEINT)
      EXTERNAL NAME 'org.trafodion.libmgmt.FileMgmt.get (java.lang.String,int,java.lang.String[],long[])'
      EXTERNAL SECURITY DEFINER
      LIBRARY ${LIB_NAME}
      LANGUAGE JAVA
      PARAMETER STYLE JAVA
      READS SQL DATA
      NO TRANSACTION REQUIRED
      ;
      GRANT EXECUTE ON PROCEDURE GETFILE TO ${DB__LIBMGRROLE};
            
      CREATE PROCEDURE CONNECTBY(IN NEWTABLENAME VARCHAR(1024) CHARACTER SET UTF8,
      IN SOURCETABLENAME VARCHAR(1024) CHARACTER SET UTF8,
      IN COLUMNNAME VARCHAR(1024) CHARACTER SET UTF8, 
      IN PARENTCOLUMNNAME VARCHAR(1024) CHARACTER SET UTF8) 
      EXTERNAL NAME 'org.trafodion.libmgmt.FileMgmt.connectBy (java.lang.String,java.lang.String,java.lang.String,java.lang.String)' 
      LIBRARY ${LIB_NAME}
      EXTERNAL SECURITY DEFINER 
      LANGUAGE JAVA 
      PARAMETER STYLE JAVA 
      CONTAINS SQL   
      DYNAMIC RESULT SETS 0  
      NO TRANSACTION REQUIRED
      ;
      GRANT EXECUTE ON PROCEDURE CONNECTBY TO ${DB__LIBMGRROLE};
      
      cqd CAT_IGNORE_ALREADY_EXISTS_ERROR 'off';
      cqd CAT_IGNORE_DOES_NOT_EXIST_ERROR 'off';

      exit;
procEOF
    
}

dropAndCreateSchema
createProcedures
echo "Successfully completed installation of procedures."
