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
using System.Text;
using System.Data.Odbc;
using System.Windows.Forms;
using System.Threading;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.WorkloadArea.Controls;

namespace Trafodion.Manager.WorkloadArea.Model
{
	public class TriageFilterQueryData
	{
		#region Members
        private enum Dimension {App, Client, DataSource, UserName };
		private List<String> m_appIDs = null;
		private List<String> m_clientIDs = null;
		private List<String> m_dataSources = null;
		private List<String> m_userNames = null;
#if INCLUDE_EXTRA
		private List<String> m_errors = null;
		private List<String> m_types = null;
		private List<String> m_states = null;
		private List<String> m_startTimes = null;
		private List<String> m_endTimes = null;
		private List<String> m_elapsedTimes = null;
#endif
		private TriageHelper _theTriageHelper = null;
		private bool m_completed = false;
        private ConnectionDefinition _theConnectionDefinition = null;
        private Connection _theCurrentConnection = null;

		private bool _abortingConnection = false;


		#endregion Members

		#region Properties
		public List<String> ApplicationIDs
		{
			get 
            {
                if (m_appIDs == null)
                {
                    m_appIDs = loadQueryDataThread(Dimension.App);
                }
                return m_appIDs; 
            }
			set { m_appIDs = value; }
		}

		public List<String> ClientIDs
		{
			get             
            {
                if (m_clientIDs == null)
                {
                    m_clientIDs = loadQueryDataThread(Dimension.Client);
                }
                return m_clientIDs; 
            }
			set { m_clientIDs = value; }
		}

		public List<String> DataSources
		{
			get 
            {
                if (m_dataSources == null)
                {
                    m_dataSources = loadQueryDataThread(Dimension.DataSource);
                } 
                return m_dataSources; 
            }
			set { m_dataSources = value; }
		}

		public List<String> UserNames
		{
			get 
            {
                if (m_userNames == null)
                {
                    m_userNames = loadQueryDataThread(Dimension.UserName);
                }
                return m_userNames; 
            }
			set { m_userNames = value; }
		}

#if INCLUDE_EXTRA
		public List<String> SQLErrors 
		{
			get { return m_errors; }
			set { m_errors = value; }
		}

		public List<String> SQLTypes
		{
			get { return m_types; }
			set { m_types = value; }
		}

		public List<String> QueryStates
		{
			get { return m_states; }
			set { m_states = value; }
		}

		public List<String> StartTimes
		{
			get { return m_startTimes; }
			set { m_startTimes = value; }
		}

		public List<String> EndTimes
		{
			get { return m_endTimes; }
			set { m_endTimes = value; }
		}

		public List<String> ElapsedTimes
		{
			get { return m_elapsedTimes; }
			set { m_elapsedTimes = value; }
		}
#endif 

		public bool LoadCompleted
		{
			get { return m_completed; }
			set { m_completed = value; }
		}
		#endregion Properties

        public TriageFilterQueryData(TriageHelper aTriageHelper) 
		{
            _theTriageHelper = aTriageHelper;
            this._theConnectionDefinition = aTriageHelper.TheConnectionDefinition;
		}

        //public void loadRepositoryQueryData()
        //{
        //    if (_theTriageHelper.IsConnected) {
        //        WaitCallback cb = new WaitCallback(this.loadQueryDataThread);
        //        ThreadPool.QueueUserWorkItem(cb, null);
        //    }

        //}


        //public void abortRepositoryQueryDataLoad() {
        //    lock (this) {
        //        if (!m_completed && (null != _theConnectionDefinition)) {
        //            this._abortingConnection = true;
        //            this._theConnectionDefinition = null;
        //        }
        //    }

        //    this._theTriageHelper.resetBackgroundConnection();
        //}
        
