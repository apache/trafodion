/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.trafodion.dcs.util;

import java.math.BigDecimal;
import java.math.BigInteger;
import java.sql.SQLException;
import java.sql.Types;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Hashtable;
import java.util.Arrays;
import java.io.UnsupportedEncodingException;

import org.trafodion.dcs.Constants;
import org.trafodion.dcs.servermt.ServerConstants;
import org.trafodion.dcs.servermt.serverDriverInputOutput.Descriptor2;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

public class SqlUtils {

    private static  final Log LOG = LogFactory.getLog(SqlUtils.class);

    private SqlUtils() {
    }

    static private Hashtable valueToCharset;
    static {
        valueToCharset = new Hashtable(11);
        valueToCharset.put(new Integer(1), "ISO8859_1"); // ISO
        valueToCharset.put(new Integer(10), "MS932"); // SJIS
        valueToCharset.put(new Integer(11), "UTF-16BE"); // UCS2
        valueToCharset.put(new Integer(12), "EUCJP"); // EUCJP
        valueToCharset.put(new Integer(13), "MS950"); // BIG5
        valueToCharset.put(new Integer(14), "GB18030"); // GB18030
        valueToCharset.put(new Integer(15), "UTF-8"); // UTF8
        valueToCharset.put(new Integer(16), "MS949"); // MB_KSC5601
        valueToCharset.put(new Integer(17), "GB2312"); // GB2312
    }
    static private Hashtable charsetToValue;
    static {
        charsetToValue = new Hashtable(11);
        charsetToValue.put("ISO8859_1", new Integer(1)); // ISO
        charsetToValue.put("MS932", new Integer(10)); // SJIS
        charsetToValue.put("UTF-16BE", new Integer(11)); // UCS2
        charsetToValue.put("EUCJP", new Integer(12)); // EUCJP
        charsetToValue.put("MS950", new Integer(13)); // BIG5
        charsetToValue.put("GB18030", new Integer(14)); // GB18030
        charsetToValue.put("UTF-8", new Integer(15)); // UTF8
        charsetToValue.put("MS949", new Integer(16)); // MB_KSC5601
        charsetToValue.put("GB2312", new Integer(17)); // GB2312
    }

    static final int SQLCHARSETCODE_UNKNOWN = 0;
    static final String SQLCHARSET_UNKNOWN = "UNKNOWN";

    // these are the only real column types
    static public final int SQLCHARSETCODE_ISO88591 = 1;
    static public final String SQLCHARSET_ISO88591 = "ISO88591";
    static public final int SQLCHARSETCODE_UNICODE = 11;
    static public final String SQLCHARSET_UNICODE = "UCS2";

    // ISO_MAPPING values
    static public final int SQLCHARSETCODE_SJIS = 10;
    static public final int SQLCHARSETCODE_UTF8 = 15;

    static public String getCharsetName(int charset) {
        String ret = (String) valueToCharset.get(new Integer(charset));

        if (ret == null)
            ret = SQLCHARSET_UNKNOWN;

        return ret;
    }

    static public int getCharsetValue(String charset) {
        Integer i = (Integer) charsetToValue.get(charset);
        int ret;

        if (i == null)
            ret = SQLCHARSETCODE_UNKNOWN;
        else
            ret = i.intValue();

        return ret;
    }

