#region Copyright info
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2014-2015 Hewlett-Packard Development Company, L.P.
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
#endregion Copyright info

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Data.Odbc;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    public partial class RunScriptDialog : TrafodionForm
    {
        private System.ComponentModel.BackgroundWorker _backgroundWorker;
        private String LOADSCRIPTS = "LoadScripts";
        private String RUNSCRIPT = "RunScript";
        private string _strSelectedNodeID = string.Empty;
        private bool _blnStandardOutError = false;
        private string _scriptName = string.Empty;
        private ConnectionDefinition _connectionDefinition;

        public ConnectionDefinition ConnectionDefinition
        {
            get { return _connectionDefinition; }
            set
            {
                _connectionDefinition = value;
            }
        }

        private string WindowTitle
        {
            get
            {
                string systemIdentifier = (_connectionDefinition != null) ? _connectionDefinition.Name + " : " : "";
                string scriptName = _scriptName.Equals(string.Empty) ? "" : " - "+_scriptName;
                return TrafodionForm.TitlePrefix + systemIdentifier + Properties.Resources.RunScriptDialogTitle + scriptName;
            }
        }

        public RunScriptDialog(ConnectionDefinition aConnectionDefn)
        {
            InitializeComponent();
            _connectionDefinition = aConnectionDefn;
            TrafodionBannerControl1.ConnectionDefinition = aConnectionDefn;
            InitializeBackgoundWorker();
            CenterToScreen();
            this.Text = WindowTitle;
            ConnectionDefinition.Changed += ConnectionDefinition_Changed;
            DisplayStatusBar("Loading available script names...");
            _backgroundWorker.RunWorkerAsync(new String[] { LOADSCRIPTS });
            runscriptTooltips.SetToolTip(_theStandardOutErrorCheckBox, Properties.Resources.RunScriptTooltipsStandardOut);
        }

        void ConnectionDefinition_Changed(object aSender, ConnectionDefinition aConnectionDefinition, ConnectionDefinition.Reason aReason)
        {
            UpdateControls();
        }

 
       private delegate void UpdateControlsDelegate();
       private void UpdateControls()
       {
           if (InvokeRequired)
           {
               Invoke(new UpdateControlsDelegate(UpdateControls), new object[0]);
           }
           else
           {
               if (ConnectionDefinition.TheState != ConnectionDefinition.State.TestSucceeded)
               {
                   _executeButton.Enabled = false;
                   _lblErrorConnection.Text = Properties.Resources.RunScriptDisconnectedError;
               }
               else
               {
                   _executeButton.Enabled = true;
                   _lblErrorConnection.Text = string.Empty;
               }
           }
       }

        /// <summary>
        /// Set up the BackgroundWorker object by attaching event handlers. 
        /// </summary>
        private void InitializeBackgoundWorker()
        {
            _backgroundWorker = new System.ComponentModel.BackgroundWorker();
            _backgroundWorker.WorkerReportsProgress = true;
            _backgroundWorker.WorkerSupportsCancellation = true;
            _backgroundWorker.DoWork += new DoWorkEventHandler(BackgroundWorker_DoWork);
            _backgroundWorker.RunWorkerCompleted +=
                new RunWorkerCompletedEventHandler(BackgroundWorker_RunWorkerCompleted);
            _backgroundWorker.ProgressChanged += new ProgressChangedEventHandler(_backgroundWorker_ProgressChanged);
        }

        void _backgroundWorker_ProgressChanged(object sender, ProgressChangedEventArgs e)
        {
            _outputTextBox.AppendText(e.UserState as String);
        }

        private void BackgroundWorker_RunWorkerCompleted(
            object sender, RunWorkerCompletedEventArgs e)
        {
            // First, handle the case where an exception was thrown.
            HideStatusBar();

            if (e.Error != null)
            {
                _outputTextBox.Text = e.Error.Message;
            }
            else if (e.Cancelled)
            {
                // Next, handle the case where the user canceled 
                // the operation.
                // Note that due to a race condition in 
                // the DoWork event handler, the Cancelled
                // flag may not have been set, even though
                // CancelAsync was called.
                _outputTextBox.Text = "----Operation Aborted";
            }
            else
            {
                // Finally, handle the case where the operation 
                // succeeded.
                //_scriptNameComboBox.DataSource = scriptNames;
                Object[] results = (Object[])e.Result;
                if (((String)results[0]).Equals(LOADSCRIPTS))
                {
                    List<string> scriptNames = (List<string>)results[1];
                    scriptNames.Sort();
                    _scriptNameComboBox.DataSource = scriptNames;

                }
                else
                {
                    _outputTextBox.AppendText(results[1] as String);
                }
            }
            _executeButton.Enabled = true;
        }

        private void DisplayStatusBar(String message)
        {
            toolStripStatusLabel.Text = message;
            toolStripProgressBar.Visible = toolStripStatusLabel.Visible = true;
        }

        private void HideStatusBar()
        {
            toolStripStatusLabel.Text = "";
            toolStripStatusLabel.Visible = false;
            toolStripProgressBar.Visible = false;
        }

        private void BackgroundWorker_DoWork(object sender, DoWorkEventArgs e)
        {
            // Get the BackgroundWorker that raised this event.
            BackgroundWorker worker = sender as BackgroundWorker;
            // Assign the result of the computation
            // to the Result property of the DoWorkEventArgs
            // object. This is will be available to the 
            // RunWorkerCompleted eventhandler.
    
            DoWork((String[])e.Argument, worker, e);
        }

        private void _executeButton_Click(object sender, System.EventArgs e)
        {
            Application.DoEvents(); //consume other application events before invoking the background worker
            _scriptName = _scriptNameComboBox.Text.Trim();
            this.Text = WindowTitle; 
            _outputTextBox.Clear();

            DisplayStatusBar(string.Format("Executing {0} ...", _scriptNameComboBox.Text.Trim()));
            _executeButton.Enabled = false;
            _strSelectedNodeID = _theNodeTextBox.Text.Trim();
            _blnStandardOutError = _theStandardOutErrorCheckBox.Checked;
            // Start the asynchronous operation.
            _backgroundWorker.RunWorkerAsync(new String[] { 
                RUNSCRIPT, _scriptNameComboBox.Text.Trim(), _parametersTextBox.Text.Trim(), _tableNameTextBox.Text.Trim()
            });
        }

        void DoWork(string[] args, BackgroundWorker worker, DoWorkEventArgs e)
        {
            if (args[0].Equals(LOADSCRIPTS))
            {
                TrafodionSystem sqlMxSystem = TrafodionSystem.FindTrafodionSystem(ConnectionDefinition);
                List<string> scriptNames = new List<string>();
                Dictionary<String, List<String>> userComponentPrivileges = ConnectionDefinition.ComponentPrivileges;
                
                if (userComponentPrivileges.ContainsKey("SQ_SCRIPTS"))
                {
                    foreach (String privName in userComponentPrivileges["SQ_SCRIPTS"])
                    {
                        scriptNames.Add(privName.Substring("SCRIPT_".Length).ToLower());
                    }
                }

                e.Result = new object[] { args[0], scriptNames };
            }
            else
            {
                Connection connection = new Connection(ConnectionDefinition);
                string cmd = "CALL TRAFODION.TRAFODION_SP.RUN_SCRIPT(?, ?, ?, ?)";

                OdbcCommand theQuery = new OdbcCommand(cmd, connection.OpenOdbcConnection);
                theQuery.CommandType = CommandType.StoredProcedure;

                OdbcParameter param1 = theQuery.Parameters.Add("@scriptName", OdbcType.Text);
                param1.Direction = System.Data.ParameterDirection.Input;
                if (_strSelectedNodeID.Trim().Length>0)
                {
                    param1.Value = string.Format("{0}:{1}", args[1], _strSelectedNodeID);
                }
                else
                {
                    param1.Value = args[1];
                }

                if (_blnStandardOutError) param1.Value = "-d " + param1.Value;

                OdbcParameter param2 = theQuery.Parameters.Add("@scriptParams", OdbcType.Text);
                param2.Direction = System.Data.ParameterDirection.Input;
                //This is to process a problem that Sublayer may contain multiple words. 
                //The format would look like: '"PROCESS BALANCE" "ALL"' or '"WMS" "FAILURE"' etc.
                param2.Value = args[2];

                OdbcParameter param3 = theQuery.Parameters.Add("@tableName", OdbcType.Text);
                param3.Direction = System.Data.ParameterDirection.Input;
                param3.Value = args[3];

                OdbcParameter param4 = theQuery.Parameters.Add("@outResult", OdbcType.Text, 500);
                param4.Direction = System.Data.ParameterDirection.Output;

                OdbcDataReader reader = Utilities.ExecuteReader(theQuery, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Database, "RunScript", true);
                while (reader.Read())
                {
                    StringBuilder result = new StringBuilder();
                    for (int colNum = 0; colNum < reader.FieldCount; colNum++)
                    {
                        result.Append(reader.GetValue(colNum));
                    }
                    result.AppendLine();
                    worker.ReportProgress(0, result.ToString());
                }

                reader.Close();
                e.Result = new object[] { args[0], "" };
            }
        }



        private void _theCloseButton_Click(object sender, EventArgs e)
        {
            this.Close();
        }


        /// <summary>
        /// Cleanup before dispose
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {
            CancelAsync();
            RemoveEventHandlers();
        }

        /// <summary>
        /// Cancels any currently running background work
        /// </summary>
        private void CancelAsync()
        {
            if (_backgroundWorker != null && _backgroundWorker.IsBusy)
            {
                _backgroundWorker.CancelAsync();
            }
        }

        private void RemoveEventHandlers()
        {
            ConnectionDefinition.Changed -= ConnectionDefinition_Changed;
            _backgroundWorker.DoWork -=BackgroundWorker_DoWork;
            _backgroundWorker.RunWorkerCompleted -=BackgroundWorker_RunWorkerCompleted;
            _backgroundWorker.ProgressChanged -=_backgroundWorker_ProgressChanged;
        }

        private void _theHelpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.RunScriptDialog);
        }
 
    }
}
