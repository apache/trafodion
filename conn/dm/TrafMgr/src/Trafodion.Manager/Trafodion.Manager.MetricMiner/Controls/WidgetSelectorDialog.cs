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
using Trafodion.Manager.DatabaseArea.Queries;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.MetricMiner.Controls
{
    public partial class WidgetSelectorDialog : TrafodionForm
    {
        string selectedWidget = null;
        int initialHeight = 80;

        public string SelectedWidget
        {
            get { return selectedWidget; }
            set { selectedWidget = value; }
        }

        public List<ReportParameter> GetUpdatedParameters()
        {
            return _theReportSelectorControl.GetUpdatedParameters();
        }

        public WidgetSelectorDialog(UniversalWidgetConfig aConfig, WidgetLinkerObject widgetLinker)
        {
            InitializeComponent();
            Text = "Select Report";
            _theReportSelectorControl.OnButtonStatus += new ReportSelectorControl.ButtonStatus(_theReportSelectorControl_OnButtonStatus);
            _theReportSelectorControl.PopulateUI(aConfig, widgetLinker);
            StartPosition = FormStartPosition.CenterParent;
            //this.ClientSize = new Size(this.Width, initialHeight + _theReportSelectorControl.Height);
            //this._theAssociatedWidgets.Items.Clear();
            //this._theAssociatedWidgets.Items.Add("-- Select--");
            //foreach (AssociatedWidgetConfig associatedWidget in aConfig.AssociatedWidgets)
            //{
            //    this._theAssociatedWidgets.Items.Add(associatedWidget.CalledWidgetName);
            //}
            //this._theAssociatedWidgets.SelectedIndex = 0;

            //control ok button status
            _theReportSelectorControl_OnButtonStatus(null,null);
        }

        void _theReportSelectorControl_OnButtonStatus(object sender, EventArgs e)
        {
            if (_theReportSelectorControl.LinkedReportGrid.Rows.Count > 0 && _theReportSelectorControl.IsReportParametersValid)
            {
                _theOkButton.Enabled = true;
            }
            else
            {
                _theOkButton.Enabled = false;
            }
        }

        private void _theOkButton_Click(object sender, EventArgs e)
        {
            selectedWidget = null;
            _theReportSelectorControl.UpdateParamsFromUserInput();
            AssociatedWidgetConfig assocConfig = _theReportSelectorControl.GetSelectedReport();
            if (assocConfig != null)
            {
                selectedWidget = assocConfig.CalledWidgetID;
            }
            //if ((_theAssociatedWidgets.SelectedItem != null) && (_theAssociatedWidgets.SelectedItem.Equals("-- Select--")))
            //{
            //    selectedWidget = null;
            //}
            //selectedWidget = _theAssociatedWidgets.SelectedItem as String;
            this.Close();
        }

        private void _theCancelButton_Click(object sender, EventArgs e)
        {
            selectedWidget = null;
            this.Close();
        }
    }
}
