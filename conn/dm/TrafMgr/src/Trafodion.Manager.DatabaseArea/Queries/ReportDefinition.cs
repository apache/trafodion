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
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
using System.ComponentModel;
using Trafodion.Manager.DatabaseArea.Queries.Controls;
using Trafodion.Manager.DatabaseArea.NCC;

namespace Trafodion.Manager.DatabaseArea.Queries
{

    [Serializable]
    abstract public class ReportDefinition
    {
        //Report types
        public const int TEXT_REPORT = 1;
        public const int CRYSTAL_REPORT = 2;

        //Report origins
        public const int STANDARD_REPORT = 1;
        public const int USER_REPORT = 2;

        /// <summary>
        /// Report properties
        /// </summary>
        public const string ID = "ID";
        public const string TYPE = "TYPE";
        public const string ORIGIN = "ORIGIN";
        public const string VERSION = "VERSION";
        public const string DEFINITION = "DEFINITION";
        public const string CREATE_TIME = "CREATE";
        public const string MODIFIED_TIME = "MODIFIED";
        public const string PARAMETERS = "PARAMS";
        public const string RAW_QUERY = "RAW_QUERY"; // Same as DEFINITION for simple report
        public const string ACTUAL_QUERY = "ACTUAL_QUERY"; // RAW_QUERY with parameters substituted
        public const string LAST_EXECUTED_AT_TIME = "LAST_EXECUTED_AT_TIME"; // Last time this query was executed
        public const string LAST_EXECUTION_TIME = "LAST_EXECUTION_TIME"; // How long it took for the last command to be executed
        public const string ROWS_AFFECTED = "ROWS_AFFECTED"; // How may rows were affected by the command execution
        public const string LAST_EXECUTION_STATUS = "LAST_EXECUTION_STATUS"; //Successful or cancelled by user
        public const string FULL_RAW_TEXT = "FULL_RAW_TEXT"; // full statement text, used in case when executing only highlighted portion of text
        public const string LAST_FETCH_TIME = "LAST_FETCH_TIME"; // How long it took for the last command to fetch all the data

        public const string FETCH_NEXT_PAGE = "FETCH_NEXT_PAGE";
        public const string CURRENT_EXECUTION_STATUS = "CURRENT_EXECUTION_STATUS"; // current status
        public const string STATUS_COMPLETED = "COMPLETED";
        public const string STATUS_EXECUTING = "EXECUTING";
        public const string EXPLAIN_PLAN_DATA = "EXPLAIN_PLAN_DATA"; // used for explain plan return
        public const string CONTROL_STATEMENTS = "CONTROL_STATEMENTS"; // used to store control settings
        public const string LAST_EXECUTED_CONTROL_STATEMENTS = "LAST_EXECUTED_CONTROL_STATEMENT";  // used to store last executed control statements
        public const string ELAPSED_TIME = "ELAPSED_TIME";
        public const string LAST_EXECUTION_SYSTEM = "LAST_EXECUTION_SYSTEM";
        public const string FETCH_TABLE_STATS = "FETCH_TABLE_STATS";

        // For saving execution plan and result into files
        public const string SAVED_PLAN_TIME_LABEL = "SAVED_PLAN_TIME_LABEL";
        public const string SAVED_PLAN_ELAPSED_TIME_LABEL = "SAVED_PLAN_ELAPSED_TIME_LABEL";
        public const string SAVED_PLAN_STATUS_LABEL = "SAVED_PLAN_STATUS_LABEL";
        public const string SAVED_PLAN_EXCEPTION = "SAVED_PLAN_EXCEPTION";
        public const string SAVED_EXEC_TIME_LABEL = "SAVED_EXEC_TIME_LABEL";
        public const string SAVED_EXEC_ELAPSED_TIME_LABEL = "SAVED_EXEC_ELAPSED_TIME_LABEL";
        public const string SAVED_EXEC_STATUS_LABEL = "SAVED_EXEC_STATUS_LABEL";
        public const string SAVED_EXEC_EXCEPTION = "SAVED_EXEC_EXCEPTION";
        public const string START_TIME = "START_TIME";
        public const string DEFAULT_STATMENT_NAME_PREFIX = "statement_";