    static public String getComponentId(int componentId){
        String str = "UNKNOWN Component [" + componentId + "]";
        switch (componentId){
            case ServerConstants.DCS_MASTER_COMPONENT:
                str = "DCS_MASTER_COMPONENT [" + componentId + "]";
                break;
            case ServerConstants.SQL_COMPONENT:
                str = "SQL_COMPONENT [" + componentId + "]";
                break;
            case ServerConstants.ODBC_SRVR_COMPONENT:
                str = "ODBC_SRVR_COMPONENT [" + componentId + "]";
                break;
            case ServerConstants.DRVR_COMPONENT:
                str = "DRVR_COMPONENT [" + componentId + "]";
                break;
            case ServerConstants.APP_COMPONENT:
                str = "APP_COMPONENT [" + componentId + "]";
                break;
            case ServerConstants.JDBC_DRVR_COMPONENT:
                str = "JDBC_DRVR_COMPONENT [" + componentId + "]";
                break;
            case ServerConstants.LINUX_DRVR_COMPONENT:
                str = "LINUX_DRVR_COMPONENT [" + componentId + "]";
                break;
            case ServerConstants.DOT_NET_DRVR_COMPONENT:
                str = "DOT_NET_DRVR_COMPONENT [" + componentId + "]";
                break;
            case ServerConstants.WIN_UNICODE_DRVR_COMPONENT:
                str = "WIN_UNICODE_DRVR_COMPONENT [" + componentId + "]";
                break;
            case ServerConstants.LINUX_UNICODE_DRVR_COMPONENT:
                str = "LINUX_UNICODE_DRVR_COMPONENT [" + componentId + "]";
                break;
            default:
        }
        return str;
    }
    static public int getSqlStmtType(int stmtType) {
        int retQueryType;
        switch(stmtType){
            case ServerConstants.TYPE_SELECT:
                retQueryType = ServerConstants.SQL_SELECT_NON_UNIQUE;
                break;
            case ServerConstants.TYPE_UPDATE:
                retQueryType = ServerConstants.SQL_UPDATE_NON_UNIQUE;
                break;
            case ServerConstants.TYPE_DELETE:
                retQueryType = ServerConstants.SQL_DELETE_NON_UNIQUE;
                break;
            case ServerConstants.TYPE_INSERT:
                retQueryType = ServerConstants.SQL_INSERT_NON_UNIQUE;
                break;
            default:
                retQueryType = stmtType;
        }
        return retQueryType;
    }
    static public short getSqlStmtType(String str) {
        //
        // Kludge to determin if the type of statement.
        //
        String tokens[] = str.split("[^a-zA-Z]+", 3);
        short rt1 = ServerConstants.TYPE_UNKNOWN;
        String str3 = "";
        //
        // If there are no separators (i.e. no spaces, {, =, etc.) in front of
        // the
        // first token, then the first token is the key word we are looking for.
        // Else, the first token is an empty string (i.e. split thinks the first
        // token is the empty string followed by a separator), and the second
        // token is the key word we are looking for.
        //
        if (tokens[0].length() > 0) {
            str3 = tokens[0].toUpperCase();
        } else {
            str3 = tokens[1].toUpperCase();
        }
        if ((str3.equals("SELECT")) || (str3.equals("SHOWSHAPE"))
                || (str3.equals("INVOKE")) || (str3.equals("SHOWCONTROL"))
                || (str3.equals("SHOWPLAN"))) {
              rt1 = ServerConstants.SQL_SELECT_NON_UNIQUE;
//            rt1 = ServerConstants.TYPE_SELECT;
       } else if (str3.equals("UPDATE")) {
              rt1 = ServerConstants.SQL_UPDATE_NON_UNIQUE;
//            rt1 = ServerConstants.TYPE_UPDATE;
        } else if (str3.equals("DELETE")) {
              rt1 = ServerConstants.SQL_DELETE_NON_UNIQUE;
//            rt1 = ServerConstants.TYPE_DELETE;
        } else if (str3.equals("INSERT") || (str.equals("UPSERT"))) {
            if (str.indexOf('?') == -1) {
              rt1 = ServerConstants.SQL_INSERT_NON_UNIQUE;
//            rt1 = ServerConstants.TYPE_INSERT;
            } else {
                rt1 = ServerConstants.TYPE_INSERT_PARAM;
            }
        } else if (str3.equals("EXPLAIN")) {
            rt1 = ServerConstants.TYPE_EXPLAIN;
        } else if (str3.equals("CREATE")) {
            rt1 = ServerConstants.TYPE_CREATE;
        } else if (str3.equals("GRANT")) {
            rt1 = ServerConstants.TYPE_GRANT;
        } else if (str3.equals("DROP")) {
            rt1 = ServerConstants.TYPE_DROP;
        } else if (str3.equals("CALL")) {
            rt1 = ServerConstants.TYPE_CALL;
        } else if (str3.equals("EXPLAIN")) {
            rt1 = ServerConstants.TYPE_EXPLAIN;
        } else if (str3.equals("CONTROL")) {
            rt1 = ServerConstants.TYPE_CONTROL;
        } else {
            rt1 = ServerConstants.TYPE_UNKNOWN;
        }
        return rt1;
    }
    static public String getSqlCharsetName(int code) {

        String str = ServerConstants.sqlCharsetSTRING_UNKNOWN;
        switch (code){
        case ServerConstants.sqlCharsetCODE_ISO88591:
            return ServerConstants.sqlCharsetSTRING_ISO88591;
        case ServerConstants.sqlCharsetCODE_KANJI:
            return ServerConstants.sqlCharsetSTRING_KANJI;
        case ServerConstants.sqlCharsetCODE_KSC5601:
            return ServerConstants.sqlCharsetSTRING_KSC5601;
        case ServerConstants.sqlCharsetCODE_SJIS:
            return ServerConstants.sqlCharsetSTRING_SJIS;
        case ServerConstants.sqlCharsetCODE_UCS2:
            return ServerConstants.sqlCharsetSTRING_UNICODE;
        default:
        }
        return str;
    }
    static public String getSqlError(int retcode)
    {
        String rc;

        // SQL_NO_DATA_FOUND can be defined as SQL_NO_DATA
        if (retcode==ServerConstants.SQL_NO_DATA_FOUND)
        {
            if (ServerConstants.SQL_NO_DATA_FOUND==ServerConstants.SQL_NO_DATA) return "SQL_NO_DATA_FOUND|SQL_NO_DATA" ;
            return("SQL_NO_DATA_FOUND");
        }
        switch (retcode)
        {
        case ServerConstants.SQL_SUCCESS:
            return("SQL_SUCCESS");
        case ServerConstants.SQL_SUCCESS_WITH_INFO:
            return("SQL_SUCCESS_WITH_INFO");
        case ServerConstants.SQL_NO_DATA:
            return("SQL_NO_DATA");
        case ServerConstants.SQL_ERROR:
            return("SQL_ERROR");
/*
        case ServerConstants.SQL_INVALID_HANDLE:
            return("SQL_INVALID_HANDLE");
        case ServerConstants.SQL_STILL_EXECUTING:
            return("SQL_STILL_EXECUTING");
        case ServerConstants.SQL_NEED_DATA:
            return("SQL_NEED_DATA");
        case ServerConstants.STMT_ID_MISMATCH_ERROR:
            return("STMT_ID_MISMATCH_ERROR");
        case ServerConstants.DIALOGUE_ID_NULL_ERROR:
            return("DIALOGUE_ID_NULL_ERROR");
        case ServerConstants.STMT_ID_NULL_ERROR:
            return("STMT_ID_NULL_ERROR");
        case ServerConstants.NOWAIT_PENDING:
            return("NOWAIT_PENDING");
        case ServerConstants.STMT_ALREADY_EXISTS:
            return("STMT_ALREADY_EXISTS");
        case ServerConstants.STMT_DOES_NOT_EXIST:
            return("STMT_DOES_NOT_EXIST");
        case ServerConstants.STMT_IS_NOT_CALL:
            return("STMT_IS_NOT_CALL");
        case ServerConstants.RS_INDEX_OUT_OF_RANGE:
            return("RS_INDEX_OUT_OF_RANGE");
        case ServerConstants.RS_ALREADY_EXISTS:
            return("RS_ALREADY_EXISTS");
        case ServerConstants.RS_ALLOC_ERROR:
            return("RS_ALLOC_ERROR");
        case ServerConstants.RS_DOES_NOT_EXIST:
            return("RS_DOES_NOT_EXIST");
        case ServerConstants.PROGRAM_ERROR:
            return("PROGRAM_ERROR");
        case ServerConstants.ODBC_SERVER_ERROR:
            return("ODBC_SERVER_ERROR");
        case ServerConstants.ODBC_RG_ERROR:
            return("ODBC_RG_ERROR");
        case ServerConstants.ODBC_RG_WARNING:
            return("ODBC_RG_WARNING");
        case ServerConstants.SQL_RETRY_COMPILE_AGAIN:
            return("SQL_RETRY_COMPILE_AGAIN");
        case SQL_QUERY_CANCELLED:
            return("SQL_QUERY_CANCELLED");
        case ServerConstants.CANCEL_NOT_POSSIBLE:
            return("CANCEL_NOT_POSSIBLE");
        case ServerConstants.NOWAIT_ERROR:
            return("NOWAIT_ERROR");
*/
        }
        rc = "Unknown SQL Error (" + retcode + ")";
        return rc;
    }
    static public String getSqlStatementType(short stmtType)
    {
        String rc;

        if (stmtType == ServerConstants.TYPE_UNKNOWN) return "TYPE_UNKNOWN";
        rc = "";
        if ((stmtType & ServerConstants.TYPE_SELECT) != 0) rc = rc + "|TYPE_SELECT";
        if ((stmtType & ServerConstants.TYPE_UPDATE) != 0) rc = rc + "|TYPE_UPDATE";
        if ((stmtType & ServerConstants.TYPE_DELETE) != 0) rc = rc + "|TYPE_DELETE";
        if ((stmtType & ServerConstants.TYPE_INSERT) != 0) rc = rc + "|TYPE_INSERT";
        if ((stmtType & ServerConstants.TYPE_EXPLAIN) != 0) rc = rc + "|TYPE_EXPLAIN";
        if ((stmtType & ServerConstants.TYPE_CREATE) != 0) rc = rc + "|TYPE_CREATE";
        if ((stmtType & ServerConstants.TYPE_GRANT) != 0) rc = rc + "|TYPE_GRANT";
        if ((stmtType & ServerConstants.TYPE_DROP) != 0) rc = rc + "|TYPE_DROP";
        if ((stmtType & ServerConstants.TYPE_CALL) != 0) rc = rc + "|TYPE_CALL";
        if (rc.length()==0) rc = "UNKNOWN(" + stmtType +")";
        return rc;
    }
    static public String getSqlQueryStatementType(int stmtType)
    {
        switch (stmtType)
        {
        case ServerConstants.INVALID_SQL_QUERY_STMT_TYPE:
            return("INVALID_SQL_QUERY_STMT_TYPE");
        case ServerConstants.SQL_OTHER:
            return("SQL_OTHER");
        case ServerConstants.SQL_UNKNOWN:
            return("SQL_UNKNOWN");
        case ServerConstants.SQL_SELECT_UNIQUE:
            return("SQL_SELECT_UNIQUE");
        case ServerConstants.SQL_SELECT_NON_UNIQUE:
            return("SQL_SELECT_NON_UNIQUE");
        case ServerConstants.SQL_INSERT_UNIQUE:
            return("SQL_INSERT_UNIQUE");
        case ServerConstants.SQL_INSERT_NON_UNIQUE:
            return("SQL_INSERT_NON_UNIQUE");
        case ServerConstants.SQL_UPDATE_UNIQUE:
            return("SQL_UPDATE_UNIQUE");
        case ServerConstants.SQL_UPDATE_NON_UNIQUE:
            return("SQL_UPDATE_NON_UNIQUE");
        case ServerConstants.SQL_DELETE_UNIQUE:
            return("SQL_DELETE_UNIQUE");
        case ServerConstants.SQL_DELETE_NON_UNIQUE:
            return("SQL_DELETE_NON_UNIQUE");
        case ServerConstants.SQL_CONTROL:
            return("SQL_CONTROL");
        case ServerConstants.SQL_SET_TRANSACTION:
            return("SQL_SET_TRANSACTION");
        case ServerConstants.SQL_SET_CATALOG:
            return("SQL_SET_CATALOG");
        case ServerConstants.SQL_SET_SCHEMA:
            return("SQL_SET_SCHEMA");
        case ServerConstants.SQL_CALL_NO_RESULT_SETS:
            return("SQL_CALL_NO_RESULT_SETS");
        case ServerConstants.SQL_CALL_WITH_RESULT_SETS:
            return("SQL_CALL_WITH_RESULT_SETS");
        case ServerConstants.SQL_SP_RESULT_SET:
            return("SQL_SP_RESULT_SET");
        }
        return "Unknown (" + stmtType + ")";
    };

