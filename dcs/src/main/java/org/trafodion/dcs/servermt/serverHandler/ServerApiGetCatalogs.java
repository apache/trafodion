/**
* @@@ START COPYRIGHT @@@

* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at

*   http://www.apache.org/licenses/LICENSE-2.0

* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.

* @@@ END COPYRIGHT @@@
 */
package org.trafodion.dcs.servermt.serverHandler;

import java.net.*;
import java.io.*;
import java.nio.*;
import java.nio.channels.*;
import java.nio.channels.spi.*;
import java.sql.*;

import org.apache.trafodion.jdbc.t2.*;

import org.trafodion.dcs.Constants;
import org.trafodion.dcs.util.*;
import org.trafodion.dcs.servermt.ServerConstants;
import org.trafodion.dcs.servermt.ServerUtils;
import org.trafodion.dcs.servermt.serverDriverInputOutput.*;
import org.trafodion.dcs.servermt.serverSql.*;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

public class ServerApiGetCatalogs {

    static final int odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_ = 1;
    static final int odbc_SQLSvc_GetSQLCatalogs_InvalidConnection_exn_ = 2;
    static final int odbc_SQLSvc_GetSQLCatalogs_SQLError_exn_ = 3;
    static final int odbc_SQLSvc_GetSQLCatalogs_SQLInvalidHandle_exn_ = 4;

    private static  final Log LOG = LogFactory.getLog(ServerApiGetCatalogs.class);
    private int instance;
    private int serverThread;
    private String serverWorkerName;
    private ClientData clientData;
    private TrafConnection trafConn;
    private TrafStatement trafStmt;
    private boolean oldFormat;

    private Descriptor2List outDescList;
    private DatabaseMetaData dbmd;
    private ResultSet rs;
    private ResultSetMetaData rsmd;
    private TResultSetMetaData trsmd;
    private SQLMXResultSetMetaData strsmd;
    private String proxySyntax;
    private int resultSetCount;
    private int outCount;

    private int dialogueId;
    private String stmtLabel;
    private short APIType;
    private String catalogNm;
    private String schemaNm;
    private String tableNm;
    private String tableTypeList;
    private String columnNm;
    private int columnType;
    private int rowIdScope;
    private int nullable;
    private int uniqueness;
    private int accuracy;
    private short sqlType;
    private int metadataId;
    private String fkCatalogNm;
    private String fkSchemaNm;
    private String fkTableNm;

    //-------------T2 desc fields-------------------
    private int sqlCharset_;
    private int odbcCharset_;
    private int sqlDataType_;
    private int dataType_;
    private short sqlPrecision_;
    private short sqlDatetimeCode_;
    private int sqlOctetLength_;
    private int isNullable_;
    private String  name_;
    private int scale_;
    private int precision_;
    private boolean isSigned_;
    private boolean isCurrency_;
    private boolean isCaseSensitive_;
    private String  catalogName_;
    private String  schemaName_;
    private String  tableName_;
    private int fsDataType_;
    private int intLeadPrec_;
    private int paramMode_;
    private int paramIndex_;
    private int paramPos_;

    private int odbcPrecision_;
    private int  maxLen_;

    private int displaySize_;
    private String label_;

    private SQLWarningOrErrorList errorList = null;

    private ServerException serverException;

