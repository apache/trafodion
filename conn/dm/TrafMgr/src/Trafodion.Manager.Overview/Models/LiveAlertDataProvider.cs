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
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Trafodion.Manager.LiveFeedFramework.Models;
using Trafodion.Manager.UniversalWidget;
using System.Data;
using Trafodion.Manager.Framework;
using Trafodion.Manager.OverviewArea.Controls;
using Trafodion.Manager.Framework.Connections;
using System.Data.Odbc;

namespace Trafodion.Manager.OverviewArea.Models
{
    class LiveAlertDataProvider : CachedLiveFeedProtoBufDataProvider
    {
        #region Fields
        private DataTable _theDataTable=new DataTable();
        private DataTable _theProtoBufDataTable;

        private string DIMENSION_SEVERITY = "SEVERITY";
        private string DIMENSION_SEVERITY_NAME = "SEVERITY_NAME";

        private string DIMENSION_TYPE = "TYPE";
        private string DIMENSION_TYPE_DESCRIPTION = "TYPE_DESCRIPTION";

        private string DIMENSION_COMPONENT_ID = "COMPONENT_ID";
        private string DIMENSION_COMPONENT_NAME = "COMPONENT_NAME";

        private string DIMENSION_PROBLEM_COMPONENT_ID = "PROBLEM_COMPONENT_ID";
        private string DIMENSION_PROBLEM_COMPONENT_NAME = "PROBLEM_COMPONENT_NAME";
        
        public static DataTable DIMENSION_SEVERITY_TABLE;
        public static DataTable DIMENSION_COMPONENT_TABLE;
        public static DataTable DIMENSION_PROBLEMTYPE_TABLE;

        private const string TRACE_SUB_AREA_NAME = "Alerts";

        public static string problem_open_routing_key="problem_management.problem_open";        
        public static string problem_close_routing_key = "problem_management.problem_close";

         private static Dictionary<string, string> repository_protobuf_mapping_dictionary_open = new Dictionary<string, string>()
        {
            {"INSTANCE_ID", "info_header_info_instance_id"},        //1
            {"TENANT_ID", "info_header_info_tenant_id"},             
            {"CALENDAR_DATE_LCT", "info_header_info_generation_time_ts_lct"},
            {"CALENDAR_DATE_UTC", "info_header_info_generation_time_ts_utc"},
            {"GEN_TS_LCT", "info_header_info_generation_time_ts_lct"},
            {"GEN_TS_UTC", "info_header_info_generation_time_ts_utc"},
            {"UPSERT_TS_LCT", ""},
            {"UPSERT_TS_UTC", ""},
            {"COMPONENT_ID", "info_header_info_component_id"},
            {"COMPONENT_NAME", ""},                                     //10
            {"PROCESS_ID", "info_header_info_process_id"},
            {"THREAD_ID", "info_header_info_thread_id"},
            {"NODE_ID", "info_header_info_node_id"},
            {"PNID_ID", "info_header_info_pnid_id"},
            {"HOST_ID", "info_header_info_host_id"},
            {"IP_ADDRESS_ID", "info_header_info_ip_address_id"},
            {"SEQUENCE_NUMBER", "info_header_info_sequence_num"},
            {"PROCESS_NAME", "info_header_info_process_name"},
            {"CREATE_TS_LCT", "creation_time_ts_lct"},
            {"CREATE_TS_UTC", "creation_time_ts_utc"},          //20
            {"CREATOR_PROCESS_ID", "creator_process_id"},
            {"CREATOR_HOST_ID", "creator_host_id"},
            {"LAST_UPDATE_TS_LCT", "creation_time_ts_lct"},
            {"LAST_UPDATE_TS_UTC", "creation_time_ts_utc"},
            {"CLOSE_TS_LCT", ""},
            {"CLOSE_TS_UTC", ""},
            {"PROBLEM_INSTANCE_ID", "problem_instance_id"},
            {"PROBLEM_TENANT_ID", "problem_tenant_id"},
            {"PROBLEM_COMPONENT_ID", "problem_component_id"},
            {"PROBLEM_COMPONENT_NAME", ""},                 //30
            {"PROBLEM_PROCESS_ID", "problem_process_id"},
            {"PROBLEM_THREAD_ID", "problem_thread_id"},
            {"PROBLEM_NODE_ID", "problem_node_id"},
            {"PROBLEM_PNID_ID", "problem_pnid_id"},
            {"PROBLEM_HOST_ID", "problem_host_id"},
            {"PROBLEM_IP_ADDRESS_ID", "problem_ip_address_id"},
            {"PROBLEM_PROCESS_NAME", "problem_process_name"},
            {"RESOURCE_NAME", "resource_name"},
            {"RESOURCE_TYPE", "resource_type"},
            {"SEVERITY", "severity"},                               //40
            {"SEVERITY_NAME", ""},
            {"STATUS", "status"},
            {"DESCRIPTION", "description"},
            {"TYPE", "type"},
            {"TYPE_DESCRIPTION", ""},
            {"NOTES", ""}
        };

