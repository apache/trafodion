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
using System.ComponentModel;
using System.IO;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.WorkloadArea.Model;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.WorkloadArea.Controls 
{
    /// <summary>
    /// This user control generates DDL text for all objects in the given WMS System.
    /// The user control is added to a ManagedWindow at the time this control is instantiated.
    /// </summary>
    public partial class ExportConfigControl : UserControl
    {
        private WmsSystem _wmsSystem = null;
        private SaveFileDialog _saveFileDialog = new SaveFileDialog();
        private System.ComponentModel.BackgroundWorker _backgroundWorker;
        bool isOperationCancelled = false;

        /// <summary>
        /// Constructs this user control
        /// </summary>
        /// <param name="sourceWmsSystem">The WMS system model associated with the WMS tree node, from which this tool was launched</param>
        public ExportConfigControl(WmsSystem sourceWmsSystem)
        {
            InitializeComponent();
            InitializeBackgoundWorker();
            _wmsSystem = sourceWmsSystem;

            _saveFileDialog.InitialDirectory = Trafodion.Manager.Framework.Utilities.FileDialogLocation();
            _saveFileDialog.AddExtension = true;
            _saveFileDialog.DefaultExt = "sql";
            _saveFileDialog.Filter =  "Text File (*.sql;*.txt)|*.sql;*.txt|All Files (*.*)|*.*";
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
            _backgroundWorker.ProgressChanged +=
                new ProgressChangedEventHandler(BackgroundWorker_ProgressChanged);
        }

        /// <summary>
        /// Peform load time initializations
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void ExportConfigControl_Load(object sender, EventArgs e)
        {
            try
            {
                ResetProgressBar();
                GenerateDDL();
            }
            catch (Exception ex)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), ex.Message, global::Trafodion.Manager.Properties.Resources.Error,
                        MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        /// <summary>
        /// If the button is in "Cancel" mode ask the background worker to stop working
        /// If the button is in "Done" mode, ask the parent form to close
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void doneButton_Click(object sender, EventArgs e)
        {
            if (doneButton.Text.Equals(Properties.Resources.Done))
            {
                ((Form)Parent).Close();
            }
            else
            {
                this._backgroundWorker.CancelAsync();
            }
        }

        /// <summary>
        /// Saves the DDL text to a file
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void saveButton_Click(object sender, EventArgs e)
        {
            if (_saveFileDialog.ShowDialog() == DialogResult.OK)
            {
                StreamWriter sw = null;
                FileStream fs = null;
                try
                {
                    fs = new FileStream(_saveFileDialog.FileName, FileMode.Create);
                    sw = new StreamWriter(fs);

                    string[] separators = { Environment.NewLine, "\n" };
                    string[] textArray = ddlOutputTextBox.Text.Split(separators, StringSplitOptions.None);
                    foreach (string lineOfText in textArray)
                    {
                        sw.WriteLine(lineOfText);
                    }

                    // Persist the last file-location: 
                    Trafodion.Manager.Framework.Utilities.FileDialogLocation(_saveFileDialog.FileName);
                }
                catch (Exception ex)
                {
                    MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.FileSaveFailure + " : " + ex.Message,
                    global::Trafodion.Manager.Properties.Resources.Error, MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
                finally
                {
                    if (sw != null)
                    {
                        sw.Close();
                    }
                    if (fs != null)
                    {
                        fs.Close();
                    }
                }
            }

        }

        /// <summary>
        /// Reset the status bar for reporting progress
        /// </summary>
        public void ResetProgressBar()
        {
            //Total # of services in system plus 1 for the system
            int count = 1 + _wmsSystem.WmsServices.Count + _wmsSystem.WmsRules.Count + _wmsSystem.WmsAdminRoles.Count;
            toolStripProgressBar.Maximum = count + 1;
            toolStripProgressBar.Minimum = 1;
            toolStripProgressBar.Step = 1;
            toolStripProgressBar.Value = 1;
        }

        /// <summary>
        /// Hide the status and progress bar when they are not displaying information
        /// </summary>
        private void HideStatusBar()
        {
            toolStripStatusLabel.Text = "";
            toolStripStatusLabel.Visible = false;

            ResetProgressBar();
            toolStripProgressBar.Visible = false;
        }

        /// <summary>
        /// Calls the background worker to work on loading the DDL for the system and its objects
        /// </summary>
        private void GenerateDDL()
        {
            doneButton.Text = Properties.Resources.Cancel; //Temporarily change the text on the done button to cancel
            saveButton.Enabled = false; //Save button is disabled until work completes
//            ddlOutputTextBox.AppendText("----" + String.Format(Properties.Resources.DDLStartMessage, _wmsSystem.Name + " (" + _wmsSystem.ConnectionDefinition.OdbcServerVersion + ") ", Trafodion.Manager.Framework.Utilities.CurrentFormattedDateTime));
            DateTime now = DateTime.Now;
            String workStartMsg = String.Format("{0}  -  Generating script for system {1} ({2})",
                                                 Trafodion.Manager.Framework.Utilities.GetFormattedDateTime(now, true),
                                                 _wmsSystem.ConnectionDefinition.Server,
                                                 _wmsSystem.ConnectionDefinition.PlatformReleaseVersionString);
            ddlOutputTextBox.AppendText("--  " + workStartMsg + Environment.NewLine + Environment.NewLine);
            ddlOutputTextBox.AppendText("--  Open WMS. All commands that follow are WMS commands. " +
                                        Environment.NewLine);
            ddlOutputTextBox.AppendText("WMSOPEN;" + Environment.NewLine);
            ddlOutputTextBox.AppendText(Environment.NewLine);

            Application.DoEvents(); //consume other application events before invoking the background worker

            // Start the asynchronous operation.
            _backgroundWorker.RunWorkerAsync(_wmsSystem);
        }

        /// <summary>
        /// This event handler is where the actual,
        /// potentially time-consuming DDL work is done.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void BackgroundWorker_DoWork(object sender,
            DoWorkEventArgs e)
        {
            // Get the BackgroundWorker that raised this event.
            BackgroundWorker worker = sender as BackgroundWorker;

            isOperationCancelled = false;
            // Assign the result of the computation
            // to the Result property of the DoWorkEventArgs
            // object. This is will be available to the 
            // RunWorkerCompleted eventhandler.
            FetchDDL((WmsSystem)e.Argument, worker, e);
        }


        /// <summary>
        /// This event handler deals with the results of the
        /// background operation.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void BackgroundWorker_RunWorkerCompleted(
            object sender, RunWorkerCompletedEventArgs e)
        {
            if (isOperationCancelled) return;

            // First, handle the case where an exception was thrown.
            if (e.Error != null)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), e.Error.Message, global::Trafodion.Manager.Properties.Resources.Error, MessageBoxButtons.OK);
            }
            else if (e.Cancelled)
            {
                // Next, handle the case where the user canceled 
                // the operation.
                // Note that due to a race condition in 
                // the DoWork event handler, the Cancelled
                // flag may not have been set, even though
                // CancelAsync was called.
                DateTime now = DateTime.Now;
                String endMsg = String.Format("{0}  -  Operation aborted.",
                                              Trafodion.Manager.Framework.Utilities.GetFormattedDateTime(now, true));
                ddlOutputTextBox.AppendText(Environment.NewLine + Environment.NewLine + "--  " + endMsg);
            }
            else
            {
                // Finally, handle the case where the operation 
                // succeeded.
                ddlOutputTextBox.AppendText(Environment.NewLine + Environment.NewLine);
                ddlOutputTextBox.AppendText("--  Close WMS. " + Environment.NewLine + "WMSCLOSE;");

                DateTime now = DateTime.Now;
                String endMsg = String.Format("{0}  -  Operation Completed.",
                                              Trafodion.Manager.Framework.Utilities.GetFormattedDateTime(now, true));
                ddlOutputTextBox.AppendText(Environment.NewLine + Environment.NewLine + "--  " + endMsg);
            }

            ddlOutputTextBox.AppendText(Environment.NewLine);
            ddlOutputTextBox.AppendText(Environment.NewLine);
            HideStatusBar();

            if (ddlOutputTextBox.Text.Length > 0)
                saveButton.Enabled = true; //Enable the save button

            doneButton.Text = Properties.Resources.Done;
        }

        /// <summary>
        /// This event handler updates the progress bar and appends the DDL text to the output textbox
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
 
        private void BackgroundWorker_ProgressChanged(object sender,
            ProgressChangedEventArgs e)
        {
            if (isOperationCancelled) return;
            toolStripProgressBar.Visible = true;
            toolStripStatusLabel.Visible = true;
            toolStripProgressBar.PerformStep();

            string ddlText = e.UserState as string;
            ddlOutputTextBox.AppendText(ddlText);

            string text = string.Format("Generated WMS command for {0} of {1} objects",
                                            toolStripProgressBar.Value, toolStripProgressBar.Maximum);
            toolStripStatusLabel.Text = text;
            statusStrip.Update();
        }

        /// <summary>
        /// This method is invoked by the worker thread to fetch DDL for the Wms System and all its objects
        /// The fetched DDL is reported back in a progress event
        /// </summary>
        /// <param name="wmsSystem"></param>
        /// <param name="worker"></param>
        /// <param name="e"></param>
        void FetchDDL(WmsSystem wmsSystem, BackgroundWorker worker, DoWorkEventArgs e)
        {
            //Fetch the DDL of the system first
            string text = Environment.NewLine + "--  WMS System configuration" + Environment.NewLine;

            try
            {
                text += wmsSystem.DDLText;
            }
            catch (Exception ex)
            {
                text += ex.Message;
            }
            worker.ReportProgress(0, text);

            try
            {
                //Now loop through each of the service levels and fetch their DDL text
                for (int i = 0; i < wmsSystem.WmsServices.Count; i++)
                {
                    // Abort the operation if the user has canceled.
                    // Note that a call to CancelAsync may have set 
                    // CancellationPending to true just after the
                    // last invocation of this method exits, so this 
                    // code will not have the opportunity to set the 
                    // DoWorkEventArgs.Cancel flag to true. This means
                    // that RunWorkerCompletedEventArgs.Cancelled will
                    // not be set to true in your RunWorkerCompleted
                    // event handler. This is a race condition.

                    if (worker.CancellationPending)
                    {
                        e.Cancel = true;
                    }
                    else
                    {
                        text = Environment.NewLine + Environment.NewLine;
                        if (i == 0)
                        {
                            text += "--  Service configuration" + Environment.NewLine;
                        }
                        try
                        {
                            text += wmsSystem.WmsServices[i].DDLText;
                        }
                        catch (Exception ex)
                        {
                            text += ex.Message;
                        }
                        worker.ReportProgress(0, text);
                    }
                }

                //Now loop through each of the rules and fetch their DDL text
                for (int i = 0; i < wmsSystem.WmsRules.Count; i++)
                {
                    if (worker.CancellationPending)
                    {
                        e.Cancel = true;
                    }
                    else
                    {
                        text = Environment.NewLine + Environment.NewLine;
                        if (i == 0)
                        {
                            text += "--  Rule configuration" + Environment.NewLine;
                        }

                        try
                        {
                            text += wmsSystem.WmsRules[i].DDLText;
                        }
                        catch (Exception ex)
                        {
                            text += ex.Message;
                        }
                        worker.ReportProgress(0, text);
                    }
                }

                //Rule Associations
                string connRuleStrings = wmsSystem.AssociateConnectionRuleString;
                if (!String.IsNullOrEmpty(connRuleStrings))
                {
                    text = Environment.NewLine + Environment.NewLine + "--  Connection Rule Associations";
                    text += Environment.NewLine + connRuleStrings;
                    worker.ReportProgress(0, text);
                }
                for (int i = 0; i < wmsSystem.WmsServices.Count; i++)
                {
                    string compRuleStrings = wmsSystem.WmsServices[i].AssociateCompilationRuleString;
                    if (!String.IsNullOrEmpty(compRuleStrings))
                    {
                        text = Environment.NewLine + Environment.NewLine +
                            "--  Compilation Rule Associations for service " +
                            wmsSystem.WmsServices[i].Name + Environment.NewLine +
                            compRuleStrings;
                        worker.ReportProgress(0, text);
                    }
                    string execRuleStrings = wmsSystem.WmsServices[i].AssociateExecutionRuleString;
                    if (!String.IsNullOrEmpty(execRuleStrings))
                    {
                        text = Environment.NewLine + Environment.NewLine +
                            "--  Execution Rule Associations for service " +
                            wmsSystem.WmsServices[i].Name + Environment.NewLine +
                            execRuleStrings;
                        worker.ReportProgress(0, text);
                    }
                }

                //Now loop through each of the rules and fetch their DDL text
                for (int i = 0; i < wmsSystem.WmsAdminRoles.Count; i++)
                {
                    if (worker.CancellationPending)
                    {
                        e.Cancel = true;
                    }
                    else
                    {
                        text = Environment.NewLine;
                        if (i == 0)
                        {
                            text += Environment.NewLine + "--  Admin Role configuration";
                            worker.ReportProgress(0, text);
                            text = Environment.NewLine;
                        }
                        try
                        {
                            if (wmsSystem.WmsAdminRoles[i].isSystemAdminRole)
                                continue;
                            else
                                text += wmsSystem.WmsAdminRoles[i].DDLText;
                        }
                        catch (Exception ex)
                        {
                            text += ex.Message;
                        }
                        worker.ReportProgress(0, text);
                    }
                }
            }
            catch (Exception exc)
            {
                text += exc.Message;
                worker.ReportProgress(0, text);
            }
        }

        private void helpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.ExportConfiguration);
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
                isOperationCancelled = true;
                _backgroundWorker.CancelAsync();
            }
        }

        private void RemoveEventHandlers()
        {

        }
    }
}
