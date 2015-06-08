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
using Trafodion.Manager.DatabaseArea;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.UniversalWidget;

namespace Trafodion.Manager.OverviewArea.Models
{
    /// <summary>
    /// Repository Data provider for the Alerts widget
    /// </summary>
    public class RepoAlertDataProvider : DatabaseDataProvider
    {
        public bool IsValidationRun = true;
        public bool ViewExists = false;
        public bool UserCanView = false;
        public bool UserCanUpdate = false;

        public const string PROBLEM_MGMT_TABLE = "MANAGEABILITY.INSTANCE_REPOSITORY.PROBLEM_INSTANCE_TABLE";
        public const string PROBLEM_MGMT_VIEW = "MANAGEABILITY.INSTANCE_REPOSITORY.PROBLEM_INSTANCE_1";
        public const string PROBLEM_MGMT_UPDATE_PROCEDURE = "TRAFODION.SP.UPDATE_PROBLEM";

        private const string TRACE_SUB_AREA_NAME = "System Alerts";

        public RepoAlertDataProvider()
            : base(new DatabaseDataProviderConfig())
        {

        }

        public override void DoFetchData(Trafodion.Manager.Framework.WorkerThread worker, System.ComponentModel.DoWorkEventArgs e)
        {
            DatabaseDataProviderConfig dbConfig = DataProviderConfig as DatabaseDataProviderConfig;
            if (IsValidationRun)
            {
                ViewExists = DoesAlertsViewExist(dbConfig.ConnectionDefinition);
                if (ViewExists)
                {
                    UserCanView = CanUserView(dbConfig.ConnectionDefinition, PROBLEM_MGMT_VIEW);
                    if (UserCanView)
                    {
                        UserCanUpdate = CanUserUpdate(dbConfig.ConnectionDefinition, true);
                    }
                    else
                    {
                        return;
                    }
                }
                else
                {
                    return;
                }
            }
            else
            {
                base.DoFetchData(worker, e);
            }
        }

        #region Helper methods

        /// <summary>
        /// Checks to see if the Alerts SQL view exist. This check is used to enable/disable the Alerts widget 
        /// </summary>
        /// <param name="aConnectionDefinition">Connection definition that identifies the system</param>
        /// <returns>True or False</returns>
        public static bool DoesAlertsViewExist(ConnectionDefinition aConnectionDefinition)
        {
            Connection aConnection = null;
            OdbcDataReader reader = null;
            bool result = false;

            try
            {
                aConnection = new Connection(aConnectionDefinition);

                //If the info object command returns any rows, it means the object exists
                OdbcCommand odbcCommand = new OdbcCommand("INFO OBJECT " + PROBLEM_MGMT_VIEW,
                                                aConnection.OpenOdbcConnection);
                //reader = odbcCommand.ExecuteReader();
                reader = Utilities.ExecuteReader(odbcCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Monitoring, TRACE_SUB_AREA_NAME, true);

                if (reader.Read())
                {
                    //Object exists return true
                    return true;
                }
            }
            catch (Exception ex)
            {
                result = false;
            }
            finally
            {
                try
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
                catch (Exception e)
                {
                }
            }

            //Object does not exist, return false
            return result;
        }

