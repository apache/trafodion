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
using System.Data;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Connections.Controls;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.MetricMiner.Controls
{
    public partial class QueryDesigner : UserControl
    {
        public  string[] NumericFunctions = new string[] {    "ABS"
                                                    , "ACOS"
                                                    , "ASIN"
                                                    , "ATAN"
                                                    , "ATAN2"
                                                    , "CEILING"
                                                    , "COS"
                                                    , "COSH"
                                                    , "DEGREES"
                                                    , "EXP"
                                                    , "FLOOR"
                                                    , "LOG"
                                                    , "LOG10"
                                                    , "MOD"
                                                    , "PI"
                                                    , "POWER"
                                                    , "RADIANS"
                                                    , "RANDSIGN"
                                                    , "SIN"
                                                    , "SINH"
                                                    , "SORT"
                                                    , "TAN"
                                                    , "TANH"};

        public  string[] StringFunctions = new string[]{     "ASCII"
                                                    , "CHAR"
                                                    , "CHAR_LENGTH"
                                                    , "CONCAT"
                                                    , "INSERT"
                                                    , "LCASE"
                                                    , "LEFT"
                                                    , "LOCATE"
                                                    , "LOWER"
                                                    , "LPAD"
                                                    , "LTRIM"
                                                    , "OCTET_LENGTHPOSITION"
                                                    , "REPEAT"
                                                    , "REPLACE"
                                                    , "RIGHT"
                                                    , "RPAD"
                                                    , "RTRIM"
                                                    , "SPACE"
                                                    , "SUBSTRING"
                                                    , "TRIM"
                                                    , "UCASE"};

        public  string[] SystemFunctions = new string[]{  "CURRENT_USER"
                                                   , "USER"};

        public  string[] TimeDateFunctions = new string[]{   "CONVERTTIMESTAMP"
                                                    , "CURRENT"
                                                    , "CURRENT_DATE"
                                                    , "CURRENT_TIME"
                                                    , "CURRENT_TIMESTAMPDATEFORMAT"
                                                    , "DAY"
                                                    , "DAYNAME"
                                                    , "DAYOFMONTH"
                                                    , "DAYOFWEEK"
                                                    , "DAYOFYEAR"
                                                    , "EXTRACT"
                                                    , "HOUR"
                                                    , "JULIANTIMESTAMP"
                                                    , "MINUTEMONTH"
                                                    , "MONTHNAME"
                                                    , "QUARTER"
                                                    , "SECOND"
                                                    , "WEEK"
                                                    , "YEAR"};

        public string[] FunctionGroups = new string[]{    "Numeric Functions"
                                                    , "String Functions"
                                                    , "System Functions"
                                                    , "Time/Date Functions"};


        public string[] DDLStatements = new String[] { "ALTER INDEX"
                                                    , "ALTER SQLMP ALIAS"
                                                    , "ALTER TABLE"
                                                    , "ALTER TRIGGER"
                                                    , "CREATE CATALOG"
                                                    , "CREATE INDEX"
                                                    , "CREATE PROCEDURE"
                                                    , "CREATE SCHEMA"
                                                    , "CREATE SQLMP ALIAS"
                                                    , "CREATE TABLE"
                                                    , "CREATE TRIGGER"
                                                    , "CREATE VIEW" 
                                                    , "DROP CATALOG"
                                                    , "DROP INDEX"
                                                    , "DROP PROCEDURE"
                                                    , "DROP SCHEMA"
                                                    , "DROP SQL" 
                                                    , "DROP SQLMP ALIAS"
                                                    , "DROP TABLE"
                                                    , "DROP TRIGGER"
                                                    , "DROP VIEW"
                                                    , "GRANT"
                                                    , "GRANT EXECUTE"
                                                    , "INITIALIZE SQL"
                                                    , "REVOKE"
                                                    , "REVOKE EXECUTE"
                                                    , "SET"
                                                    , "SET CATALOG"
                                                    , "SET MPLOC"
                                                    , "SET NAMETYPE"
                                                    , "SET SCHEMA"
                                                    , "SIGNAL SQLSTATE" };

        public string[] DMLStatements = new String[] { "CALL"
            , "DELETE"
            , "EXECUTE"
            , "INSERT"
            , "PREPARE"
            , "SELECT"
            , "UPDATE"};
        public string[] TransactionStatements = new String[] { "BEGIN WORK"
            , "COMMIT WORK"
            , "ROLLBACK WORK"
            , "SET TRANSACTION"};
        public string[] ControlStatements = new String[] { "CONTROL QUERY DEFAULT"
            , "CONTROL QUERY SHAPE"
            , "CONTROL TABLE"
            , "SET TABLE TIMEOUT"};

        public string[] StatementGroups = new string[]{ "CQD Statements"
                                                    ,"DDL Statements"
                                                    , "DML Statements"
                                                    , "Transaction Statements"
                                                    };

        Dictionary<string, string[]> Functions = new Dictionary<string, string[]>();
        Dictionary<string, string[]> Statements = new Dictionary<string, string[]>();
        private ConnectionDefinition _theSelectedConnectionDefinition = null;
        private const string tablePrefix = "Table - ";
        private const string viewPrefix = "View - ";


        public QueryDesigner()
        {
            InitializeComponent();
            loadFunctions();
            populateFunctionsCombo();
            loadStatements();
            populateStatementsCombo();

            _theSystemsCombo.SelectedIndexChanged += new EventHandler(_theSystemsCombo_SelectedIndexChanged);
            _theCatalogsComboBox.SelectedIndexChanged += new EventHandler(_theCatalogsComboBox_SelectedIndexChanged);
            _theSchemasComboBox.SelectedIndexChanged += new EventHandler(_theSchemasComboBox_SelectedIndexChanged);
           // _theSchemasComboBox.SelectedValueChanged += new EventHandler(_theSchemasComboBox_SelectedValueChanged);

            if (_theSystemsCombo.Items.Count > 0)
            {
                if (TrafodionContext.Instance.CurrentConnectionDefinition != null)
                {
                    if (TrafodionContext.Instance.CurrentConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
                    {
                        _theSystemsCombo.SelectedConnectionDefinition = TrafodionContext.Instance.CurrentConnectionDefinition;
                        SelectConnectionDefinition(_theSystemsCombo.SelectedConnectionDefinition);
                    }
                    else
                    {
                        _theSystemsCombo.SelectedIndex = -1;
                    }
                }
                else
                {
                    _theSystemsCombo.SelectedIndex = -1;
                }
            }


        }

        void _theSystemsCombo_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (!_theSystemsCombo.IsLoading)
            {
                _theSelectedConnectionDefinition = _theSystemsCombo.SelectedConnectionDefinition;
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
                                _theCatalogsComboBox.SelectedTrafodionCatalog = _theCatalogsComboBox.TheTrafodionSystem.FindCatalog(_theSystemsCombo.SelectedCatalogName);
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

                            if (_theSystemsCombo.SelectedSchemaName != null)
                            {
                                if (!String.IsNullOrEmpty(_theSystemsCombo.SelectedSchemaName))
                                {
                                    try
                                    {
                                        _theSchemasComboBox.SelectedTrafodionSchema = _theCatalogsComboBox.SelectedTrafodionCatalog.FindSchema(_theSystemsCombo.SelectedSchemaName);
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
                        //TrafodionContext.Instance.OnConnectionDefinitionSelection(this);
                    }
                    else
                    {
                        _theSystemsCombo.SelectedIndex = -1;
                        _theCatalogsComboBox.TheTrafodionSystem = null;
                        _theSchemasComboBox.TheTrafodionCatalog = null;
                        //Clear table list and column list
                        _theColumnsBox.Items.Clear();
                        _theTablesBox.Items.Clear();
                        //SetSystemDescription(null);
                    }
                }
                else
                {
                    //SetSystemDescription(null);
                }
            }

        }


        public void SelectConnectionDefinition(ConnectionDefinition aConnectionDefinition)
        {
            _theSelectedConnectionDefinition = aConnectionDefinition;

            bool showCatalogs = (_theSelectedConnectionDefinition != null);
            //_theCatalogsComboBox.Visible = showCatalogs;
            //_theCatalogsLabel.Visible = showCatalogs;           

            if (_theSelectedConnectionDefinition != null)
            {
                if (_theSelectedConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
                {
                    _theSystemsCombo.SelectedConnectionDefinition = _theSelectedConnectionDefinition;
                    _theCatalogsComboBox.TheTrafodionSystem = TrafodionSystem.FindTrafodionSystem(_theSelectedConnectionDefinition);

                    try
                    {
                        _theCatalogsComboBox.SelectedTrafodionCatalog = _theCatalogsComboBox.TheTrafodionSystem.FindCatalog(_theSystemsCombo.SelectedCatalogName);
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

                    if (_theSystemsCombo.SelectedSchemaName != null)
                    {
                        if (!String.IsNullOrEmpty(_theSystemsCombo.SelectedSchemaName))
                        {
                            try
                            {
                                _theSchemasComboBox.SelectedTrafodionSchema = _theCatalogsComboBox.SelectedTrafodionCatalog.FindSchema(_theSystemsCombo.SelectedSchemaName);
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
                    }

                }
            }
        }

        public string QueryText
        {
            get { return _theQueryTextBox.Text; }
            set { _theQueryTextBox.Text = value;}
        }
        void _theSchemasComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            loadTablesList();
        }


        //void _theSchemasComboBox_SelectedValueChanged(object sender, EventArgs e)
        //{
        //    loadTablesList();
        //}

        void _theCatalogsComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            //Clear table/Column list
            _theTablesBox.Items.Clear();
            _theColumnsBox.Items.Clear();
            _theSchemasComboBox.SelectedIndex = -1;
            _theSchemasComboBox.TheTrafodionCatalog = _theCatalogsComboBox.SelectedTrafodionCatalog;
            
        }

        public ConnectionDefinition TheSelectedConnectionDefinition
        {
            get { return _theSystemsCombo.SelectedConnectionDefinition; }
            set
            {
                _theSystemsCombo.SelectedConnectionDefinition = value;
                SelectConnectionDefinition(value);
            }
        }
        private void loadFunctions()
        {
            Functions.Add(FunctionGroups[0], NumericFunctions);
            Functions.Add(FunctionGroups[1], StringFunctions);
            Functions.Add(FunctionGroups[2], SystemFunctions);
            Functions.Add(FunctionGroups[3], TimeDateFunctions);
        }

        private void loadStatements()
        {
            Statements.Add(StatementGroups[0], ControlStatements);
            Statements.Add(StatementGroups[1], DDLStatements);
            Statements.Add(StatementGroups[2], DMLStatements);
            Statements.Add(StatementGroups[3], TransactionStatements);
        }

        private void populateFunctionsCombo()
        {
            _theFunctionsComboBox.Items.AddRange(FunctionGroups);
            _theFunctionsComboBox.SelectedIndex = 0;
        }


        private void populateStatementsCombo()
        {
            this._theStatementsCombo.Items.AddRange(StatementGroups);
            _theStatementsCombo.SelectedIndex = 2;
        }

        private void loadTablesList()
        {
            //Load new table should clear columns info first.
            _theColumnsBox.Items.Clear();
            _theTablesBox.Items.Clear();
            _theTablesBox.Items.Add("Loading Table List...");
            string[] tables=null;
            if (_theSchemasComboBox.SelectedTrafodionSchema != null)
            {
                try
                {
                    tables = this.getTablesForSchema(_theCatalogsComboBox.SelectedTrafodionCatalog.ExternalName, _theSchemasComboBox.SelectedTrafodionSchema.ExternalName);
                }
                catch
                {
                    MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.ErrorOutofSyncOfDBObjects, "Error Loading Table(s)",
                        MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
            _theTablesBox.Items.Clear();
            if (tables != null)
            {
                _theTablesBox.Items.AddRange(tables);
            }
        }


        private void loadColumnList()
        {
            _theColumnsBox.Items.Clear();
            _theColumnsBox.Items.Add("Loading Column List...");
            string[] columns = null;
            try
            {
                columns = this.getColumnsForTable(_theCatalogsComboBox.SelectedTrafodionCatalog.ExternalName, _theSchemasComboBox.SelectedTrafodionSchema.ExternalName, _theTablesBox.SelectedItem as string);
            }
            catch
            {
                MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.ErrorOutofSyncOfDBObjects, "Error Loading Column(s)",
                MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            _theColumnsBox.Items.Clear();
            if (columns != null)
            {
                _theColumnsBox.Items.AddRange(columns);
            }
        }


        private void _theCommandsbox_MouseDoubleClick(object sender, MouseEventArgs e)
        {

        }

        private void _theTablesBox_MouseDoubleClick(object sender, MouseEventArgs e)
        {
            string selectedItem = _theTablesBox.SelectedItem as string;
            if (selectedItem!=null)
            _theQueryTextBox.AppendText(getTableViewName(selectedItem));
            _theQueryTextBox.Focus();
        }

        private string getTableViewName(string aName)
        {
            string aObjectName = aName;
            if (aName.StartsWith(tablePrefix))
            {
                aObjectName = aName.Substring(tablePrefix.Length);
            }
            else if (aName.StartsWith(viewPrefix))
            {
                aObjectName = aName.Substring(viewPrefix.Length);
            }
            return TrafodionName.InternalForm(aObjectName);
        }

        private void _theTablesBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            loadColumnList();
        }

        private void _theColumnsBox_MouseDoubleClick(object sender, MouseEventArgs e)
        {
            string selectedItem = _theColumnsBox.SelectedItem as string;
            if (selectedItem!=null)
            _theQueryTextBox.AppendText(selectedItem);
            _theQueryTextBox.Focus();

        }

        private void _theFunctionsComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            string functionSelected = (String)_theFunctionsComboBox.SelectedItem;
            _theCommandsBox.Items.Clear();
            string[] functions = Functions[functionSelected];
            _theCommandsBox.Items.AddRange(functions);
        }

        private void _theCommandsBox_MouseDoubleClick_1(object sender, MouseEventArgs e)
        {
            string selectedItem = _theCommandsBox.SelectedItem as string;
            _theQueryTextBox.AppendText(selectedItem);
            _theQueryTextBox.Focus();
        }

        private string[] getTablesForSchema(string catalog, string schema)
        {
            string[] ret = null;
            try
            {
                ConnectionDefinition aConnectionDefinition = TheSelectedConnectionDefinition;
                //Load the SQL model for the Alerts View and look up the privileges
                TrafodionSystem aTrafodionSystem = TrafodionSystem.FindTrafodionSystem(aConnectionDefinition);

                TrafodionCatalog aTrafodionCatalog = null;
                TrafodionSchema aTrafodionSchema = null;
                TrafodionView alertsView = null;

                if (TrafodionObject.Exists<TrafodionCatalog>(aTrafodionSystem.TrafodionCatalogs, catalog))
                {
                    aTrafodionCatalog = aTrafodionSystem.FindCatalog(catalog);
                }
                else
                {
                    aTrafodionCatalog = aTrafodionSystem.LoadTrafodionCatalog(catalog);
                }

                if (aTrafodionCatalog != null)
                {
                    if (TrafodionObject.Exists<TrafodionSchema>(aTrafodionCatalog.TrafodionSchemas, schema))
                    {
                        aTrafodionSchema = aTrafodionCatalog.FindSchema(schema);
                    }
                    else
                    {
                        aTrafodionSchema = aTrafodionCatalog.LoadTrafodionSchema(schema);
                    }
                    if (aTrafodionSchema != null)
                    {
                        List<TrafodionTable> tables =  aTrafodionSchema.TrafodionTables;
                        List<TrafodionView> views = aTrafodionSchema.TrafodionViews;

                        ret = new string[tables.Count + views.Count];
                        int i = 0;
                        foreach(TrafodionTable table in tables)
                        {
                            ret[i++] = tablePrefix + table.ExternalName;
                        }

                        foreach (TrafodionView view in views)
                        {
                            ret[i++] = viewPrefix + view.ExternalName;
                        }
                    }
                }
            }
            catch
            {
                throw;
            }
            return ret;
        }


        private string[] getColumnsForTable(string catalog, string schema, string objectName)
        {
            string[] ret = null;
            try
            {
                ConnectionDefinition aConnectionDefinition = TheSelectedConnectionDefinition;
                //Load the SQL model for the Alerts View and look up the privileges
                TrafodionSystem aTrafodionSystem = TrafodionSystem.FindTrafodionSystem(aConnectionDefinition);

                TrafodionCatalog aTrafodionCatalog = null;
                TrafodionSchema aTrafodionSchema = null;
                

                if (TrafodionObject.Exists<TrafodionCatalog>(aTrafodionSystem.TrafodionCatalogs, catalog))
                {
                    aTrafodionCatalog = aTrafodionSystem.FindCatalog(catalog);
                }
                else
                {
                    aTrafodionCatalog = aTrafodionSystem.LoadTrafodionCatalog(catalog);
                }

                if (aTrafodionCatalog != null)
                {
                    if (TrafodionObject.Exists<TrafodionSchema>(aTrafodionCatalog.TrafodionSchemas, schema))
                    {
                        aTrafodionSchema = aTrafodionCatalog.FindSchema(schema);
                    }
                    else
                    {
                        aTrafodionSchema = aTrafodionCatalog.LoadTrafodionSchema(schema);
                    }
                    if (aTrafodionSchema != null)
                    {
                        List<TrafodionColumn> cols = null;
                        
                        if (objectName.StartsWith(tablePrefix))
                        {
                            string tableName = getTableViewName(objectName);
                            TrafodionTable aTable = null;
                            List<TrafodionTable> tables = aTrafodionSchema.TrafodionTables;
                            if (TrafodionObject.Exists<TrafodionTable>(aTrafodionSchema.TrafodionTables, tableName))
                            {
                                aTable = aTrafodionSchema.FindTable(tableName);
                            }
                            else
                            {
                                aTable = aTrafodionSchema.LoadTableByName(tableName);
                            }

                            if (aTable != null)
                            {
                                cols = aTable.Columns;
                            }
                        }
                        else if (objectName.StartsWith(viewPrefix))
                        {
                            string viewName = getTableViewName(objectName);
                            TrafodionView aView = null;
                            List<TrafodionView> views = aTrafodionSchema.TrafodionViews;
                            if (TrafodionObject.Exists<TrafodionView>(aTrafodionSchema.TrafodionViews, viewName))
                            {
                                aView = aTrafodionSchema.FindView(viewName);
                            }
                            else
                            {
                                aView = aTrafodionSchema.LoadViewByName(viewName);
                            }

                            if (aView != null)
                            {
                                cols = aView.Columns;
                            }
                        }


                        if (cols != null)
                        {
                            ret = new String[cols.Count];
                            int i = 0;
                            foreach (TrafodionColumn col in cols)
                            {
                                ret[i++] = col.ExternalName;
                            }
                        }

                    }
                }
            }
            catch
            {
                throw;
            }
            return ret;
        }

        private void _theStatementsCombo_SelectedIndexChanged(object sender, EventArgs e)
        {
            string statementSelected = (String)this._theStatementsCombo.SelectedItem;
            this._theStatementList.Items.Clear();
            string[] statements = Statements[statementSelected];
            _theStatementList.Items.AddRange(statements);

        }

        private void _theStatementList_MouseDoubleClick(object sender, MouseEventArgs e)
        {
            string selectedItem = this._theStatementList.SelectedItem as string;
            _theQueryTextBox.AppendText(selectedItem + " " );
            _theQueryTextBox.Focus();

        }
    }
}
