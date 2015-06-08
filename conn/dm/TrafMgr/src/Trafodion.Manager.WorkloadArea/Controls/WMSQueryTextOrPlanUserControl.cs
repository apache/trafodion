//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
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
using System.Text;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.WorkloadArea.Model;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    /// <summary>
    /// User control to display query text or query plan
    /// </summary>
    public class WMSQueryTextOrPlanUserControl : WMSTextBoxUserControl
    {

        #region Fields

        private WMSWorkloadCanvas m_parent = null;
        private string m_qid = null;
        private string m_start_ts = null;
        private string m_sqlText = null;
        private string m_title = null;
        private bool m_getSqlPlan = true;
        #endregion Fields

        #region Properties

        #endregion Properties

        #region Constructors

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aTitle"></param>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="aProcessName"></param>
        public WMSQueryTextOrPlanUserControl(WMSWorkloadCanvas parent, ConnectionDefinition aConnectionDefinition, string qid, string start_ts, string sqlText, bool aToGetPlan)
        : base("", aConnectionDefinition)
        {
            m_qid = qid;
            m_start_ts = start_ts;
            m_sqlText = sqlText;
            m_getSqlPlan = aToGetPlan; 
            
            if (parent is OffenderWorkloadCanvas)
                this.Title = Properties.Resources.SystemOffender;
            else if (parent is MonitorWorkloadCanvas)
                this.Title = Properties.Resources.LiveWorkloads;

            this.IDTitle = Properties.Resources.QueryID;
            this.ID = m_qid;

            this.DisplayText = InitialText();
            this.HelpTopics = (m_getSqlPlan) ? Trafodion.Manager.WorkloadArea.HelpTopics.PlanText : Trafodion.Manager.WorkloadArea.HelpTopics.SQLText;

            this.WMSOffenderDatabaseDataProvider.FetchRepositoryDataOption = 
                (m_getSqlPlan) ? WMSOffenderDatabaseDataProvider.FetchDataOption.Option_SQLPlan : WMSOffenderDatabaseDataProvider.FetchDataOption.Option_SQLText;
            this.WMSOffenderDatabaseDataProvider.QueryID = qid;
            this.WMSOffenderDatabaseDataProvider.START_TS = start_ts;

            Start();
        }

        #endregion Constructors

        #region Public methods
        /// <summary>
        /// To initialize the TextBox
        /// </summary>
        /// <returns></returns>
        public override string InitialText()
        {
            return string.Format("-- QUERY_ID: {0}{1}", m_qid, Environment.NewLine);
        }

        /// <summary>
        /// Load new data to the TextBox
        /// </summary>
        /// <param name="aDataTable"></param>
        /// <returns></returns>
        public override string LoadNewData(DataTable aDataTable)
        {
            if (m_getSqlPlan)
            {
                return LoadSQLPlan(aDataTable);
            }
            else
            {
                return LoadSQLText();
            }
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// To load the fetched query plan to the container
        /// </summary>
        /// <param name="aDataTable"></param>
        public string LoadSQLPlan(DataTable aDataTable)
        {
            bool found = false;
            string planInfo = "";
            StringBuilder sb = new StringBuilder();

            sb.Append(string.Format("-- QUERY_ID: {0}", m_qid));
            sb.Append(Environment.NewLine);
            sb.Append(Environment.NewLine);

            if (aDataTable != null)
            {

                if (aDataTable != null)
                {
                    foreach (DataRow r in aDataTable.Rows)
                    {
                        object[] cols = r.ItemArray;
                        planInfo = (string)cols[0];
                        found = true;
                        break;
                    }
                }
            }

            if (found)
            {
                sb.Append(planInfo);
                sb.Append(Environment.NewLine);
            }
            else
            {
                sb.Append("SQL plan not available for the selected query");
                sb.Append(Environment.NewLine);
            }

            return sb.ToString();
        }

        /// <summary>
        /// To load the fetched query text to the container
        /// </summary>
        public string LoadSQLText()
        {

            bool found = false;
            StringBuilder sb = new StringBuilder();

            sb.Append(string.Format("-- QUERY_ID: {0}", m_qid));
            sb.Append(Environment.NewLine);
            sb.Append(Environment.NewLine);

            if (!string.IsNullOrEmpty(this.WMSOffenderDatabaseDataProvider.QueryText))
            {
                sb.Append(this.WMSOffenderDatabaseDataProvider.QueryText);
                sb.Append(Environment.NewLine);
            }
            else
            {
                sb.Append("SQL Text not available for the selected query");
                sb.Append(Environment.NewLine);
            }

            return sb.ToString();
        }

        #endregion Private methods
    }
}
