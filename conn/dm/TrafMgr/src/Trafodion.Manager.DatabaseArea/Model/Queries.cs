//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2007-2015 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
//

using System;
using System.Data;
using System.Data.Odbc;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// Queries for metadata
    /// </summary>
    static public class Queries
    {

        private static string TRACE_SUB_AREA_NAME = "Execute Queries";

        /// <summary>
        /// Query text that returns the UID of a catalog given its internal name
        /// </summary>
        /// <param name="aConnection">The connection</param>
        /// <param name="anInternalCatalogName">The internal catalog name</param>
        /// <returns>The query text</returns>
        static public string SelectCatalogUIDQueryText(Connection aConnection, string anInternalCatalogName)
        {
            return String.Format("select cat_uid from {0}.system_schema.catsys where cat_name = '{1}' FOR READ UNCOMMITTED ACCESS",
                new object[] { aConnection.SystemCatalogName, anInternalCatalogName });
        }

        /// <summary>
        /// Get the names and attributes of the schemas in a catalog
        /// </summary>
        /// <param name="aConnection">an ODBC connection</param>
        /// <param name="anInternalCatalogName">the internal name of the catalog</param>
        /// <returns>The query returns a result set with one row per schema<para/>| schema internal name | owner | version | subvol | UID |</returns>
        static public OdbcDataReader ExecuteSelectSchemaAttributes(Connection aConnection, string anInternalCatalogName)
        {
/*            OdbcCommand theQuery = new OdbcCommand(
                String.Format("select schema_name,schema_owner,authname(schema_owner),schema_version,schema_subvolume,schema_uid"
                + " from {0}.system_schema.schemata where cat_uid = ({1}) and current_operation <> 'VS' "
                + "order by schema_name FOR READ UNCOMMITTED ACCESS;",
                new object[] { aConnection.SystemCatalogName, SelectCatalogUIDQueryText(aConnection, anInternalCatalogName) })
                );*/
                        OdbcCommand theQuery = new OdbcCommand(
                            String.Format("select distinct schema_name,0,'',1,'',1"
                            + " from {0}.\"_MD_\".objects "
                            + "order by schema_name FOR READ UNCOMMITTED ACCESS;",
                            aConnection.SystemCatalogName)
                            );
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Get the name and attributes of a specific schema in a catalog
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="anInternalCatalogName"></param>
        /// <param name="anInternalSchemaName"></param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectSchemaAttributes(Connection aConnection, string anInternalCatalogName, string anInternalSchemaName)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format("select distinct schema_name,0,'',1,'',1"
                + " from {0}.\"_MD_\".objects "
                + " where schema_name = '{1}' order by schema_name FOR READ UNCOMMITTED ACCESS;",
                new object[] { aConnection.SystemCatalogName, anInternalSchemaName })
                );
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }
        /// <summary>
        /// Get the names and attributes of all catalogs
        /// </summary>
        /// <param name="aConnection">an ODBC connection</param>
        /// <returns>A result set</returns>
        static public OdbcDataReader ExecuteSelectCatalogsAttributes(Connection aConnection)
        {
            OdbcCommand theQuery = new OdbcCommand(String.Format("select cat_name,cat_uid,local_smd_volume from {0}.system_schema.catsys ORDER BY cat_name FOR READ UNCOMMITTED ACCESS",
                new object[] { aConnection.SystemCatalogName }));
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Get the attributes of a specific catalog
        /// </summary>
        /// <param name="aConnection">an ODBC connection</param>
        /// <param name="anInternalCatalogName">the internal name of the catalog</param>
        /// <returns>A result set</returns>
        static public OdbcDataReader ExecuteSelectCatalogAttributes(Connection aConnection, String anInternalCatalogName)
        {
            OdbcCommand theQuery = new OdbcCommand(String.Format("select cat_uid,local_smd_volume from {0}.system_schema.catsys where cat_name = '{1}' FOR READ UNCOMMITTED ACCESS",
                new object[] { aConnection.SystemCatalogName, anInternalCatalogName }));
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Retrieves information about a catalog's registration.
        /// </summary>
        /// <param name="systemCatalogName">A system catalog name</param>
        /// <param name="aConnection">An ODBC connection.</param>
        /// <param name="aCatalogUID">The UID of the catalog.</param>
        /// <returns>The query returns a result set with one row per schema<para/>| segment name | volume name | rule |</returns>
        static public OdbcDataReader ExecuteSelectTrafodionCatalogRegistrations(String systemCatalogName, Connection aConnection, long aCatalogUID)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format("select node_name,smd_volume,replication_rule from {0}.system_schema.cat_references where cat_uid = {1} FOR READ UNCOMMITTED ACCESS;",
                new object[] { systemCatalogName, aCatalogUID }));
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /**
         * Get info from the ACCESS_PATHS table for a all of the objects of a given type and namespace in a schema
         * @param aConnection an ODBC connection
         * @param aColumnNamesList a comma separated string of the names of the desired columns - OBJECT_NAME and OBJECT_UI will be prepended internally
         * @param aCatalogName the name of the catalog
         * @param aSchemaUID the UID of the schema
         * @param aSchemaVersion the schema's version
         * @param anObjectType The type of objects to return e.g. "BT" for base table.
         * @param anObjectNameSpace The namespace of the objects to return e.g. "TA" for the table namespace.
         * @throws SQLException failure
         * @return a result for OBJECT_NAME and OBJECT_UID followed by the desired columns
         */
        static public OdbcDataReader ExecuteSelectAccessPathsInfo(Connection aConnection, String aColumnNamesList, TrafodionCatalog aTrafodionCatalog, long aSchemaUID, int aSchemaVersion, String anObjectType, String anObjectNameSpace)
        {
            string theQueryText = "SELECT OBJECT_NAME,O.OBJECT_UID," + aColumnNamesList + " FROM " + aTrafodionCatalog.ExternalName + "."_MD_"" + ".OBJECTS AS O"
                + " LEFT JOIN " + aTrafodionCatalog.ExternalName + "."_MD_"" + ".ACCESS_PATHS AS A"
                + " ON O.OBJECT_UID = A.ACCESS_PATH_UID"
                + " WHERE OBJECT_TYPE IN ('" + anObjectType + "') AND OBJECT_NAME_SPACE IN ('" + anObjectNameSpace + "') AND SCHEMA_UID = " + aSchemaUID + " FOR READ UNCOMMITTED ACCESS";

            OdbcCommand theQuery = new OdbcCommand(theQueryText);
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Select the sql object attributes information from the metadata tables
        /// </summary>
        /// <param name="aConnection">Connection</param>
        /// <param name="aColumnNamesList">a list of columns to select from the ACCESS_PATHS table</param>
        /// <param name="aTrafodionCatalog">Catalog name</param>
        /// <param name="aSchemaUID">Schema UID</param>
        /// <param name="aSchemaVersion">Schema version</param>
        /// <param name="anObjectUID">Object UID</param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectAccessPathsInfo(Connection aConnection, String aColumnNamesList, TrafodionCatalog aTrafodionCatalog, long aSchemaUID, int aSchemaVersion, long anObjectUID)
        {
            string theQueryText = String.Format("SELECT OBJECT_NAME, O.OBJECT_UID, {0} "
                + " FROM {1}."_MD_".OBJECTS AS O"
                + " LEFT JOIN {1}."_MD_".ACCESS_PATHS AS A"
                + " ON O.OBJECT_UID = A.ACCESS_PATH_UID"
                + " WHERE OBJECT_UID = {3} AND SCHEMA_UID = {4} FOR READ UNCOMMITTED ACCESS",
                new object[] { aColumnNamesList, aTrafodionCatalog.ExternalName, aSchemaVersion, anObjectUID, aSchemaUID });

            OdbcCommand theQuery = new OdbcCommand(theQueryText);
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Retrieves information about objects under a schema given the schema's catalog and UID.
        /// </summary>
        /// <param name="aConnection">An ODBC connection.</param>
        /// <param name="aTrafodionCatalog">The catalog in which to look.</param>
        /// <param name="aSchemaUID">The UID of the schema.</param>
        /// <param name="aSchemaVersion">The schema's version.</param>
        /// <param name="anObjectType">The type of objects to return. e.g. "BT" for base table.</param>
        /// <param name="anObjectNameSpace">The namespace of the object.</param>
        /// <returns>
        /// The query returns a result set with one row per schema object.<para/>| internal name | UID | create time | redef time | security class |
        /// </returns>
        static public OdbcDataReader ExecuteSelectSchemaObjectNames(Connection aConnection, TrafodionCatalog aTrafodionCatalog, String schemaName, int aSchemaVersion, String anObjectType, String anObjectNameSpace)
        {
            OdbcCommand theQuery = new OdbcCommand(String.Format("SELECT OBJECT_NAME,OBJECT_UID,CREATE_TIME,REDEF_TIME, '', '' FROM {0}.\"_MD_\".OBJECTS"
                + " WHERE OBJECT_TYPE = '{2}' AND SCHEMA_NAME = '{4}' ORDER BY OBJECT_NAME FOR READ UNCOMMITTED ACCESS;",
                new object[] { aTrafodionCatalog.ExternalName, aSchemaVersion, anObjectType, anObjectNameSpace, schemaName }));

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Retrieves UDF Action object names
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aTrafodionUDFunction"></param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectUDFActionObjectNames(Connection aConnection, TrafodionUDFunction aTrafodionUDFunction)
        {
            OdbcCommand theQuery = new OdbcCommand(String.Format("SELECT ACTION_CAT_UID, ACTION_SCH_UID, ACTION_UID, ACTION_NAME FROM {0}."_MD_".ROUTINE_ACTIONS"
                + " WHERE UUDF_UID = {2} AND UUDF_CAT_UID = {3} AND UUDF_SCH_UID = {4} ORDER BY ACTION_NAME FOR READ UNCOMMITTED ACCESS;",
                new object[] { aTrafodionUDFunction.TheTrafodionCatalog.ExternalName, aTrafodionUDFunction.TheTrafodionSchema.Version, aTrafodionUDFunction.UID, aTrafodionUDFunction.TheTrafodionCatalog.UID, aTrafodionUDFunction.TheTrafodionSchema.UID }));

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Retrieves information about views in a given schema
        /// </summary>
        /// <param name="aConnection">An ODBC connection.</param>
        /// <param name="aTrafodionSchema">The schema.</param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectViewsInSchema(Connection aConnection, TrafodionSchema aTrafodionSchema)
        {
            OdbcCommand theQuery = new OdbcCommand();

            theQuery.CommandText = String.Format("SELECT T1.OBJECT_NAME, T1.OBJECT_UID, T1.CREATE_TIME, T1.REDEF_TIME, '', "
            + " '', '', T1.VALID_DEF "
            + " FROM {0}.\"_MD_\".OBJECTS T1"
            + " WHERE T1.OBJECT_TYPE = '{1}' AND T1.SCHEMA_NAME = '{2}' "
            + " ORDER BY T1.OBJECT_NAME FOR READ UNCOMMITTED ACCESS;",
            new object[] { aTrafodionSchema.TheTrafodionCatalog.ExternalName, TrafodionView.ObjectType, aTrafodionSchema.InternalName});

            
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Retrieves information about a named object under a schema given the schema's catalog and UID.
        /// </summary>
        /// <param name="aConnection">An ODBC connection.</param>
        /// <param name="aTrafodionCatalog">The catalog in which to look.</param>
        /// <param name="aSchemaUID">The UID of the schema.</param>
        /// <param name="aSchemaVersion">The schema's version.</param>
        /// <param name="anObjectType">The type of objects to return. e.g. "BT" for base table.</param>
        /// <param name="anObjectName">Object name</param>
        /// <param name="anObjectNameSpace">The namespace of the object.</param>
        /// <returns>
        /// The query returns a result set with one row about the schema object.<para/>| internal name | UID | create time | redef time | security class |
        /// </returns>
        static public OdbcDataReader ExecuteSelectSchemaObjectByName(Connection aConnection, TrafodionCatalog aTrafodionCatalog, String aSchemaName, int aSchemaVersion, String anObjectType, String anObjectName, String anObjectNameSpace)
        {
            OdbcCommand theQuery = new OdbcCommand(String.Format("SELECT OBJECT_NAME,OBJECT_UID,CREATE_TIME,REDEF_TIME, '', '' FROM {0}.\"_MD_\".OBJECTS"
                + " WHERE OBJECT_TYPE = '{1}' AND SCHEMA_NAME = '{2}' AND OBJECT_NAME = '{3}' ORDER BY OBJECT_NAME FOR READ UNCOMMITTED ACCESS;",
                new object[] { aTrafodionCatalog.ExternalName, anObjectType, aSchemaName, anObjectName }));

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Retrives information for a specificed view using the view name
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aTrafodionSchema"></param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectViewByName(Connection aConnection, TrafodionSchema aTrafodionSchema, String anObjectName)
        {
            OdbcCommand theQuery = new OdbcCommand();
            if (aConnection.TheConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140)
            {
                theQuery.CommandText = String.Format("SELECT T1.OBJECT_NAME, T1.OBJECT_UID, T1.CREATE_TIME, T1.REDEF_TIME, T1.OBJECT_SECURITY_CLASS, "
                + " T2.VIEW_TYPE, AUTHNAME(T1.OBJECT_OWNER), T1.VALID_DEF "
                + " FROM {0}."_MD_".OBJECTS T1, {0}."_MD_".VWS T2 "
                + " WHERE T1.OBJECT_TYPE = '{2}' AND T1.OBJECT_NAME_SPACE = '{3}' AND T1.SCHEMA_UID = {4} "
                + " AND T1.OBJECT_NAME = '{5}'"
                + " AND T1.OBJECT_UID = T2.OBJECT_UID"
                + " ORDER BY T1.OBJECT_NAME FOR READ UNCOMMITTED ACCESS;",
                new object[] { aTrafodionSchema.TheTrafodionCatalog.ExternalName, aTrafodionSchema.Version, 
                TrafodionView.ObjectType, TrafodionView.ObjectNameSpace, aTrafodionSchema.UID, anObjectName});
            }
            else 
            {
                theQuery.CommandText = String.Format("SELECT T1.OBJECT_NAME, T1.OBJECT_UID, T1.CREATE_TIME, T1.REDEF_TIME, T1.OBJECT_SECURITY_CLASS, "
                + " T2.VIEW_TYPE, AUTHNAME(T1.OBJECT_OWNER) "
                + " FROM {0}."_MD_".OBJECTS T1, {0}."_MD_".VWS T2 "
                + " WHERE T1.OBJECT_TYPE = '{2}' AND T1.OBJECT_NAME_SPACE = '{3}' AND T1.SCHEMA_UID = {4} "
                + " AND T1.OBJECT_NAME = '{5}'"
                + " AND T1.OBJECT_UID = T2.OBJECT_UID"
                + " ORDER BY T1.OBJECT_NAME FOR READ UNCOMMITTED ACCESS;",
                new object[] { aTrafodionSchema.TheTrafodionCatalog.ExternalName, aTrafodionSchema.Version, 
                TrafodionView.ObjectType, TrafodionView.ObjectNameSpace, aTrafodionSchema.UID, anObjectName});
            } 
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Selects the table column definition details
        /// </summary>
        /// <param name="aConnection">A connection</param>
        /// <param name="aColumnNamesList">List of column details to fetch</param>
        /// <param name="aCatalogExternalName">Catalog</param>
        /// <param name="aTrafodionTableUID">Table UID</param>
        /// <param name="aSchemaVersion">Schema version</param>
        /// <returns>A datareader</returns>
        static public OdbcDataReader ExecuteSelectTableColumnInfo(Connection aConnection, TrafodionSchemaObject schemaObj)
        {
            string theQueryText = String.Format("SELECT COLUMN_NAME, COLUMN_NUMBER, SQL_DATA_TYPE, FS_DATA_TYPE, COLUMN_SIZE, "
                    + "COLUMN_SCALE, COLUMN_PRECISION, NULLABLE, CHARACTER_SET, DATETIME_START_FIELD, DATETIME_END_FIELD,"
                    + "DATETIME_QUALIFIER, DEFAULT_VALUE FROM {0}.\"_MD_\".COLUMNS_VIEW"
                + " WHERE CATALOG_NAME = '{1}' AND SCHEMA_NAME = '{2}' AND TABLE_NAME = '{3}' FOR READ UNCOMMITTED ACCESS",
                new object[] { schemaObj.TheTrafodionCatalog.ExternalName, schemaObj.TheTrafodionCatalog.InternalName,
                    schemaObj.TheTrafodionSchema.InternalName, schemaObj.InternalName});

            OdbcCommand theQuery = new OdbcCommand(theQueryText);
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        static public OdbcDataReader ExecuteSelectSchemaObjectColumnInfo(Connection aConnection, TrafodionSchemaObject schemaObj)
        {
            string theQueryText = String.Format("SELECT C.COLUMN_NAME, C.COLUMN_NUMBER, "
                + " CAST(CASE WHEN C.FS_DATA_TYPE = 130 THEN    'SIGNED SMALLINT   '  "
                + " WHEN C.FS_DATA_TYPE = 131 THEN 'UNSIGNED SMALLINT ' "
                + " WHEN C.FS_DATA_TYPE = 132 THEN 'SIGNED INTEGER    '  "
                + " WHEN C.FS_DATA_TYPE = 133 THEN 'UNSIGNED INTEGER  '  "
	            + " WHEN C.FS_DATA_TYPE = 134 THEN 'SIGNED LARGEINT   '  "
	            + " WHEN C.FS_DATA_TYPE = 135 THEN 'UNSIGNED SMALLINT ' "
                + " WHEN C.FS_DATA_TYPE = 140 THEN 'REAL              '  "
	            + " WHEN C.FS_DATA_TYPE = 141 THEN 'DOUBLE            '  "
	            + " WHEN C.FS_DATA_TYPE = 150 THEN 'UNSIGNED DECIMAL  '  "
	            + " WHEN C.FS_DATA_TYPE = 151 THEN 'SIGNED DECIMAL    ' "
                + " WHEN C.FS_DATA_TYPE = 155 THEN 'UNSIGNED NUMERIC  '  "
	            + " WHEN C.FS_DATA_TYPE = 156 THEN 'SIGNED NUMERIC    '  "
	            + " WHEN C.FS_DATA_TYPE = 0 THEN   'CHARACTER         '  "
	            + " WHEN C.FS_DATA_TYPE = 2 THEN 'CHARACTER         ' "
                + " WHEN C.FS_DATA_TYPE = 70 THEN 'LONG VARCHAR      '  "
	            + " WHEN C.FS_DATA_TYPE = 64 THEN 'VARCHAR           '  "
	            + " WHEN C.FS_DATA_TYPE = 66 THEN 'VARCHAR           '  "
	            + " WHEN C.FS_DATA_TYPE = 100 THEN 'VARCHAR           ' "
                + " WHEN C.FS_DATA_TYPE = 101 THEN 'VARCHAR           '  "
	            + " WHEN C.FS_DATA_TYPE = 192 THEN 'DATETIME          '  "
	            + " WHEN C.FS_DATA_TYPE >= 196 AND C.FS_DATA_TYPE <= 207 THEN 'INTERVAL          '  "
	            + " ELSE 'UNKNOWN' END AS CHAR(24)) SQL_DATA_TYPE,  "
	            + " C.FS_DATA_TYPE, C.COLUMN_SIZE, "
                + " C.COLUMN_PRECISION, C.COLUMN_SCALE, C.NULLABLE, C.CHARACTER_SET, "
                + " C.DATETIME_START_FIELD, C.DATETIME_END_FIELD,  "
	            + " CAST(CASE WHEN C.FS_DATA_TYPE = 192 AND C.DATETIME_END_FIELD = 6 THEN '(' || TRIM(CAST(C.COLUMN_SCALE AS VARCHAR(2))) || ')'  "
	            + " ELSE ' ' END AS CHAR(28)) DATETIME_QUALIFIER,  "
	            + " TRANSLATE(TRIM(C.DEFAULT_VALUE) USING UCS2TOUTF8) DEFAULT_VALUE  "
	            + " FROM {0}.\"_MD_\".OBJECTS O, {0}.\"_MD_\".COLUMNS C  "
	            + " WHERE O.CATALOG_NAME = '{1}' AND O.SCHEMA_NAME = '{2}'  "
	            + " AND O.OBJECT_NAME = '{3}' "
	            + " AND O.OBJECT_UID = C.OBJECT_UID  "
	            + " AND O.OBJECT_TYPE = '{4}'  "
	            + " FOR READ UNCOMMITTED ACCESS ORDER BY 2 ;",
                new object[] { schemaObj.TheTrafodionCatalog.ExternalName, schemaObj.TheTrafodionCatalog.InternalName,
                    schemaObj.TheTrafodionSchema.InternalName, schemaObj.InternalName, schemaObj.SchemaObjectType});

            OdbcCommand theQuery = new OdbcCommand(theQueryText);
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }
        /// <summary>
        /// Retruns the parameter information for a routine
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aColumnNamesList"></param>
        /// <param name="aTrafodionRoutine"></param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectRoutineColumnInfo(Connection aConnection, String aColumnNamesList, TrafodionRoutine aTrafodionRoutine)
        {
            string theQueryText = String.Format("SELECT {0} FROM {1}."_MD_".ROUTINE_PARAMS"
                + " WHERE ROUTINE_UID = {3} FOR READ UNCOMMITTED ACCESS",
                new object[] { aColumnNamesList, aTrafodionRoutine.TheTrafodionCatalog.ExternalName, aTrafodionRoutine.TheTrafodionSchema.Version, aTrafodionRoutine.UID });

            OdbcCommand theQuery = new OdbcCommand(theQueryText);
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }
        /**
         *
         * * @param aColumnNamesList a comma separated string of the names of the desired columns
         * @param aCatalogName the name of the catalog
         * @param aSchemaVersion the schema's version
         * @param anObjectUID the UID of the object
         * @return
         */
        static public OdbcDataReader ExecuteSelectPartitionInfo(Connection aConnection, String aColumnNamesList, TrafodionCatalog aTrafodionCatalog, int aSchemaVersion, long anObjectUID)
        {
            string theQueryText = "SELECT " + aColumnNamesList + " FROM "
            + aTrafodionCatalog.ExternalName + "."_MD_"" + ".PARTITIONS"
            + " WHERE OBJECT_UID = " + anObjectUID
            + " ORDER BY ENCODED_KEY, PARTITION_STATUS"
            + " FOR READ UNCOMMITTED ACCESS";

            OdbcCommand theQuery = new OdbcCommand(theQueryText);
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="aConnection">An ODBC connection.</param>
        /// <param name="aColumnNamesList">aColumnNamesList a comma separated string of the names of the desired columns</param>
        /// <param name="aFullObjectName">A Fully qualified three part object name</param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectPartitionDetailInfo(Connection aConnection, String aColumnNamesList, String aFullObjectName, bool isIndex)
        {
            string type = (isIndex) ? "INDEX " : "";
            string theQueryText = "SELECT " + aColumnNamesList + " FROM TABLE(DISK LABEL STATISTICS(" + type + aFullObjectName + ")) ORDER BY PARTITION_NUM FOR READ UNCOMMITTED ACCESS;";

            OdbcCommand theQuery = new OdbcCommand(theQueryText);
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Select the rowcount for a given table
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aFullObjectName">The 3 part ansi name of the table</param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectTableRowCount(Connection aConnection, String aFullObjectName)
        {
            string theQueryText = "SELECT SUM(ROW_COUNT) FROM TABLE(DISK LABEL STATISTICS(" + aFullObjectName + ")) FOR READ UNCOMMITTED ACCESS";

            OdbcCommand theQuery = new OdbcCommand(theQueryText);
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Fetches all table constraints. This includes primary key, unique, foreign and check constraints
        /// </summary>
        /// <param name="aConnection">A connection</param>
        /// <param name="anExternalCatalogName">The catalog name</param>
        /// <param name="aSchemaVersion">Schema version</param>
        /// <param name="aSchemaUID">UID of schema</param>
        /// <param name="aTableUID">UID of table</param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectFromTBL_CONSTRAINTS(Connection aConnection, String anExternalCatalogName, String aSchemaName, long aTableUID)
        {
            OdbcCommand theQuery;

                theQuery = new OdbcCommand(String.Format("SELECT T1.CONSTRAINT_UID, T1.CONSTRAINT_TYPE, T1.INDEX_UID, T2.OBJECT_NAME, T2.OBJECT_TYPE"
                  + " FROM {0}.\"_MD_\".TABLE_CONSTRAINTS T1, {0}.\"_MD_\".OBJECTS T2"
                  + " WHERE T1.TABLE_UID = {1} AND T1.CONSTRAINT_UID = T2.OBJECT_UID AND T2.SCHEMA_NAME = '{2}' FOR READ UNCOMMITTED ACCESS;",
                  new object[] { anExternalCatalogName, aTableUID, aSchemaName })
                  );


            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Select the columns from the OBJECTS metadata table
        /// </summary>
        /// <param name="aConnection">Connection</param>
        /// <param name="aColumnNamesList">columns to fetch</param>
        /// <param name="anExternalCatalogName">Catalog name</param>
        /// <param name="aSchemaVersion">Schema version</param>
        /// <param name="anObjectUID">Object UID</param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectFromOBJECTS(Connection aConnection, String aColumnNamesList, String anExternalCatalogName, int aSchemaVersion, long anObjectUID)
        {
            string theQueryText = "select " + aColumnNamesList + " from " + anExternalCatalogName + "."_MD_"" + ".OBJECTS where OBJECT_UID = " + anObjectUID + " FOR READ UNCOMMITTED ACCESS";

            OdbcCommand theQuery = new OdbcCommand(theQueryText);
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Select the check constraint column names
        /// </summary>
        /// <param name="aConnection">Connection</param>
        /// <param name="anExternalCatalogName">Catalog name</param>
        /// <param name="aSchemaVersion">Schema version</param>
        /// <param name="aTableUID">Table UID</param>
        /// <param name="aConstraintUID">Constraint UID</param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectCheckConstraintColumnNames(Connection aConnection, String anExternalCatalogName, int aSchemaVersion, long aTableUID, long aConstraintUID)
        {
            string theQueryText = "SELECT T1.COLUMN_NAME FROM " + anExternalCatalogName + "."_MD_"" + ".COLS AS T1, " + anExternalCatalogName + "."_MD_"" + ".CK_COL_USAGE AS T2"
            + " WHERE T2.CONSTRAINT_UID = " + aConstraintUID + " AND T2.TABLE_UID = " + aTableUID + " AND T2.TABLE_UID = T1.OBJECT_UID AND T2.COLUMN_NUMBER = T1.COLUMN_NUMBER FOR READ UNCOMMITTED ACCESS;";

            OdbcCommand theQuery = new OdbcCommand(theQueryText);
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Select the primary key details from metadata
        /// </summary>
        /// <param name="aConnection">Connection</param>
        /// <param name="anExternalCatalogName">catalog name</param>
        /// <param name="aSchemaVersion">Schema version</param>
        /// <param name="aTableUID">Table UID</param>
        /// <param name="anIndexUID">Index UID</param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectPrimaryKeyColumnInfo(Connection aConnection, String anExternalCatalogName, int aSchemaVersion, long aTableUID, long anIndexUID)
        {
            string theQueryText = String.Format("SELECT T2.COLUMN_NAME, T1.ORDERING, T1.CLUSTERING_KEY_SEQ_NUM,"
                + " T1.SYSTEM_ADDED_COLUMN, T2.COLUMN_NUMBER FROM {0}."_MD_".ACCESS_PATH_COLS AS T1, "
                + " {0}."_MD_".COLS AS T2"
                + " WHERE T1.ACCESS_PATH_UID = {2} AND T2.OBJECT_UID = {2} AND"
                + " T1.COLUMN_NUMBER = T2.COLUMN_NUMBER AND T1.CLUSTERING_KEY_SEQ_NUM > 0"
                + " ORDER BY T1.CLUSTERING_KEY_SEQ_NUM FOR READ UNCOMMITTED ACCESS;",
                new object[] { anExternalCatalogName, aSchemaVersion, aTableUID });

            OdbcCommand theQuery = new OdbcCommand(theQueryText);
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Select the primary key details from metadata
        /// </summary>
        /// <param name="aConnection">Connection</param>
        /// <param name="anExternalCatalogName">catalog name</param>
        /// <param name="aSchemaVersion">Schema version</param>
        /// <param name="aTableUID">Table UID</param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectPrimaryKeyColumnInfo(Connection aConnection, String anExternalCatalogName, int aSchemaVersion, long aTableUID, TrafodionPrimaryKey aTrafodionPrimaryKey)
        {
            string theQueryText = String.Format("SELECT T2.COLUMN_NAME, T2.COLUMN_NUMBER, T2.COLUMN_NUMBER,"
                + " T2.ORDERING, T2.KEYSEQ_NUMBER, T2.KEYSEQ_NUMBER, 'N'"
                + " FROM {0}.\"_MD_\".KEYS AS T2 "
                + " WHERE T2.OBJECT_UID = {3} AND T2.COLUMN_NAME <> '_SALT_'"
                + " ORDER BY T2.KEYSEQ_NUMBER FOR READ UNCOMMITTED ACCESS;",
                new object[] { anExternalCatalogName, aSchemaVersion, aTrafodionPrimaryKey.UID, aTableUID });

            OdbcCommand theQuery = new OdbcCommand(theQueryText);
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Fetches all the materialized views that use a given table or MV
        /// </summary>
        /// <param name="aConnection">The connection</param>
        /// <param name="anExternalCatalogName">The name of the catalog containing the definition schema</param>
        /// <param name="aSchemaVersion">schema version</param>
        /// <param name="aTableUID">UID of the table</param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectMVsUsingObject(Connection aConnection, String anExternalCatalogName, int aSchemaVersion, long aTableUID)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format("SELECT T2.OBJECT_NAME as MV_NAME, T2.SCHEMA_UID"
                + " FROM {0}."_MD_".MVS_USED AS T1, {0}."_MD_".OBJECTS AS T2"
                + " WHERE T1.USED_OBJECT_UID = {2} AND T1.USAGE_ORIGIN <> 'E' AND T2.OBJECT_UID = T1.MV_UID ORDER BY MV_NAME FOR READ UNCOMMITTED ACCESS;",
                new object[] { anExternalCatalogName, aSchemaVersion, aTableUID })
                );

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Fetches all the views that use a given object
        /// </summary>
        /// <param name="aConnection">The connection</param>
        /// <param name="anExternalCatalogName">The name of the catalog containing the definition schema</param>
        /// <param name="aSchemaVersion">schema version</param>
        /// <param name="aObjectUID">UID of the object</param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectViewsUsingObject(Connection aConnection, String anExternalCatalogName, int aSchemaVersion, long aObjectUID)
        {
            DatabaseAreaOptions options = DatabaseAreaOptions.GetOptions();
            OdbcCommand theQuery = new OdbcCommand();

            theQuery.CommandText = String.Format("SELECT T2.OBJECT_NAME, T2.SCHEMA_UID, T3.VIEW_TYPE"
                + " FROM {0}."_MD_".VW_TBL_USAGE AS T1, {0}."_MD_".OBJECTS AS T2"
                + " , {0}."_MD_".VWS AS T3"
                + " WHERE T1.USED_OBJ_UID = {2} "
                + " AND T2.OBJECT_UID = T1.USING_VIEW_UID"
                + " AND T2.OBJECT_UID = T3.OBJECT_UID "
                + " FOR READ UNCOMMITTED ACCESS;",
                new object[] { anExternalCatalogName, aSchemaVersion, aObjectUID }
                );
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Selects a list of synonyms defined on the Sql object. 
        /// This method can be used to find the synonym usage for tables, mvs and views
        /// </summary>
        /// <param name="aConnection">The connection</param>
        /// <param name="anExternalCatalogName">The name of the catalog containing the definition schema</param>
        /// <param name="aSchemaVersion">The schema version</param>
        /// <param name="anObjectUID">The UID of the sql object</param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectSynonymsOnObjects(Connection aConnection, String anExternalCatalogName, int aSchemaVersion, long anObjectUID)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format("SELECT T2.OBJECT_NAME AS NAME, T2.SCHEMA_UID"
                + " FROM {0}."_MD_".SYNONYM_USAGE AS T1, {0}."_MD_".OBJECTS AS T2"
                + " WHERE T1.UNDERLYING_OBJ_UID = {2} AND T2.OBJECT_UID = T1.OBJECT_UID ORDER BY NAME FOR READ UNCOMMITTED ACCESS;",
                new object[] { anExternalCatalogName, aSchemaVersion, anObjectUID })
                );

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);

        }

        /// <summary>
        /// Selects a list of MVs used by a Object
        /// </summary>
        /// <param name="aConnection">The connection</param>
        /// <param name="anExternalCatalogName">The name of the catalog containing the definition schema</param>
        /// <param name="aSchemaVersion">The schema version</param>
        /// <param name="anObjectUID">The UID of the sql object</param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectMVsUsedByMe(Connection aConnection, String anExternalCatalogName, int aSchemaVersion, long anObjectUID)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format("SELECT T2.OBJECT_NAME AS NAME, T2.SCHEMA_UID"
                + " FROM {0}."_MD_".MVS_USED AS T1, {0}."_MD_".OBJECTS AS T2"
                + " WHERE T1.MV_UID = {2} AND T1.USED_OBJECT_UID = T2.OBJECT_UID  AND T1.USAGE_ORIGIN <> 'E' AND T1.USED_OBJECT_TYPE <> 'BT'  ORDER BY NAME FOR READ UNCOMMITTED ACCESS;",
                new object[] { anExternalCatalogName, aSchemaVersion, anObjectUID })
                );

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);

        }

        /// <summary>
        /// Select the routines used by the materialized view
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aTrafodionMaterializedView"></param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectRoutinesUsedByMV(Connection aConnection, TrafodionMaterializedView aTrafodionMaterializedView)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format("SELECT T2.OBJECT_NAME AS NAME, T2.SCHEMA_UID , T3.UDR_TYPE "
                    + "FROM {0}."_MD_".MVS_USED AS T1, "
                    + "{0}."_MD_".OBJECTS AS T2 , "
                    + "{0}."_MD_".ROUTINES AS T3 "
                    + "WHERE T1.MV_UID = {2} "
                    + "AND T1.USED_OBJECT_UID = T2.OBJECT_UID "
                    + "AND T2.OBJECT_TYPE = 'UR' "
                    + "AND T1.USED_OBJECT_UID = T3.UDR_UID "
                    + "ORDER BY NAME FOR READ UNCOMMITTED ACCESS;",
                new object[] { aTrafodionMaterializedView.TheTrafodionCatalog.ExternalName, aTrafodionMaterializedView.TheTrafodionSchema.Version, aTrafodionMaterializedView.UID })
                );

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }
        /// <summary>
        /// Selects a list of Tables used by a Object
        /// </summary>
        /// <param name="aConnection">The connection</param>
        /// <param name="anExternalCatalogName">The name of the catalog containing the definition schema</param>
        /// <param name="aSchemaVersion">The schema version</param>
        /// <param name="anObjectUID">The UID of the sql object</param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectTablesUsedByMe(Connection aConnection, String anExternalCatalogName, int aSchemaVersion, long anObjectUID)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format("SELECT T2.OBJECT_NAME AS NAME, T2.SCHEMA_UID"
                + " FROM {0}."_MD_".MVS_USED AS T1, {0}."_MD_".OBJECTS AS T2"
                + " WHERE T1.MV_UID = {2} AND T1.USED_OBJECT_UID = T2.OBJECT_UID  AND T1.USAGE_ORIGIN <> 'E' AND T1.USED_OBJECT_TYPE <> 'MV'  ORDER BY NAME FOR READ UNCOMMITTED ACCESS;",
                new object[] { anExternalCatalogName, aSchemaVersion, anObjectUID })
                );

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);

        }

        /// <summary>
        /// Fetches the isInsertLog attribute for the table
        /// </summary>
        /// <param name="aConnection">The connection</param>
        /// <param name="anExternalCatalogName">The name of the catalog containing the definition schema</param>
        /// <param name="aSchemaVersion">The schema version</param>
        /// <param name="aTableUID">UID of the table</param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectTableInsertLogStatus(Connection aConnection, String anExternalCatalogName, int aSchemaVersion, long aTableUID)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format("SELECT IS_INSERTLOG FROM {0}."_MD_".MVS_TABLE_INFO"
                + " WHERE BASE_TABLE_UID = {2} FOR READ UNCOMMITTED ACCESS;",
                new object[] { anExternalCatalogName, aSchemaVersion, aTableUID })
                );

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Fetches the Table's Reorg and Update Statistics Enable/Disable status by invoking the MAINTAIN utility
        /// </summary>
        /// <param name="aConnection">The connection</param>
        /// <param name="aRealAnsiTableName">The ansi name of the table</param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteFallbackSelectMaintainStatus(Connection aConnection, String aRealAnsiTableName)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format("MAINTAIN TABLE {0} ,REORG, UPDATE STATISTICS, DISPLAY",
                new object[] { aRealAnsiTableName })
               );

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Fetches the Table's Reorg and Update Statistics Enable/Disable status by doing a select on the view defined
        /// on the maintain control info table
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="anInternalCatalogName"></param>
        /// <param name="anInternalSchemaName"></param>
        /// <param name="anInternalTableName"></param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectTableMaintainStatus(Connection aConnection, string anInternalCatalogName, string anInternalSchemaName, string anInternalTableName)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format("SELECT MAINTAIN_OP, STATUS FROM {0}.PUBLIC_ACCESS_SCHEMA.MAINTAININFOVIEW WHERE CATALOG_NAME = '{1}'"
                    + " AND SCHEMA_NAME = '{2}' AND OBJECT_NAME = '{3}' AND MAINTAIN_OP IN ('{4}','{5}') FOR READ UNCOMMITTED ACCESS;",
                new object[] { TrafodionTable.MaintainViewCatalogName, anInternalCatalogName, anInternalSchemaName, anInternalTableName, 
                    TrafodionTable.MaintainReorgOperationName,  TrafodionTable.MaintainUpdateStatsOperationName})
                );

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Select the last update stats timestamp of a table
        /// </summary>
        /// <param name="aConnection">Connection</param>
        /// <param name="anExternalCatalogName">Catalog Name</param>
        /// <param name="anExternalSchemaName">Schema name</param>
        /// <param name="aTableUID">Table UID</param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectTableLastUpdateStatsTime(Connection aConnection, String anExternalCatalogName, String anExternalSchemaName, long aTableUID)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format("SELECT CASE WHEN MAX(STATS_TIME) IS NULL THEN 0 ELSE JULIANTIMESTAMP(MAX(STATS_TIME)) END"
                + " FROM {0}.{1}.HISTOGRAMS WHERE TABLE_UID = {2} FOR READ UNCOMMITTED ACCESS;",
                new object[] { anExternalCatalogName, anExternalSchemaName, aTableUID })
                );

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Select the Not null attribute for the table columns
        /// </summary>
        /// <param name="aConnection">Connection</param>
        /// <param name="anExternalCatalogName">Catalog name</param>
        /// <param name="aSchemaVersion">Schema version</param>
        /// <param name="aNotNullConstraintUID">NotNull constraint uid</param>
        /// <param name="aTableUID">Table uid</param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectTableNotNullColumns(Connection aConnection, String anExternalCatalogName, int aSchemaVersion, long aNotNullConstraintUID, long aTableUID)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format("SELECT T1.COLUMN_NAME"
                + " FROM {0}."_MD_".COLS AS T1, {0}."_MD_".CK_COL_USAGE AS T2"
                + " WHERE T2.CONSTRAINT_UID = {2} AND T2.TABLE_UID = {3} AND T2.TABLE_UID = T1.OBJECT_UID AND"
                + " T2.COLUMN_NUMBER = T1.COLUMN_NUMBER FOR READ UNCOMMITTED ACCESS;",
                new object[] { anExternalCatalogName, aSchemaVersion, aNotNullConstraintUID, aTableUID })
                );

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Select the list of indexes and the associated table or MV
        /// </summary>
        /// <param name="aConnection">an ODBC connection</param>
        /// <param name="aTrafodionSchema">a schema</param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectIndexesForSchema(Connection aConnection, TrafodionSchema aTrafodionSchema)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format("SELECT T.OBJECT_NAME, T.OBJECT_TYPE, IX.OBJECT_NAME, IX.OBJECT_UID, IX.CREATE_TIME, IX.REDEF_TIME, '' " +
                    "FROM {0}.\"_MD_\".OBJECTS as T, " +
                        "(SELECT AP.BASE_TABLE_UID, OB.OBJECT_NAME, OB.OBJECT_UID, OB.CREATE_TIME, OB.REDEF_TIME " +
                        "FROM {0}.\"_MD_\".OBJECTS as OB, " +
                        "{0}.\"_MD_\".INDEXES as AP " +
                        "WHERE OB.SCHEMA_NAME = '{2}' AND OB.OBJECT_TYPE = 'IX' " +
                        "AND OB.OBJECT_UID = AP.INDEX_UID " +
                        "FOR READ UNCOMMITTED ACCESS) AS IX " +
                    "WHERE T.SCHEMA_NAME= '{2}' AND T.OBJECT_UID = IX.BASE_TABLE_UID FOR READ UNCOMMITTED ACCESS;",
                new object[] { aTrafodionSchema.TheTrafodionCatalog.ExternalName, aTrafodionSchema.Version, aTrafodionSchema.InternalName })
                );

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Gets the name, UID, create time, and redef time of an index on a given object
        /// </summary>
        /// <param name="aConnection">an ODBC connection</param>
        /// <param name="aTrafodionCatalog">a TrafodionCatalog</param>
        /// <param name="aSchemaUID">the UID of a Schema</param>
        /// <param name="aTableUID">the UID of a Table</param>
        /// <param name="aSchemaVersion">the Version of Schema</param>
        /// <param name="aPathType">the Path Type</param>
        /// <param name="anObjectNameSpace">the two letter namespace of an object</param>
        /// <returns>The query returns a result set with one row per index</returns>
        static public OdbcDataReader ExecuteSelectIndexNames(Connection aConnection, TrafodionCatalog aTrafodionCatalog, String aSchemaName, long aTableUID)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format("SELECT T2.OBJECT_NAME, T2.OBJECT_UID, T2.CREATE_TIME, T2.REDEF_TIME, '' " +
                    "FROM {0}.\"_MD_\".INDEXES as T1, " +
                        "{0}.\"_MD_\".OBJECTS as T2 " +
                    "WHERE T1.BASE_TABLE_UID = {1} " +
                        "AND T2.SCHEMA_NAME = '{2}' " +
                        "AND T1.INDEX_UID = T2.OBJECT_UID " +
                    "FOR READ UNCOMMITTED ACCESS;",
                new object[] { aTrafodionCatalog.ExternalName, aTableUID, aSchemaName })
                );
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Selects an index given its name
        /// </summary>
        /// <param name="aConnection">an ODBC connection</param>
        /// <param name="aTrafodionCatalog">a TrafodionCatalog</param>
        /// <param name="aSchemaUID">the UID of a Schema</param>
        /// <param name="aTableUID">the UID of a Table</param>
        /// <param name="aSchemaVersion">the Version of Schema</param>
        /// <param name="aPathType">the Path Type</param>
        /// <param name="anObjectNameSpace">the two letter namespace of an object</param>
        /// <param name="anIndexName">The index name</param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectIndexByName(Connection aConnection, TrafodionCatalog aTrafodionCatalog, String aSchemaName, long aTableUID, int aSchemaVersion, String aPathType, String anObjectNameSpace, string anIndexName)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format("SELECT T2.OBJECT_NAME, T2.OBJECT_UID, T2.CREATE_TIME, T2.REDEF_TIME, '' " +
                    "FROM {0}.\"_MD_\".INDEXES as T1, " +
                        "{0}.\"_MD_\".OBJECTS as T2 " +
                    "WHERE T1.BASE_TABLE_UID = {1} " +
                        "AND T2.SCHEMA_NAME = '{2}' " +
                        "AND T1.INDEX_UID = T2.OBJECT_UID " +
                        "AND T2.OBJECT_NAME = '{3}' " +
                        "FOR READ UNCOMMITTED ACCESS;",
                new object[] { aTrafodionCatalog.ExternalName, aTableUID, aSchemaName, anIndexName })
                );
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }
        /// <summary>
        /// Gets the Columns Data of an Index
        /// </summary>
        /// <param name="aConnection">an ODBC connection</param>
        /// <param name="aTrafodionCatalog">a Trafodion Catalog</param>
        /// <param name="anIndex">an Index Object</param>
        /// <param name="aSchemaVersion">the version fo the schema</param>
        /// <returns>The query returns a result set with one row per column</returns>
        static public OdbcDataReader ExecuteSelectIndexColumnInfo(Connection aConnection, TrafodionCatalog aTrafodionCatalog, TrafodionIndex anIndex, int aSchemaVersion)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format(" SELECT T2.COLUMN_NAME, '','' " +
                "FROM {0}.\"_MD_\".INDEXES as T1, " +
                    "{0}.\"_MD_\".COLUMNS as T2 " +
                "WHERE T1.INDEX_UID = {1} AND T1.INDEX_UID = T2.OBJECT_UID AND T2.COLUMN_NUMBER > 0 AND T2.COLUMN_NAME <> '_SALT_'" +
                "ORDER BY T2.COLUMN_NAME FOR READ UNCOMMITTED ACCESS;",
                new object[] { aTrafodionCatalog.ExternalName, anIndex.UID, })
                );
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Gets the Attribute Data of an Index
        /// </summary>
        /// <param name="aConnection">an ODBC Connection</param>
        /// <param name="aTrafodionCatalog">a Trafodion Catalog</param>
        /// <param name="anIndex">an Index</param>
        /// <param name="aSchemaVersion">the version of the schema</param>
        /// <returns>The query returns a result set with one row of index attributes</returns>
        static public OdbcDataReader ExecuteSelectIndexAttributes(Connection aConnection, TrafodionCatalog aTrafodionCatalog, TrafodionIndex anIndex, int aSchemaVersion)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format(" SELECT  T1.AUDIT_COMPRESS, T1.BUFFERED, T1.DATA_COMPRESSED, T1.INDEX_COMPRESSED, T1.CLEAR_ON_PURGE, T1.BLOCK_SIZE, T1.KEY_LENGTH, T1.LOCK_LENGTH, T1.TABLE_UID, T1.UNIQUES, T1.EXPLICIT, T1.PARTITIONING_SCHEME, T1.AUDITED, T1.VALID_DATA, T1.ROW_FORMAT " +
                "FROM {0}."_MD_".ACCESS_PATHS as T1 " +
                "WHERE T1.ACCESS_PATH_UID = {2} FOR READ UNCOMMITTED ACCESS;",
                new object[] { aTrafodionCatalog.ExternalName, aSchemaVersion, anIndex.UID, })
                );
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }
        /// <summary>
        /// Gets the Attribute Data of an MaterializedView
        /// </summary>
        /// <param name="aConnection">an ODBC Connection</param>
        /// <param name="aTrafodionCatalog">a Trafodion Catalog</param>
        /// <param name="anMV">an Materialized View</param>
        /// <param name="aSchemaVersion">the version of the schema</param>
        /// <returns>The query returns a result set with one row of MV attributes</returns>
        static public OdbcDataReader ExecuteSelectMVAttributes(Connection aConnection, TrafodionCatalog aTrafodionCatalog, TrafodionMaterializedView anMV, int aSchemaVersion)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format(" SELECT  T1.AUDIT_COMPRESS, T1.CLEAR_ON_PURGE, T1.BLOCK_SIZE, T2.REFRESHED_AT, T2.COMMIT_REFRESH_EACH, T2.CREATION_REFRESH_TYPE, T2.MV_UID " +
                "FROM {0}."_MD_".ACCESS_PATHS as T1, " +
                "{0}."_MD_".MVS as T2 " +
                "WHERE T1.ACCESS_PATH_UID = T2.MV_UID AND " +
                "T2.MV_UID = {2} FOR READ UNCOMMITTED ACCESS;",
                new object[] { aTrafodionCatalog.ExternalName, aSchemaVersion, anMV.UID })
                );
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }


        /// <summary>
        /// Selects a list of Objects (Tables, Views, Procedures) used by this Trigger
        /// </summary>
        /// <param name="aConnection">The connection</param>
        /// <param name="anExternalCatalogName">The name of the catalog containing the definition schema</param>
        /// <param name="aSchemaVersion">The schema version</param>
        /// <param name="anObjectUID">The UID of the sql object</param>
        /// <returns>The objects and their Operation and Is Subject values</returns>
        static public OdbcDataReader ExecuteSelectTriggerUsages(Connection aConnection, String anExternalCatalogName, int aSchemaVersion, long anObjectUID)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format("SELECT T1.OBJECT_NAME, T1.OBJECT_TYPE, T1.SCHEMA_UID, T2.OPERATION, T2.IS_SUBJECT_TABLE" +
                " FROM {0}."_MD_".OBJECTS AS T1," +
                       "{0}."_MD_".TRIGGER_USED AS T2 " +
                " WHERE T2.TRIGGER_UID = {2} " +
                      "AND T2.USED_OBJECT_UID = T1.OBJECT_UID " +
                      "AND T1.OBJECT_NAME_SPACE = 'TA' " +
                      "AND T1.OBJECT_SECURITY_CLASS = 'UT' " +
                "FOR READ UNCOMMITTED ACCESS;",
                new object[] { anExternalCatalogName, aSchemaVersion, anObjectUID })
                );

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);

        }
        /// <summary>
        /// Select the routines used by the trigger
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aTrafodionTrigger"></param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectRoutinesUsedByTrigger(Connection aConnection, TrafodionTrigger aTrafodionTrigger)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format("SELECT T2.OBJECT_NAME AS NAME, T2.SCHEMA_UID , T1.OPERATION, T1.IS_SUBJECT_TABLE, T3.UDR_TYPE "
                    + "FROM {0}."_MD_".TRIGGER_USED AS T1, "
                    + "{0}."_MD_".OBJECTS AS T2 , "
                    + "{0}."_MD_".ROUTINES AS T3 "
                    + "WHERE T1.TRIGGER_UID = {2} "
                    + "AND T1.USED_OBJECT_UID = T2.OBJECT_UID "
                    + "AND T2.OBJECT_TYPE = 'UR' "
                    + "AND T1.USED_OBJECT_UID = T3.UDR_UID "
                    + "ORDER BY NAME FOR READ UNCOMMITTED ACCESS;",
                new object[] { aTrafodionTrigger.TheTrafodionCatalog.ExternalName, aTrafodionTrigger.TheTrafodionSchema.Version, aTrafodionTrigger.UID })
                );

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }
        /// <summary>
        /// Gets the list of triggers defined on this table 
        /// </summary>
        /// <param name="aConnection">The connection</param>
        /// <param name="aTrafodionCatalog">The catalog containing the definition schema</param>
        /// <param name="aSchemaVersion">The schema version</param>
        /// <param name="aTableUID">The UID of the table</param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectTriggerNames(Connection aConnection, TrafodionCatalog aTrafodionCatalog, int aSchemaVersion, long aTableUID)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format("SELECT T2.OBJECT_NAME, T1.TRIGGER_UID, T1.TRIGGER_CREATED, T2.REDEF_TIME, T1.ENABLED, T1.ACTIVATION_TIME, T1.OPERATION, T1.GRANULARITY, AUTHNAME(T2.OBJECT_OWNER) "
                + "FROM {0}."_MD_".TRIGGERS AS T1, "
                + "{0}."_MD_".OBJECTS as T2 "
                + "WHERE "
                + "T1.SUBJECT_UID = {2}  AND "
                + "T1.TRIGGER_UID = T2.OBJECT_UID "
                + "FOR READ UNCOMMITTED ACCESS;",
                new object[] { aTrafodionCatalog.ExternalName, aSchemaVersion, aTableUID })
                );
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);

        }

        /// <summary>
        /// Selects a trigger given its name
        /// </summary>
        /// <param name="aConnection">The connection</param>
        /// <param name="aTrafodionCatalog">The catalog containing the definition schema</param>
        /// <param name="aSchemaVersion">The schema version</param>
        /// <param name="aTableUID">The UID of the table</param>
        /// <param name="aTriggerName">The trigger name</param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectTriggerByName(Connection aConnection, TrafodionCatalog aTrafodionCatalog, int aSchemaVersion, long aTableUID, string aTriggerName)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format("SELECT T2.OBJECT_NAME, T1.TRIGGER_UID, T1.TRIGGER_CREATED, T2.REDEF_TIME, T1.ENABLED, T1.ACTIVATION_TIME, T1.OPERATION, T1.GRANULARITY, AUTHNAME(T2.OBJECT_OWNER) "
                + "FROM {0}."_MD_".TRIGGERS AS T1, "
                + "{0}."_MD_".OBJECTS as T2 "
                + "WHERE "
                + "T1.SUBJECT_UID = {2} AND "
                + "T2.OBJECT_NAME = '{3}' AND "
                + "T1.TRIGGER_UID = T2.OBJECT_UID "
                + "FOR READ UNCOMMITTED ACCESS;",
                new object[] { aTrafodionCatalog.ExternalName, aSchemaVersion, aTableUID, aTriggerName })
                );
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);

        }

        /// <summary>
        /// Gets the DDL for the sql object
        /// </summary>
        /// <param name="aConnection">A connection</param>
        /// <param name="aTrafodionObject">The Sql object whose DDL needs to be fetched</param>
        /// <returns>an OdbcdataReader</returns>
        static public OdbcDataReader ExecuteShowDDL(Connection aConnection, TrafodionObject aTrafodionObject)
        {
            OdbcCommand theQuery = new OdbcCommand();
            if (aTrafodionObject is TrafodionUDFunction || aTrafodionObject is TrafodionTableMappingFunction)
            {
                theQuery.CommandText = string.Format("{0} {1}{2}, PRIVILEGES", "SHOWDDL FUNCTION ", aTrafodionObject.RealAnsiName, "");
            }
            else if (aTrafodionObject is TrafodionFunctionAction)
            {
                theQuery.CommandText = string.Format("{0} {1} ACTION {2}{3}, PRIVILEGES", "SHOWDDL FUNCTION ", ((TrafodionFunctionAction)aTrafodionObject).TrafodionUDFunction.RealAnsiName, aTrafodionObject.RealAnsiName, "");
            }
            else if (aTrafodionObject is TrafodionLibrary)
            {
                theQuery.CommandText = string.Format("{0} {1}{2}, PRIVILEGES", "SHOWDDL LIBRARY ", ((TrafodionLibrary)aTrafodionObject).RealAnsiName, "");
            }
            else if (aTrafodionObject is TrafodionSequence)
            {
                theQuery.CommandText = string.Format("{0} {1}{2}, PRIVILEGES", "SHOWDDL SEQUENCE ", ((TrafodionSequence)aTrafodionObject).RealAnsiName, "");
            }
            else
            {
                theQuery.CommandText = string.Format("{0} {1}{2}, PRIVILEGES", aTrafodionObject is TrafodionSchema ? "SHOWDDL SCHEMA" : "SHOWDDL", aTrafodionObject.RealAnsiName, "");
            }
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Load the object that the synonym references. This could be either a table, mv or view
        /// The Object name and type is used to narrow down the object.
        /// </summary>
        /// <param name="aConnection">Connection</param>
        /// <param name="aTrafodionSynonym">Synonym</param>
        /// <returns>A data reader</returns>
        static public OdbcDataReader ExecuteGetSynonymReferenceObject(Connection aConnection, TrafodionSynonym aTrafodionSynonym)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format("SELECT T2.SCHEMA_UID, T2.OBJECT_NAME, T2.OBJECT_TYPE"
                + " FROM {0}."_MD_".SYNONYM_USAGE T1, {0}."_MD_".OBJECTS AS T2"
                + " WHERE T1.OBJECT_UID = {2} AND T1.USAGE_TYPE = -1 AND T1.UNDERLYING_SCH_UID = T2.SCHEMA_UID"
                + " AND T1.UNDERLYING_OBJ_UID = T2.OBJECT_UID FOR READ UNCOMMITTED ACCESS;",
                new object[] { aTrafodionSynonym.TheTrafodionCatalog.ExternalName, aTrafodionSynonym.TheTrafodionSchema.Version, aTrafodionSynonym.UID })
                );

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Gets the store order columns of a table
        /// </summary>
        /// <param name="aConnection">the connection</param>
        /// <param name="aTrafodionCatalog">the catalog</param>
        /// <param name="aSchemaVersion">the schema version</param>
        /// <param name="aTrafodionTable">the table</param>
        /// <returns>a Data Reader containing a list of store order columns</returns>
        static public OdbcDataReader ExecuteSelectStoreOrderColumnDefs(Connection aConnection, TrafodionCatalog aTrafodionCatalog,
            int aSchemaVersion, TrafodionTable aTrafodionTable)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format("SELECT T3.COLUMN_NAME, T2.POSITION_IN_ROW, T2.COLUMN_NUMBER, " +
                    "T2.ORDERING, T2.PART_KEY_SEQ_NUM, T2.CLUSTERING_KEY_SEQ_NUM, " +
                    "T2.SYSTEM_ADDED_COLUMN " +
                    "FROM {0}."_MD_".ACCESS_PATH_COLS AS T2, " +
                        "{0}."_MD_".COLS AS T3 " +
                    "WHERE T2.ACCESS_PATH_UID = {2} " +
                        "AND T2.ACCESS_PATH_UID = T3.OBJECT_UID " +
                        "AND (T2.CLUSTERING_KEY_SEQ_NUM > 0 OR T2.PART_KEY_SEQ_NUM > 0) " +
                        "AND T2.COLUMN_NUMBER = T3.COLUMN_NUMBER " +
                    "ORDER BY T2.CLUSTERING_KEY_SEQ_NUM " +
                    "FOR READ UNCOMMITTED ACCESS;",
                    new object[] { aTrafodionCatalog.ExternalName, aSchemaVersion, aTrafodionTable.UID })
                );
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Fetches the Division By key information for tables/MVs/Indexes
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aTrafodionSchemaObject"></param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectDivisionByColumnDefs(Connection aConnection, TrafodionSchemaObject aTrafodionSchemaObject)
        {
            OdbcCommand theQuery = new OdbcCommand();

            if (aTrafodionSchemaObject is TrafodionIndex)
            {
                theQuery.CommandText = String.Format("SELECT APC.DIVISION_KEY_SEQ_NUM AS SEQ_NUM, COLS.COLUMN_NUMBER AS COLNUM, " +
                                        "COLUMN_NAME, APC.ORDERING AS ORD, " +
                                        "TEXT.TEXT AS COMP_EXPRESSION, APC.SYSTEM_ADDED_COLUMN " +
                                        "FROM {0}."_MD_".COLS COLS " +
                                        "JOIN {0}."_MD_".ACCESS_PATHS AP " +
                                        "ON COLS.OBJECT_UID = AP.TABLE_UID " +
                                        "JOIN {0}."_MD_".ACCESS_PATH_COLS APC " +
                                        "ON AP.ACCESS_PATH_UID = APC.ACCESS_PATH_UID AND " +
                                        "COLS.COLUMN_NUMBER = APC.COLUMN_NUMBER " +
                                        "JOIN {0}."_MD_".TEXT TEXT " +
                                        "ON COLS.OBJECT_UID = TEXT.OBJECT_UID " +
                                        "AND TEXT.OBJECT_SUB_ID = 20000 + COLS.COLUMN_NUMBER " +
                                        "WHERE AP.ACCESS_PATH_UID = {1} " +
                                        "AND APC.DIVISION_KEY_SEQ_NUM > 0 " +
                                        "AND TEXT.SEQUENCE_NUM = 0 " +
                                        "ORDER BY 1" +
                                        "FOR READ UNCOMMITTED ACCESS;",
                                        aTrafodionSchemaObject.TheTrafodionCatalog.ExternalName, aTrafodionSchemaObject.UID);

            }
            else
            {
                theQuery.CommandText = String.Format("SELECT APC.DIVISION_KEY_SEQ_NUM AS SEQ_NUM, COLS.COLUMN_NUMBER AS COLNUM, " +
                                        "COLUMN_NAME, APC.ORDERING AS ORD, " +
                                        "TEXT.TEXT AS COMP_EXPRESSION, APC.SYSTEM_ADDED_COLUMN " +
                                        "FROM {0}."_MD_".COLS COLS " +
                                        "JOIN {0}."_MD_".ACCESS_PATHS AP " +
                                        "ON COLS.OBJECT_UID = AP.TABLE_UID " +
                                        "JOIN {0}."_MD_".ACCESS_PATH_COLS APC " +
                                        "ON AP.ACCESS_PATH_UID = APC.ACCESS_PATH_UID AND " +
                                        "COLS.COLUMN_NUMBER = APC.COLUMN_NUMBER " +
                                        "JOIN {0}."_MD_".TEXT TEXT " +
                                        "ON COLS.OBJECT_UID = TEXT.OBJECT_UID " +
                                        "AND TEXT.OBJECT_SUB_ID = 20000 + COLS.COLUMN_NUMBER " +
                                        "WHERE COLS.OBJECT_UID = {1} " +
                                        "AND AP.ACCESS_PATH_UID = COLS.OBJECT_UID " +
                                        "AND AP.ACCESS_PATH_TYPE = 'BT' " +
                                        "AND APC.DIVISION_KEY_SEQ_NUM > 0 " +
                                        "AND TEXT.SEQUENCE_NUM = 0 " +
                                        "ORDER BY 1 FOR READ UNCOMMITTED ACCESS;",
                                        aTrafodionSchemaObject.TheTrafodionCatalog.ExternalName, aTrafodionSchemaObject.UID);
            }
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Select the hash key details from metadata
        /// </summary>
        /// <param name="aConnection">Connection</param>
        /// <param name="aTrafodionCatalog">a catalog</param>
        /// <param name="aSchemaVersion">Schema version</param>
        /// <param name="aTrafodionTable">a Table</param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectHashKeyColumnInfo(Connection aConnection, TrafodionCatalog aTrafodionCatalog,
            int aSchemaVersion, TrafodionTable aTrafodionTable)
        {
            string theQueryText = String.Format("SELECT T2.COLUMN_NAME, T2.COLUMN_NUMBER, T1.POSITION_IN_ROW, T1.PART_KEY_SEQ_NUM, T1.ORDERING"
                + " FROM {0}."_MD_".ACCESS_PATH_COLS AS T1, "
                + " {0}."_MD_".COLS AS T2"
                + " WHERE T1.ACCESS_PATH_UID = {2} AND"
                + " T1.ACCESS_PATH_UID = T2.OBJECT_UID AND"
                + " T1.PART_KEY_SEQ_NUM > 0 AND"
                + " T1.COLUMN_NUMBER = T2.COLUMN_NUMBER"
                + " ORDER BY T1.PART_KEY_SEQ_NUM FOR READ UNCOMMITTED ACCESS;",
                new object[] { aTrafodionCatalog.ExternalName, aSchemaVersion, aTrafodionTable.UID });

            OdbcCommand theQuery = new OdbcCommand(theQueryText);
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// This query is used to get the column names and number for a constraint on a table
        /// </summary>
        /// <param name="aConnection">the connection</param>
        /// <param name="aTrafodionCatalogExternalName">the catalog's external name</param>
        /// <param name="aSchemaVersion">the version of the schema</param>
        /// <param name="aConstraintUID">the UID of the constraint</param>
        /// <param name="aTrafodionTableUID">the UID of the table</param>
        /// <returns>a list of column names and numbers</returns>
        static public OdbcDataReader ExecuteSelectConstraintColumnName(Connection aConnection, string aTrafodionCatalogExternalName,
            int aSchemaVersion, long aConstraintUID, long aTrafodionTableUID)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format("SELECT T1.COLUMN_NAME, T1.COLUMN_NUMBER " +
                    "FROM {0}."_MD_".COLS AS T1, " +
                        "{0}."_MD_".KEY_COL_USAGE AS T2 " +
                    "WHERE T2.CONSTRAINT_UID = {2} " +
                        "AND T1.OBJECT_UID = {3} " +
                        "AND T1.COLUMN_NUMBER = T2.COLUMN_NUMBER " +
                    "ORDER BY T2.ORDINAL_POSITION " +
                    "FOR READ UNCOMMITTED ACCESS;",
                    new object[] { aTrafodionCatalogExternalName, aSchemaVersion, aConstraintUID, aTrafodionTableUID })
                    );
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// This query gets the unique constraint UID of a foreign key constraint as well as the catalog, schema, and schema version of the foreign table
        /// </summary>
        /// <param name="aConnection">the Connection</param>
        /// <param name="aTrafodionCatalogExternalName">The external catalog name</param>
        /// <param name="aSchemaVersion">The version of the schema</param>
        /// <param name="aConstraintUID">the UID of the constraint</param>
        /// <returns>a datareader</returns>
        static public OdbcDataReader ExecuteSelectForeignKeyUniqueConstraint(Connection aConnection, string aTrafodionCatalogExternalName,
            int aSchemaVersion, long aConstraintUID)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format("SELECT T1.CAT_UID, T2.SCHEMA_UID, T3.UNIQUE_CONSTRAINT_UID " +
                    "FROM {0}.SYSTEM_SCHEMA.CATSYS AS T1, " +
                        "{0}.SYSTEM_SCHEMA.SCHEMATA AS T2, " +
                        "{1}."_MD_".REF_CONSTRAINTS AS T3 " +
                    "WHERE T3.CONSTRAINT_UID = {3} " +
                        "AND T2.SCHEMA_UID = T3.UNIQUE_CONSTRAINT_SCH_UID " +
                        "AND T1.CAT_UID = T3.UNIQUE_CONSTRAINT_CAT_UID " +
                        "FOR READ UNCOMMITTED ACCESS;",
                        new object[] { aConnection.SystemCatalogName, aTrafodionCatalogExternalName, aSchemaVersion, aConstraintUID })
                        );
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// This query is used to get the name and UID of a foreign table that uses a certain constraint
        /// </summary>
        /// <param name="aConnection">the connection</param>
        /// <param name="aTrafodionCatalogExternalName">the catalog's external name</param>
        /// <param name="aSchemaVersion">the schema version</param>
        /// <param name="aConstraintUID">the UID of the constraint</param>
        /// <returns>a Data Reader</returns>
        static public OdbcDataReader ExecuteSelectConstraintForeignTable(Connection aConnection, string aTrafodionCatalogExternalName,
            int aSchemaVersion, long aConstraintUID)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format("SELECT T2.OBJECT_NAME, T2.OBJECT_UID " +
                    "FROM {0}."_MD_".TBL_CONSTRAINTS AS T1, " +
                    "{0}."_MD_".OBJECTS AS T2 " +
                    "WHERE T1.CONSTRAINT_UID =  {2} " +
                    "AND T1.TABLE_UID = T2.OBJECT_UID " +
                    "FOR READ UNCOMMITTED ACCESS;",
                    new object[] { aTrafodionCatalogExternalName, aSchemaVersion, aConstraintUID })
                    );
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// This query gets the text of a constraint
        /// </summary>
        /// <param name="aConnection">the Connection</param>
        /// <param name="aTrafodionCatalogExternalName">the catalog's external name</param>
        /// <param name="aSchemaVersion">the schema version</param>
        /// <param name="aTrafodionTableUID">the UID of the table</param>
        /// <param name="aConstraintType">the constraint type</param>
        /// <returns>a Data Reader</returns>
        static public OdbcDataReader ExecuteSelectConstraintText(Connection aConnection, string aTrafodionCatalogExternalName,
            int aSchemaVersion, long aTrafodionTableUID, char aConstraintType)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format("SELECT T2.OBJECT_NAME, T3.TEXT " +
                    "FROM {0}."_MD_".TBL_CONSTRAINTS  AS T1, " +
                        "{0}."_MD_".OBJECTS AS T2, " +
                        "{0}."_MD_".TEXT AS T3 " +
                    "WHERE T1.TABLE_UID = {3} AND " +
                        "T1.CONSTRAINT_TYPE = '{4}' AND " +
                        "T1.CONSTRAINT_UID = T2.OBJECT_UID AND " +
                        "T3.OBJECT_UID = T1.CONSTRAINT_UID " +
                        "ORDER BY T3.SEQUENCE_NUM " +
                    "FOR READ UNCOMMITTED ACCESS;",
                    new object[] { aTrafodionCatalogExternalName, aConnection.SystemCatalogName, aSchemaVersion, aTrafodionTableUID, aConstraintType })
                    );
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Gets the memebers of a MV Group 
        /// </summary>
        /// <param name="aConnection">an ODBC Connection</param>
        /// <param name="anExternalCatalogName">The External name of the Catalog </param>
        /// <param name="aSchemaVersion">the version of the schema</param>
        /// <param name="aMVGroupUID">the UID of the MVGroup </param>
        /// <returns>The query returns a result set with rows of MVs in the MVGroup</returns>
        /// 
        static public OdbcDataReader ExecuteSelectMVGroupMembers(Connection aConnection, String anExternalCatalogName, int aSchemaVersion, long aMVGroupUID)
        {

            // retrieve MV-Name, schema-UID & CAT-UID to be able to map in the Client
            OdbcCommand theQuery = new OdbcCommand(
                String.Format(" SELECT TRIM(OBJECT_NAME) AS MV_NAME, MV_SCHEMA_UID, MV_CATALOG_UID " +
                        " FROM {0}."_MD_".MVGROUPS  MVG, {0}."_MD_".OBJECTS O " +
                        " WHERE  MVG.MVRGROUP_UID = {2}  AND MVG.MV_UID = O.OBJECT_UID " +
                        " ORDER BY  MV_NAME  FOR READ UNCOMMITTED ACCESS;",
                        new object[] { anExternalCatalogName, aSchemaVersion, aMVGroupUID }));
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Gets DDL Text for MV
        /// </summary>
        /// <param name="aConnection">an ODBC Connection</param>
        /// <param name="anExternalCatalogName">The External name of the Catalog </param>
        /// <param name="aSchemaVersion">the version of the schema</param>
        /// <param name="aMVUID">the UID of the MV </param>
        /// <returns>The query returns a result set with rows of DDL Text for MV</returns>
        /// 
        static public OdbcDataReader ExecuteSelectMVDDLText(Connection aConnection, String anExternalCatalogName, int aSchemaVersion, long aMVUID)
        {

            OdbcCommand theQuery = new OdbcCommand(
                String.Format(" SELECT TEXT " +
                        "FROM {0}."_MD_".TEXT " +
                        "WHERE OBJECT_UID = {2}" +
                        "ORDER BY SEQUENCE_NUM FOR READ UNCOMMITTED ACCESS;",
                        new object[] { anExternalCatalogName, aSchemaVersion, aMVUID }));
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Gets the privileges for a schema.
        /// </summary>
        /// <param name="aConnection">The ODBC connection to query.</param>
        /// <param name="aTrafodionSchema">The schema whose privileges are being looked up.</param>
        /// <returns>A resultset with the following columns sorted by Grantee, Is_grantable, and Grantor: 
        /// { GRANTOR, GRANTOR_TYPE, GRANTEE, GRANTEE_TYPE, PRIVILEGE_TYPE, IS_GRANTABLE }</returns>
        static public OdbcDataReader ExecuteSelectSchemaPrivileges(Connection aConnection, TrafodionSchema aTrafodionSchema)
        {

            if (aTrafodionSchema.Version < 2000)
            {
                throw new ApplicationException(Properties.Resources.Error_PrivilegeIsNotSupportedForSchemaVersion);
            }

            string queryString = String.Format("SELECT GRANTOR, AUTHNAME(GRANTOR), GRANTOR_TYPE, GRANTEE, AUTHNAME(GRANTEE), GRANTEE_TYPE, PRIVILEGE_TYPE, IS_GRANTABLE " +
                            "FROM {0}."_MD_".OBJECTS O, {0}."_MD_".SCH_PRIVILEGES S " +
                            "WHERE O.SCHEMA_UID = {2} AND O.OBJECT_TYPE = 'SL' AND S.{3} = O.OBJECT_UID " +
                            "ORDER BY GRANTEE, IS_GRANTABLE, GRANTOR " +
                            "FOR READ UNCOMMITTED ACCESS;",
                            new object[] { aTrafodionSchema.TheTrafodionCatalog.ExternalName, aTrafodionSchema.Version, aTrafodionSchema.UID,
                                ((aTrafodionSchema.Version < 2300)? "SCHEMA_UID" : "TABLE_UID")});

            OdbcCommand theQuery = new OdbcCommand(queryString);
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Gets the privileges for a schema object.
        /// </summary>
        /// <param name="aConnection">The ODBC connection to use.</param>
        /// <param name="aSchema">The object's schema.</param>
        /// <param name="aSchemaObject">The object whose privileges are to be looked up.</param>
        /// <returns>The query returns a result set with the following columns sorted by Grantee, Is_grantable, and Grantor:
        /// { GRANTOR, GRANTOR_TYPE, GRANTEE, GRANTEE_TYPE, PRIVILEGE_TYPE, IS_GRANTABLE }</returns>
        static public OdbcDataReader ExecuteSelectSchemaObjectPrivileges(Connection aConnection, TrafodionSchema aSchema, TrafodionObject aSchemaObject)
        {
            if (aSchema.Version < 2000)
            {
                throw new ApplicationException(Properties.Resources.Error_PrivilegeIsNotSupportedForSchemaVersion);
            }

            // The sort order is VERY important. Do not mess with it!
            OdbcCommand theQuery = new OdbcCommand(
                String.Format("SELECT GRANTOR, AUTHNAME(GRANTOR), GRANTOR_TYPE, GRANTEE, AUTHNAME(GRANTEE), GRANTEE_TYPE, PRIVILEGE_TYPE, IS_GRANTABLE " +
                "FROM {0}."_MD_".TBL_PRIVILEGES " +
                "WHERE TABLE_UID = {2} " +
                "ORDER BY GRANTEE, IS_GRANTABLE, GRANTOR " +
                "FOR READ UNCOMMITTED ACCESS;",
                new object[] { aSchema.TheTrafodionCatalog.ExternalName, aSchema.Version, aSchemaObject.UID })
                );
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Gets the privileges for a table's columns.
        /// </summary>
        /// <param name="aConnection">The ODBC connection to use.</param>
        /// <param name="aSchema">The table's schema.</param>
        /// <param name="aSchemaObject">The schema object whose column privileges are to be looked up.</param>
        /// <returns>The query returns a result set with the following columns sorted by Grantee, Is_grantable, and Grantor:
        /// { COLUMN_NAME, COLUMN_CLASS, COLUMN_NUMBER, GRANTOR, GRANTOR_TYPE, GRANTEE, GRANTEE_TYPE, PRIVILEGE_TYPE, IS_GRANTABLE }</returns>
        static public OdbcDataReader ExecuteSelectColumnPrivileges(Connection aConnection, TrafodionSchema aSchema, TrafodionSchemaObject aSchemaObject)
        {
            if (aSchema.Version < 2000)
            {
                throw new ApplicationException(Properties.Resources.Error_PrivilegeIsNotSupportedForSchemaVersion);
            }

            // The sort order is VERY important. Do not mess with it!
            OdbcCommand theQuery = new OdbcCommand(
                String.Format("SELECT C.COLUMN_NAME, C.COLUMN_CLASS, C.COLUMN_NUMBER, CP.GRANTOR, AUTHNAME(CP.GRANTOR), CP.GRANTOR_TYPE, CP.GRANTEE, AUTHNAME(CP.GRANTEE), CP.GRANTEE_TYPE, CP.PRIVILEGE_TYPE, CP.IS_GRANTABLE " +
                "FROM {0}."_MD_".COLS AS C, {0}."_MD_".COL_PRIVILEGES AS CP " +
                "WHERE CP.TABLE_UID = {2} AND C.OBJECT_UID = CP.TABLE_UID AND C.COLUMN_NUMBER = CP.COLUMN_NUMBER " + //AND CP.GRANTEE_TYPE = 'U' " +
                "ORDER BY CP.GRANTEE, CP.IS_GRANTABLE, CP.GRANTOR, C.COLUMN_NUMBER " +
                "FOR READ UNCOMMITTED ACCESS;",
                new object[] { aSchema.TheTrafodionCatalog.ExternalName, aSchema.Version, aSchemaObject.UID })
                );
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Returns the number of columns that exists under a particular schema object.
        /// </summary>
        /// <param name="aConnection">The ODBC connection to use.</param>
        /// <param name="aSchema">The schema in which the schema object resides.</param>
        /// <param name="aSchemaObject">The schema object whose column count is to be looked up.</param>
        /// <returns>The query returns a result set with one row and one column. It represents the number of columns.</returns>
        static public OdbcDataReader ExecuteSelectColumnCount(Connection aConnection, TrafodionSchema aSchema, TrafodionSchemaObject aSchemaObject)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format(
                "SELECT count(COLUMN_NAME) " +
                "FROM {0}."_MD_".COLS " +
                "WHERE OBJECT_UID = {2} " +
                "FOR READ UNCOMMITTED ACCESS;",
                new object[] { aSchema.TheTrafodionCatalog.ExternalName, aSchema.Version, aSchemaObject.UID })
                );
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Gets the routines defined in the schema
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aTrafodionSchema"></param>
        /// <param name="anObjectNameSpace"></param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectRoutines(Connection aConnection, TrafodionSchema aTrafodionSchema, string anObjectNameSpace)
        {
            //Is Universal column is only available in schema version 2500 and above
            string column7 = aTrafodionSchema.Version >= 2500 ? "T2.IS_UNIVERSAL" : "Y";

            OdbcCommand theQuery = new OdbcCommand(String.Format("SELECT T1.OBJECT_NAME, T1.OBJECT_UID, "
                + " T1.CREATE_TIME, T1.REDEF_TIME, T2.UDR_TYPE, T2.UDR_TYPE, AUTHNAME(T1.OBJECT_OWNER), 'Y' "
                + " FROM {0}.\"_MD_\".OBJECTS T1"
                + " , {0}.\"_MD_\".ROUTINES T2"
                + " WHERE T1.SCHEMA_NAME = '{2}' "
                + " AND T1.OBJECT_TYPE = 'UR'"
                 + " AND T1.OBJECT_UID = T2.UDR_UID "
                + " ORDER BY T1.OBJECT_NAME FOR READ UNCOMMITTED ACCESS;",
                new object[] { aTrafodionSchema.TheTrafodionCatalog.ExternalName, aTrafodionSchema.Version, aTrafodionSchema.InternalName, anObjectNameSpace, column7 }));

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }


        /// <summary>
        /// Gets the routines defined in the schema
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aTrafodionSchema"></param>
        /// <param name="anObjectNameSpace"></param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectLibraries(Connection aConnection, TrafodionSchema aTrafodionSchema, string anObjectNameSpace)
        {
            const string SELECT_LIBRARIES = "SELECT TObj.OBJECT_NAME, TObj.OBJECT_UID, TObj.CREATE_TIME, TObj.REDEF_TIME, " +
                                                "'', AUTHNAME(TObj.OBJECT_OWNER), " +
                                                "TLib.LIBRARY_FILENAME, '','' " +
                                                "FROM    {0}.\"_MD_\".OBJECTS TObj, " +
                                                       "{0}.\"_MD_\".LIBRARIES TLib " +
                                                "WHERE TObj.SCHEMA_NAME = '{1}' AND " +
                                                       "TObj.OBJECT_TYPE = '{2}' AND " +
                                                       "TObj.OBJECT_UID = TLib.LIBRARY_UID " +
                                               "ORDER BY TObj.OBJECT_NAME FOR READ UNCOMMITTED ACCESS;";

            string commandText = String.Format(SELECT_LIBRARIES,
                aTrafodionSchema.TheTrafodionCatalog.ExternalName,
                aTrafodionSchema.InternalName,
                anObjectNameSpace);


#if DEBUG
            Console.WriteLine(commandText);
#endif

            OdbcCommand command = new OdbcCommand(commandText);
            command.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(command);
        }

        /// <summary>
        /// Gets the Library's information
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aTrafodionSchema"></param>
        /// <param name="aLibraryName"></param>
        /// <param name="anObjectNameSpace"></param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectLibraryByName(Connection aConnection, TrafodionSchema aTrafodionSchema, string aLibraryName, string anObjectNameSpace)
        {
            const string SELECT_LIBRARY = "SELECT TObj.OBJECT_NAME, TObj.OBJECT_UID, TObj.CREATE_TIME, TObj.REDEF_TIME, " +
                                                "TObj.OBJECT_SECURITY_CLASS, AUTHNAME(TObj.OBJECT_OWNER), " +
                                                "TLib.LIBRARY_FILENAME, '', '' " +
                                                "FROM    {0}.\"_MD_\".OBJECTS TObj, " +
                                                       "{0}.\"_MD_\".LIBRARIES TLib " +
                                                "WHERE TObj.SCHEMA_NAME = '{1}' AND " +
                                                       "TObj.OBJECT_TYPE = '{2}' AND TObj.OBJECT_NAME = '{3}' AND " +
                                                       "TObj.OBJECT_UID = TLib.LIBRARY_UID " +
                                               "ORDER BY TObj.OBJECT_NAME FOR READ UNCOMMITTED ACCESS;";

            string commandText = String.Format(SELECT_LIBRARY,
                aTrafodionSchema.TheTrafodionCatalog.ExternalName,
                aTrafodionSchema.InternalName,
                anObjectNameSpace,
                aLibraryName );


#if DEBUG
            Console.WriteLine(commandText);
#endif

            OdbcCommand command = new OdbcCommand(commandText);
            command.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(command);

        }

        /// <summary>
        /// Gets the libraries client/client file name list
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aTrafodionSchema"></param>
        /// <param name="anObjectNameSpace"></param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectLibrariesClient(Connection aConnection, TrafodionSchema aTrafodionSchema, string anObjectNameSpace)
        {
            const string SELECT_LIBCLIENT = @"  SELECT DISTINCT TLib.CLIENT_NAME, TLib.CLIENT_FILENAME 
                                                FROM    {0}."_MD_".ROUTINE_LIBS TLib 
                                                ORDER BY TObj.OBJECT_NAME FOR READ UNCOMMITTED ACCESS;";
            string commandText = String.Format(SELECT_LIBCLIENT,
                aTrafodionSchema.TheTrafodionCatalog.ExternalName);
#if DEBUG
            Console.WriteLine(commandText);
#endif

            OdbcCommand command = new OdbcCommand(commandText);
            command.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(command);
        }
        /// <summary>
        /// Gets the routine's information give the routine name
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aTrafodionSchema"></param>
        /// <param name="aRoutineName"></param>
        /// <param name="anObjectNameSpace"></param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectRoutineByName(Connection aConnection, TrafodionSchema aTrafodionSchema, string aRoutineName, string anObjectNameSpace)
        {

            //Is Universal column is only available in schema version 2500 and above
            string column7 = aTrafodionSchema.Version >= 2500 ? "T2.IS_UNIVERSAL" : "''";

            OdbcCommand theQuery = new OdbcCommand(String.Format("SELECT T1.OBJECT_NAME, T1.OBJECT_UID, "
                + " T1.CREATE_TIME, T1.REDEF_TIME, '',  T2.UDR_TYPE, AUTHNAME(T1.OBJECT_OWNER), {5} "
                + " FROM {0}.\"_MD_\".OBJECTS T1"
                + " , {0}.\"_MD_\".ROUTINES T2"
                + " WHERE T1.SCHEMA_NAME = '{2}' "
                + " AND T1.OBJECT_NAME = '{3}'"
                + " AND T1.OBJECT_TYPE = 'UR'"
                + " AND T1.OBJECT_UID = T2.UDR_UID "
                + " ORDER BY T1.OBJECT_NAME FOR READ UNCOMMITTED ACCESS;",
                new object[] { aTrafodionSchema.TheTrafodionCatalog.ExternalName, aTrafodionSchema.Version, aTrafodionSchema.InternalName, aRoutineName, anObjectNameSpace, column7 }));

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);

        }

        /// <summary>
        /// Select the function actions in a schema
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aTrafodionSchema"></param>
        /// <param name="anObjectNameSpace"></param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectFunctionActions(Connection aConnection, TrafodionSchema aTrafodionSchema)
        {
            OdbcCommand theQuery = new OdbcCommand(String.Format("SELECT T1.OBJECT_NAME, T1.OBJECT_UID, "
                + " T1.CREATE_TIME, T1.REDEF_TIME, T1.OBJECT_SECURITY_CLASS, T2.UUDF_SCH_UID, T2.UUDF_UID, AUTHNAME(T1.OBJECT_OWNER) "
                + " FROM {0}."_MD_".OBJECTS T1"
                + " , {0}."_MD_".ROUTINE_ACTIONS T2"
                + " WHERE T1.SCHEMA_UID = {2} "
                + " AND T1.OBJECT_TYPE = 'UR'"
                + " AND T1.OBJECT_NAME_SPACE = 'AC' "
                + " AND T1.OBJECT_UID = T2.ACTION_UID "
                + " ORDER BY T1.OBJECT_NAME FOR READ UNCOMMITTED ACCESS;",
                new object[] { aTrafodionSchema.TheTrafodionCatalog.ExternalName, aTrafodionSchema.Version, aTrafodionSchema.UID }));

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        static public OdbcDataReader ExecuteSelectActionByName(Connection aConnection, TrafodionSchema aTrafodionSchema, string aActionName)
        {
            OdbcCommand theQuery = new OdbcCommand(String.Format("SELECT T1.OBJECT_NAME, T1.OBJECT_UID, "
                + " T1.CREATE_TIME, T1.REDEF_TIME, T1.OBJECT_SECURITY_CLASS, T2.UUDF_SCH_UID, T2.UUDF_UID, AUTHNAME(T1.OBJECT_OWNER) "
                + " FROM {0}."_MD_".OBJECTS T1"
                + " , {0}."_MD_".ROUTINE_ACTIONS T2"
                + " WHERE T1.SCHEMA_UID = {2} "
                + " AND T1.OBJECT_TYPE = 'UR'"
                + " AND T1.OBJECT_NAME_SPACE = 'AC' "
                + " AND T1.OBJECT_UID = T2.ACTION_UID "
                + " AND T1.OBJECT_NAME = '{3}'"
                + " ORDER BY T1.OBJECT_NAME FOR READ UNCOMMITTED ACCESS;",
                new object[] { aTrafodionSchema.TheTrafodionCatalog.ExternalName, aTrafodionSchema.Version, aTrafodionSchema.UID, aActionName }));

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Select the function actions in a schema
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aTrafodionSchema"></param>
        /// <param name="anObjectNameSpace"></param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectFunctionActionByName(Connection aConnection, TrafodionSchema aTrafodionSchema, string objectName)
        {
            OdbcCommand theQuery = new OdbcCommand(String.Format("SELECT T1.OBJECT_NAME, T1.OBJECT_UID, "
                + " T1.CREATE_TIME, T1.REDEF_TIME, T1.OBJECT_SECURITY_CLASS, T2.UUDF_SCH_UID, T2.UUDF_UID "
                + " FROM {0}."_MD_".OBJECTS T1"
                + " , {0}."_MD_".ROUTINE_ACTIONS T2"
                + " WHERE T1.SCHEMA_UID = {2} "
                + " AND T1.OBJECT_NAME = '{3}'"
                + " AND T1.OBJECT_TYPE = 'UR'"
                + " AND T1.OBJECT_NAME_SPACE = 'AC' "
                + " AND T1.OBJECT_UID = T2.ACTION_UID "
                + " ORDER BY T1.OBJECT_NAME FOR READ UNCOMMITTED ACCESS;",
                new object[] { aTrafodionSchema.TheTrafodionCatalog.ExternalName, aTrafodionSchema.Version, aTrafodionSchema.UID, objectName }));

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }
        /// <summary>
        /// Gets the Attribute Data of a Procedure
        /// </summary>
        /// <param name="aConnection">an ODBC Connection</param>
        /// <param name="aRoutine">a Routine</param>
        /// <returns>The query returns a result set with one row of procedure attributes</returns>
        static public OdbcDataReader ExecuteSelectRoutineAttributes(Connection aConnection, TrafodionRoutine aRoutine)
        {
            OdbcCommand theQuery = new OdbcCommand();

                theQuery.CommandText = String.Format("SELECT UDR_TYPE, LANGUAGE_TYPE,DETERMINISTIC_BOOL, SQL_ACCESS, ISOLATE_BOOL, PARAM_STYLE, TRANSACTION_ATTRIBUTES, MAX_RESULTS, '', '', '',EXTERNAL_NAME, ISNULL(TEXT,'')  , " +
                    "CALL_ON_NULL, 0, 0, 0, 0, 0, 0, 0, 'Y', PARALLELISM, 'Y', USER_VERSION, EXTERNAL_SECURITY, 0, EXECUTION_MODE,LIBRARY_UID " +
                                    "FROM {0}.\"_MD_\".ROUTINES as T1, " +
                                           "{0}.\"_MD_\".OBJECTS as T2 " +
                                           "LEFT JOIN {0}.\"_MD_\".TEXT as T3 " +
                                           "ON T2.OBJECT_UID = T3.TEXT_UID " +
                                    "WHERE T1.UDR_UID = {2} AND " +
                                           " T2.OBJECT_UID = T1.UDR_UID " +
                                           " ORDER BY T3.SEQ_NUM " +
                                    "FOR READ UNCOMMITTED ACCESS;",
                                    new object[] { aRoutine.TheTrafodionCatalog.ExternalName, aRoutine.TheTrafodionSchema.Version, aRoutine.UID });



            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);

        }


        /// <summary>
        /// Fetch the table statistics from the histograms table for schema version 2300 and later
        /// </summary>
        /// <param name="aConnection">The Connection object</param>
        /// <param name="aTrafodionTable">The sql table</param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectTableStatistics2300(Connection aConnection, PartitionedSchemaObject aTrafodionTable)
        {
            OdbcCommand odbcCommand = new OdbcCommand(
                String.Format("SELECT T1.COLUMN_NUMBER, ISNULL(T2.INTERVAL_ROWCOUNT,0), T1.LOW_VALUE, T1.HIGH_VALUE, T1.CV, " +
                "T1.TOTAL_UEC, T1.ROWCOUNT, T1.STATS_TIME " +
                "FROM (SELECT TABLE_UID, HISTOGRAM_ID, COLUMN_NUMBER, LOW_VALUE, HIGH_VALUE, CV, TOTAL_UEC, ROWCOUNT, " +
                "JULIANTIMESTAMP(STATS_TIME) AS STATS_TIME FROM {0}.{1}.HISTOGRAMS WHERE TABLE_UID = {2} AND COLCOUNT = 1) AS T1 " +
                "LEFT JOIN {0}.{1}.HISTOGRAM_INTERVALS T2 ON (T1.TABLE_UID = T2.TABLE_UID AND T1.HISTOGRAM_ID = T2.HISTOGRAM_ID " +
                " AND T2.INTERVAL_BOUNDARY = '(NULL)') ORDER BY COLUMN_NUMBER FOR READ UNCOMMITTED ACCESS;", aTrafodionTable.TheTrafodionCatalog.ExternalName,
                aTrafodionTable.TheTrafodionSchema.ExternalName, aTrafodionTable.UID)
                );
#if DEBUG
            Console.WriteLine(odbcCommand.CommandText);
#endif

            odbcCommand.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(odbcCommand);
        }

        /// <summary>
        /// Fetch the table statistics from the histograms table for schema version 2000 and earlier
        /// </summary>
        /// <param name="aConnection">The connection object</param>
        /// <param name="aTrafodionTable">Sql Table model</param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectTableStatistics2000(Connection aConnection, PartitionedSchemaObject aTrafodionTable)
        {
            OdbcCommand odbcCommand = new OdbcCommand(
                String.Format("SELECT T1.COLUMN_NUMBER, ISNULL(T2.INTERVAL_UEC,0), T1.LOW_VALUE, T1.HIGH_VALUE, " +
                "T1.TOTAL_UEC, T1.ROWCOUNT, T1.STATS_TIME " +
                "FROM (SELECT TABLE_UID, HISTOGRAM_ID, COLUMN_NUMBER, LOW_VALUE, HIGH_VALUE, TOTAL_UEC, ROWCOUNT, " +
                "JULIANTIMESTAMP(STATS_TIME) AS STATS_TIME FROM {0}.{1}.HISTOGRAMS WHERE TABLE_UID = {2} AND COLCOUNT = 1) AS T1 " +
                "LEFT JOIN {0}.{1}.HISTOGRAM_INTERVALS T2 ON (T1.TABLE_UID = T2.TABLE_UID AND T1.HISTOGRAM_ID = T2.HISTOGRAM_ID " +
                " AND T2.INTERVAL_BOUNDARY = '(NULL)') ORDER BY COLUMN_NUMBER FOR READ UNCOMMITTED ACCESS;", aTrafodionTable.TheTrafodionCatalog.ExternalName,
                aTrafodionTable.TheTrafodionSchema.ExternalName, aTrafodionTable.UID)
                );
#if DEBUG
            Console.WriteLine(odbcCommand.CommandText);
#endif

            odbcCommand.Connection = aConnection.OpenOdbcConnection;
            odbcCommand.CommandTimeout = 0;
            return ExecuteReader(odbcCommand);
        }

        /// <summary>
        /// Creates a temporary table to hold the sampled data to compute the table column statistics
        /// </summary>
        /// <param name="aOdbcConnection">Odbc connection</param>
        /// <param name="tempTableName">The temporary table name that will hold the sampled data</param>
        /// <param name="aTrafodionTable">The table name for which the sample was created</param>
        /// <param name="columnName">Column Name</param>
        /// <param name="samplesClause">Sample sql clause</param>
        /// <returns></returns>
        static public int ExecuteCreateTempStatsTable(OdbcConnection aOdbcConnection, string tempTableName, PartitionedSchemaObject aTrafodionTable, string columnName, string samplesClause)
        {
            OdbcCommand odbcCommand = new OdbcCommand(
                String.Format("CREATE VOLATILE TABLE {0} AS SELECT RUNNINGCOUNT({1}) AS SEQNO, HASH2PARTFUNC({1} for {2}) AS P_{0}, {1} FROM {3} {4} SEQUENCE BY {1} FOR READ UNCOMMITTED ACCESS;",
                    tempTableName, columnName, aTrafodionTable.Partitions.Count, aTrafodionTable.RealAnsiName, samplesClause)
                );
#if DEBUG
            Console.WriteLine(odbcCommand.CommandText);
#endif
            odbcCommand.Connection = aOdbcConnection;
            odbcCommand.CommandTimeout = 0;
            return ExecuteNonQuery(odbcCommand);
        }

        /// <summary>
        /// Selects the skew and number of nulls from the temporary table that holds the sampled data for the table column
        /// </summary>
        /// <param name="aOdbcConnection">Odbc connection</param>
        /// <param name="tempTableName">Temporary table that holds the sample data</param>
        /// <param name="columnName">Column name</param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectSampledTableColumnStats(OdbcConnection aOdbcConnection, string tempTableName, string columnName)
        {
            OdbcCommand odbcCommand = new OdbcCommand(
                String.Format("SELECT ISNULL(CAST(MAX(CNT)/AVG(CNT) AS NUMERIC(6,2)),0) AS SKEW_FACTOR, ISNULL(SUM(CNT) - SUM(CNTNOTNULLS),0) AS NUMNULLS "
                + " FROM (SELECT P_{1}, CAST(COUNT(*) AS NUMERIC) CNT, CAST(COUNT({0}) AS NUMERIC) CNTNOTNULLS "
                + " FROM {1} GROUP BY 1) PART(P_{1}, CNT, CNTNOTNULLS)", columnName, tempTableName)
                );

#if DEBUG
            Console.WriteLine(odbcCommand.CommandText);
#endif
            odbcCommand.Connection = aOdbcConnection;
            return ExecuteReader(odbcCommand);
        }
        /// <summary>
        /// Select the top N frequent values from the sampled data
        /// </summary>
        /// <param name="aOdbcConnection">Odbc connection</param>
        /// <param name="tempTableName">Temporary table that holds the sample data</param>
        /// <param name="columnName">Column name</param>
        /// <param name="numberFrequentValues">Number of frequent values to fetch</param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectTableColumnFrequentValues(OdbcConnection aOdbcConnection, string tempTableName, string columnName, int numberFrequentValues)
        {
            OdbcCommand odbcCommand = new OdbcCommand(
                String.Format("SELECT [FIRST {0}] {1}, CAST(COUNT({1}) AS NUMERIC) CNT FROM {2} WHERE {1} IS NOT NULL "
                    + " GROUP BY 1 ORDER BY 2 DESC FOR READ UNCOMMITTED ACCESS;", numberFrequentValues, columnName, tempTableName)
                );
#if DEBUG
            Console.WriteLine(odbcCommand.CommandText);
#endif
            odbcCommand.Connection = aOdbcConnection;
            odbcCommand.CommandTimeout = 0;
            return ExecuteReader(odbcCommand);
        }

        /// <summary>
        /// Selects the boundary intervals from the histogram intervals for this table column
        /// </summary>
        /// <param name="aOdbcConnection">Odbc connection</param>
        /// <param name="aTrafodionTable">Sql table</param>
        /// <param name="columnNumber">Column number of the table column</param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectTableColumnStatIntervals(OdbcConnection aOdbcConnection, PartitionedSchemaObject aTrafodionTable, int columnNumber)
        {
            OdbcCommand odbcCommand = new OdbcCommand(
                String.Format("SELECT T2.INTERVAL_NUMBER, T2.INTERVAL_BOUNDARY, T2.INTERVAL_UEC, T2.INTERVAL_ROWCOUNT "
                    + " FROM {0}.{1}.HISTOGRAMS T1, {0}.{1}.HISTOGRAM_INTERVALS T2 WHERE T1.TABLE_UID = {2} "
                    + " AND T1.COLCOUNT = 1 AND T1.COLUMN_NUMBER = {3} AND T1.TABLE_UID = T2.TABLE_UID "
                    + " AND T1.HISTOGRAM_ID = T2.HISTOGRAM_ID ORDER BY T2.INTERVAL_NUMBER FOR READ UNCOMMITTED ACCESS;", aTrafodionTable.TheTrafodionCatalog.ExternalName,
                aTrafodionTable.TheTrafodionSchema.ExternalName, aTrafodionTable.UID, columnNumber)
             );
#if DEBUG
            Console.WriteLine(odbcCommand.CommandText);
#endif
            odbcCommand.Connection = aOdbcConnection;
            odbcCommand.CommandTimeout = 0;
            return ExecuteReader(odbcCommand);
        }

        /// <summary>
        /// Selects the column's uec and cardinality from the temporary table for the given interval boundaries
        /// </summary>
        /// <param name="aOdbcConnection">Odbc connection</param>
        /// <param name="tempTableName">Temporary table that holds the sample data for the column</param>
        /// <param name="columnName">Column name whose sample is being used</param>
        /// <param name="minBoundary">min value for this interval boundary</param>
        /// <param name="maxBoundary">max value for this interval boundary</param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectStatsForIntervals(OdbcConnection aOdbcConnection, string tempTableName, string columnName, string minBoundary, string maxBoundary)
        {
            string queryText = "";
            if (minBoundary.Equals(maxBoundary))
            {
                if (minBoundary.Equals("NULL", StringComparison.OrdinalIgnoreCase))
                {
                    queryText = String.Format("SELECT CAST(COUNT(DISTINCT {0}) AS NUMERIC), CAST(COUNT({0}) AS NUMERIC) "
                        + " FROM {1} FOR READ UNCOMMITTED ACCESS;", columnName, tempTableName);
                }
                else
                {
                    queryText = String.Format("SELECT CAST(COUNT(DISTINCT {0}) AS NUMERIC), CAST(COUNT({0}) AS NUMERIC) "
                        + " FROM {1} where {0} <= {2} FOR READ UNCOMMITTED ACCESS;", columnName, tempTableName, minBoundary);
                }
            }
            else
            {
                if (minBoundary.Equals("NULL", StringComparison.OrdinalIgnoreCase))
                {
                    // Since min != max, so the max should never be NULL at this time. 
                    queryText = String.Format("SELECT CAST(COUNT(DISTINCT {0}) AS NUMERIC), CAST(COUNT({0}) AS NUMERIC)"
                        + " FROM {1} where {0} <= {2} FOR READ UNCOMMITTED ACCESS;", columnName, tempTableName, maxBoundary);
                }
                else
                {
                    if (maxBoundary.Equals("NULL", StringComparison.OrdinalIgnoreCase))
                    {
                        queryText = String.Format("SELECT CAST(COUNT(DISTINCT {0}) AS NUMERIC), CAST(COUNT({0}) AS NUMERIC)"
                            + " FROM {1} where {0} > {2} FOR READ UNCOMMITTED ACCESS;", columnName, tempTableName, minBoundary);
                    }
                    else
                    {
                        queryText = String.Format("SELECT CAST(COUNT(DISTINCT {0}) AS NUMERIC), CAST(COUNT({0}) AS NUMERIC)"
                            + " FROM {1} where {0} > {2} and {0} <= {3} FOR READ UNCOMMITTED ACCESS;", columnName, tempTableName, minBoundary, maxBoundary);
                    }
                }
            }

            OdbcCommand odbcCommand = new OdbcCommand(queryText);
#if DEBUG
            Console.WriteLine(odbcCommand.CommandText);
#endif
            odbcCommand.Connection = aOdbcConnection;
            odbcCommand.CommandTimeout = 0;
            return ExecuteReader(odbcCommand);
        }
        /// <summary>
        /// Get Column usage info for Views
        /// </summary>
        /// <param name="aConnection">Connection</param>
        /// <param name="aTrafodionCatalog">Catalog name</param>
        /// <param name="aSchemaVersion">Schema version</param>
        /// <param name="aTrafodionView">The View</param>
        /// <returns>The list of used objects in column number order</returns>
        static public OdbcDataReader ExecuteSelectColUsage(Connection aConnection, TrafodionCatalog aTrafodionCatalog, int aSchemaVersion, TrafodionView aTrafodionView)
        {
            string theQueryText = String.Format("SELECT O.OBJECT_NAME, 1 "
                + " FROM {0}.\"_MD_\".OBJECTS AS O,"
                + " {0}.\"_MD_\".VIEWS_USAGE AS U "
                + " WHERE U.USING_VIEW_UID = {2} AND O.OBJECT_UID = U.USED_OBJECT_UID "
                + " FOR READ UNCOMMITTED ACCESS",
                new object[] { aTrafodionCatalog.ExternalName, aSchemaVersion, aTrafodionView.UID });

            OdbcCommand theQuery = new OdbcCommand(theQueryText);
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Get Column usage info for MaterializedViews
        /// </summary>
        /// <param name="aConnection">Connection</param>
        /// <param name="aTrafodionMaterializedView">The Materialized View</param>
        /// <returns>The list of used objects in column number order</returns>
        static public OdbcDataReader ExecuteSelectMVColUsage(Connection aConnection, TrafodionMaterializedView aTrafodionMaterializedView)
        {
            string theQueryText = String.Format("SELECT O.OBJECT_NAME, U.ORIGINAL_TABLE_SCHEMA_UID "
                + " FROM {0}."_MD_".OBJECTS AS O,"
                + " {0}."_MD_".MVS_COLS AS U "
                + " WHERE U.MV_UID = {2} AND O.OBJECT_UID = U.ORIGINAL_TABLE_UID "
                + " ORDER by U.MV_COL_NUM FOR READ UNCOMMITTED ACCESS",
                new object[] { aTrafodionMaterializedView.TheTrafodionCatalog.ExternalName, aTrafodionMaterializedView.TheTrafodionSchema.Version, aTrafodionMaterializedView.UID });

            OdbcCommand theQuery = new OdbcCommand(theQueryText);
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }


        /// <summary>
        /// Selects a list of Tables used by a View
        /// </summary>
        /// <param name="aConnection">The connection</param>
        /// <param name="anExternalCatalogName">The name of the catalog containing the definition schema</param>
        /// <param name="aSchemaVersion">The schema version</param>
        /// <param name="anObjectUID">The UID of the sql object</param>
        /// <returns>Rows of Table Name and their Schedma UIDs</returns>
        static public OdbcDataReader ExecuteSelectTablesUsedByView(Connection aConnection, String anExternalCatalogName, int aSchemaVersion, long anObjectUID)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format("SELECT T2.OBJECT_NAME AS NAME, T2.SCHEMA_UID"
                + " FROM {0}."_MD_".VW_TBL_USAGE AS T1, {0}."_MD_".OBJECTS AS T2"
                + " WHERE T1.USING_VIEW_UID = {2} AND T1.USED_OBJ_UID = T2.OBJECT_UID  AND T2.OBJECT_TYPE = 'BT'  ORDER BY NAME FOR READ UNCOMMITTED ACCESS;",
                new object[] { anExternalCatalogName, aSchemaVersion, anObjectUID })
                );

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }





        /// <summary>
        /// Selects a list of MVs used by a View
        /// </summary>
        /// <param name="aConnection">The connection</param>
        /// <param name="anExternalCatalogName">The name of the catalog containing the definition schema</param>
        /// <param name="aSchemaVersion">The schema version</param>
        /// <param name="anObjectUID">The UID of the sql object</param>
        /// <returns>Rows of MV Name and their Schedma UIDs</returns>
        static public OdbcDataReader ExecuteSelectMVsUsedByView(Connection aConnection, String anExternalCatalogName, int aSchemaVersion, long anObjectUID)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format("SELECT T2.OBJECT_NAME AS NAME, T2.SCHEMA_UID"
                + " FROM {0}."_MD_".VW_TBL_USAGE AS T1, {0}."_MD_".OBJECTS AS T2"
                + " WHERE T1.USING_VIEW_UID = {2} AND T1.USED_OBJ_UID = T2.OBJECT_UID  AND T2.OBJECT_TYPE = 'MV'  ORDER BY NAME FOR READ UNCOMMITTED ACCESS;",
                new object[] { anExternalCatalogName, aSchemaVersion, anObjectUID })
                );

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }



        /// <summary>
        /// Selects a list of Views used by a View
        /// </summary>
        /// <param name="aConnection">The connection</param>
        /// <param name="anExternalCatalogName">The name of the catalog containing the definition schema</param>
        /// <param name="aSchemaVersion">The schema version</param>
        /// <param name="anObjectUID">The UID of the sql object</param>
        /// <returns>Rows of View Name and their Schedma UIDs</returns>
        static public OdbcDataReader ExecuteSelectViewsUsedByView(Connection aConnection, String anExternalCatalogName, int aSchemaVersion, long anObjectUID)
        {
            DatabaseAreaOptions options = DatabaseAreaOptions.GetOptions();
            OdbcCommand theQuery = new OdbcCommand();
            theQuery.CommandText = String.Format("SELECT T2.OBJECT_NAME AS NAME, T2.SCHEMA_UID, T3.VIEW_TYPE"
                + " FROM {0}."_MD_".VW_TBL_USAGE AS T1, {0}."_MD_".OBJECTS AS T2"
                + " , {0}."_MD_".VWS AS T3"
                + " WHERE T1.USING_VIEW_UID = {2} AND T1.USED_OBJ_UID = T2.OBJECT_UID  AND T2.OBJECT_TYPE = 'VI'"
                + " AND T2.OBJECT_UID = T3.OBJECT_UID "
                + " ORDER BY NAME FOR READ UNCOMMITTED ACCESS;",
                new object[] { anExternalCatalogName, aSchemaVersion, anObjectUID }
                );
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Select the routines used by the view
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aTrafodionView"></param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectRoutinesUsedByView(Connection aConnection, TrafodionView aTrafodionView)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format("SELECT T2.OBJECT_NAME AS NAME, T2.SCHEMA_UID , T3.UDR_TYPE "
                    + "FROM {0}."_MD_".VW_TBL_USAGE AS T1, "
                    + "{0}."_MD_".OBJECTS AS T2 , "
                    + "{0}."_MD_".ROUTINES AS T3 "
                    + "WHERE T1.USING_VIEW_UID = {2} "
                    + "AND T1.USED_OBJ_UID = T2.OBJECT_UID "
                    + "AND T2.OBJECT_TYPE = 'UR' "
                    + "AND T1.USED_OBJ_UID = T3.UDR_UID "
                    + "ORDER BY NAME FOR READ UNCOMMITTED ACCESS;",
                new object[] { aTrafodionView.TheTrafodionCatalog.ExternalName, aTrafodionView.TheTrafodionSchema.Version, aTrafodionView.UID })
                );

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Select the routines used by the library
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aTrafodionLibrary"></param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectRoutinesUsedByLibrary(Connection aConnection, TrafodionLibrary aTrafodionLibrary)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format("SELECT T2.OBJECT_NAME AS NAME, T2.SCHEMA_NAME , T3.UDR_TYPE "
                    + "FROM {0}."_MD_".LIBRARIES_USAGE AS T1, "
                    + "{0}.\"_MD_\".OBJECTS AS T2 , "
                    + "{0}.\"_MD_\".ROUTINES AS T3 "
                    + "WHERE T1.LIBRARY_UID = {1} "
                    + "AND T1.LIBRARY_UID=T3.USING_LIBRARY_UID "
                    + "AND T2.OBJECT_UID=T1.USED_UDR_UID "
                    + "AND T2.OBJECT_TYPE = 'UR' "
                    + "AND T1.USED_UDR_UID = T3.UDR_UID "
                    + "ORDER BY NAME FOR READ UNCOMMITTED ACCESS;",
                new object[] { aTrafodionLibrary.TheTrafodionCatalog.ExternalName, aTrafodionLibrary.UID })
                );

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Selects a list of Views USING the View
        /// </summary>
        /// <param name="aConnection">The connection</param>
        /// <param name="anExternalCatalogName">The name of the catalog containing the definition schema</param>
        /// <param name="aSchemaVersion">The schema version</param>
        /// <param name="anObjectUID">The UID of the sql object</param>
        /// <returns>Rows of View Name and their Schedma UIDs</returns>
        static public OdbcDataReader ExecuteSelectViewsUsingView(Connection aConnection, String anExternalCatalogName, int aSchemaVersion, long anObjectUID)
        {
            DatabaseAreaOptions options = DatabaseAreaOptions.GetOptions();
            OdbcCommand theQuery = new OdbcCommand();
            theQuery.CommandText = String.Format("SELECT T2.OBJECT_NAME AS NAME, T2.SCHEMA_UID, T3.VIEW_TYPE"
                + " FROM {0}."_MD_".VW_TBL_USAGE AS T1, {0}."_MD_".OBJECTS AS T2"
                + " , {0}."_MD_".VWS AS T3"
                + " WHERE T1.USED_OBJ_UID = {2} AND T1.USING_VIEW_UID = T2.OBJECT_UID  AND T2.OBJECT_TYPE = 'VI' "
                + " AND T2.OBJECT_UID = T3.OBJECT_UID "
                + " ORDER BY NAME FOR READ UNCOMMITTED ACCESS;",
                new object[] { anExternalCatalogName, aSchemaVersion, anObjectUID }
                );
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        static public OdbcDataReader ExecuteSelectTrafodionSchemaObjectsUsedByRoutine(Connection aConnection, TrafodionRoutine aTrafodionRoutine)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format(" SELECT CATALOG_UID, SCHEMA_UID, OBJECT_UID, OBJECT_TYPE "
                + " FROM ( "
                + " SELECT VIEW_CATALOG_UID AS CATALOG_UID, VIEW_SCHEMA_UID AS SCHEMA_UID, USING_VIEW_UID AS OBJECT_UID, 'VI' AS OBJECT_TYPE "
                + " FROM {0}."_MD_".VW_TBL_USAGE T1, "
                + " {0}."_MD_".OBJECTS T2 "
                + " WHERE USED_OBJ_UID = {2} "
                + " AND T1.USED_OBJ_UID = T2.OBJECT_UID "
                + " FOR READ UNCOMMITTED ACCESS "
                + " UNION "
                + " SELECT MV_CATALOG_UID AS CATALOG_UID, MV_SCHEMA_UID AS SCHEMA_UID, MV_UID AS OBJECT_UID,  'MV' AS OBJECT_TYPE  "
                + " FROM {0}."_MD_".MVS_USED T1, "
                + " {0}."_MD_".OBJECTS T2 "
                + " WHERE USED_OBJECT_UID = {2} "
                + " AND T1.USED_OBJECT_UID = T2.OBJECT_UID "
                + " FOR READ UNCOMMITTED ACCESS "
                + " UNION "
                + " SELECT {3} AS CATALOG_UID, {4} AS SCHEMA_UID, LIBRARY_OBJECT_ID AS OBJECT_UID,  'LB' AS OBJECT_TYPE  "
                + " FROM {0}."_MD_".ROUTINE_LIBS_USAGE T1, "
                + " {0}."_MD_".OBJECTS T2 "
                + " WHERE T1.ROUTINE_OBJECT_ID = {2} "
                + " AND T1.LIBRARY_OBJECT_ID= T2.OBJECT_UID "
                + " FOR READ UNCOMMITTED ACCESS "
                + " UNION "
                + " SELECT TRIGGER_CATALOG_UID AS CATALOG_UID, TRIGGER_SCHEMA_UID AS SCHEMA_UID, TRIGGER_UID AS OBJECT_UID,  'TR' AS OBJECT_TYPE  "
                + " FROM {0}."_MD_".TRIGGER_USED T1, "
                + " {0}."_MD_".OBJECTS T2 "
                + " WHERE USED_OBJECT_UID = {2} "
                + " AND T1.USED_OBJECT_UID = T2.OBJECT_UID "
                + " FOR READ UNCOMMITTED ACCESS "
                + " ) AS T; ",
                new object[] { aTrafodionRoutine.TheTrafodionCatalog.ExternalName, aTrafodionRoutine.TheTrafodionSchema.Version, aTrafodionRoutine.UID,
                    aTrafodionRoutine.TheTrafodionCatalog.UID,aTrafodionRoutine.TheTrafodionSchema.UID })
                );

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Selects the sequence generator attributes for an identity column
        /// </summary>
        /// <param name="aConnection">The connection</param>
        /// <param name="aTrafodionSchemaObject">the object using the sequence generator</param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectSequenceAttributes(Connection aConnection, TrafodionSchemaObject aTrafodionSchemaObject)
        {
            OdbcCommand odbcCommand = new OdbcCommand(
                String.Format("SELECT START_VALUE, INCREMENT, MIN_VALUE, MAX_VALUE, CYCLE_OPTION"
                    + " FROM {0}."_MD_".SG_USAGE as T1, {0}."_MD_".SEQUENCE_GENERATORS as T2"
                    + " WHERE T1.USING_OBJECT_OBJ_UID = {2} AND T2.OBJECT_UID = T1.SG_OBJECT_UID"
                    + " FOR READ UNCOMMITTED ACCESS;",
                    new object[] { aTrafodionSchemaObject.TheTrafodionCatalog.InternalName, aTrafodionSchemaObject.TheTrafodionSchema.Version, aTrafodionSchemaObject.UID }));

            odbcCommand.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(odbcCommand);

        }
        /// <summary>
        /// Selects the detailed summary of the tables in a schema
        /// </summary>
        /// <param name="aConnection">The connection</param>
        /// <param name="aTrafodionSchema">The model of the schema object for which the tables are needed</param>        
        /// <returns>OdbcCommand</returns>
        static public OdbcCommand ExecuteSelectTableDetailsSummary(Connection aConnection, TrafodionSchema aTrafodionSchema)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format(


                  " SELECT  CASE WHEN  max(temp_histograms.stats_time) IS NULL THEN 0 ELSE JULIANTIMESTAMP(MAX(temp_histograms.stats_time)) END"
                + " ,temp_table.a"
                + " ,t3.object_uid "
                + " ,t3.OBJECT_SECURITY_CLASS"
                + " ,sum(temp_table.b)"
                + " ,sum(temp_table.c)"
                + " ,sum(temp_table.d)"
                + " FROM "
                + " (select object_name, row_count, cast(((PRIMARY_EXTENTS + (SECONDARY_EXTENTS * (MAX_EXTENTS -1))) * 2048) as numeric(40,0)), cast(current_eof as numeric(40,0)) from table(disk label statistics( using"
                + " (select * from (get all tables in schema {1}.{2}, no header, return full names) x(a) ))) ) temp_table(a,b,c,d)"
                + " , {0}.system_schema.schemata t1"
                + " , {0}.system_schema.catsys t2"
                + " ,{1}.definition_schema_version_{4}.objects t3"
                + " left join (select max(t4.stats_time), t4.table_uid from {1}.{2}.histograms t4 group by t4.table_uid)  temp_histograms(stats_time, table_uid) on temp_histograms.table_uid = t3.object_uid"
                + " WHERE "
                + " temp_table.a = t3.object_name"
                + " and t3.object_type = 'BT'"
                + " and t3.object_name_space = 'TA'"
                + " and t1.schema_name = '{3}'"
                + " and t1.cat_uid = t2.cat_uid"
                + " and t2.cat_name = '{1}'"
                + " and t3.schema_uid = t1.schema_uid"
                + " GROUP BY (temp_table.a, t3.object_uid, t3.OBJECT_SECURITY_CLASS) order by t3.object_uid;"
                , new object[] { aConnection.SystemCatalogName 
                               , aTrafodionSchema.TheTrafodionCatalog.ExternalName
                               , aTrafodionSchema.ExternalName
                               , aTrafodionSchema.InternalName
                               , aTrafodionSchema.Version
                               })
                );

            //theQuery.CommandTimeout = 0;
            theQuery.Connection = aConnection.OpenOdbcConnection;

            return theQuery;

        }

        /// <summary>
        /// Selects the detailed summary of the indexes in a schema
        /// </summary>
        /// <param name="aConnection">The connection</param>
        /// <param name="aTrafodionSchema">The model of the schema object for which the tables are needed</param>        
        /// <returns>OdbcCommand</returns>
        static public OdbcCommand ExecuteSelectIndexDetailsSummary(Connection aConnection, TrafodionSchema aTrafodionSchema)
        {
            OdbcCommand theQuery = new OdbcCommand(
                 String.Format(

                      " SELECT  temp_table.object_name"
                    + "    ,sum(temp_table.row_count)"
                    + "    ,cast(sum(temp_table.max_size) as numeric(40,0))"
                    + "    ,cast(sum(temp_table.current_eof) as numeric(40,0))"
                    + " FROM "
                    + "    (select object_name, row_count, ((PRIMARY_EXTENTS + (SECONDARY_EXTENTS * (MAX_EXTENTS -1))) * 2048) as max_size, current_eof from table(disk label statistics( indexes using"
                    + "    (select * from (get all indexes in schema {0}.{1}, no header, return full names) x(a) )))) temp_table(object_name,row_count,max_size,current_eof)"
                    + " GROUP BY (temp_table.object_name);"
                , new object[] { aTrafodionSchema.TheTrafodionCatalog.ExternalName, aTrafodionSchema.ExternalName })
                );

            //theQuery.CommandTimeout = 0;
            theQuery.Connection = aConnection.OpenOdbcConnection;

            return theQuery;

        }

        /// <summary>
        /// Selects the detailed summary of the tables in a schema
        /// </summary>
        /// <param name="aConnection">The connection</param>
        /// <param name="aTrafodionSchema">The model of the schema object for which the tables are needed</param>        
        /// <returns>OdbcCommand</returns>

        static public OdbcCommand ExecuteSelectMVDetailsSummary(Connection aConnection, TrafodionSchema aTrafodionSchema)
        {
            OdbcCommand theQuery = new OdbcCommand(
                 String.Format(

                // The modification is to remove the use of MaintainInfoView, which is still considered as
                // internal and may not be available on all customer's system. Leave the code here is to
                // allow easy restore once this view is made available for customer use.
                // 1/27/2009

                //  " SELECT "
                //+ "	temp_table.object_name"
                //+ "	,t3.object_uid"
                //+ "	,t3.OBJECT_SECURITY_CLASS"
                //+ " ,sum(temp_table.row_count)"
                //+ "	,sum(temp_table.max_size)"
                //+ "	,sum(temp_table.current_eof)"
                //+ "	,CASE WHEN t4.op_time IS NULL THEN 0 ELSE JULIANTIMESTAMP(t4.op_time) END"
                //+ " FROM "
                //+ "	(select object_name, row_count, max_size, current_eof from table(disk label statistics( using"
                //+ "	(select * from (get mvs in schema {0}.{1}, no header, return full names) x(a) ))) ) temp_table(object_name,row_count,max_size,current_eof)"
                //+ "	, {3}.system_schema.schemata t1"
                //+ "	, {3}.system_schema.catsys t2"
                //+ "	,{0}.definition_schema_version_{2}.objects t3"
                //+ "	 left join trafodion.public_access_schema.maintaininfoview t4 on t3.object_name = t4.object_name and"
                //+ "	  t4.object_type = 'MV' and t4.maintain_op = 'REFRESH' and t4.catalog_name = '{0}' and t4.schema_name = '{1}'"
                //+ " WHERE "
                //+ "	temp_table.object_name = t3.object_name"
                //+ "	and t3.object_type = 'MV'"
                //+ "	and t1.schema_name = '{5}'"
                //+ "	and t1.cat_uid = t2.cat_uid"
                //+ "	and t2.cat_name = '{4}'"
                //+ "	and t3.schema_uid = t1.schema_uid"
                //+ " GROUP BY (temp_table.object_name, t3.object_uid, t3.OBJECT_SECURITY_CLASS, t4.op_time) order by t3.object_uid;"

                  " SELECT "
                + "	temp_table.object_name"
                + " ,sum(temp_table.row_count)"
                + "	,cast(sum(temp_table.max_size) as numeric(40,0))"
                + "	,cast(sum(temp_table.current_eof) as numeric(40,0))"
                + " FROM "
                + "	(select object_name, row_count, ((PRIMARY_EXTENTS + (SECONDARY_EXTENTS * (MAX_EXTENTS -1))) * 2048) as max_size, current_eof from table(disk label statistics( using"
                + "	(select * from (get all mvs in schema {0}.{1}, no header, return full names) x(a) )))) temp_table(object_name,row_count,max_size,current_eof)"
                + " GROUP BY (temp_table.object_name);"

                //, new object[] { aTrafodionSchema.TheTrafodionCatalog.ExternalName
                //               , aTrafodionSchema.ExternalName
                //               , aTrafodionSchema.Version 
                //               , aConnection.SystemCatalogName
                //               , aTrafodionSchema.TheTrafodionCatalog.InternalName
                //               , aTrafodionSchema.InternalName })
                , new object[] { aTrafodionSchema.TheTrafodionCatalog.ExternalName, aTrafodionSchema.ExternalName })
                );

            //theQuery.CommandTimeout = 0;
            theQuery.Connection = aConnection.OpenOdbcConnection;

            return theQuery;

        }

        /// <summary>
        /// Selects the detailed summary of the indexes of a table
        /// </summary>
        /// <param name="aConnection">The connection</param>
        /// <param name="aTrafodionSchema">The model of the schema object for which the tables are needed</param>        
        /// <returns>OdbcCommand</returns>

        static public OdbcCommand ExecuteSelectTableIndexAggregate(Connection aConnection, TrafodionSchema aTrafodionSchema)
        {
            return ExecuteSelectIndexAggregate(aConnection, aTrafodionSchema, TrafodionTable.ObjectType);
        }


        /// <summary>
        /// Selects the detailed summary of the indexes of an MV
        /// </summary>
        /// <param name="aConnection">The connection</param>
        /// <param name="aTrafodionSchema">The model of the schema object for which the tables are needed</param> 
        /// <returns>OdbcCommand</returns>

        static public OdbcCommand ExecuteSelectMVIndexAggregate(Connection aConnection, TrafodionSchema aTrafodionSchema)
        {
            return ExecuteSelectIndexAggregate(aConnection, aTrafodionSchema, TrafodionMaterializedView.ObjectType);
        }

        /// <summary>
        /// Selects the detailed summary of indexes of an object (Table or MV) based on the flag passed.
        /// 
        /// </summary>
        /// <param name="aConnection">The connection</param>
        /// <param name="aTrafodionSchema">The model of the schema object for which the indexes are needed</param>        
        /// <param name="aFlag">Dictates the object type for which the indexes are to be found
        ///                     The valid values ate "BT" and "MV"
        /// </param>
        /// <returns>OdbcCommand</returns>

        static private OdbcCommand ExecuteSelectIndexAggregate(Connection aConnection,
            TrafodionSchema aTrafodionSchema,
            String aFlag)
        {
            OdbcCommand theQuery = new OdbcCommand(
               String.Format(

                    " SELECT "
                    + "    cast(sum(temp.max_size) as numeric(40,0))"
                    + "    , cast(sum(temp.current_eof) as numeric(40,0))"
                    + " FROM "
                    + "    (select object_name, ((PRIMARY_EXTENTS + (SECONDARY_EXTENTS * (MAX_EXTENTS -1))) * 2048) as max_size,current_eof from table(disk label statistics(indexes using"
                    + "     (select * from (get all indexes in schema {0}.{1}, no header, return full names) x(a) ))) ) temp (object_name,max_size,current_eof)"
                    + "    , {4}.system_schema.catsys c"
                    + "    , {4}.system_schema.schemata s"
                    + "    , {0}.definition_schema_version_{2}.objects o"
                    + "    , {0}.definition_schema_version_{2}.access_paths a"
                    + " WHERE "
                    + "    temp.object_name = o.object_name"
                    + "    and o.object_type = 'IX'"
                    + "    and o.object_name_space = 'IX'"
                    + "    and s.schema_name = '{6}'"
                    + "    and c.cat_uid = s.cat_uid"
                    + "    and c.cat_name = '{5}'"
                    + "    and o.schema_uid = s.schema_uid"
                    + "    and o.object_uid = a.access_path_uid"
                    + "    and a.table_uid in ("
                    + "        select object_uid from {0}.definition_schema_version_{2}.objects where object_type='{3}' for read uncommitted access) for read uncommitted access;"
            , new object[] { aTrafodionSchema.TheTrafodionCatalog.ExternalName 
                             , aTrafodionSchema.ExternalName 
                             , aTrafodionSchema.Version 
                             , aFlag 
                             , aConnection.SystemCatalogName
                             , aTrafodionSchema.TheTrafodionCatalog.InternalName
                             , aTrafodionSchema.InternalName
                           })
                );

            //theQuery.CommandTimeout = 0;
            theQuery.Connection = aConnection.OpenOdbcConnection;

            return theQuery;

        }

        /// <summary>
        /// Selects the aggregated Table size for all the tables in a schema.
        /// </summary>
        /// <param name="aConnection">The connection</param>
        /// <param name="aTrafodionSchema">The schema object for which the aggregates are needed</param>        
        /// <returns>OdbcCommand</returns>
        static public OdbcCommand ExecuteSelectTableAggregate(Connection aConnection, TrafodionSchema aTrafodionSchema)
        {
            return ExecuteSelectObjectAggregate(aConnection, aTrafodionSchema, "tables");
        }

        /// <summary>
        /// Selects the aggregated size for all the indexes in a schema.
        /// </summary>
        /// <param name="aConnection">The connection</param>
        /// <param name="aTrafodionSchema">The schema object for which the aggregates are needed</param>        
        /// <returns>OdbcCommand</returns>
        static public OdbcCommand ExecuteSelectAllIndexAggregate(Connection aConnection, TrafodionSchema aTrafodionSchema)
        {
            OdbcCommand theQuery = new OdbcCommand(
                 String.Format(
                      " SELECT CAST(SUM((PRIMARY_EXTENTS + (SECONDARY_EXTENTS * (MAX_EXTENTS -1))) * 2048) AS NUMERIC(40,0)), CAST(SUM(CURRENT_EOF) AS NUMERIC(40,0)) FROM TABLE(DISK LABEL STATISTICS( INDEXES USING "
                    + "     (SELECT * FROM (GET ALL INDEXES IN SCHEMA {0}.{1}, NO HEADER, RETURN FULL NAMES) X(A) )));"
            , new object[] { aTrafodionSchema.TheTrafodionCatalog.ExternalName, aTrafodionSchema.ExternalName })
                );

            //theQuery.CommandTimeout = 0;
            theQuery.Connection = aConnection.OpenOdbcConnection;

            return theQuery;

        }

        /// <summary>
        /// Selects the aggregated MV size for all the tables in a schema.
        /// </summary>
        /// <param name="aConnection">The connection</param>
        /// <param name="aTrafodionSchema">The schema object for which the aggregates are needed</param>        
        /// <returns>OdbcCommand</returns>
        static public OdbcCommand ExecuteSelectMVAggregate(Connection aConnection, TrafodionSchema aTrafodionSchema)
        {
            return ExecuteSelectObjectAggregate(aConnection, aTrafodionSchema, "mvs");
        }


        /// <summary>
        /// Selects the aggregated object size for the (Table or MV) objects  in a schema based on the flag passed.
        /// 
        /// </summary>
        /// <param name="aConnection">The connection</param>
        /// <param name="aTrafodionSchema">The schema object for which the aggregates are needed</param>        
        /// <param name="aFlag">Dictates the object type for which the aggregated size is to be computed
        ///                     The valid values ate "tables" and "mvs"
        /// </param>
        /// <returns>OdbcCommand</returns>
        static private OdbcCommand ExecuteSelectObjectAggregate(Connection aConnection,
            TrafodionSchema aTrafodionSchema,
            String aFlag)
        {
            OdbcCommand theQuery = new OdbcCommand(
                 String.Format(
                      " SELECT CAST(SUM((PRIMARY_EXTENTS + (SECONDARY_EXTENTS * (MAX_EXTENTS -1))) * 2048) AS NUMERIC(40,0)), CAST(SUM(CURRENT_EOF) AS NUMERIC(40,0)) FROM TABLE(DISK LABEL STATISTICS( USING "
                    + "     (SELECT * FROM (GET ALL {2} IN SCHEMA {0}.{1}, NO HEADER, RETURN FULL NAMES) X(A) )));"
            , new object[] { aTrafodionSchema.TheTrafodionCatalog.ExternalName, aTrafodionSchema.ExternalName, aFlag })
                );

            //theQuery.CommandTimeout = 0;
            theQuery.Connection = aConnection.OpenOdbcConnection;

            return theQuery;
        }

        /// <summary>
        /// Selects the detailed summary of indexes of an object (Table or MV) based on the flag passed.
        /// 
        /// </summary>
        /// <param name="aConnection">The connection</param>
        /// <param name="anExternalCatalogName">The name of the catalog containing the definition schema</param>
        /// <param name="anExternalSchemaName">The name of the schema containing the target object</param>        
        /// <param name="aSchemaObjectName">The name of the object which the indexes are needed</param>
        /// <param name="aType">Dictates the object type for which the indexes are to be found
        ///                     The valid values ate "table" and "mv"
        /// </param>
        /// <returns>OdbcCommand</returns>
        static public OdbcCommand ExecuteSelectIndexDetailsForSchemaObject(Connection aConnection,
            String anExternalCatalogName,
            String anExternalSchemaName,
            String aSchemaObjectName,
            String aType)
        {
            OdbcCommand theQuery = new OdbcCommand(
                 String.Format(

                      " SELECT "
                    + " OBJECT_NAME "
                    + " ,SUM(ROW_COUNT)"
                    + " ,SUM((PRIMARY_EXTENTS + (SECONDARY_EXTENTS * (MAX_EXTENTS -1))) * 2048)"
                    + " ,SUM(CURRENT_EOF)"
                    + " FROM "
                    + " TABLE(DISK LABEL STATISTICS(INDEXES USING(SELECT * FROM (GET INDEXES ON {3} {0}.{1}.{2}, NO HEADER, RETURN FULL NAMES) X(A))))"
                    + " GROUP BY "
                    + " OBJECT_NAME;"

                 , new object[] { anExternalCatalogName, anExternalSchemaName, aSchemaObjectName, aType })
                );

            //theQuery.CommandTimeout = 0;
            theQuery.Connection = aConnection.OpenOdbcConnection;

            return theQuery;
        }

        /// <summary>
        /// To retrieve the object security class for an object with its three part names
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aCatalogName"></param>
        /// <param name="aSchemaName"></param>
        /// <param name="anObjectName"></param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteInfoObjectSecurityClass(Connection aConnection, string aCatalogName, string aSchemaName, string anObjectName)
        {
            OdbcCommand odbcCommand = new OdbcCommand(
                String.Format("SELECT C.CAT_NAME, S.SCHEMA_NAME, O.OBJECT_NAME, O.OBJECT_SECURITY_CLASS FROM"
                    + " {0}.SYSTEM_SCHEMA.CATSYS as C,"
                    + " {0}.SYSTEM_SCHEMA.SCHEMATA as S,"
                    + " {1}."_MD_".OBJECTS as O"
                    + " WHERE"
                    + " C.CAT_NAME = '{1}' AND"
                    + " S.SCHEMA_NAME = '{2}' AND"
                    + " O.OBJECT_NAME = '{3}' AND"
                    + " C.CAT_UID = S.CAT_UID AND S.SCHEMA_UID = O.SCHEMA_UID"
                    + " FOR READ UNCOMMITTED ACCESS;",
                    new object[] { aConnection.SystemCatalogName, aCatalogName, aSchemaName, anObjectName }));

            odbcCommand.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(odbcCommand);
        }

        /// <summary>
        /// Get the column encoding of a sql object
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="aCatalogName"></param>
        /// <param name="aSchemaName"></param>
        /// <param name="anObjectName"></param>
        /// <param name="aColumnName"></param>
        /// <returns></returns>
        static public string GetColumnEncoding(ConnectionDefinition aConnectionDefinition, string aCatalogName, string aSchemaName, string anObjectName, string aColumnName)
        {
            OdbcDataReader reader = null;
            Connection aConnection = new Connection(aConnectionDefinition);
            string characterEncoding = "";
            try
            {
                OdbcCommand odbcCommand = new OdbcCommand(
                    String.Format("SELECT TRIM(CL.CHARACTER_SET) FROM"
                                   + " {0}.SYSTEM_SCHEMA.CATSYS as C,"
                                   + " {0}.SYSTEM_SCHEMA.SCHEMATA as S,"
                                   + " {1}."_MD_".OBJECTS as O,"
                                   + " {1}."_MD_".COLS CL"
                                   + " WHERE C.CAT_NAME = '{1}'"
                                   + " AND C.CAT_UID = S.CAT_UID"
                                   + " AND S.SCHEMA_NAME = '{2}'"
                                   + " AND S.SCHEMA_UID = O.SCHEMA_UID"
                                   + " AND O.OBJECT_NAME = '{3}'"
                                   + " AND O.OBJECT_UID = CL.OBJECT_UID"
                                   + " AND CL.COLUMN_NAME = '{4}'"
                                   + " FOR READ UNCOMMITTED ACCESS;",
                        new object[] { aConnectionDefinition.SystemCatalogName, aCatalogName, aSchemaName, anObjectName, aColumnName }));

                odbcCommand.Connection = aConnection.OpenOdbcConnection;
                reader = ExecuteReader(odbcCommand);
                if (reader.Read())
                {
                    characterEncoding = reader.GetString(0);
                }
            }
            catch (Exception ex)
            {

            }
            finally
            {
                if (reader != null)
                {
                    reader.Close();
                }
                if (aConnection != null)
                {
                    aConnection.Close();
                }
            }
            return characterEncoding;
        }

        static public int ExecuteGrantRevoke(ConnectionDefinition aConnectionDefinition, string grantRevokeSQL, out DataTable warningsTable)
        {
            Connection aConnection = new Connection(aConnectionDefinition);
            try
            {
                OdbcCommand odbcCommand = new OdbcCommand(grantRevokeSQL);

                odbcCommand.Connection = aConnection.OpenOdbcConnection;
                return Utilities.ExecuteNonQuery(odbcCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Database, TRACE_SUB_AREA_NAME, true, out warningsTable);
            }
            finally
            {
                if (aConnection != null)
                {
                    aConnection.Close();
                }
            }
        }
        /// <summary>
        /// Wrapper for executing a non query command with timeout.
        /// </summary>
        /// <param name="anOpenCommand"></param>
        /// <returns></returns>
        static private int ExecuteNonQuery(OdbcCommand anOpenCommand)
        {
            return ExecuteNonQuery(anOpenCommand, true);
        }

        /// <summary>
        /// Wrapper for executing a query command with timeout.
        /// </summary>
        /// <param name="anOpenCommand"></param>
        /// <returns></returns>
        static private OdbcDataReader ExecuteReader(OdbcCommand anOpenCommand)
        {
            return ExecuteReader(anOpenCommand, true);
        }

        /// <summary>
        /// Wrapper for executing a non query command with the option of specifying timeout or not.
        /// </summary>
        /// <param name="anOpenCommand"></param>
        /// <param name="timeout"></param>
        /// <returns></returns>
        static public int ExecuteNonQuery(OdbcCommand anOpenCommand, bool timeout)
        {
            return Utilities.ExecuteNonQuery(anOpenCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Database, TRACE_SUB_AREA_NAME, timeout);
        }

        /// <summary>
        /// Wrapper for executing a query command with the option of specifying timeout or not.
        /// </summary>
        /// <param name="anOpenCommand"></param>
        /// <param name="timeout"></param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteReader(OdbcCommand anOpenCommand, bool timeout)
        {
            return Utilities.ExecuteReader(anOpenCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Database, TRACE_SUB_AREA_NAME, timeout);
        }

        /// <summary>
        /// Selects the list of database users
        /// </summary>
        /// <param name="aConnection"></param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectDatabaseUserNames(Connection aConnection)
        {
            OdbcCommand theQuery = new OdbcCommand(string.Format("SELECT USER_NAME, AUTHTYPE(USER_ID),USER_ID, EXTERNAL_USER_NAME,IS_AUTO_REGISTERED, IS_VALID_USER "
                + "FROM {0}.TRAFODION_INFORMATION_SCHEMA.USER_INFO ORDER BY USER_NAME FOR READ UNCOMMITTED ACCESS",
                new object[] { aConnection.TheConnectionDefinition.SystemCatalogName }));
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);

        }

        /// <summary>
        /// Get a list of roles for the given database user name
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aDBUserName"></param>
        /// <returns></returns>
        static public OdbcDataReader GetRolesForDBUser(Connection aConnection, string aDBUserName)
        {

            string query = String.Format("SELECT ROLE_NAME, ROLE_ID " +
                                            "FROM TRAFODION_INFORMATION_SCHEMA.USER_INFO, " +
                                            "TRAFODION_INFORMATION_SCHEMA.USER_ROLE_INFO " +
                                            "WHERE USER_NAME = '{0}' AND USER_ID = GRANTEE_ID " +
                                            "ORDER BY ROLE_NAME FOR READ UNCOMMITTED ACCESS", aDBUserName.ToUpper());

            OdbcCommand theQuery = new OdbcCommand(query);
            theQuery.Connection = aConnection.OpenOdbcConnection;
            OdbcDataReader reader = ExecuteReader(theQuery);
            return reader;
        }

        /// <summary>
        /// Selects the list of database roles
        /// </summary>
        /// <param name="aConnection"></param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectDatabaseRoles(Connection aConnection)
        {
            OdbcCommand theQuery = new OdbcCommand(string.Format("SELECT ROLE_NAME,AUTHTYPE(ROLE_ID),ROLE_ID "
                + "FROM {0}.TRAFODION_INFORMATION_SCHEMA.ROLE_INFO ORDER BY ROLE_NAME FOR READ UNCOMMITTED ACCESS",
                new object[] { aConnection.TheConnectionDefinition.SystemCatalogName }));
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);

        }

        /// <summary>
        /// Selects the components defines on the system
        /// </summary>
        /// <param name="aConnection"></param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectComponents(Connection aConnection)
        {
            OdbcCommand theQuery = new OdbcCommand("SELECT COMPONENT_UID, COMPONENT_NAME FROM TRAFODION.TRAFODION_SECURITY_SCHEMA.COMPONENTS ORDER BY COMPONENT_NAME FOR READ UNCOMMITTED ACCESS");
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);

        }
        /// <summary>
        /// Selects the component privilege types configured on the system
        /// </summary>
        /// <param name="aConnection"></param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectComponentPrivilegeUsages(Connection aConnection)
        {
            OdbcCommand theQuery = new OdbcCommand("SELECT COMPONENT_UID, PRIV_TYPE, PRIV_NAME,	PRIV_DESCRIPTION FROM TRAFODION.TRAFODION_SECURITY_SCHEMA.COMPONENT_PRIVILEGE_USAGE ORDER BY COMPONENT_UID FOR READ UNCOMMITTED ACCESS");
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);

        }

        /// <summary>
        /// Selects all the component privileges granted to users/roles
        /// </summary>
        /// <param name="aConnection"></param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectComponentPrivileges(Connection aConnection)
        {
            OdbcCommand theQuery = new OdbcCommand("SELECT COMPONENT_UID, GRANTOR, AUTHNAME(GRANTOR) AS GRANTOR, GRANTEE, AUTHNAME(GRANTEE) AS GRANTEE, PRIVILEGE_TYPE,"
                + "IS_GRANTABLE,AUTHTYPE(GRANTEE) AS GRANTEETYPE,AUTHTYPE(GRANTOR) AS GRANTORTYPE FROM TRAFODION.TRAFODION_SECURITY_SCHEMA.COMPONENT_PRIVILEGES FOR READ UNCOMMITTED ACCESS ORDER BY COMPONENT_UID");

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Gets the SQL space usage at the system level across all catalogs
        /// </summary>
        /// <returns></returns>
        static public string GetSystemLevelSpaceUsageQueryString()
        {
            return "SELECT T2.CATALOG_NAME AS NAME, MIN(T2.LAST_COLLECTION_TIME) as LAST_COLLECTION_TIME, " +
                    "CAST(SUM(ISNULL((U.PRIMARY_EXTENTS + (U.SECONDARY_EXTENTS * (U.MAX_EXTENTS -1))),0) * 2048) AS NUMERIC(30)) AS TOTAL_MAX_SIZE, " +
                    "CASE " +
                    "WHEN MAX(U.COMPRESSION_TYPE) != 0 THEN CAST(SUM(ISNULL((U.COMPRESSED_EOF_SECTORS * 512),0)) AS NUMERIC(30)) "+
                    "ELSE -1 " +
                    "END AS COMPRESSED_SPACE_USED, " +
                    "CAST(SUM(ISNULL(U.CURRENT_EOF,0)) AS NUMERIC(30)) AS SPACE_USED " +
                    "FROM " +
                    "(  " +
                    "SELECT T1.CATALOG_NAME, T1.SCHEMA_NAME, U.OBJECT_NAME, U.OBJECT_NAME_TYPE, ISNULL(MAX(U.COLLECTION_TIME),0) AS LAST_COLLECTION_TIME " +
                    "FROM  " +
                    "(  " +
                    "SELECT C.CAT_NAME AS CATALOG_NAME, S.SCHEMA_NAME    " +
                    "FROM TRAFODION.SYSTEM_SCHEMA.CATSYS C, TRAFODION.SYSTEM_SCHEMA.SCHEMATA S   " +
                    "WHERE C.CAT_UID = S.CAT_UID   " +
                    "AND NOT (S.SCHEMA_NAME LIKE '@%')   " +
                    "AND S.CURRENT_OPERATION != 'VS'   " +
                    "FOR READ UNCOMMITTED ACCESS  " +
                    ") AS T1   " +
                    "LEFT JOIN MANAGEABILITY.INSTANCE_REPOSITORY.SPACE_SQL_SUMMARY_1 AS U   " +
                    "ON T1.CATALOG_NAME = U.CATALOG_NAME AND T1.SCHEMA_NAME = U.SCHEMA_NAME   " +
                    "GROUP BY T1.CATALOG_NAME, T1.SCHEMA_NAME, U.OBJECT_NAME, U.OBJECT_NAME_TYPE FOR READ UNCOMMITTED ACCESS  " +
                    ") AS T2 " +
                    "LEFT JOIN MANAGEABILITY.INSTANCE_REPOSITORY.SPACE_SQL_SUMMARY_1 U  " +
                    "ON T2.CATALOG_NAME = U.CATALOG_NAME  " +
                    "AND T2.SCHEMA_NAME = U.SCHEMA_NAME  " +
                    "AND T2.OBJECT_NAME = U.OBJECT_NAME " +
                    "AND T2.OBJECT_NAME_TYPE = U.OBJECT_NAME_TYPE " +
                    "AND T2.LAST_COLLECTION_TIME = U.COLLECTION_TIME " +
                    "GROUP BY T2.CATALOG_NAME " +
                    "ORDER BY T2.CATALOG_NAME FOR READ UNCOMMITTED ACCESS ";
        }

        /// <summary>
        /// Gets the SQL space usage for a given catalog
        /// </summary>
        /// <param name="aTrafodionCatalog">The given catalog model</param>
        /// <returns></returns>
        static public string GetCatalogLevelSpaceUsageQueryString(TrafodionCatalog aTrafodionCatalog)
        {
            return string.Format(
                    "SELECT T2.SCHEMA_NAME AS NAME, MIN(T2.LAST_COLLECTION_TIME) as LAST_COLLECTION_TIME, " +
                    "CAST(SUM((U.PRIMARY_EXTENTS + (U.SECONDARY_EXTENTS * (U.MAX_EXTENTS -1))) * 2048) AS NUMERIC(30)) AS TOTAL_MAX_SIZE,  " +
                    "CASE " +
                    "WHEN MAX(U.COMPRESSION_TYPE) != 0 THEN CAST(SUM(ISNULL((U.COMPRESSED_EOF_SECTORS * 512),0)) AS NUMERIC(30)) " +
                    "ELSE -1 " +
                    "END AS COMPRESSED_SPACE_USED, " +
                    "CAST(SUM(ISNULL(U.CURRENT_EOF,0)) AS NUMERIC(30)) AS SPACE_USED,ISNULL(MAX(U.COMPRESSION_TYPE),0) AS COMPRESSION_TYPE " +
                    "FROM   " +
                    "(  " +
                    "SELECT T1.CATALOG_NAME, T1.SCHEMA_NAME, U.OBJECT_NAME, U.OBJECT_NAME_TYPE, ISNULL(MAX(U.COLLECTION_TIME),0) AS LAST_COLLECTION_TIME   " +
                    "FROM   " +
                    "(  " +
                    "SELECT C.CAT_NAME AS CATALOG_NAME, S.SCHEMA_NAME    " +
                    "FROM TRAFODION.SYSTEM_SCHEMA.CATSYS C, TRAFODION.SYSTEM_SCHEMA.SCHEMATA S   " +
                    "WHERE C.CAT_NAME = '{0}' AND S.CAT_UID = C.CAT_UID   " +
                    "AND NOT (S.SCHEMA_NAME LIKE '@%')   " +
                    "AND S.CURRENT_OPERATION != 'VS'   " +
                    "FOR READ UNCOMMITTED ACCESS  " +
                    ") AS T1   " +
                    "LEFT JOIN MANAGEABILITY.INSTANCE_REPOSITORY.SPACE_SQL_SUMMARY_1 AS U   " +
                    "ON T1.CATALOG_NAME = U. CATALOG_NAME AND T1.SCHEMA_NAME = U.SCHEMA_NAME   " +
                    "GROUP BY T1.CATALOG_NAME, T1.SCHEMA_NAME, U.OBJECT_NAME, U.OBJECT_NAME_TYPE FOR READ UNCOMMITTED ACCESS  " +
                    ") AS T2   " +
                    "LEFT JOIN MANAGEABILITY.INSTANCE_REPOSITORY.SPACE_SQL_SUMMARY_1 U  " +
                    "ON T2.CATALOG_NAME = U.CATALOG_NAME  " +
                    "AND T2.SCHEMA_NAME = U.SCHEMA_NAME  " +
                    "AND T2.OBJECT_NAME = U.OBJECT_NAME " +
                    "AND T2.OBJECT_NAME_TYPE = U.OBJECT_NAME_TYPE " +
                    "AND T2.LAST_COLLECTION_TIME = U.COLLECTION_TIME " +
                    "GROUP BY T2.CATALOG_NAME, T2.SCHEMA_NAME  " +
                    "ORDER BY T2.SCHEMA_NAME FOR READ UNCOMMITTED ACCESS ", aTrafodionCatalog.InternalName);
        }

        /// <summary>
        /// Gets the SQL space usage distribution for a given schema
        /// </summary>
        /// <param name="aTrafodionSchema"></param>
        /// <param name="isLive">boolean value to indicate if data should be fetched real time or from repository</param>
        /// <returns></returns>
        static public string GetSchemaSpaceUsageQueryString(TrafodionSchema aTrafodionSchema, bool isLive)
        {
            if (isLive)
            {
                #region Live space

                    return string.Format(
                            "SELECT * FROM ( " +
                            "SELECT 'Tables' AS OBJECT_TYPE, JULIANTIMESTAMP(CURRENT_TIMESTAMP) AS LAST_COLLECTION_TIME, " +
                            "CAST(SUM((PRIMARY_EXTENTS + (SECONDARY_EXTENTS * (MAX_EXTENTS -1))) * 2048) AS NUMERIC(30)) AS TOTAL_MAX_SIZE, " +
                            "CASE " +
                            "WHEN MAX(U.COMPRESSION_TYPE) != 0 THEN CAST(SUM(ISNULL((U.COMPRESSED_EOF_SECTORS * 512),0)) AS NUMERIC(30)) " +
                            "ELSE -1 " +
                            "END AS COMPRESSED_SPACE_USED, " +
                            "CAST(SUM(ISNULL(U.CURRENT_EOF,0)) AS NUMERIC(30)) AS SPACE_USED,ISNULL(MAX(U.COMPRESSION_TYPE),0) AS COMPRESSION_TYPE " +
                            "FROM TABLE(DISK LABEL STATISTICS( USING ( " +
                            "       SELECT * FROM (GET ALL TABLES IN SCHEMA {0}.{1}, NO HEADER, RETURN FULL " +
                            "       NAMES) X(A))) " +
                            ") AS U " +
                            "UNION " + 
                            "SELECT 'Materialized Views' AS OBJECT_TYPE, JULIANTIMESTAMP(CURRENT_TIMESTAMP) AS LAST_COLLECTION_TIME, " +
                            "CAST(SUM((PRIMARY_EXTENTS + (SECONDARY_EXTENTS * (MAX_EXTENTS -1))) * 2048) AS NUMERIC(30)) AS TOTAL_MAX_SIZE, " +
                            "CASE " +
                            "WHEN MAX(U.COMPRESSION_TYPE) != 0 THEN CAST(SUM(ISNULL((U.COMPRESSED_EOF_SECTORS * 512),0)) AS NUMERIC(30)) " +
                            "ELSE -1 " +
                            "END AS COMPRESSED_SPACE_USED, " +
                            "CAST(SUM(ISNULL(U.CURRENT_EOF,0)) AS NUMERIC(30)) AS SPACE_USED,ISNULL(MAX(U.COMPRESSION_TYPE),0) AS COMPRESSION_TYPE " +
                            "FROM TABLE(DISK LABEL STATISTICS( USING ( " +
                            "       SELECT * FROM (GET ALL MVS IN SCHEMA {0}.{1}, NO HEADER, RETURN FULL NAMES) " +
                            "      X(A))) " +
                            ") AS U " +
                            "UNION " + 
                            "SELECT 'Indexes' AS OBJECT_TYPE, JULIANTIMESTAMP(CURRENT_TIMESTAMP) AS LAST_COLLECTION_TIME, " +
                            "CAST(SUM((PRIMARY_EXTENTS + (SECONDARY_EXTENTS * (MAX_EXTENTS -1))) * 2048) AS NUMERIC(30)) AS TOTAL_MAX_SIZE,  " +
                            "CASE " +
                            "WHEN MAX(U.COMPRESSION_TYPE) != 0 THEN CAST(SUM(ISNULL((U.COMPRESSED_EOF_SECTORS * 512),0)) AS NUMERIC(30)) " +
                            "ELSE -1 " +
                            "END AS COMPRESSED_SPACE_USED, " +
                            "CAST(SUM(ISNULL(U.CURRENT_EOF,0)) AS NUMERIC(30)) AS SPACE_USED,ISNULL(MAX(U.COMPRESSION_TYPE),0) AS COMPRESSION_TYPE " +
                            "FROM TABLE(DISK LABEL STATISTICS( INDEXES USING ( " +
                            "       SELECT * FROM (GET ALL INDEXES IN SCHEMA {0}.{1}, NO HEADER, RETURN FULL " +
                            "       NAMES) X(A))) " +
                            ") AS U " +
                            "UNION " + 
                            "SELECT 'Internal Objects' AS OBJECT_TYPE, JULIANTIMESTAMP(CURRENT_TIMESTAMP) AS LAST_COLLECTION_TIME, " +
                            "CAST(SUM((PRIMARY_EXTENTS + (SECONDARY_EXTENTS * (MAX_EXTENTS -1))) * 2048) AS NUMERIC(30)) AS TOTAL_MAX_SIZE,  " +
                            "CASE " +
                            "WHEN MAX(U.COMPRESSION_TYPE) != 0 THEN CAST(SUM(ISNULL((U.COMPRESSED_EOF_SECTORS * 512),0)) AS NUMERIC(30)) " +
                            "ELSE -1 " +
                            "END AS COMPRESSED_SPACE_USED, " +
                            "CAST(SUM(ISNULL(U.CURRENT_EOF,0)) AS NUMERIC(30)) AS SPACE_USED,ISNULL(MAX(U.COMPRESSION_TYPE),0) AS COMPRESSION_TYPE " +
                            "FROM " +
                            "(" +
                            "   SELECT * FROM TABLE(DISK LABEL STATISTICS( IUD_LOG_TABLE USING (  " +
                            "       SELECT * FROM (GET IUDLOG TABLES IN SCHEMA {0}.{1}, NO HEADER, RETURN FULL " +
                            "       NAMES) X(A)))" +
                            "   ) " +
                            "   UNION " + 
                            "   SELECT * FROM TABLE(DISK LABEL STATISTICS( RANGE_LOG_TABLE USING  (  " +
                            "       SELECT * FROM (GET RANGELOG TABLES IN SCHEMA {0}.{1}, NO HEADER, RETURN " +
                            "       FULL NAMES) X(A)))" +
                            "   ) " +
                            "   UNION " + 
                            "   SELECT * FROM TABLE(DISK LABEL STATISTICS( TEMP_TABLE USING ( " +
                            "       SELECT * FROM (GET TEMP_TABLE TABLES IN SCHEMA {0}.{1}, NO HEADER, RETURN " +
                            "       FULL NAMES) X(A)))" +
                            "   )" +
                            ") AS U " +
                            ") AS T1 ORDER BY SPACE_USED DESC ", aTrafodionSchema.TheTrafodionCatalog.ExternalName, aTrafodionSchema.ExternalName);

                #endregion Live space
            }
            else
            {
                #region historical space

                return string.Format(
                        "SELECT T6.OBJECT_TYPE, MAX(LAST_COLLECTION_TIME) AS LAST_COLLECTION_TIME, MAX(TOTAL_MAX_SIZE) AS TOTAL_MAX_SIZE, " +
                        "ISNULL(MAX(COMPRESSED_SPACE_USED),0) AS COMPRESSED_SPACE_USED, " +
                        "ISNULL(MAX(SPACE_USED),0) AS SPACE_USED,ISNULL(MAX(COMPRESSION_TYPE),0) AS  COMPRESSION_TYPE " +
                        "FROM " +
                        "( " +
                        "SELECT CASE  " +
                        "WHEN T2.OBJECT_TYPE = 'BT' AND T2.OBJECT_NAME_SPACE = 'TA' THEN 'Tables' " +
                        "WHEN T2.OBJECT_TYPE = 'IX' THEN 'Indexes' " +
                        "WHEN T2.OBJECT_TYPE = 'MV' THEN 'Materialized Views' " +
                        "ELSE 'Internal Objects' " +
                        "END AS OBJECT_TYPE, " +
                        "MIN(T2.LAST_COLLECTION_TIME) as LAST_COLLECTION_TIME,  " +
                        "CAST(SUM((U.PRIMARY_EXTENTS + (U.SECONDARY_EXTENTS * (U.MAX_EXTENTS -1))) * 2048) AS NUMERIC(30)) AS TOTAL_MAX_SIZE,   " +
                        "CASE " +
                        "WHEN MAX(U.COMPRESSION_TYPE) != 0 THEN CAST(SUM(ISNULL((U.COMPRESSED_EOF_SECTORS * 512),0)) AS NUMERIC(30)) " +
                        "ELSE -1 " +
                        "END AS COMPRESSED_SPACE_USED, " +
                        "CAST(SUM(ISNULL(U.CURRENT_EOF,0)) AS NUMERIC(30)) AS SPACE_USED,ISNULL(MAX(U.COMPRESSION_TYPE),0) AS COMPRESSION_TYPE " +
                        "FROM    " +
                        "(  " +
                        "SELECT T1.CATALOG_NAME, T1.SCHEMA_NAME, T1.OBJECT_NAME, T1.OBJECT_TYPE, ISNULL(MAX(U.COLLECTION_TIME),0) AS LAST_COLLECTION_TIME, T1.OBJECT_NAME_SPACE " +
                        "FROM  " +
                        "( " +
                        "SELECT C.CAT_NAME AS CATALOG_NAME, S.SCHEMA_NAME, OBJECT_NAME, OBJECT_TYPE, OBJECT_NAME_SPACE " +
                        "FROM TRAFODION.SYSTEM_SCHEMA.CATSYS C,  " +
                        "TRAFODION.SYSTEM_SCHEMA.SCHEMATA S, " +
                            "{0}."_MD_".OBJECTS O " +
                        "WHERE C.CAT_NAME = '{1}' AND C.CAT_UID = S.CAT_UID  " +
                        "AND S.SCHEMA_NAME = '{2}' " +
                        "AND S.SCHEMA_UID = O.SCHEMA_UID AND O.OBJECT_TYPE IN ('BT','IX','MV') " +
                        "FOR READ UNCOMMITTED ACCESS " +
                        ") AS T1  " +
                        "LEFT JOIN MANAGEABILITY.INSTANCE_REPOSITORY.SPACE_SQL_SUMMARY_1 AS U  " +
                        "ON T1.CATALOG_NAME = U. CATALOG_NAME  " +
                        "AND T1.SCHEMA_NAME = U.SCHEMA_NAME " +
                        "AND T1.OBJECT_NAME = U.OBJECT_NAME " +
                        "AND T1.OBJECT_TYPE = U.OBJECT_NAME_TYPE " +
                        "AND T1.OBJECT_NAME_SPACE = U.OBJECT_NAME_SPACE " +
                        "GROUP BY T1.CATALOG_NAME, T1.SCHEMA_NAME, T1.OBJECT_NAME, T1.OBJECT_TYPE, T1.OBJECT_NAME_SPACE  " +
                        "FOR READ UNCOMMITTED ACCESS " +
                        ") AS T2   " +
                        "LEFT JOIN MANAGEABILITY.INSTANCE_REPOSITORY.SPACE_SQL_SUMMARY_1 U   " +
                        "ON T2.CATALOG_NAME = U.CATALOG_NAME  " +
                        "AND T2.SCHEMA_NAME = U.SCHEMA_NAME  " +
                        "AND T2.OBJECT_NAME = U.OBJECT_NAME  " +
                        "AND T2.OBJECT_TYPE = U.OBJECT_NAME_TYPE  " +
                        "AND T2.LAST_COLLECTION_TIME = U.COLLECTION_TIME  " +
                        "GROUP BY T2.OBJECT_TYPE, T2.OBJECT_NAME_SPACE  " +
                        "FOR READ UNCOMMITTED ACCESS " +
                        ") AS T6 (OBJECT_TYPE, LAST_COLLECTION_TIME, TOTAL_MAX_SIZE, COMPRESSED_SPACE_USED,SPACE_USED,COMPRESSION_TYPE)" +
                        "GROUP BY T6.OBJECT_TYPE ORDER BY SPACE_USED DESC FOR READ UNCOMMITTED ACCESS "
                        , aTrafodionSchema.TheTrafodionCatalog.ExternalName, aTrafodionSchema.TheTrafodionCatalog.InternalName, aTrafodionSchema.InternalName
                    );
                
                #endregion historical space

                }
        }


        /// <summary>
        /// Gets the SQL space usage for All tables or All Mvs in a schema
        /// </summary>
        /// <param name="aTrafodionSchema">The given schema model</param>
        /// <param name="objectCategory">Either Tables or MVs</param>
        /// <param name="objectType">Object type of table or MV</param>
        /// <param name="objectNameSpace">Object name space of table or MV</param>
        /// <param name="isLive">boolean value to indicate if data should be fetched real time or from repository</param>
        /// <returns></returns>
        static public string GetTablesOrMVsSpaceUsageSummaryQueryString(TrafodionSchema aTrafodionSchema, string objectCategory, string objectType, 
                                                                                            string objectNameSpace, bool isLive)
        {
            if (isLive)
            {
                #region Live space

                return string.Format(
                    " SELECT CASE WHEN MAX(TEMP_HISTOGRAMS.STATS_TIME) IS NULL THEN 0 ELSE JULIANTIMESTAMP(MAX(TEMP_HISTOGRAMS.STATS_TIME)) END AS LAST_UPDATE_STATS_TIME"
                + " ,JULIANTIMESTAMP(CURRENT_TIMESTAMP) AS LAST_COLLECTION_TIME "
                + " ,TEMP_TABLE.OBJECT_NAME AS OBJECT_NAME"
                + " ,SUM(TEMP_TABLE.ROW_COUNT) AS ROW_COUNT"
                + " ,SUM(TEMP_TABLE.MAX_SIZE) AS TOTAL_MAX_SIZE"
                + " ,SUM(TEMP_TABLE.COMPRESSED_SPACE_USED) AS COMPRESSED_SPACE_USED"
                + " ,SUM(TEMP_TABLE.SPACE_USED) AS SPACE_USED"
                + " ,MAX(TEMP_TABLE.COMPRESSION_TYPE) AS COMPRESSION_TYPE"
                + " ,COUNT(TEMP_TABLE.NUMBER_PARTITIONS) AS NUMBER_PARTITIONS, MAX(TEMP_HISTOGRAMS.STATS_ROWCOUNT) AS STATS_ROWCOUNT"
                + ", MAX(TEMP_TABLE.LAST_MOD_TIME) AS LAST_MOD_TIME, MAX(TEMP_TABLE.LAST_OPEN_TIME) AS LAST_OPEN_TIME "
                + ", CAST(SUM(RFORK_EOF) AS NUMERIC(30)) AS OVERHEAD, CAST(SUM(ACCESS_COUNTER) AS NUMERIC(30)) AS ACCESS_COUNTER "
                + " FROM "
                + " (SELECT OBJECT_NAME, ROW_COUNT, CAST(((PRIMARY_EXTENTS + (SECONDARY_EXTENTS * (MAX_EXTENTS -1))) * 2048) AS NUMERIC(40,0)), " 
                + " CASE "
                + " WHEN MAX(U.COMPRESSION_TYPE) != 0 THEN CAST(SUM(ISNULL((U.COMPRESSED_EOF_SECTORS * 512),0)) AS NUMERIC(30)) "
                + " ELSE -1 "
                + " END AS COMPRESSED_SPACE_USED, "
                + " CAST(SUM(ISNULL(U.CURRENT_EOF,0)) AS NUMERIC(30)) AS SPACE_USED, "
                + " MAX(U.COMPRESSION_TYPE) AS COMPRESSION_TYPE, "
                + " PARTITION_NUM, LAST_MOD_TIME, LAST_OPEN_TIME, RFORK_EOF, ACCESS_COUNTER "
                + " FROM TABLE(DISK LABEL STATISTICS( USING"
                + " (SELECT * FROM (GET ALL {0} IN SCHEMA {1}.{2}, NO HEADER, RETURN FULL NAMES) X(A) ))) AS U " 
                + "GROUP BY OBJECT_NAME,ROW_COUNT,PRIMARY_EXTENTS,SECONDARY_EXTENTS,MAX_EXTENTS,PARTITION_NUM, LAST_MOD_TIME, LAST_OPEN_TIME, RFORK_EOF, ACCESS_COUNTER "
                + " ) TEMP_TABLE(OBJECT_NAME,ROW_COUNT,MAX_SIZE,COMPRESSED_SPACE_USED,SPACE_USED,COMPRESSION_TYPE,NUMBER_PARTITIONS,LAST_MOD_TIME,LAST_OPEN_TIME, RFORK_EOF, ACCESS_COUNTER)"
                + " , TRAFODION.SYSTEM_SCHEMA.SCHEMATA T1"
                + " , TRAFODION.SYSTEM_SCHEMA.CATSYS T2"
                + " ,{1}."_MD_".OBJECTS T3"
                + " LEFT JOIN (SELECT MAX(T4.STATS_TIME), MAX(ROWCOUNT), T4.TABLE_UID FROM {1}.{2}.HISTOGRAMS T4 GROUP BY T4.TABLE_UID)  TEMP_HISTOGRAMS(STATS_TIME, STATS_ROWCOUNT, TABLE_UID) ON TEMP_HISTOGRAMS.TABLE_UID = T3.OBJECT_UID"
                + " WHERE "
                + " TEMP_TABLE.OBJECT_NAME = T3.OBJECT_NAME"
                + " AND T3.OBJECT_TYPE = '{5}'"
                + " AND T3.OBJECT_NAME_SPACE = '{6}'"
                + " AND T1.SCHEMA_NAME = '{4}'"
                + " AND T1.CAT_UID = T2.CAT_UID"
                + " AND T2.CAT_NAME = '{3}'"
                + " AND T3.SCHEMA_UID = T1.SCHEMA_UID"
                + " GROUP BY (TEMP_TABLE.OBJECT_NAME) ORDER BY TEMP_TABLE.OBJECT_NAME FOR READ UNCOMMITTED ACCESS"
                , new object[] { objectCategory, aTrafodionSchema.TheTrafodionCatalog.ExternalName
                            , aTrafodionSchema.ExternalName,
                            aTrafodionSchema.TheTrafodionCatalog.InternalName,
                            aTrafodionSchema.InternalName,
                            objectType, 
                            objectNameSpace
                            });
                #endregion Live space
            }
            else
            {
                #region historical space

                return String.Format(
                    "SELECT CASE WHEN MAX(T7.STATS_TIME) IS NULL THEN 0 ELSE JULIANTIMESTAMP(MAX(T7.STATS_TIME)) END AS LAST_UPDATE_STATS_TIME, " +
                    "MIN(LAST_COLLECTION_TIME) as LAST_COLLECTION_TIME, OBJECT_NAME, OBJECT_UID, " +
                    "MAX(ROW_COUNT) AS ROW_COUNT, MAX(TOTAL_MAX_SIZE) AS TOTAL_MAX_SIZE, MAX(COMPRESSED_SPACE_USED) AS COMPRESSED_SPACE_USED, " +
                    "MAX(SPACE_USED) AS SPACE_USED,MAX(COMPRESSION_TYPE) AS COMPRESSION_TYPE," +
                    "MAX(NUMBER_PARTITIONS) AS NUMBER_PARTITIONS, MAX(T7.ROWCOUNT) AS STATS_ROWCOUNT, " +
                    "MAX(OVERHEAD) AS OVERHEAD, MAX(ACCESS_COUNTER) AS ACCESS_COUNTER FROM " +
                    "( " +
                    "SELECT MAX(LAST_COLLECTION_TIME) AS LAST_COLLECTION_TIME, " +
                    "T5.OBJECT_NAME, T5.OBJECT_UID, COUNT(PARTITION_NUM) AS NUMBER_PARTITIONS," +
                    "SUM(U.ROW_COUNT) AS ROW_COUNT, " +
                    "CAST(SUM((U.PRIMARY_EXTENTS + (U.SECONDARY_EXTENTS * (U.MAX_EXTENTS -1))) * 2048) AS NUMERIC(30)) AS TOTAL_MAX_SIZE,  " +
                    " CASE " +
                    " WHEN MAX(U.COMPRESSION_TYPE) != 0 THEN CAST(SUM(ISNULL((U.COMPRESSED_EOF_SECTORS * 512),0)) AS NUMERIC(30)) " +
                    " ELSE -1 " +
                    " END AS COMPRESSED_SPACE_USED, " +
                    " CAST(SUM(ISNULL(U.CURRENT_EOF,0)) AS NUMERIC(30)) AS SPACE_USED, " +
                    " ISNULL(MAX(U.COMPRESSION_TYPE),0) AS COMPRESSION_TYPE, " +
                    "MAX(LAST_MOD_TIME) AS LAST_MOD_TIME, MAX(LAST_OPEN_TIME) AS LAST_OPEN_TIME,  " +
                    "CAST(SUM(U.SPACE_INTERNAL_OVERHEAD) AS NUMERIC(30)) AS OVERHEAD, CAST(SUM(U.ACCESS_COUNTER) AS NUMERIC(30)) AS ACCESS_COUNTER " +
                    "FROM  " +
                    "( " +
                        "SELECT T4.OBJECT_NAME, T4.OBJECT_UID, ISNULL(MAX(U.COLLECTION_TIME),0) AS LAST_COLLECTION_TIME  " +
                        "FROM  " +
                        "( " +
                            "SELECT T3.OBJECT_NAME, T3.OBJECT_UID " +
                            "FROM TRAFODION.SYSTEM_SCHEMA.CATSYS T1, TRAFODION.SYSTEM_SCHEMA.SCHEMATA T2, {0}."_MD_".OBJECTS T3 " +
                            "WHERE T1.CAT_NAME = '{2}' " +
                            "AND T1.CAT_UID = T2.CAT_UID " +
                            "AND T2.SCHEMA_NAME = '{3}' " +
                            "AND T2.SCHEMA_UID = T3.SCHEMA_UID " +
                            "AND T3.OBJECT_TYPE = '{4}' " +
                            "AND T3.OBJECT_NAME_SPACE = '{5}' " +
                            "FOR READ UNCOMMITTED ACCESS " +
                        ") AS T4 " +
                        "LEFT JOIN MANAGEABILITY.INSTANCE_REPOSITORY.SPACE_SQL_SUMMARY_2 AS U  " +
                        "ON U.CATALOG_NAME = '{2}'  " +
                        "AND U.SCHEMA_NAME = '{3}' " +
                        "AND U.OBJECT_NAME = T4.OBJECT_NAME  " +
                        "AND U.OBJECT_NAME_TYPE = '{4}' " +
                        "AND U.OBJECT_NAME_SPACE = '{5}' " +
                        "GROUP BY T4.OBJECT_NAME, T4.OBJECT_UID " +
                        "FOR READ UNCOMMITTED ACCESS " +
                    ")AS T5 " +
                    "LEFT JOIN MANAGEABILITY.INSTANCE_REPOSITORY.SPACE_SQL_SUMMARY_2 U  " +
                    "ON U.CATALOG_NAME = '{2}'  " +
                    "AND U.SCHEMA_NAME = '{3}' " +
                    "AND U.OBJECT_NAME = T5.OBJECT_NAME  " +
                    "AND U.COLLECTION_TIME = T5.LAST_COLLECTION_TIME " +
                    "AND U.OBJECT_NAME_TYPE = '{4}' " +
                    "AND U.OBJECT_NAME_SPACE = '{5}' " +
                    "GROUP BY T5.OBJECT_NAME, T5.OBJECT_UID " +
                    "FOR READ UNCOMMITTED ACCESS " +
                    ") AS T6 " +
                    "LEFT JOIN {0}.{1}.HISTOGRAMS T7 " +
                    "ON T7.TABLE_UID = T6.OBJECT_UID " +
                    "GROUP BY T6.OBJECT_NAME, T6.OBJECT_UID ORDER BY OBJECT_NAME " +
                    "FOR READ UNCOMMITTED ACCESS "
                , new object[] { aTrafodionSchema.TheTrafodionCatalog.ExternalName
                            , aTrafodionSchema.ExternalName,
                            aTrafodionSchema.TheTrafodionCatalog.InternalName,
                            aTrafodionSchema.InternalName,
                            objectType, 
                            objectNameSpace
                            });

                #endregion historical space
            }
        }

        /// <summary>
        /// Gets the SQL space usage for all indexes in a schema or all indexes on a table/MV
        /// </summary>
        /// <param name="aTrafodionObject">A Schema model or table model or MV model</param>
        /// <param name="isLive">boolean value to indicate if data should be fetched real time or from repository</param>
        /// <returns></returns>
        static public string GetIndexesSpaceUsageSummaryQueryString(TrafodionObject aTrafodionObject, bool isLive)
        {
            string baseObjectClause = "";
            string catalogName = "";
            string schemaName = "";

            if (aTrafodionObject is TrafodionSchema)
            {
                baseObjectClause = "IN SCHEMA";
                catalogName = ((TrafodionSchema)aTrafodionObject).TheTrafodionCatalog.InternalName;
                schemaName = ((TrafodionSchema)aTrafodionObject).InternalName;
            }
            if (aTrafodionObject is TrafodionTable)
            {
                baseObjectClause = "ON TABLE";
                catalogName = ((TrafodionTable)aTrafodionObject).TheTrafodionCatalog.InternalName;
                schemaName = ((TrafodionTable)aTrafodionObject).TheTrafodionSchema.InternalName;
            }
            if (aTrafodionObject is TrafodionMaterializedView)
            {
                baseObjectClause = "ON MV";
                catalogName = ((TrafodionMaterializedView)aTrafodionObject).TheTrafodionCatalog.InternalName;
                schemaName = ((TrafodionMaterializedView)aTrafodionObject).TheTrafodionSchema.InternalName;
            }

            if (isLive)
            {
                #region Live space

                return string.Format(
                    String.Format(
                        " SELECT JULIANTIMESTAMP(CURRENT_TIMESTAMP) as LAST_COLLECTION_TIME, TEMP_TABLE.OBJECT_NAME"
                    + " ,SUM(TEMP_TABLE.ROW_COUNT) AS ROW_COUNT, CAST(SUM(TEMP_TABLE.MAX_SIZE) AS NUMERIC(40,0)) AS TOTAL_MAX_SIZE"
                    + " ,CAST(SUM(TEMP_TABLE.COMPRESSED_CURRENT_EOF) AS NUMERIC(40,0)) AS COMPRESSED_SPACE_USED "
                    + " ,CAST(SUM(TEMP_TABLE.CURRENT_EOF) AS NUMERIC(40,0)) AS SPACE_USED "
                    + " ,MAX(TEMP_TABLE.COMPRESSION_TYPE) AS COMPRESSION_TYPE "
                    + " ,CAST(SUM(RFORK_EOF) AS NUMERIC(30)) AS OVERHEAD, CAST(SUM(ACCESS_COUNTER) AS NUMERIC(30)) AS ACCESS_COUNTER "
                    + " FROM "
                    + " (SELECT OBJECT_NAME, ROW_COUNT, ((PRIMARY_EXTENTS + (SECONDARY_EXTENTS * (MAX_EXTENTS -1))) * 2048) AS MAX_SIZE, "
                    + " CASE "
                    + " WHEN MAX(U.COMPRESSION_TYPE) != 0 THEN CAST(SUM(ISNULL((U.COMPRESSED_EOF_SECTORS * 512),0)) AS NUMERIC(30)) "
                    + " ELSE -1 "
                    + " END AS COMPRESSED_SPACE_USED, "
                    + " CAST(SUM(ISNULL(U.CURRENT_EOF,0)) AS NUMERIC(30)) AS SPACE_USED, "
                    + " ISNULL(MAX(U.COMPRESSION_TYPE),0) AS COMPRESSION_TYPE, "
                    + " RFORK_EOF, ACCESS_COUNTER FROM TABLE(DISK LABEL STATISTICS( INDEXES USING "
                    + " (SELECT * FROM (GET ALL INDEXES {0} {1}, NO HEADER, RETURN FULL NAMES) X(A) ))) AS U"
                    + " GROUP BY OBJECT_NAME, ROW_COUNT,PRIMARY_EXTENTS,SECONDARY_EXTENTS,MAX_EXTENTS,RFORK_EOF, ACCESS_COUNTER "
                    + " ) TEMP_TABLE(OBJECT_NAME,ROW_COUNT,MAX_SIZE,COMPRESSED_CURRENT_EOF,CURRENT_EOF,COMPRESSION_TYPE,RFORK_EOF, ACCESS_COUNTER)"
                    + " GROUP BY (TEMP_TABLE.OBJECT_NAME) ORDER BY OBJECT_NAME FOR READ UNCOMMITTED ACCESS;"
                , baseObjectClause, aTrafodionObject.RealAnsiName)
                );

                #endregion Live space
            }
            else
            {
                #region historical space

                return String.Format(
                "SELECT MIN(LAST_COLLECTION_TIME) AS LAST_COLLECTION_TIME, " +
                "T5.OBJECT_NAME, SUM(U.ROW_COUNT) AS ROW_COUNT, " +
                "CAST(SUM((U.PRIMARY_EXTENTS + (U.SECONDARY_EXTENTS * (U.MAX_EXTENTS -1))) * 2048) AS NUMERIC(30)) AS TOTAL_MAX_SIZE, " +
                " CASE " +
                " WHEN MAX(U.COMPRESSION_TYPE) != 0 THEN CAST(SUM(ISNULL((U.COMPRESSED_EOF_SECTORS * 512),0)) AS NUMERIC(30)) " +
                " ELSE -1 " +
                " END AS COMPRESSED_SPACE_USED, " +
                " CAST(SUM(ISNULL(U.CURRENT_EOF,0)) AS NUMERIC(30)) AS SPACE_USED, " +
                " MAX(ISNULL(U.COMPRESSION_TYPE,0)) AS COMPRESSION_TYPE, " +
                "CAST(SUM(U.SPACE_INTERNAL_OVERHEAD) AS NUMERIC(30)) AS OVERHEAD, CAST(SUM(U.ACCESS_COUNTER) AS NUMERIC(30)) AS ACCESS_COUNTER  " +
                "FROM " +
                "( " +
                    "SELECT T4.OBJECT_NAME,  ISNULL(MAX(U.COLLECTION_TIME),0) AS LAST_COLLECTION_TIME " +
                    "FROM " +
                    "( " +
                        "SELECT * FROM (GET ALL INDEXES {0} {1}, NO HEADER) X(OBJECT_NAME)" +
                    ") AS T4 " +
                    "LEFT JOIN MANAGEABILITY.INSTANCE_REPOSITORY.SPACE_SQL_SUMMARY_2 AS U " +
                    "ON U.CATALOG_NAME = '{2}' " +
                    "AND U.SCHEMA_NAME = '{3}' " +
                    "AND U.OBJECT_NAME = T4.OBJECT_NAME " +
                    "AND U.OBJECT_NAME_TYPE = 'IX' " +
                    "GROUP BY T4.OBJECT_NAME " +
                    "FOR READ UNCOMMITTED ACCESS " +
                ")AS T5 " +
                "LEFT JOIN MANAGEABILITY.INSTANCE_REPOSITORY.SPACE_SQL_SUMMARY_2 U " +
                "ON U.CATALOG_NAME = '{2}' " +
                "AND U.SCHEMA_NAME = '{3}' " +
                "AND U.OBJECT_NAME = T5.OBJECT_NAME " +
                "AND U.COLLECTION_TIME = T5.LAST_COLLECTION_TIME " +
                "AND U.OBJECT_NAME_TYPE = 'IX' " +
                "GROUP BY T5.OBJECT_NAME ORDER BY T5.OBJECT_NAME " +
                "FOR READ UNCOMMITTED ACCESS "
                , new object[] {
                baseObjectClause, aTrafodionObject.RealAnsiName, catalogName, schemaName }
                );

                #endregion historical space
            }
        }

        /// <summary>
        /// Get the total SQL space for a given system
        /// </summary>
        /// <param name="aConnection"></param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteSelectTotalSQLSpace(Connection aConnection)
        {
            OdbcCommand theQuery = new OdbcCommand("INFO DISK ALL", aConnection.OpenOdbcConnection);
            return ExecuteReader(theQuery);
        }

        static public int ExecuteValidationView(Connection aConnection, TrafodionView aTrafodionView, bool cascade, out DataTable warningsTable)
        {            
            OdbcCommand odbcCommand = new OdbcCommand(String.Format("ALTER VIEW {0} COMPILE {1}", aTrafodionView.RealAnsiName, cascade ? "CASCADE" : ""));
            odbcCommand.Connection = aConnection.OpenOdbcConnection;
            odbcCommand.CommandTimeout = 0;
            return Utilities.ExecuteNonQuery(odbcCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Database, TRACE_SUB_AREA_NAME, true, out warningsTable);
        }

    }
}


