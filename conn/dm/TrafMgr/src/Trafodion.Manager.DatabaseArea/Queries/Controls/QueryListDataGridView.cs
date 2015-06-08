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
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Controls;
using TenTec.Windows.iGridLib;
using Trafodion.Manager.DatabaseArea.Model;

namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    /// <summary>
    /// This control is a list of the names and one line summaries of all of the persisted ReportDefinitons
    /// with a checkbox next to each.
    /// <para/>
    /// It also listens to the ReportDefinition class so that it can update the names and one line summaries of 
    /// any ReportDefinitions that it contains.  Rows in the grid should never be directly updated.
    /// <para/>
    /// Because its contents are entirely event driven, multiple instances of this control will all change in sync.
    /// </summary>
    /// Staement names is unique and have no specified syntax.
    public partial class QueryListDataGridView : UserControl
    {

        #region Member Variables

        /// <summary>
        /// Column key for the checkbox
        /// </summary>
        public const string COL_KEY_CHECKBOX = "theCheckBoxColumn";

        /// <summary>
        /// Column key for the group name
        /// </summary>
        public const string COL_KEY_GROUPNAME = "theGroupNameColumn";

        /// <summary>
        /// Column key for the report definition 
        /// </summary>
        public const string COL_KEY_REPORTDEFINITION = "theReportDefinitionColumn";

        /// <summary>
        /// Column key for the one line report definition summary 
        /// </summary>
        public const string COL_KEY_ONELINESUMMARY = "theOneLineSummaryColumn";

        /// <summary>
        /// Column key for the total row count
        /// Note: this is not visiable in the grid display
        /// </summary>
        public const string COL_KEY_TOTAL = "theTotalCountColumn";

        /// <summary>
        /// Column key for the start time
        /// Note: this is to note that the statement has been run at the start time. 
        /// </summary>
        public const string COL_KEY_STARTTIME = "theStartTimeColumn";

        /// <summary>
        /// Denoting the current selected row index so that we could simulate the selection changed event.
        /// </summary>
        private int _theCurrRowIndex = -1;

        /// <summary>
        /// Both of these are the options for displaying explain plan result.  
        /// SQLWhiteboard option handles these options.
        /// </summary>
        private bool _enableExplainColorProcessBoundaries = true;
        private bool _enableSortExplainGirdByLevels = true;

        private QueryStringsPersistence.AddedRemovedHandler _theReportDefinitionAddedRemovedHandler = null;
        private ReportDefinition.ChangedHandler _theReportDefinitionChangedHandler = null;

        /// <summary>
        /// Keys for event handlers
        /// </summary>
        private static readonly string _theCheckBoxChangedKey = "CheckBoxChanged";
        private static readonly string _theSelectionChangedKey = "SelectionChanged";
        private static readonly string _theDoubleClickedKey = "DoubleClicked";

        /// <summary>
        /// Define a hash table to store statement count in a group.
        /// </summary>
        private Hashtable _theGroupCount = new Hashtable();

        private EventHandlerList _theEventHandlers = new EventHandlerList();

        /// <summary>
        /// The manager for aggregate rows in groups
        /// </summary>
        private TrafodionIGridGroupRowCountManager _theGroupRowCountMgr = new TrafodionIGridGroupRowCountManager();

        /// <summary>
        /// The iGrid default font color.  When an executed statement reported an exception, the statement listed in
        /// the grid will turn into RED.  After a successful run of the same statement, we need to turn the font back
        /// to the original font color. 
        /// </summary>
        private Color _iGridDefaultForColor;

        /// <summary>
        /// This is to denote that the whiteboard is running in batch mode. 
        /// </summary>
        private bool _batchMode = false;

        #endregion

        #region Properties

        /// <summary>
        /// Property: CurrRowIndex - the Grid current index
        /// </summary>
        public int SelectedRowIndex
        {
            get 
            {
                if (iGrid1.SelectedRowIndexes != null && iGrid1.SelectedRowIndexes.Count > 0)
                {
                    return iGrid1.SelectedRowIndexes[0];
                }
                else
                {
                    return -1;
                }
            }
        }

        /// <summary>
        /// Property: Rows - the Grid's rows
        /// </summary>
        public iGRowCollection Rows
        {
            get { return iGrid1.Rows; }
        }

        /// <summary>
        /// Property: SelectedRow - the currently selected row
        /// </summary>
        public iGRow SelectedRow
        {
            get 
            {
                if (iGrid1.SelectedRows.Count > 0)
                {
                    return iGrid1.SelectedRows[0];
                }
                else
                {
                    return null;
                }
            }
        }

        /// <summary>
        /// Property: BatchMode - whether the whiteboard is running in batch mode.
        /// Note: basically lock down the grid so the batch execution will not be 
        ///       interrupted by user's mouse click. 
        /// </summary>
        public bool BatchMode
        {
            get { return _batchMode; }
            set 
            { 
                _batchMode = value;
                iGrid1.MouseDownLocked = value;
            }
        }

        #endregion Properties

        #region Constructor

        /// <summary>
        /// Default constructor 
        /// </summary>
        public QueryListDataGridView()
        {
            InitializeComponent();

            // First, use group row count manager to tally row counts
            _theGroupRowCountMgr.Attach(iGrid1, COL_KEY_TOTAL, CustomDrawCellForGroundMethod);
            iGrid1.GroupObject.Add(COL_KEY_GROUPNAME);
            iGrid1.SortObject.Add(COL_KEY_GROUPNAME);
            iGrid1.SelectionMode = TenTec.Windows.iGridLib.iGSelectionMode.One;

            // We're interested in reports being add or removed
            _theReportDefinitionAddedRemovedHandler = new QueryStringsPersistence.AddedRemovedHandler(QueryStringsPersistenceAddedRemoved);
            QueryStringsPersistence.AddedRemoved += _theReportDefinitionAddedRemovedHandler;

            // Also, listen to the report changes.
            _theReportDefinitionChangedHandler = new ReportDefinition.ChangedHandler(ReportDefinitionChanged);
            ReportDefinition.Changed += _theReportDefinitionChangedHandler;
            iGrid1.DoubleClickHandler = TheGrid_DoubleClick;
            iGrid1.AfterAutoGroupRowCreated += new iGAfterAutoGroupRowCreatedEventHandler(iGrid1_AfterAutoGroupRowCreated);
            this._iGridDefaultForColor = iGrid1.ForeColor;

            try
            {
                foreach (ReportDefinition theReportDefinition in QueryStringsPersistence.TheReportDefinitions)
                {
                    // Just loaded from the persistence file, clear the start time
                    theReportDefinition.SetProperty(ReportDefinition.START_TIME, null);
                    if (String.IsNullOrEmpty(theReportDefinition.Name))
                    {
                        // Fillin a new name if a blank name is saved by an older version.
                        theReportDefinition.Name = SimpleReportDefinition.CreateNewName("");
                    }
                    AddReportDefinition(theReportDefinition);
                }
            }
            catch (PersistenceNotLoadedException)
            {
                // We got here before it was loaded so let the events populate us when it IS loaded
            }

            iGrid1.PerformAction(iGActions.DeselectAllRows);

            //to enable use of SQL Whiteboard options
            _enableExplainColorProcessBoundaries = DatabaseAreaOptions.GetOptions().EnableExplainColorProcessBoundaries;
            _enableSortExplainGirdByLevels = DatabaseAreaOptions.GetOptions().EnableSortExplainGirdByLevels;
            DatabaseAreaOptions.GetOptions().DatabaseOptionsChanged += QueryListDataGridView_DatabaseOptionsChanged;
        }

        #endregion Constructor

        #region Public methods

        /// <summary>
        /// Delegate to handle changes in our checkboxes
        /// </summary>
        /// <param name="sender">the QueryListDataGridView whose checkbox changed</param>
        /// <param name="e">the DataGridViewCellEventArgs for the change</param>
        public delegate void CheckBoxChangedHandler(object sender, iGCellClickEventArgs e);

        /// <summary>
        /// The list of listeners to our checkbox chnages
        /// </summary>
        public event CheckBoxChangedHandler CheckBoxChanged
        {
            add { _theEventHandlers.AddHandler(_theCheckBoxChangedKey, value); }
            remove { _theEventHandlers.RemoveHandler(_theCheckBoxChangedKey, value); }
        }

        /// <summary>
        /// Delegate to handle changes in our grid selection
        /// </summary>
        /// <param name="sender">the QueryListDataGridView whose selection changed</param>
        /// <param name="e">the EventArg for the change</param>
        public delegate void SelectionChangedHandler(object sender, EventArgs e);

        /// <summary>
        /// The list of listeners to our selection changes
        /// </summary>
        public event SelectionChangedHandler SelectionChanged
        {
            add { _theEventHandlers.AddHandler(_theSelectionChangedKey, value); }
            remove { _theEventHandlers.RemoveHandler(_theSelectionChangedKey, value); }
        }

        /// <summary>
        /// Delegate to handle double clicked event
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="rowIndex"></param>
        public delegate void DoubleClickedHandler(object sender, int rowIndex);

        /// <summary>
        /// The list of listeners to our double clicked events
        /// </summary>
        public event DoubleClickedHandler OnDoubleClicked
        {
            add { _theEventHandlers.AddHandler(_theDoubleClickedKey, value); }
            remove { _theEventHandlers.RemoveHandler(_theDoubleClickedKey, value); }
        }

        /// <summary>
        /// Find out if a ReportDefinition is in this grid and return its index if so
        /// </summary>
        /// <param name="aReportDefinition">the ReportDefinition</param>
        /// <returns>-1 if not in this grid else row index</returns>
        public int IndexOfReportDefinition(ReportDefinition aReportDefinition)
        {
            return IndexOfReportDefinition(aReportDefinition, ((SimpleReportDefinition)aReportDefinition).Group);
        }

        /// <summary>
        /// Find out if a ReportDefinition is in this grid
        /// </summary>
        /// <param name="aReportDefinition">the ReportDefinition</param>
        /// <returns>true if in this grid else false</returns>
        public bool ContainsReportDefinition(ReportDefinition aReportDefinition)
        {
            return (IndexOfReportDefinition(aReportDefinition) >= 0);
        }

        /// <summary>
        /// Return the index of the report with the specified group name.
        /// </summary>
        /// <param name="aReportDefinition"></param>
        /// <param name="aGroupName"></param>
        /// <returns></returns>
        public int IndexOfReportDefinition(string aReportName, string aGroupName)
        {
            if (String.IsNullOrEmpty(aGroupName))
            {
                aGroupName = Properties.Resources.PersistenceFile;
            }

            int theIndex = 0;

            foreach (iGRow theRow in iGrid1.Rows)
            {
                if (theRow.Type == iGRowType.Normal)
                {
                    if (aGroupName.Equals(theRow.Cells[COL_KEY_GROUPNAME].Value as string, StringComparison.OrdinalIgnoreCase))
                    {
                        ReportDefinition theReportDefinition = GetReportDefinition(theRow);
                        if (theReportDefinition.Name.Equals(aReportName, StringComparison.OrdinalIgnoreCase))
                        {
                            return theIndex;
                        }
                    }
                }
                theIndex++;
            }

            return -1;
        }

        /// <summary>
        /// Find out if a ReportDefinition is in this grid
        /// </summary>
        /// <param name="aReportDefinition">the ReportDefinition</param>
        /// <returns>true if in this grid else false</returns>
        public bool ContainsReportDefinition(string aReportName, string aGroupName)
        {
            return (IndexOfReportDefinition(aReportName, aGroupName) >= 0);
        }

        /// <summary>
        /// Find out if a group name is in this grid
        /// </summary>
        /// <param name="aGroupName"></param>
        /// <returns></returns>
        public bool ContainsGroup(string aGroupName)
        {
            foreach (iGRow theRow in iGrid1.Rows)
            {
                if (theRow.Type == iGRowType.Normal)
                {
                    if (aGroupName.Equals(theRow.Cells[COL_KEY_GROUPNAME].Value as string, StringComparison.OrdinalIgnoreCase))
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        /// <summary>
        /// Get a list of reports in a specific grouop
        /// </summary>
        /// <param name="aGroupName"></param>
        /// <returns></returns>
        public List<ReportDefinition> GetGroupReports(string aGroupName)
        {
            List<ReportDefinition> reports = new List<ReportDefinition>();
            foreach (iGRow theRow in iGrid1.Rows)
            {
                if (theRow.Type == iGRowType.Normal)
                {
                    if (aGroupName.Equals(theRow.Cells[COL_KEY_GROUPNAME].Value as string, StringComparison.OrdinalIgnoreCase))
                    {
                        reports.Add(theRow.Cells[COL_KEY_REPORTDEFINITION].Value as ReportDefinition);
                    }
                }
            }

            return reports;
        }

        /// <summary>
        /// Remove the entire group from the grid
        /// </summary>
        /// <param name="aGroupName"></param>
        public void RemoveWholeGroup(string aGroupName)
        {
            foreach (iGRow theRow in iGrid1.Rows)
            {
                if (theRow.Type == iGRowType.Normal)
                {
                    if (aGroupName.Equals(theRow.Cells[COL_KEY_GROUPNAME].Value as string, StringComparison.OrdinalIgnoreCase))
                    {
                        ReportDefinition reportDefinition = GetReportDefinition(theRow);
                        if (reportDefinition != null)
                        {
                            if (reportDefinition.ResultContainer != null)
                            {
                                reportDefinition.ResultContainer.Dispose();
                                reportDefinition.ResultContainer = null;
                            }
                            if (reportDefinition.PlanContainer != null)
                            {
                                reportDefinition.PlanContainer.Dispose();
                                reportDefinition.PlanContainer = null;
                            }
                            RemoveReportDefinition(reportDefinition);
                        }
                    }
                }
            }

            try
            {
                //FireSelectionChanged(this, new EventArgs());
                iGrid1.Group();
            }
            catch (Exception)
            {
                //Currently, we jsut drop anything failed.
            }
        }

        /// <summary>
        /// Verify if the report is in the group.
        /// </summary>
        /// <param name="aReportDefinition"></param>
        /// <param name="aGroupName"></param>
        /// <returns></returns>
        private bool ContainsReportDefinition(ReportDefinition aReportDefinition, string aGroupName)
        {
            return (IndexOfReportDefinition(aReportDefinition, aGroupName) >= 0);
        }

        /// <summary>
        /// Return the index of the report with the specified group name.
        /// </summary>
        /// <param name="aReportDefinition"></param>
        /// <param name="aGroupName"></param>
        /// <returns></returns>
        private int IndexOfReportDefinition(ReportDefinition aReportDefinition, string aGroupName)
        {
            if (String.IsNullOrEmpty(aGroupName))
            {
                aGroupName = Properties.Resources.PersistenceFile;
            }

            int theIndex = 0;

            foreach (iGRow theRow in iGrid1.Rows)
            {
                if (theRow.Type == iGRowType.Normal)
                {
                    if (aGroupName.Equals(theRow.Cells[COL_KEY_GROUPNAME].Value as string, StringComparison.OrdinalIgnoreCase))
                    {
                        ReportDefinition theReportDefinition = GetReportDefinition(theRow);
                        if (aReportDefinition.Equals(theReportDefinition))
                        {
                            return theIndex;
                        }
                    }
                }
                theIndex++;
            }

            return -1;
        }


        /// <summary>
        /// Delete all rows whose checkbox is checked
        /// </summary>
        public void DeleteCheckedRows()
        {
            int currentRowIndex = (iGrid1.CurRow != null) ? iGrid1.CurRow.Index : -1;

            foreach (ReportDefinition theReportDefinition in CheckedReportDefinitions)
            {
                string executionStatus = theReportDefinition.GetProperty(ReportDefinition.CURRENT_EXECUTION_STATUS) as string;
                if (string.IsNullOrEmpty(executionStatus) || !executionStatus.Equals(ReportDefinition.STATUS_EXECUTING))
                {
                    if (theReportDefinition.ResultContainer != null)
                    {
                        theReportDefinition.ResultContainer.Dispose();
                        theReportDefinition.ResultContainer = null;
                    }
                    if (theReportDefinition.PlanContainer != null)
                    {
                        theReportDefinition.PlanContainer.Dispose();
                        theReportDefinition.PlanContainer = null;
                    }

                    if (((SimpleReportDefinition)theReportDefinition).Group == Properties.Resources.PersistenceFile)
                    {
                        // Update persistence store
                        QueryStringsPersistence.Remove(theReportDefinition);
                    }
                    else
                    {
                        // Just remove it from the grid
                        RemoveReportDefinition(theReportDefinition);
                    }
                }
            }

            if (iGrid1.Rows.Count == 0)
            {
                FireSelectionChanged(this, new EventArgs());
            }
            else
            {
                // If there are rows in the grid, perform re-grouping and select the next avilable row.
                try
                {
                    iGrid1.Group();
                    if (currentRowIndex > 0)
                    {
                        iGrid1.Rows[currentRowIndex].Selected = true;
                    }
                }
                catch (Exception)
                {
                }
            }
        }

        /// <summary>
        /// Delete results of reports whose checkbox is checked
        /// </summary>
        public void DiscardResultsOfCheckedRows()
        {
            foreach (ReportDefinition theReportDefinition in CheckedReportDefinitions)
            {
                string executionStatus = theReportDefinition.GetProperty(ReportDefinition.CURRENT_EXECUTION_STATUS) as string;
                if (string.IsNullOrEmpty(executionStatus) || !executionStatus.Equals(ReportDefinition.STATUS_EXECUTING))
                {
                    if (theReportDefinition.ResultContainer != null)
                    {
                        theReportDefinition.ResultContainer.Dispose();
                        theReportDefinition.ResultContainer = null;
                    }
                    if (theReportDefinition.PlanContainer != null)
                    {
                        theReportDefinition.PlanContainer.Dispose();
                        theReportDefinition.PlanContainer = null;
                    }
                    theReportDefinition.RaiseResultsDiscardedEvent();
                }
            }

            try
            {
                iGrid1.Group();
            }
            catch (Exception)
            {
            }
        }

        /// <summary>
        /// Sets or returns the currently selected ReportDefinition.  The code all depends that 
        /// it not be possible to select more than one ReportDefintion selected.
        /// <para/>
        /// get returns the selected ReportDefinition or null if none.
        /// <para/>
        /// set clears the selection and, if the specified ReportDefinition is present, selects it.
        /// </summary>
        public ReportDefinition SelectedReportDefinition
        {
            get
            {
                if (iGrid1.SelectedRows.Count < 1)
                {
                    return null;
                }
                if (iGrid1.SelectedRows.Count > 1)
                {
                    throw new System.NotSupportedException("The logic in this control and its users requires SINGLE selection");
                }

                if (iGrid1.SelectedRows[0].Type == iGRowType.Normal)
                {
                    // There should be only one
                    return GetReportDefinition(iGrid1.SelectedRows[0]);
                }
                else
                {
                    return null;
                }
            }
            set
            {
                if (iGrid1.SelectedRows.Count > 0)
                {
                    iGrid1.SelectedRows[0].Selected = false;
                }

                if (value != null)
                {
                    int theIndex = IndexOfReportDefinition(value);
                    if (theIndex >= 0)
                    {
                        iGrid1.Rows[theIndex].Selected = true;
                    }

                }
            }
        }

        /// <summary>
        /// The selected statement or the empty string if no ReportDefinition is selected.
        /// </summary>
        public string SelectedQueryString
        {
            get
            {
                ReportDefinition theReportDefinition = SelectedReportDefinition;
                string theQueryString = null;
                if (theReportDefinition != null)
                {
                    theQueryString = theReportDefinition.GetProperty(ReportDefinition.DEFINITION) as string;
                }
                return (theQueryString == null) ? "" : theQueryString;
            }
        }

        /// <summary>
        /// The selected statement's name or the empty string if no ReportDefinition is selected.
        /// </summary>
        public string SelectedQueryName
        {
            get
            {
                ReportDefinition theReportDefinition = SelectedReportDefinition;
                if (theReportDefinition != null)
                {
                    return theReportDefinition.Name;
                }
                return "";
            }
        }

        /// <summary>
        /// Returns a list of all of the ReportDefinitions whose checkboxes are set.  Returns an
        /// empty list is no checkboxes are set.
        /// </summary>
        public List<ReportDefinition> CheckedReportDefinitions
        {
            get
            {
                List<ReportDefinition> theCheckedReportDefinitions = new List<ReportDefinition>();
                foreach (iGRow theCheckedRow in CheckedRows)
                {
                    ReportDefinition theReportDefinition = GetReportDefinition(theCheckedRow);
                    if (theReportDefinition != null)
                    {
                        theCheckedReportDefinitions.Add(theReportDefinition);
                    }
                }
                return theCheckedReportDefinitions;
            }
        }

        /// <summary>
        /// Returns a list of all of the rows whose checkboxes are set.  Returns an
        /// empty list is no checkboxes are set.
        /// </summary>
        public List<iGRow> CheckedRows
        {
            get
            {
                List<iGRow> theRowCollection = new List<iGRow>();
                foreach (iGRow theRow in iGrid1.Rows)
                {
                    if (theRow.Type == iGRowType.Normal)
                    {
                        if ((bool)theRow.Cells[COL_KEY_CHECKBOX].Value)
                            theRowCollection.Add(theRow);
                    }
                }
                return theRowCollection;
            }
        }

        /// <summary>
        /// Returns true if all rows are checked.
        /// </summary>
        public bool AllRowsAreChecked
        {
            get
            {
                List<iGRow> theRowCollection = new List<iGRow>();
                foreach (iGRow theRow in iGrid1.Rows)
                {
                    if (theRow.Type == iGRowType.Normal)
                    {
                        if (!(bool)theRow.Cells[COL_KEY_CHECKBOX].Value)
                            return false;
                    }
                }
                return true;
            }
        }

        /// <summary>
        /// Set all of the checkboxes
        /// </summary>
        public void CheckAll()
        {
            SetAllCheckBoxes(true);
        }

        /// <summary>
        /// Clear all of the checkboxes
        /// </summary>
        public void UncheckAll()
        {
            SetAllCheckBoxes(false);
        }

        /// <summary>
        /// Returns the report definition contained in the given row
        /// </summary>
        /// <param name="aRowNumber">The row number</param>
        /// <returns>The report definition</returns>
        public ReportDefinition GetReportDefinition(int aRowNumber)
        {
            return iGrid1.Rows[aRowNumber].Cells[COL_KEY_REPORTDEFINITION].Value as ReportDefinition;
        }

        /// <summary>
        /// To load report definition 
        /// </summary>
        /// <param name="aReportDefinition"></param>
        /// <param name="from"></param>
        public void LoadReportDefinition(ReportDefinition aReportDefinition, string from)
        {
            AddReportDefinition(aReportDefinition, from);
        }

        /// <summary>
        /// To select the Grid.
        /// </summary>
        new public void Select()
        {
            iGrid1.Select();
        }

        /// <summary>
        /// To cause a double click
        /// </summary>
        /// <param name="rowIndex"></param>
        public void InvokeDoubleClicked(int rowIndex)
        {
            FireDoubleClicked(this, rowIndex);
        }

        /// <summary>
        /// Move those reports whose checkbox is checked to the persistence file
        /// </summary>
        public void MovedCheckedRowsToPersistenceFile()
        {
            foreach (ReportDefinition theReportDefinition in CheckedReportDefinitions)
            {
                string groupName = ((SimpleReportDefinition)theReportDefinition).Group;
                if (string.IsNullOrEmpty(groupName) || !groupName.Equals(Properties.Resources.PersistenceFile, StringComparison.OrdinalIgnoreCase))
                {
                    // Remove it from the grid first
                    RemoveReportDefinition(theReportDefinition);
                    if (QueryStringsPersistence.ReportDefinitionExists(theReportDefinition.Name))
                    {
                        ReportDefinition existingReport = QueryStringsPersistence.GetReportDefinition(theReportDefinition.Name);
                        if (existingReport != null)
                        {
                            theReportDefinition.Name = ((SimpleReportDefinition)existingReport).GetNewName();
                        }
                    }
                    // Add it back to the persistence file
                    QueryStringsPersistence.Add(theReportDefinition);
                }
            }

            try
            {
                iGrid1.Group();
            }
            catch (Exception)
            {
            }
        }

        /// <summary>
        /// Examine all checked statements to see if anyone is in the Non-Persistent Group.
        /// Note: this is used to enable to move to persistence file button
        /// </summary>
        /// <returns></returns>
        public bool CheckedRowsContainNonPersistentGroup()
        {
            foreach (ReportDefinition theReportDefinition in CheckedReportDefinitions)
            {
                string groupName = ((SimpleReportDefinition)theReportDefinition).Group;
                if (!Properties.Resources.PersistenceFile.Equals(groupName, StringComparison.OrdinalIgnoreCase))
                {
                    return true;
                }
            }
            return false;
        }

        /// <summary>
        /// To generate a unique statement name using total statement count.
        /// </summary>
        /// <returns></returns>
        public string GenerateUniqueQueryName()
        {
            int index = this.Rows.Count;
            string name = null;
            while (ContainsReportDefinition((name = String.Format("{0}{1}", ReportDefinition.DEFAULT_STATMENT_NAME_PREFIX, index.ToString("D3"))), Properties.Resources.PersistenceFile))
            {
                index++;
            }

            return name;
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// Grid double clicked event handler. 
        /// </summary>
        /// <param name="rowIndex"></param>
        private void TheGrid_DoubleClick(int rowIndex)
        {
            FireDoubleClicked(this, rowIndex);
        }

        /// <summary>
        /// Event handler
        /// </summary>
        /// <param name="e"></param>
        protected override void  OnKeyDown(KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Delete)
            {
            }
            else
            {
 	            base.OnKeyDown(e);
            }
        }

        /// <summary>
        /// Clean up after disposing. 
        /// </summary>
        /// <param name="disposing"></param>
        protected void myDispose(bool disposing)
        {
            if (_theReportDefinitionAddedRemovedHandler != null)
            {
                QueryStringsPersistence.AddedRemoved -= _theReportDefinitionAddedRemovedHandler;
                _theReportDefinitionAddedRemovedHandler = null;
            }
            if (_theReportDefinitionChangedHandler != null)
            {
                ReportDefinition.Changed -= _theReportDefinitionChangedHandler;
                _theReportDefinitionChangedHandler = null;
            }

            //to enable use of SQL Whiteboard options
            DatabaseAreaOptions.GetOptions().DatabaseOptionsChanged -= QueryListDataGridView_DatabaseOptionsChanged;
        }

        /// <summary>
        /// The handler of SQL whiteboard changed event
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void QueryListDataGridView_DatabaseOptionsChanged(object sender, EventArgs e)
        {
            // Verify if the changes are relevant to us 
            if (_enableExplainColorProcessBoundaries != DatabaseAreaOptions.GetOptions().EnableExplainColorProcessBoundaries ||
                _enableSortExplainGirdByLevels != DatabaseAreaOptions.GetOptions().EnableSortExplainGirdByLevels)
            {
                // Remember the new changes, and flag all existing plan to be reloaded.
                _enableExplainColorProcessBoundaries = DatabaseAreaOptions.GetOptions().EnableExplainColorProcessBoundaries;
                _enableSortExplainGirdByLevels = DatabaseAreaOptions.GetOptions().EnableSortExplainGirdByLevels;
                FlagDirtyPlans();
            }
        }

        /// <summary>
        /// When the whiteboard option is changed, all of the existing explain plans need to be reloaded.
        /// The reload will happen when a plan is to be displayed in the Details Tab page.
        /// </summary>
        private void FlagDirtyPlans()
        {
            for (int i = 0; i < iGrid1.Rows.Count; i++)
            {
                iGRow theRow = iGrid1.Rows[i];
                if (theRow.Type == iGRowType.Normal)
                {
                    ReportDefinition report = theRow.Cells[COL_KEY_REPORTDEFINITION].Value as ReportDefinition;
                    if (report != null)
                    {
                        if (report.PlanContainer != null)
                        {
                            ((QueryPlanContainer)report.PlanContainer).PlanIsDirty = true;
                        }    
                    }
                }
            }

            FireSelectionEvent(SelectedRowIndex);
        }

        /// <summary>
        /// Set all checkboxes in the grid to the new value.
        /// </summary>
        /// <param name="aNewValue"></param>
        private void SetAllCheckBoxes(bool aNewValue)
        {
            int theColumnIndex = iGrid1.Cols["theCheckBoxColumn"].Index;

            for (int i = 0; i < iGrid1.Rows.Count; i++)
            {
                iGRow theRow = iGrid1.Rows[i];
                if (theRow.Type == iGRowType.Normal)
                {
                    if ((bool)theRow.Cells[COL_KEY_CHECKBOX].Value != aNewValue)
                    {
                        theRow.Cells[COL_KEY_CHECKBOX].Value = aNewValue;
                        FireCheckBoxChanged(this, new iGCellClickEventArgs(i, theColumnIndex, new System.Drawing.Rectangle(), Keys.Down, iGControl.CheckBox));
                    }
                }
                else if (theRow.Type == iGRowType.AutoGroupRow || theRow.Type == iGRowType.ManualGroupRow)
                {
                    if (aNewValue)
                    {
                        theRow.Cells[-1].ImageIndex = 1;
                    }
                    else
                    {
                        theRow.Cells[-1].ImageIndex = 0;
                    }
                }
            }
        }

        /// <summary>
        /// Set all checkboxes belong to the given group with the new value.  
        /// </summary>
        /// <param name="aGroupName"></param>
        /// <param name="aNewValue"></param>
        private void SetEntireGroupCheckBoxes(string aGroupName, bool aNewValue)
        {
            int theColumnIndex = iGrid1.Cols["theCheckBoxColumn"].Index;

            for (int i = 0; i < iGrid1.Rows.Count; i++)
            {
                iGRow theRow = iGrid1.Rows[i];
                if (theRow.Type == iGRowType.Normal &&
                    aGroupName.Equals(theRow.Cells[COL_KEY_GROUPNAME].Value as string, StringComparison.OrdinalIgnoreCase))
                {
                    if ((bool)theRow.Cells[COL_KEY_CHECKBOX].Value != aNewValue)
                    {
                        theRow.Cells[COL_KEY_CHECKBOX].Value = aNewValue;
                        FireCheckBoxChanged(this, new iGCellClickEventArgs(i, theColumnIndex, new System.Drawing.Rectangle(), Keys.Down, iGControl.CheckBox));
                    }
                }
            }
        }

        /// <summary>
        /// Verify all checkboxes belong to the given group to see if they are all checked or unchecked.
        /// </summary>
        /// <param name="aGroupName"></param>
        /// <param name="aNewValue"></param>
        private bool VerifyEntireGroupCheckBoxes(string aGroupName, bool aValue, out int aFirstRowId)
        {
            aFirstRowId = -1;
            if (iGrid1.Rows.Count == 0)
            {
                return false;
            }

            for (int i = 0; i < iGrid1.Rows.Count; i++)
            {
                iGRow theRow = iGrid1.Rows[i];
                if (theRow.Type == iGRowType.Normal &&
                    aGroupName.Equals(theRow.Cells[COL_KEY_GROUPNAME].Value as string, StringComparison.OrdinalIgnoreCase))
                {
                    if (aFirstRowId == -1)
                    {
                        aFirstRowId = theRow.Index;
                    }

                    if ((bool)theRow.Cells[COL_KEY_CHECKBOX].Value != aValue)
                    {
                        return false;
                    }
                }
            }

            return true;
        }

        /// <summary>
        /// Fire up the Checkbox changed event with cell click event args.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void FireCheckBoxChanged(object sender, iGCellClickEventArgs e)
        {

            // Get all of the listeners to the changed event
            CheckBoxChangedHandler theCheckBoxChangedHandlers = (CheckBoxChangedHandler)_theEventHandlers[_theCheckBoxChangedKey];

            // Check to see if there are any
            if (theCheckBoxChangedHandlers != null)
            {

                // Multicast to them
                theCheckBoxChangedHandlers(this, e);

            }
        }

        /// <summary>
        /// Fire up the checkbox changed event with cell mouse up event args. 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void FireCheckBoxChanged(object sender, iGCellMouseUpEventArgs e)
        {

            iGCellClickEventArgs e1 = new iGCellClickEventArgs(e.RowIndex, e.ColIndex, e.Bounds, e.ModifierKeys, e.OverControl);
            // Get all of the listeners to the changed event
            CheckBoxChangedHandler theCheckBoxChangedHandlers = (CheckBoxChangedHandler)_theEventHandlers[_theCheckBoxChangedKey];

            // Check to see if there are any
            if (theCheckBoxChangedHandlers != null)
            {

                // Multicast to them
                theCheckBoxChangedHandlers(this, e1);

            }
        }

        /// <summary>
        /// Fire up the grid selection changed event. 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void FireSelectionChanged(object sender, EventArgs e)
        {
            // Get all of the listeners to the changed event
            SelectionChangedHandler theSelectionChangedHandlers = (SelectionChangedHandler)_theEventHandlers[_theSelectionChangedKey];

            // Check to see if there are any
            if (theSelectionChangedHandlers != null)
            {

                // Multicast to them
                theSelectionChangedHandlers(this, e);
            }
        }

        /// <summary>
        /// Fire up a double clicked event. 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="rowIndex"></param>
        private void FireDoubleClicked(object sender, int rowIndex)
        {
            // Get all of the listeners to the double clicked event
            DoubleClickedHandler theDoubleClickedHandlers = (DoubleClickedHandler)_theEventHandlers[_theDoubleClickedKey];

            // Check to see if there are any
            if (theDoubleClickedHandlers != null)
            {
                // Multicast to them
                theDoubleClickedHandlers(this, rowIndex);
            }
        }

        /// <summary>
        /// A report definition changed event handler. 
        /// </summary>
        /// <param name="aSender"></param>
        /// <param name="aReportDefinition"></param>
        /// <param name="aReason"></param>
        private void ReportDefinitionChanged(object aSender, ReportDefinition aReportDefinition, ReportDefinition.Reason aReason)
        {
            int theRowIndex = IndexOfReportDefinition(aReportDefinition, ((SimpleReportDefinition)aReportDefinition).Group);
            if (theRowIndex >= 0)
            {
                switch (aReason)
                {
                    case ReportDefinition.Reason.NameChanged:
                        {

                            // We need to invalidate the cell because we are not changing the actual data in this column.
                            // We want to paint the new name of the same report definition so we are forcing
                            // the report definition's ToString() to be called and that will return the new name.
                            iGrid1.Update();
                            //iGrid1.Rows[theRowIndex].Cells["theReportDefinitionColumn"].
                            //InvalidateCell(theColumnIndex, theRowIndex);
                            break;
                        }
                    case ReportDefinition.Reason.StatementChanged:
                        {
                            iGrid1.Rows[theRowIndex].Cells[COL_KEY_ONELINESUMMARY].Value = aReportDefinition.OneLineSummary;
                            break;
                        }
                    case ReportDefinition.Reason.ExecutionCompleted:
                        {
                            iGrid1.Rows[theRowIndex].Cells[COL_KEY_STARTTIME].Value = aReportDefinition.GetProperty(ReportDefinition.START_TIME);
                            string executionStatus = (string)aReportDefinition.GetProperty(ReportDefinition.LAST_EXECUTION_STATUS);
                            if (!executionStatus.Equals(Properties.Resources.QueryExecutionSuccess))
                            {
                                iGrid1.Rows[theRowIndex].ForeColor = Color.Red;
                            }
                            else
                            {
                                iGrid1.Rows[theRowIndex].ForeColor = this._iGridDefaultForColor;
                            }
                            break;
                        }
                    case ReportDefinition.Reason.ResultsDiscarded:
                        {
                            break;
                        }
                    default:
                        {
                            break;
                        }
                }
            }
        }

        /// <summary>
        /// To add or remove a statement from the persistence file. 
        /// </summary>
        /// <param name="aReportDefinition"></param>
        /// <param name="aQueryStringsOperation"></param>
        private void QueryStringsPersistenceAddedRemoved(ReportDefinition aReportDefinition, QueryStringsPersistence.Operation aQueryStringsOperation)
        {
            switch (aQueryStringsOperation)
            {
                case QueryStringsPersistence.Operation.Added:
                    {
                        AddReportDefinition(aReportDefinition);
                        break;
                    }
                case QueryStringsPersistence.Operation.Removed:
                    {
                        RemoveReportDefinition(aReportDefinition);
                        break;
                    }
            }
        }

        /// <summary>
        /// The IGrid selection change event handler
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void iGrid1_SelectionChanged(object sender, EventArgs e)
        {
            FireSelectionEvent(SelectedRowIndex);
        }

        /// <summary>
        /// Fire selection changed event if the row selection has been changed.
        /// </summary>
        /// <param name="index"></param>
        private void FireSelectionEvent(int index)
        {
           FireSelectionChanged(this, new EventArgs());
        }

        /// <summary>
        /// Make a new row for a new report. 
        /// </summary>
        /// <param name="aReportDefinition"></param>
        /// <returns></returns>
        private iGRow MakeRow(ReportDefinition aReportDefinition)
        {
            // By default, place it in the persistence store. 
            return MakeRow(aReportDefinition, Properties.Resources.PersistenceFile);
        }

        /// <summary>
        /// Make a new row for a new report. 
        /// </summary>
        /// <param name="aReportDefinition"></param>
        /// <param name="aGroupName"></param>
        /// <returns></returns>
        private iGRow MakeRow(ReportDefinition aReportDefinition, string aGroupName)
        {
            iGRow theRow = iGrid1.Rows.Add();
            theRow.Cells[COL_KEY_CHECKBOX].Value = false;
            theRow.Cells[COL_KEY_GROUPNAME].Value = aGroupName;
            theRow.Cells[COL_KEY_GROUPNAME].ImageList = imageList1;
            theRow.Cells[COL_KEY_GROUPNAME].ImageIndex = 0;
            theRow.Cells[COL_KEY_REPORTDEFINITION].Value = (SimpleReportDefinition)aReportDefinition;
            theRow.Cells[COL_KEY_ONELINESUMMARY].Value = aReportDefinition.OneLineSummary;

            // Also remember the group name in the Report itself.
            ((SimpleReportDefinition)aReportDefinition).Group = aGroupName;

            object start_time = aReportDefinition.GetProperty(ReportDefinition.START_TIME);
            if (start_time != null)
            {
                theRow.Cells[COL_KEY_STARTTIME].Value = start_time;
            }

            return theRow;
        }

        /// <summary>
        /// Add the given report to the specified group.
        /// </summary>
        /// <param name="aReportDefinition"></param>
        /// <param name="from"></param>
        private void AddReportDefinition(ReportDefinition aReportDefinition, string from)
        {

            // In case we load manually at init time and peristence fires adds at us
            if (!ContainsReportDefinition(aReportDefinition, from))
            {
                MakeRow(aReportDefinition, from);//.Selected = true;

                // Maintain the group count
                if (_theGroupCount.ContainsKey(from))
                {
                    _theGroupCount[from] = ((int)_theGroupCount[from]) + 1;
                }
                else
                {
                    _theGroupCount.Add(from, (int)1);
                }

                try
                {
                    iGrid1.Group();
                }
                catch (Exception)
                {
                    //Currently, we jsut drop anything failed.
                }
            }
        }

        /// <summary>
        /// Add the given report.
        /// </summary>
        /// <param name="aReportDefinition"></param>
        private void AddReportDefinition(ReportDefinition aReportDefinition)
        {
            AddReportDefinition(aReportDefinition, Properties.Resources.PersistenceFile);
        }

        /// <summary>
        /// Remove the specified report.
        /// </summary>
        /// <param name="aReportDefinition"></param>
        private void RemoveReportDefinition(ReportDefinition aReportDefinition)
        {
            string groupName = ((SimpleReportDefinition)aReportDefinition).Group;
            ((SimpleReportDefinition)aReportDefinition).Group = "";
            int theIndex = IndexOfReportDefinition(aReportDefinition, groupName);
            if (theIndex >= 0)
            {
                iGrid1.Rows.RemoveAt(theIndex);

                // Every report is belong to one group.
                _theGroupCount[groupName] = ((int)_theGroupCount[groupName]) - 1;

                if (((int)_theGroupCount[groupName]) == 0)
                {
                    _theGroupCount.Remove(groupName);
                }

                try
                {
                    iGrid1.Group();
                }
                catch (Exception)
                {
                    //Currently, we jsut drop anything failed.
                }
            }
        }

        /// <summary>
        /// Get report from a selected row.
        /// </summary>
        /// <param name="aRow"></param>
        /// <returns></returns>
        private ReportDefinition GetReportDefinition(iGRow aRow)
        {
            return aRow.Cells[COL_KEY_REPORTDEFINITION].Value as ReportDefinition;
        }

        /// <summary>
        /// Add image in front of the group names
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void iGrid1_AfterAutoGroupRowCreated(object sender, TenTec.Windows.iGridLib.iGAfterAutoGroupRowCreatedEventArgs e)
        {
            iGCell myGroupRowCell;
            switch (iGrid1.Cols[e.GroupedColIndex].Key)
            {
                case "theGroupNameColumn":
                    // Add text to the group rows (as the group name column is grouped by image index
                    // it's group rows will contain only images).
                    myGroupRowCell = iGrid1.RowTextCol.Cells[e.AutoGroupRowIndex];
                    iGCell myFirstCellInGroup = iGrid1.Cells[e.GroupedRowIndex, e.GroupedColIndex];
                    myGroupRowCell.ImageIndex = myFirstCellInGroup.ImageIndex;
                    myGroupRowCell.Style = iGrid1.GroupRowLevelStyles[0].Clone();
                    myGroupRowCell.ImageList = iGrid1.Cols[e.GroupedColIndex].CellStyle.ImageList;
                    break;
            }
        }

        /// <summary>
        /// To handle the event when the group's checkbox is clicked.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void iGrid1_CellMouseUp(object sender, iGCellMouseUpEventArgs e)
        {
            if (!BatchMode)
            {
                if ((e.RowIndex >= 0) && (e.ColIndex >= 0))
                {
                    iGCell theDataGridViewCell = iGrid1.Rows[e.RowIndex].Cells[e.ColIndex];
                    if (e.ColIndex == iGrid1.Cols[COL_KEY_CHECKBOX].Index &&
                        iGrid1.Rows[e.RowIndex].Type == iGRowType.Normal)
                    {
                        theDataGridViewCell.Value = !((bool)theDataGridViewCell.Value);
                        FireCheckBoxChanged(this, e);
                        string groupName = iGrid1.Rows[e.RowIndex].Cells[COL_KEY_GROUPNAME].Value as string;
                        int firstRowID = -1;
                        bool allMatched = VerifyEntireGroupCheckBoxes(groupName, (bool)theDataGridViewCell.Value, out firstRowID);
                        if (allMatched)
                        {
                            // The entire group is now checked or unchecked. So, it is time to change the group checkbox image. 
                            if (firstRowID != -1)
                            {
                                if (String.Format("Loaded From: {0}", groupName).Equals(iGrid1.Rows[firstRowID - 1].Cells[-1].Value as string, StringComparison.CurrentCultureIgnoreCase))
                                {
                                    if ((bool)theDataGridViewCell.Value)
                                    {
                                        iGrid1.Rows[firstRowID - 1].Cells[-1].ImageIndex = 1;
                                    }
                                    else
                                    {
                                        iGrid1.Rows[firstRowID - 1].Cells[-1].ImageIndex = 0;
                                    }
                                }
                            }
                        }
                    }
                }
                else if (e.RowIndex >= 0 && e.ColIndex == -1 && e.MousePos.X > 20 && e.MousePos.X < 30)
                {
                    if (iGrid1.Rows[e.RowIndex].Type == iGRowType.AutoGroupRow ||
                        iGrid1.Rows[e.RowIndex].Type == iGRowType.ManualGroupRow)
                    {
                        if (iGrid1.Rows[e.RowIndex].Cells[e.ColIndex].ImageIndex == 0)
                        {

                            iGrid1.Rows[e.RowIndex].Cells[e.ColIndex].ImageIndex = 1;
                        }
                        else
                        {
                            iGrid1.Rows[e.RowIndex].Cells[e.ColIndex].ImageIndex = 0;
                        }

                        string groupName = iGrid1.Rows[e.RowIndex].Cells[e.ColIndex].Text.Substring(13);
                        SetEntireGroupCheckBoxes(groupName, (iGrid1.Rows[e.RowIndex].Cells[e.ColIndex].ImageIndex > 0));
                    }
                }
            }
        }

        /// <summary>
        /// To format the group name with number of rows in the group.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void CustomDrawCellForGroundMethod(object sender, iGCustomDrawCellEventArgs e)
        {
            string myGroupText = (string)iGrid1.Cells[e.RowIndex, e.ColIndex].Value;
            object myTotalValue = iGrid1.Cells[e.RowIndex, COL_KEY_TOTAL].Value;
            if (myTotalValue != null)
            {
                myGroupText = String.Format("{0}  ({1})", myGroupText, myTotalValue);
            }

            Rectangle rec = new Rectangle(e.Bounds.X, e.Bounds.Y, e.Bounds.Height - 1, e.Bounds.Height - 1);
            e.Graphics.DrawImageUnscaledAndClipped(iGrid1.Cells[e.RowIndex, e.ColIndex].ImageList.Images[iGrid1.Cells[e.RowIndex, e.ColIndex].ImageIndex], rec);
            rec.X = e.Bounds.X + e.Bounds.Height + 4;
            rec.Width = e.Bounds.Width - e.Bounds.Height;
            Font font = new Font(iGrid1.Font, FontStyle.Bold);
            SolidBrush brush = new SolidBrush(iGrid1.ForeColor);
            e.Graphics.DrawString(myGroupText, font, brush, rec);
        }

        #endregion Private methods
    }
}