         private static Dictionary<string, string> repository_protobuf_mapping_dictionary_close = new Dictionary<string, string>()
        {
            {"INSTANCE_ID", "info_header_info_instance_id"},        //1
            {"TENANT_ID", "info_header_info_tenant_id"},             
            {"CALENDAR_DATE_LCT", "info_header_info_generation_time_ts_lct"},
            {"CALENDAR_DATE_UTC", "info_header_info_generation_time_ts_utc"},
            {"GEN_TS_LCT", "info_header_info_generation_time_ts_lct"},
            {"GEN_TS_UTC", "info_header_info_generation_time_ts_utc"},
            {"UPSERT_TS_LCT", ""},
            {"UPSERT_TS_UTC", ""},
            {"COMPONENT_ID", "info_header_info_component_id"},
            {"COMPONENT_NAME", ""},                                     //10
            {"PROCESS_ID", "info_header_info_process_id"},
            {"THREAD_ID", "info_header_info_thread_id"},
            {"NODE_ID", "info_header_info_node_id"},
            {"PNID_ID", "info_header_info_pnid_id"},
            {"HOST_ID", "info_header_info_host_id"},
            {"IP_ADDRESS_ID", "info_header_info_ip_address_id"},
            {"SEQUENCE_NUMBER", "info_header_info_sequence_num"},
            {"PROCESS_NAME", "info_header_info_process_name"},
            {"CREATE_TS_LCT", "creation_time_ts_lct"},
            {"CREATE_TS_UTC", "creation_time_ts_utc"},          //20
            {"CREATOR_PROCESS_ID", "creator_process_id"},
            {"CREATOR_HOST_ID", "creator_host_id"},
            {"LAST_UPDATE_TS_LCT", "creation_time_ts_lct"},
            {"LAST_UPDATE_TS_UTC", "creation_time_ts_utc"},
            {"CLOSE_TS_LCT", "info_header_info_generation_time_ts_lct"},
            {"CLOSE_TS_UTC", "info_header_info_generation_time_ts_utc"},
            {"PROBLEM_INSTANCE_ID", "problem_instance_id"},
            {"PROBLEM_TENANT_ID", "problem_tenant_id"},
            {"PROBLEM_COMPONENT_ID", "problem_component_id"},
            {"PROBLEM_COMPONENT_NAME", ""},                 //30
            {"PROBLEM_PROCESS_ID", "problem_process_id"},
            {"PROBLEM_THREAD_ID", "problem_thread_id"},
            {"PROBLEM_NODE_ID", "problem_node_id"},
            {"PROBLEM_PNID_ID", "problem_pnid_id"},
            {"PROBLEM_HOST_ID", "problem_host_id"},
            {"PROBLEM_IP_ADDRESS_ID", "problem_ip_address_id"},
            {"PROBLEM_PROCESS_NAME", "problem_process_name"},
            {"RESOURCE_NAME", "resource_name"},
            {"RESOURCE_TYPE", "resource_type"},
            {"SEVERITY", "severity"},                               //40
            {"SEVERITY_NAME", ""},
            {"STATUS", "status"},
            {"DESCRIPTION", "description"},
            {"TYPE", "type"},
            {"TYPE_DESCRIPTION", ""},
            {"NOTES", ""}
        };



