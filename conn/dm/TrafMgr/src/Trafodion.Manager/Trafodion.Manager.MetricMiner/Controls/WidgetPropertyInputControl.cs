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
using System.IO;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.MetricMiner.Controls
{
    public partial class WidgetPropertyInputControl : UserControl
    {
        UniversalWidgetConfig _theConfig;
               
        /// <summary>
        /// Delegate for required fields changed.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="args"></param>
        public delegate void RequiredFieldsChanged(object sender, EventArgs args);

        /// <summary>
        /// Event for required fields changed.
        /// </summary>
        public event RequiredFieldsChanged OnRequiredFieldsChanged;

        public delegate void OptionalFieldsChanged(object sender, EventArgs args);

        public event OptionalFieldsChanged OnOptionalFieldsChanged;

        public WidgetPropertyInputControl()
        {
            InitializeComponent();

            //Also, monitor the query text for any changes
            this._theQueryInputControl.OnQueryTextChanged += _theQueryInputControl_OnQueryTextChanged;

            _theDrillDownMappingUserControl.OnFieldsChanged += new DrillDownMappingUserControl.FieldsChanged(_theDrillDownMappingUserControl_OnFieldsChanged);
        }

        void _theDrillDownMappingUserControl_OnFieldsChanged(object sender, EventArgs args)
        {
            FireOptionalFieldsChanged(new EventArgs());
        }

        public WidgetPropertyInputControl(UniversalWidgetConfig aUniversalWidgetConfig) : this()
        {
            _theConfig = aUniversalWidgetConfig;
            SetUIValues(aUniversalWidgetConfig);
        }

        public void UpdateUniversalWidgetConfig(UniversalWidgetConfig aUniversalWidgetConfig)
        {
            if (aUniversalWidgetConfig == null)
            {
                return;
            }
            string widgetFullName = Path.Combine(LibraryPath,ReportFileName);
            UniversalWidgetConfig temp = WidgetRegistry.GetInstance().GetWidgetForFileName(widgetFullName);
            if (temp != null)
            {
                aUniversalWidgetConfig.ReportID = temp.ReportID;
            }
            else
            {
                aUniversalWidgetConfig.ReportID = WidgetRegistry.GetWidgetID();
            }
            aUniversalWidgetConfig.ReportFileName = ReportFileName;//MM1
            aUniversalWidgetConfig.ReportPath = LibraryPath;            
            aUniversalWidgetConfig.Name         = WidgetName;       
            aUniversalWidgetConfig.Title        = Title   ;   
            aUniversalWidgetConfig.Description  = Description;
            aUniversalWidgetConfig.Author       = Author;
            aUniversalWidgetConfig.WidgetVersion = WidgetVersion;
            aUniversalWidgetConfig.ServerVersion = ServerVersion;
            DatabaseDataProviderConfig dbConfig = aUniversalWidgetConfig.DataProviderConfig as DatabaseDataProviderConfig;
            if (dbConfig != null)
            {
                dbConfig.SQLText = QueryText;
            }

        }

        public void SetUIValues(UniversalWidgetConfig aUniversalWidgetConfig)
        {
            _theConfig = aUniversalWidgetConfig;
            WidgetName = aUniversalWidgetConfig.Name;
            if (!string.IsNullOrEmpty(aUniversalWidgetConfig.ReportPath))
            {
                _theReportFileLocationComboBox.Text =
                                        Path.Combine(aUniversalWidgetConfig.ReportPath, aUniversalWidgetConfig.ReportFileName);
                                        //string.Format("{0}{1}{2}", aUniversalWidgetConfig.ReportPath, 
                                        //(!string.IsNullOrEmpty(aUniversalWidgetConfig.ReportPath) && aUniversalWidgetConfig.ReportPath.Trim().EndsWith("\\")) ? "" : "\\", 
                                        //aUniversalWidgetConfig.ReportFileName);
            }            
            DatabaseDataProviderConfig dbConfig = aUniversalWidgetConfig.DataProviderConfig as DatabaseDataProviderConfig;
            if (dbConfig != null)
            {
                QueryText = dbConfig.SQLText;
            }            
            Title = aUniversalWidgetConfig.Title;
            Description = aUniversalWidgetConfig.Description;
            Author = aUniversalWidgetConfig.Author;
            WidgetVersion = aUniversalWidgetConfig.WidgetVersion;
            ServerVersion = aUniversalWidgetConfig.ServerVersion;
            _theDrillDownMappingUserControl.Config = aUniversalWidgetConfig;
            SetLibraryPaths();
        }

        public void SetLibraryPaths()
        {
            foreach (string path in TabbedMetricMinerControl.TheLibraryManager.LibraryPaths)
            {
                LibraryPath = path;
            }
        }

        public string WidgetName
        {
            get {return GetTextToDisplay(this._theWidgetName.Text); }
            set { this._theWidgetName.Text = GetTextToDisplay(value); }
        }

        public string ReportFileName
        {
            get
            {
                if (_theReportFileLocationComboBox.Text != null && _theReportFileLocationComboBox.Text.Trim() != "")
                {
                    return _theReportFileLocationComboBox.Text.Substring(_theReportFileLocationComboBox.Text.LastIndexOf(@"\") + 1); 
                }
                else
                {
                    return "";
                }
            }
            set
            {
                _theReportFileLocationComboBox.Text = value;
            }
        }

        public void SetReportPathStatus(bool aButtonFlag, bool aPathFlag)
        {
            _theReportFileLocationComboBox.Enabled = aButtonFlag;
            _theWidgetName.Enabled = aButtonFlag;
            TrafodionButton1.Enabled = aButtonFlag;
            if (!aPathFlag)
            {
                string reportPath = TabbedMetricMinerControl.TheLibraryManager.LastUsedPath;
                if (!string.IsNullOrEmpty(reportPath) && !reportPath.Trim().EndsWith("\\"))
                {
                    reportPath += "\\";
                }
                if (!this._theReportFileLocationComboBox.Items.Contains(reportPath))
                {
                    _theReportFileLocationComboBox.Items.Add(reportPath);
                }
                _theReportFileLocationComboBox.SelectedItem = reportPath;
            }
        }

        public string Title
        {
            get { return GetTextToDisplay(this._theWidgetTitle.Text); }
            set { this._theWidgetTitle.Text = GetTextToDisplay(value); }
        }

        public string Description
        {
            get { return GetTextToDisplay(this._theDescription.Text); }
            set { this._theDescription.Text = GetTextToDisplay(value); }
        }

        public string Author
        {
            get { return GetTextToDisplay(this._theAuthor.Text); }
            set { this._theAuthor.Text = GetTextToDisplay(value); }
        }

        public string WidgetVersion
        {
            get { return GetTextToDisplay(this._theVersion.Text); }
            set { this._theVersion.Text = GetTextToDisplay(value); }
        }

        public string ServerVersion
        {
            get { return GetTextToDisplay(this._theServerVersion.Text); }
            set { this._theServerVersion.Text = GetTextToDisplay(value); }
        }
        public string QueryText
        {
            get { return GetTextToDisplay(this._theQueryInputControl.QueryText); }
            set { this._theQueryInputControl.QueryText = GetTextToDisplay(value); }
        }


        public string LibraryPath
        {
            //get { return GetTextToDisplay((string)this._theLocationCombo.SelectedItem); }MM1
            get
            {
                string reportPath = "";
                if (this._theReportFileLocationComboBox.Text != null && this._theReportFileLocationComboBox.Text != "")
                {
                    try
                    {
                        reportPath = Path.GetDirectoryName(_theReportFileLocationComboBox.Text);
                        if (reportPath.Equals(""))
                        {
                            reportPath = Path.GetPathRoot(_theReportFileLocationComboBox.Text);
                        }
                        
                    }
                    catch
                    {
                        reportPath = _theReportFileLocationComboBox.Text;
                    }
                    //if (_theReportFileLocationComboBox.Text.Contains(".widget") && _theReportFileLocationComboBox.Text.LastIndexOf("\\")>0)
                    //{
                    //    reportPath = this._theReportFileLocationComboBox.Text.Substring(0, _theReportFileLocationComboBox.Text.LastIndexOf("\\"));
                    //}
                    //else
                    //{
                    //    reportPath = _theReportFileLocationComboBox.Text;
                    //}
                }
                return reportPath;
            }
            set
            {
                string reportPath = value;
                //string fullReportName = (string.IsNullOrEmpty(reportPath)) ? "" : (reportPath + "\\" + ReportFileName);
                //string fullReportName = Path.Combine(reportPath, ReportFileName);
                string fullReportName = string.Format("{0}{1}{2}", reportPath, (!string.IsNullOrEmpty(reportPath) || !string.IsNullOrEmpty(ReportFileName)) ? "\\" : "", ReportFileName);
                if ( !string.IsNullOrEmpty(fullReportName) && !this._theReportFileLocationComboBox.Items.Contains(fullReportName))
                {
                    _theReportFileLocationComboBox.Items.Add(fullReportName);
                }
                _theReportFileLocationComboBox.SelectedItem = fullReportName;
            }
        }

        /// <summary>
        /// Validate any required fields and return the resaon of failing.
        /// Return null if everything is fine. 
        /// Note: If the reprot name is legal, the report name will be used as the file name.
        /// </summary>
        /// <returns></returns>
        public string ValidateRequiredFields()
        {
            if (string.IsNullOrEmpty(WidgetName))
            {
                return string.Format(Properties.Resources.ErrorMissingRequiredField, Properties.Resources.TheReportNameField);
            }
            else
            {
                try
                {
                    //Some of these combination of characters are not legal to be in the path name, 
                    //so do a pre-screen first. 
                    if (WidgetName.IndexOfAny(System.IO.Path.GetInvalidFileNameChars()) != -1 ||
                          WidgetName.IndexOfAny(System.IO.Path.GetInvalidPathChars()) != -1)
                    {
                        return Properties.Resources.ErrorInvalidateFilePathName;
                    }

                    if (!Directory.Exists(LibraryPath)) return Properties.Resources.ErrorNotExistedFilePath;

                    if (!string.IsNullOrEmpty(Path.GetExtension(ReportFileName)) && Path.GetExtension(ReportFileName).Trim().ToUpper().Equals(".WIDGET"))
                    {
                        //valid extension in the path
                    }
                    else
                    {
                        return Properties.Resources.ErrorNotValidExtensionName;
                    }

                    //string reportName = Path.Combine(LibraryPath, WidgetName + ".widget");
                    //if (this._theReportFileLocationComboBox.Enabled)
                    //{
                    //    this._theReportFileLocationComboBox.Text = reportName;
                    //}
                }
                catch (ArgumentException)
                {
                    return Properties.Resources.ErrorInvalidateFilePathName;
                }
            }

            if (string.IsNullOrEmpty(QueryText))
            {
                return string.Format(Properties.Resources.ErrorMissingRequiredField, Properties.Resources.TheQueryTextField);
            }

            return null;
        }

        /// <summary>
        /// private dispose method, which manually cleanup everything locally.
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {
            //cleanup
            if (disposing)
            {
                this._theQueryInputControl.OnQueryTextChanged -= _theQueryInputControl_OnQueryTextChanged;
                this._theTabControl.SelectedIndexChanged -= this._theTabControl_SelectedIndexChanged;
                _theDrillDownMappingUserControl.OnFieldsChanged -= _theDrillDownMappingUserControl_OnFieldsChanged;
            }
        }

        private string GetTextToDisplay(string text)
        {
            return (text != null) ? text.Trim() : "";
        }

        private void _theLibraryBrowserButton_Click(object sender, EventArgs e)
        {
            FolderBrowserDialog dlg = new FolderBrowserDialog();
            dlg.Description = "Select the folder where you want to save this widget";
            dlg.ShowNewFolderButton = true;
            dlg.RootFolder = Environment.SpecialFolder.MyComputer;
            if (LibraryManager.Instance.LastUsedPath != null)
            {
                dlg.SelectedPath = LibraryManager.Instance.LastUsedPath;
            }
            if (dlg.ShowDialog() == DialogResult.OK)
            {
                string path = dlg.SelectedPath;
                LibraryPath = path;
            }

        }

        void _theQueryInputControl_OnQueryTextChanged(object aSender, EventArgs e)
        {
            FireRequiredFieldsChanged(e);
        }

        private void _theTabControl_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (_theTabControl.SelectedIndex == 2)
            {
                this.UpdateUniversalWidgetConfig(_theConfig);
            }
        }

        private bool changeFlag = false;
        private void TrafodionButton1_Click(object sender, EventArgs e)
        {
            string fileName = WidgetName + ".widget";
            string folderName = "";
            if (LibraryManager.Instance.LastUsedPath != null)
            {
                folderName = LibraryManager.Instance.LastUsedPath;
            }

            SaveFileDialog saveFileDialog = new SaveFileDialog();
            saveFileDialog.DefaultExt = ".widget";
            saveFileDialog.Filter = "Widget files(*.widget)|*.widget";
            saveFileDialog.InitialDirectory = folderName;
            saveFileDialog.FileName = fileName.Substring(fileName.LastIndexOf(@"\") + 1);
            if (saveFileDialog.ShowDialog() == DialogResult.OK)
            {
                string temp = saveFileDialog.FileName;

                if (!string.IsNullOrEmpty(temp) && !this._theReportFileLocationComboBox.Items.Contains(temp))
                {
                    _theReportFileLocationComboBox.Items.Add(temp);
                    _theReportFileLocationComboBox.SelectedItem = temp;
                    changeFlag = true;
                }
            }
        }

        private void _theWidgetName_TextChanged(object sender, EventArgs e)
        {
            if (_theReportFileLocationComboBox.Enabled)
            {
                //try
                //{
                if (WidgetName.IndexOfAny(System.IO.Path.GetInvalidPathChars()) == -1 &&
                    WidgetName.IndexOfAny(System.IO.Path.GetInvalidFileNameChars())==-1)
                {
                    this._theReportFileLocationComboBox.Text =
                        //string.Format("{0}{1}{2}.widget", LibraryPath, (!string.IsNullOrEmpty(LibraryPath) && LibraryPath.Trim().EndsWith("\\")) ? "" : "\\", WidgetName);
                        Path.Combine(LibraryPath, WidgetName + ".widget");
                }
                //}
                //catch (ArgumentException)
                //{        
                //    MessageBox.Show(Utilities.GetForegroundControl(),
                //        Properties.Resources.ErrorInvalidateFilePathName,
                //        string.Format(global::Trafodion.Manager.Properties.Resources.ErrorTitle, global::Trafodion.Manager.Properties.Resources.ProductName, global::Trafodion.Manager.Properties.Resources.MetricMiner),
                //        MessageBoxButtons.OK,
                //        MessageBoxIcon.Error);
                //}
            }

            //FireRequiredFieldsChanged(new EventArgs());
        }

        /// <summary>
        /// Fire events if required field changed.
        /// </summary>
        /// <param name="e"></param>
        private void FireRequiredFieldsChanged(EventArgs e)
        {
            if (OnRequiredFieldsChanged != null)
            {
                OnRequiredFieldsChanged(this, e);
            }
        }

        private void _theWidgetTitle_TextChanged(object sender, EventArgs e)
        {
            FireOptionalFieldsChanged(new EventArgs());
        }

        private void _theDescription_TextChanged(object sender, EventArgs e)
        {
            FireOptionalFieldsChanged(new EventArgs());
        }

        private void _theAuthor_TextChanged(object sender, EventArgs e)
        {
            FireOptionalFieldsChanged(new EventArgs());
        }

        private void _theVersion_TextChanged(object sender, EventArgs e)
        {
            FireOptionalFieldsChanged(new EventArgs());
        }

        private void _theServerVersion_TextChanged(object sender, EventArgs e)
        {
            FireOptionalFieldsChanged(new EventArgs());
        }

        private void FireOptionalFieldsChanged(EventArgs e)
        {
            if (OnOptionalFieldsChanged != null)
            {
                OnOptionalFieldsChanged(this, e);
            }
        }

        private void _theReportFileLocationComboBox_TextChanged(object sender, EventArgs e)
        {
            if (OnRequiredFieldsChanged != null)
            {
                OnRequiredFieldsChanged(this, e);
            }
        }


    }
}
