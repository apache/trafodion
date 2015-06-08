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
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// User control that lets user create a procedure
    /// </summary>
    public partial class CreateSPJ : TrafodionForm
    {
        ProcedureParametersDataGridView _parametersGrid = null;
        SqlMxProcedure _sqlMxProcedure = null;

        /// <summary>
        /// Construct the user control and identify in which schema the procedure would be created
        /// </summary>
        /// <param name="aSqlMxSchema"></param>
        public CreateSPJ(SqlMxSchema aSqlMxSchema)
        {
            InitializeComponent();
            _theCatalogName.Text = aSqlMxSchema.TheSqlMxCatalog.ExternalName;
            _theSchemaName.Text = aSqlMxSchema.ExternalName;
            if (aSqlMxSchema.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ120)
            {
                _theExternalSecurityGroupBox.Visible = true;
            }
            else
            {
                _theExternalSecurityGroupBox.Visible = false;
            }
            _bannerControl.ConnectionDefinition = aSqlMxSchema.ConnectionDefinition;

            try
            {
                Cursor = Cursors.WaitCursor;
                //Initialize a new procedure model
                _sqlMxProcedure = new SqlMxProcedure(aSqlMxSchema, "", 0, 0, 0, "", "");
                _theSPJName.MaxLength = SqlMxName.MaximumNameLength;

                //Initialize the parameters grid
                _parametersGrid = new ProcedureParametersDataGridView(null, _sqlMxProcedure);
                _parametersGrid.Dock = DockStyle.Fill;
                gridPanel.Controls.Add(_parametersGrid);
                _parametersGrid.DoubleClick += new EventHandler(_parametersGrid_DoubleClick);
            }
            finally
            {
                Cursor = Cursors.Default;
            }
            CenterToScreen();
            UpdateControls();
        }

        /// <summary>
        /// If user double-clicks on a parameter in the parameters grid, launch the Edit parameter UI
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void _parametersGrid_DoubleClick(object sender, EventArgs e)
        {
            EditParameter();
        }

        /// <summary>
        /// Handle the create button click and create the procedure
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theCreateButton_Click(object sender, EventArgs e)
        {
            Cursor = Cursors.WaitCursor;

            //Check if controls have valid data
            if (ValidateControls())
            {
                //Update the procedure model with information from the UI controls
                UpdateModel();

                try
                {
                    //Ask the procecure model to create the procedure.
                    _sqlMxProcedure.Create();

                    //Display a success message
                    MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), string.Format(Properties.Resources.ProcedureCreateSuccess, _sqlMxProcedure.ExternalName),
                        Properties.Resources.CreateProcedure, MessageBoxButtons.OK, MessageBoxIcon.Information);

                    //We are done. Close the dialog
                    Close();
                }
                catch (Exception ex)
                {
                    MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), ex.Message,
                        Properties.Resources.CreateProcedure, MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
                finally
                {
                    Cursor = Cursors.Default;
                }
            }
            Cursor = Cursors.Default;
        }

        /// <summary>
        /// Validate the data in the controls and set an appropriate error message
        /// </summary>
        /// <returns></returns>
        private bool ValidateControls()
        {
            StringBuilder validationErrorMessageHolder = new StringBuilder();
            if (!SqlMxName.Validate(this._theSPJName.Text.Trim(), validationErrorMessageHolder))
            {
                _errorText.Text = validationErrorMessageHolder.ToString();
                if (_theSPJName.CanFocus)
                {
                    _theSPJName.Select();
                }
                return false;
            }
            return true;
        }

        /// <summary>
        /// Update the state of the controls
        /// </summary>
        private void UpdateControls()
        {
            bool isValid = false;
            StringBuilder errorHolder = new StringBuilder();
            isValid = SqlMxName.Validate(_theSPJName.Text, errorHolder);

            //Disable the browse button if name is empty
            _theFileBrowserButton.Enabled = isValid;

            if (isValid)
            {
                isValid = String.IsNullOrEmpty(_theJarFileName.Text.Trim()) ? false : true;
                _errorText.Text = "";
                if (isValid)
                {
                    isValid = String.IsNullOrEmpty(_methodNameTextBox.Text.Trim()) ? false : true;
                    _errorText.Text = "";
                }
                else
                {
                    _errorText.Text = Properties.Resources.SelectProcedureCodeFile;
                }
            }
            else
            {
                _errorText.Text = errorHolder.ToString();
            }

            //Create button is only enabled if name, code file details are present
            this._theCreateButton.Enabled = isValid;
        }

        /// <summary>
        /// Update the procedure model with information from the UI controls
        /// </summary>
        private void UpdateModel()
        {
            _sqlMxProcedure.ExternalName = _theSPJName.Text.Trim();
            _sqlMxProcedure.SQLAccess = _theAccessesDatabase.Checked ? SqlMxProcedure.MODIFIES_SQL_DATA_KEY : SqlMxProcedure.NO_SQL_KEY;
            _sqlMxProcedure.MaxResults = (int)_theDynamicResultSets.Value;
            _sqlMxProcedure.ExternalSecurity = _theDefinerRadioButton.Checked ? "D" : "I";
        }   

        /// <summary>
        /// Handle the browse button click and launch the PCF code file tool
        /// After a code file has been selected, populate the parameters grid with the selected
        /// method parameters.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theFileBrowserButton_Click(object sender, EventArgs e)
        {
            try
            {
                PCFBrowserDialog browserDialog = new PCFBrowserDialog(_sqlMxProcedure.ConnectionDefinition);

                if (browserDialog.ShowDialog() == DialogResult.OK)
                {
                    if (browserDialog.SelectedMethod != null && browserDialog.SelectedClassTreeNode != null)
                    {
                        //If user selected a valid class and method, set selected method information in the UI controls
                        CodeFile codeFile = browserDialog.SelectedClassTreeNode.CodeFile;
                        JavaMethod javaMethod = browserDialog.SelectedMethod;

                        _theJarFileName.Text = codeFile.Name;
                        _classNameTextBox.Text = browserDialog.SelectedClassTreeNode.ShortDescription;
                        _methodNameTextBox.Text = javaMethod.MethodName;

                        _sqlMxProcedure.ExternalName = _theSPJName.Text.Trim();
                        _sqlMxProcedure.ExternalPath = codeFile.FullyQualifiedPath;
                        _sqlMxProcedure.ExternalClassFileName = _classNameTextBox.Text;
                        _sqlMxProcedure.ExternalMethodName = _methodNameTextBox.Text;
                        _sqlMxProcedure.SignatureText = javaMethod.MethodSignature;
                        _sqlMxProcedure.Columns.Clear();

                        bool containsResultSet = false;

                        List<JavaParameter> parameters = javaMethod.Parameters;
                        _editButton.Enabled = (parameters.Count > 0);

                        //Construct a SqlMxProcedureColumn model for each of the method parameters
                        foreach (JavaParameter theParam in parameters)
                        {
                            SqlMxProcedureColumn column = new SqlMxProcedureColumn();
                            column.ExternalName = theParam.Name;
                            column.TheDirection = theParam.Direction;
                            column.TheSQLDataType = theParam.SqlDataType;
                            column.JavaDataType = theParam.JavaDataType;
                            column.TheColumnSize = theParam.DefaultSize;
                            column.TheColPrecision = theParam.DefaultSize;
                            column.TheDatetimeTrailingPrecision = theParam.DefaultSize;
                            column.TheCharacterSet = DataTypeHelper.CHARSET_ISO88591;
                            if (column.JavaDataType.Contains(DataTypeHelper.JAVA_RESULTSET))
                            {
                                containsResultSet = true;
                            }
                            column.SqlMxSchemaObject = _sqlMxProcedure;
                            _sqlMxProcedure.Columns.Add(column);
                        }

                        //Reload the parameters grid everytime
                        _parametersGrid.Reload(_sqlMxProcedure);

                        _theDynamicResultSets.Enabled = containsResultSet;
                        _theDynamicResultSets.Minimum = containsResultSet ? 1 : 0;

                        //Update the control states
                        UpdateControls();
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), 
                    string.Format(Properties.Resources.ErrorSelectingProcedureMethod, "\n\n", ex.Message),
                    Properties.Resources.Error, MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        /// <summary>
        /// Edit a parameter and change its name, sql type and attributes
        /// </summary>
        private void EditParameter()
        {
            if (_parametersGrid != null && _parametersGrid.Rows.Count > 0 && _parametersGrid.SelectedRows.Count > 0)
            {
                //First column of the parameters grid the Procedure column model
                SqlMxProcedureColumn column = _parametersGrid.SelectedRows[0].Cells[0].Value as SqlMxProcedureColumn;
                try
                {
                    if (column != null)
                    {
                        EditParameterDialog paramDialog = new EditParameterDialog(column);
                        if (paramDialog.ShowDialog() == DialogResult.OK)
                        {
                            //Reflect the changes done to the procedure column model in the parameters grid.
                            _parametersGrid.UpdateRow(_parametersGrid.SelectedRows[0].Index, column);
                            _parametersGrid.Invalidate();
                        }
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                        string.Format(Properties.Resources.EditColumnFailed, 
                            new string[]{column.ExternalName, "\n\n", ex.Message}),
                        Properties.Resources.Error, MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
        }

        /// <summary>
        /// Closes the parent
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theCancelButton_Click(object sender, EventArgs e)
        {
            Close();
        }

        /// <summary>
        /// Edits the parameter
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _editButton_Click(object sender, EventArgs e)
        {
            EditParameter();
        }

        /// <summary>
        /// Store the dynamic resultsets in the model
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theDynamicResultSets_ValueChanged(object sender, EventArgs e)
        {
            if (_sqlMxProcedure != null)
            {
                _sqlMxProcedure.MaxResults = (int)_theDynamicResultSets.Value;
            }
        }

        /// <summary>
        /// Store the access database property in the model
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theAccessesDatabase_Click(object sender, EventArgs e)
        {
            _sqlMxProcedure.SQLAccess = _theAccessesDatabase.Checked ? SqlMxProcedure.MODIFIES_SQL_DATA_KEY : SqlMxProcedure.NO_SQL_KEY;
            if (_theAccessesDatabase.Checked)
            {
                _theExternalSecurityGroupBox.Enabled = true;
            }
            else
            {
                _theExternalSecurityGroupBox.Enabled = false;
                _theInvokerRadioButton.Checked = true;
            }
        }

        /// <summary>
        /// Name is a mandatory field.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theSPJName_TextChanged(object sender, EventArgs e)
        {
            UpdateControls();
        }

        #region IMainToolBarConsumer implementation

        /// <summary>
        /// Implementating the IMainToolBarConsumer interface, which the consumer could elect buttons to show and modify 
        /// the Help button to invoke context sensitive help topic.
        /// </summary>
        /// <param name="aMainToolBar"></param>
        public void CustomizeMainToolBarItems(Trafodion.Manager.Framework.MainToolBar aMainToolBar)
        {
            // Now, turn on all of the tool strip buttons for PCFTool
            aMainToolBar.TheSystemToolToolStripItem.Visible = true;
            aMainToolBar.TheSystemsToolStripSeparator.Visible = true;
            aMainToolBar.TheSQLWhiteboardToolStripItem.Visible = true;
            aMainToolBar.TheNCIToolStripItem.Visible = true;
            aMainToolBar.TheMetricMinerToolStripItem.Visible = true;
            aMainToolBar.TheOptionsToolStripItem.Visible = true;
            aMainToolBar.TheToolsStripSeparator.Visible = true;
            aMainToolBar.TheWindowManagerToolStripItem.Visible = true;
            aMainToolBar.TheWindowManagerStripSeparator.Visible = true;
            aMainToolBar.TheHelpToolStripItem.Visible = true;

            ///Customize the help topic if it is desired.
            aMainToolBar.UnRegisterDefaultHelpEventHandler();
            aMainToolBar.TheHelpToolStripItem.Alignment = ToolStripItemAlignment.Right;
            aMainToolBar.TheHelpToolStripItem.Click += new EventHandler(TheHelpToolStripItem_Click);
        }

        /// <summary>
        /// The event handler for the context sensitive 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void TheHelpToolStripItem_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.UseCreateProcDialog);
        }

        #endregion 

        private void _helpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.UseCreateProcDialog);
        }
    }
}
