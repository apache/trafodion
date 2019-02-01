/**********************************************************************
Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"); you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.
**********************************************************************/

/***************************************************
 * A TMUDF that executes a generic JDBC query
 * and returns the result of the one SQL statement
 * in the list that produces results as a table-valued
 * output
 *
 * Invocation (all arguments are strings):
 *
 * select ... from udf(JDBC(
 *    <name of JDBC driver jar>, // file name of the JDBC driver jar, stored
 *                               // in $TRAF_VAR/udr/public/external_libs
 *    <name of JDBC driver class in the jar>,
 *    <connection string>,
 *    <user name>,
 *    <password>,
 *    <statement_type>,
 *    <sql statement 1>
 *    [ , <sql statements 2 ...n> ] )) ...
 *
 * The first 7 arguments are required and must be
 * string literals that are available at compile
 * time.
 * Statement type:
 *    'source': This statement produces a result
 *              (only type allowed at this time)
 *              (may support "target" to insert
 *               into a table via JDBC later)
 *
 * Note that only one of the SQL statements can be
 * a select or other result-producing statements.
 * The others can perform setup and cleanup
 * operations, if necessary (e.g. create table,
 * insert, select, drop table).
 *
 * For an example, see file
 * core/sql/regress/udr/TEST002.
 ***************************************************/

package org.trafodion.libmgmt;

import org.trafodion.sql.udr.*;
import java.sql.*;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Vector;
import java.lang.Math;
import java.util.Properties;
import java.util.logging.Logger;

class JDBCUDR extends UDR
{
    // class used to connect, both at compile and at runtime
    static class JdbcConnectionInfo
    {
        String driverJar_;
        String driverClassName_;
        String connectionString_;
        String username_;
        String password_;
        boolean debug_;

        Connection conn_;

        public void setJar(String jar)
                                                     { driverJar_ = jar; }
        public void setClass(String className)
                                         { driverClassName_ = className; }
        public void setConnString(String connString)
                                       { connectionString_ = connString; }
        public void setUsername(String userName)
                                                 { username_ = userName; }
        public void setPassword(String password)
                                                 { password_ = password; }
        public void setDebug(boolean debug)            { debug_ = debug; }

        public Connection connect() throws UDRException
        {
          try {
            Path driverJarPath = Paths.get(driverJar_);

            // for security reasons, we sandbox the allowed driver jars
            // into $TRAF_VAR/udr/public/external_libs
            driverJarPath = driverJarPath.normalize();
            if (driverJarPath.isAbsolute())
              {
                if (! driverJarPath.startsWith(
                                  LmUtility.getSandboxRootForUser(null)))
                  throw new UDRException(
                    38010,
                    "The jar name of the JDBC driver must be a name relative to %s, got %s",
                    LmUtility.getSandboxRootForUser(null).toString(),
                    driverJar_);
              }
            else
              driverJarPath = LmUtility.getExternalLibsDirForUser(null).resolve(
                    driverJarPath);

            // for security reasons we also reject the Trafodion T2
            // driver (check both class name and URL)
            if (driverClassName_.equals("org.apache.trafodion.jdbc.t2.T2Driver"))
                throw new UDRException(
                    38012,
                    "This UDF does not support the Trafodion T2 driver class %s",
                    driverClassName_);

            if (LmT2Driver.checkURL(connectionString_))
                throw new UDRException(
                    38013,
                    "This UDF does not support the Trafodion T2 driver URL %s",
                    connectionString_);
 
            // Create a class loader that can access the jar file
            // specified by the caller. Note that this is only needed
            // because the JDBC UDR is a predefined UDR and is loaded
            // by the standard class loader. If it were a regular UDR,
            // it would have been loaded by LmClassLoader and we would
            // not need to create an LmClassLoader here.
            LmClassLoader jdbcJarLoader = LmUtility.createClassLoader(
                                               driverJarPath.toString(),0);

            Driver d = (Driver) Class.forName(driverClassName_,
                                              true,
                                              jdbcJarLoader).newInstance();

            // go through an intermediary driver, since the DriverManager
            // will not accept classes that are not loaded by the default
            // class loader
            DriverManager.registerDriver(new URLDriver(d));
            conn_ = DriverManager.getConnection(connectionString_,
                                                username_,
                                                password_);
            return conn_;
          }
          catch (ClassNotFoundException cnf) {
              throw new UDRException(
                38020,
                "JDBC driver class %s not found. Please make sure the JDBC driver jar %s is stored in %s. Message: %s",
                driverClassName_,
                driverJar_,
                LmUtility.getSandboxRootForUser(null).toString(),
                cnf.getMessage());
          }
          catch (SQLException se) {
              throw new UDRException(
                38020,
                "SQL exception during connect. Message: %s",
                se.getMessage());
          }
          catch (Exception e) {
              if (debug_)
                  {
                      System.out.println("Debug: Exception during connect:");
                      try { e.printStackTrace(System.out); }
                      catch (Exception e2) {}
                  }
              throw new UDRException(
                38020,
                "Exception during connect: %s",
                e.getMessage());
          }
        }

