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
using System.Windows.Forms;
using Trafodion.Manager.UniversalWidget;

namespace Trafodion.Manager.MetricMiner.Controls
{
    public class AdhocReportTabContent : MetricMinerReportTabContent
    {
        public AdhocReportTabContent()
            : base()
        {
        }

        public AdhocReportTabContent(UniversalWidgetConfig aUniversalWidgetConfig)
            : base(aUniversalWidgetConfig)
        {
            aUniversalWidgetConfig.Name = "";
            aUniversalWidgetConfig.Title = "";
        }

        //We don't want the drill down 
        protected override void AddMenuItems()
        {
        }

        protected override void runButton_Click(object sender, EventArgs e)
        {
            //DatabaseDataProviderConfig dbDataProviderConfig = this._theWidget.UniversalWidgetConfiguration.DataProviderConfig as DatabaseDataProviderConfig;
            ////if the SQL is changed, we want to reset all mappings from the previous execution
            //if (!_theQueryInputControl.QueryText.Equals(dbDataProviderConfig.SQLText))
            //{
            //    dbDataProviderConfig.DefaultVisibleColumnNames = null;
            //    dbDataProviderConfig.ColumnSortObjects = null;
            //    dbDataProviderConfig.ColumnMappings = null;
            //    _theTabularDataDisplayControl.DataGrid.Clear();
            //    _theTabularDataDisplayControl.DataGrid.Cols.Clear();
            //    _theTabularDataDisplayControl.DataGrid.CurrentFilter = null;
            //}

            base.runButton_Click(sender, e);
        }
        /// <summary>
        /// Add custom tool strip buttons 
        /// </summary>
        protected override void AddToolStripButtons()
        {

            runButton = new ToolStripButton();
            runButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.ImageAndText;
            runButton.Image = (System.Drawing.Image)global::Trafodion.Manager.Properties.Resources.RunIcon;
            runButton.ImageTransparentColor = System.Drawing.Color.Transparent;
            runButton.Name = "runButton";
            runButton.Size = new System.Drawing.Size(23, 22);
            runButton.Text = "Execute";
            runButton.ToolTipText = "Execute the SQL statement";
            runButton.Click += new EventHandler(runButton_Click);
            this._theWidget.AddToolStripItem(runButton);

            explainButton = new ToolStripButton();
            explainButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.ImageAndText;
            explainButton.Image = (System.Drawing.Image)global::Trafodion.Manager.Properties.Resources.SqlPlanIcon;
            explainButton.ImageTransparentColor = System.Drawing.Color.Transparent;
            explainButton.Name = "explainButton";
            explainButton.Text = "Explain";
            explainButton.Click += new EventHandler(explainButton_Click);
            this._theWidget.AddToolStripItem(explainButton);

            this._theWidget.AddToolStripItem(new ToolStripSeparator());

            ToolStripButton saveButton = new ToolStripButton();
            saveButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            saveButton.Image = (System.Drawing.Image)global::Trafodion.Manager.Properties.Resources.NewReportIcon;
            saveButton.ImageTransparentColor = System.Drawing.Color.Transparent;
            saveButton.Name = "saveButton";
            saveButton.Size = new System.Drawing.Size(23, 22);
            saveButton.Text = "Saves the report configuration";
            saveButton.Click += new EventHandler(saveButton_Click);
            this._theWidget.AddToolStripItem(saveButton);

            ToolStripButton cancelButton = new ToolStripButton();
            cancelButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            cancelButton.Image = (System.Drawing.Image)global::Trafodion.Manager.Properties.Resources.CloseIcon;
            cancelButton.ImageTransparentColor = System.Drawing.Color.Transparent;
            cancelButton.Name = "cancelButton";
            cancelButton.Size = new System.Drawing.Size(23, 22);
            cancelButton.Text = "Closes the report tab";
            cancelButton.Alignment = ToolStripItemAlignment.Right;
            cancelButton.Click += new EventHandler(cancelButton_Click);
            this._theWidget.AddToolStripItem(cancelButton);

            this._theWidget.AddToolStripItem(new ToolStripSeparator());

            //ToolStripButton historyButton = new ToolStripButton();
            //historyButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            //historyButton.Image = (System.Drawing.Image)global::Trafodion.Manager.Properties.Resources.HistoryIcon;
            //historyButton.ImageTransparentColor = System.Drawing.Color.Transparent;
            //historyButton.Name = "historyButton";
            //historyButton.Size = new System.Drawing.Size(23, 22);
            //historyButton.Text = "Displays the history of all queries executed";
            //historyButton.Click += new EventHandler(historyButton_Click);
            //this._theWidget.AddToolStripItem(historyButton);

            ToolStripButton FullScreenButton = new ToolStripButton();
            FullScreenButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            FullScreenButton.Image = (System.Drawing.Image)global::Trafodion.Manager.Properties.Resources.FullScreenIcon;
            FullScreenButton.ImageTransparentColor = System.Drawing.Color.Transparent;
            FullScreenButton.Name = "FullScreenButton";
            FullScreenButton.Size = new System.Drawing.Size(23, 22);
            FullScreenButton.Text = "Expand the tab to occupy the full screen";
            FullScreenButton.Click += new EventHandler(FullScreenButton_Click);
            this._theWidget.AddToolStripItem(FullScreenButton);


            //ToolStripDropDownButton exportButtons = new ToolStripDropDownButton();
            //exportButtons.DisplayStyle = ToolStripItemDisplayStyle.Image;
            //exportButtons.Image = (System.Drawing.Image)global::Trafodion.Manager.Properties.Resources.Export;
            //exportButtons.DropDownItems.Add(getToolStripMenuItemForExport("Clipboard", 1));
            //exportButtons.DropDownItems.Add(getToolStripMenuItemForExport("Browser", 2));
            //exportButtons.DropDownItems.Add(getToolStripMenuItemForExport("Spreadsheet", 3));
            //exportButtons.DropDownItems.Add(getToolStripMenuItemForExport("File", 4));
            //exportButtons.ImageTransparentColor = System.Drawing.Color.Transparent;
            //exportButtons.Name = "exportButtons";
            //exportButtons.Size = new System.Drawing.Size(23, 22);
            //exportButtons.Text = "Export data to the selected format";
            //this._theWidget.AddToolStripItem(exportButtons);

            ToolStripButton sqlDesigner = new ToolStripButton();
            sqlDesigner.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
            sqlDesigner.ImageTransparentColor = System.Drawing.Color.Transparent;
            sqlDesigner.Name = "sqlDesigner";
            sqlDesigner.Size = new System.Drawing.Size(23, 22);
            sqlDesigner.Text = "SQL Designer";
            sqlDesigner.ToolTipText = "Displays the SQL Designer";
            sqlDesigner.Click += new EventHandler(sqlDesigner_Click);
            this._theWidget.AddToolStripItem(sqlDesigner);

        }

        protected override void saveButton_Click(object sender, EventArgs e)
        {
            setUniversalWidget();
            //used to control report path and browse button status. it's different from non adhoc report.
            saveReport(true, false, false);

        }

        //the ad hoc report property window should be displayed in empty.
        private void setUniversalWidget()
        {
            Config.Name = "";
            Config.Title = "";
            Config.Description = "";
            Config.Author = "";
            Config.WidgetVersion = "";
            Config.ServerVersion = "";
        }
    }
}