    ServerApiGetCatalogs(int instance, int serverThread) {
        this.instance = instance;
        this.serverThread = serverThread;
        serverWorkerName = ServerConstants.SERVER_WORKER_NAME + "_" + instance + "_" + serverThread;
     }
    void init(){
        reset();
        serverException = new ServerException();
    }
    void reset(){
        trafConn = null;
        trafStmt = null;
        oldFormat = false;
        dbmd = null;
        outDescList = null;
        outCount = 0;
        resultSetCount = 0;
        rs = null;
        rsmd = null;
        trsmd = null;
        strsmd = null;

        dialogueId = 0;
        stmtLabel = "";
        APIType = 0;
        catalogNm = "";
        schemaNm = "";
        tableNm = "";
        tableTypeList = "";
        columnNm = "";
        columnType = 0;
        rowIdScope = 0;
        nullable = 0;
        uniqueness = 0;
        accuracy = 0;
        sqlType = 0;
        metadataId = 0;
        fkCatalogNm = "";
        fkSchemaNm = "";
        fkTableNm = "";

        errorList = null;
        proxySyntax = "";

        trafConn = null;
        serverException = null;
    }
    ClientData processApi(ClientData clientData) {
        this.clientData = clientData;
        init();
//
// ==============process input ByteBuffer===========================
//
        ByteBuffer bbHeader = clientData.bbHeader;
        ByteBuffer bbBody = clientData.bbBody;
        Header hdr = clientData.hdr;

        bbHeader.flip();
        bbBody.flip();

        try {
            hdr.extractFromByteArray(bbHeader);

            dialogueId =  bbBody.getInt();
            stmtLabel = ByteBufferUtils.extractString(bbBody);
            APIType =  bbBody.getShort();
            catalogNm = ByteBufferUtils.extractString(bbBody);
            schemaNm = ByteBufferUtils.extractString(bbBody);
            tableNm = ByteBufferUtils.extractString(bbBody);
            tableTypeList = ByteBufferUtils.extractString(bbBody);
            columnNm = ByteBufferUtils.extractString(bbBody);
            columnType =  bbBody.getInt();
            rowIdScope =  bbBody.getInt();
            nullable =  bbBody.getInt();
            uniqueness =  bbBody.getInt();
            accuracy =  bbBody.getInt();
            sqlType =  bbBody.getShort();
            metadataId =  bbBody.getInt();
            fkCatalogNm = ByteBufferUtils.extractString(bbBody);
            fkSchemaNm = ByteBufferUtils.extractString(bbBody);
            fkTableNm = ByteBufferUtils.extractString(bbBody);

            if(LOG.isDebugEnabled()){
                LOG.debug(serverWorkerName + ". dialogueId :" + dialogueId);
                LOG.debug(serverWorkerName + ". stmtLabel :" + stmtLabel);
                LOG.debug(serverWorkerName + ". APIType :" + APIType);
                LOG.debug(serverWorkerName + ". catalogNm :" + catalogNm);
                LOG.debug(serverWorkerName + ". schemaNm :" + schemaNm);
                LOG.debug(serverWorkerName + ". tableNm :" + tableNm);
                LOG.debug(serverWorkerName + ". tableTypeList :" + tableTypeList);
                LOG.debug(serverWorkerName + ". columnNm :" + columnNm);
                LOG.debug(serverWorkerName + ". columnType :" + columnType);
                LOG.debug(serverWorkerName + ". rowIdScope :" + rowIdScope);
                LOG.debug(serverWorkerName + ". nullable :" + nullable);
                LOG.debug(serverWorkerName + ". uniqueness :" + uniqueness);
                LOG.debug(serverWorkerName + ". accuracy :" + accuracy);
                LOG.debug(serverWorkerName + ". sqlType :" + sqlType);
                LOG.debug(serverWorkerName + ". metadataId :" + metadataId);
                LOG.debug(serverWorkerName + ". fkCatalogNm :" + fkCatalogNm);
                LOG.debug(serverWorkerName + ". fkSchemaNm :" + fkSchemaNm);
                LOG.debug(serverWorkerName + ". fkTableNm :" + fkTableNm);
            }
            if (dialogueId < 1 ) {
                throw new SQLException(serverWorkerName + ". Wrong dialogueId :" + dialogueId);
            }
            if (dialogueId != clientData.getDialogueId() ) {
                throw new SQLException(serverWorkerName + ". Wrong dialogueId sent by the Client [sent/expected] : [" + dialogueId + "/" + clientData.getDialogueId() + "]");
            }
//=====================Process ServerApiGetCatalogs===========================
            try {
                trafConn = clientData.getTrafConnection();
                trafStmt = trafConn.createTrafStatement(stmtLabel, ServerConstants.TYPE_CATOLOG, 0);
//              trafStmt.setResultSet(null);

                dbmd = trafConn.getConnection().getMetaData();
                switch(APIType){
                    case ServerConstants.SQL_API_SQLTABLES:                 //odbc
                        if (tableNm.equals("") == false){
                           if (tableTypeList == null || tableTypeList.equals("")){
                               if(LOG.isDebugEnabled())
                                   LOG.debug(serverWorkerName + ". getTables (catalogNm :" + catalogNm + ", schemaNm :" + schemaNm + ", tableNm :" + tableNm + ", tableTypeList :null)");
                               rs = dbmd.getTables(catalogNm, schemaNm, tableNm, null);
                           }
                           else {
                               if(LOG.isDebugEnabled())
                                   LOG.debug(serverWorkerName + ". getTables (catalogNm :" + catalogNm + ", schemaNm :" + schemaNm + ", tableNm :" + tableNm + ", tableTypeList :" + tableTypeList + ")");
                               String[] tpList = tableTypeList.split(",");
                               rs = dbmd.getTables(catalogNm, schemaNm, tableNm, tpList);
                           }
                        }
                        else if (catalogNm.equals("%") == true){
                            if(LOG.isDebugEnabled())
                                LOG.debug(serverWorkerName + ". getCatalogs()");
                            rs = dbmd.getCatalogs();
                        }
                        else if (schemaNm.equals("%") == true){
                            if(LOG.isDebugEnabled())
                                LOG.debug(serverWorkerName + ". getSchemas()");
                            rs = dbmd.getSchemas();
                        }
                        break;
                    case ServerConstants.SQL_API_SQLTABLES_JDBC:
                        if (tableNm.equals("") == false){
                           if (tableTypeList == null || tableTypeList.equals("")){
                               if(LOG.isDebugEnabled())
                                   LOG.debug(serverWorkerName + ". getTables (catalogNm :" + catalogNm + ", schemaNm :" + schemaNm + ", tableNm :" + tableNm + ", tableTypeList :null)");
                               rs = dbmd.getTables(catalogNm, schemaNm, tableNm, null);
                           }
                           else {
                               if(LOG.isDebugEnabled())
                                   LOG.debug(serverWorkerName + ". getTables (catalogNm :" + catalogNm + ", schemaNm :" + schemaNm + ", tableNm :" + tableNm + ", tableTypeList :" + tableTypeList + ")");
                               String[] tpList = tableTypeList.split(",");
                               rs = dbmd.getTables(catalogNm, schemaNm, tableNm, tpList);
                           }
                        }
                        else if (catalogNm.equals("%") == true){
                            if(LOG.isDebugEnabled())
                                LOG.debug(serverWorkerName + ". getCatalogs()");
                            rs = dbmd.getCatalogs();
                        }
                        else if (schemaNm.equals("%") == true){
                            if(LOG.isDebugEnabled())
                                LOG.debug(serverWorkerName + ". getSchemas()");
                            rs = dbmd.getSchemas();
                        }
                        break;
                    case ServerConstants.SQL_API_SQLCOLUMNS:        //odbc
                        if(LOG.isDebugEnabled())
                            LOG.debug(serverWorkerName + ". getColumns (catalogNm :" + catalogNm + ", schemaNm :" + schemaNm + ", tableNm:" + tableNm + ", columnNm :" + columnNm + ")");
                        rs = dbmd.getColumns(catalogNm, schemaNm, tableNm, columnNm);
                        break;
                    case ServerConstants.SQL_API_SQLCOLUMNS_JDBC:
                        if(LOG.isDebugEnabled())
                            LOG.debug(serverWorkerName + ". getColumns (catalogNm :" + catalogNm + ", schemaNm :" + schemaNm + ", tableNm:" + tableNm + ", columnNm :" + columnNm + ")");
                        rs = dbmd.getColumns(catalogNm, schemaNm, tableNm, columnNm);
                        break;
                    case ServerConstants.SQL_API_SQLTABLEPRIVILEGES:
                        if(LOG.isDebugEnabled())
                            LOG.debug(serverWorkerName + ". getTablePrivileges (catalogNm :" + catalogNm + ", schemaNm :" + schemaNm + ", tableNm :" + tableNm + ")");
                        rs = dbmd.getTablePrivileges(catalogNm, schemaNm, tableNm);
                        break;
                    case ServerConstants.SQL_API_SQLCOLUMNPRIVILEGES:
                        if(LOG.isDebugEnabled())
                            LOG.debug(serverWorkerName + ". getColumnPrivileges (catalogNm :" + catalogNm + ", schemaNm :" + schemaNm + ", tableNm :" + tableNm + ", columnNm :" + columnNm + ")");
                        rs = dbmd.getColumnPrivileges(catalogNm, schemaNm, tableNm, columnNm);
                        break;
                    case ServerConstants.SQL_API_SQLSPECIALCOLUMNS:
                        if (columnType == ServerConstants.SQL_BEST_ROWID ){
                            if(LOG.isDebugEnabled())
                                LOG.debug(serverWorkerName + ". getBestRowIdentifier (catalogNm :" + catalogNm + ", schemaNm :" + schemaNm + ", tableNm :" + tableNm + ", rowIdScope :" + rowIdScope + ", nullable :" + ((nullable == 1)? true : false) + ")");
                            rs = dbmd.getBestRowIdentifier(catalogNm, schemaNm, tableNm, rowIdScope, (nullable == 1)? true : false);
                        }
                        else if (columnType == ServerConstants.SQL_ROWVER){
                            if(LOG.isDebugEnabled())
                                LOG.debug(serverWorkerName + ". getVersionColumns (catalogNm :" + catalogNm + ", schemaNm :" + schemaNm + ", tableNm :" + tableNm + ")");
                            rs = dbmd.getVersionColumns(catalogNm, schemaNm, tableNm);
                        }
                        break;
                   case ServerConstants.SQL_API_SQLPROCEDURES:
                        if(LOG.isDebugEnabled())
                           LOG.debug(serverWorkerName + ". getProcedures (catalogNm :" + catalogNm + ", schemaNm :" + schemaNm + ", tableNm :" + tableNm + ")");
                        rs = dbmd.getProcedures(catalogNm, schemaNm, tableNm);
                        break;
                    case ServerConstants.SQL_API_SQLPROCEDURECOLUMNS:
                        if(LOG.isDebugEnabled())
                            LOG.debug(serverWorkerName + ". getProcedureColumns (catalogNm :" + catalogNm + ", schemaNm :" + schemaNm + ", tableNm :" + tableNm + ", columnNm :" + columnNm + ")");
                        rs = dbmd.getProcedureColumns(catalogNm, schemaNm, tableNm, columnNm);
                        break;
                    case ServerConstants.SQL_API_SQLPRIMARYKEYS:
                        if(LOG.isDebugEnabled())
                            LOG.debug(serverWorkerName + ". getPrimaryKeys (catalogNm :" + catalogNm + ", schemaNm :" + schemaNm + ", tableNm :" + tableNm + ")");
                        rs = dbmd.getPrimaryKeys(catalogNm, schemaNm, tableNm);
                        break;
                    case ServerConstants.SQL_API_SQLFOREIGNKEYS:
                        if (fkCatalogNm.equals("") && fkSchemaNm.equals("") && fkTableNm.equals("")){
                            if(LOG.isDebugEnabled())
                                LOG.debug(serverWorkerName + ". getExportedKeys (catalogNm :" + catalogNm + ", schemaNm :" + schemaNm + ", tableNm :" + tableNm + ")");
                            rs = dbmd.getExportedKeys(catalogNm, schemaNm, tableNm);
                        }
                        else{
                            if(LOG.isDebugEnabled())
                                LOG.debug(serverWorkerName + ". getImportedKeys (fkCatalogNm :" + fkCatalogNm + ", fkSchemaNm :" + fkSchemaNm + ", fkTableNm :" + fkTableNm + ")");
                            rs = dbmd.getImportedKeys(fkCatalogNm, fkSchemaNm, fkTableNm);
                        }
                        break;
                    case ServerConstants.SQL_API_SQLGETTYPEINFO:
                        if(LOG.isDebugEnabled())
                            LOG.debug(serverWorkerName + ". getTypeInfo()");
                        rs = dbmd.getTypeInfo();
                        break;
                    case ServerConstants.SQL_API_SQLSTATISTICS:
                        if(LOG.isDebugEnabled())
                            LOG.debug(serverWorkerName + ". getIndexInfo (catalogNm :" + catalogNm + ", schemaNm :" + schemaNm + ", tableNm :" + tableNm + ", uniqueness :" + ((uniqueness == 1)? true : false) + ", true)");
                        rs = dbmd.getIndexInfo(catalogNm, schemaNm, tableNm, (uniqueness == 1)? true : false, true);
                       break;
                    default:
                        throw new SQLException(serverWorkerName + ". Unknown APIType sent by the Client : [" + APIType + "]", "HY024");
                }
//              trafStmt.setResultSet(rs);
                rsmd = rs.getMetaData();
                outCount = rsmd.getColumnCount();

            } catch (SQLException se){
                LOG.error(serverWorkerName + ". GetCatalogs.SQLException " + se);
                errorList = new SQLWarningOrErrorList(se);
                serverException.setServerException (odbc_SQLSvc_GetSQLCatalogs_SQLError_exn_, 0, se);
            } catch (Exception ex){
                LOG.error(serverWorkerName + ". GetCatalogs.Exception " + ex);
                throw ex;
            }
            if (outCount > 0){
                strsmd = (SQLMXResultSetMetaData)rsmd;
                outDescList = new Descriptor2List(outCount, true);

                resultSetCount = 1;
                for (int column = 1; column <= outCount; column++){
                    sqlCharset_ = strsmd.getSqlCharset(column);
                    odbcCharset_ = strsmd.getOdbcCharset(column);
                    sqlDataType_ = strsmd.getSqlDataType(column);
                    dataType_ = strsmd.getDataType(column);
                    sqlPrecision_ = strsmd.getSqlPrecision(column);
                    sqlDatetimeCode_ = strsmd.getSqlDatetimeCode(column);
                    sqlOctetLength_ = strsmd.getSqlOctetLength(column);
                    isNullable_ = strsmd.getIsNullable(column);
                    name_ = strsmd.getName(column);
                    scale_ = strsmd.getScale(column);
                    precision_ = strsmd.getPrecision(column);
                    isSigned_ = strsmd.getIsSigned(column);
                    isCurrency_ = strsmd.getIsCurrency(column);
                    isCaseSensitive_ = strsmd.getIsCaseSensitive(column);
                    catalogName_ = strsmd.getCatalogName(column);
                    schemaName_ = strsmd.getSchemaName(column);
                    tableName_ = strsmd.getTableName(column);
                    fsDataType_ = strsmd.getFsDataType(column);
                    intLeadPrec_ = strsmd.getIntLeadPrec(column);
                    paramMode_ = strsmd.getMode(column);
                    paramIndex_ = strsmd.getIndex(column);
                    paramPos_ = strsmd.getPos(column);

                    odbcPrecision_ = strsmd.getOdbcPrecision(column);
                    maxLen_ = strsmd.getMaxLen(column);

                    displaySize_ = strsmd.getDisplaySize(column);
                    label_ = strsmd.getLabel(column);

                    Descriptor2 columnDesc = new Descriptor2(sqlCharset_,odbcCharset_,sqlDataType_,dataType_,sqlPrecision_,sqlDatetimeCode_,
                        sqlOctetLength_,isNullable_,name_,scale_,precision_,isSigned_,
                        isCurrency_,isCaseSensitive_,catalogName_,schemaName_,tableName_,
                        fsDataType_,intLeadPrec_,paramMode_,paramIndex_,paramPos_,odbcPrecision_,
                        maxLen_,displaySize_,label_, true);
                    outDescList.addDescriptor(column,columnDesc);
                }
                if(LOG.isDebugEnabled()){
                    for (int column = 1; column <= outCount; column++){
                        Descriptor2 dsc = outDescList.getDescriptors2()[column-1];
                        LOG.debug(serverWorkerName + ". [" + column + "] Column descriptor -------------" );
                        LOG.debug(serverWorkerName + ". oldFormat " + column + " :" + dsc.getOldFormat());
                        LOG.debug(serverWorkerName + ". noNullValue " + column + " :" + dsc.getNoNullValue());
                        LOG.debug(serverWorkerName + ". nullValue " + column + " :" + dsc.getNullValue());
                        LOG.debug(serverWorkerName + ". version " + column + " :" + dsc.getVersion());
                        LOG.debug(serverWorkerName + ". dataType " + column + " :" + SqlUtils.getDataType(dsc.getDataType()));
                        LOG.debug(serverWorkerName + ". datetimeCode " + column + " :" + dsc.getDatetimeCode());
                        LOG.debug(serverWorkerName + ". maxLen " + column + " :" + dsc.getMaxLen());
                        LOG.debug(serverWorkerName + ". precision " + column + " :" + dsc.getPrecision());
                        LOG.debug(serverWorkerName + ". scale " + column + " :" + dsc.getScale());
                        LOG.debug(serverWorkerName + ". nullInfo " + column + " :" + dsc.getNullInfo());
                        LOG.debug(serverWorkerName + ". signed " + column + " :" + dsc.getSigned());
                        LOG.debug(serverWorkerName + ". odbcDataType " + column + " :" + dsc.getOdbcDataType());
                        LOG.debug(serverWorkerName + ". odbcPrecision " + column + " :" + dsc.getOdbcPrecision());
                        LOG.debug(serverWorkerName + ". sqlCharset " + column + " :" + SqlUtils.getCharsetName(dsc.getSqlCharset()) + "[" + dsc.getSqlCharset() + "]");
                        LOG.debug(serverWorkerName + ". odbcCharset " + column + " :" + SqlUtils.getCharsetName(dsc.getOdbcCharset()) + "[" + dsc.getOdbcCharset() + "]");
                        LOG.debug(serverWorkerName + ". colHeadingNm " + column + " :" + dsc.getColHeadingNm());
                        LOG.debug(serverWorkerName + ". tableName " + column + " :" + dsc.getTableName());
                        LOG.debug(serverWorkerName + ". schemaName " + column + " :" + dsc.getSchemaName());
                        LOG.debug(serverWorkerName + ". headingName " + column + " :" + dsc.getHeadingName());
                        LOG.debug(serverWorkerName + ". intLeadPrec " + column + " :" + dsc.getParamMode());
                        LOG.debug(serverWorkerName + ". paramMode " + column + " :" + dsc.getColHeadingNm());
                        LOG.debug(serverWorkerName + ". varLength " + column + " :" + dsc.getVarLength());
                        LOG.debug(serverWorkerName + ". Column descriptor End-------------");
                    }
                }
            }
            if (outCount > 0){
                trafStmt.addTResultSet(new TrafResultSet(rs, 0, stmtLabel, 0, outDescList,""));
            }
//
//===================calculate length of output ByteBuffer========================
//
            bbHeader.clear();
            bbBody.clear();
//
// check if ByteBuffer is big enough for output
//
            int dataLength = serverException.lengthOfData();
            dataLength += ByteBufferUtils.lengthOfString(stmtLabel); //stmtLabel
            if (outDescList != null)
                dataLength += outDescList.lengthOfData();
            else
                dataLength += ServerConstants.INT_FIELD_SIZE;
            if (errorList != null)
                dataLength += errorList.lengthOfData();
            else
                dataLength += ServerConstants.INT_FIELD_SIZE;
            dataLength += ByteBufferUtils.lengthOfString(proxySyntax); //proxySyntax

            int availableBuffer = bbBody.capacity() - bbBody.position();
            if(LOG.isDebugEnabled())
                LOG.debug(serverWorkerName + ". dataLength :" + dataLength + " availableBuffer :" + availableBuffer);
            if (dataLength > availableBuffer )
                bbBody = ByteBufferUtils.increaseCapacity(bbBody, dataLength > ServerConstants.BODY_SIZE ? dataLength : ServerConstants.BODY_SIZE );

//===================== build output ==============================================
            serverException.insertIntoByteBuffer(bbBody);
            ByteBufferUtils.insertString(stmtLabel, bbBody);
            if (outDescList != null)
                outDescList.insertIntoByteBuffer(bbBody);
            else
                bbBody.putInt(0);
            if (errorList != null)
                errorList.insertIntoByteBuffer(bbBody);
            else
                bbBody.putInt(0);

            ByteBufferUtils.insertString(proxySyntax, bbBody);
            bbBody.flip();
//=========================Update header================================
            hdr.setTotalLength(bbBody.limit());
            hdr.insertIntoByteBuffer(bbHeader);
            bbHeader.flip();

            clientData.setByteBufferArray(bbHeader, bbBody);
            clientData.setHdr(hdr);
            clientData.setRequest(ServerConstants.REQUST_WRITE_READ);

        } catch (UnsupportedEncodingException ue){
            LOG.error(serverWorkerName + ". GetCatalogs.UnsupportedEncodingException :" + ue);
            clientData.setRequestAndDisconnect();
        } catch (Exception e){
            LOG.error(serverWorkerName + ". GetCatalogs.Exception :" + e);
            clientData.setRequestAndDisconnect();
        }
        reset();
        return clientData;
    }
}

