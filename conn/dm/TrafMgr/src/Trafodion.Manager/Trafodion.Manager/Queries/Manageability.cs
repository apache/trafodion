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
using System.Collections.Generic;
using System.Data.Odbc;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.Framework.Queries
{
    /// <summary>
    /// This class contains information and functions which pertain to the Manageability catalog.
    /// </summary>
    public class Manageability
    {
        #region Fields

        /// <summary>
        /// The fully qualified pathname to the TrafodionManager schema.
        /// </summary>
        public static readonly string TrafodionManagerSchemaName = "MANAGEABILITY.TrafodionManager";

        /// <summary>
        /// The fully qualified pathname to the System Message table.
        /// </summary>
        public static readonly string SystemMessageTableName = Manageability.TrafodionManagerSchemaName + ".SYSTEM_MESSAGE";

        /// <summary>
        /// A comma seperated list of the administrators of the Manageability catalog.
        /// </summary>
        public static readonly string Administrators = "\"super.services\", \"role.mgr\", \"role.dba\"";

        /// <summary>
        /// The maximum message length for the MESSAGE_TEXT column.
        /// </summary>
        public static readonly int MaxMessageSequenceLength = 32683;

        private const string TRACE_SUB_AREA_NAME = "Manageability";

        #endregion

        /// <summary>
        /// Indicates if the given user's role has administrative abilities over this schema.
        /// </summary>
        /// <param name="role"></param>
        /// <returns></returns>
        static public bool IsAdministrator(string role)
        {
            if (role == null)
            {
                return false;
            }

            if (role.Equals("role.mgr", StringComparison.OrdinalIgnoreCase))
            {
                return true;
            }
            else if (role.Equals("role.dba", StringComparison.OrdinalIgnoreCase))
            {
                return true;
            }
            else if (role.Equals("super.services", StringComparison.OrdinalIgnoreCase))
            {
                return true;
            }

            return false;
        }

        /// <summary>
        /// Creates the Manageability.TrafodionManager schema.
        /// Since it prefers to use a transaction to perform its work, it is recommended
        /// that aConnection not already have a transaction started.
        /// If it can start its own transaction it will commit when complete or rollback
        /// its work on any error. Otherwise, it will not cleanup on an error.
        /// It will always throw an OdbcException if one occurs regardless if it can
        /// start it's own transaction or not.
        /// </summary>
        /// <param name="aConnection">The connection to use to create the schema.</param>
        static public void ExecuteCreateTrafodionManagerSchema(Connection aConnection)
        {
            // Only a known administrator can create the TrafodionManager schema.
            if (!IsAdministrator(aConnection.TheConnectionDefinition.RoleName))
            {
                return;
            }

            int rowsaffected;
            OdbcCommand command = new OdbcCommand();
            command.Connection = aConnection.OpenOdbcConnection;

            // Try to start a transaction.
            bool ownTransaction = StartTransaction(aConnection);

            try
            {
                // Create the schema.
                command.CommandText = string.Format(
                    "CREATE SCHEMA {0};",
                    new Object[] { TrafodionManagerSchemaName }
                    );
                rowsaffected = Utilities.ExecuteNonQuery(command, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Framework, TRACE_SUB_AREA_NAME, true);

                // Grant administrative privileges to the schema to the administrators.
                command.CommandText = string.Format(
                    "GRANT ALL PRIVILEGES ON SCHEMA {0} TO {1} WITH GRANT OPTION;",
                    new Object[] { TrafodionManagerSchemaName, Administrators }
                    );
                rowsaffected = Utilities.ExecuteNonQuery(command, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Framework, TRACE_SUB_AREA_NAME, true);

                // Only commit if we're in a transaction.
                if (ownTransaction)
                {
                    command.CommandText = "COMMIT;";
                    rowsaffected = Utilities.ExecuteNonQuery(command, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Framework, TRACE_SUB_AREA_NAME, true);

                }
            }
            catch (OdbcException e)
            {
                // Cleanup if necessary.
                if (ownTransaction)
                {
                    command.CommandText = "ROLLBACK;";
                    rowsaffected = Utilities.ExecuteNonQuery(command, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Framework, TRACE_SUB_AREA_NAME, true);
                }

                throw e;
            }
        }

        /// <summary>
        /// Creates the system message table and schema if necesary.
        /// </summary>
        /// <param name="aConnection">The connection used to connect to the server.</param>
        /// <param name="createSchema">Indicates that the schema needs to be created as well.</param>
        static public void ExecuteCreateSystemMessageTable(Connection aConnection, bool createSchema)
        {
            // Only publishers have the ability to create the schema/table.
            if (!IsAdministrator(aConnection.TheConnectionDefinition.RoleName))
            {
                return;
            }

            // Create the schema if requested.
            if (createSchema)
            {
                Manageability.ExecuteCreateTrafodionManagerSchema(aConnection);
            }

            int rowsaffected;
            OdbcCommand command = new OdbcCommand();
            command.Connection = aConnection.OpenOdbcConnection;

            // Try to start a transaction.
            bool ownTransaction = StartTransaction(aConnection);

            try
            {
                command.CommandText = string.Format(
                    "CREATE TABLE {0} ( " +
                    "  SEQUENCE INTEGER UNSIGNED DEFAULT 0 NOT NULL NOT DROPPABLE, " +
                    "  LASTUPDATED TIMESTAMP (6) DEFAULT CURRENT_TIMESTAMP NOT NULL NOT DROPPABLE, " +
                    "  MESSAGE_TEXT VARCHAR ({1}) CHARACTER SET ISO88591 DEFAULT '' NOT NULL NOT DROPPABLE, " +
                    "  PRIMARY KEY (SEQUENCE ASC) NOT DROPPABLE) " +
                    "ATTRIBUTE BLOCKSIZE 32768;",
                    new Object[] { SystemMessageTableName, MaxMessageSequenceLength });
                rowsaffected = Utilities.ExecuteNonQuery(command, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Framework, TRACE_SUB_AREA_NAME, true);

                // Select privileges.
                command.CommandText = string.Format(
                    "GRANT SELECT ON {0} TO PUBLIC;",
                    new Object[] { SystemMessageTableName }
                    );
                rowsaffected = Utilities.ExecuteNonQuery(command, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Framework, TRACE_SUB_AREA_NAME, true);

                if (ownTransaction)
                {
                    command.CommandText = "COMMIT;";
                    rowsaffected = Utilities.ExecuteNonQuery(command, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Framework, TRACE_SUB_AREA_NAME, true);
                }
            }
            catch (OdbcException e)
            {
                // Cleanup if necessary.
                if (ownTransaction)
                {
                    command.CommandText = "ROLLBACK;";
                    try
                    {
                        rowsaffected = Utilities.ExecuteNonQuery(command, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Framework, TRACE_SUB_AREA_NAME, true);
                    }
                    catch (Exception ex)
                    {
                    }
                }
                throw e;
            }
        }

        /// <summary>
        /// Starts a transaction for the given connection.
        /// </summary>
        /// <param name="aConnection">The connection to check.</param>
        /// <returns>True if the connection is in a transaction, otherwise false.</returns>
        private static bool StartTransaction(Connection aConnection)
        {
            bool ownTransaction = true;
            int rowsaffected;

            // Determine if we can start a transaction. If not, do not
            // rollback or commit later.
            try
            {
                OdbcCommand command = new OdbcCommand("BEGIN WORK;");
                command.Connection = aConnection.OpenOdbcConnection;
                rowsaffected = Utilities.ExecuteNonQuery(command, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Framework, TRACE_SUB_AREA_NAME, true);
            }
            catch (OdbcException e)
            {
                // Error -8603 means that we're already in a transaction.
                bool error8603Found = false;
                foreach (OdbcError error in e.Errors)
                {
                    if (error.NativeError == -8603)
                        error8603Found = true;
                }
                if (e.Errors.Count > 0 && error8603Found)
                {
                    ownTransaction = false;
                }
                else
                {
                    throw e;
                }
            }

            return ownTransaction;
        }
    }
}
