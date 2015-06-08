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
using System.Data;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Model
{
    public class ComponentPrivilegeModel
    {
       private static Dictionary<ConnectionDefinition, ComponentPrivilegeModel> _activeSystemModels = 
            new Dictionary<ConnectionDefinition, ComponentPrivilegeModel>(new MyConnectionDefinitionComparer());        
        public static readonly string NameStatusColumn = "Name";
        public static readonly string StatusColumName = "Status";
        public static readonly string MessageColumName = "Message";

        private ConnectionDefinition _theConnectionDefinition = null;
        private Connection _theCurrentConnection = null;

        private const string SuccessStatus = "Success";
        private const string WarningStatus = "Warning";
        private const string FailureStatus = "Failure";

        
        public ConnectionDefinition ConnectionDefinition
        {
            get
            {
                return _theConnectionDefinition;
            }
            set
            {
                _theConnectionDefinition = value;
            }
        }

        public Connection CurrentConnection
        {
            get
            {
                return _theCurrentConnection;
            }
            set
            {
                _theCurrentConnection = value;
            }
        }

        //public bool IsAdminUser
        //{
        //    get
        //    {
        //        if (ConnectionDefinition.IsWmsAdminRole)
        //        {
        //            return true;
        //        }
        //        else
        //        {
        //            return false;
        //        }
        //    }
        //}


        private ComponentPrivilegeModel(ConnectionDefinition aConnectionDefinition)
        {
            ConnectionDefinition = aConnectionDefinition;
            //Add this instance to the static dictionary, so in future if need a reference to this system, 
            //we can look up using the connection definition
            if (!_activeSystemModels.ContainsKey(aConnectionDefinition))
                _activeSystemModels.Add(aConnectionDefinition, this);

            //Subscribe to connection definition's events, so that you can maintain the static dictionary
            ConnectionDefinition.Changed += ConnectionDefinition_Changed;

        }

        ~ComponentPrivilegeModel()
        {
            ConnectionDefinition.Changed -= ConnectionDefinition_Changed;
        }

        public static ComponentPrivilegeModel FindSystemModel(ConnectionDefinition connectionDefinition)
        {
            ComponentPrivilegeModel comPrivModel = null;
            _activeSystemModels.TryGetValue(connectionDefinition, out comPrivModel);
            if (comPrivModel == null)
            {
                comPrivModel = new ComponentPrivilegeModel(connectionDefinition);
            }
            return comPrivModel;
        }


        /// <summary>
        /// If the connection definition has changed/removed, the static dictionary is updated accordingly
        /// </summary>
        /// <param name="aSender"></param>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="aReason"></param>
        void ConnectionDefinition_Changed(object aSender, ConnectionDefinition aConnectionDefinition, ConnectionDefinition.Reason aReason)
        {
            if (aReason != ConnectionDefinition.Reason.Tested)
            {
                _activeSystemModels.Remove(aConnectionDefinition);
            }
        }
        /// <summary>
        /// Execute Grant/Revoke command,return the status table for result.
        /// </summary>
        /// <param name="lstAlterPrivCmd"></param>
        /// <returns></returns>
        public DataTable GrantRevokeComponentPrivileges(List<string> lstAlterPrivCmd)
        {
            DataTable statusTable = GetStatusDataTable();
            try
            {

                if (!GetConnection())
                {
                    AddStatusToTable("Grant/Revoke Component privileges", FailureStatus, "Unable to get connection", statusTable);
                    return statusTable;
                }

                int result = -1;
                if (lstAlterPrivCmd.Count > 0)
                {
                    String cmd = string.Empty;
                    DataTable dtWarnings = new DataTable();
                    for (int i = 0; i < lstAlterPrivCmd.Count; i++)
                    {
                        try
                        {
                            cmd = lstAlterPrivCmd[i];
                            result = Queries.ExecuteGrantRevoke(ConnectionDefinition, cmd, out dtWarnings);
                            if (dtWarnings.Rows.Count == 0)
                            {
                                AddStatusToTable(cmd, SuccessStatus, "", statusTable);
                            }
                            else
                            {
                                foreach (DataRow warning in dtWarnings.Rows)
                                {
                                    AddStatusToTable(cmd, WarningStatus, warning[0] as string, statusTable);
                                }
                            }

                        }
                        catch (Exception ex)
                        {
                            AddStatusToTable(cmd, FailureStatus, ex.Message, statusTable);
                        }
                    }

                    //After all privileges have been committed,reload the privileges list.
                    TrafodionSystem _sqlMxSystem = TrafodionSystem.FindTrafodionSystem(_theConnectionDefinition);
                    _sqlMxSystem.RefreshComponentPrivileges();
                }
            }
            finally
            {
                CloseConnection();
            }
            return statusTable;
        }


        private DataTable GetStatusDataTable()
        {
            DataTable table = new DataTable();
            table.Columns.Add(NameStatusColumn);
            table.Columns.Add(StatusColumName);
            table.Columns.Add(MessageColumName);
            return table;
        }

        public bool GetConnection()
        {
            if (this._theCurrentConnection == null && this._theConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
            {
                _theCurrentConnection = new Connection(ConnectionDefinition);
                return true;
            }
            else
            {
                return true;
            }
        }

        public void CloseConnection()
        {
            if (this._theCurrentConnection != null)
            {
                _theCurrentConnection.Close();
                _theCurrentConnection = null;
            }
        }

        private void AddStatusToTable(string anOperation, string aStatus, string aMessage, DataTable aTable)
        {
            aTable.Rows.Add(new object[] { anOperation, aStatus, aMessage });
        }

        public int GetSuccessRowCount(DataTable aDataTable)
        {
            DataRow[] successRows = aDataTable.Select(string.Format("Status = '{0}'", SuccessStatus));
            return successRows.Length;
        }
        public int GetFailureRowCount(DataTable aDataTable)
        {
            DataRow[] successRows = aDataTable.Select(string.Format("Status = '{0}'", FailureStatus));
            return successRows.Length;
        }

    }
}