        /// <summary>
        /// Checks to see if the current logged on user has privileges to view the Alert details
        /// </summary>
        /// <param name="aConnectionDefinition">Connection definition with the logged on user information</param>
        /// <returns>True or False</returns>
        public static bool CanUserView(ConnectionDefinition aConnectionDefinition, string ansiViewName)
        {

            bool canUserView = false;

            try
            {
                //Load the SQL model for the Alerts View and look up the privileges
                TrafodionSystem aTrafodionSystem = TrafodionSystem.FindTrafodionSystem(aConnectionDefinition);
                string[] nameParts = Utilities.CrackSQLAnsiName(ansiViewName);

                TrafodionCatalog aTrafodionCatalog = null;
                TrafodionSchema aTrafodionSchema = null;
                TrafodionView alertsView = null;

                if (TrafodionObject.Exists<TrafodionCatalog>(aTrafodionSystem.TrafodionCatalogs, nameParts[0]))
                {
                    aTrafodionCatalog = aTrafodionSystem.FindCatalog(nameParts[0]);
                }
                else
                {
                    aTrafodionCatalog = aTrafodionSystem.LoadTrafodionCatalog(nameParts[0]);
                }

                if (aTrafodionCatalog != null)
                {
                    if (TrafodionObject.Exists<TrafodionSchema>(aTrafodionCatalog.TrafodionSchemas, nameParts[1]))
                    {
                        aTrafodionSchema = aTrafodionCatalog.FindSchema(nameParts[1]);
                    }
                    else
                    {
                        aTrafodionSchema = aTrafodionCatalog.LoadTrafodionSchema(nameParts[1]);
                    }
                    if (aTrafodionSchema != null)
                    {
                        if (TrafodionObject.Exists<TrafodionView>(aTrafodionSchema.TrafodionViews, nameParts[2]))
                        {
                            alertsView = aTrafodionSchema.FindView(nameParts[2]);
                        }
                        else
                        {
                            alertsView = aTrafodionSchema.LoadViewByName(nameParts[2]);
                        }
                        
                        if (alertsView != null)
                        {
                            //Reload the view's privileges. It may have changed since the last time it was looked up.
                            alertsView.ClearPrivileges();

                            //Look up if the ROLE has SELECT privileges on the ALERTS view
                            canUserView = alertsView.DoesUserHaveObjectPrivilege(aConnectionDefinition.RoleName, TrafodionPrivilegeTypeHelper.TYPE_SELECT);
                            if (!canUserView)
                            {
                                //Look up if the db user has SELECT privileges on the ALERTS view
                                canUserView = alertsView.DoesUserHaveObjectPrivilege(aConnectionDefinition.DatabaseUserName, TrafodionPrivilegeTypeHelper.TYPE_SELECT);
                            }
                            if (!canUserView)
                            {
                                //IF NO object level SELECT, check if the ROLE has schema level select
                                canUserView = aTrafodionSchema.DoesUserHavePrivilege(aConnectionDefinition.RoleName, TrafodionPrivilegeTypeHelper.TYPE_SELECT);
                                if (!canUserView)
                                {
                                    //Look up if the db user has has schema level select
                                    canUserView = alertsView.DoesUserHaveObjectPrivilege(aConnectionDefinition.DatabaseUserName, TrafodionPrivilegeTypeHelper.TYPE_SELECT);
                                }
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                throw ex;
            }

            //User does NOT have select privileges
            return canUserView;
        }

        /// <summary>
        /// Checks to see if the current logged on user has privileges to update an alert
        /// </summary>
        /// <param name="aConnectionDefinition">Connection definition with the logged on user information</param>
        /// <returns>True or False</returns>
        public bool CanUserUpdate(ConnectionDefinition aConnectionDefinition, bool refreshPrivileges)
        {

            bool canUserUpdate = false;

            try
            {
                //Load the SQL model for the Alarms base table and look up the privileges
                TrafodionSystem aTrafodionSystem = TrafodionSystem.FindTrafodionSystem(aConnectionDefinition);
                TrafodionCatalog aTrafodionCatalog = null;
                TrafodionSchema aTrafodionSchema = null;
                TrafodionProcedure updateProcedure = null;
                string[] nameParts = Utilities.CrackSQLAnsiName(PROBLEM_MGMT_UPDATE_PROCEDURE);

                if (TrafodionObject.Exists<TrafodionCatalog>(aTrafodionSystem.TrafodionCatalogs, nameParts[0]))
                {
                    aTrafodionCatalog = aTrafodionSystem.FindCatalog(nameParts[0]);
                }
                else
                {
                    aTrafodionCatalog = aTrafodionSystem.LoadTrafodionCatalog(nameParts[0]);
                }

                if (aTrafodionCatalog != null)
                {
                    if (TrafodionObject.Exists<TrafodionSchema>(aTrafodionCatalog.TrafodionSchemas, nameParts[1]))
                    {
                        aTrafodionSchema = aTrafodionCatalog.FindSchema(nameParts[1]);
                    }
                    else
                    {
                        aTrafodionSchema = aTrafodionCatalog.LoadTrafodionSchema(nameParts[1]);
                    }
                    if (aTrafodionSchema != null)
                    {
                        if (TrafodionObject.Exists<TrafodionProcedure>(aTrafodionSchema.TrafodionProcedures, nameParts[2]))
                        {
                            updateProcedure = aTrafodionSchema.FindProcedure(nameParts[2]);
                        }
                        else
                        {
                            updateProcedure = aTrafodionSchema.LoadProcedureByName(nameParts[2]);
                        }

                        if (updateProcedure != null)
                        {
                            if (refreshPrivileges)
                            {
                                //Reload privileges everytime. It may have changed since the last time it was looked up.
                                updateProcedure.ClearPrivileges();
                            }

                            //Look up if the ROLE has EXECUTE privileges on the update procedure
                            canUserUpdate = updateProcedure.DoesUserHaveObjectPrivilege(aConnectionDefinition.RoleName, TrafodionPrivilegeTypeHelper.TYPE_EXECUTE);
                            if (!canUserUpdate)
                            {
                                //Look up if the user has EXECUTE privileges on the update procedure
                                canUserUpdate = updateProcedure.DoesUserHaveObjectPrivilege(aConnectionDefinition.DatabaseUserName, TrafodionPrivilegeTypeHelper.TYPE_EXECUTE);
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                throw ex;
            }

            //User does NOT have update privileges
            return canUserUpdate;
        }

        public static int GetAlarmNotesLength(ConnectionDefinition aConnectionDefinition)
        {
            int notesLength = 0;

            try
            {
                //Load the SQL model for the Alarms base table and look up the size of notes column
                TrafodionSystem aTrafodionSystem = TrafodionSystem.FindTrafodionSystem(aConnectionDefinition);
                TrafodionCatalog aTrafodionCatalog = null;
                TrafodionSchema aTrafodionSchema = null;
                TrafodionTable alarmsTable = null;
                string[] nameParts = Utilities.CrackSQLAnsiName(PROBLEM_MGMT_TABLE);

                if (TrafodionObject.Exists<TrafodionCatalog>(aTrafodionSystem.TrafodionCatalogs, nameParts[0]))
                {
                    aTrafodionCatalog = aTrafodionSystem.FindCatalog(nameParts[0]);
                }
                else
                {
                    aTrafodionCatalog = aTrafodionSystem.LoadTrafodionCatalog(nameParts[0]);
                }

                if (aTrafodionCatalog != null)
                {
                    if (TrafodionObject.Exists<TrafodionSchema>(aTrafodionCatalog.TrafodionSchemas, nameParts[1]))
                    {
                        aTrafodionSchema = aTrafodionCatalog.FindSchema(nameParts[1]);
                    }
                    else
                    {
                        aTrafodionSchema = aTrafodionCatalog.LoadTrafodionSchema(nameParts[1]);
                    }
                    if (aTrafodionSchema != null)
                    {
                        if (TrafodionObject.Exists<TrafodionTable>(aTrafodionSchema.TrafodionTables, nameParts[2]))
                        {
                            alarmsTable = aTrafodionSchema.FindTable(nameParts[2]);
                        }
                        else
                        {
                            alarmsTable = aTrafodionSchema.LoadTableByName(nameParts[2]);
                        }

                        if (alarmsTable != null)
                        {
                            TrafodionColumn notesColumn = alarmsTable.FindColumnByInternalName("NOTES");
                            if (notesColumn != null)
                            {
                                notesLength = notesColumn.TheColumnSize;
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Trafodion.Manager.Framework.Logger.OutputErrorLog("Check for size of notes column in alarms table failed : " + ex.Message);
            }

            return notesLength;
        }

        #endregion Helper methods





    }

    
}
