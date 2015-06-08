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
using System.Collections;
using System.Data;
using Trafodion.Manager.Framework;

//Ported from NPA SQ R1.0.
//Class used to store query related info
namespace Trafodion.Manager.DatabaseArea.NCC
{
    /// <summary>
    /// Class: NCCWorkbenchQueryData - to house Query plan data and table stats
    /// </summary>
	[Serializable]
	public class NCCWorkbenchQueryData
    {
        #region Fields

        //Public 

        //Execution result
        public enum ExplainResult { Successful, Get_TableStats_Error, Get_Explain_Error };

		public ArrayList QueryPlanArray = new ArrayList();
		public ArrayList ControlStatements = new ArrayList();
		public String queryVersion = "";  
				
		//Holds an error if the explain plan failed
		public String planErrorString;

		public bool explainPlanCached = false;
		public bool executeQueryCached = false;
		public bool QueryHasExecuted = false;
		public DataTable queryRunDataTable;
		public String queryRunDataErrorString;
        public String controlQueryShape = null;

        //Hash table used to help build explain plan tree
        public Hashtable planStepsHT = new Hashtable();

        // Private members
        private ArrayList _maxRows = new ArrayList();
        private int _currentMaxRowsSelectedIndex = 0;  
		private QueryPlanData _queryPlanData;
		private QueryPlanData _rootPlan;
		private String  _maxRowsToUse = null;
		private QueryStatsData _queryStatsData;
		private PlanSummaryInfo _planSummaryInfo = null;
        private ExplainResult _result = ExplainResult.Successful;
		private String _qryNamespace = null;
        private String _fetchPlanError = null;
        private String _fetchTableStatsError = null;
        private Hashtable _tableDetails_ht = new Hashtable();
        private Hashtable _lastTableDetailUpdate_ht = new Hashtable();

        [NonSerialized]
        private bool _skipTableStats = true;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: FetchTableStats - Indicates if table statistics should be fetched after explain
        /// </summary>
        public bool SkipTableStats
        {
            get { return _skipTableStats; }
            set { _skipTableStats = value; }
        }

        /// <summary>
        /// Property: MaxRows - Keeps a record of all the rows retrieved values
        /// </summary>
        public ArrayList MaxRows
        {
            set { _maxRows = value; }
            get { return _maxRows; }

        }
        
        /// <summary>
        /// Property: QueryNamespace
        /// </summary>
        public String QueryNamespace
        {
            get { return _qryNamespace; }
            set { this._qryNamespace = value; }
        }

        /// <summary>
        /// Property: currentMaxRowsSelectedIndex
        /// </summary>
        public int currentMaxRowsSelectedIndex
        {
            set { _currentMaxRowsSelectedIndex = value; }
            get { return _currentMaxRowsSelectedIndex; }
        }

        /// <summary>
        /// Property: CurrentMaxRowCount
        /// </summary>
        public int CurrentMaxRowCount
        {
            set { _maxRowsToUse = value.ToString(); }
            get { return Int16.Parse(_maxRowsToUse); }
        }

        /// <summary>
        /// gets or sets the queryPlanData items
        /// </summary>
        public QueryPlanData queryPlanData
        {
            set { _queryPlanData = value; }
            get { return _queryPlanData; }
        }

        /// <summary>
        /// gets or sets the queryStatsData items
        /// </summary>
        public QueryStatsData queryStatsData
        {
            set { _queryStatsData = value; }
            get { return _queryStatsData; }
        }

        /// <summary>
        /// Property: planSummaryInfo
        /// </summary>
        public PlanSummaryInfo planSummaryInfo
        {
            set { _planSummaryInfo = value; }
            get { return _planSummaryInfo; }
        }

        /// <summary>
        /// Property: rootPlan - Used to hold Root plan for tree
        /// </summary>
        public QueryPlanData rootPlan
        {
            set { _rootPlan = value; }
            get { return _rootPlan; }
        }