        public Connection getConnection()                 { return conn_; }

        public void disconnect() throws SQLException
        {
            conn_.close();
            conn_ = null;
        }
    };

    // list of SQL statements to execute
    static class SQLStatementInfo
    {
        // list of SQL statements to execute
        Vector<String> sqlStrings_;

        // which of the above is the one that
        // produces the table-valued result?
        int resultStatementIndex_;

        // prepared result-producing statement
        PreparedStatement resultStatement_;

        SQLStatementInfo()
        {
            sqlStrings_ = new Vector<String>();
            resultStatementIndex_ = -1;
        }

        void addStatementText(String sqlText)
        {
            sqlStrings_.add(sqlText);
        }

        void addResultProducingStatement(PreparedStatement preparedStmt,
                                         int resultStatementIndex)
        {
            resultStatement_ = preparedStmt;
            resultStatementIndex_ = resultStatementIndex;
        }

        String getStatementText(int ix)    { return sqlStrings_.get(ix); }
        PreparedStatement getResultStatement(){ return resultStatement_; }
        int getNumStatements()              { return sqlStrings_.size(); }
        int getResultStatementIndex()    { return resultStatementIndex_; }
    };

    // Define data that gets passed between compiler phases
    static class JdbcCompileTimeData extends UDRWriterCompileTimeData
    {
        JdbcConnectionInfo jci_;
        SQLStatementInfo sqi_;

        JdbcCompileTimeData()
        {
            jci_ = new JdbcConnectionInfo();
            sqi_ = new SQLStatementInfo();
        }
    };

    static class URLDriver implements Driver {
	private Driver driver_;
	URLDriver(Driver d) { driver_ = d; }
	public boolean acceptsURL(String u) throws SQLException {
            return driver_.acceptsURL(u);
	}
	public Connection connect(String u, Properties p) throws SQLException {
            return driver_.connect(u, p);
	}
	public int getMajorVersion() {
            return driver_.getMajorVersion();
	}
	public int getMinorVersion() {
            return driver_.getMinorVersion();
	}
	public DriverPropertyInfo[] getPropertyInfo(String u, Properties p) throws SQLException {
            return driver_.getPropertyInfo(u, p);
	}
	public boolean jdbcCompliant() {
            return driver_.jdbcCompliant();
	}
        public Logger getParentLogger() throws SQLFeatureNotSupportedException {
            return driver_.getParentLogger();
        }
    }

    JdbcConnectionInfo getConnectionInfo(UDRInvocationInfo info) throws UDRException
    {
        return ((JdbcCompileTimeData) info.getUDRWriterCompileTimeData()).jci_;
    }

    SQLStatementInfo getSQLStatementInfo(UDRInvocationInfo info) throws UDRException
    {
        return ((JdbcCompileTimeData) info.getUDRWriterCompileTimeData()).sqi_;
    }


    // default constructor
    public JDBCUDR()
    {}

