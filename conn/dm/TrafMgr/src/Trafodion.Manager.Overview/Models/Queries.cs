//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2012-2015 Hewlett-Packard Development Company, L.P.
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
using System.Data.Odbc;
using System.Text;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using System.Data;

namespace Trafodion.Manager.OverviewArea.Models
{
    /// <summary>
    /// Audit Logging Configuration queries
    /// </summary>
    static public class Queries
    {
        public const string TRACE_SUB_AREA_NAME_AUDIT_LOG = "Audit Logging Configuration Queries";
        public const string TRACE_SUB_AREA_NAME_HEALTH_STATE = "Health/State";

        private static string health_warpper_script = "health_diagnose";

        static public OdbcDataReader DisplayAuditLogsConfig(Connection aConnection)
        {
            string cmd = string.Empty;
            //if (aConnection.TheConnectionDefinition.ServerVersion <= ConnectionDefinition.SERVER_VERSION.SQ140)
            //{
                cmd = "CALL TRAFODION.TRAFODION_SP.DISPLAY_SECURITY_AUDIT_CONFIGURATION();";
            //}
            //else
            //{
            //    cmd = "CALL TRAFODION.TRAFODION_SP.DISPLAY_SECURITY_AUDIT_CONFIGURATION_2('ALL');";
            //}
            
            OdbcCommand theQuery = new OdbcCommand(cmd);
            theQuery.Connection = aConnection.OpenOdbcConnection;
            OdbcDataReader reader = ExecuteReader(theQuery);
            return reader;
        }


        static public int ResetAuditLogsConfig(Connection aConnection,string logType)
        {            
            string cmd = string.Empty;
            //if (aConnection.TheConnectionDefinition.ServerVersion <= ConnectionDefinition.SERVER_VERSION.SQ140)
            //{
                cmd = "CALL TRAFODION.TRAFODION_SP.RESET_SECURITY_AUDIT_CONFIGURATION();";
            //}
            //else
            //{
            //    cmd = string.Format("CALL TRAFODION.TRAFODION_SP.RESET_SECURITY_AUDIT_CONFIGURATION('{0}');",logType);
            //}
            OdbcCommand theQuery = new OdbcCommand(cmd);

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteNonQuery(theQuery);
        }

        static public int RestoreAuditLogsConfig(Connection aConnection, string logType)
        {
            string cmd = string.Empty;
            //if (aConnection.TheConnectionDefinition.ServerVersion <= ConnectionDefinition.SERVER_VERSION.SQ140)
            //{
                cmd = "CALL TRAFODION.TRAFODION_SP.RESTORE_SECURITY_AUDIT_CONFIGURATION();";
            //}
            //else
            //{
            //    cmd = string.Format("CALL TRAFODION.TRAFODION_SP.RESTORE_SECURITY_AUDIT_CONFIGURATION('{0}');", logType);
            //}
            OdbcCommand theQuery = new OdbcCommand(cmd);

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteNonQuery(theQuery);
        }

        static public OdbcDataReader DrillDownHealthStateInfo(Connection aConnection, string aSubLayer, bool allInfo)
        {
            string cmd = "CALL TRAFODION.TRAFODION_SP.RUN_SCRIPT(?, ?, ?, ?)";

            OdbcCommand theQuery = new OdbcCommand(cmd, aConnection.OpenOdbcConnection);
            theQuery.CommandType = CommandType.StoredProcedure;

            OdbcParameter param1 = theQuery.Parameters.Add("@scriptName", OdbcType.Text);
            param1.Direction = System.Data.ParameterDirection.Input;
            param1.Value = health_warpper_script;

            OdbcParameter param2 = theQuery.Parameters.Add("@scriptParams", OdbcType.Text);
            param2.Direction = System.Data.ParameterDirection.Input;
            //This is to process a problem that Sublayer may contain multiple words. 
            //The format would look like: '"PROCESS BALANCE" "ALL"' or '"WMS" "FAILURE"' etc.
            param2.Value = "\"" + aSubLayer + "\"" + " " + (allInfo ? "\"ALL\"" : "\"FAILURE\""); 

            OdbcParameter param3 = theQuery.Parameters.Add("@tableName", OdbcType.Text);
            param3.Direction = System.Data.ParameterDirection.Input;
            param3.Value = "";

            OdbcParameter param4 = theQuery.Parameters.Add("@outResult", OdbcType.Text, 500);
            param4.Direction = System.Data.ParameterDirection.Output;            

            OdbcDataReader reader = Utilities.ExecuteReader(theQuery, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Monitoring, TRACE_SUB_AREA_NAME_HEALTH_STATE, true);
            return reader;
        }

        static public int AlterAuditLogsConfig(Connection aConnection, string alterCommand)
        {     

            OdbcCommand theQuery = new OdbcCommand(alterCommand);

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteNonQuery(theQuery);
        }


        static private int ExecuteNonQuery(OdbcCommand anOpenCommand)
        {
            return Utilities.ExecuteNonQuery(anOpenCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.UserManagement, TRACE_SUB_AREA_NAME_AUDIT_LOG, false);
        }

        static private OdbcDataReader ExecuteReader(OdbcCommand anOpenCommand)
        {
            return Utilities.ExecuteReader(anOpenCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.UserManagement, TRACE_SUB_AREA_NAME_AUDIT_LOG, false);
        }
    }
}