        /// <summary>
        /// Property: LastTableDetailUpdateHashtable - To hold the last retrieved table details
        /// </summary>
        public Hashtable LastTableDetailUpdateHashtable
        {
            get { return _lastTableDetailUpdate_ht; }
            set { _lastTableDetailUpdate_ht = value; }
        }

        /// <summary>
        /// Property: FetchTableStatsError - Any error encountered while fetching table stats
        /// </summary>
        public String FetchTableStatsError
        {
            get { return _fetchTableStatsError; }
            set { _fetchTableStatsError = value; }
        }

        /// <summary>
        /// Property: FetchPlanError - Any error encountered while fetching explain plan
        /// </summary>
        public String FetchPlanError
        {
            get { return _fetchPlanError; }
            set { _fetchPlanError = value; }
        }

        /// <summary>
        /// Property: Result - the result of fetching explain plan and table stats
        /// </summary>
        public ExplainResult Result
        {
            get { return _result; }
            set { _result = value; }
        }

        /// <summary>
        /// Property: TableDetailsHashtable - To hold the table names, which stats have been retrieved.
        /// </summary>
        public Hashtable TableDetailsHashtable
        {
            get { return _tableDetails_ht; }
            set { _tableDetails_ht = value; }
        }

        #endregion Properties

        #region Constructor

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="qryNamespace"></param>
        /// <param name="fetchLimit"></param>
        public NCCWorkbenchQueryData(String qryNamespace, int fetchLimit) {
			this._maxRowsToUse = fetchLimit.ToString();
			_queryStatsData = new QueryStatsData();
			_qryNamespace = qryNamespace;
        }

        #endregion Constructor

        #region Public inner classes

        /// <summary>
        /// Class: ControlStatementHolder
        /// </summary>
        [Serializable]
		public class ControlStatementHolder
		{
			public bool disable = false;
			public String controlType;
			public String controlAttribute;
			public String defaultValue;
		}

        /// <summary>
        /// Class: QueryStatsData
        /// </summary>
		[Serializable]
		public class QueryStatsData
		{
            /// <summary>
            /// Public members
            /// </summary>
			public string Query_ID = "";
			public string Start_Time;
			public string End_Time;
			public string Elapsed_Time;
			public string Data_Source;
			public string Error_Code;
			public string Est_Cost;
			public string Odbc_Elapsed_Time;
			public string Odbc_Execution_Time;
			public string Disc_Reads;
			public string Client_Id;
			public string Lock_Escalations;
			public string MsgsBytesToDisc;
			public string Lock_Waits;
			public string MsgsToDisc;
			public string Rows_Accessed;
			public string Rows_Retrieved;
			public string Num_Rows_Uid;
			public string Total_Executes;
			private string _sql_text = "";
			public string Segment_Id;
			public string Cpu_Num;
			public string Entry_Id;
			public string Sequence_num;
			public string Statment_Status;
			public string Total_Odbc_Elapsed_Time;
			public string Total_Odbc_Execution_Time;
			private string _statementLabel = null;
			private String _schemaName = "";
			public String SessionID = "";

			public string SQL_Text
			{
				get { return _sql_text; }
				set
				{

					if ((null == _statementLabel) ||
						((null != _sql_text) && _sql_text.Equals(_statementLabel)))
						_statementLabel = value;

					_sql_text = value;
				}
			}

			public string StatementLabel
			{
				get { return _statementLabel; }
				set { _statementLabel = value; }
			}

			public String SchemaName
			{
				get { return this._schemaName; }
				set { this._schemaName = value; }
			}