    static public String getSqlAttrType(int code)
    {
        String rc;
        switch (code)
        {
            case ServerConstants.SQL_ATTR_CURSOR_HOLDABLE:
                return("SQL_ATTR_CURSOR_HOLDABLE");
            case ServerConstants.SQL_ATTR_INPUT_ARRAY_MAXSIZE:
                return("SQL_ATTR_INPUT_ARRAY_MAXSIZE");
            case ServerConstants.SQL_ATTR_QUERY_TYPE:
                return("SQL_ATTR_QUERY_TYPE");
            case ServerConstants.SQL_ATTR_MAX_RESULT_SETS:
                return("SQL_ATTR_MAX_RESULT_SETS");
        }
        rc = "Unknown (" + code + ")";
        return rc;
    }
    static public String getSqlDataType(int code)
    {
        String rc;
        switch (code)
        {
            case ServerConstants.SQLTYPECODE_CHAR:
                return("SQLTYPECODE_CHAR");
            case ServerConstants.SQLTYPECODE_NUMERIC:
                return("SQLTYPECODE_NUMERIC");
            case ServerConstants.SQLTYPECODE_NUMERIC_UNSIGNED:
                return("SQLTYPECODE_NUMERIC_UNSIGNED");
            case ServerConstants.SQLTYPECODE_DECIMAL:
                return("SQLTYPECODE_DECIMAL");
            case ServerConstants.SQLTYPECODE_DECIMAL_UNSIGNED:
                return("SQLTYPECODE_DECIMAL_UNSIGNED");
            case ServerConstants.SQLTYPECODE_DECIMAL_LARGE:
                return("SQLTYPECODE_DECIMAL_LARGE");
            case ServerConstants.SQLTYPECODE_DECIMAL_LARGE_UNSIGNED:
                return("SQLTYPECODE_DECIMAL_LARGE_UNSIGNED");
            case ServerConstants.SQLTYPECODE_INTEGER:
                return("SQLTYPECODE_INTEGER");
            case ServerConstants.SQLTYPECODE_INTEGER_UNSIGNED:
                return("SQLTYPECODE_INTEGER_UNSIGNED");
            case ServerConstants.SQLTYPECODE_LARGEINT:
                return("SQLTYPECODE_LARGEINT");
            case ServerConstants.SQLTYPECODE_SMALLINT:
                return("SQLTYPECODE_SMALLINT");
            case ServerConstants.SQLTYPECODE_SMALLINT_UNSIGNED:
                return("SQLTYPECODE_SMALLINT_UNSIGNED");
            case ServerConstants.SQLTYPECODE_BPINT_UNSIGNED:
                return("SQLTYPECODE_BPINT_UNSIGNED");
            case ServerConstants.SQLTYPECODE_TDM_FLOAT:
                return("SQLTYPECODE_TDM_FLOAT");
            case ServerConstants.SQLTYPECODE_IEEE_FLOAT:
                return("SQLTYPECODE_IEEE_FLOAT");
            case ServerConstants.SQLTYPECODE_TDM_REAL:
                return("SQLTYPECODE_TDM_REAL");
            case ServerConstants.SQLTYPECODE_IEEE_REAL:
                return("SQLTYPECODE_IEEE_REAL");
            case ServerConstants.SQLTYPECODE_TDM_DOUBLE:
                return("SQLTYPECODE_TDM_DOUBLE");
            case ServerConstants.SQLTYPECODE_IEEE_DOUBLE:
                return("SQLTYPECODE_IEEE_DOUBLE");
            case ServerConstants.SQLTYPECODE_DATETIME:
                return("SQLTYPECODE_DATETIME");
            case ServerConstants.SQLTYPECODE_INTERVAL:
                return("SQLTYPECODE_INTERVAL");
            case ServerConstants.SQLTYPECODE_VARCHAR:
                return("SQLTYPECODE_VARCHAR");
            case ServerConstants.SQLTYPECODE_VARCHAR_WITH_LENGTH:
                return("SQLTYPECODE_VARCHAR_WITH_LENGTH");
            case ServerConstants.SQLTYPECODE_VARCHAR_LONG:
                return("SQLTYPECODE_VARCHAR_LONG");
            case ServerConstants.SQLTYPECODE_BIT:
                return("SQLTYPECODE_BIT");
            case ServerConstants.SQLTYPECODE_BITVAR:
                return("SQLTYPECODE_BITVAR");
        };
        rc = "UNKNOWN (" + code + ")";
        return rc;
    }
    static public String getDataType(int dataType)
    {
        switch (dataType)
        {
            case Types.SMALLINT:
                return "SMALLINT";
            case Types.INTEGER:
                return "INTEGER";
            case Types.BIGINT:
                return "BIGINT";
            case Types.REAL:
                return "REAL";
            case Types.FLOAT:
                return "FLOAT";
            case Types.DOUBLE:
                return "DOUBLE PRECISION";
            case Types.NUMERIC:
                return "NUMERIC";
            case Types.DECIMAL:
                return "DECIMAL";
            case Types.CHAR:
                return "CHAR";
            case Types.VARCHAR:
                return "VARCHAR";
            case Types.LONGVARCHAR:
                return "LONG VARCHAR";
            case Types.DATE:
                return "DATE";
            case Types.TIME:
                return "TIME";
            case Types.TIMESTAMP:
                return "TIMESTAMP";
            case Types.BLOB:
                return "BLOB";
            case Types.CLOB:
                return "CLOB";
            case Types.OTHER:
                return "OTHER";
            case Types.BIT:
                return "BIT";
            case Types.TINYINT:
                return "TINYINT";
            default:
                return "UNKNOWN (" + dataType + ")";
        }
    }
    static public BigDecimal getBigDecimalValue(Object paramValue) throws SQLException {
        BigDecimal tmpbd;

        if (paramValue instanceof Long) {
            tmpbd = BigDecimal.valueOf(((Long) paramValue).longValue());
        } else if (paramValue instanceof Integer) {
            tmpbd = BigDecimal.valueOf(((Integer) paramValue).longValue());
        } else if (paramValue instanceof BigDecimal) {
            tmpbd = (BigDecimal) paramValue;
        } else if (paramValue instanceof String) {
            String sVal = (String) paramValue;
            if (sVal.equals("true") == true) {
                sVal = "1";
            } else if (sVal.equals("false") == true) {
                sVal = "0";
            }
            tmpbd = new BigDecimal(sVal);
        } else if (paramValue instanceof Float) {
            tmpbd = new BigDecimal(paramValue.toString());
        } else if (paramValue instanceof Double) {
            tmpbd = new BigDecimal(((Double) paramValue).toString());
        } else if (paramValue instanceof Boolean) {
            tmpbd = BigDecimal.valueOf(((((Boolean) paramValue).booleanValue() == true) ? 1 : 0));
        } else if (paramValue instanceof Byte) {
            tmpbd = BigDecimal.valueOf(((Byte) paramValue).longValue());
        } else if (paramValue instanceof Short) {
            tmpbd = BigDecimal.valueOf(((Short) paramValue).longValue());
        } else if (paramValue instanceof Integer) {
            tmpbd = BigDecimal.valueOf(((Integer) paramValue).longValue());
            // For LOB Support SB: 10/25/2004
            /*
             * else if (paramValue instanceof DataWrapper) tmpbd =
             * BigDecimal.valueOf(((DataWrapper)paramValue).longValue);
             */
        } else {
            throw new SQLException("object_type_not_supported");
        }
        return tmpbd;
    } // end getBigDecimalValue

