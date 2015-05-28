
echo "[ODBC]" > MXODSN
echo "traceFlags              = 6" >> MXODSN
echo "TraceStart              = 1" >> MXODSN
echo "TraceFile               = TRLOG" >> MXODSN
echo "" >> MXODSN
echo "[ODBC Data Sources]" >> MXODSN
echo "$DATASOURCE             = NonStop ODBC/MX 2.0" >> MXODSN
echo "" >> MXODSN
echo "DataSourceName          = Driver" >> MXODSN
echo "" >> MXODSN
echo "[DataSourceName]" >> MXODSN
echo "Driver                  = NonStop ODBC/MX 2.0" >> MXODSN
echo "" >> MXODSN
echo "[$DATASOURCE]" >> MXODSN
echo "Description                 = Default Data Source" >> MXODSN
echo "Catalog                     = $CAT " >> MXODSN
echo "Schema                      = $SCH " >> MXODSN
echo "DataLang                    = 0" >> MXODSN
echo "FetchBufferSize             = SYSTEM_DEFAULT" >> MXODSN
echo "Server                      = TCP:$SERVER:$PORT" >> MXODSN
echo "SQL_ATTR_CONNECTION_TIMEOUT = SYSTEM_DEFAULT" >> MXODSN
echo "SQL_LOGIN_TIMEOUT           = SYSTEM_DEFAULT" >> MXODSN
echo "SQL_QUERY_TIMEOUT           = NO_TIMEOUT" >> MXODSN
