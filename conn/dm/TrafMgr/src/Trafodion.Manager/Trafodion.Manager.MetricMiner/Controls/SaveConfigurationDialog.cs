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
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;

namespace Trafodion.Manager.MetricMiner.Controls
{
    public partial class SaveConfigurationDialog : TrafodionForm
    {
        UniversalWidgetConfig _theConfig;
        DialogResult _theSelectedOption = DialogResult.None;



        public WidgetPropertyInputControl TheWidgetPropertyInputControl
        {
            get { return _theWidgetPropertyInputControl; }
        }



        public DialogResult SelectedOption
        {
            get { return _theSelectedOption; }
        }

        /// <summary>
        /// StatusMessage: the error message to be displayed.
        /// </summary>
        public string StatusMessage
        {
            get { return _theStatusTextBox.Text; }
            set
            {
                _theStatusTextBox.Text = value;
                if (String.IsNullOrEmpty(value))
                {
                    _theStatusTextBox.Visible = false;
                }
                else
                {
                    _theStatusTextBox.Visible = true;
                }
            }
        }

        public SaveConfigurationDialog()
        {
            InitializeComponent();
            //make sure the path is clean in the beginning.
            this._theWidgetPropertyInputControl.LibraryPath = "";
            this.Text = "Report Configuration";
            StartPosition = FormStartPosition.CenterParent;
            //StatusMessage = _theWidgetPropertyInputControl.ValidateRequiredFields();
            _theWidgetPropertyInputControl.OnRequiredFieldsChanged += HandleRequiredFieldsChanged;
            _theWidgetPropertyInputControl.OnOptionalFieldsChanged += HandleOptionalFieldsChanged;
            this.Disposed += new EventHandler(SaveConfigurationDialog_Disposed);
        }

        public SaveConfigurationDialog(UniversalWidgetConfig aConfig) :this()
        {
            _theConfig = aConfig;
            _theWidgetPropertyInputControl.SetUIValues(_theConfig);
            _theSaveButton.Enabled = true;
            StatusMessage = _theWidgetPropertyInputControl.ValidateRequiredFields();
        }

        public string LibraryPath
        {
            get { return _theWidgetPropertyInputControl.LibraryPath; }
            set { _theWidgetPropertyInputControl.LibraryPath = value; }
        }

        public void SetReportPathStatus(bool aButtonFlag, bool aPathFlag)
        {
            _theWidgetPropertyInputControl.SetReportPathStatus(aButtonFlag, aPathFlag);
        }

        private void SaveConfigurationDialog_Disposed(object sender, EventArgs e)
        {
            _theWidgetPropertyInputControl.OnRequiredFieldsChanged -= HandleRequiredFieldsChanged;
            _theWidgetPropertyInputControl.OnOptionalFieldsChanged -= HandleOptionalFieldsChanged;
        }

        private void _theSaveButton_Click(object sender, EventArgs e)
        {
            _theWidgetPropertyInputControl.UpdateUniversalWidgetConfig(_theConfig);
            foreach (AssociatedWidgetConfig awc in _theConfig.AssociatedWidgets)
            {
                awc.CallingWidgetID = _theConfig.ReportID;
                awc.CallingWidgetName = _theConfig.Name;
            }
            _theSelectedOption = DialogResult.OK;
            this.Hide();
        }

        private void _theCancelButton_Click(object sender, EventArgs e)
        {
            this.Hide();
        }

        /// <summary>
        /// Event handler for required fields changed event
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="args"></param>
        private void HandleRequiredFieldsChanged(object sender, EventArgs args)
        {
            StatusMessage = _theWidgetPropertyInputControl.ValidateRequiredFields();
            _theSaveButton.Enabled = string.IsNullOrEmpty(StatusMessage);
        }

        private void HandleOptionalFieldsChanged(object sender, EventArgs args)
        {
            StatusMessage = _theWidgetPropertyInputControl.ValidateRequiredFields();
            if (string.IsNullOrEmpty(StatusMessage))
            {
                _theSaveButton.Enabled = true;
            }
            else
            {
                _theSaveButton.Enabled = false;
            }
        }

    }
}