        private static Dictionary<string, Type> repository_column_datatype = new Dictionary<string, Type>()
        {
            {"INSTANCE_ID", typeof(System.Int64)},        //1
            {"TENANT_ID",  typeof(System.Int64)},             
            {"CALENDAR_DATE_LCT",  typeof(System.DateTime)},
            {"CALENDAR_DATE_UTC",  typeof(System.DateTime)},
            {"GEN_TS_LCT", typeof(System.DateTime)},
            {"GEN_TS_UTC", typeof(System.DateTime)},
            {"UPSERT_TS_LCT", typeof(System.DateTime)},
            {"UPSERT_TS_UTC", typeof(System.DateTime)},
            {"COMPONENT_ID", typeof(System.Int64)},
            {"COMPONENT_NAME", typeof(System.String)},                                     //10
            {"PROCESS_ID", typeof(System.Int32)},
            {"THREAD_ID", typeof(System.Int64)},
            {"NODE_ID", typeof(System.Int64)},
            {"PNID_ID", typeof(System.Int64)},
            {"HOST_ID", typeof(System.Int64)},
            {"IP_ADDRESS_ID", typeof(System.String)},
            {"SEQUENCE_NUMBER", typeof(System.Int64)},
            {"PROCESS_NAME", typeof(System.String)},
            {"CREATE_TS_LCT",  typeof(System.DateTime)},
            {"CREATE_TS_UTC",  typeof(System.DateTime)},          //20
            {"CREATOR_PROCESS_ID",  typeof(System.Int32)},
            {"CREATOR_HOST_ID", typeof(System.Int64)},
            {"LAST_UPDATE_TS_LCT", typeof(System.DateTime)},
            {"LAST_UPDATE_TS_UTC", typeof(System.DateTime)},
            {"CLOSE_TS_LCT", typeof(System.DateTime)},
            {"CLOSE_TS_UTC", typeof(System.DateTime)},
            {"PROBLEM_INSTANCE_ID", typeof(System.Int64)},
            {"PROBLEM_TENANT_ID", typeof(System.Int64)},
            {"PROBLEM_COMPONENT_ID", typeof(System.Int64)},
            {"PROBLEM_COMPONENT_NAME", typeof(System.String)},                 //30
            {"PROBLEM_PROCESS_ID", typeof(System.Int32)},
            {"PROBLEM_THREAD_ID", typeof(System.Int64)},
            {"PROBLEM_NODE_ID", typeof(System.Int64)},
            {"PROBLEM_PNID_ID", typeof(System.Int64)},
            {"PROBLEM_HOST_ID", typeof(System.Int64)},
            {"PROBLEM_IP_ADDRESS_ID", typeof(System.String)},
            {"PROBLEM_PROCESS_NAME", typeof(System.String)},
            {"RESOURCE_NAME", typeof(System.String)},
            {"RESOURCE_TYPE", typeof(System.String)},
            {"SEVERITY", typeof(System.Int32)},                               //40
            {"SEVERITY_NAME", typeof(System.String)},
            {"STATUS", typeof(System.String)},
            {"DESCRIPTION", typeof(System.String)},
            {"TYPE", typeof(System.Int32)},
            {"TYPE_DESCRIPTION", typeof(System.String)},
            {"NOTES", typeof(System.String)}
        };

        

        #endregion Fields

        #region Properties

        protected DataTable TheDataTable
        {
          get { return _theDataTable; }
          set { _theDataTable = value; }
        }

        #endregion



        #region Constructors

        /// <summary>
        /// The constructor of this data provider
        /// </summary>
        /// <param name="aDataProviderConfig"></param>
        public LiveAlertDataProvider(DataProviderConfig aDataProviderConfig)
            : base(aDataProviderConfig)
        {
            ConstructLiveAlertDataTableUsingRepoSchema();
            ConstrucDimensionTables();
        }
        #endregion Constructors


        #region Public Methods
        