    // a method to process the input parameters, this is
    // used both at compile time and at runtime
    private void handleInputParams(UDRInvocationInfo info,
                                   JdbcConnectionInfo jci,
                                   SQLStatementInfo sqi,
                                   boolean isCompileTime)
                                             throws UDRException
    {
        int numInParams = info.par().getNumColumns();

        // Right now we don't support table inputs
        if (isCompileTime && info.getNumTableInputs() != 0)
            throw new UDRException(
              38300,
              "%s must be called with no table-valued inputs",
              info.getUDRName());

        if (numInParams < 7)
            throw new UDRException(
              38310,
              "Expecting at least 7 parameters for %s UDR",
              info.getUDRName());

        // loop over scalar input parameters
        for (int p=0; p<numInParams; p++)
            {
                if (isCompileTime &&
                    ! info.par().isAvailable(p))
                    throw new UDRException(
                      38320,
                      "Parameter %d of %s must be a compile time constant",
                      p+1,
                      info.getUDRName());

                String paramValue = info.par().getString(p);

                switch (p)
                    {
                    case 0:
                        jci.setJar(paramValue);
                        break;

                    case 1:
                        jci.setClass(paramValue);
                        break;

                    case 2:
                        jci.setConnString(paramValue);
                        break;

                    case 3:
                        jci.setUsername(paramValue);
                        break;

                    case 4:
                        jci.setPassword(paramValue);
                        break;

                    case 5:
                        // Only statement type supported
                        // so far is select, we may support insert later
                        if (paramValue.compareToIgnoreCase("source") != 0)
                            throw new UDRException(
                              38330,
                              "The only statement type supported so far is 'source' in parameter 6 of %s",
                              info.getUDRName());
                        break;

                    default:
                        // SQL statement (there could be multiple)
                        sqi.addStatementText(paramValue);
                        break;

                    }

                if (isCompileTime)
                    // add the actual parameter as a formal parameter
                    // (the formal parameter list is initially empty)
                    info.addFormalParameter(info.par().getColumn(p));
            }

        jci.setDebug(info.getDebugFlags() != 0);

        // Prepare each provided statement. We will verify that
        // only one of these statements produces result rows,
        // which will become our table-valued output.
        int numSQLStatements = sqi.getNumStatements();

        // sanity check
        if (numSQLStatements != numInParams-6)
            throw new UDRException(383400, "internal error");

        if (numSQLStatements < 1)
            throw new UDRException(383500, "At least one SQL statement must be given in parameters 6 and following");

        if (isCompileTime)
        {
            // walk through all statements, check whether they are
            // valid by preparing them, and determine which one is
            // the one that generates a result set
            String currentStmtText = "";
            try
            {
                jci.connect();

                for (int s=0; s<numSQLStatements; s++)
                {
                    currentStmtText = sqi.getStatementText(s);
                    // System.out.printf("Statement to prepare: %s\n", currentStmtText);
                    PreparedStatement preparedStmt =
                            jci.getConnection().prepareStatement(currentStmtText);
                    // if (preparedStmt != null)
                    //    System.out.printf("Prepare was successful\n");
                    ParameterMetaData pmd = preparedStmt.getParameterMetaData();
                    if (pmd != null && pmd.getParameterCount() != 0)
                        throw new UDRException(
                                38360,
                                "Statement %s requires %d input parameters, which is not supported",
                                currentStmtText, pmd.getParameterCount());
                    ResultSetMetaData desc = preparedStmt.getMetaData();

                    int numResultCols = desc.getColumnCount();
                    // System.out.printf("Number of output columns: %d", numResultCols);

                    if (numResultCols > 0)
                    {
                        if (sqi.getResultStatementIndex() >= 0)
                            throw new UDRException(
                                    38370,
                                    "More than one of the statements provided produce output, this is not supported (%d and %d)",
                                    sqi.getResultStatementIndex()+1,
                                    s+1);

                        // we found the statement that is producing the result
                        sqi.addResultProducingStatement(preparedStmt, s);

                        // now add the output columns
                        for (int c=0; c<numResultCols; c++)
                        {
                            String colName = desc.getColumnLabel(c+1);
                            TypeInfo udrType = getUDRTypeFromJDBCType(desc, c+1);
                            info.out().addColumn(new ColumnInfo(colName, udrType));
                        }
                    }
                }
                jci.disconnect();
            }
            catch (SQLException e)
            {
                throw new UDRException(
                        38380,
                        "SQL Exception when preparing SQL statement %s. Exception text: %s",
                        currentStmtText, e.getMessage());
            }
        }
    }

