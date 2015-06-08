//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.
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
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Controls;
using System.Collections.Generic;
using System.Data;
using System.Collections;
using System.Drawing;
using System.IO;
using System.Text.RegularExpressions;
using Trafodion.Manager.DatabaseArea.NCC;
using System.Data.Odbc;
using Trafodion.Manager.Framework.Connections;
using System.ComponentModel;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class MaintainScript : UserControl
    {
        private const string COMMAND_TEMPLATE
            = "MAINTAIN TABLE {0}, \r\n"
            + "      UPDATE STATISTICS '{1}'; \r\n";
        private const string FILE_FILTER = "Text File (*.sql;*.txt)|*.sql;*.txt|All Files (*.*)|*.*";
        private const string ACTION_EXECUTE = "Execute";
        private const string ACTION_CANCEL = "Cancel";
        private const string FORMAT_EXECUTION_RESULT = " {0} {1}";
        private const string COLUMN_COMMAND = "Command";
        private const string COLUMN_STATUS = "Status";
        private const string COLUMN_ERROR = "Error";
        private const string COLUMN_TIME = "Time";
        private bool _isExecuting = false;
        private string _currentWork = string.Empty;
        private readonly List<string> _options = null;
        private ConnectionDefinition _connectionDefinition = null;
        private SaveFileDialog _saveFileDialog = new SaveFileDialog();
        private BackgroundWorker _executor = null;
        private List<string> _executedCommands = new List<string>();
        private List<string> _cancelledCommands = new List<string>();
        private DataTable _dtMessages = new DataTable();


        public MaintainScript(string queryId, List<string> tableNames, ConnectionDefinition connectionDefinition)
        {
            InitializeComponent();

            _options = new List<string>
                        {             
                            rbtnOnNecessaryColumns.Text,
                            rbtnOnExistingColumn.Text, 
                            rbtnOnEveryColumn.Text
                        };
            _connectionDefinition = connectionDefinition;

            _dtMessages.Columns.Add(COLUMN_COMMAND, typeof(string));
            _dtMessages.Columns.Add(COLUMN_STATUS, typeof(string));
            _dtMessages.Columns.Add(COLUMN_ERROR, typeof(string));
            _dtMessages.Columns.Add(COLUMN_TIME, typeof(DateTime));

            txtQueryId.Text = queryId;
            txtScript.WordWrap = true;
            executePanel.Visible = false;

            this.Load += (sender, e) =>
                {
                    this.ParentForm.FormBorderStyle = FormBorderStyle.FixedToolWindow;
                    this.ParentForm.FormClosing += FormClosing;
                    GenerateScript(tableNames);
                };
        }

        private string SelectedOption
        {
            get
            {
                if (rbtnOnNecessaryColumns.Checked)
                {
                    return rbtnOnNecessaryColumns.Text;
                }
                else if (rbtnOnExistingColumn.Checked)
                {
                    return rbtnOnExistingColumn.Text;
                }
                else if (rbtnOnEveryColumn.Checked)
                {
                    return rbtnOnEveryColumn.Text;
                }

                throw new Exception("No Option is checked!");
            }
        }

        private void GenerateScript(List<string> tableNames)
        {
            List<string> commands = tableNames.ConvertAll(tableName => string.Format(COMMAND_TEMPLATE, tableName, SelectedOption));
            string script = String.Join(Environment.NewLine, commands.ToArray());
            txtScript.Text = script;       
        }

        private void PreDoWork()
        {
            if (this._currentWork == ACTION_EXECUTE)
            {
                btnExecute.Text = ACTION_CANCEL;
            }
            else
            {
                btnExecute.Enabled = false;
            }

            rbtnOnNecessaryColumns.Enabled = rbtnOnExistingColumn.Enabled = rbtnOnEveryColumn.Enabled = false;
            txtScript.ReadOnly = true;
            txtScript.BackColor = Color.WhiteSmoke;
            executePanel.Visible = true;
        }

        private void PostDoWork()
        {
            btnExecute.Text = ACTION_EXECUTE;
            executePanel.Visible = false;

            txtScript.ReadOnly = false;
            txtScript.BackColor = Color.White;

            SetAvailability();
        }

        private void SetAvailability()
        {
            bool isScriptEmpty = txtScript.Text.Replace(Environment.NewLine, string.Empty).Replace("\n", string.Empty).Trim().Length == 0;
            btnCopy.Enabled = btnSave.Enabled = btnExecute.Enabled
                = rbtnOnNecessaryColumns.Enabled = rbtnOnExistingColumn.Enabled = rbtnOnEveryColumn.Enabled
                = !isScriptEmpty;
        }

        private void RenewScript()
        {
            foreach (string option in this._options)
            {
                // Use Regular Expression so as to replace the option text by case insensitive
                Regex regex = new Regex(option.Replace(" ", @"\s+"), RegexOptions.IgnoreCase);
                txtScript.Text = regex.Replace(txtScript.Text, SelectedOption);
            }
        }

        private void CancelWork()
        {
            if (this._isExecuting && this._executor != null)
            {
                if (this._executor.IsBusy || !this._executor.CancellationPending)
                {
                    this._executor.CancelAsync();
                }
            }
        }

        private void ShowMessage(string message, DataTable dtMessages, bool hasFailed, bool hasCancelled)
        {
            Icon icon = SystemIcons.Information;
            if (hasFailed)
            {
                icon = SystemIcons.Error;
            }
            else if (hasCancelled)
            {
                icon = SystemIcons.Warning;
            }

            DataTable dtSortedMessages = dtMessages.Clone();
            DataRow[] sortedMessageRows = dtMessages.Select(string.Empty, string.Format("{0} ASC", COLUMN_TIME));
            foreach (DataRow messageRow in sortedMessageRows)
            {
                dtSortedMessages.Rows.Add(messageRow.ItemArray);
            }
            dtSortedMessages.Columns.Remove(COLUMN_TIME);

            TrafodionMultipleMessageDialog messageDialog = new TrafodionMultipleMessageDialog(message, dtSortedMessages, icon);
            messageDialog.Size = new Size(740, 550);
            messageDialog.Text = Properties.Resources.MaintainScript_ExecutionResultCaption;
            messageDialog.Shown += (sender, args) =>
            {
                messageDialog.GridView.Columns[COLUMN_COMMAND].Width = 500;
                messageDialog.GridView.Columns[COLUMN_STATUS].Width = 60;
            };
            messageDialog.StartPosition = FormStartPosition.CenterParent;
            messageDialog.ShowDialog(this.ParentForm);
        }

        private void rbtnOnEveryColumn_CheckedChanged(object sender, EventArgs e)
        {
            RenewScript();
        }

        private void rbtnOnExistingColumn_CheckedChanged(object sender, EventArgs e)
        {
            RenewScript();
        }

        private void rbtnOnNecessaryColumns_CheckedChanged(object sender, EventArgs e)
        {
            RenewScript();
        }

        private void btnCopy_Click(object sender, EventArgs e)
        {
            txtScript.SelectAll();
            txtScript.Copy();
        }

        private void btnSave_Click(object sender, EventArgs e)
        {
            _saveFileDialog.Filter = FILE_FILTER;
            if (_saveFileDialog.ShowDialog() == DialogResult.OK)
            {
                StreamWriter sw = null;
                FileStream fs = null;
                try
                {
                    fs = new FileStream(_saveFileDialog.FileName, FileMode.Create);
                    sw = new StreamWriter(fs);

                    string[] textLines = txtScript.Text.Split(new string[]{ Environment.NewLine, "\n" }, StringSplitOptions.None);
                    foreach (string textLine in textLines)
                    {
                        sw.WriteLine(textLine);
                    }
                    // Persist the last file-location: 
                    Trafodion.Manager.Framework.Utilities.FileDialogLocation(_saveFileDialog.FileName);
                }
                catch (Exception ex)
                {
                    MessageBox.Show(this.ParentForm, Properties.Resources.FileSaveFailure + " : " + ex.Message,
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

        private void FormClosing(object sender, FormClosingEventArgs e)
        {
            CancelWork();
        }

        private void BackgroundWorker_DoWork(object sender, DoWorkEventArgs e)
        {
            this._executedCommands.Clear();
            this._cancelledCommands.Clear();
            this._dtMessages.Clear();

            string script = (string)e.Argument;
            List<string> commandTexts = new List<string>();
            commandTexts.AddRange( script.Trim().Split(new char[] { ';' }, StringSplitOptions.RemoveEmptyEntries) );
            for (int i = commandTexts.Count - 1; i >= 0; i-- )
            {
                commandTexts[i] = commandTexts[i].Trim();
                if (commandTexts[i].Length == 0)
                {
                    commandTexts.RemoveAt(i);
                }
            }

            if (commandTexts.Count > 0)
            {
                _cancelledCommands.AddRange(commandTexts);
                foreach (string commandText in commandTexts)
                {
                    if (_executor.CancellationPending)
                    {
                        e.Cancel = true;
                        break;
                    }

                    OdbcCommand odbcCommand = new OdbcCommand(commandText);
                    odbcCommand.Connection = null;
                    try
                    {
                        odbcCommand.Connection = new Connection(this._connectionDefinition).OpenOdbcConnection;
                        _cancelledCommands.Remove(commandText);
                        _executedCommands.Add(commandText);
                        Utilities.ExecuteNonQuery(odbcCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Connectivity, "Batch Maintain Script", false);
                        _dtMessages.Rows.Add(new object[] { commandText, ExecutionStatus.Successful.ToString(), string.Empty, DateTime.Now });
                    }
                    catch (Exception ex)
                    {
                        _dtMessages.Rows.Add(new object[] { commandText, ExecutionStatus.Failed.ToString(), ex.Message, DateTime.Now });
                    }
                    finally
                    {
                        if (odbcCommand.Connection != null && odbcCommand.Connection.State != ConnectionState.Closed)
                        {
                            odbcCommand.Connection.Close();
                        }
                    }
                }
            }
        }

        private void BackgroundWorker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            if (this.ParentForm != null && this.ParentForm.Visible)
            {
                this._isExecuting = false;
                PostDoWork();

                foreach (string commandText in this._cancelledCommands)
                {
                    this._dtMessages.Rows.Add(new object[] { commandText, ExecutionStatus.Cancelled.ToString(), string.Empty, DateTime.Now });
                }

                int failed = this._dtMessages.Select(string.Format("{0} = '{1}'", COLUMN_STATUS, ExecutionStatus.Failed.ToString())).Length;
                int cancelled = this._dtMessages.Select(string.Format("{0} = '{1}'", COLUMN_STATUS, ExecutionStatus.Cancelled.ToString())).Length;
                int successful = this._dtMessages.Select(string.Format("{0} = '{1}'", COLUMN_STATUS, ExecutionStatus.Successful.ToString())).Length;
                bool hasFailed = failed > 0;
                bool hasCancelled = cancelled > 0;

                string message = Properties.Resources.MaintainScript_ExecuteSuccessfully;
                if (hasFailed || hasCancelled)
                {
                    message = string.Format(Properties.Resources.MaintainScript_ExecutionResult, failed, cancelled, successful);
                }

                ShowMessage(message, _dtMessages, hasFailed, hasCancelled);
            }
        }

        private void btnExecute_Click(object sender, EventArgs e)
        {
            this._currentWork = ((Button)sender).Text.Trim();

            PreDoWork();

            if (this._currentWork == ACTION_EXECUTE)
            {
                this._isExecuting = true;

                _executor = new BackgroundWorker();
                _executor.WorkerSupportsCancellation = true;
                _executor.DoWork += BackgroundWorker_DoWork;
                _executor.RunWorkerCompleted += BackgroundWorker_RunWorkerCompleted;
                _executor.RunWorkerAsync(txtScript.Text);
            }
            else
            {
                CancelWork();
            }
        }

        private void btnHelp_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(Trafodion.Manager.WorkloadArea.HelpTopics.MaintainScript);
        }

        private void txtScript_TextChanged(object sender, EventArgs e)
        {
            SetAvailability();
        }

        private enum ExecutionStatus
        {
            Successful,
            Cancelled,
            Failed
        }
    }
}