        /// <summary>
        /// Gets a new connection object
        /// </summary>
        /// <returns></returns>
        public bool GetConnection()
        {
            if (this._theCurrentConnection == null && this._theConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
            {
                _theCurrentConnection = new Connection(_theConnectionDefinition);
                return true;
            }
            else
            {
                return false;
            }
        }

        /// <summary>
        /// Closes a connecttion
        /// </summary>
        public void CloseConnection()
        {
            if (this._theCurrentConnection != null)
            {
                _theCurrentConnection.Close();
                _theCurrentConnection = null;
            }
        }

        private List<String> loadQueryDataThread(Dimension aDimension)
        {
            List<String> ret = new List<String>();
			String readAccessClause = " FOR READ UNCOMMITTED ACCESS";
            String viewName = _theTriageHelper.SchemaName + "." + _theTriageHelper.TheConnectionDefinition.MetricQueryView;
            try 
            {
                String repoSchemaName = this._theTriageHelper.Options.RepositorySchema;
                bool systemRepoInUse = Trafodion.Manager.WorkloadArea.Controls.WorkloadOption.SYSTEM_REPOSITORY_NAME.Equals(repoSchemaName);
                if (!systemRepoInUse)
                    viewName = repoSchemaName + "." + _theTriageHelper.TheConnectionDefinition.MetricQueryView;

            } 
            catch (Exception) 
            {
            }

            String applicationDimension = "APPLICATION_NAME";
            String clientDimension = "CLIENT_ID";
            if (_theTriageHelper.IsViewSingleRowPerQuery)
            {
                applicationDimension = "APPLICATION_NAME";
                clientDimension = "CLIENT_NAME";
            }

            String sql = "";
            String sqlWhere = "";

            if (aDimension == Dimension.App)
            {
                sqlWhere = " WHERE "+applicationDimension + " IS NOT NULL";
                sql = "SELECT DISTINCT UPPER(" + applicationDimension + ") FROM " + viewName +sqlWhere+ readAccessClause;
            }
            else if (aDimension == Dimension.Client)
            {
                sqlWhere = " WHERE " + clientDimension + " IS NOT NULL";
                sql = "SELECT DISTINCT UPPER(" + clientDimension + ") FROM " + viewName + sqlWhere + readAccessClause;
            }
            else if (aDimension == Dimension.DataSource)
            {
                sqlWhere = " WHERE DATASOURCE IS NOT NULL";
                sql = "SELECT DISTINCT DATASOURCE FROM " + viewName + sqlWhere + readAccessClause; ;
            }
            else if (aDimension == Dimension.UserName)
            {
                sqlWhere = " WHERE USER_NAME IS NOT NULL";
                sql = "SELECT DISTINCT UPPER(USER_NAME) FROM " + viewName + sqlWhere + readAccessClause;
            }


            Object[] parameters = new Object[] {sql};
            TrafodionProgressArgs args = new TrafodionProgressArgs("Getting values from server...", this, "GetListFromDB", parameters);
            TrafodionProgressDialog progressDialog = new TrafodionProgressDialog(args);
            progressDialog.ShowDialog();
            if (progressDialog.Error != null)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), progressDialog.Error.Message, "Error obtaining values from server",
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            else
            {
                ret = (List<String>)progressDialog.ReturnValue;
            }
            return ret;
		}  /*  End of  method  loadQueryThread.  */

        public List<String> GetListFromDB(string sql)
        {
            List<String> ret = new List<String>();
            if (GetConnection())
            {
                try
                {
                    OdbcCommand theQuery = new OdbcCommand(sql);
                    theQuery.Connection = _theCurrentConnection.OpenOdbcConnection;
                    OdbcDataReader reader = Trafodion.Manager.Framework.Utilities.ExecuteReader(theQuery, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Overview, TriageHelper.TRIAGE_SUB_AREA_NAME, false);
                    while (reader.Read())
                    {
                        ret.Add(reader.IsDBNull(0)?string.Empty:reader.GetString(0).Trim());
                    }
                }
                finally
                {
                    CloseConnection();
                }
            }
            return ret;
        }
	}
}