    TypeInfo getUDRTypeFromJDBCType(ResultSetMetaData desc,
                                    int colNumOneBased) throws UDRException
    {
        TypeInfo result;

        final int maxLength = 100000;

        int colJDBCType;

        // the ingredients to make a UDR type and their default values
        TypeInfo.SQLTypeCode      sqlType      = TypeInfo.SQLTypeCode.UNDEFINED_SQL_TYPE;
        int                       length       = 0;
        boolean                   nullable     = false;
        int                       scale        = 0;
        TypeInfo.SQLCharsetCode   charset      = TypeInfo.SQLCharsetCode.CHARSET_UCS2;
        TypeInfo.SQLIntervalCode  intervalCode = TypeInfo.SQLIntervalCode.UNDEFINED_INTERVAL_CODE;
        int                       precision    = 0;
        TypeInfo.SQLCollationCode collation    = TypeInfo.SQLCollationCode.SYSTEM_COLLATION;

        try {
            colJDBCType = desc.getColumnType(colNumOneBased);
            nullable = (desc.isNullable(colNumOneBased) != ResultSetMetaData.columnNoNulls);

            // map the JDBC type to a Trafodion UDR parameter type
            switch (colJDBCType)
            {
            case java.sql.Types.SMALLINT:
            case java.sql.Types.TINYINT:
            case java.sql.Types.BOOLEAN:
                if (desc.isSigned(colNumOneBased))
                    sqlType = TypeInfo.SQLTypeCode.SMALLINT;
                else
                    sqlType = TypeInfo.SQLTypeCode.SMALLINT_UNSIGNED;
                break;

            case java.sql.Types.INTEGER:
                if (desc.isSigned(colNumOneBased))
                    sqlType = TypeInfo.SQLTypeCode.INT;
                else
                    sqlType = TypeInfo.SQLTypeCode.INT_UNSIGNED;
                break;

            case java.sql.Types.BIGINT:
                sqlType = TypeInfo.SQLTypeCode.LARGEINT;
                break;

            case java.sql.Types.DECIMAL:
            case java.sql.Types.NUMERIC:
                if (desc.isSigned(colNumOneBased))
                    sqlType = TypeInfo.SQLTypeCode.NUMERIC;
                else
                    sqlType = TypeInfo.SQLTypeCode.NUMERIC_UNSIGNED;
                precision = desc.getPrecision(colNumOneBased);
                scale = desc.getScale(colNumOneBased);
                break;

            case java.sql.Types.REAL:
                sqlType = TypeInfo.SQLTypeCode.REAL;
                break;

            case java.sql.Types.DOUBLE:
            case java.sql.Types.FLOAT:
                sqlType = TypeInfo.SQLTypeCode.DOUBLE_PRECISION;
                break;

            case java.sql.Types.CHAR:
            case java.sql.Types.NCHAR:
                sqlType = TypeInfo.SQLTypeCode.CHAR;
                length  = Math.min(desc.getPrecision(colNumOneBased), maxLength);
                charset = TypeInfo.SQLCharsetCode.CHARSET_UCS2;
                break;

            case java.sql.Types.VARCHAR:
            case java.sql.Types.NVARCHAR:
                sqlType = TypeInfo.SQLTypeCode.VARCHAR;
                length  = Math.min(desc.getPrecision(colNumOneBased), maxLength);
                charset = TypeInfo.SQLCharsetCode.CHARSET_UCS2;
                break;

            case java.sql.Types.DATE:
                sqlType = TypeInfo.SQLTypeCode.DATE;
                break;

            case java.sql.Types.TIME:
                sqlType = TypeInfo.SQLTypeCode.TIME;
                break;

            case java.sql.Types.TIMESTAMP:
                sqlType = TypeInfo.SQLTypeCode.TIMESTAMP;
                scale   = 3;
                break;

                // BLOB - not supported yet, map to varchar
                // case java.sql.Types.BLOB:
                // sqlType = TypeInfo.SQLTypeCode.BLOB;
                // break;

                // CLOB - not supported yet, map to varchar
                // case java.sql.Types.CLOB:
                // sqlType = TypeInfo.SQLTypeCode.CLOB;
                // break;

            case java.sql.Types.ARRAY:
            case java.sql.Types.BINARY:
            case java.sql.Types.BIT:
            case java.sql.Types.BLOB:
            case java.sql.Types.DATALINK:
            case java.sql.Types.DISTINCT:
            case java.sql.Types.JAVA_OBJECT:
            case java.sql.Types.LONGVARBINARY:
            case java.sql.Types.NULL:
            case java.sql.Types.OTHER:
            case java.sql.Types.REF:
            case java.sql.Types.STRUCT:
            case java.sql.Types.VARBINARY:
                // these types produce a binary result, represented
                // as varchar(n) character set iso88591
                sqlType = TypeInfo.SQLTypeCode.VARCHAR;
                length  = Math.min(desc.getPrecision(colNumOneBased), maxLength);
                charset = TypeInfo.SQLCharsetCode.CHARSET_ISO88591;
                break;

            case java.sql.Types.LONGVARCHAR:
            case java.sql.Types.LONGNVARCHAR:
            case java.sql.Types.CLOB:
            case java.sql.Types.NCLOB:
            case java.sql.Types.ROWID:
            case java.sql.Types.SQLXML:
                // these types produce a varchar(n) character set utf8 result
                sqlType = TypeInfo.SQLTypeCode.VARCHAR;
                length  = Math.min(desc.getPrecision(colNumOneBased), maxLength);
                charset = TypeInfo.SQLCharsetCode.CHARSET_UCS2;
                break;
            }
        } catch (SQLException e) {
            throw new UDRException(
                    38500,
                    "Error determinging the type of output column %d: ",
                    colNumOneBased,
                    e.getMessage());
        }

        result = new TypeInfo(
                sqlType,
                length,
                nullable,
                scale,
                charset,
                intervalCode,
                precision,
                collation);

        return result;
    }