            /// <summary>
            /// Copy method
            /// </summary>
            /// <param name="updatedStats"></param>
			public void updateQueryStats(QueryStatsData updatedStats) {
				this.End_Time = updatedStats.End_Time;
				this.Elapsed_Time = updatedStats.Elapsed_Time;
				this.Error_Code = updatedStats.Error_Code;
				this.Est_Cost = updatedStats.Est_Cost;
				this.Odbc_Elapsed_Time = updatedStats.Odbc_Elapsed_Time;
				this.Odbc_Execution_Time = updatedStats.Odbc_Execution_Time;

				this.Disc_Reads = updatedStats.Disc_Reads;
				this.Lock_Escalations = updatedStats.Lock_Escalations;
				this.MsgsBytesToDisc = updatedStats.MsgsBytesToDisc;
				this.Lock_Waits = updatedStats.Lock_Waits;
				this.MsgsToDisc = updatedStats.MsgsToDisc;
				this.Rows_Accessed = updatedStats.Rows_Accessed;
				this.Rows_Retrieved = updatedStats.Rows_Retrieved;
				this.Num_Rows_Uid = updatedStats.Num_Rows_Uid;
				this.Total_Executes = updatedStats.Total_Executes;
				this.Entry_Id = updatedStats.Entry_Id;
				this.Statment_Status = updatedStats.Statment_Status;
				this.Total_Odbc_Elapsed_Time = updatedStats.Total_Odbc_Elapsed_Time;
				this.Total_Odbc_Execution_Time = updatedStats.Total_Odbc_Execution_Time;
			}

            /// <summary>
            /// To print the class
            /// </summary>
            /// <returns></returns>
			public override String ToString()
			{
				if (null != _statementLabel)
					return _statementLabel;

				return base.ToString();
			}
		}

        /// <summary>
        /// Class: QueryPlanData
        /// </summary>
		[Serializable]
		public class QueryPlanData
		{
			//  public String planID;
			public String sequenceNumber;
			//  public String statementName;
			public String theOperator;
			public String leftChildSeqNum;
			public String rightChildSeqNum;
			public String tableName;
			public String cardinality;
			public String operatorCost;
			public String totalCost;
			public String detailCost;
			public String description;

            public String GetTableName()
            {
                String tableName = "";

                try
                {
                    tableName = this.tableName.Trim();
                    if (tableName.EndsWith(")"))
                    {
                        int startIdx = tableName.IndexOf("(");
                        int endIdx = tableName.Length - 2;
                        tableName = tableName.Substring(startIdx + 1, (endIdx - startIdx));
                    }
                }
                catch (Exception)
                {
                }

                return tableName;
            } 
		}

        /// <summary>
        /// Class: PlanSummaryInfo
        /// </summary>
		[Serializable]
		public class PlanSummaryInfo
		{
			public int totalOperators = 0;
			public int totalEspExchanges = 0;
			public int totalEspProcesses = 0;
			public int totalScans = 0;
			public int totalNestedJoins = 0;
			public int totalHashJoins = 0;
			public int totalMergeJoins = 0;
			public int totalSorts = 0;
			public int totalIUDs = 0; //Total Inserts, Updates, Deletes
			public int totalOverallJoins = 0; //Total of all join type operators
			public int totalChildProcesses = 0;
        }

        #endregion Public inner classes

        #region Public methods

