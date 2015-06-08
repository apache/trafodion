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
using System.IO;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.OverviewArea.Controls;
using Trafodion.Manager.OverviewArea.Models;
using Trafodion.Manager.Framework;
using System.Data;
using Trafodion.Manager.Framework.Connections;
using System.Collections.ObjectModel;

namespace Trafodion.Manager.OverviewArea
{
    /// <summary>
    /// class for OSIM data download
    /// </summary>
    public partial class OSIMDataDownloadUserControl : UserControl
    {
        private OSIMDataGridView _dataGridView;
        private Hashtable _operationParamaters = new Hashtable();
        private System.Data.Odbc.OdbcCommand _theQuery = null;
        private TarHandler _tarHandler;
        
        #region Constructors, Destructors
        public OSIMDataDownloadUserControl(ConnectionDefinition aConnectionDefinition)
        {
            _tarHandler = new TarHandler(aConnectionDefinition);
            _dataGridView = new OSIMDataGridView(_tarHandler);
            InitializeComponent();
            _dataGridView.Dock = DockStyle.Fill;
            TrafodionPanel1.Controls.Add(_dataGridView);

            InitializeBackgoundWorker();
            this.ListOSIMFiles();

            //make checkbox linked
            selectAllCheckBox.MouseDown += new MouseEventHandler(selectAllCheckBox_MouseDown);
            selectAllCheckBox.CheckedChanged += new EventHandler(selectAllCheckBox_Click);
            _dataGridView.CellContentClick += new DataGridViewCellEventHandler(dataGridView_CellContentClick);
        }

        /// <summary>
        /// Cleanup before dispose
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {
            CancelAsync();
            this._dataGridView.Dispose();

        }
        #endregion

        private void ListOSIMFiles()
        {
            toolStripStatusLabel.Text = Properties.Resources.FetchOSIMFiles;
            this.TrafodionStatusStrip.Visible = true;
            this.downloadButton.Enabled = false;
            this.refreshButton.Enabled = false;
            this._operationParamaters.Clear();
            // Remove any current rows
            DataTable dt = (DataTable)_dataGridView.DataSource;
            if (dt != null)
                dt.Clear();
            this._operationParamaters.Add(KeyStore.Operation, TarHandler.TarOperation.List);
            this.backgroundWorker.RunWorkerAsync(_operationParamaters);
        }

        public void Populate()
        {
            _dataGridView.Populate();
        }

        /// <summary>
        /// Initialize the BackgroundWorker object by attaching event handlers. 
        /// </summary>
        private void InitializeBackgoundWorker()
        {
            //Before do that, first prepare for the case which user has canceled the request.
            backgroundWorker.WorkerReportsProgress = true;
            backgroundWorker.WorkerSupportsCancellation = true;

            backgroundWorker.DoWork += new DoWorkEventHandler(BackgroundWorker_DoWork);
            backgroundWorker.RunWorkerCompleted +=
                new RunWorkerCompletedEventHandler(BackgroundWorker_RunWorkerCompleted);
            backgroundWorker.ProgressChanged +=
                new ProgressChangedEventHandler(BackgroundWorker_ProgressChanged);
        }

        #region BackgroundWorker event
        /// <summary>
        /// Background Method that is asynchronously invoked
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void BackgroundWorker_DoWork(object sender, System.ComponentModel.DoWorkEventArgs e)
        {
            Hashtable argument = (Hashtable)e.Argument;
            TarHandler.TarOperation tarOperation = (TarHandler.TarOperation)argument[KeyStore.Operation];
            // Get the BackgroundWorker that raised this event.
            BackgroundWorker worker = sender as BackgroundWorker;

            //initialize the e.Result as Hashtable
            if (!(e.Result is Hashtable))
            {
                Hashtable result = new Hashtable();
                e.Result = result;
            }

            try
            {
                if (tarOperation == TarHandler.TarOperation.List)
                {
                    this.Populate();
                    ((Hashtable)e.Result).Add(KeyStore.Operation, TarHandler.TarOperation.List);
                }
                else if (tarOperation == TarHandler.TarOperation.Get)
                {
                    ((Hashtable)e.Result).Add(KeyStore.Operation, TarHandler.TarOperation.Get);
                    this.DownloadFiles(argument[KeyStore.FolderName].ToString(), (List<String>)argument[KeyStore.FileList], backgroundWorker, e);
                }
                else if (tarOperation == TarHandler.TarOperation.Cancel)
                {
                    _theQuery.Cancel();
                }
            }
            catch (Exception ex)
            {
                if (worker.CancellationPending)
                {
                    e.Cancel = true;
                }
                throw ex;
            }
        }