    // determine output columns dynamically at compile time
    @Override
    public void describeParamsAndColumns(UDRInvocationInfo info)
        throws UDRException
    {
        // create an object with common info for this
        // UDF invocation that we will carry through the
        // compilation phases
        info.setUDRWriterCompileTimeData(new JdbcCompileTimeData());

        // retrieve the compile time data, we will do this for
        // every compile phase
        JdbcConnectionInfo jci = getConnectionInfo(info);
        SQLStatementInfo   sqi = getSQLStatementInfo(info);

        // process input parameters
        handleInputParams(info, jci, sqi, true);
   }

    // override the runtime method
    @Override
    public void processData(UDRInvocationInfo info,
                            UDRPlanInfo plan)
        throws UDRException
    {
        // retrieve the compile time data, we will do this for
        // every compile phase
        JdbcConnectionInfo jci = new JdbcConnectionInfo();
        SQLStatementInfo   sqi = new SQLStatementInfo();
        int numCols = info.out().getNumColumns();

        // process input parameters (again, now at runtime)
        handleInputParams(info, jci, sqi, false);

        int numSQLStatements = sqi.getNumStatements();
        int numSQLResultSets = 0;
        String stmtText = null;

        try {
            Connection conn = jci.connect();
            Statement stmt = conn.createStatement();

            for (int s=0; s<numSQLStatements; s++)
            {
                stmtText = sqi.getStatementText(s);

                boolean hasResultSet = stmt.execute(stmtText);

                if (hasResultSet)
                {
                    ResultSet rs = stmt.getResultSet();
                    numSQLResultSets++;

                    if (numSQLResultSets > 1)
                        throw new UDRException(
                                38700,
                                "More than one result set returned by UDF %s",
                                info.getUDRName());

                    if (rs.getMetaData().getColumnCount() != numCols)
                        throw new UDRException(
                                38702,
                                "Number of columns returned by UDF %s (%d) differs from the number determined at compile time (%d)",
                                info.getUDRName(),
                                rs.getMetaData().getColumnCount(),
                                numCols);

                    while (rs.next())
                    {
                        for (int c=0; c<numCols; c++)
                        {
                            TypeInfo typ = info.out().getColumn(c).getType();

                            switch (typ.getSQLTypeSubClass())
                            {
                            case FIXED_CHAR_TYPE:
                            case VAR_CHAR_TYPE:
                                info.out().setString(c, rs.getString(c+1));
                                break;

                            case EXACT_NUMERIC_TYPE:
                                info.out().setLong(c, rs.getLong(c+1));
                                break;

                            case APPROXIMATE_NUMERIC_TYPE:
                                info.out().setDouble(c, rs.getDouble(c+1));
                                break;

                            case DATE_TYPE:
                                info.out().setTime(c, rs.getDate(c+1));
                                break;

                            case TIME_TYPE:
                                info.out().setTime(c, rs.getTime(c+1));
                                break;

                            case TIMESTAMP_TYPE:
                                info.out().setTime(c, rs.getTimestamp(c+1));
                                break;

                            case LOB_SUB_CLASS:
                                throw new UDRException(38710, "LOB parameters not yet supported");

                            default:
                                throw new UDRException(38720, "Unexpected data type encountered");

                            } // switch

                            if (rs.wasNull())
                                info.out().setNull(c);
                        } // loop over columns

                        // produce a result row
                        emitRow(info);

                    } // loop over result rows
                } // statement produces a result set
            } // loop over statements
            jci.disconnect();
        } catch (SQLException e) {
            throw new UDRException(
                    38730,
                    "Error preparing statement %s at runtime: %s",
                    stmtText,
                    e.getMessage());
        }
    }
};
