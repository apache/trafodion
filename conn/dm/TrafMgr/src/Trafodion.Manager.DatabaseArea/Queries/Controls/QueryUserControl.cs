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

#define EXPLAIN
using System;
using System.Collections;
using System.Collections.Generic;
using System.Data;
using System.Data.Odbc;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Controls.Tree;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.DatabaseArea.NCC;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Connections.Controls;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    /// <summary>
    /// The encompassing control where a user can type in a query and execute it.  
    /// </summary>
    public partial class QueryUserControl : UserControl, IConnectionDefinitionSelector
    {
        #region Fields

        private ConnectionDefinition.ChangedHandler _theConnectionDefinitionChangedHandler = null;

        private DatabaseTreeView _theDatabaseTreeView = null;

        private QueryListUserControl _theQueryListUserControl = null;
        private QueryDetailsUserControl _theQueryDetailsUserControl = null;
        private int _suppressDataGridViewSelectionChanged = -1;
        private bool _suppressQueryTextBoxTextChanged = false;
        private bool _suppressNameTextBoxTextChanged = false;
        private bool _suppressCQSettingChanged = false;

        ReportDefinition _theSelectedReportDefinition = null;
        private ConnectionDefinition _theSelectedConnectionDefinition = null;

        /// <summary>
        /// Keep tracks of all control reset statements associated with a session.  Since this survives 
        /// along with the session, we will have to keep this in sync with the active connections and cleaned up 
        /// when the session is cleaned.
        /// </summary>
        private Dictionary<string, ArrayList> _activeControlResetStatements = new Dictionary<string, ArrayList>();

        private Dictionary<string, CacheConnectionObject> _activeCacheConnectionObjects = new Dictionary<string, CacheConnectionObject>();

        private TrafodionCatalog _theSelectedTrafodionCatalog = null;
        private TrafodionSchema _theSelectedTrafodionSchema = null;
        private ThreadWithState _queryThread = null;

        //private DataGridViewCellMouseEventHandler _theQueryListUserControlMouseEventHandler = null;
        //private EventHandler _theQueryListDataGridViewSelectionChangedHandler = null;

        const string _theCreateCatalogRegexp = @"^(\s)*(create)(\s)+(catalog)(\s)+";
        const string _theCreateSchemaRegexp = @"^(\s)*(create)(\s)+(schema)(\s)+";
        const string _theDropCatalogRegexp = @"^(\s)*(drop)(\s)+(catalog)(\s)+";
        const string _theDropSchemaRegexp = @"^(\s)*(drop)(\s)+(schema)(\s)+";

        //This would return a data set
        const string _selectRegexp = "^(\\s)*(select)(\\s)+";

        //This would return a plan
        const string _explainRegexp = "^(\\s)*(explain)(\\s)+";

        //These would return an int indicating the number of rows affected
        const string _insertRegexp = "^(\\s)*(insert)(\\s)+";
        const string _deleteRegexp = "^(\\s)*(delete)(\\s)+";
        const string _updateRegexp = "^(\\s)*(update)(\\s)+";

        //These are the special mode swich commands
        const string _theWMSOpenRegexp = "^(\\s)*(wmsopen)(\\s)*";
        const string _theWMSCloseRegexp = "^(\\s)*(wmsclose)(\\s)*";
        const string _theCMDOpenRegexp = "^(\\s)*(cmdopen)(\\s)*";
        const string _theCMDCloseRegexp = "^(\\s)*(cmdclose)(\\s)*";

        //Create the regular expression to evaluate
        public static Regex _selectReg = new Regex(_selectRegexp);
        public static Regex _explainReg = new Regex(_explainRegexp);
        public static Regex _insertReg = new Regex(_insertRegexp);
        public static Regex _deleteReg = new Regex(_deleteRegexp);
        public static Regex _updateReg = new Regex(_updateRegexp);

        static Regex _theCreateCatalogReg = new Regex(_theCreateCatalogRegexp, RegexOptions.Compiled | RegexOptions.IgnoreCase);
        static Regex _theCreateSchemaReg = new Regex(_theCreateSchemaRegexp, RegexOptions.Compiled | RegexOptions.IgnoreCase);
        static Regex _theDropCatalogReg = new Regex(_theDropCatalogRegexp, RegexOptions.Compiled | RegexOptions.IgnoreCase);
        static Regex _theDropSchemaReg = new Regex(_theDropSchemaRegexp, RegexOptions.Compiled | RegexOptions.IgnoreCase);

        public static Regex _theWMSOpenReg = new Regex(_theWMSOpenRegexp, RegexOptions.Compiled | RegexOptions.IgnoreCase);
        public static Regex _theWMSCloseReg = new Regex(_theWMSCloseRegexp, RegexOptions.Compiled | RegexOptions.IgnoreCase);
        public static Regex _theCMDOpenReg = new Regex(_theCMDOpenRegexp, RegexOptions.Compiled | RegexOptions.IgnoreCase);
        public static Regex _theCMDCloseReg = new Regex(_theCMDCloseRegexp, RegexOptions.Compiled | RegexOptions.IgnoreCase);
        ReportDefinition.ChangedHandler _theReportDefinitionChangedHandler;

        private SqlCommandObject _executingSqlCommandObject;

        /// <summary>
        /// Session modes
        /// </summary>
        public enum SessionMode { WMSMode, CMDMode, SQLMode };

        private int DEFAULT_ROWS_PER_PAGE = 0;
        private int MAX_ROWS_PER_PAGE = 100000;
        private double initialUsedVirtualMemory = 0;

        public const String TRACE_SUB_AREA_NAME = "Query Execution";

        public delegate void AbortCurrentQueryMethod(bool closeConnectionDuringAbort);

        public delegate bool AllowToStartQueryMethod();

        private bool _batchMode = false;

        public delegate void MySystemsChanged(object sender, EventArgs e);
        public event MySystemsChanged OnMySystemChanged;

        #endregion Fields

        /// <summary>
        /// Constructor for the Query User Control
        /// </summary>
        public QueryUserControl()
        {
            InitializeComponent();

            _theReportDefinitionChangedHandler = new ReportDefinition.ChangedHandler(ReportDefinitionChanged);
            ReportDefinition.Changed += _theReportDefinitionChangedHandler;

            TheQueryTextBox.TextChanged += new EventHandler(TheQueryTextBoxTextChanged);

            if (_theMySystemsComboBox.Items.Count > 0)
            {
                if (TrafodionContext.Instance.CurrentConnectionDefinition != null)
                {
                    if (TrafodionContext.Instance.CurrentConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
                    {
                        _theMySystemsComboBox.SelectedConnectionDefinition = TrafodionContext.Instance.CurrentConnectionDefinition;
                        SelectConnectionDefinition(_theMySystemsComboBox.SelectedConnectionDefinition);
                    }
                    else
                    {
                        _theMySystemsComboBox.SelectedIndex = -1;
                    }
                }
                else
                {
                    _theMySystemsComboBox.SelectedIndex = -1;
                }
            }

            _theConnectionDefinitionChangedHandler = new ConnectionDefinition.ChangedHandler(ConnectionDefinitionChanged);
            ConnectionDefinition.Changed += _theConnectionDefinitionChangedHandler;
            TheCQSettingControl.OnCQSettingChanged += new CQSettingControl.CQSettingChanged(TheCQSettingControl_OnCQSettingChanged);
            SQLWhiteboardOptions.GetOptions().SQLWhiteboardOptionsChanged += HandleSQLWhiteboardOptionsChanged;

            SetDefaultMaxRows();

            UpdateControls();

        }

        /// <summary>
        /// Clean up resources before this control is disposed
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {

            if (disposing)
            {
                //Abort any running queries
                //if (_queryThread != null)
                //{
                //    if (_queryThread.ExecutingThread != null && _queryThread.ExecutingThread.IsAlive)
                //    {
                //        Thread abortThread = new Thread(new ThreadStart(_queryThread.Abort));
                //        //abortThread.IsBackground = true;
                //        //Its a kludge to prevent the UI thread from blocking
                //        abortThread.Start();

                //        //dont wait till the thread completes else it's going to wait for awhile in a busy system
                //        //abortThread.Join();
                //    }
                //}
                
                AbortCurrentQuery(false);

                if (_theConnectionDefinitionChangedHandler != null)
                {
                    ConnectionDefinition.Changed -= _theConnectionDefinitionChangedHandler;
                    RemoveDataGridEventHandlers();
                }

                if (_theReportDefinitionChangedHandler != null)
                {
                    ReportDefinition.Changed -= _theReportDefinitionChangedHandler;
                }

                // Clean up all caches
                foreach (CacheConnectionObject cacheConnection in _activeCacheConnectionObjects.Values)
                {
                    if (cacheConnection.Connection != null)
                    {
                        cacheConnection.Connection.Close();
                    }

                    if (cacheConnection.ControlResetStatements != null)
                    {
                        cacheConnection.ControlResetStatements.Clear();
                    }
                }

                _activeCacheConnectionObjects.Clear();
                SQLWhiteboardOptions.GetOptions().SQLWhiteboardOptionsChanged -= HandleSQLWhiteboardOptionsChanged;
            }

        }

        #region Properties

        public ReportDefinition TheSelectedReportDefinition
        {
            get { return _theSelectedReportDefinition; }
            set
            {
                _theSelectedReportDefinition = value;
                TheQueryInputControl.TheReportDefinition = _theSelectedReportDefinition;
            }
        }

        public DatabaseTreeView TheDatabaseTreeView
        {
            get { return _theDatabaseTreeView; }
            set
            {
                _theDatabaseTreeView = value;
                UpdateControls();
            }
        }
        public QueryInputControl TheQueryInputControl
        {
            get { return _theQueryInputControl; }
        }

        public SqlStatementTextBox TheQueryTextBox
        {
            get { return TheQueryInputControl.TheQueryTextBox; }
        }

        public QueryListDataGridView TheQueryListDataGridView
        {
            get
            {
                return _theQueryListUserControl.TheQueryListDataGridView;
            }
        }

        public QueryListUserControl TheQueryListUserControl
        {
            get { return _theQueryListUserControl; }
            set
            {

                if (_theQueryListUserControl != null)
                {
                    RemoveDataGridEventHandlers();
                }

                _theQueryListUserControl = value;

                if (value != null)
                {
                    TheQueryListDataGridView.OnDoubleClicked += TheGrid_DoubleClick;
                    TheQueryListDataGridView.SelectionChanged += TheGrid_SelectionChanged;
                    _theQueryListUserControl.AbortCurrentQueryMethod = new AbortCurrentQueryMethod(this.AbortCurrentQuery);
                    _theQueryListUserControl.AllowToStartQueryMethod = new AllowToStartQueryMethod(this.AllowToStartQuery);
                    _theQueryListUserControl.BeginBatchOperation += new QueryListUserControl.BatchOperationChanged(_theQueryListUserControl_BeginBatchOperation);
                    _theQueryListUserControl.EndBatchOperation += new QueryListUserControl.BatchOperationChanged(_theQueryListUserControl_EndBatchOperation);
                    //this.TheInputName = String.Format("{0}{1}", ReportDefinition.DEFAULT_STATMENT_NAME_PREFIX, (TheQueryListDataGridView.Rows.Count).ToString("D3"));
                    this.TheInputName = TheQueryListDataGridView.GenerateUniqueQueryName();
                }
            }
        }

        /// <summary>
        /// The Control Settings Control
        /// </summary>
        public CQSettingControl TheCQSettingControl
        {
            get { return _theCQSettingControl; }
            set { _theCQSettingControl = value; }
        }


        /// <summary>
        /// The current connection definition
        /// </summary>
        public ConnectionDefinition CurrentConnectionDefinition
        {
            get
            {
                return TheSelectedConnectionDefinition;
            }
        }

        public QueryDetailsUserControl TheQueryDetailsUserControl
        {
            get { return _theQueryDetailsUserControl; }
            set { _theQueryDetailsUserControl = value; }
        }

        public ConnectionDefinition TheSelectedConnectionDefinition
        {
            get { return _theMySystemsComboBox.SelectedConnectionDefinition; }
            set
            {
                _theMySystemsComboBox.SelectedConnectionDefinition = value;
                SelectConnectionDefinition(value);
            }
        }

        public TrafodionCatalog TheSelectedTrafodionCatalog
        {
            get { return _theCatalogsComboBox.SelectedTrafodionCatalog; }
            set
            {
                _theCatalogsComboBox.SelectedTrafodionCatalog = value;
            }
        }
        /// <summary>
        /// The current selected Schema object
        /// </summary>
        public TrafodionSchema TheSelectedTrafodionSchema
        {
            get { return _theSchemasComboBox.SelectedTrafodionSchema; }
            set { _theSchemasComboBox.SelectedTrafodionSchema = value; }
        }

        private string TheInputName
        {
            get { return _theNameTextBox.Text; }
            set
            {
                // Statement name can not be empty; if so, generate a new name.
                if (!string.IsNullOrEmpty(value))
                {
                _theNameTextBox.Text = value;
                }
                else
                {
                    _theNameTextBox.Text = TheQueryListDataGridView.GenerateUniqueQueryName();
                }

                UpdateControls();
            }
        }

        private string TheSelectedName
        {
            get
            {
                string theSelectedName = "";
                if (TheSelectedReportDefinition != null)
                {
                    theSelectedName = TheSelectedReportDefinition.Name;
                }
                return theSelectedName;
            }
            set
            {
                if (TheSelectedReportDefinition != null)
                {
                    UpdateControls();
                }
            }
        }

        // Sets and gets the executed statement
        private string TheSelectedStatement
        {
            get
            {
                string theSelectedStatement = "";
                if (TheSelectedReportDefinition != null)
                {
                    theSelectedStatement = TheSelectedReportDefinition.GetProperty(ReportDefinition.DEFINITION) as string;
                    if (theSelectedStatement == null)
                    {
                        theSelectedStatement = "";
                    }
                }
                return theSelectedStatement;
            }
            set
            {
                if (TheSelectedReportDefinition != null)
                {
                    TheSelectedReportDefinition.SetProperty(ReportDefinition.DEFINITION, value);
                    UpdateControls();
                }
            }
        }

        private string TheSelectedFullStatement
        {
            get
            {
                if (TheSelectedReportDefinition != null)
                {
                    string theFullStatement = TheSelectedReportDefinition.GetProperty(ReportDefinition.FULL_RAW_TEXT) as string;
                    if (theFullStatement == null)
                    {
                        string theSelectedStatement = TheSelectedReportDefinition.GetProperty(ReportDefinition.DEFINITION) as string;
                        if (theSelectedStatement == null)
                        {
                            return "";
                        }
                        else
                        {
                            return theSelectedStatement;
                        }
                    }
                    else
                    {
                        return theFullStatement;
                    }
                }
                return "";
            }
            set
            {
                if (TheSelectedReportDefinition != null)
                {
                    TheSelectedReportDefinition.SetProperty(ReportDefinition.FULL_RAW_TEXT, value);

                    UpdateControls();
                }
            }
        }

        /// <summary>
        /// Property: TheSelectedControlStatements - Get the control statements from the selected report or Set the report with
        ///           the given control statements.
        /// </summary>
        private List<ReportControlStatement> TheSelectedControlStatements
        {
            get
            {
                if (TheSelectedReportDefinition != null)
                {
                    List<ReportControlStatement> controlStatements = TheSelectedReportDefinition.GetProperty(ReportDefinition.CONTROL_STATEMENTS) as List<ReportControlStatement>;
                    return controlStatements;
                }
                return null;
            }
            set
            {
                if (TheSelectedReportDefinition != null)
                {
                    TheSelectedReportDefinition.SetProperty(ReportDefinition.CONTROL_STATEMENTS, value);
                    UpdateControls();
                }
            }
        }

        bool IsSystemSelected
        {
            get { return (_theMySystemsComboBox.SelectedIndex >= 0); }
        }

        bool IsStatementTextPresent
        {
            //get { return (TheInputStatement.Length > 0); }
            get { return (!String.IsNullOrEmpty(TheInputStatement)); }
        }

        bool IsStatementNamePresent
        {
            get { return (!String.IsNullOrEmpty(TheInputName)); }
        }

        bool IsStatementNameChanged
        {
            get { return !(TheSelectedName.Equals(TheInputName)); }
        }

        bool IsStatementTextChanged
        {
            get { return !(TheSelectedFullStatement.Equals(TheInputStatement)); }
        }

        /// <summary>
        /// Property: IsControlSettingChanged - compare the Selected and Input control settings to see
        ///           if there are the same.
        /// </summary>
        bool IsControlSettingChanged
        {
            get 
            {
                List<ReportControlStatement> controlStatements = TheSelectedControlStatements;
                List<ReportControlStatement> currentControlStatements = TheInputControlStatements;
                if (controlStatements == null && currentControlStatements == null)
                {
                    return false;
                }
                else if (controlStatements == null || currentControlStatements == null)
                {
                    return true;
                }
                else
                {
                    if (controlStatements.Count != currentControlStatements.Count)
                    {
                        return true;
                    }
                    else
                    {
                        controlStatements.Sort();
                        currentControlStatements.Sort();
                        int idx = 0;
                        foreach (ReportControlStatement rcs in controlStatements)
                        {
                            if (!rcs.Equals(currentControlStatements[idx]))
                            {
                                return true;
                            }
                            idx++;
                        }
                    }
                    return false;
                }
            }
        }

        bool IsStatementNameOrTextOrControlSettingChanged
        {
            get { return IsStatementTextChanged || IsStatementNameChanged || IsControlSettingChanged; }
        }

        bool IsStatementNameAndTextPresent
        {
            get { return IsStatementNamePresent && IsStatementTextPresent; }
        }

        bool CanAddOrUpdate
        {
            get { return (IsStatementNameOrTextOrControlSettingChanged && IsStatementNameAndTextPresent); }
        }

        /// <summary>
        /// Returns the user can or can not close the current selected System connection
        /// </summary>
        bool CanCloseConnection
        {
            get
            {
                return false;
            }
        }
        bool CanExecuteOrExplain
        {
            get
            {
                return (IsActiveSystemSelected
                    && IsStatementTextPresent
                    && MaxRowsIsValid);
            }
        }
        bool IsActiveSystemSelected
        {
            get
            {
                Connection con = null;
                //if ((_theMySystemsComboBox.SelectedConnectionDefinition != null) && (_activeConnections.ContainsKey(_theMySystemsComboBox.SelectedConnectionDefinition.Name)))
                //{
                //    con = _activeConnections[_theMySystemsComboBox.SelectedConnectionDefinition.Name];
                //}
                if ((_theMySystemsComboBox.SelectedConnectionDefinition != null) && (_activeCacheConnectionObjects.ContainsKey(_theMySystemsComboBox.SelectedConnectionDefinition.Name)))
                {
                    CacheConnectionObject cacheConnection = _activeCacheConnectionObjects[_theMySystemsComboBox.SelectedConnectionDefinition.Name];
                    con = cacheConnection.Connection;
                }
                return IsSystemSelected &&
                    (//Either there is a tested connection
                    (_theMySystemsComboBox.SelectedConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
                    //or there is an established connection for the selected connection definition
                    || ((con != null) ? con.IsConnectionOpen : false));
            }
        }
        private string TheInputStatement
        {
            get { return TheQueryTextBox.Text; }
            set
            {
                TheQueryTextBox.Text = value;
                UpdateControls();
            }
        }

        private List<ReportControlStatement> TheInputControlStatements
        {
            get { return _theCQSettingControl.CurrentControlStatements; }
            set
            {
                _theCQSettingControl.SetCQGridValues(value);
                UpdateControls();
            }
        }

        private string TheInputHighlightedStatement
        {
            get
            {
                return TheQueryTextBox.SelectedText;
            }
        }

        private bool MaxRowsIsValid
        {
            get
            {
                try
                {
                    Int64 theMaxRows = Int64.Parse(_theMaxRowsTextBox.Text);
                    if (theMaxRows > 0 && theMaxRows <= Int32.MaxValue)
                    {
                        return true;
                    }
                }
                catch (Exception ipe)
                {
                }
                return false;
            }
        }

        private int MaxRows
        {
            get
            {
                try
                {
                    int theMaxRows = Int32.Parse(_theMaxRowsTextBox.Text);
                    if (theMaxRows > 0)
                    {
                        return theMaxRows;
                    }
                }
                catch (Exception ipe)
                {
                }
                _theMaxRowsTextBox.Text = QueryInputControl.MAX_ROWS_DEFAULT.ToString();
                return QueryInputControl.MAX_ROWS_DEFAULT;
            }
        }

        private int RowsPerPage
        {
            get
            {
                try
                {
                    int rowsPerPage = Int32.Parse(this._rowsPerPageTextBox.Text);
                    if (rowsPerPage >= 0)
                    {
                        return rowsPerPage;
                    }
                }
                catch (Exception ipe)
                {
                }
                _rowsPerPageTextBox.Text = DEFAULT_ROWS_PER_PAGE.ToString();
                return DEFAULT_ROWS_PER_PAGE;
            }
        }
        #endregion Properties

        /// <summary>
        /// Fire the state change event
        /// </summary>
        /// <param name="e"></param>
        private void FireOnMySystemChanged(EventArgs e)
        {
            if (OnMySystemChanged != null)
            {
                OnMySystemChanged(this, e);
            }
        }

        /// <summary>
        /// Event handler for Query List control ends batch operation
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="args"></param>
        private void _theQueryListUserControl_EndBatchOperation(object sender, EventArgs args)
        {
            ExitBatchMode();
        }

        /// <summary>
        /// Event handler for Query List control starts batch operation
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="args"></param>
        private void _theQueryListUserControl_BeginBatchOperation(object sender, EventArgs args)
        {
            EnterBatchMode();
        }

        /// <summary>
        /// To enter batch mode and disable most of the user controls
        /// </summary>
        private void EnterBatchMode()
        {
            _batchMode = true;
            _theAddButton.Enabled = false;
            _theUpdateButton.Enabled = false;
            _theExecuteButton.Enabled = false;
            _theExplainButton.Enabled = false;
            _theCatalogsComboBox.Enabled = false;
            _theSchemasComboBox.Enabled = false;
            _theMySystemsComboBox.Enabled = false;
            _theCQSettingControl.Enabled = false;
            _theMaxRowsTextBox.Enabled = false;
            _theNameTextBox.Enabled = false;
            _rowsPerPageTextBox.Enabled = false;
            _theQueryInputControl.Enabled = false;
        }

        /// <summary>
        /// Exiting the batch mode and enable all user controls
        /// </summary>
        private void ExitBatchMode()
        {
            _theAddButton.Enabled = true;
            _theUpdateButton.Enabled = true;
            _theExecuteButton.Enabled = true;
            _theExplainButton.Enabled = true;
            _theCatalogsComboBox.Enabled = true;
            _theSchemasComboBox.Enabled = true;
            _theMySystemsComboBox.Enabled = true;
            _theCQSettingControl.Enabled = true;
            _theMaxRowsTextBox.Enabled = true;
            _theNameTextBox.Enabled = true;
            _rowsPerPageTextBox.Enabled = true;
            _theQueryInputControl.Enabled = true;
            _batchMode = false;
        }

        /// <summary>
        /// Update all user controls according to the current state.
        /// </summary>
        private void UpdateControls()
        {
            if (_batchMode)
            {
                return;
            }

            _theAddButton.Enabled = CanAddOrUpdate;
            _theUpdateButton.Enabled = CanAddOrUpdate;

            _theExecuteButton.Enabled = CanExecuteOrExplain && !(_theExplainButton.Text == Properties.Resources.Cancel);
            _theExplainButton.Enabled = CanExecuteOrExplain && !(_theExecuteButton.Text == Properties.Resources.Cancel);
            if (_theQueryListUserControl != null)
            {
                _theQueryListUserControl.EnableBatchButtons = IsActiveSystemSelected;
            }

            if (IsActiveSystemSelected)
            {
                headerPanel.Controls.Clear();
                headerPanel.Hide();
            }
            else
            {
                TrafodionLabel label = new TrafodionLabel();
                label.ForeColor = System.Drawing.Color.Red;
                label.Text = Properties.Resources.SelectSystemPrompt;
                label.Dock = DockStyle.Fill;
                headerPanel.Controls.Add(label);
                headerPanel.Show();
            }

            _theSystemsAndSchemasPanel.Visible = true;
            if (TrafodionContext.Instance.isCommunityEdition)
            {
                _theCatalogsPanel.Visible = _theSchemasPanel.Visible = false;
            }
            else
            {
                _theCatalogsPanel.Visible = _theSchemasPanel.Visible = true;
            }

            //Enforce Controlled Schema access restriction, if enabled.
            //If controlled schema access is on, users are restricted to their default catalog and schema
            //They should not be able to select a different catalog/schema from the combobox
            if (IsActiveSystemSelected)
            {
                _theCatalogsComboBox.Enabled = !_theMySystemsComboBox.SelectedConnectionDefinition.IsControlledSchemaAccessEnabled;
                _theSchemasComboBox.Enabled = !_theMySystemsComboBox.SelectedConnectionDefinition.IsControlledSchemaAccessEnabled;
            }
        }

        private void TheAddButtonClick(object sender, EventArgs e)
        {
            DoAdd();
        }

        private void DoAdd()
        {
            // Do not allow blank statement name or statement contains only white spaces.
            if (String.IsNullOrEmpty(TheInputName) ||
                String.IsNullOrEmpty(TheInputStatement))
            {
                MessageBox.Show(Utilities.GetForegroundControl(),
                    Properties.Resources.ErrorBlankStatementNameOrText,
                    Properties.Resources.SQLWhiteboardErrorMessageTitle,
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
                return;
            }
            // Do not allow duplicate statement names in the persistence store. 
            if (QueryStringsPersistence.ReportDefinitionExists(TheInputName))
            {
                MessageBox.Show(Utilities.GetForegroundControl(), 
                    Properties.Resources.ErrorDuplicateStatementName,
                    Properties.Resources.SQLWhiteboardErrorMessageTitle, 
                    MessageBoxButtons.OK, 
                    MessageBoxIcon.Error);
                return;
            }

            SimpleReportDefinition theNewReportDefinition = new SimpleReportDefinition(TheInputName);
            theNewReportDefinition.SetProperty(ReportDefinition.DEFINITION, TheInputStatement);
            theNewReportDefinition.SetProperty(ReportDefinition.FULL_RAW_TEXT, TheInputStatement);

            // Set the current control settings
            List<ReportControlStatement> controlStatements = _theCQSettingControl.CurrentControlStatements;
            if (controlStatements != null && controlStatements.Count > 0)
            {
                theNewReportDefinition.SetProperty(ReportDefinition.CONTROL_STATEMENTS, controlStatements);
            }

            // Now we need to select the new one without the selection changed handler thinking
            // that the old one changed
            TheInputName = TheSelectedName;
            TheInputStatement = TheSelectedFullStatement;
            TheInputControlStatements = TheSelectedControlStatements;

            QueryStringsPersistence.Add(theNewReportDefinition); // TODO - Name exist?
            _theQueryListUserControl.TheQueryListDataGridView.SelectedReportDefinition = theNewReportDefinition;
        }

        private void DoUpdate()
        {
            // Do not allow blank statement name or statement contains only white spaces.
            if (String.IsNullOrEmpty(TheInputName) ||
                String.IsNullOrEmpty(TheInputStatement))
            {
                MessageBox.Show(Utilities.GetForegroundControl(),
                    Properties.Resources.ErrorBlankStatementNameOrText,
                    Properties.Resources.SQLWhiteboardErrorMessageTitle,
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
                return;
            }
            // Validate duplicate statement name if name has been changed. 
            if (TheSelectedReportDefinition != null)
            {
                if (!TheSelectedReportDefinition.Name.Equals(TheInputName, StringComparison.OrdinalIgnoreCase))
                {
                    // The Input name has been changed. 
                    string groupName = ((SimpleReportDefinition)TheSelectedReportDefinition).Group;
                    if (TheQueryListDataGridView.ContainsReportDefinition(TheInputName, groupName))
                    {
                        MessageBox.Show(Utilities.GetForegroundControl(),
                            Properties.Resources.ErrorDuplicateStatementName,
                            Properties.Resources.SQLWhiteboardErrorMessageTitle,
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Error);
                        return;
                    }
                }
            }

            TheSelectedName = TheInputName;
            TheSelectedFullStatement = TheInputStatement;
            if (!TheSelectedStatement.Equals(TheInputStatement))
            {
                if (TheSelectedReportDefinition != null)
                {
                    DiscardOtherResults(TheSelectedReportDefinition);
                    TheSelectedReportDefinition.SetProperty(ReportDefinition.ACTUAL_QUERY, null);
                    TheQueryDetailsUserControl.TheReportDefinition = TheSelectedReportDefinition;
                }
            }
            TheSelectedStatement = TheInputStatement;
            TheSelectedControlStatements = TheInputControlStatements;
            UpdateControls();
        }

        private void TheExecuteButtonClick(object sender, EventArgs e)
        {
            // Handle the case when the button becomes a Cancel Button
            if (_theExecuteButton.Text.Equals(Properties.Resources.Cancel))
            {
                AbortCurrentQuery(true);
            }
            else
            {
                DoExecute();
            }
        }

        /// <summary>
        /// Clean up the UI when cancel is called and then launch a thread to abort the thread that is running the query.
        /// 
        /// We are launching a separate thread to abort the query running thread because there were situations where the 
        /// ODBC driver was getting blocked in unmanaged code and the thread was getting blocked and not responding to abort.
        /// 
        /// Calling the abort from the UI thread was therefore blocking the UI as well.
        /// </summary>
        private void AbortCurrentQuery(bool closeConnectionDuringAbort)
        {
            if (_queryThread != null)
            {
                if ((_queryThread.SqlCommandObject != null) && (_queryThread.SqlCommandObject.TheReportDefinition != null))
                {
                    ExecuteCancelled(_queryThread.SqlCommandObject.TheReportDefinition);
                    _queryThread.SqlCommandObject.TheReportDefinition.SetProperty(ReportDefinition.FETCH_NEXT_PAGE, true);
                }

                _queryThread.CloseConnectionDuringAbort = closeConnectionDuringAbort;

                //Spawn a new thread to call the cancel. That way the UI thread will never be blocked
                //even if the query execution thread is blocked inside un-managed code.
                Thread abortThread = new Thread(new ThreadStart(_queryThread.Abort));
                abortThread.IsBackground = true;
                abortThread.Start();
            }
        }

        /// <summary>
        /// To validate the state of the query thread to see if it is allowable to start a new query.  
        /// </summary>
        private bool AllowToStartQuery()
        {
            //Make sure the previous query completed or stopped completely.
            if (_queryThread != null)
            {
                if (_queryThread.ExecutingThread.ThreadState != ThreadState.Stopped && _queryThread.ExecutingThread.ThreadState != ThreadState.Aborted)
                {
                    return false;
                }
            }

            return true;
        }

        /// <summary>
        /// This method is called when the execute button is clicked and also when a report definition is double-clicked
        /// </summary>
        private void DoExecute()
        {
            DoExecute(ReportDefinition.Operation.Execute);
        }

        /// <summary>
        /// Do execute or explain
        /// </summary>
        /// <param name="op"></param>
        private void DoExecute(ReportDefinition.Operation op)
        {
            //Make sure the previous query completed or stopped completely.
            if (_queryThread != null)
            {
                if (_queryThread.ExecutingThread.ThreadState != ThreadState.Stopped && _queryThread.ExecutingThread.ThreadState != ThreadState.Aborted)
                {
                    MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.QueryAlreadyExecutingMessage, Properties.Resources.Warning, MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    return;
                }
            }
            
            // Store these values since the selected report definition may be changed
            string theFullStatement = _theQueryInputControl.TheStatement;
            string theHighlightedStatement = TheInputHighlightedStatement;
            if (theHighlightedStatement.Length < 1)
            {
                theHighlightedStatement = theFullStatement;
            }

            // If no selected report definition, make one using the highlighted text
            if (TheSelectedReportDefinition == null)
            {
                if (QueryStringsPersistence.ReportDefinitionExists(TheInputName))
                {
                    MessageBox.Show(Utilities.GetForegroundControl(),
                        Properties.Resources.ErrorDuplicateStatementName,
                        Properties.Resources.SQLWhiteboardErrorMessageTitle,
                        MessageBoxButtons.OK,
                        MessageBoxIcon.Error);
                    return;
                }

                TheSelectedReportDefinition = new SimpleReportDefinition(TheInputName);
                TheSelectedStatement = theHighlightedStatement;
                TheSelectedFullStatement = theFullStatement;
                TheSelectedControlStatements = TheCQSettingControl.CurrentControlStatements;
            }

            if (!CanExecuteOrExplain)
            {
                return;
            }

            // If the selected report definition has been changed
            if ((TheSelectedReportDefinition is SimpleReportDefinition) && IsStatementNameOrTextOrControlSettingChanged)
            {
                ReportDefinition theNewReportDefinition = null;
                if (IsStatementNameChanged)
                {
                    if (QueryStringsPersistence.ReportDefinitionExists(TheInputName))
                    {
                        MessageBox.Show(Utilities.GetForegroundControl(),
                            Properties.Resources.ErrorDuplicateStatementName,
                            Properties.Resources.SQLWhiteboardErrorMessageTitle,
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Error);
                        return;
                    }

                // Make a new report definition using the highlighted statement
                    theNewReportDefinition = new SimpleReportDefinition(TheInputName);
                }
                else if (IsStatementTextChanged)
                {
                    //Generate a new unique name using the reports original name
                    string reportName = ((SimpleReportDefinition)TheSelectedReportDefinition).GetNewName();
                    theNewReportDefinition = new SimpleReportDefinition(reportName);

                    //Propogate the original report's orignal name to the new report definition
                    string originalReportName = TheSelectedReportDefinition.GetProperty(SimpleReportDefinition.ORIGINAL_NAME) as string;
                    if (string.IsNullOrEmpty(originalReportName))
                    {
                        originalReportName = TheSelectedReportDefinition.Name;
                    }
                    theNewReportDefinition.SetProperty(SimpleReportDefinition.ORIGINAL_NAME, originalReportName);
                }
                else
                {
                    // Then, it must be the control setting.  If control setting is the only thing got changed, 
                    // simply update the control settings and not create a new report. 
                    TheSelectedReportDefinition.SetProperty(ReportDefinition.CONTROL_STATEMENTS, TheCQSettingControl.CurrentControlStatements);
                }

                // If a new report is created ...
                if (theNewReportDefinition != null)
                {
                    theNewReportDefinition.SetProperty(ReportDefinition.DEFINITION, theHighlightedStatement);
                    theNewReportDefinition.SetProperty(ReportDefinition.FULL_RAW_TEXT, theFullStatement);
                    theNewReportDefinition.SetProperty(ReportDefinition.CONTROL_STATEMENTS, TheCQSettingControl.CurrentControlStatements);                  

                    QueryStringsPersistence.Add(theNewReportDefinition);

                    // Now we need to select the new one without the selection chnaged handler thinking
                    // that the old one changed
                    TheInputName = TheSelectedName;
                    TheInputStatement = TheSelectedFullStatement;
                    TheInputControlStatements = TheSelectedControlStatements;

                    _theQueryListUserControl.TheQueryListDataGridView.SelectedReportDefinition = theNewReportDefinition;

                    TheSelectedReportDefinition = theNewReportDefinition; 
                    TheSelectedName = TheInputName;
                    TheSelectedStatement = theHighlightedStatement;
                    TheSelectedFullStatement = theFullStatement;
                    TheSelectedControlStatements = TheCQSettingControl.CurrentControlStatements;

                }
            }
            else
            {
                // Handle the case that only occurs when user adds first Report Definition
                if (((SimpleReportDefinition)TheSelectedReportDefinition).Group == Properties.Resources.PersistenceFile)
                {
                    QueryStringsPersistence.Add(TheSelectedReportDefinition);
                }
            }

            ConnectionDefinition theSelectedConnectionDefinition = _theMySystemsComboBox.SelectedConnectionDefinition;
            ScratchConnectionDefinition theScratchConnectionDefinition = new ScratchConnectionDefinition(theSelectedConnectionDefinition);

            // if there is highlighting, change the definition of the selected report definition
            string theStatement = theHighlightedStatement;
            TheSelectedReportDefinition.SetProperty(ReportDefinition.DEFINITION, theStatement);
            TheSelectedReportDefinition.CurrentOperation = op;

            //Check if any parameters are needed
            ReportParameterProcessor theReportParameterProcessor = ReportParameterProcessor.Instance;
            List<ReportParameter> theReportParameters = theReportParameterProcessor.getReportParams(TheSelectedReportDefinition);
            String theRawQuery = theReportParameterProcessor.getRawQuery(TheSelectedReportDefinition);

            Hashtable predefinedParametersHash = new Hashtable();
            string defaultCatalogName = (_theSelectedTrafodionCatalog == null) ? "" : _theSelectedTrafodionCatalog.ExternalName;
            string defaultSchemaName = (_theSelectedTrafodionSchema == null) ? "" : _theSelectedTrafodionSchema.ExternalName;
            //theScratchConnectionDefinition.DefaultCatalog = defaultCatalogName;
            theScratchConnectionDefinition.DefaultSchema = defaultSchemaName;

            predefinedParametersHash.Add(ReportParameterProcessor.CATALOG_NAME, defaultCatalogName);
            predefinedParametersHash.Add(ReportParameterProcessor.SCHEMA_NAME, defaultSchemaName);

            if (theReportParameters.Count > 0 && TheQueryListUserControl.TheCurrentBatchMode == QueryListUserControl.BatchMode.None)
            {
                // Only for non-batch mode
                if (!theReportParameterProcessor.populateParamsFromUser(theReportParameters, predefinedParametersHash))
                {
                    // There are parameters but there is a problem with them.
                    // User clicked cancel on parameters dialog box
                    if (op == ReportDefinition.Operation.Execute)
                    {
                        if (TheSelectedReportDefinition.ResultContainer == null)
                        {
                            TheSelectedReportDefinition.ResultContainer = new QueryResultContainer(new QueryResultControl());
                        }
                    }
                    else
                    {
                        if (TheSelectedReportDefinition.PlanContainer == null)
                        {
                            TheSelectedReportDefinition.PlanContainer = new QueryPlanContainer(new QueryPlanControl());
                        }
                    }

                    ExecuteCancelled(TheSelectedReportDefinition);
                    TheQueryDetailsUserControl.TheReportDefinition = TheSelectedReportDefinition;
                    ResetExecuteControls();
                    return;
                }
            }
            TheSelectedReportDefinition.SetProperty(ReportDefinition.PARAMETERS, theReportParameters);
            theStatement = theReportParameterProcessor.getQueryForExecution(theRawQuery, theReportParameters);
            TheSelectedReportDefinition.SetProperty(ReportDefinition.RAW_QUERY, theRawQuery);
            TheSelectedReportDefinition.SetProperty(ReportDefinition.ACTUAL_QUERY, theStatement);

            //Recreate the result control every time the query is executed
            if (op == ReportDefinition.Operation.Execute)
            {
                //Dispose the old result container to clean up resources
                if (TheSelectedReportDefinition.ResultContainer != null)
                {
                    TheSelectedReportDefinition.ResultContainer.Dispose();
                }

                if (_selectReg.IsMatch(theStatement.ToLower()))
                {
                    TheSelectedReportDefinition.ResultContainer = new QueryResultContainer(new QueryResultControl());
                }
                else if ((_insertReg.IsMatch(theStatement.ToLower())) || (_deleteReg.IsMatch(theStatement.ToLower())) || (_updateReg.IsMatch(theStatement.ToLower())))
                {
                    TheSelectedReportDefinition.ResultContainer = new QueryResultContainer(new NonQueryResultControl(theStatement, ""));
                }
                else
                {
                    TheSelectedReportDefinition.ResultContainer = new QueryResultContainer(new QueryResultControl());
                }
            }
            else
            {
                //Dispose the old result container to clean up resources
                if (TheSelectedReportDefinition.PlanContainer != null)
                {
                    TheSelectedReportDefinition.PlanContainer.Dispose();
                }

                TheSelectedReportDefinition.PlanContainer = new QueryPlanContainer(new QueryPlanControl());
            }

            TheQueryDetailsUserControl.TheReportDefinition = TheSelectedReportDefinition;
            TheSelectedReportDefinition.DataTable = new DataTable();
            TheSelectedReportDefinition.SetProperty(ReportDefinition.LAST_EXECUTION_STATUS, "");
            TheSelectedReportDefinition.SetProperty(ReportDefinition.CURRENT_EXECUTION_STATUS, ReportDefinition.STATUS_EXECUTING);
            TheSelectedReportDefinition.SetProperty(ReportDefinition.START_TIME, DateTime.Now);
            TheSelectedReportDefinition.SetProperty(ReportDefinition.FETCH_NEXT_PAGE, true);
            TheSelectedReportDefinition.SetProperty(ReportDefinition.FETCH_TABLE_STATS, _includeTableStatsCheckBox.Checked);

            // Create a SqlCommandObject to pass to the worker execution thread
            SqlCommandObject theCommandObject = new SqlCommandObject(TheSelectedReportDefinition, theScratchConnectionDefinition, op, theStatement, MaxRows, RowsPerPage);
            theCommandObject.DefaultCatalogName = defaultCatalogName;
            theCommandObject.DefaultSchemaName = defaultSchemaName;

            _queryThread = new ThreadWithState(theCommandObject, 
                                                _activeCacheConnectionObjects,
                                                ProgressCallback, ResultCallback, ReportException, UpdatePageCallback);

            if (op == ReportDefinition.Operation.Execute)
            {
                ((QueryResultContainer)TheSelectedReportDefinition.ResultContainer).GetReadyExecute(MaxRows);
            }
            else
            {
                PreFetchExplainPlanSetup();
                ((QueryPlanContainer)TheSelectedReportDefinition.PlanContainer).GetReadyExecute();
            }

            GetReadyExecute(op);

            Utilities.MEMORYSTATUSEX memStatus = Utilities.GetCurrentMemoryStatus();
            initialUsedVirtualMemory = memStatus.ullTotalVirtual - memStatus.ullAvailVirtual;

            // Tell the worker to do work
            _queryThread.Start();

        }

        private void GetReadyExecute(ReportDefinition.Operation op)
        {
            // Update button only if not batch mode
            if (!_batchMode)
            {
                if (op == ReportDefinition.Operation.Execute)
                {
                    _theExecuteButton.Text = Properties.Resources.Cancel;
                    _theExplainButton.Enabled = false;
                }
                else
                {
                    _theExecuteButton.Enabled = false;
                    _theExplainButton.Text = Properties.Resources.Cancel;
                }
            }
        }

        // This method is called when the execute is completed or cancelled
        private void ResetExecuteControls()
        {
            if (!_batchMode)
            {
                _theExecuteButton.Text = Properties.Resources.Execute;
                _theExecuteButton.Enabled = true;
                _theExplainButton.Text = Properties.Resources.Explain;
                _theExplainButton.Enabled = true;
            }
        }

        private void ResetExplainControls()
        {
            if (!_batchMode)
            {
                _theExecuteButton.Text = Properties.Resources.Execute;
                _theExecuteButton.Enabled = true;
                _theExplainButton.Text = Properties.Resources.Explain;
                _theExplainButton.Enabled = true;
            }
        }

        //Clean up when execute is cancelled
        private void ExecuteCancelled(ReportDefinition aReportDefinition)
        {
            if (aReportDefinition.CurrentOperation == ReportDefinition.Operation.Execute)
            {
                QueryResultContainer queryResultControl = aReportDefinition.ResultContainer as QueryResultContainer;
                if (queryResultControl != null)
                {
                    queryResultControl.ExecuteCancelled(aReportDefinition);
                }
            }
            else
            {
                QueryPlanContainer queryPlanControl = aReportDefinition.PlanContainer as QueryPlanContainer;
                if (queryPlanControl != null)
                {
                    queryPlanControl.ExecuteCancelled(aReportDefinition);
                }
            }

            ResetExecuteControls();
            aReportDefinition.RaiseExecuteCompleteEvent();
        }

        // Updates the Schema and Catalog combo boxes after create commands are executed.  
        private void UpdateLists()
        {
            string theStatement = _theQueryInputControl.TheStatement;

            if (theStatement.Length > 0)
            {

                string theObjectName;
                SpecialCommand theCategory = CategorizeCommand(theStatement, out theObjectName);

                if (theCategory == SpecialCommand.CreateCatalog || theCategory == SpecialCommand.DropCatalog)
                {
                    _theCatalogsComboBox.RefreshContents();
                    _theSchemasComboBox.RefreshContents();
                }
                else
                {
                    if (theCategory == SpecialCommand.CreateSchema || theCategory == SpecialCommand.DropSchema)
                    {
                        _theSchemasComboBox.RefreshContents();
                    }
                }

                if (theCategory == SpecialCommand.CreateCatalog)
                {
                    _theCatalogsComboBox.SelectExternalName(theObjectName);
                }
                else if (theCategory == SpecialCommand.CreateSchema)
                {
                    _theSchemasComboBox.SelectExternalName(theObjectName);
                }

            }
        }

        private void ReportDefinitionChanged(object aSender, ReportDefinition aReportDefinition, ReportDefinition.Reason aReason)
        {
            if (aReportDefinition == TheSelectedReportDefinition)
            {
                switch (aReason)
                {
                    case ReportDefinition.Reason.NameChanged:
                        {
                            TheInputName = aReportDefinition.Name;
                            break;
                        }
                    case ReportDefinition.Reason.StatementChanged:
                        {
                            string theFullStatement = aReportDefinition.GetProperty(ReportDefinition.FULL_RAW_TEXT) as string;
                            // If there is no full raw text, get the definition
                            if (theFullStatement == null)
                            {
                                string theDefinitionStatement = aReportDefinition.GetProperty(ReportDefinition.DEFINITION) as string;
                                if (theDefinitionStatement == null)
                                {
                                    TheInputStatement = "";
                                }
                                else
                                {
                                    TheInputStatement = theDefinitionStatement;
                                }
                            }
                            else
                            {
                                TheInputStatement = theFullStatement;
                            }
                            break;
                        }

                    case ReportDefinition.Reason.ControlSettingChanged:
                        {
                            List<ReportControlStatement> controlStatements = aReportDefinition.GetProperty(ReportDefinition.CONTROL_STATEMENTS) as List<ReportControlStatement>;
                            TheInputControlStatements = controlStatements;
                            break;
                        }

                    default:
                        {
                            break;
                        }
                }
            }
        }

        void TheGrid_SelectionChanged(object sender, EventArgs e)
        {
            if (_theSelectedConnectionDefinition != null)
            {
                try
                {
                    _suppressDataGridViewSelectionChanged++;
                    if (_suppressDataGridViewSelectionChanged == 0)
                    {
                        if (IsStatementTextPresent && IsStatementNameOrTextOrControlSettingChanged)
                        {
                            ReportDefinitionChangedByUserDialog theReportDefinitionChangedByUserDialog = new ReportDefinitionChangedByUserDialog();
                            if (theReportDefinitionChangedByUserDialog.ShowDialog() == DialogResult.OK)
                            {
                                switch (theReportDefinitionChangedByUserDialog.TheChoice)
                                {
                                    case ReportDefinitionChangedByUserDialog.Choice.AddNew:
                                        {
                                            string reportName = TheInputName;
                                            if(QueryStringsPersistence.ReportDefinitionExists(reportName))
                                            {
                                                ReportDefinition existingReport = QueryStringsPersistence.GetReportDefinition(reportName);
                                                if (existingReport != null)
                                                {
                                                    reportName = ((SimpleReportDefinition)existingReport).GetNewName();
                                                }
                                            }
                                            ReportDefinition theNewReportDefinition = new SimpleReportDefinition(reportName);
                                            theNewReportDefinition.SetProperty(ReportDefinition.DEFINITION, TheInputStatement);
                                            theNewReportDefinition.SetProperty(ReportDefinition.FULL_RAW_TEXT, TheInputStatement);
                                            List<ReportControlStatement> controlStatements = _theCQSettingControl.CurrentControlStatements;
                                            if (controlStatements != null && controlStatements.Count > 0)
                                            {
                                                theNewReportDefinition.SetProperty(ReportDefinition.CONTROL_STATEMENTS, controlStatements);
                                            }
                                            QueryStringsPersistence.Add(theNewReportDefinition);
                                            _theQueryListUserControl.TheQueryListDataGridView.SelectedReportDefinition = theNewReportDefinition;
                                            _theQueryDetailsUserControl.TheReportDefinition = theNewReportDefinition;
                                            break;
                                        }
                                    case ReportDefinitionChangedByUserDialog.Choice.UpdateExisting:
                                        {
                                            DoUpdate();
                                            break;
                                        }
                                    default:
                                        {
                                            throw new ApplicationException("Code error: Bad ReportDefinitionChangedByUserDialog.Choice value.");
                                        }
                                }
                            }
                        }
                    }
                }
                finally
                {
                    _suppressDataGridViewSelectionChanged--;
                }
            }

            ProcessDataGridSelectionChanged();
        }

        /// <summary>
        /// Update the entire SQL Whiteboard with the new selected report definition
        /// </summary>
        private void ProcessDataGridSelectionChanged()
        {
            // Since we'll be changing Name, Statements, etc., so it would be ok to suppress the some of the events for now. 
            _suppressNameTextBoxTextChanged = true;
            _suppressQueryTextBoxTextChanged = true;
            _suppressCQSettingChanged = true;

            try
            {
                TheSelectedReportDefinition = TheQueryListDataGridView.SelectedReportDefinition;

                TheInputName = TheSelectedName;
                TheInputStatement = TheSelectedFullStatement;
                TheInputControlStatements = TheSelectedControlStatements;

                TheQueryDetailsUserControl.TheReportDefinition = TheSelectedReportDefinition;

                //UpdateControls();

                TheQueryTextBox.ResetSyntax();
                TheQueryListDataGridView.Select();
            }
            finally
            {
                _suppressNameTextBoxTextChanged = false;
                _suppressQueryTextBoxTextChanged = false;
                _suppressCQSettingChanged = false;
            }
        }

        private void RemoveDataGridEventHandlers()
        {
            TheQueryListDataGridView.SelectionChanged -= TheGrid_SelectionChanged;
            TheQueryListDataGridView.OnDoubleClicked -= TheGrid_DoubleClick;
        }

        private void TheGrid_DoubleClick(object sender, int rowIndex)
        {
            if (TheQueryListDataGridView.SelectedRowIndex <= 0 ||
                TheQueryListDataGridView.SelectedRow.Type != TenTec.Windows.iGridLib.iGRowType.Normal)
            {
                return;
            }

            TheInputName = TheSelectedName;
            TheInputStatement = TheSelectedFullStatement;
            TheInputControlStatements = TheSelectedControlStatements;

            if (CanExecuteOrExplain && (_theMySystemsComboBox.SelectedIndex >= 0))
            {
                if (TheQueryListUserControl.TheCurrentBatchMode == QueryListUserControl.BatchMode.BatchExplain)
                {
                    DoExplain();
                }
                else
                {
                    DoExecute();
                }
            }
            UpdateControls();
        }

        /// <summary>
        /// To select a new system.
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        public void SelectConnectionDefinition(ConnectionDefinition aConnectionDefinition)
        {
            _theSelectedConnectionDefinition = aConnectionDefinition;

            bool showCatalogs = (_theSelectedConnectionDefinition != null);

            _theCatalogsPanel.Visible = showCatalogs;

            if (_theSelectedConnectionDefinition != null)
            {
                if (_theSelectedConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
                {
                    _theMySystemsComboBox.SelectedConnectionDefinition = _theSelectedConnectionDefinition;
                   // _theCatalogsComboBox.TheTrafodionSystem = TrafodionSystem.FindTrafodionSystem(_theSelectedConnectionDefinition);

                   /* try
                    {
                        _theCatalogsComboBox.SelectedTrafodionCatalog = _theCatalogsComboBox.TheTrafodionSystem.FindCatalog(_theMySystemsComboBox.SelectedCatalogName);
                        _theSchemasComboBox.TheTrafodionCatalog = _theCatalogsComboBox.SelectedTrafodionCatalog;
                    }
                    catch (Exception ex)
                    {
                        if (_theCatalogsComboBox.Items.Count > 0)
                        {
                            _theCatalogsComboBox.SelectedIndex = 0;
                        }
                        else
                        {
                            _theCatalogsComboBox.SelectedIndex = -1;
                        }
                    }

                    if (_theMySystemsComboBox.SelectedSchemaName != null)
                    {
                        if (!String.IsNullOrEmpty(_theMySystemsComboBox.SelectedSchemaName))
                        {
                            try
                            {
                                _theSchemasComboBox.SelectedTrafodionSchema = _theCatalogsComboBox.SelectedTrafodionCatalog.FindSchema(_theMySystemsComboBox.SelectedSchemaName);
                            }
                            catch (Exception)
                            {
                                if (_theSchemasComboBox.Items.Count > 0)
                                {
                                    _theSchemasComboBox.SelectedIndex = 0;
                                }
                                else
                                {
                                    _theSchemasComboBox.SelectedIndex = -1;
                                }
                            }
                        }
                    }*/

                }
            }

            _theCQSettingControl.ConnectionDefn = _theSelectedConnectionDefinition;

            UpdateControls();
            FireOnMySystemChanged(new EventArgs());
        }

        void ConnectionDefinitionChanged(object aSender, ConnectionDefinition aConnectionDefinition, ConnectionDefinition.Reason aReason)
        {
            ConnectionDefinition theOldConnectionDefinition = aSender as ConnectionDefinition;

            if (theOldConnectionDefinition != null)
            {
                if ((aReason == ConnectionDefinition.Reason.Name) && (theOldConnectionDefinition.Name != aConnectionDefinition.Name))
                {
                    // user is changing Connection Definition Name, check if theConnection already established
                    if (_activeCacheConnectionObjects.ContainsKey(theOldConnectionDefinition.Name))
                    {
                        // get the Connection
                        CacheConnectionObject cacheConnection = (CacheConnectionObject)_activeCacheConnectionObjects[theOldConnectionDefinition.Name];
                        _activeCacheConnectionObjects.Add(aConnectionDefinition.Name, cacheConnection);

                        // remove the Old connection 
                        _activeCacheConnectionObjects.Remove(theOldConnectionDefinition.Name);
                    }
                }
                else if (aReason == ConnectionDefinition.Reason.Password)
                {
                    if (_activeCacheConnectionObjects.ContainsKey(theOldConnectionDefinition.Name))
                    {
                        //Connection established but password has been cleared. We don't need to do anything now
                        //but we will do a cleanup when the connection is closed.
                        if (aConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
                        {
                            //All is good
                        }
                        else
                        {
                            //This is a strange situation where the connection is open for the white board but the
                            //passwords for the connection definition has been cleared. We might want to apply some 
                            //logic to disable the catalog combo here.
                        }

                    }
                    else
                    {
                        if (aConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
                        {
                            //The password has been applied to a connection def and it's a tested connection, all is good
                            //and nothing needs to be done here. The population of catalog and schema have been handled 
                            //as a part of the SelectConnectionDefinition method.
                        }
                        else
                        {
                            //There is no connection and the password has been cleared. So remove the catalog and schema information
                            _theCatalogsComboBox.TheTrafodionSystem = null;
                            _theSchemasComboBox.TheTrafodionCatalog = null;
                        }
                    }
                }

            }

            UpdateControls();
        }

        //void TheQueryTabControlSelectedIndexChanged(object sender, EventArgs e)
        //{
        //    UpdateControls();
        //}

        private void TheQueryTextBoxTextChanged(object sender, EventArgs e)
        {
            if (!_suppressQueryTextBoxTextChanged)
            {
                UpdateControls();
            }
        }

        private enum SpecialCommand { CreateCatalog, CreateSchema, DropCatalog, DropSchema, NotSpecial };

        private SpecialCommand CategorizeCommand(string aStatement, out string aName)
        {
            SpecialCommand theResult;

            // Check for create or drop and refresh if so 
            if (_theCreateCatalogReg.IsMatch(aStatement))
            {
                theResult = SpecialCommand.CreateCatalog;
            }
            else if (_theCreateSchemaReg.IsMatch(aStatement))
            {
                theResult = SpecialCommand.CreateSchema;
            }
            else if (_theDropCatalogReg.IsMatch(aStatement))
            {
                theResult = SpecialCommand.DropCatalog;
            }
            else if (_theDropSchemaReg.IsMatch(aStatement))
            {
                theResult = SpecialCommand.DropSchema;
            }
            else
            {
                aName = null;
                return SpecialCommand.NotSpecial;
            }

            string[] theWords = Regex.Split(aStatement, @"(\s)+");

            aName = (theWords.Length > 4) ? theWords[4] : "";   // Verb type name
            //                                                       0 1  2 3 4
            return theResult;

        }

        /// <summary>
        /// When the user has clicked on the Explain button
        /// </summary>
        private void DoExplain()
        {
            DoExecute(ReportDefinition.Operation.Explain);
        }

        //void theQueryPlanContainer_OnPostExplain(object aSender)
        //{
        //    //ResetExplainControls();
        //    ResetExecuteControls();
        //}

        /// <summary>
        /// PreFetchExplainPlanSetup
        /// </summary>
        public void PreFetchExplainPlanSetup()
        {
            if (!_batchMode)
            {
                this._theExplainButton.Enabled = true;
                this._theExplainButton.Text = Properties.Resources.Cancel;
                this._theExecuteButton.Enabled = false;
            }
        }

        private void SetDefaultMaxRows()
        {
            //to enable use of SQL Whiteboard options
            //_theMaxRowsTextBox.Text = QueryInputControl.MAX_ROWS_DEFAULT.ToString();
            _theMaxRowsTextBox.Text = SQLWhiteboardOptions.GetOptions().DefaultMaxRowCount.ToString();
            _rowsPerPageTextBox.Text = DEFAULT_ROWS_PER_PAGE.ToString();
        }

        private void TheCatalogsComboBoxSelectedIndexChanged(object sender, EventArgs e)
        {
            _theSelectedTrafodionCatalog = _theCatalogsComboBox.SelectedTrafodionCatalog;
            _theSchemasComboBox.TheTrafodionCatalog = _theSelectedTrafodionCatalog;
            if (!_theCatalogsComboBox.IsLoading && _theCatalogsComboBox.SelectedTrafodionCatalog != null)
            {
                _theMySystemsComboBox.SelectedCatalogName = _theCatalogsComboBox.SelectedTrafodionCatalog.ExternalName;
            }
            UpdateControls();
        }
        private void TheSchemasComboBoxSelectedIndexChanged(object sender, EventArgs e)
        {
            _theSelectedTrafodionSchema = _theSchemasComboBox.SelectedTrafodionSchema;
            if (!_theSchemasComboBox.IsLoading && _theSchemasComboBox.SelectedTrafodionSchema != null)
            {
                _theMySystemsComboBox.SelectedSchemaName = _theSchemasComboBox.SelectedTrafodionSchema.ExternalName;
            }
            UpdateControls();
        }

        /// <summary>
        /// The handler of SQL whiteboard changed event
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void HandleSQLWhiteboardOptionsChanged(object sender, EventArgs e)
        {
            //_theMaxRowsTextBox.Text = SQLWhiteboardOptions.GetOptions().DefaultMaxRowCount.ToString();
        }

        private void TheUpdateButtonClick(object sender, EventArgs e)
        {
            DoUpdate();
        }
        private void TheExplainButtonClick(object sender, EventArgs e)
        {
            // Handle the case when the button becomes a Cancel Button
            if (_theExplainButton.Text.Equals(Properties.Resources.Cancel))
            {
                AbortCurrentQuery(true);
            }
            else
            {
                DoExplain();
            }
        }

        private void TheMySystemsComboBoxSelectedIndexChanged(object sender, EventArgs e)
        {
            if (!_theMySystemsComboBox.IsLoading)
            {
                headerPanel.Hide();
                _theSelectedConnectionDefinition = _theMySystemsComboBox.SelectedConnectionDefinition;
                if (_theSelectedConnectionDefinition != null)
                {
                    if (_theSelectedConnectionDefinition.TheState != ConnectionDefinition.State.TestSucceeded)
                    {
                        ConnectionDefinitionDialog theConnectionDefinitionDialog = new ConnectionDefinitionDialog(false);
                        theConnectionDefinitionDialog.Edit(_theSelectedConnectionDefinition);
                    }
                    if (_theSelectedConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
                    {
                        Cursor.Current = Cursors.WaitCursor;

                        try
                        {
                            _theCatalogsComboBox.TheTrafodionSystem = TrafodionSystem.FindTrafodionSystem(_theSelectedConnectionDefinition);
                            try
                            {
                                _theCatalogsComboBox.SelectedTrafodionCatalog = _theCatalogsComboBox.TheTrafodionSystem.FindCatalog(_theMySystemsComboBox.SelectedCatalogName);
                                _theSchemasComboBox.TheTrafodionCatalog = _theCatalogsComboBox.SelectedTrafodionCatalog;
                            }
                            catch (Exception ex)
                            {
                                if (_theCatalogsComboBox.Items.Count > 0)
                                {
                                    _theCatalogsComboBox.SelectedIndex = 0;
                                }
                                else
                                {
                                    _theCatalogsComboBox.SelectedIndex = -1;
                                }
                            }

                            if (_theMySystemsComboBox.SelectedSchemaName != null)
                            {
                                if (!String.IsNullOrEmpty(_theMySystemsComboBox.SelectedSchemaName))
                                {
                                    try
                                    {
                                        _theSchemasComboBox.SelectedTrafodionSchema = _theCatalogsComboBox.SelectedTrafodionCatalog.FindSchema(_theMySystemsComboBox.SelectedSchemaName);
                                    }
                                    catch (Exception ex)
                                    {
                                        if (_theSchemasComboBox.Items.Count > 0)
                                        {
                                            _theSchemasComboBox.SelectedIndex = 0;
                                        }
                                        else
                                        {
                                            _theSchemasComboBox.SelectedIndex = -1;
                                        }
                                    }
                                }
                            }
                        }
                        catch (Exception e1)
                        {

                        }
                        finally
                        {
                            Cursor.Current = Cursors.Default;
                        }
                        TrafodionContext.Instance.OnConnectionDefinitionSelection(this);
                        SetSystemDescription(_theSelectedConnectionDefinition);
                    }
                    else
                    {
                        _theMySystemsComboBox.SelectedIndex = -1;
                        _theCatalogsComboBox.TheTrafodionSystem = null;
                        _theSchemasComboBox.TheTrafodionCatalog = null;
                        SetSystemDescription(null);
                    }
                }
                else
                {
                    SetSystemDescription(null);
                }
            }

            UpdateControls();
            FireOnMySystemChanged(new EventArgs());
        }

        private void SetSystemDescription(ConnectionDefinition aConnectionDefinition)
        {
            Control parent = this.TopLevelControl;
            if (parent != null && parent is ManagedWindow)
            {
                ((ManagedWindow)parent).ConnectionDefn = aConnectionDefinition;
            }

            // Relay the new connection definition to the CQ Setting widget.
            if (TheCQSettingControl != null)
            {
                TheCQSettingControl.ConnectionDefn = _theSelectedConnectionDefinition;
            }
        }

        private void TheNameTextBoxTextChanged(object sender, EventArgs e)
        {
            if (!_suppressNameTextBoxTextChanged)
            {
                UpdateControls();
            }
        }

        private void TheCQSettingControl_OnCQSettingChanged(object sender, EventArgs e)
        {
            if (!_suppressCQSettingChanged)
            {
                UpdateControls();
            }
        }

        private void TheMaxRowsTextBoxTextChanged(object sender, EventArgs e)
        {
            UpdateControls();
        }

        // The callback method must match the signature of the
        // callback delegate.
        //
        public void UpdateErrorMessage(ReportDefinition reportDefinition, Exception exception)
        {
            reportDefinition.ResetDataTable();
            ReportDefinition.Operation currentOp = reportDefinition.CurrentOperation;
            string errorMessage = exception.Message;
            if (exception is OdbcException && errorMessage.Contains(ThreadWithState._error08S01))
            {
                errorMessage = Properties.Resources.WhiteboardConnectionDisappearedMessage;
            }
            if (exception is OutOfMemoryException)
            {
                if (MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.WhiteboardOutOfMemoryMessage, Properties.Resources.Error, MessageBoxButtons.YesNo,
                    MessageBoxIcon.Error) == DialogResult.Yes)
                {
                    try
                    {
                        AbortCurrentQuery(true);
                        DiscardAllResults();
                        DoExecute(currentOp);
                        return;
                    }
                    catch (Exception)
                    {
                        //Fallback to setting the error message below
                    }
                }
                errorMessage = Properties.Resources.WhiteboardOutOfMemoryNoRetryMessage;
            }

            if (currentOp == ReportDefinition.Operation.Execute)
            {
                //((QueryResultContainer)reportDefinition.ResultContainer).QueryResultControl = new QueryTextTabPage(Properties.Resources.ExecutionError, errorMessage, true);
                SqlStatementTextBox theTextBox = new SqlStatementTextBox();
                theTextBox.ReadOnly = true;
                theTextBox.Text = errorMessage;
                theTextBox.Dock = DockStyle.Fill;
                theTextBox.WordWrap = true;
                ((QueryResultContainer)reportDefinition.ResultContainer).QueryResultControl = theTextBox;
                reportDefinition.SetProperty(ReportDefinition.SAVED_EXEC_EXCEPTION, errorMessage);
                //If an exception was thrown in the background method, report it
                ((QueryResultContainer)reportDefinition.ResultContainer).ExecutionFailed(reportDefinition);
                TheQueryDetailsUserControl.TheQueryResultTabPage.Select();
            }
            else
            {
                //((QueryPlanContainer)reportDefinition.PlanContainer).QueryPlanControl = new QueryTextTabPage(Properties.Resources.PlanError, errorMessage, true);
                SqlStatementTextBox theTextBox = new SqlStatementTextBox();
                theTextBox.ReadOnly = true;
                theTextBox.Text = errorMessage;
                theTextBox.Dock = DockStyle.Fill;
                theTextBox.WordWrap = true;
                ((QueryPlanContainer)reportDefinition.PlanContainer).QueryPlanControl = theTextBox;
                reportDefinition.SetProperty(ReportDefinition.SAVED_PLAN_EXCEPTION, errorMessage);
                //If an exception was thrown in the background method, report it
                ((QueryPlanContainer)reportDefinition.PlanContainer).ExecutionFailed(reportDefinition);
                TheQueryDetailsUserControl.TheQueryPlanTabPage.Select();
            }

            TheQueryDetailsUserControl.TheReportDefinition = reportDefinition;
            ResetExecuteControls();
            reportDefinition.RaiseExecuteCompleteEvent();
        }

        public void ReportException(ReportDefinition reportDefinition, Exception exception)
        {
            try
            {
                this.BeginInvoke(new UpdateMessage(UpdateErrorMessage), new object[] { reportDefinition, exception });
            }
            catch (Exception ex)
            {
                //do nothing
            }
        }

        // The callback method must match the signature of the
        // callback delegate.
        //
        public void ProgressCallback(ReportDefinition reportDefinition, int rowCount, DataTable tableCopy)
        {
            try
            {
                this.BeginInvoke(new UpdateProgress(UpdateProgressBar), new object[] { reportDefinition, rowCount, tableCopy });
            }
            catch (Exception ex)
            {
                //do nothing
            }
        }

        public void UpdateProgressBar(ReportDefinition reportDefinition, int rowCount, DataTable tableCopy)
        {
            if (reportDefinition.CurrentOperation == ReportDefinition.Operation.Execute)
            {
                QueryResultContainer queryResultContainer = reportDefinition.ResultContainer as QueryResultContainer;
                queryResultContainer.UpdateProgressBar(rowCount);
                string theStatement = (string)reportDefinition.GetProperty(ReportDefinition.DEFINITION);

                if ((_insertReg.IsMatch(theStatement.ToLower())) || (_deleteReg.IsMatch(theStatement.ToLower())) || (_updateReg.IsMatch(theStatement.ToLower())))
                {
                    //No op
                }
                else
                {
                    if (reportDefinition.IsGridReport || _selectReg.IsMatch(theStatement.ToLower()))
                    {
                        queryResultContainer.SetRowsRetrieved(rowCount);
                        int currentCount = ((QueryResultControl)queryResultContainer.QueryResultControl).TheGrid.Rows.Count; 
                        if (RowsPerPage > 0 && currentCount + tableCopy.Rows.Count > RowsPerPage)
                        {
                            ((QueryResultControl)queryResultContainer.QueryResultControl).TheGrid.Rows.Clear();
                            currentCount = 0;
                        }

                        if (currentCount > 0)
                        {
                            if (currentCount > reportDefinition.DataTable.Rows.Count)
                            {
                                ((QueryResultControl)queryResultContainer.QueryResultControl).TheGrid.InsertRows(currentCount, tableCopy, 0, tableCopy.Rows.Count);
                            }
                            else
                            {
                                ((QueryResultControl)queryResultContainer.QueryResultControl).TheGrid.InsertRows(currentCount, tableCopy, currentCount, tableCopy.Rows.Count);
                            }
                            ((QueryResultControl)queryResultContainer.QueryResultControl).TheGrid.PerformAction(TenTec.Windows.iGridLib.iGActions.GoLastRow);

                        }
                        else
                        {
                            ((QueryResultControl)queryResultContainer.QueryResultControl).LoadTable(tableCopy);
                            ((QueryResultControl)queryResultContainer.QueryResultControl).LastRowNumber = 0;
                        }
                    }
                    else
                    {
                        string outputText = (string)reportDefinition.GetProperty(ReportDefinition.ROWS_AFFECTED);
                        if (String.IsNullOrEmpty(outputText))
                        {
                            outputText = Properties.Resources.SqlStatementExecuteSuccess;
                        }

                        if (queryResultContainer.QueryResultControl is NonQueryResultControl)
                        {
                            ((NonQueryResultControl)queryResultContainer.QueryResultControl).Status = outputText;
                        }
                        else
                        {
                            queryResultContainer.ReplaceResultControl(new NonQueryResultControl(outputText, theStatement));
                        }
                    }
                }

                //If the virtual memory has exceeded threshold, force the execution into page mode
                if (!IsUsedMemoryUnderThreshold())
                {
                    reportDefinition.SetProperty(ReportDefinition.FETCH_NEXT_PAGE, false);
                    queryResultContainer.DisplayHeaderMessage(Properties.Resources.LowMemoryPaginationMessage);
                }
                else
                {
                    queryResultContainer.ResetHeader();
                }
            }
        }

        /// <summary>
        /// Check if virtual memory used by application is under threshold
        /// </summary>
        /// <returns></returns>
        bool IsUsedMemoryUnderThreshold()
        {
            Utilities.MEMORYSTATUSEX memStatus = Utilities.GetCurrentMemoryStatus();
            
            double currentUsedMem = memStatus.ullTotalVirtual - memStatus.ullAvailVirtual;
            if (currentUsedMem < initialUsedVirtualMemory)
            {
                initialUsedVirtualMemory = currentUsedMem;
            }
            double memoryGrowth = currentUsedMem - initialUsedVirtualMemory;
            double allowedVirtual = memStatus.ullTotalVirtual * 0.75;

            if ((currentUsedMem + memoryGrowth) >= allowedVirtual)
            {
                return false;
            }
            return true;
        }

        // The callback method must match the signature of the
        // callback delegate.
        //
        public void UpdatePageCallback(ReportDefinition reportDefinition, int pageNumber, int rowsProcessed)
        {
            try
            {
                this.BeginInvoke(new UpdatePage(UpdateResultPage), new object[] { reportDefinition, pageNumber, rowsProcessed });
            }
            catch (Exception ex)
            {
                //do nothing
            }
        }

        /// <summary>
        /// Displays the current page of results
        /// </summary>
        /// <param name="reportDefinition"></param>
        /// <param name="pageNumber"></param>
        public void UpdateResultPage(ReportDefinition reportDefinition, int pageNumber, int rowsProcessed)
        {
            if (reportDefinition.CurrentOperation == ReportDefinition.Operation.Execute)
            {
                QueryResultContainer queryResultContainer = reportDefinition.ResultContainer as QueryResultContainer;
                string currentStatusText = queryResultContainer.GetExecutionTimeLabelText();
                queryResultContainer.SetExecutionTimeLabel(Properties.Resources.LoadingCurrentPageMessage);
                try
                {
                    int currentCount = ((QueryResultControl)queryResultContainer.QueryResultControl).TheGrid.Rows.Count;
                    int startIndex = currentCount % RowsPerPage;
                    if (currentCount + reportDefinition.DataTable.Rows.Count > RowsPerPage)
                    {
                        ((QueryResultControl)queryResultContainer.QueryResultControl).TheGrid.Rows.Clear();
                        currentCount = 0;
                    }
                    if (currentCount > 0)
                    {
                        ((QueryResultControl)queryResultContainer.QueryResultControl).TheGrid.InsertRows(currentCount, reportDefinition.DataTable, startIndex, reportDefinition.DataTable.Rows.Count);
                        ((QueryResultControl)queryResultContainer.QueryResultControl).TheGrid.PerformAction(TenTec.Windows.iGridLib.iGActions.GoFirstCell);
                        ((QueryResultControl)queryResultContainer.QueryResultControl).LastRowNumber = rowsProcessed;
                    }
                    else
                    {
                        string outputText = (string)reportDefinition.GetProperty(ReportDefinition.ROWS_AFFECTED);
                        if (String.IsNullOrEmpty(outputText))
                        {
                            outputText = Properties.Resources.SqlStatementExecuteSuccess;
                        }
                        ((QueryResultControl)queryResultContainer.QueryResultControl).LoadTable(reportDefinition.DataTable);
                        ((QueryResultControl)queryResultContainer.QueryResultControl).LastRowNumber = rowsProcessed;
                    }
                }
                catch (OutOfMemoryException oe)
                {
                    try
                    {
                        DiscardOtherResults(reportDefinition);
                        ((QueryResultControl)queryResultContainer.QueryResultControl).LoadTable(reportDefinition.DataTable);
                        MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.WhiteboardOutOfMemoryUpdateResultsMessage, Properties.Resources.Info, MessageBoxButtons.OK,
                            MessageBoxIcon.Information);
                    }
                    catch (OutOfMemoryException e1)
                    {
                        try
                        {
                            reportDefinition.ResetDataTable();
                            DiscardAllResults();
                            MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.WhiteboardOutOfMemoryReduceRowCountMessage, Properties.Resources.Error, MessageBoxButtons.OK,
                                MessageBoxIcon.Error);
                            AbortCurrentQuery(true);
                            return;
                        }
                        catch (OutOfMemoryException e2)
                        {
                            ReportException(reportDefinition, e2);
                            AbortCurrentQuery(true);
                            return;
                        }
                        catch (Exception ex)
                        {
                            MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.WhiteboardOutOfMemoryNoRetryMessage, Properties.Resources.Error, MessageBoxButtons.OK,
                                MessageBoxIcon.Error);
                            AbortCurrentQuery(true);
                            return;
                        }
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.WhiteboardOutOfMemoryNoRetryMessage, Properties.Resources.Error, MessageBoxButtons.OK,
                            MessageBoxIcon.Error);
                    }

                }

                //If the virtual memory threshold has exceeded, force a garbage collection to free up memory
                if (!IsUsedMemoryUnderThreshold())
                {
                    //GC.Collect();
                    //GC.WaitForPendingFinalizers();
                    ThreadPool.QueueUserWorkItem(new WaitCallback(PerformGC));
                }

                ((QueryResultControl)queryResultContainer.QueryResultControl).PageNumber = pageNumber;
                queryResultContainer.SetExecutionTimeLabel(currentStatusText);
                reportDefinition.DataTable.Rows.Clear();
                queryResultContainer.UpdatePage();
            }
        }

        private void PerformGC(Object stateInfo)
        {
            try
            {
                GC.Collect();
                GC.WaitForPendingFinalizers();
            }
            catch (Exception)
            {
            }
        }

        public void ResultCallback(ReportDefinition reportDefinition)
        {
            try
            {
                this.BeginInvoke(new UpdateResult(UpdateResult), new object[] { reportDefinition });
            }
            catch (Exception ex)
            {
                //Do nothing
            }
        }

        public void UpdateResult(ReportDefinition reportDefinition)
        {
            try
            {
                UpdateReportDefinitionResult(reportDefinition);
            }
            catch (OutOfMemoryException oe)
            {
                try
                {
                    DiscardOtherResults(reportDefinition);
                    UpdateReportDefinitionResult(reportDefinition);
                    MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.WhiteboardOutOfMemoryUpdateResultsMessage, Properties.Resources.Info, MessageBoxButtons.OK,
                        MessageBoxIcon.Information);
                }
                catch (OutOfMemoryException e1)
                {
                    try
                    {
                        reportDefinition.ResetDataTable();
                        DiscardAllResults();
                        MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.WhiteboardOutOfMemoryReduceRowCountMessage, Properties.Resources.Error, MessageBoxButtons.OK,
                            MessageBoxIcon.Error);
                    }
                    catch (OutOfMemoryException e2)
                    {
                        ReportException(reportDefinition, e2);
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.WhiteboardOutOfMemoryNoRetryMessage, Properties.Resources.Error, MessageBoxButtons.OK,
                            MessageBoxIcon.Error);
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.WhiteboardOutOfMemoryNoRetryMessage, Properties.Resources.Error, MessageBoxButtons.OK,
                        MessageBoxIcon.Error);
                }
            }
            // Reset the controls
            ResetExecuteControls();
            reportDefinition.RaiseExecuteCompleteEvent();
        }

        void UpdateReportDefinitionResult(ReportDefinition reportDefinition)
        {
            ReportDefinition.Operation currentOp = reportDefinition.CurrentOperation;
            string theStatement = (string)reportDefinition.GetProperty(ReportDefinition.DEFINITION);
            string executionStatus = reportDefinition.GetProperty(ReportDefinition.LAST_EXECUTION_STATUS) as string;
            if (string.IsNullOrEmpty(executionStatus))
            {
                executionStatus = Properties.Resources.QueryExecutionSuccess;
                reportDefinition.SetProperty(ReportDefinition.LAST_EXECUTION_STATUS, executionStatus);
            }

            if (currentOp == ReportDefinition.Operation.Execute)
            {
                QueryResultContainer queryResultContainer = reportDefinition.ResultContainer as QueryResultContainer;
                if ((_insertReg.IsMatch(theStatement.ToLower())) || (_deleteReg.IsMatch(theStatement.ToLower())) || (_updateReg.IsMatch(theStatement.ToLower())))
                {
                    ((NonQueryResultControl)queryResultContainer.QueryResultControl).Status = String.Format(Properties.Resources.TotalRowsAffected, reportDefinition.GetProperty(ReportDefinition.ROWS_AFFECTED));
                }
                else
                {
                    if (_selectReg.IsMatch(theStatement.ToLower()) || reportDefinition.IsGridReport)
                    {
                        //tabular data
                        if (executionStatus != null && executionStatus.Equals(Properties.Resources.QueryExecutionSuccess))
                        {
                            queryResultContainer.SetExecutionTimeLabel(Properties.Resources.LoadingCurrentPageMessage);
                            int currentCount = ((QueryResultControl)queryResultContainer.QueryResultControl).TheGrid.Rows.Count;
                            if (RowsPerPage > 0)
                            {
                                int currentPage = reportDefinition.DataTable.Rows.Count / RowsPerPage;
                                if (currentPage * RowsPerPage < reportDefinition.DataTable.Rows.Count)
                                {
                                    ((QueryResultControl)queryResultContainer.QueryResultControl).TheGrid.Rows.Clear();
                                    currentCount = 0;
                                }
                            }
                            if (currentCount > 0)
                            {
                                if (currentCount > reportDefinition.DataTable.Rows.Count)
                                {
                                    ((QueryResultControl)queryResultContainer.QueryResultControl).TheGrid.InsertRows(currentCount, reportDefinition.DataTable, 0, reportDefinition.DataTable.Rows.Count);
                                }
                                else
                                {
                                    ((QueryResultControl)queryResultContainer.QueryResultControl).TheGrid.InsertRows(currentCount, reportDefinition.DataTable, currentCount, reportDefinition.DataTable.Rows.Count);
                                }
                                ((QueryResultControl)queryResultContainer.QueryResultControl).TheGrid.PerformAction(TenTec.Windows.iGridLib.iGActions.GoFirstCell);

                            }
                            else
                            {
                                ((QueryResultControl)queryResultContainer.QueryResultControl).LoadTable(reportDefinition.DataTable);
                                //((QueryResultControl)queryResultContainer.QueryResultControl).LastRowNumber = 0;
                            }
                        }
                        reportDefinition.ResetDataTable();
                    }
                    else
                    {
                        //textual output
                        reportDefinition.ResetDataTable();
                        string outputText = (string)reportDefinition.GetProperty(ReportDefinition.ROWS_AFFECTED);
                        if (String.IsNullOrEmpty(outputText))
                        {
                            if (queryResultContainer.QueryResultControl is NonQueryResultControl)
                            {
                                if (string.IsNullOrEmpty(((NonQueryResultControl)queryResultContainer.QueryResultControl).Status))
                                    outputText = Properties.Resources.SqlStatementExecuteSuccess;
                            }
                            else
                            {
                                outputText = Properties.Resources.SqlStatementExecuteSuccess;
                            }
                        }

                        if (queryResultContainer.QueryResultControl is NonQueryResultControl)
                        {
                            ((NonQueryResultControl)queryResultContainer.QueryResultControl).Status = outputText;
                        }
                        else
                        {
                            queryResultContainer.ReplaceResultControl(new NonQueryResultControl(outputText, theStatement));
                        }
                    }
                }

                queryResultContainer.ExecuteComplete(reportDefinition);
            }
            else
            {
                //Explain operation
                QueryPlanContainer queryPlanContainer = reportDefinition.PlanContainer as QueryPlanContainer;
                NCCWorkbenchQueryData wbqd = (NCCWorkbenchQueryData)reportDefinition.GetProperty(ReportDefinition.EXPLAIN_PLAN_DATA);
                if (wbqd != null)
                {
                    ((QueryPlanControl)queryPlanContainer.QueryPlanControl).LoadQueryData(wbqd);
                }
                queryPlanContainer.ExecuteComplete(reportDefinition);
            }

            UpdateLists();

            TheQueryDetailsUserControl.TheReportDefinition = reportDefinition;

        }

        void DiscardAllResults()
        {
            foreach (DataGridViewRow aRow in TheQueryListDataGridView.Rows)
            {
                ReportDefinition aReportDefinition = TheQueryListDataGridView.GetReportDefinition(aRow.Index);
                if (aReportDefinition != null)
                {
                    aReportDefinition.ResetDataTable();
                    if (aReportDefinition.ResultContainer != null)
                    {
                        aReportDefinition.ResultContainer.Dispose();
                    }
                    if (aReportDefinition.PlanContainer != null)
                    {
                        aReportDefinition.PlanContainer.Dispose();
                    }
                }
            }
        }

        void DiscardOtherResults(ReportDefinition aReportDefinition)
        {

            if(aReportDefinition != null)
            {
                aReportDefinition.ResetDataTable();
                if (aReportDefinition.ResultContainer != null)
                {
                    aReportDefinition.ResultContainer.Dispose();
                }
                if (aReportDefinition.PlanContainer != null)
                {
                    aReportDefinition.PlanContainer.Dispose();
                }
            }
        }

        private void _rowsPerPageTextBox_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (Char.IsDigit(e.KeyChar) || e.KeyChar == '\b')
            {
                //OK
            }
            else
            {
                e.Handled = true;
            }
        }

        private void _theMaxRowsTextBox_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (Char.IsDigit(e.KeyChar) || e.KeyChar == '\b')
            {
                //OK
            }
            else
            {
                e.Handled = true;
            }
        }

        private void _rowsPerPageTextBox_KeyUp(object sender, KeyEventArgs e)
        {
            if (RowsPerPage > MAX_ROWS_PER_PAGE)
            {
                _rowsPerPageTextBox.Text = MAX_ROWS_PER_PAGE.ToString();
            }
        }

        private void _theMaxRowsTextBox_KeyUp(object sender, KeyEventArgs e)
        {
            Int64 theMaxRows = 0;
            try
            {
                theMaxRows = Int64.Parse(_theMaxRowsTextBox.Text);
            }
            catch (Exception)
            {
            }
            if (theMaxRows == 0)
            {
                this._theMaxRowsTextBox.Text = QueryInputControl.MAX_ROWS_DEFAULT.ToString();
            }
            else
            if (theMaxRows > Int32.MaxValue)
            {
                this._theMaxRowsTextBox.Text = Int32.MaxValue.ToString();
            }
        }
    }

    #region CacheConnectionObject class

    /// <summary>
    /// For caching sessions
    /// </summary>
    public class CacheConnectionObject
    {
        private String _name = null;
        private Connection _connection = null;
        private QueryUserControl.SessionMode _sessionMode = QueryUserControl.SessionMode.SQLMode;
        private ArrayList _controlResetStatements = null;
        private OdbcCommand _odbcCommand = null;

        /// <summary>
        /// Property: Name
        /// </summary>
        public String Name
        {
            get { return _name; }
            set { _name = value; }
        }

        /// <summary>
        /// Property: Connection
        /// </summary>
        public Connection Connection
        {
            get { return _connection; }
            set { _connection = value; }
        }

        /// <summary>
        /// Property: SessionMode
        /// </summary>
        public QueryUserControl.SessionMode SessionMode
        {
            get { return _sessionMode; }
            set { _sessionMode = value; }
        }

        /// <summary>
        /// Property: ControlResetStatements
        /// </summary>
        public ArrayList ControlResetStatements
        {
            get { return _controlResetStatements; }
            set { _controlResetStatements = value; }
        }

        /// <summary>
        /// Property: OdbcCommand
        /// </summary>
        public OdbcCommand OdbcCommand
        {
            get { return _odbcCommand; }
            set { _odbcCommand = value; }
        } 

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aName"></param>
        public CacheConnectionObject(string aName)
        {
            _name = aName;
        }
    }

    #endregion CacheConnectionObject class

    #region ThreadWithState class


    /// <summary>
    /// The ThreadWithState class contains the information needed for
    /// a task, the method that executes the task, and a delegate
    /// to call when the task is complete.
    /// </summary>
    public class ThreadWithState
    {
        // State information used in the task.
        private SqlCommandObject _sqlCommandObject;
        private const string TRACE_SUB_AREA_NAME = "Background Thread";
        bool _closeConnectionDuringAbort = true;

        /// <summary>
        /// Returns the SqlCommandObject associated with this ThreadWithState object    
        /// </summary>
        public SqlCommandObject SqlCommandObject
        {
            get { return _sqlCommandObject; }
            set { _sqlCommandObject = value; }
        }

        /// <summary>
        /// Property: Connection
        /// </summary>
        private Connection Connection
        {
            get
            {
                if (CacheConnection != null)
                {
                    return CacheConnection.Connection;
                }
                else
                {
                    return null;
                }
            }
        }

        private Dictionary<string, Connection> _activeConnections;

        // Needs to re-use the command object due to the fact that WMS and NDCS commands require to use
        // the same command started with WMSOPEN and CMDOPEN.  Also, from here now on, we'll have only 
        // one command object per connection. 
        private OdbcCommand OdbcCommand
        {
            get
            {
                if (CacheConnection != null)
                {
                    return CacheConnection.OdbcCommand;
                }
                else
                {
                    return null;
                }
            }
        }

        private Dictionary<string, OdbcCommand> _activeCommands;

        // Needs to remember the mode of each session is currently in. It could be in WMS, NDCS CMD, or SQL mode.
        // A session enters the WMS mode via the execution of WMSOPEN and exits the WMS mode via the WMSCLOSE. 
        // A session enters the NDCS CMD mode via the exection of CMDOPEN and exits the NDCS CMD mode via the CMDCLOSE.
        // A session is in the SQL mode by default. 
        private QueryUserControl.SessionMode SessionMode
        {
            get
            {
                if (CacheConnection != null)
                {
                    return CacheConnection.SessionMode;
                }
                else
                {
                    return QueryUserControl.SessionMode.SQLMode;
                }
            }
        }

        private Dictionary<string, QueryUserControl.SessionMode> _activeSessionModes;

        /// <summary>
        /// Property: ControlResetStatements
        /// </summary>
        private ArrayList ControlResetStatements
        {
            get
            {
                if (CacheConnection != null)
                {
                    return CacheConnection.ControlResetStatements;
                }
                else
                {
                    return null;
                }

            }
        }

        public bool CloseConnectionDuringAbort
        {
            get { return _closeConnectionDuringAbort; }
            set { _closeConnectionDuringAbort = value; }
        }

        private Dictionary<string, ArrayList> _activeControlResetStatements;

        private Dictionary<string, CacheConnectionObject> _activeCacheConnectionObjects;

        /// <summary>
        /// Property: CacheConnection
        /// </summary>
        private CacheConnectionObject CacheConnection
        {
            get
            {
                CacheConnectionObject cacheConnection = null;
                if (_activeCacheConnectionObjects.TryGetValue(SqlCommandObject.TheConnectionDefinition.Name, out cacheConnection))
                {
                    return cacheConnection;
                }
                else
                {
                    return null;
                }
            }
        }

        // The ODBC error code handles here
        public const string _error08S01 = "08S01";
        public const string _error2050 = "ERROR[2050]";
        public const string _error8822 = "ERROR[8822]";

        // Delegate used to execute the callback method when the
        // task is complete.
        private UpdateProgress _progressCallback;
        private UpdatePage _updatePageCallback;
        private UpdateResult _resultCallback;
        private UpdateMessage _exceptionCallback;
        Thread _thread = null;
        // The constructor obtains the state information and the
        // callback delegate.
        public ThreadWithState(SqlCommandObject cmdObject, 
            Dictionary<string, CacheConnectionObject> activeCacheConnectionObjects,
            UpdateProgress progressDelegate, UpdateResult resultDelegate,
            UpdateMessage reportException, UpdatePage updatePage)
        {
            _sqlCommandObject = cmdObject;
            _activeCacheConnectionObjects = activeCacheConnectionObjects;
            _progressCallback = progressDelegate;
            _resultCallback = resultDelegate;
            _exceptionCallback = reportException;
            _updatePageCallback = updatePage;
        }

        public Thread ExecutingThread
        {
            get { return _thread; }
        }

        /// <summary>
        /// Starts a new thread with the ThreadProc as the method
        /// </summary>
        public void Start()
        {
            ThreadStart method = new ThreadStart(this.ThreadProc);
            _thread = new Thread(method);
            _thread.IsBackground = true;
            _thread.Start();
        }

        /// <summary>
        /// Aborts the thread and cleans up the resources
        /// </summary>
        public void Abort()
        {
            if (Logger.IsTracingEnabled)
            {
                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.SQLWhiteboard, TRACE_SUB_AREA_NAME, "Thread aborted by user");
            }

            try
            {
                //Cancel the odbc command to abort the query and stop the server side process.
                if (_sqlCommandObject.OdbcCommandObject != null)
                {
                    try
                    {
                        if (_sqlCommandObject.TheOperation == ReportDefinition.Operation.Execute)
                        {
                            _sqlCommandObject.OdbcCommandObject.Cancel();
                        }
                        else
                        {
                            _sqlCommandObject.NCCQueryPlan.CancelLastExecutedQuery();
                        }
                    }
                    catch (Exception)
                    {
                    }
                }

                if (_closeConnectionDuringAbort)
                {
                    // Due to the timing issue, we want to remove these cached conneciton and 
                    // command out of the dictionary before aborting the thread. The aborting of 
                    // thread will enable the execute button and the user could potentially launch 
                    // another execution right away.  If these elements are not removed first, 
                    // they could be re-used in the thread again while it is in the process of 
                    // aborting. 
                    Connection currentConnection = Connection;

                    if (OdbcCommand != null)
                    {
                        OdbcCommand.Dispose();
                    }

                    // Remove all cached connection and associated stuff.
                    if (CacheConnection != null)
                    {
                        _activeCacheConnectionObjects.Remove(_sqlCommandObject.TheConnectionDefinition.Name);
                    }

                    // The cancel causes the server process to abend, so the connection is not valid anymore
                    if (currentConnection != null)
                    {
                        currentConnection.Close();
                    }
                }
                
                if (_sqlCommandObject.TheOperation == ReportDefinition.Operation.Explain)
                {
                    _sqlCommandObject.NCCQueryPlan = null;
                }

                if (_thread != null)
                {
                    // Abort the thread
                    _thread.Abort("Thread aborted by user");
                }

            }
            catch (Exception ex)
            {
                //do nothing for now
            }
        }

        /// <summary>
        /// Update Session Mode according to the sql statement
        /// </summary>
        /// <param name="aStatement"></param>
        /// <param name="aCacheConnection"></param>
        public void UpdateSessionMode(string aStatement, CacheConnectionObject aCacheConnection)
        {
            // Looking for mode switch:
            // Look for WMSOpen or CMDOpen first.
            if (QueryUserControl._theWMSOpenReg.IsMatch(aStatement))
            {
                // Found the WMSOpen command, switch to WMS mode
                aCacheConnection.SessionMode = QueryUserControl.SessionMode.WMSMode;
            }
            else if (QueryUserControl._theCMDOpenReg.IsMatch(aStatement))
            {
                // Found the CMDOpen command, switch to CMD mode
                aCacheConnection.SessionMode = QueryUserControl.SessionMode.CMDMode;
            }
            else if (QueryUserControl._theCMDCloseReg.IsMatch(aStatement))
            {
                if (SessionMode == QueryUserControl.SessionMode.CMDMode)
                {
                    // Will switch back to SQL mode only if the current mode is CMD, else current mode remains.
                    aCacheConnection.SessionMode = QueryUserControl.SessionMode.SQLMode;
                }
            }
            else if (QueryUserControl._theWMSCloseReg.IsMatch(aStatement))
            {
                if (SessionMode == QueryUserControl.SessionMode.WMSMode)
                {
                    // Will switch back to SQL mode only if the current mode is WMS, else current mode remains.
                    aCacheConnection.SessionMode = QueryUserControl.SessionMode.SQLMode;
                }
            }

            // The rest of commands will not cause mode switching. 
        }


        /// <summary>
        /// The thread procedure executes the report definition
        /// and then invokes the callback delegates with the 
        /// progress, result or error.
        /// </summary>
        public void ThreadProc()
        {
            ReportDefinition aReportDefinition = _sqlCommandObject.TheReportDefinition;
            int _theMaxRows = _sqlCommandObject.TheMaxRows;
            Int32 rowsPerPage = _sqlCommandObject.RowsPerPage;
            if (rowsPerPage == 0)
            {
                rowsPerPage = Int32.MaxValue;
            }
            string aStatement = _sqlCommandObject.TheStatement;
            if (aStatement.Length > 0)
            {
                Connection theConnection = null;
                OdbcCommand theOdbcCommand = null;
                OdbcDataReader theReader = null;
                QueryUserControl.SessionMode theCurrentSessionMode = QueryUserControl.SessionMode.SQLMode;
                OdbcConnection theOdbcConnection = null;
                ReportDefinition.Operation theOperation = _sqlCommandObject.TheOperation;
                ArrayList theControlResetStatements = null;
                CacheConnectionObject theCacheConnectionObject = null;

                aReportDefinition.SetProperty(ReportDefinition.LAST_EXECUTION_SYSTEM, _sqlCommandObject.TheConnectionDefinition.Name);
                aReportDefinition.SetProperty(ReportDefinition.ROWS_AFFECTED, "");
                try
                {
                    if (_activeCacheConnectionObjects.TryGetValue(_sqlCommandObject.TheConnectionDefinition.Name, out theCacheConnectionObject))
                    {
                        theConnection = theCacheConnectionObject.Connection;
                        theCurrentSessionMode = theCacheConnectionObject.SessionMode;
                        theOdbcCommand = theCacheConnectionObject.OdbcCommand;
                        theControlResetStatements = theCacheConnectionObject.ControlResetStatements;
                    }
                    else
                    {
                        theCacheConnectionObject = new CacheConnectionObject(_sqlCommandObject.TheConnectionDefinition.Name);
                        _activeCacheConnectionObjects.Add(_sqlCommandObject.TheConnectionDefinition.Name, theCacheConnectionObject);
                    }

                    if (theConnection == null)
                    {
                        theConnection = new Connection(_sqlCommandObject.TheConnectionDefinition);

                        //Explicitly set a session name so the whiteboard queries do not run under the default
                        //session name of "SQLWHITEBOARD".
                        theConnection.SessionName = "SQLWHITEBOARD";

                        // set connection property with NO timeout

                        // keep tracking all available connections established
                        theCacheConnectionObject.Connection = theConnection;

                        // set session mode to SQL by default
                        theCurrentSessionMode = QueryUserControl.SessionMode.SQLMode;
                        theCacheConnectionObject.SessionMode = theCurrentSessionMode;

                        // set control reset arrays
                        theControlResetStatements = new ArrayList();
                        theCacheConnectionObject.ControlResetStatements = theControlResetStatements;
                    }

                    try
                    {
                        theOdbcConnection = theConnection.OpenNonCachedSQWBOdbcConnection;
                    }
                    catch (Exception oe)
                    {
                        if (Logger.IsTracingEnabled)
                        {
                            Logger.OutputToLog(
                                TraceOptions.TraceOption.DEBUG,
                                TraceOptions.TraceArea.SQLWhiteboard,
                                TRACE_SUB_AREA_NAME,
                                "Open connection exception: " + oe.Message);
                        }

                        //If open connection fails, remove connection from dictionary
                        if (theConnection != null)
                        {
                            theConnection.Close();
                        }

                        if (OdbcCommand != null)
                        {
                            OdbcCommand.Dispose();
                        }

                        _activeCacheConnectionObjects.Remove(_sqlCommandObject.TheConnectionDefinition.Name);

                        //Rethrow exception so background worker can handle it.
                        if (oe is OutOfMemoryException)
                        {
                            throw oe;
                        }
                        else
                        {
                            throw new Exception("Error opening connection: " + oe.Message + Environment.NewLine + "Check the connection and schema name settings.");
                        }
                    }


                    // check if a command already established
                    if (theOdbcCommand != null)
                    {
                        // reuse a command in SQL Whiteboard
                        theOdbcCommand.Connection = theOdbcConnection;
                        // but, remember to clean it up first
                        theOdbcCommand.Parameters.Clear();
                    }
                    else
                    {
                        theOdbcCommand = theOdbcConnection.CreateCommand();

                        // keep tracking all available connections established
                        theCacheConnectionObject.OdbcCommand = theOdbcCommand;
                    }

                    theOdbcCommand.CommandType = CommandType.Text;

                    //store the reference of the odbc command object in the sql command object
                    //The UI thread has access to the sql command object and cancel the query if needed by
                    //cancelling the odbc command
                    _sqlCommandObject.OdbcCommandObject = theOdbcCommand;

                    //check if the catalog and schema settings have changed since the last time, but do it only when the session is in SQL mode
                    if (theCurrentSessionMode == QueryUserControl.SessionMode.SQLMode)
                    {
                        if (theConnection.TheConnectionDefinition.DefaultCatalog.Equals(_sqlCommandObject.DefaultCatalogName) &&
                            theConnection.TheConnectionDefinition.DefaultSchema.Equals(_sqlCommandObject.DefaultSchemaName))
                        {
                            //If they are same, nothing to do.
                        }
                        else
                        {
                            if (!TrafodionContext.Instance.isCommunityEdition)
                            {
                            //If catalog or schema has changed, issue a fresh set schema command.
                            string SettingCommandSchema = "SET SCHEMA " + _sqlCommandObject.DefaultCatalogName + "." + _sqlCommandObject.DefaultSchemaName + ";";
                            theOdbcCommand.CommandText = SettingCommandSchema;

                            try
                            {
                                // test for open communication link
                                Utilities.ExecuteNonQuery(theOdbcCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.SQLWhiteboard, TRACE_SUB_AREA_NAME, true);

                                //Save the catalog and schema to the open connection, so we remember it for the next execution
                                //theConnection.TheConnectionDefinition.DefaultCatalog = _sqlCommandObject.DefaultCatalogName;
                                theConnection.TheConnectionDefinition.DefaultSchema = _sqlCommandObject.DefaultSchemaName;
                            }
                            catch (Exception ex)
                            {
                                //Ignore set schema errors. controlled schema access for example does not allow set schema commands
                                if (ex is OutOfMemoryException)
                                {
                                    throw ex;
                                    }
                                }
                            }
                        }

                        // Reset previous control settings if needed
                        if (theControlResetStatements.Count > 0)
                        {
                            for (int i = 0; theControlResetStatements.Count > i; i++)
                            {
                                theOdbcCommand.CommandText = theControlResetStatements[i].ToString();
                                try
                                {
                                    Utilities.ExecuteNonQuery(theOdbcCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.SQLWhiteboard, TRACE_SUB_AREA_NAME, true);
                                }
                                catch (Exception ex)
                                {
                                    if (ex.Message.Contains(_error2050) || ex.Message.Contains(_error8822))
                                    {
                                        // The CQD is undefined, remove it so that the problem would not persist.
                                        theControlResetStatements.RemoveAt(i);
                                    }
                                    else
                                    {
                                        throw;
                                    }
                                }
                            }

                            theControlResetStatements.Clear();
                        }

                        // Set new control settings
                        List<ReportControlStatement> listControlStatements = (List<ReportControlStatement>)aReportDefinition.GetProperty(ReportDefinition.CONTROL_STATEMENTS);
                        if (listControlStatements != null)
                        {
                            // Prepare all control statements
                            ArrayList controlStatements = NCCCQSettingControl.PrepareControlStatements(listControlStatements);
                            // Prepare the reset control statements too
                            NCCCQSettingControl.SetControlStatementsToReset(listControlStatements, theControlResetStatements);

                            // Now, execute all control statements.
                            for (int i = 0; controlStatements.Count > i; i++)
                            {
                                theOdbcCommand.CommandText = controlStatements[i].ToString();
                                try
                                {
                                    Utilities.ExecuteNonQuery(theOdbcCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.SQLWhiteboard, TRACE_SUB_AREA_NAME, true);
                                }
                                catch (Exception ex)
                                {
                                    // Encountered an exception, clean up all of the RESET control statements for those control statements not yet evaluated
                                    theControlResetStatements.RemoveRange(i, controlStatements.Count - i);
                                    throw ex;
                                }
                            }
                        }
                            
                        // Now, remembered the executed control statements
                        aReportDefinition.SetProperty(ReportDefinition.LAST_EXECUTED_CONTROL_STATEMENTS, listControlStatements);
                    }

                    // Now, it's time to execute or explain the statement
                    if (theOperation == ReportDefinition.Operation.Execute)
                    {
                        theOdbcCommand.CommandText = aStatement;

                        //It's select, so populate the data set 
                        if (QueryUserControl._selectReg.IsMatch(aStatement.ToLower()))
                        {

                            //Get the data table assoicated with the report definition
                            DataTable theTable = aReportDefinition.DataTable;

                            //theReader = theOdbcCommand.ExecuteReader();
                            theReader = Utilities.ExecuteReader(theOdbcCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.SQLWhiteboard, TRACE_SUB_AREA_NAME, false);

                            //Create the columns for the table
                            try
                            {
                                //Try to get columns from result set metadata
                                GetColumnsFromResultSetMetatData(theReader, ref theTable);
                            }
                            catch (Exception ex)
                            {
                                if (ex is OutOfMemoryException)
                                {
                                    throw ex;
                                }
                                else
                                {
                                    //fall back approach and manually figure out the column info
                                    CreateColumnsFromResultSet(theReader, ref theTable);
                                }
                            }

                            int pageNumber = 1;
                            int unprocessedRows = 0;

                            // Add the rows to the result table
                            for (int i = 0; i < _theMaxRows; i++)
                            {
                                // Read until either max rows is reached, or until the reader runs out of data
                                if (theReader.Read())
                                {
                                    object[] theCurrRow = new object[theReader.FieldCount];
                                    for (int currField = 0; currField < theReader.FieldCount; currField++)
                                    {
                                        try
                                        {
                                            //For TIME types, .NET truncates the milliseconds when converting to TimeSpan
                                            //So read the timespan objects as strings and then convert it back to timespan.
                                            if (theTable.Columns[currField].DataType == typeof(System.TimeSpan))
                                            {
                                                string timeSpanString = theReader.GetString(currField);
                                                try
                                                {
                                                    theCurrRow[currField] = TimeSpan.Parse(timeSpanString);
                                                }
                                                catch (Exception oe)
                                                {
                                                    if (oe is OutOfMemoryException)
                                                    {
                                                        throw oe;
                                                    } 
                                                    theCurrRow[currField] = timeSpanString;
                                                }
                                            }
                                            else
                                            {
                                                theCurrRow[currField] = theReader.GetValue(currField);
                                            }
                                        }
                                        catch (Exception ex)
                                        {
                                            if (Logger.IsTracingEnabled)
                                            {
                                                Logger.OutputToLog(
                                                    TraceOptions.TraceOption.DEBUG,
                                                    TraceOptions.TraceArea.SQLWhiteboard,
                                                    TRACE_SUB_AREA_NAME,
                                                    "Reader fetch exception: " + ex.Message);
                                            }

                                            if (ex is OutOfMemoryException)
                                            {
                                                throw ex;
                                            }

                                            try
                                            {
                                                theCurrRow[currField] = theReader.GetString(currField);
                                                theTable.Columns[currField].DataType = typeof(string);
                                            }
                                            catch (Exception e1)
                                            {
                                                if (e1 is OutOfMemoryException)
                                                {
                                                    throw e1;
                                                }
                                            }
                                        }
                                    }

                                    bool fetchNextPage = true;
                                    fetchNextPage = (bool)aReportDefinition.GetProperty(ReportDefinition.FETCH_NEXT_PAGE);

                                    if (!fetchNextPage || (i > 0 && i < _theMaxRows && (i % rowsPerPage == 0)))
                                    {
                                        fetchNextPage = false;
                                        aReportDefinition.SetProperty(ReportDefinition.FETCH_NEXT_PAGE, fetchNextPage);
                                        if (_updatePageCallback != null)
                                        {
                                            _updatePageCallback(aReportDefinition, pageNumber, i);
                                            pageNumber++;
                                            unprocessedRows = 0;
                                        }

                                        while (fetchNextPage == false)
                                        {
                                            if (aReportDefinition.GetProperty(ReportDefinition.FETCH_NEXT_PAGE) != null)
                                            {
                                                fetchNextPage = (bool)aReportDefinition.GetProperty(ReportDefinition.FETCH_NEXT_PAGE);
                                            }

                                            if (!fetchNextPage)
                                            {
                                                Thread.Sleep(3000);
                                            }
                                        }
                                    }

                                    //Add rows to the result data table
                                    theTable.Rows.Add(theCurrRow);
                                    unprocessedRows++;

                                    //Report progress with the current total number of rows processed
                                    if (unprocessedRows % 50 == 0)
                                    {
                                        if (_progressCallback != null)
                                        {
                                            DataTable tableCopy = theTable.Copy();
                                            _progressCallback(aReportDefinition, i+1, tableCopy);
                                            unprocessedRows = 0;
                                        }
                                    }
                                }
                                else
                                {
                                    break;
                                }
                            }
                        }
                        //these would return no of rows update status
                        else if ((QueryUserControl._insertReg.IsMatch(aStatement.ToLower())) || (QueryUserControl._deleteReg.IsMatch(aStatement.ToLower())) || (QueryUserControl._updateReg.IsMatch(aStatement.ToLower())))
                        {
                            aReportDefinition.IsGridReport = false;
                            int rowsAffected = Utilities.ExecuteNonQuery(theOdbcCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.SQLWhiteboard, TRACE_SUB_AREA_NAME, true);

                            aReportDefinition.SetProperty(ReportDefinition.ROWS_AFFECTED, rowsAffected);
                        }
                        //The others will have no row count
                        else
                        {
                            theReader = Utilities.ExecuteReader(theOdbcCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.SQLWhiteboard, TRACE_SUB_AREA_NAME, true);

                            // First, make sure whether we should preceed to read the reader.
                            // For WMS and NDCS commands, checking reader's HasRows would sometimes result in 
                            // invalid cursor state when there is no row returned.  
                            // In the meantime, we also introduced a flag to prevent the following code to 
                            // initiate the Read of the reader since that would cause the command object no longer 
                            // be able to be reused again.  
                            bool readReader = false;
                            try
                            {
                                readReader = theReader.HasRows;
                            }
                            catch (OdbcException oe)
                            {
                                if (Logger.IsTracingEnabled)
                                {
                                    Logger.OutputToLog(
                                        TraceOptions.TraceOption.DEBUG,
                                        TraceOptions.TraceArea.SQLWhiteboard,
                                        TRACE_SUB_AREA_NAME,
                                        "Reader fetch exception: " + oe.Message);
                                }

                                if (oe.Message.Contains("Invalid cursor state"))
                                {
                                    readReader = false;
                                }
                                else
                                {
                                    throw oe;
                                }
                            }

                            // If the result has more than 1 columns, the result will be displayed in grid.
                            // If the result has only 1 column, the result in all rows will be appended into 
                            // a single string and displayed in the result text box.  The reason is that there
                            // are many commands (such as explain, showddl, maintain, etc) would result in only 
                            // one column with many rows.  Here we'll display all of the rows in separate lines.
                            if (readReader && (theReader.FieldCount > 1))
                            {
                                aReportDefinition.IsGridReport = true;
                                //Get the data table assoicated with the report definition
                                DataTable theTable = aReportDefinition.DataTable;

                                // Add columns to the result data table
                                //int theFieldCount = theReader.FieldCount;
                                //for (int colNum = 0; colNum < theFieldCount; colNum++)
                                //{
                                //    try
                                //    {
                                //        string colName = theReader.GetName(colNum);
                                //        theTable.Columns.Add(colName, theReader.GetFieldType(colNum));
                                //    }
                                //    catch (Exception ex)
                                //    {
                                //        theTable.Columns.Add(new DataColumn());
                                //    }
                                //}

                                //Create the columns for the table
                                try
                                {
                                    //Try to get columns from result set metadata
                                    GetColumnsFromResultSetMetatData(theReader, ref theTable);
                                }
                                catch (Exception ex)
                                {
                                    if (Logger.IsTracingEnabled)
                                    {
                                        Logger.OutputToLog(
                                            TraceOptions.TraceOption.DEBUG,
                                            TraceOptions.TraceArea.SQLWhiteboard,
                                            TRACE_SUB_AREA_NAME,
                                            "Get Column metadata exception: " + ex.Message);
                                    }

                                    if (ex is OutOfMemoryException)
                                    {
                                        throw ex;
                                    }
                                    //fall back approach and manually figure out the column info
                                    CreateColumnsFromResultSet(theReader, ref theTable);
                                }

                                int unprocessedRows = 0;
                                // Add the rows to the result table
                                for (int i = 0; i < _theMaxRows; i++)
                                {
                                    // Read until either max rows is reached, or until the reader runs out of data
                                    if (theReader.Read())
                                    {
                                        object[] theCurrRow = new object[theReader.FieldCount];
                                        for (int currField = 0; currField < theReader.FieldCount; currField++)
                                        {
                                            try
                                            {
                                                //For TIME types, .NET truncates the milliseconds when converting to TimeSpan
                                                //So read the timespan objects as strings and then convert it back to timespan.
                                                if (theTable.Columns[currField].DataType == typeof(System.TimeSpan))
                                                {
                                                    string timeSpanString = theReader.GetString(currField);
                                                    try
                                                    {
                                                        theCurrRow[currField] = TimeSpan.Parse(timeSpanString);
                                                    }
                                                    catch (Exception oe)
                                                    {
                                                        if (oe is OutOfMemoryException)
                                                        {
                                                            throw oe;
                                                        }
                                                        theCurrRow[currField] = timeSpanString;
                                                    }
                                                }
                                                else
                                                {
                                                    theCurrRow[currField] = theReader.GetValue(currField);
                                                }
                                            }
                                            catch (Exception ex)
                                            {
                                                if (Logger.IsTracingEnabled)
                                                {
                                                    Logger.OutputToLog(
                                                        TraceOptions.TraceOption.DEBUG,
                                                        TraceOptions.TraceArea.SQLWhiteboard,
                                                        TRACE_SUB_AREA_NAME,
                                                        "Get result exception: " + ex.Message);
                                                }

                                                if (ex is OutOfMemoryException)
                                                {
                                                    throw ex;
                                                }
                                                try
                                                {
                                                    theCurrRow[currField] = theReader.GetString(currField);
                                                    theTable.Columns[currField].DataType = typeof(string);
                                                }
                                                catch (Exception e1)
                                                {
                                                    if (e1 is OutOfMemoryException)
                                                    {
                                                        throw e1;
                                                    }
                                                }
                                            }
                                        }

                                        //Add rows to the result data table
                                        theTable.Rows.Add(theCurrRow);
                                        unprocessedRows++;

                                        //Report progress with the current total number of rows processed
                                        if (unprocessedRows % 50 == 0)
                                        {
                                            if (_progressCallback != null)
                                            {
                                                DataTable tableCopy = theTable.Copy();
                                                _progressCallback(aReportDefinition, i + 1, tableCopy);
                                                unprocessedRows = 0;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        // Break the for loop, the reader has run out of data
                                        break;
                                    }
                                }
                            }
                            else
                            {
                                aReportDefinition.IsGridReport = false;
                                // Only one column is returned, append all rows into a string.
                                if (readReader && (theReader.FieldCount > 0))
                                {
                                    StringBuilder stringBuilder = new StringBuilder();
                                    int i = 0;
                                    while (theReader.Read())
                                    {
                                        stringBuilder.AppendLine(theReader.GetString(0));
                                        //string text = theReader.GetString(0);
                                        i++;
                                        //if ((i + 1) % 2 == 0)
                                        {
                                            if (_progressCallback != null)
                                            {
                                                aReportDefinition.SetProperty(ReportDefinition.ROWS_AFFECTED, stringBuilder.ToString());
                                                //aReportDefinition.SetProperty(ReportDefinition.ROWS_AFFECTED, text + Environment.NewLine);
                                                _progressCallback(aReportDefinition, i + 1, null);
                                                //stringBuilder = new StringBuilder();
                                            }
                                        }
                                    }
                                    aReportDefinition.SetProperty(ReportDefinition.ROWS_AFFECTED, stringBuilder.ToString());

                                    //aReportDefinition.SetProperty(ReportDefinition.ROWS_AFFECTED, "");
                                }
                            }
                        }
                    }
                    else
                    {
                        // This is a Explain command.
                        string errorStr = "";
                        string tableStatsErrorStr = "";
                        NCCQueryPlan theNCCQueryPlan = new NCCQueryPlan(theConnection);
                        _sqlCommandObject.NCCQueryPlan = theNCCQueryPlan;

                        object fetchTableStatsProp = aReportDefinition.GetProperty(ReportDefinition.FETCH_TABLE_STATS);
                        bool fetchTableStats = false;

                        if (fetchTableStatsProp != null)
                        {
                            fetchTableStats = (bool)fetchTableStatsProp;
                        }
                        NCCWorkbenchQueryData wbqd = theNCCQueryPlan.GetExplainPlan(aStatement, out errorStr, out tableStatsErrorStr, fetchTableStats);
                        if (!String.IsNullOrEmpty(errorStr))
                        {
                            wbqd.Result = NCCWorkbenchQueryData.ExplainResult.Get_Explain_Error;
                        }
                        else if (!String.IsNullOrEmpty(tableStatsErrorStr))
                        {
                            wbqd.Result = NCCWorkbenchQueryData.ExplainResult.Get_TableStats_Error;
                        }

                        aReportDefinition.SetProperty(ReportDefinition.EXPLAIN_PLAN_DATA, wbqd);
                    }

                    //If no errors, persist the params
                    ReportParameterProcessor theReportParameterProcessor = ReportParameterProcessor.Instance;
                    List<ReportParameter> theReportParameters = (List<ReportParameter>)aReportDefinition.GetProperty(ReportDefinition.PARAMETERS);
                    if (theReportParameters.Count > 0)
                    {
                        theReportParameterProcessor.persistReportParams(theReportParameters);

                    }

                    //Send the query results back to the UI thread
                    if (_resultCallback != null)
                    {
                        _resultCallback(aReportDefinition);
                        if (theOperation == ReportDefinition.Operation.Execute)
                        {
                            UpdateSessionMode(aStatement, theCacheConnectionObject);
                        }
                    }
                }
                catch (ThreadAbortException)
                {
                    //Do nothing. The clean up is done in the abort code
                }
                catch (Exception ex)
                {
                    if (Logger.IsTracingEnabled)
                    {
                        Logger.OutputToLog(
                            TraceOptions.TraceOption.DEBUG,
                            TraceOptions.TraceArea.SQLWhiteboard,
                            TRACE_SUB_AREA_NAME,
                            "Whiteboard thread proc exception: " + ex.Message);
                    }

                    if (ex is OutOfMemoryException)
                    {
                        aReportDefinition.ResetDataTable();
                    }
                    string errorMessage = ex.Message;

                    // Got an ODBC connection.
                    // Always close the connection if there is one "08S01" 
                    if (errorMessage.Contains(_error08S01))
                    {
                        // ERROR [08S01]: The communication link between the driver and the data source to which
                        // the driver was attempting to connect failed before the function completed processing.
                        if (theConnection != null)
                        {
                            theConnection.Close();
                        }

                        if (theOdbcCommand != null)
                        {
                            theOdbcCommand.Dispose();
                        }

                        // Clean up cache
                        _activeCacheConnectionObjects.Remove(_sqlCommandObject.TheConnectionDefinition.Name);
                        errorMessage = Properties.Resources.WhiteboardConnectionDisappearedMessage;
                    }

                    //Report the exception to the UI thread
                    if (_exceptionCallback != null)
                    {
                        if (_thread.ThreadState == ThreadState.Running || _thread.ThreadState == ThreadState.Background)
                        {
                            _exceptionCallback(aReportDefinition, ex);
                        }
                    }
                }
                finally
                {
                    if (this._closeConnectionDuringAbort)
                    {
                        if (theReader != null)
                        {
                            theReader.Close();
                            theReader.Dispose();
                        }
                    }
                    // Donot dispose command since we're now reusing the command object.
                }
            }
        }

        /// <summary>
        /// Get the columns for the grid datatable by reading the result set metadata
        /// You get the column name, type, precision and scale
        /// Will not work if result set has interval columns
        /// </summary>
        /// <param name="reader"></param>
        /// <param name="gridDataTable"></param>
        void GetColumnsFromResultSetMetatData(OdbcDataReader reader, ref DataTable gridDataTable)
        {
            DataTable schemaTable = reader.GetSchemaTable();
            gridDataTable.Columns.Clear();

            foreach (DataRow row in schemaTable.Rows)
            {
                string colName = row["ColumnName"] as string;
                Type type = (Type)row["DataType"];
                short precision = (short)row["NumericPrecision"];
                short scale = (short)row["NumericScale"];

                DataColumn col = new DataColumn();
                if (gridDataTable.Columns.Contains(colName))
                {
                    col.Caption = colName;
                }
                else
                {
                    col.ColumnName = colName;
                }
                col.DataType = (type != null) ? type : typeof(System.Object);
                if (type == typeof(System.DateTime))
                {
                    if (precision == 10)
                    {
                        col.ExtendedProperties.Add("SQL_TYPE", "DATE");
                    }
                    else
                    {
                        col.ExtendedProperties.Add("SQL_TYPE", "TIMESTAMP");
                        col.ExtendedProperties.Add("SCALE", scale);
                    }
                }
                if (type == typeof(System.TimeSpan))
                {
                    col.ExtendedProperties.Add("SQL_TYPE", "TIME");
                    col.ExtendedProperties.Add("SCALE", scale);
                }
                gridDataTable.Columns.Add(col);
            }
        }

        /// <summary>
        /// This is a fallback approach to create the columns of the grid data table
        /// Iterate through the result set column set and create a data column
        /// using the result column's name and datatype
        /// If an exception is thrown like for interval datatypes, treat it as a string column
        /// </summary>
        /// <param name="reader"></param>
        /// <param name="gridDataTable"></param>
        void CreateColumnsFromResultSet(OdbcDataReader reader, ref DataTable gridDataTable)
        {
            int theFieldCount = reader.FieldCount;
            for (int colNum = 0; colNum < theFieldCount; colNum++)
            {
                string colName = reader.GetName(colNum);
                try
                {
                    System.Type type = reader.GetFieldType(colNum);
                    string typeName = reader.GetDataTypeName(colNum);
                    if (gridDataTable.Columns.Contains(colName))
                    {
                        DataColumn column = gridDataTable.Columns.Add();
                        column.Caption = colName;
                        column.DataType = type;
                        column.ExtendedProperties.Add("SQL_TYPE", typeName);
                    }
                    else
                    {
                        DataColumn column = gridDataTable.Columns.Add(colName, type);
                        column.ExtendedProperties.Add("SQL_TYPE", typeName);
                    }
                }
                catch (Exception ex)
                {
                    if (Logger.IsTracingEnabled)
                    {
                        Logger.OutputToLog(
                            TraceOptions.TraceOption.DEBUG,
                            TraceOptions.TraceArea.SQLWhiteboard,
                            TRACE_SUB_AREA_NAME,
                            "Create columns from resultset exception: " + ex.Message);
                    }

                    if (gridDataTable.Columns.Contains(colName))
                    {
                        DataColumn column = gridDataTable.Columns.Add();
                        column.Caption = colName;
                    }
                    else
                    {
                        gridDataTable.Columns.Add(new DataColumn(colName));
                    }
                }
            }
        }

    }

    // Delegate that defines the signature for the callback method.
    //
    public delegate void UpdateProgress(ReportDefinition reportDefinition, int rowCount, DataTable processedData);
    public delegate void UpdatePage(ReportDefinition reportDefinition, int pageNumber, int processedRows);
    public delegate void UpdateResult(ReportDefinition reportDefinition);
    public delegate void UpdateMessage(ReportDefinition reportDefinition, Exception ex);

    #endregion ThreadWithState class
}