        public override System.Data.DataTable GetDataTable()
        {
            Dictionary<string, List<object>> dataStats = base.GetStats();
            //_theDataTable.Rows.Clear();
            lock (dataStats)
            {
                string pubName = base.LastReceivedPublication;
                if (string.IsNullOrEmpty(pubName) || dataStats.Count == 0) 
                {
                    _theDataTable.Clear();
                    return _theDataTable;
                }                    
                List<object> stats = dataStats[pubName];
                List<object> newStats = stats.ToList<object>();

                if (stats.Count > 0)
                {
                    switch (pubName)
                    {
                        case LiveFeedRoutingKeyMapper.PUBS_problem_management_problem_open:
                            {
                                foreach (var item in newStats)
                                {
                                    if (_theProtoBufDataTable == null)
                                    {
                                        _theProtoBufDataTable = LiveFeedStatsTransformer.TransformStatsToDataTable(pubName, item);
                                    }
                                    else
                                    {
                                        LiveFeedStatsTransformer.TransformStatsToDataTable(pubName, item, _theProtoBufDataTable);
                                    }
                                }
                                //Construction Complete. Now Transform to Problem Scheme.                                
                                _theDataTable = TransformLiveProblemDataTable(_theProtoBufDataTable, true);
                                //Clear ProtoBufDataTable, next time it will start over
                                _theProtoBufDataTable.Rows.Clear();
                            }
                            break;
                        case LiveFeedRoutingKeyMapper.PUBS_problem_management_problem_close:
                            {
                                //DataTable protobufDataTable = LiveFeedStatsTransformer.TransformStatsToDataTable(pubName, stats[stats.Count - 1]);
                                //_theDataTable = TransformLiveProblemDataTable(protobufDataTable, false);
                                //LiveDataMerge(_theNewComingDataTable); 
                                foreach (var item in newStats)
                                {
                                    if (_theProtoBufDataTable == null)
                                    {
                                        _theProtoBufDataTable = LiveFeedStatsTransformer.TransformStatsToDataTable(pubName, item);
                                    }
                                    else
                                    {
                                        LiveFeedStatsTransformer.TransformStatsToDataTable(pubName, item, _theProtoBufDataTable);
                                    }
                                }
                                //Construction Complete. Now Transform to Problem Scheme.                                
                                _theDataTable = TransformLiveProblemDataTable(_theProtoBufDataTable, false);
                                //Clear ProtoBufDataTable, next time it will start over
                                _theProtoBufDataTable.Rows.Clear();
                            }
                            break;
                    }
                }
                else 
                {
                    _theDataTable.Clear();
                }
                stats.Clear(); 
            }
            
            return _theDataTable;
        }

        public void ConstrucDimensionTablesfromDB(Connection _theCurrentConnection) 
        {
            DataTable dt1 = ConstructDIMENSION_SEVERITY_TABLEfromDB(_theCurrentConnection);
            if (dt1 != null)
                DIMENSION_SEVERITY_TABLE = dt1;

            DataTable dt2 = ConstructDIMENSION_COMPONENT_TABLEfromDB(_theCurrentConnection);
            if (dt2 != null)
                DIMENSION_COMPONENT_TABLE = dt2;

            DataTable dt3 = ConstructDIMENSION_PROBLEMTYPE_TABLEfromDB(_theCurrentConnection);
            if (dt3 != null)
                DIMENSION_PROBLEMTYPE_TABLE = dt3;
        }


        #endregion Public Methods

        #region Private Methods
        private void ConstructLiveAlertDataTableUsingRepoSchema() 
        {
            if (_theDataTable.Columns.Count == 0)
            {
                _theDataTable = ConstructRepositorySchemeDataTable();        
            }
        }


        private void ConstrucDimensionTables() 
        {
            ConstructDIMENSION_SEVERITY_TABLE();
            ConstructDIMENSION_COMPONENT_TABLE();
            ConstructDIMENSION_PROBLEMTYPE_TABLE();
        }

