//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2012-2015 Hewlett-Packard Development Company, L.P.
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
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// User control that lets user create/Alter a Lib
    /// </summary>
    public partial class CreateLibraryUserControl : TrafodionForm
    {
        #region private fields
        TrafodionSchema _TrafodionSchema = null;
        TrafodionLibrary _sqlMxLibrary = null;
        BackgroundWorker _backgroundWorker = null;
        private string _outputFileName = string.Empty;
        enum OperationProgress : int { Start,Validate, UploadingJar,ProcessingLibrary, Finish };
        enum LibraryOperation { CreateLibrary,UpdateLibrary };
        private LibraryOperation _mode = LibraryOperation.CreateLibrary;
        long FILE_UPLOAD_SIZE = -1;
        private string _theExternalLibName = string.Empty;
        #endregion

        #region constructors

        /// <summary>
        /// Default Constructor
        /// </summary>
        public CreateLibraryUserControl()
        {
            InitializeComponent();
            ReportStatusStrip(string.Empty);
            CenterToScreen();
        }

        /// <summary>
        /// Constructor of create library
        /// </summary>
        /// <param name="aTrafodionSchema">In which schema</param>
        public CreateLibraryUserControl(TrafodionSchema aTrafodionSchema)
            : this()
        {
            _TrafodionSchema = aTrafodionSchema;
            _theCatalogName.Text = aTrafodionSchema.TheTrafodionCatalog.ExternalName;
            _theSchemaName.Text = aTrafodionSchema.ExternalName;
            //Initialize a new library model
            _sqlMxLibrary = new TrafodionLibrary(aTrafodionSchema, "", 0, 0, 0, "", "", "", "", "");
            _mode = LibraryOperation.CreateLibrary;
            Text = Properties.Resources.CreateLibrary;
            _bannerControl.ConnectionDefinition = aTrafodionSchema.ConnectionDefinition;
            if (aTrafodionSchema.ConnectionDefinition.MaxUserTableSize > 0)
            {
                FILE_UPLOAD_SIZE = aTrafodionSchema.ConnectionDefinition.MaxUserTableSize * 1024 * 1024;//user quota in MB converted to bytes.
            }
            UpdateControls();            
        }

        /// <summary>
        /// Constructor of Alter library
        /// </summary>
        /// <param name="aTrafodionLibrary">the library object to be update</param>
        public CreateLibraryUserControl(TrafodionLibrary aTrafodionLibrary)
            : this()
        {
            _sqlMxLibrary = aTrafodionLibrary;
            _TrafodionSchema = aTrafodionLibrary.TheTrafodionSchema;
            _theCatalogName.Text = _TrafodionSchema.TheTrafodionCatalog.ExternalName;
            _theSchemaName.Text = _TrafodionSchema.ExternalName;
            _theLibName.Text = aTrafodionLibrary.ExternalName;
            _theJarFileName.Text = aTrafodionLibrary.ClientFileName;
            _theLibName.ReadOnly = true;
            _mode = LibraryOperation.UpdateLibrary;
            _theCreateButton.Text = "&Alter";
            Text = Properties.Resources.AlterLibrary;
            _bannerControl.ConnectionDefinition = aTrafodionLibrary.ConnectionDefinition;
            if (aTrafodionLibrary.ConnectionDefinition.MaxUserTableSize > 0)
            {
                FILE_UPLOAD_SIZE = aTrafodionLibrary.ConnectionDefinition.MaxUserTableSize * 1024 * 1024;//user quota in MB converted to bytes.
            } 
            UpdateControls();
        }

        #endregion 

        #region BackgroundWorker
        /// <summary>
        /// initialize a background worker.
        /// </summary>
        private void initBackgroundWorker()
        {
            _backgroundWorker = new BackgroundWorker();
            _backgroundWorker.WorkerReportsProgress = true;
            _backgroundWorker.DoWork += new DoWorkEventHandler(_backgroundWorker_DoWork);
            _backgroundWorker.RunWorkerCompleted += new RunWorkerCompletedEventHandler(_backgroundWorker_RunWorkerCompleted);
            _backgroundWorker.ProgressChanged += new ProgressChangedEventHandler(_backgroundWorker_ProgressChanged);
        }

        /// <summary>
        /// Do work of Creating/Altering library
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _backgroundWorker_DoWork(object sender, DoWorkEventArgs e)
        {
            BackgroundWorker worker = sender as BackgroundWorker;

            try
            {
                worker.ReportProgress((int)OperationProgress.Start);
                HandleLibrary(worker, e);
                worker.ReportProgress((int)OperationProgress.Finish);
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
        /// update status to UI progress bar.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _backgroundWorker_ProgressChanged(object sender, ProgressChangedEventArgs e)
        {
            switch ((OperationProgress)e.ProgressPercentage)
            {
                case OperationProgress.Start:
                case OperationProgress.Validate:
                    {
                        ReportStatusStrip(String.Format(Properties.Resources.StatusValidatingJarFileMsg, ""));
                        break;
                    }
                case OperationProgress.UploadingJar:
                    {
                        ReportStatusStrip(String.Format(Properties.Resources.StatusUploading, ""));
                        break;
                    }
                case OperationProgress.ProcessingLibrary:
                    {
                        if (_mode == LibraryOperation.CreateLibrary)
                        {
                            ReportStatusStrip(String.Format(Properties.Resources.CreateLibraryProcessMsg, _theExternalLibName));
                        }
                        else if (_mode == LibraryOperation.UpdateLibrary)
                        {
                            ReportStatusStrip(String.Format(Properties.Resources.UpdateLibraryProcessMsg, _theExternalLibName));
                        }
                        
                        break;
                    }
                case OperationProgress.Finish:
                    {
                        ReportStatusStrip(string.Empty);
                        break;
                    }
                default:
                    break;

            }
        }

        /// <summary>
        /// worker completes process.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _backgroundWorker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            ReportStatusStrip(string.Empty);
            UpdateControls();
            if (e.Error != null)
            {
                MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), e.Error.Message,
                    _mode == LibraryOperation.CreateLibrary ? Properties.Resources.CreateLibrary : Properties.Resources.AlterLibrary, 
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            else
            {
                if (_mode == LibraryOperation.CreateLibrary)
                {
                    _TrafodionSchema.AddLibrary(_sqlMxLibrary);
                    _theLibName.Text = string.Empty;
                    _theJarFileName.Text = string.Empty;
                    //Display a success message
                    MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), 
                        string.Format(Properties.Resources.LibraryCreateSuccess, _sqlMxLibrary.InternalName),
                        Properties.Resources.CreateLibrary, MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
                else if (_mode == LibraryOperation.UpdateLibrary)
                {
                    _TrafodionSchema.AlterLibrary(_sqlMxLibrary);
                    //Display a success message
                    MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), 
                        string.Format(Properties.Resources.LibraryUpdateSuccess, 
                        _sqlMxLibrary.InternalName),
                        Properties.Resources.AlterLibrary, MessageBoxButtons.OK, MessageBoxIcon.Information);
                    Close();
                }
                
            }
        }

        /// <summary>
        /// Create/Alter library function
        /// </summary>
        /// <param name="worker"></param>
        /// <param name="e"></param>
        private void HandleLibrary(BackgroundWorker worker, DoWorkEventArgs e)
        {
            LibraryOperation operation = _mode;         

            switch (operation)
            {
                case LibraryOperation.CreateLibrary:
                    {
                        worker.ReportProgress((int)OperationProgress.Validate);
                        ValidateLibrary(_theExternalLibName, _theJarFileName.Text.Trim());
                        worker.ReportProgress((int)OperationProgress.UploadingJar);
                        UploadFile(_theJarFileName.Text.Trim(), worker, e);
                        worker.ReportProgress((int)OperationProgress.ProcessingLibrary);
                        CreateLibrary(_TrafodionSchema.RealAnsiName + "." + _theExternalLibName, _theJarFileName.Text.Trim());                        
                        break;
                    }
                case LibraryOperation.UpdateLibrary:
                    {
                        worker.ReportProgress((int)OperationProgress.Validate);
                        ValidateLibrary(_theLibName.Text, _theJarFileName.Text.Trim());
                        worker.ReportProgress((int)OperationProgress.UploadingJar);
                        UploadFile(_theJarFileName.Text.Trim(), worker, e);
                        worker.ReportProgress((int)OperationProgress.ProcessingLibrary);
                        UpdateLibrary(_theJarFileName.Text.Trim());
                        break;
                    }
                default:
                    {
                        break;
                    }
            }
        }

        #endregion

        #region private functions

        /// <summary>
        /// Check the uploading file whether it is existed on the server.
        /// </summary>
        /// <param name="fileName"></param>
        /// <returns></returns>
        private bool ClientJarFileExists(string libraryName, string fileName)
        {
            return _TrafodionSchema.TheTrafodionCatalog.ClientJarFileExists(
                libraryName.Trim(),
                System.Environment.MachineName, 
                fileName.Trim());
        }

        public void ValidateLibrary(string libraryName, string fileName)
        {
            //Check whether the jar is already used by a library on the server.
            if (_TrafodionSchema.TheTrafodionCatalog.ClientJarFileExists(libraryName.Trim(),
                                System.Environment.MachineName,
                                fileName.Trim())
                )
            {
                throw new Exception(Properties.Resources.ClientJarFileExistsErrorMsg);
            }

            if (!fileName.EndsWith(".jar", StringComparison.OrdinalIgnoreCase))
            {
                throw new Exception(string.Format("Cannot upload file {0}. {1}.", fileName, Properties.Resources.PCF_NOT_JAR_MESSAGE));
            }
            FileInfo f = new FileInfo(fileName);
            if (f.Length == 0)
            {
                throw new Exception(string.Format("Cannot upload file {0}. {1}.", fileName, Properties.Resources.PCF_ZERO_SIZE_MESSAGE));
            }
            if (FILE_UPLOAD_SIZE > 0 && f.Length > FILE_UPLOAD_SIZE)
            {
                throw new Exception(string.Format("Cannot upload file {0}. {1}.", fileName, string.Format(Properties.Resources.PCF_SIZE_EXCEEDS_MESSAGE, Utilities.FormatSize(FILE_UPLOAD_SIZE))));
            }

        }
        /// <summary>
        /// Upload a Jar file.
        /// </summary>
        /// <param name="fileName">Client file name</param>
        /// <param name="worker"></param>
        /// <param name="e"></param>
        private void UploadFile(string fileName, BackgroundWorker worker, DoWorkEventArgs e)
        {
            int BLOCK_SIZE = 10240;
            int BUFFER_SIZE = BLOCK_SIZE * 3 / 4;
            string[] names = fileName.Split('\\');
            string name = names[names.Length - 1];

            string content = "";
            FileInfo f = new FileInfo(fileName);


            BinaryReader br = new BinaryReader(f.OpenRead());

            Boolean first = true;
            int count = 0;
            byte[] buffer = new byte[BUFFER_SIZE];
            char[] charString;
            Array.Clear(buffer, 0, BUFFER_SIZE);
            try
            {
                _outputFileName = fileName;
                //Read the file contents in binary and convert to base64
                //and send 10K chunks of bytes at a time
                //for considering uploading file is big, we check cancellation flag
                while ((count = br.Read(buffer, 0, BUFFER_SIZE)) > 0 && !worker.CancellationPending)
                {
                    content = System.Convert.ToBase64String(buffer, 0, count);
                    charString = content.ToCharArray();
                    //int length = charString.Length;
                    if (first)
                    {
                        //The first chunk, identified by the last parameter of 1
                        _sqlMxLibrary.write(charString, 1, ref _outputFileName);
                        first = false;
                    }
                    else
                    {
                        _sqlMxLibrary.write(charString, 0, ref _outputFileName);
                    }

                    Array.Clear(buffer, 0, BUFFER_SIZE);
                }
            }
            finally
            {
                br.Close();

                //considering big file
                if (worker.CancellationPending)
                {
                    e.Cancel = true;
                }

            }
        }

        /// <summary>
        /// create library to the server
        /// </summary>
        /// <param name="libName">library name</param>
        /// <param name="localFileName">client file name</param>
        private void CreateLibrary(string libName,string localFileName)
        {
            _sqlMxLibrary.CreateLibrary(libName, _outputFileName, System.Environment.MachineName, localFileName);
            _sqlMxLibrary.InternalName = libName;
            
        }

        /// <summary>
        /// Alter library to the server
        /// </summary>
        /// <param name="localFileName">new client file name</param>
        private void UpdateLibrary(string localFileName)
        {
            _sqlMxLibrary.UpdateLibrary(_outputFileName, System.Environment.MachineName, localFileName);
        }


        /// <summary>
        /// Update the state of the controls
        /// </summary>
        private void UpdateControls()
        {
            bool isValid = false;

            //Disable the browse button if name is empty
            _theFileBrowserButton.Enabled = isValid;
            _theCloseButton.Enabled = true;

            if (String.IsNullOrEmpty(_theLibName.Text.Trim()))
            {
                _errorText.Text = Properties.Resources.NameCannotBeEmpty;
            }
            else
            {
                _theFileBrowserButton.Enabled = true;
                if (String.IsNullOrEmpty(_theJarFileName.Text.Trim()))
                {
                    _errorText.Text = Properties.Resources.SelectJarFile;
                }
                else
                {
                    if (!File.Exists(_theJarFileName.Text.Trim()))
                    {
                        _errorText.Text = Properties.Resources.SelectJarFileDoesExistedErrorMsg;
                    }
                    else
                    {
                        _theExternalLibName = Utilities.ExternalForm(_theLibName.Text.Trim(), true);
                        _errorText.Text = "";
                        isValid = true;
                    }
                }

            }
     
            //Create button is only enabled if name, code file details are present
            this._theCreateButton.Enabled = isValid;
        }


        private void ReportStatusStrip(String statusMessage)
        {
            _statusLabel.Text = statusMessage;
            _progressBar.Visible = !string.IsNullOrEmpty(statusMessage);
        }

        #endregion

        #region Events

        /// <summary>
        /// Closes the dialog
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theCloseButton_Click(object sender, EventArgs e)
        {
            Close();
        }

        /// <summary>
        /// text changed will cause validating controls.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theLibName_TextChanged(object sender, EventArgs e)
        {
            UpdateControls();
        }

        /// <summary>
        /// Open dialog for browsing client jar file.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theFileBrowserButton_Click(object sender, EventArgs e)
        {
            //Display the File Open dialog
            OpenFileDialog dlg = new OpenFileDialog();
            dlg.Filter = "SPJ Code Jar File(*.jar)|*.jar";
            dlg.FilterIndex = 0;
            dlg.Title = Properties.Resources.SelectJarFileDialogCaption;
            dlg.Multiselect = false;
            dlg.FileName = "";
            dlg.AddExtension = true;

            if (dlg.ShowDialog() == DialogResult.OK)
            {
                _theJarFileName.Text = dlg.FileName;
            }
        }

        /// <summary>
        /// text changed will cause validating controls.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theJarFileName_TextChanged(object sender, EventArgs e)
        {
            UpdateControls();
        }

        /// <summary>
        /// create/Alter library 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theCreateButton_Click(object sender, EventArgs e)
        {
            //"Altering the library might affect stored procedures that use the library.\nDo you want to continue?"
            if (_mode==LibraryOperation.UpdateLibrary && MessageBox.Show(Properties.Resources.AlterLibraryWarningMessage,
                    Properties.Resources.Warning, MessageBoxButtons.YesNo, MessageBoxIcon.Warning) == DialogResult.No)
            {
                return;
            }
            //Disable buttons
            _theFileBrowserButton.Enabled = false;
            _theCreateButton.Enabled = false;
            _theCloseButton.Enabled = false;

            //Initiate the upload as a background operation           
            initBackgroundWorker();
            _backgroundWorker.RunWorkerAsync();
            
        }

        #endregion

        private void _theHelpButton_Click(object sender, EventArgs e)
        {
            if (_mode == LibraryOperation.CreateLibrary)
            {
                TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.UseCreateLibDialog);
            }
            else if (_mode == LibraryOperation.UpdateLibrary)
            {
                TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.UseAlterLibDialog);
            }
            
        }

    }
}