        /// <summary>
        /// Handle completion events from the background worker
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void BackgroundWorker_RunWorkerCompleted(object sender, System.ComponentModel.RunWorkerCompletedEventArgs e)
        {
            if (this.IsDisposed)
                return;

            resetControlStatus();

            if (this.backgroundWorker.CancellationPending || e.Cancelled)
            {
                if (TarHandler.TarOperation.Get == (TarHandler.TarOperation)this._operationParamaters[KeyStore.PreOperation])
                {
                    MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), Properties.Resources.CancelDownload, Properties.Resources.DownloadOSIMFiles,
                MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
                this._operationParamaters.Clear();
                return;
            }

            String operationName = null;
            Hashtable result = e.Result as Hashtable;
            TarHandler.TarOperation operation = (TarHandler.TarOperation)result[KeyStore.Operation];
            if (operation == TarHandler.TarOperation.List)
            {
                operationName = "list tar file";
            }
            else if (operation == TarHandler.TarOperation.Get)
            {
                operationName = "download tar file";
            }
            if (e.Error == null)
            {
                if (operation == TarHandler.TarOperation.List)
                {
                    this.selectAllCheckBox.CheckState = CheckState.Unchecked;
                    this._dataGridView.bindDatasouce();
                }
                else if (operation == TarHandler.TarOperation.Get)
                {
                    int downloadedFileCount = (int)result[KeyStore.DownloadedNum];
                    if (downloadedFileCount > 0)
                        MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), 
                            string.Format(Properties.Resources.DownloadedCountMessage, downloadedFileCount, result[KeyStore.FolderName]), 
                            Properties.Resources.DownloadOSIMFiles, MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
                
                result.Clear();
            }

            this._operationParamaters.Clear();

            if (e.Error != null)
            {
                MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                                String.Format(Properties.Resources.OperationFailed, operationName) + "\n\n" + e.Error.Message,
                                Properties.Resources.Error, MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        /// <summary>
        /// Handles any progress events from the background worker
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void BackgroundWorker_ProgressChanged(object sender, System.ComponentModel.ProgressChangedEventArgs e)
        {
            //set the status label for multi-file operations
            this.toolStripStatusLabel.Text = "Downloading " + e.UserState as string + "...";
        }

        #endregion

        /// <summary>
        /// Cancels any currently running background work
        /// </summary>
        private void CancelAsync()
        {
            this.toolStripStatusLabel.Text = Properties.Resources.CancelRequestDownload;
            if (backgroundWorker != null && backgroundWorker.IsBusy)
            {
                backgroundWorker.CancelAsync();
                if (_theQuery != null)
                {
                    try
                    {
                        TarHandler.TarOperation preOperation = (TarHandler.TarOperation)this._operationParamaters[KeyStore.Operation];
                        //clear the paramater here just in case
                        this._operationParamaters.Clear();
                        this._operationParamaters.Add(KeyStore.Operation,TarHandler.TarOperation.Cancel);
                        this._operationParamaters.Add(KeyStore.PreOperation, preOperation);
                        this.backgroundWorker.RunWorkerAsync(this._operationParamaters);
                    }
                    catch (Exception)
                    {
                    }
                }
            }
        }

        private void downloadButton_Click(object sender, EventArgs e)
        {
            //Display a folder browser dialog to let the user select the target folder.
            FolderBrowserDialog folderDialog = new FolderBrowserDialog();
            folderDialog.Description = Properties.Resources.SelectTargetFolder;
            folderDialog.SelectedPath = Trafodion.Manager.Framework.Utilities.FileDialogLocation();

            if (folderDialog.ShowDialog() == DialogResult.OK)
            {
                string targetDirectory = folderDialog.SelectedPath;
                //Save the file location for future reference
                Trafodion.Manager.Framework.Utilities.FileDialogLocation(targetDirectory);

                List<String> downloadFiles = new List<string>();
                for (int i = 0; i < _dataGridView.RowCount; i++)
                {
                    if ((bool)_dataGridView.Rows[i].Cells[0].EditedFormattedValue)
                        downloadFiles.Add(_dataGridView.Rows[i].Cells[1].Value.ToString());
                }
                string[] originalDownloadFiles = downloadFiles.ToArray();
                List<string> sameFiles;
                //if there is any file in the DIR with the same name as downloadFiles, give a warning message
                if (getSameFileNum(targetDirectory, downloadFiles,out sameFiles) > 0)
                {
                    string message = "\n";
                    if (sameFiles.Count < 6)
                    {
                        foreach (string name in sameFiles)
                            message += name + "\n";
                        message = string.Format(Properties.Resources.FileExistedWarningWithDetail, sameFiles.Count, message);
                    }
                    else
                    {   message = Properties.Resources.FileExistedWarning; }

                    DialogResult dr = MessageBox.Show(message, Properties.Resources.DownloadOSIMFiles, MessageBoxButtons.YesNoCancel, MessageBoxIcon.Question);
                    if (dr == DialogResult.Yes)
                    {
                        //get original download files
                        string[] temp = new string[originalDownloadFiles.Length];
                        originalDownloadFiles.CopyTo(temp,0);
                        downloadFiles = new List<string>(temp);
                    }
                    else if (dr == DialogResult.Cancel)
                    {
                        return;
                    }
                }

                if (downloadFiles.Count == 0)
                    return;

                try
                {
                    //set control status
                    this.TrafodionStatusStrip.Visible = true;
                    this.toolStripStatusLabel.Text = Properties.Resources.DownloadingOSIMFiles;
                    this.downloadButton.Enabled = false;
                    this.TrafodionCloseButton.Text = "&Cancel";
                    this.refreshButton.Enabled = false;
                    //store param for backgroundworker
                    this._operationParamaters.Add(KeyStore.Operation, TarHandler.TarOperation.Get);
                    this._operationParamaters.Add(KeyStore.FolderName, targetDirectory);
                    this._operationParamaters.Add(KeyStore.FileList, downloadFiles);

                    this.backgroundWorker.RunWorkerAsync(_operationParamaters);
                }
                catch (Exception ex)
                {
                    throw ex;
                }
            }
        }

        /// <summary>
        /// get the number of the files with the same name as downloadFiles in the targetDirectory
        /// </summary>
        /// <param name="targetDirectory"></param>
        /// <param name="downloadFiles">file list to be downloaded</param>
        /// <param name="sameFiles">file list with the same name</param>
        /// <returns>same files number</returns>
        private int getSameFileNum(string targetDirectory, List<string> downloadFiles, out List<string> sameFiles)
        {
            int count = 0;
            List<string> filenames = new List<string>();
            sameFiles = new List<string>();
            DirectoryInfo dirinfo = new DirectoryInfo(targetDirectory);
            FileInfo[] files = dirinfo.GetFiles(Properties.Resources.TarExt);

            foreach (FileInfo file in files)
                filenames.Add(file.Name);

            foreach (String filename in filenames)
            {
                if (downloadFiles.Contains(filename))
                {
                    sameFiles.Add(filename);
                    downloadFiles.Remove(filename);
                    count++;
                }
            }

            return count;
        }


        private void selectAllCheckBox_Click(object sender, EventArgs e)
        {
            int i = 0;
            if (this.selectAllCheckBox.CheckState == CheckState.Checked)
            {
                if (_dataGridView.Rows.Count > 0 && (this.backgroundWorker == null || !this.backgroundWorker.IsBusy))
                    this.downloadButton.Enabled = true;
                while (i < _dataGridView.Rows.Count)
                    _dataGridView.Rows[i++].Cells[0].Value = true;
            }
            else if (this.selectAllCheckBox.CheckState == CheckState.Unchecked)
            {
                //disable download button if no row is selected
                this.downloadButton.Enabled = false;
                while (i < _dataGridView.Rows.Count)
                    _dataGridView.Rows[i++].Cells[0].Value = false;
            }
            else if (this.selectAllCheckBox.CheckState == CheckState.Indeterminate && (this.backgroundWorker == null || !this.backgroundWorker.IsBusy))
            {
                this.downloadButton.Enabled = true;
            }
        }

        private void selectAllCheckBox_MouseDown(object sender, MouseEventArgs e)
        {
            this.selectAllCheckBox.ThreeState = false;
        }

        private void dataGridView_CellContentClick(object sender, DataGridViewCellEventArgs e)
        {
            int i = 1;
            this.selectAllCheckBox.ThreeState = true;

            if (e.ColumnIndex == 0 && e.RowIndex != -1)
            {
                //make the checkbox editable
                _dataGridView.Rows[e.RowIndex].Cells[0].Value = !(bool)_dataGridView.Rows[e.RowIndex].Cells[0].EditedFormattedValue;
                //get the first row's checkbox status
                Boolean mark = Boolean.Parse(_dataGridView.Rows[0].Cells[0].EditedFormattedValue.ToString());
                while (i < _dataGridView.Rows.Count)
                    if (Boolean.Parse(_dataGridView.Rows[i++].Cells[0].EditedFormattedValue.ToString()) != mark)
                    {
                        this.selectAllCheckBox.CheckState = CheckState.Indeterminate;
                        return;
                    }
                this.selectAllCheckBox.CheckState = mark ? CheckState.Checked : CheckState.Unchecked;
            }
        }

        /// <summary>
        /// Download file list to the specified target directory
        /// </summary>
        /// <param name="targetDirectory"></param>
        /// <param name="downloadFiles">file list that to be downloaded</param>
        /// <param name="worker">the backgroudworker which is used to download files</param>
        /// <param name="e"></param>
 
        private void DownloadFiles(String targetDirectory, List<string> downloadFiles, BackgroundWorker worker, DoWorkEventArgs e)
        {
            int downloadedFileCount = 0;
            List<FileInfo> downloadedFiles = new List<FileInfo>();
            foreach (String fileName in downloadFiles)
            {
                if (!string.IsNullOrEmpty(fileName) && !this.IsDisposed && !worker.CancellationPending)
                {
                    string content = null;
                    int count = 0, totalCount = 0;
                    byte[] data = new byte[10240];

                    FileInfo f = new FileInfo(Path.Combine(targetDirectory, fileName));
                    BinaryWriter br = new BinaryWriter(f.OpenWrite());
                    try
                    {
                        //Read 10K chunks of base64 bytes and convert them to binary
                        //and write to target file
                        do
                        {
                            worker.ReportProgress(0, fileName);
                            //The count is the actual count rather than the length of the content
                            _tarHandler.getTarFile(fileName, totalCount, out content, out count, ref _theQuery);
                            int len = content.Length;
                            if (count > 0)
                            {
                                data = System.Convert.FromBase64String(content);
                                br.Write(data, 0, count);
                            } 
                            totalCount += count;
                        } while (count > 0 && !worker.CancellationPending && !this.IsDisposed);

                        downloadedFiles.Add(f);
                        downloadedFileCount++;
                    }
                    catch (Exception ex)
                    {
                        ((Hashtable)e.Result).Add(KeyStore.DownloadedNum, downloadedFileCount);
                        ((Hashtable)e.Result).Add(KeyStore.FolderName, targetDirectory);
                        if (!worker.CancellationPending)
                        {
                            throw ex;
                        }
                    }
                    finally
                    {
                        br.Close();
                        //considering big file
                        if (worker.CancellationPending)
                        {
                            e.Cancel = true;
                            //delete downloaded files if download is cancelled
                            foreach(FileInfo file in downloadedFiles)
                                file.Delete();
                        }
                    }
                }
            }

            ((Hashtable)e.Result).Add(KeyStore.DownloadedNum, downloadedFileCount);
            ((Hashtable)e.Result).Add(KeyStore.FolderName, targetDirectory);
        }

        private void helpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.OSIMDataDownload);
        }

        private void TrafodionCloseButton_Click(object sender, EventArgs e)
        {
            //If the background worker is not busy, the button text is Close
            if (!backgroundWorker.IsBusy)
            {
                this.ParentForm.Close();
            }
            else
            {
                //when initializing, we can close the form directly
                if (this.TrafodionCloseButton.Text.Contains(Properties.Resources.Close))
                    this.ParentForm.Close();
                //If background worker is busy, the button text is Cancel, cancel the background work
                CancelAsync();
            }
        }

        private void refreshButton_Click(object sender, EventArgs e)
        {
            ListOSIMFiles();
        }

        private void resetControlStatus()
        {
            this.toolStripStatusLabel.Text = string.Empty;
            this.TrafodionStatusStrip.Visible = false;
            this.refreshButton.Enabled = true;
            this.downloadButton.Enabled = this.selectAllCheckBox.Checked;
            this.TrafodionCloseButton.Text = "&"+Properties.Resources.Close;
        }

        private class KeyStore
        {
            public const string PreOperation = "PreOperation";
            public const string Operation = "Operation";
            public const string FolderName = "FolderName";
            public const string FileList = "FileList";
            public const string DownloadedNum = "DownloadedNum";
        }

    }

}