        private void ConstructDIMENSION_SEVERITY_TABLE() 
        {
            DIMENSION_SEVERITY_TABLE = new DataTable("DIMENSION_SEVERITY_TABLE");
            
            DIMENSION_SEVERITY_TABLE.Columns.Add(new DataColumn("SEVERITY", typeof(int)));
            DIMENSION_SEVERITY_TABLE.Columns.Add(new DataColumn("SEVERITY_NAME", typeof(string)));
            DIMENSION_SEVERITY_TABLE.Columns.Add(new DataColumn("SEVERITY_DESCRIPTION", typeof(string)));

            DIMENSION_SEVERITY_TABLE.PrimaryKey = new DataColumn[] { DIMENSION_SEVERITY_TABLE.Columns["SEVERITY"] };         

            DIMENSION_SEVERITY_TABLE.Rows.Add(6, "Informational", "Informational");
            DIMENSION_SEVERITY_TABLE.Rows.Add(0, "Emergency", "System unusable");
            DIMENSION_SEVERITY_TABLE.Rows.Add(1, "Immediate", "Immediate action required");
            DIMENSION_SEVERITY_TABLE.Rows.Add(4, "Warning", "Warning condition");
            DIMENSION_SEVERITY_TABLE.Rows.Add(2, "Critical", "Critical condition");
            DIMENSION_SEVERITY_TABLE.Rows.Add(3, "Error", "Error condition");
            DIMENSION_SEVERITY_TABLE.Rows.Add(5, "Notice", "Normal but significant condition");
            DIMENSION_SEVERITY_TABLE.Rows.Add(7, "Debug", "Debug-level message");
        }

        private void ConstructDIMENSION_COMPONENT_TABLE()
        {
            DIMENSION_COMPONENT_TABLE = new DataTable("DIMENSION_COMPONENT_TABLE");

            DIMENSION_COMPONENT_TABLE.Columns.Add(new DataColumn(DIMENSION_COMPONENT_ID, typeof(int)));
            DIMENSION_COMPONENT_TABLE.Columns.Add(new DataColumn(DIMENSION_COMPONENT_NAME, typeof(string)));
            DIMENSION_COMPONENT_TABLE.Columns.Add(new DataColumn("COMPONENT_DESCRIPTION", typeof(string)));

            DIMENSION_COMPONENT_TABLE.PrimaryKey = new DataColumn[] { DIMENSION_COMPONENT_TABLE.Columns[DIMENSION_COMPONENT_ID] };

            DIMENSION_COMPONENT_TABLE.Rows.Add(6, "AMP", "Audit Management Process");
            DIMENSION_COMPONENT_TABLE.Rows.Add(1, "Monitor", "Monitor component");
            DIMENSION_COMPONENT_TABLE.Rows.Add(8, "HPDCS", "Connectivity services");
            DIMENSION_COMPONENT_TABLE.Rows.Add(9, "SQL", "SQL database engine");
            DIMENSION_COMPONENT_TABLE.Rows.Add(4, "TSE", "Table Storage Engine");
            DIMENSION_COMPONENT_TABLE.Rows.Add(10, "LUNMGR", "LUN Manager");
            DIMENSION_COMPONENT_TABLE.Rows.Add(2, "SeaBed", "SeaBed library");
            DIMENSION_COMPONENT_TABLE.Rows.Add(13, "Transducer", "Manageability infrastructure");
            DIMENSION_COMPONENT_TABLE.Rows.Add(14, "SeaBridge", "Database manageability layer");
            DIMENSION_COMPONENT_TABLE.Rows.Add(3, "DTM", "Distributed Transaction Manager");
            DIMENSION_COMPONENT_TABLE.Rows.Add(5, "ACE", "Audit Storage Engine");
            DIMENSION_COMPONENT_TABLE.Rows.Add(7, "Recovery", "Transaction recovery component");
            DIMENSION_COMPONENT_TABLE.Rows.Add(15, "TSvc", "Transporter Services");
            DIMENSION_COMPONENT_TABLE.Rows.Add(17, "IODRV", "I/O Driver layer");
            DIMENSION_COMPONENT_TABLE.Rows.Add(11, "Security", "Security component");
            DIMENSION_COMPONENT_TABLE.Rows.Add(16, "WMS", "Workload Management Services");
        }