        /// <summary>
        /// Calculates summary values for quick evaluation to the 
        /// user
        /// </summary>
        /// <param name="qpa"> Array that contains the plan values from getplan</param>
        public void CreateSummary(ArrayList qpa)
        {
            if (null == qpa)
            {
                // Need to regenerate query plan array as earlier code had a bug if explain was
                // called multiple times for a query. QueryPlanArray would just keep growing ...
                this.QueryPlanArray.Clear();

                if (0 >= planStepsHT.Count)
                    return;

                // Cache away the explain data
                foreach (DictionaryEntry de in planStepsHT)
                {
                    QueryPlanData qdp = (QueryPlanData)de.Value;

                    //Store the plan item in the plan array
                    this.QueryPlanArray.Add(qdp);
                }

                qpa = this.QueryPlanArray;
            }


            PlanSummaryInfo psi = new PlanSummaryInfo();

            foreach (QueryPlanData qpd in qpa)
            {
                psi.totalOperators++;
                String Operator = qpd.theOperator;

                if (Operator.Contains("ESP_EXCHANGE"))
                {
                    psi.totalEspExchanges++;

                    //Get number of child processes
                    try
                    {
                        String cpValue = retrieveDescriptionValueOf("child_processes:", qpd.description);
                        psi.totalChildProcesses += Int16.Parse(cpValue);
                    }
                    catch (Exception)
                    {
                        //If this ever happens it means that someone in the Explain 
                        //group changed a string in the Explain Plan output -- 
                        //Might be better to display a message that there is some incompatibility issues.
                        psi.totalChildProcesses = -1;
                    }
                }
                if (Operator.Contains("SCAN"))
                    psi.totalScans++;
                if (Operator.Contains("NESTED") && Operator.Contains("JOIN"))
                {
                    psi.totalNestedJoins++;
                    psi.totalOverallJoins++;
                }
                if (Operator.Contains("HASH") && Operator.Contains("JOIN"))
                {
                    psi.totalHashJoins++;
                    psi.totalOverallJoins++;
                }
                if (Operator.Contains("MERGE") && Operator.Contains("JOIN"))
                {
                    psi.totalMergeJoins++;
                    psi.totalOverallJoins++;
                }
                if (Operator.Contains("SORT"))
                {
                    psi.totalSorts++;
                }

                if (Operator.Contains("INSERT") || Operator.Contains("UPDATE") || Operator.Contains("DELETE"))
                    psi.totalIUDs++;
            }

            if (Logger.IsTracingEnabled)
            {
                Logger.OutputToLog(TraceOptions.TraceOption.SQL,
                                   TraceOptions.TraceArea.Database,
                                   "Query Plan",
                                   "NCCWorkbenchQueryData::Summary: \n" +
                                            "  ESPs = " + psi.totalEspExchanges + " \n" +
                                            "  SCANs = " + psi.totalScans + " \n" +
                                            "  NESTED JOINS = " + psi.totalScans + " \n" +
                                            "  HASH JOINS = " + psi.totalHashJoins + " \n" +
                                            "  MERGE JOINS = " + psi.totalMergeJoins + "\n" +
                                            "  SORTS = " + psi.totalSorts + "\n"
                                  );
            }

            planSummaryInfo = psi;
        }

        /// <summary>
        /// This function stores the plan steps in and sets the root 
        /// to later aid in building a treeview
        /// </summary>
        /// <param name="planDataArray"></param>
        public void SavePlanSteps(ArrayList planDataArray)
        {
            QueryPlanData qdp = null;
            planStepsHT.Clear();
            for (int i = 0; i != planDataArray.Count; i++)
            {
                qdp = (QueryPlanData)planDataArray[i];

                int seqNum = getIntValue(qdp.sequenceNumber);
                if (0 < seqNum)
                {

                    planStepsHT.Add(seqNum, qdp);

                    String operatorName = qdp.theOperator;
                    if ((null != operatorName) &&
                        ("ROOT".Equals(operatorName.Trim().ToUpper())))
                        _rootPlan = qdp;
                }
            }
        }

        /// <summary>
        /// To initialize control statements
        /// </summary>
        public void initializeControlStatements()
        {
            if (null == ControlStatements)
                ControlStatements = new ArrayList();

            ControlStatements.Clear();
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// Parse a string for int value
        /// </summary>
        /// <param name="s"></param>
        /// <returns>Returns -1 if parsing fails</returns>
        private static int getIntValue(String s)
		{
			int val = -1;

			if ((null == s) || (0 >= s.Trim().Length))
				return -1;

			try
			{
				val = Int32.Parse(s.Trim());

			}
			catch (Exception)
			{
				val = -1;
			}

			return val;
		}

		/// <summary>
		/// Retrieves the actual description value of an item in the Description field
		/// the plan
		/// </summary>
		/// <param name="descValue"></param>
        /// <param name="fullDescription"></param>
		/// <returns>String</returns>
		private String retrieveDescriptionValueOf(String descValue, String fullDescription)
		{
			try
			{
				int index = fullDescription.IndexOf(descValue);
				String str = fullDescription.Substring(index);
				String[] strs;
				strs = str.Split(new char[] { ' ' });
				str = strs[1];
				return str;
			}
			catch(Exception)
			{
				return "";
			}
		}

        #endregion Private methods
    }
}
