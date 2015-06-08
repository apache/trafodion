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
    public partial class ConfigureMappingDialog : TrafodionForm
    {
        UniversalWidgetConfig _theConfig;
        DialogResult _theSelectedOption = DialogResult.None;
        AssociatedWidgetConfig _theAssociatedWidgetConfig = null;
        public DialogResult SelectedOption
        {
            get { return _theSelectedOption; }
        }

        public AssociatedWidgetConfig ConfiguredAssociatedWidgetConfig
        {
            get { return _theAssociatedWidgetConfig; }
        }

        public ConfigureMappingDialog()
        {
            InitializeComponent();
            Text = "Link Reports";
            StartPosition = FormStartPosition.CenterParent;
            _theAssociatedWidgetConfig = null;
        }

        public ConfigureMappingDialog(UniversalWidgetConfig aConfig) :this()
        {
            _theConfig = aConfig;
            _theMappingUserControl.OnButtonStatus += new ConfigureMappingUserControl.ButtonStatus(_theMappingUserControl_OnButtonStatus);
            _theMappingUserControl.Config =  _theConfig;
        }

        void _theMappingUserControl_OnButtonStatus(object sender, EventArgs e)
        {
            //if there's no avalable linking report then the save button should not be enabled.
            if (_theMappingUserControl.LinkedReportGrid.Rows.Count > 0)
            {
                _theSaveButton.Enabled = true;
            }
            else
            {
                _theSaveButton.Enabled = false;
            }
        }

        public AssociatedWidgetConfig GetAssociatedWidgetConfig()
        {
            return _theMappingUserControl.getAssociatedWidgetConfig();
        }

        public void SetAssociatedWidgetConfigForEdit(AssociatedWidgetConfig aAssociatedWidgetConfig)
        {
            _theMappingUserControl.SetAssociatedWidgetConfigForEdit(aAssociatedWidgetConfig);
        }

        private void _theSaveButton_Click(object sender, EventArgs e)
        {
            if (_theMappingUserControl.LinkedReportGrid.SelectedRows.Count == 0)
            {
                MessageBox.Show("Please select the report you want to link.", "Add Mapping", MessageBoxButtons.OK);
                return;
            }
            _theSelectedOption = DialogResult.OK;
            _theAssociatedWidgetConfig = _theMappingUserControl.getAssociatedWidgetConfig();
            if (_theAssociatedWidgetConfig != null)
            {
                AssociatedWidgetConfig assocConfig = _theConfig.GetAssociationByID(_theConfig.ReportID, _theAssociatedWidgetConfig.CalledWidgetID);
                if (assocConfig != null)
                {
                    _theConfig.AssociatedWidgets.Remove(assocConfig);
                }
                _theConfig.AssociatedWidgets.Add(_theAssociatedWidgetConfig);
                WidgetRegistry.GetInstance().UpdateWidgetToRegistry(_theConfig);
            }

            this.Hide();
        }

        private void _theCancelButton_Click(object sender, EventArgs e)
        {
            this.Hide();
        }


    }
}