    static public BigDecimal convertSQLBigNumToBigDecimal(ByteBuffer sourceData, int len, int scale, boolean isUnSigned) {
        String strVal = ""; // our final String
        boolean negative = false;

        // we need the data in an array which can hold UNSIGNED 16 bit values
        // in java we dont have unsigned datatypes so 32-bit signed is the best
        // we can do
        int[] dataInShorts = new int[len / 2];
        for (int i = 0; i < dataInShorts.length; i++){
            dataInShorts[i] = ByteBufferUtils.extractUShort(sourceData); // copy
        // the
        // data
            LOG.debug("dataInShorts[" + i + "] :" + dataInShorts[i]);
        }
        if (isUnSigned == false){
            if ((dataInShorts[dataInShorts.length - 1] & 0xFF00) > 0){
                negative = ((dataInShorts[dataInShorts.length - 1] & 0x8000) > 0);
                dataInShorts[dataInShorts.length - 1] &= 0x7FFF; // force sign to 0, continue
            }
            else {
                negative = ((dataInShorts[dataInShorts.length - 1] & 0x0080) > 0);
                dataInShorts[dataInShorts.length - 1] &= 0xFF7F; // force sign to 0, continue
            }
            // normally
        }

        int curPos = dataInShorts.length - 1; // start at the end
        while (curPos >= 0 && dataInShorts[curPos] == 0)
            // get rid of any trailing 0's
            curPos--;

        int remainder = 0;
        long temp; // we need to use a LONG since we will have to hold up to
        // 32-bit UNSIGNED values

        // we now have the huge value stored in 2 bytes chunks
        // we will divide by 10000 many times, converting the remainder to
        // String
        // when we are left with a single chunk <10000 we will handle it using a
        // special case
        while (curPos >= 0 || dataInShorts[0] >= 10000) {
            // start on the right, divide the 16 bit value by 10000
            // use the remainder as the upper 16 bits for the next division
            for (int j = curPos; j >= 0; j--) {
                // these operations got messy when java tried to infer what size
                // to store the value in
                // leave these as separate operations for now...always casting
                // back to a 64 bit value to avoid sign problems
                temp = remainder;
                temp &= 0xFFFF;
                temp = temp << 16;
                temp += dataInShorts[j];

                dataInShorts[j] = (int) (temp / 10000);
                remainder = (int) (temp % 10000);
            }

            // if we are done with the current 16bits, move on
            if (dataInShorts[curPos] == 0)
                curPos--;

            // go through the remainder and add each digit to the final String
            for (int j = 0; j < 4; j++) {
                strVal = (remainder % 10) + strVal;
                remainder /= 10;
            }
        }

        // when we finish the above loop we still have 1 <10000 value to include
        remainder = dataInShorts[0];
        for (int j = 0; j < 4; j++) {
            strVal = (remainder % 10) + strVal;
            remainder /= 10;
        }

        BigInteger bi = new BigInteger(strVal); // create a java BigInt
        if (negative && isUnSigned == false)
            bi = bi.negate();

        return new BigDecimal(bi, scale); // create a new BigDecimal with the
        // descriptor's scale
    }
    static public byte[] formatSqlT4Output(Descriptor2 dsc, byte[] sqlarray, long curOutPos, byte[] outValues, ByteOrder bo) throws UnsupportedEncodingException{

        ByteBuffer bb = null;
        byte[] dst = null;
        int len = 0;
        int offset = 0;
        int insNull = 0;

        String[] stDate = null;
        String[] stTime = null;
        String[] stTimestamp = null;
        String[] stNanos = null;
        Integer year = 0;
        Integer month = 0;
        Integer day = 0;
        Integer hour = 0;
        Integer minutes = 0;
        Integer seconds = 0;
        Integer nanos = 0;

        String setString = "";
        short setShort = 0;
        int setInt = 0;
        long setLong = 0L;
        boolean setSign = false;
        String charSet = "";

        int precision = dsc.getPrecision();
        int scale = dsc.getScale();
        int datetimeCode = dsc.getDatetimeCode();
        int FSDataType = dsc.getFsDataType();
        int OdbcDataType = dsc.getOdbcDataType();
        int dataCharSet = dsc.getSqlCharset();
        int length = dsc.getMaxLen();
        int dataType = dsc.getDataType();

        if(dataCharSet == SqlUtils.SQLCHARSETCODE_UNICODE)
            charSet = "UTF-16LE";
        else
            charSet = SqlUtils.getCharsetName(dataCharSet);

        len = sqlarray.length;
        ByteBuffer tb = ByteBuffer.wrap(sqlarray).order(bo);

        if(LOG.isDebugEnabled())
            LOG.debug("formatSqlT4Output: -----------");

        switch(dataType){
        case ServerConstants.SQLTYPECODE_CHAR:
            if(LOG.isDebugEnabled())
                LOG.debug("formatSqlT4Output: SQLTYPECODE_CHAR :" + Arrays.toString(sqlarray));
            break;
        case ServerConstants.SQLTYPECODE_VARCHAR:
        case ServerConstants.SQLTYPECODE_VARCHAR_LONG:
            if(LOG.isDebugEnabled())
                LOG.debug("formatSqlT4Output: SQLTYPECODE_VARCHAR/VARCHAR_LONG :" + Arrays.toString(sqlarray));
            break;
        case ServerConstants.SQLTYPECODE_VARCHAR_WITH_LENGTH:       //-601
            if( length > Short.MAX_VALUE ){
                len = tb.getInt() + 4;
            } else {
                len = tb.getShort() + 2;
            }
            if(LOG.isDebugEnabled())
                LOG.debug("formatSqlT4Output: SQLTYPECODE_VARCHAR :" + Arrays.toString(sqlarray));
            break;
        case ServerConstants.SQLTYPECODE_DATETIME:
            LOG.debug("formatSqlT4Output: SQLTYPECODE_DATETIME :" + Arrays.toString(sqlarray));
            switch(datetimeCode){
                case ServerConstants.SQLDTCODE_DATE:
                case ServerConstants.SQLDTCODE_TIME:
                case ServerConstants.SQLDTCODE_TIMESTAMP:
                    len = tb.getShort();
                    dst = new byte[len];
                    System.arraycopy(sqlarray, 2, dst, 0, len);
                    setString = new String(dst,"UTF8");
                    tb.clear();
                    switch(datetimeCode){
                        case ServerConstants.SQLDTCODE_DATE:
                            stDate = setString.split("-");
                            year = Integer.valueOf(stDate[0]);
                            month = Integer.valueOf(stDate[1]);
                            day = Integer.valueOf(stDate[2]);
                            tb.putShort(year.shortValue());
                            tb.put(month.byteValue());
                            tb.put(day.byteValue());
                            break;
                        case ServerConstants.SQLDTCODE_TIME:
                            stTime = setString.split(":");
                            hour = Integer.valueOf(stTime[0]);
                            minutes = Integer.valueOf(stTime[1]);
                            seconds = Integer.valueOf(stTime[2]);
                            tb.put(hour.byteValue());
                            tb.put(minutes.byteValue());
                            tb.put(seconds.byteValue());
                            break;
                        case ServerConstants.SQLDTCODE_TIMESTAMP:
                            stTimestamp = setString.split(" ");
                            stDate = stTimestamp[0].split("-");
                            stNanos = stTimestamp[1].split("\\.");
                            stTime = stNanos[0].split(":");
                            year = Integer.valueOf(stDate[0]);
                            month = Integer.valueOf(stDate[1]);
                            day = Integer.valueOf(stDate[2]);
                            hour = Integer.valueOf(stTime[0]);
                            minutes = Integer.valueOf(stTime[1]);
                            seconds = Integer.valueOf(stTime[2]);
                            nanos = Integer.valueOf(stNanos[1]);
                            tb.putShort(year.shortValue());
                            tb.put(month.byteValue());
                            tb.put(day.byteValue());
                            tb.put(hour.byteValue());
                            tb.put(minutes.byteValue());
                            tb.put(seconds.byteValue());
                            tb.putInt(nanos);
                            break;
                        case ServerConstants.SQLDTCODE_MPDATETIME:
                            break;
                    }
                default:
            }
            break;
        case ServerConstants.SQLTYPECODE_INTERVAL:
            if(LOG.isDebugEnabled())
                LOG.debug("formatSqlT4Output: SQLTYPECODE_INTERVAL :" + Arrays.toString(sqlarray));
            len = tb.getShort();
            dst = new byte[len];
            System.arraycopy(sqlarray, 2, dst, 0, len);
            Arrays.fill(sqlarray, (byte)0);
            tb.clear();
            if(dst[0] != '-'){
                len++;
                tb.put((byte)' ');
            }
            tb.put(dst, 0, dst.length);
            if(LOG.isDebugEnabled())
                LOG.debug("formatSqlT4Output: sqlarray :" + Arrays.toString(sqlarray));
            break;
        case ServerConstants.SQLTYPECODE_INTEGER:                    //4
            if(LOG.isDebugEnabled())
                LOG.debug("formatSqlT4Output: SQLTYPECODE_INTEGER :" + Arrays.toString(sqlarray));
            break;
        case ServerConstants.SQLTYPECODE_INTEGER_UNSIGNED:
            if(LOG.isDebugEnabled())
                LOG.debug("formatSqlT4Output: SQLTYPECODE_INTEGER_UNSIGNED :" + Arrays.toString(sqlarray));
            break;
        case ServerConstants.SQLTYPECODE_SMALLINT:
            if(LOG.isDebugEnabled())
                LOG.debug("formatSqlT4Output: SQLTYPECODE_SMALLINT :" + Arrays.toString(sqlarray));
            break;
        case ServerConstants.SQLTYPECODE_SMALLINT_UNSIGNED:
            if(LOG.isDebugEnabled())
                LOG.debug("formatSqlT4Output: SQLTYPECODE_SMALLINT_UNSIGNED :" + Arrays.toString(sqlarray));
            break;
        case ServerConstants.SQLTYPECODE_LARGEINT:
            if(LOG.isDebugEnabled())
                LOG.debug("formatSqlT4Output: SQLTYPECODE_LARGEINT :" + Arrays.toString(sqlarray));
            break;
        case ServerConstants.SQLTYPECODE_DECIMAL:
        case ServerConstants.SQLTYPECODE_DECIMAL_UNSIGNED:
            if(LOG.isDebugEnabled())
                LOG.debug("formatSqlT4Output: SQLTYPECODE_DECIMAL :" + Arrays.toString(sqlarray));
            break;
        case ServerConstants.SQLTYPECODE_IEEE_REAL:                    //6
            if(LOG.isDebugEnabled())
                LOG.debug("formatSqlT4Output: SQLTYPECODE_IEEE_REAL :" + Arrays.toString(sqlarray));
            break;
        case ServerConstants.SQLTYPECODE_IEEE_FLOAT:                   //7
            if(LOG.isDebugEnabled())
                LOG.debug("formatSqlT4Output: SQLTYPECODE_IEEE_FLOAT :" + Arrays.toString(sqlarray));
            break;
        case ServerConstants.SQLTYPECODE_IEEE_DOUBLE:                  //8
            if(LOG.isDebugEnabled())
                LOG.debug("formatSqlT4Output: SQLTYPECODE_IEEE_DOUBLE :" + Arrays.toString(sqlarray));
            break;
        case ServerConstants.SQLTYPECODE_NUMERIC:
        case ServerConstants.SQLTYPECODE_NUMERIC_UNSIGNED:
            if(LOG.isDebugEnabled())
                LOG.debug("formatSqlT4Output: SQLTYPECODE_NUMERIC :" + Arrays.toString(sqlarray));
            switch (len) {
            case 2:
                setShort = tb.getShort();
                setLong = setShort;
                if(setLong < 0){
                    setLong = -1L * setLong;
                    setSign = true;
                    tb.clear();
                    tb.put((byte) ((setLong) & 0xff));
                    tb.put((byte) (((setLong >>> 8) | 0x80) & 0xff));
                }
                break;
            case 4:
                setInt = tb.getInt();
                setLong = setInt;
                if(setLong < 0){
                    setLong = -1L * setLong;
                    setSign = true;
                    tb.clear();
                    tb.put((byte) ((setLong) & 0xff));
                    tb.put((byte) ((setLong >>> 8) & 0xff));
                    tb.put((byte) ((setLong >>> 16) & 0xff));
                    tb.put((byte) (((setLong >>> 24) | 0x80) & 0xff));
                }
                break;
            case 8:
                setLong = tb.getLong();
                if(setLong < 0){
                    setLong = -1L * setLong;
                    setSign = true;
                    tb.clear();
                    tb.put((byte) ((setLong) & 0xff));
                    tb.put((byte) ((setLong >>> 8) & 0xff));
                    tb.put((byte) ((setLong >>> 16) & 0xff));
                    tb.put((byte) ((setLong >>> 24) & 0xff));
                    tb.put((byte) ((setLong >>> 32) & 0xff));
                    tb.put((byte) ((setLong >>> 40) & 0xff));
                    tb.put((byte) ((setLong >>> 48) & 0xff));
                    tb.put((byte) (((setLong >>> 56) | 0x80) & 0xff));
                }
                break;
            }
            break;
        case ServerConstants.SQLTYPECODE_DECIMAL_LARGE:
        case ServerConstants.SQLTYPECODE_DECIMAL_LARGE_UNSIGNED:
        case ServerConstants.SQLTYPECODE_BIT:
        case ServerConstants.SQLTYPECODE_BITVAR:
        case ServerConstants.SQLTYPECODE_BPINT_UNSIGNED:
        default:
            if(LOG.isDebugEnabled())
                LOG.debug("formatSqlT4Output: default :" + Arrays.toString(sqlarray));
            break;
        }
        if(LOG.isDebugEnabled()){
            LOG.debug("formatSqlT4Output: offset :" + offset);
            LOG.debug("formatSqlT4Output: curOutPos :" + curOutPos);
            LOG.debug("formatSqlT4Output: len :" + len);
        }
        System.arraycopy(sqlarray, offset, outValues, (int)curOutPos, len);
        return outValues;
    }
}
