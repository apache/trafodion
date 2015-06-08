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
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.MetricMiner.Controls
{
    public partial class MetricMinerNavigator : UserControl
    {
        public delegate void OnReportSelected(UniversalWidgetConfig aConfig, TabPage aTabPage);
        private OnReportSelected _ReportSelectedImpl;
        private HistorylogPanel _theHistorylogPanel;
        private string windowTitle = "Metric Miner history";
        private TabbedMetricMinerControl _theMetricMinerControl = null;

        public TabbedMetricMinerControl TheMetricMinerControl
        {
            get { return _theMetricMinerControl; }
            set { _theMetricMinerControl = value; }
        }


        public MetricMinerNavigator()
        {
            InitializeComponent();
            AddToolStripButtons();
            _theQueriesCombo.Visible = false;
        }

        //Delegate that will be invoked when a report is selected
        public OnReportSelected ReportSelectedImpl
        {
            get { return _ReportSelectedImpl; }
            set { _ReportSelectedImpl = value; }
        }

        //Used to add a reference to the tab page to the navigator
        public void AddReportToCombo(UniversalWidgetConfig aConfig, TabPage aTabPage)
        {
            NavigatorComboItem navItem = new NavigatorComboItem(aConfig, aTabPage);
            
            //Remove the report if it exists
            RemoveReportFromCombo(aConfig, aTabPage);

            //Insert the new report to the top of the list
            _theQueriesCombo.Items.Insert(0,navItem);

            //Select the last inserted item
            _theQueriesCombo.SelectedItem = navItem;
        }
        
        //Remove the reference of the tab from the navigator
        public void RemoveReportFromCombo(UniversalWidgetConfig aConfig, TabPage aTabPage)
        {
            foreach(Object item in _theQueriesCombo.Items)
            {
                NavigatorComboItem navItem = item as NavigatorComboItem;
                if (navItem.TabPage == aTabPage)
                {
                    _theQueriesCombo.Items.Remove(navItem);
                    break;
                }
            }
        }

        private void _theQueriesCombo_SelectedIndexChanged(object sender, EventArgs e)
        {
            NavigatorComboItem selectedItem = _theQueriesCombo.SelectedItem as NavigatorComboItem;
            if ((selectedItem != null) && (_ReportSelectedImpl != null))
            {
                _ReportSelectedImpl(selectedItem.Config, selectedItem.TabPage);
            }
        }

        /// <summary>
        /// Add custom tool strip buttons 
        /// </summary>
        protected virtual void AddToolStripButtons()
        {
            ToolStripButton historyButton = new ToolStripButton();
            historyButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            historyButton.Image = (System.Drawing.Image)global::Trafodion.Manager.Properties.Resources.HistoryIcon;
            historyButton.ImageTransparentColor = System.Drawing.Color.Transparent;
            historyButton.Name = "historyButton";
            historyButton.Size = new System.Drawing.Size(23, 22);
            historyButton.Text = "Displays the history of all queries executed";
            historyButton.Click += new EventHandler(historyButton_Click);

           _theToolStrip.Items.Add(historyButton);

           ToolStripButton closeAllButton = new ToolStripButton();
           closeAllButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.ImageAndText;
           closeAllButton.Image = (System.Drawing.Image)global::Trafodion.Manager.Properties.Resources.CloseIcon;
           closeAllButton.ImageTransparentColor = System.Drawing.Color.Transparent;
           closeAllButton.Name = "closeAll";
           closeAllButton.Size = new System.Drawing.Size(23, 22);
           closeAllButton.Text = "Close All Reports";
           closeAllButton.Click += new EventHandler(closeAllButton_Click);

           _theToolStrip.Items.Add(closeAllButton);

        }

        void closeAllButton_Click(object sender, EventArgs e)
        {
            _theMetricMinerControl.CloseAllReports();
         
        }

        protected virtual void historyButton_Click(object sender, EventArgs e)
        {
            if (WindowsManager.Exists(windowTitle))
            {
                WindowsManager.BringToFront(windowTitle);
            }
            else
            {
                _theHistorylogPanel = new HistorylogPanel();
                _theHistorylogPanel.OnRowDoubleClickedEvent += new HistorylogPanel.OnRowDoubleClicked(_theHistorylogPanel_OnRowDoubleClickedEvent);
                WindowsManager.PutInWindow(new System.Drawing.Size(800, 600), _theHistorylogPanel, windowTitle, null);
            }
        }

        void _theHistorylogPanel_OnRowDoubleClickedEvent(HistorylogElement hle)
        {
            this._theMetricMinerControl.SetReportFromHistory(hle);
        }
    }


    public class NavigatorComboItem
    {
        UniversalWidgetConfig _theConfig;
        TabPage _theTabPage;
        public NavigatorComboItem(UniversalWidgetConfig aConfig, TabPage aTabPage)
        {
            _theConfig = aConfig;
            _theTabPage = aTabPage;
        }

        public override string ToString()
        {
            return _theConfig.DataProviderConfig.ConnectionDefinition.Name + " - " + WidgetRegistry.GetWidgetDisplayName(_theConfig);
        }

        public TabPage TabPage
        {
            get { return _theTabPage; }
            set { _theTabPage = value; }
        }

        public UniversalWidgetConfig Config
        {
            get { return _theConfig; }
            set { _theConfig = value; }
        }

    }
}
