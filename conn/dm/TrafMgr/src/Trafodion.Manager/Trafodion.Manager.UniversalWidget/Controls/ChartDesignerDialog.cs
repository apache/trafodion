//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
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
using System.Data;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.UniversalWidget.Controls
{
    /// <summary>
    /// Dialog for chart designer
    /// </summary>
    public partial class ChartDesignerDialog : TrafodionForm
    {
        #region Fields

        private GenericUniversalWidget _theWidget = null;
        private ChartDesignerUserControl _theUserControl = null;
        private DataTable _theDataTable = null;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: DataTable for populating the charts; 
        /// </summary>
        public DataTable DataTable
        {
            get { return _theDataTable; }
            set { _theDataTable = value; }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// The constructor
        /// Note: Make sure the widget does support charts.
        /// </summary>
        /// <param name="aWidget"></param>
        public ChartDesignerDialog(GenericUniversalWidget aWidget)
        {
            // defensive code to catch programming error here. 
            if (!aWidget.UniversalWidgetConfiguration.SupportCharts)
            {
                throw new Exception("Error: widget does not support charts, invoking Chart Designer is not valid.");
            }

            _theWidget = aWidget;
            _theDataTable = (_theWidget != null && _theWidget.DataProvider != null) ? _theWidget.DataProvider.GetDataTable() : null;
            InitializeComponent();
            this.Text = string.Format("{0} - {1}", global::Trafodion.Manager.Properties.Resources.ProductName, Properties.Resources.ChartDesigner);
            ShowWidget();
            CenterToParent();
        }

        #endregion Constructors

        #region Public methods

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// Show the widget
        /// </summary>
        private void ShowWidget()
        {
            _theUserControl = new ChartDesignerUserControl(_theWidget.UniversalWidgetConfiguration.ChartConfig);
            _theUserControl.Dock = DockStyle.Fill;
            _theUpperPanel.Controls.Add(_theUserControl);
            _theUserControl.OnStatusChanged += new ChartDesignerUserControl.StatusChanged(_theUserControl_OnStatusChanged);
        }

        void _theUserControl_OnStatusChanged(object sender, EventArgs e)
        {
            if (string.IsNullOrEmpty(_theUserControl.StatusMessage) || _theUserControl.StatusMessage.StartsWith("Warning"))
            {
                _theSaveButton.Enabled = true;
            }
            else
            {
                _theSaveButton.Enabled = false;
            }
        }

        /// <summary>
        /// Event handler for Ok button click 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theSaveButton_Click(object sender, EventArgs e)
        {
            if (!_theUserControl.Apply(_theDataTable)) return;

            // Also, set the new config to the widget's chart
            _theWidget.ChartControl.ChartConfiguration = _theUserControl.ChartConfig;

            // Save it to the universal widget config so that the chart config will be saved to file.
            _theWidget.UniversalWidgetConfiguration.ChartConfig = _theUserControl.ChartConfig;

            if (_theDataTable != null)
            {
                _theWidget.ChartControl.PopulateChart(_theDataTable);
            }

            DialogResult = System.Windows.Forms.DialogResult.OK;
            this.Hide();
        }

        /// <summary>
        /// Event handler for apply button click
        /// Note: the apply button does not close the dialog, it only applys the changes to the chart
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theApplyButton_Click(object sender, EventArgs e)
        {
            if (!_theUserControl.Apply(_theDataTable)) return;

            // Also, set the new config to the widget's chart
            _theWidget.ChartControl.ChartConfiguration = _theUserControl.ChartConfig;

            if (_theDataTable != null)
            {
                _theWidget.ChartControl.PopulateChart(_theDataTable);
            }
        }

        /// <summary>
        /// Event handler for cancel button click
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theCancelButton_Click(object sender, EventArgs e)
        {
            _theResetButton_Click(sender, e);
            DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.Hide();
        }

        private void _theResetButton_Click(object sender, EventArgs e)
        {
            // Reload the chart config saved in the user control before it was changed. 
            _theUserControl.Reset();

            // Also, reset the widget's chart control
            //_theWidget.ChartControl.ChartConfiguration = _theUserControl.ChartConfig;
            _theWidget.UniversalWidgetConfiguration.ChartConfig = _theUserControl.ChartConfig;
            _theWidget.ResetChartControl();

            // The re-populate the chart
            if (_theDataTable != null)
            {
                _theWidget.ChartControl.PopulateChart(_theDataTable);
            }
        }

        #endregion Private methods

        private void _helpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic("Use-Charts.html#See-the-Parts-of-the-Chart-Designer");
        }
    }
}