        private void ConstructDIMENSION_PROBLEMTYPE_TABLE()
        {
            DIMENSION_PROBLEMTYPE_TABLE = new DataTable("DIMENSION_PROBLEMTYPE_TABLE");

            DIMENSION_PROBLEMTYPE_TABLE.Columns.Add(new DataColumn("TYPE", typeof(int)));            
            DIMENSION_PROBLEMTYPE_TABLE.Columns.Add(new DataColumn("TYPE_DESCRIPTION", typeof(string)));

            DIMENSION_PROBLEMTYPE_TABLE.PrimaryKey = new DataColumn[] { DIMENSION_PROBLEMTYPE_TABLE.Columns["TYPE"] };

            DIMENSION_PROBLEMTYPE_TABLE.Rows.Add(6, "Audit trail space is full");
            DIMENSION_PROBLEMTYPE_TABLE.Rows.Add(1, "SQL internal error");
            DIMENSION_PROBLEMTYPE_TABLE.Rows.Add(4, "SE LUN is down due to hardware error");
            DIMENSION_PROBLEMTYPE_TABLE.Rows.Add(2, "SQL engine is down");
            DIMENSION_PROBLEMTYPE_TABLE.Rows.Add(3, "SE LUN is down due to software error");
            DIMENSION_PROBLEMTYPE_TABLE.Rows.Add(5, "Audit trail space is at least half full");
            DIMENSION_PROBLEMTYPE_TABLE.Rows.Add(1000, "Failure detected by subsystem health check");
            DIMENSION_PROBLEMTYPE_TABLE.Rows.Add(2000, "Inferred Problem");
        }

        public DataTable ConstructDIMENSION_SEVERITY_TABLEfromDB(Connection _theCurrentConnection) 
        {
            try
            {
                DataTable dt = new DataTable();
                string sql = "SELECT SEVERITY, SEVERITY_NAME, SEVERITY_DESCRIPTION  FROM MANAGEABILITY.ACCESS.DIMENSION_SEVERITY_TABLE for read uncommitted access";
                OdbcCommand theSelectCommand = new OdbcCommand(sql);
                theSelectCommand.Connection = _theCurrentConnection.OpenOdbcConnection;
                OdbcDataAdapter theAdapter = new OdbcDataAdapter(theSelectCommand);

                theAdapter.Fill(dt);
                return dt;
            }
            catch (Exception ex)
            {
                return null;
            }
            finally
            {
                if (_theCurrentConnection != null)
                {
                    _theCurrentConnection.Close();
                }
            }
        }

        public DataTable ConstructDIMENSION_COMPONENT_TABLEfromDB(Connection _theCurrentConnection)
        {
            try
            {
                DataTable dt = new DataTable();
                string sql = "SELECT COMPONENT_ID, COMPONENT_NAME , COMPONENT_DESCRIPTION  FROM MANAGEABILITY.ACCESS.DIMENSION_COMPONENT_TABLE for read uncommitted access";
                OdbcCommand theSelectCommand = new OdbcCommand(sql);
                theSelectCommand.Connection = _theCurrentConnection.OpenOdbcConnection;
                OdbcDataAdapter theAdapter = new OdbcDataAdapter(theSelectCommand);

                theAdapter.Fill(dt);
                return dt;
            }
            catch (Exception ex)
            {
                return null;
            }
            finally
            {
                if (_theCurrentConnection != null)
                {
                    _theCurrentConnection.Close();
                }
            }
        }

        public DataTable ConstructDIMENSION_PROBLEMTYPE_TABLEfromDB(Connection _theCurrentConnection)
        {
            try
            {
                DataTable dt = new DataTable();
                string sql = "SELECT TYPE, TYPE_DESCRIPTION  FROM MANAGEABILITY.ACCESS.DIMENSION_PROBLEMTYPE_TABLE for read uncommitted access";
                OdbcCommand theSelectCommand = new OdbcCommand(sql);
                theSelectCommand.Connection = _theCurrentConnection.OpenOdbcConnection;
                OdbcDataAdapter theAdapter = new OdbcDataAdapter(theSelectCommand);

                theAdapter.Fill(dt);
                return dt;
            }
            catch (Exception ex)
            {
                return null;
            }
            finally
            {
                if (_theCurrentConnection != null)
                {
                    _theCurrentConnection.Close();
                }
            }
        }
                

        public static DataTable ConstructRepositorySchemeDataTable()
        {
            DataTable dt=new DataTable();
            foreach (var pair in repository_column_datatype)
            {
                dt.Columns.Add(pair.Key, pair.Value);
            }
            dt.DefaultView.Sort = AlertsUserControl.ALERT_LAST_UPDATE_TS_LCT_COL_NAME + " DESC";
            return dt;
        }