        public abstract string Name { get; set; }
        public abstract string Description { get; }
        public abstract string OneLineSummary { get; }

        public abstract object GetProperty(string key);
        public abstract void SetProperty(string key, object value);
        public abstract void ResetProperty();

        public abstract Control ResultContainer { get; set; }
        public abstract Control PlanContainer { get; set; }

        [NonSerialized]
        private DataTable _dataTable = new DataTable();
        private bool _isGridReport = true;

        [NonSerialized]
        private Operation _currentOperation = Operation.Execute;

        /// <summary>
        /// Property: CurrentOperation - the current operation
        /// </summary>
        public Operation CurrentOperation
        {
            get { return _currentOperation; }
            set { _currentOperation = value; }
        }

        /// <summary>
        /// Property: IsGridReport - is this a grid report
        /// </summary>
        public bool IsGridReport
        {
            get { return _isGridReport; }
            set { _isGridReport = value; }
        }

        /// <summary>
        /// Property: DataTable 
        /// </summary>
        public DataTable DataTable
        {
            get { return _dataTable; }
            set { _dataTable = value; }
        }

        /// <summary>
        /// Event reasons
        /// </summary>
        public enum Reason
        {
            NameChanged,
            StatementChanged,
            ControlSettingChanged,
            ExecutionCompleted,
            ResultsDiscarded
        }

        /// <summary>
        /// Operation types
        /// </summary>
        public enum Operation { Execute, Explain }

        /// <summary>
        /// A changed handler
        /// </summary>
        /// <param name="aSender">The source for the event</param>
        /// <param name="aReportDefinition">The connection definition</param>
        public delegate void ChangedHandler(object aSender, ReportDefinition aReportDefinition, Reason aReason);

        [NonSerialized()]
        private static readonly string theChangedKey = "Changed";

        /// <summary>
        /// Add an event handler to, or remove one from, the list
        /// </summary>
        static public event ChangedHandler Changed
        {
            add { theEventHandlers.AddHandler(theChangedKey, value); }
            remove { theEventHandlers.RemoveHandler(theChangedKey, value); }
        }

        /// <summary>
        /// Fire an event with this connection definition as the source
        /// </summary>
        protected void FireChanged(Reason aReason)
        {
            FireChanged(this, this, aReason);
        }

        /// <summary>
        /// Fire an event with a given object as the source
        /// </summary>
        /// <param name="aSender">The source for the event</param>
        /// <param name="aReason"></param>
        protected void FireChanged(object aSender, Reason aReason)
        {
            FireChanged(aSender, this, aReason);
        }

        /// <summary>
        /// To Reset cached datatable 
        /// </summary>
        public void ResetDataTable()
        {
            //if (DataTable != null)
            //    DataTable.Dispose();
            //DataTable = new DataTable();
            if (DataTable != null)
            {
                DataTable.Clear();
            }
            else
            {
                DataTable = new DataTable();
            }
        }

        /// <summary>
        /// Raise execute complete event
        /// </summary>
        public void RaiseExecuteCompleteEvent()
        {
            FireChanged(Reason.ExecutionCompleted);
        }

        /// <summary>
        /// Raise result discarded event
        /// </summary>
        public void RaiseResultsDiscardedEvent()
        {
            FireChanged(Reason.ResultsDiscarded);
        }

        /// <summary>
        /// Fire an event for a given connection definition with a given object as the source
        /// </summary>
        /// <param name="aSender">The source for the event</param>
        /// <param name="aReportDefinition">The connection definition</param>
        static protected void FireChanged(object aSender, ReportDefinition aReportDefinition, Reason aReason)
        {

            // Get the list of the right kind of handlers
            ChangedHandler theChangedHandlers = (ChangedHandler)theEventHandlers[theChangedKey];

            // Check to see if there any
            if (theChangedHandlers != null)
            {
                // Multicast to them all
                theChangedHandlers(aSender, aReportDefinition, aReason);
            }
        }

        [NonSerialized()]
        static private EventHandlerList theEventHandlers = new EventHandlerList();
    }
}
