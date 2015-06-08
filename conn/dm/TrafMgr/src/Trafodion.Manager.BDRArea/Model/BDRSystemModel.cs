// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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
using System;
using System.Data;
using System.Data.Odbc;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using System.Collections.Generic;


namespace Trafodion.Manager.BDRArea.Model
{
        public class BDRSystemModel
        {
            #region Fields
            private static Dictionary<ConnectionDefinition, BDRSystemModel> _activeSystemModels = new Dictionary<ConnectionDefinition, BDRSystemModel>(new MyConnectionDefinitionComparer());
            private ConnectionDefinition _theConnectionDefinition = null;
            private Connection _theCurrentConnection = null;
            #endregion Fields

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

            private BDRSystemModel(ConnectionDefinition connectionDefinition)
            {
                ConnectionDefinition = connectionDefinition;
                //Add this instance to the static dictionary, so in future if need a reference to this system, 
                //we can look up using the connection definition
                if (!_activeSystemModels.ContainsKey(connectionDefinition))
                    _activeSystemModels.Add(connectionDefinition, this);

                //Subscribe to connection definition's events, so that you can maintain the static dictionary
                ConnectionDefinition.Changed += ConnectionDefinition_Changed;
            }


            ~BDRSystemModel()
            {
                ConnectionDefinition.Changed -= ConnectionDefinition_Changed;
            }

            public static BDRSystemModel FindSystemModel(ConnectionDefinition connectionDefinition)
            {
                BDRSystemModel systemModel = null;
                _activeSystemModels.TryGetValue(connectionDefinition, out systemModel);
                if (systemModel == null)
                {
                    systemModel = new BDRSystemModel(connectionDefinition);
                }
                return systemModel;
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
            /// Returns Q results in a DataTable
            /// </summary>
            /// <param name="queryText"></param>
            /// <returns></returns>
            public DataTable GetQueryResults(string queryText)
            {
                Connection connection = new Connection(_theConnectionDefinition);
                DataTable dataTable = new DataTable();

                try
                {
                    Logger.OutputToLog(TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.BDRDashboard, "", String.Format("Execute: {0}", queryText));
                    OdbcDataAdapter dataAdapter = new OdbcDataAdapter(queryText, connection.OpenOdbcConnection);
                    dataAdapter.Fill(dataTable);
                    Logger.OutputToLog(TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.BDRDashboard, "", String.Format("ExecuteReader: {0} completed successfully", queryText));
                }
                catch (Exception ex)
                {
                    Logger.OutputToLog(TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.BDRDashboard, "", String.Format("ExecuteReader: {0} failed. Exception: {1}", queryText, ex.Message));
                    throw ex;
                }
                finally
                {
                    if (connection != null) { connection.Close(); }
                }
                return dataTable;
            }
        }
    }