        private void LiveDataMerge(DataTable newComingDataTable)
        {
            List<DataRow> toRemoved = new List<DataRow>();
            foreach (DataRow drNew in newComingDataTable.Rows)
            {
                foreach (DataRow drLive in _theDataTable.Rows)
                {  
                    if (drNew[AlertsUserControl.ALERT_CREATE_TS_LCT_COL_NAME].Equals(drLive[AlertsUserControl.ALERT_CREATE_TS_LCT_COL_NAME]) &&
                        drNew[AlertsUserControl.ALERT_CREATOR_HOST_ID_COL_NAME].Equals(drLive[AlertsUserControl.ALERT_CREATOR_HOST_ID_COL_NAME]) &&
                        drNew[AlertsUserControl.ALERT_CREATOR_PROCESS_ID_COL_NAME].Equals(drLive[AlertsUserControl.ALERT_CREATOR_PROCESS_ID_COL_NAME]))
                    {                        
                        toRemoved.Add(drLive);
                    }
                }
            }
            foreach (var dr in toRemoved)
            {
                _theDataTable.Rows.Remove(dr);
            }
            _theDataTable.Merge(newComingDataTable);
            _theDataTable = _theDataTable.DefaultView.ToTable();
        }


        public DataTable TransformLiveProblemDataTable(DataTable aProtoBufDataTable, bool openAlert) 
        {
            DataTable dt = _theDataTable.Clone();
            foreach (DataRow dr in aProtoBufDataTable.Rows) 
            {
                DataRow newRow = dt.NewRow();
                foreach (var pair in openAlert ? repository_protobuf_mapping_dictionary_open : repository_protobuf_mapping_dictionary_close) 
                {
                    if (dr.Table.Columns.Contains(pair.Value) && !pair.Value.Equals("")) 
                    {
                        try
                        {
                            newRow[pair.Key] = CastLiveData(pair.Value, dr);
                        }
                        catch (Exception)
                        {                            
                        }
                    }
                        
                }
                FillDimentionInfo(newRow);
                dt.Rows.InsertAt(newRow, 0); //Always top, because latest one is coming.
            }
            return dt;
        }

        private DataRow FillDimentionInfo(DataRow dr) 
        {    
            try
            {
                //SEVERITY_NAME        
                int id1 = (int)dr[DIMENSION_SEVERITY];
                dr[DIMENSION_SEVERITY_NAME] = DIMENSION_SEVERITY_TABLE.Select(DIMENSION_SEVERITY + " = " + id1)[0][DIMENSION_SEVERITY_NAME];

                //PROBLEMTYPE

                int id2 = (int)dr[DIMENSION_TYPE];
                dr[DIMENSION_TYPE_DESCRIPTION] = DIMENSION_PROBLEMTYPE_TABLE.Select(DIMENSION_TYPE + " = " + id2)[0][DIMENSION_TYPE_DESCRIPTION];

                //COMPONENT_NAME 
                //COMPONENT_ID is long type, not int
                long id3 = (long)dr[DIMENSION_COMPONENT_ID];
                dr[DIMENSION_COMPONENT_NAME] = DIMENSION_COMPONENT_TABLE.Select(DIMENSION_COMPONENT_ID + " = " + id3)[0][DIMENSION_COMPONENT_NAME];

                //PROBLEM_COMPONENT_NAME
                long id4 = (long)dr[DIMENSION_PROBLEM_COMPONENT_ID];
                dr[DIMENSION_PROBLEM_COMPONENT_NAME] = DIMENSION_COMPONENT_TABLE.Select(DIMENSION_COMPONENT_ID + " = " + id4)[0][DIMENSION_COMPONENT_NAME];
            }
            catch (Exception)
            {

            }
            return dr;
        }


        private object CastLiveData(string colName, DataRow dr) 
        {
            object returnObject;
            switch (colName)
            {
                case "info_header_info_generation_time_ts_lct":
                case "creation_time_ts_lct":
                    returnObject=Utilities.GetFormattedTimeFromUnixTimestampLCT((long)dr[colName]);
                    break;
                case "info_header_info_generation_time_ts_utc":     
                case "creation_time_ts_utc":
                    returnObject = Utilities.GetFormattedTimeFromUnixTimestampUTC((long)dr[colName]);
                    break;
                default:
                    returnObject = dr[colName];
                    break;
            }
            return returnObject;
        }



        #endregion Private Methods

    }
}
