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
using Trafodion.Manager.LiveFeedFramework.Models;
using Trafodion.Manager.OverviewArea.Models;
using TenTec.Windows.iGridLib;
using Trafodion.Manager.DatabaseArea.Controls;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Connections.Controls;

namespace Trafodion.Manager.OverviewArea.Controls
{
    public partial class HealthPopupDialog : TrafodionForm
    {
        public enum HealthState { OK = 0, WARNING = 1, ERROR = 2 };
        public ImageList HealthStateImageList = new ImageList();
        TrafodionIGridHyperlinkCellManager _hyperLinkCellManager = new TrafodionIGridHyperlinkCellManager();
        iGCellClickEventHandler _cellClickHandler = null;
        // default is no column has underline link.
        int HyperlinkColIndex = -1;

        public HealthPopupDialog()
        {
            InitializeComponent();
            CenterToScreen();
        }

        public HealthPopupDialog(string aPublication, DataTable argDataTable, DateTime aCurrentTS)
            :this()
        {
            string argLayer = "";
            switch (aPublication)
            {
                case LiveFeedRoutingKeyMapper.PUBS_health_state_access_layer:
                    argLayer = SystemMetricModel.HealthLayerDisplayNames[(int)SystemMetricModel.HealthLayer.Access];
                    break;
                case LiveFeedRoutingKeyMapper.PUBS_health_state_database_layer:
                    argLayer = SystemMetricModel.HealthLayerDisplayNames[(int)SystemMetricModel.HealthLayer.Database];
                    break;
                case LiveFeedRoutingKeyMapper.PUBS_health_state_foundation_layer:
                    argLayer = SystemMetricModel.HealthLayerDisplayNames[(int)SystemMetricModel.HealthLayer.Foundation];
                    break;
                case LiveFeedRoutingKeyMapper.PUBS_health_state_os_layer:
                    argLayer = SystemMetricModel.HealthLayerDisplayNames[(int)SystemMetricModel.HealthLayer.OS];
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_health_state_server_layer:
                    argLayer = SystemMetricModel.HealthLayerDisplayNames[(int)SystemMetricModel.HealthLayer.Server];
                    break;
                case LiveFeedRoutingKeyMapper.PUBS_health_state_storage_layer:
                    argLayer = SystemMetricModel.HealthLayerDisplayNames[(int)SystemMetricModel.HealthLayer.Storage];
                    break;
            }

            this.Text = TitlePrefix + " Health/State : " + argLayer + " Layer";
            _theCurrentTimestampLabel.Text = string.Format("Last Reported Time: {0}", aCurrentTS.ToString(Utilities.STANDARD_DATETIME_FORMAT));
            InitializeWidget(argLayer, argDataTable);
        }

        private void InitializeWidget(string argLayer, DataTable argDataTable) 
        {
            DataTable subjectTable = new DataTable();
            subjectTable.Columns.Add("Subject Area", typeof(string));
            subjectTable.Columns.Add("State", typeof(long));
            subjectTable.Columns.Add("State Change Time", typeof(string));

            HealthStateImageList.Images.Add(Trafodion.Manager.Properties.Resources.GreenIcon);
            HealthStateImageList.Images.Add(Trafodion.Manager.Properties.Resources.YellowIcon);
            HealthStateImageList.Images.Add(Trafodion.Manager.Properties.Resources.RedIcon);
            HealthStateImageList.Images.Add(Trafodion.Manager.Properties.Resources.UnknownIcon);
            TrafodionIGridSubjectArea.ImageList = HealthStateImageList;

            TrafodionLabelLayer.Text = string.Format ("Health/State for {0} Layer", argLayer);
            //TrafodionIGridSubjectArea.FillWithData(argDataTable);
            TrafodionIGridSubjectArea.Visible = true;
            TrafodionIGridSubjectArea.AutoResizeCols = true;
            TrafodionIGridSubjectArea.AutoWidthColMode = iGAutoWidthColMode.HeaderAndCells;

            foreach (DataRow row in argDataTable.Rows)
            {
                subjectTable.Rows.Add(new object[] { row["subject_name"], 
                                        row["subject_current_score"], 
                                        Utilities.GetFormattedTimeFromUnixTimestampLCT((long)row["subject_score_change_ts_lct"]).ToString(Utilities.STANDARD_DATETIME_FORMAT)
                });
            }

            TrafodionIGridSubjectArea.FillWithData(subjectTable);
            TrafodionIGridSubjectArea.Cols.AutoWidth();

            TrafodionIGridSubjectArea.DoubleClickHandler = DrillDownHandler;

            DoPopulate();
        }

        private void DoPopulate()
        {
            //Update Health State value to image icons;
            
              
            foreach (iGRow row in TrafodionIGridSubjectArea.Rows) 
            {
                row.Cells["State"].ImageIndex = (int)(long)row.Cells["State"].Value;
               // row.Height = 30;                
            }
            iGCellStyle stateCellStyle = new iGCellStyle();
            stateCellStyle.ImageList = HealthStateImageList;
            stateCellStyle.Flags = iGCellFlags.DisplayImage;
            stateCellStyle.ImageAlign = iGContentAlignment.MiddleCenter;

            TrafodionIGridSubjectArea.Cols["State"].CellStyle = stateCellStyle;
            TrafodionIGridSubjectArea.Cols.AutoWidth();

            if (TrafodionContext.Instance.CurrentConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ134) 
            {
                HyperlinkSubAreaCell();
                AddContextMenuItems();
            }
        }

        private void _helpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.DetailedHealthStates);
        }

        private void HyperlinkSubAreaCell() 
        {
            TrafodionIGridSubjectArea.RowMode = false;
            HyperlinkColIndex = TrafodionIGridSubjectArea.Cols["Subject Area"].Index;
            _hyperLinkCellManager.Attach(TrafodionIGridSubjectArea, HyperlinkColIndex);
            _cellClickHandler = new iGCellClickEventHandler(iGrid_CellClick);
            TrafodionIGridSubjectArea.CellClick += _cellClickHandler;
        }

        private void AddContextMenuItems() 
        {
            TrafodionIGridToolStripMenuItem _drilldownFailureMenuItem = new TrafodionIGridToolStripMenuItem();
            TrafodionIGridToolStripMenuItem _drilldownAllMenuItem = new TrafodionIGridToolStripMenuItem();

            _drilldownFailureMenuItem.Text = Properties.Resources.DrillDownHealthStateFailureDetailMenu;
            _drilldownFailureMenuItem.Click += new EventHandler(drilldownFailureMenuItem_Click);

            _drilldownAllMenuItem.Text = Properties.Resources.DrillDownHealthStateAllDetailMenu;
            _drilldownAllMenuItem.Click+=new EventHandler(drilldownAllMenuItem_Click);

            TrafodionIGridSubjectArea.AddContextMenu(_drilldownFailureMenuItem);
            TrafodionIGridSubjectArea.AddContextMenu(_drilldownAllMenuItem);            
        }

        private void iGrid_CellClick(object sender, iGCellClickEventArgs e)
        {            
            // Handle hyperline here
            if (e.ColIndex == HyperlinkColIndex)
            {
                try
                {
                    DrillDownHandler(e.RowIndex);
                }
                catch (Exception ex) { }
            }
        }


        private void DrillDownHandler(int rowIndex) 
        {
            string subjectArea = TrafodionIGridSubjectArea.Rows[rowIndex].Cells["Subject Area"].Value.ToString().Trim();

            ConnectionDefinition currentConnection = TrafodionContext.Instance.CurrentConnectionDefinition;

            if (currentConnection != null && currentConnection.TheState == ConnectionDefinition.State.TestSucceeded)
            {
                DoDrillDown(subjectArea);                
            }
            else 
            {
                if (DoODBCConnect(currentConnection))
                {
                    DoDrillDown(subjectArea);
                }
                else 
                {
                    MessageBox.Show(Properties.Resources.ODBCNeededForHealthDrillDown, "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                }
            }            
        }


        private bool DoODBCConnect(ConnectionDefinition aConnectionDefinition)
        {
            ConnectionDefinitionDialog theConnectionDefinitionDialog = new ConnectionDefinitionDialog(false);
            theConnectionDefinitionDialog.Edit(aConnectionDefinition);

            if (aConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        private void DoDrillDown(string subjectArea) 
        {
            DropConfirmDialog confirmDialog = new DropConfirmDialog(Properties.Resources.HealthDrillDown, Properties.Resources.HealthDrillDownConfirm, Trafodion.Manager.Properties.Resources.Question, "Only fetch failure information", true);
          
            if (confirmDialog.ShowDialog() == DialogResult.Yes)
            {
                bool allInfo = !confirmDialog.OptionValue; //In the screen it pompts failure only.
                HealthStatesDrillDownControl drilldownControl = new HealthStatesDrillDownControl(subjectArea, allInfo);
                drilldownControl.Show();
            }
        }

        private void DoDrillDown(string subjectArea, bool allInfo) 
        {
            HealthStatesDrillDownControl drilldownControl = new HealthStatesDrillDownControl(subjectArea, allInfo);
            drilldownControl.Show();
        }
        
        private void drilldownFailureMenuItem_Click(object sender, EventArgs e) 
        {
            bool allInfo = false;
            DrillDownMenuClick(allInfo);
        }

        private void drilldownAllMenuItem_Click(object sender, EventArgs e)
        {
            bool allInfo = true;
            DrillDownMenuClick(allInfo);
        }


        private void DrillDownMenuClick(bool allInfo) 
        {
            string subjectArea = TrafodionIGridSubjectArea.CurCell.Row.Cells["Subject Area"].Value.ToString().Trim();

            ConnectionDefinition currentConnection = TrafodionContext.Instance.CurrentConnectionDefinition;

            if (currentConnection != null && currentConnection.TheState == ConnectionDefinition.State.TestSucceeded)
            {
                DoDrillDown(subjectArea, allInfo);
            }
            else
            {
                if (DoODBCConnect(currentConnection))
                {
                    DoDrillDown(subjectArea, allInfo);
                }
                else
                {
                    MessageBox.Show(Properties.Resources.ODBCNeededForHealthDrillDown, "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                }
            } 
        }

        public void LiveFeedInfrastructureCheck()
        {
            DropConfirmDialog confirmDialog = new DropConfirmDialog(Properties.Resources.LiveFeedInfrastructureDetails, Properties.Resources.LiveFeedInfrastructureDetailsConfirm, Trafodion.Manager.Properties.Resources.Question, "Only fetch failure information", true);

            if (confirmDialog.ShowDialog() == DialogResult.Yes)
            {
                string subjectArea = HealthStatesDrillDownControl.LIVEFEED_INFRA_DISPLAY;
                ConnectionDefinition currentConnection = TrafodionContext.Instance.CurrentConnectionDefinition;
                bool allInfo = !confirmDialog.OptionValue; //In the screen it pompts failure only.
                if (currentConnection != null && currentConnection.TheState == ConnectionDefinition.State.TestSucceeded)
                {
                    DoDrillDown(subjectArea, allInfo);
                }
                else
                {
                    if (DoODBCConnect(currentConnection))
                    {
                        DoDrillDown(subjectArea, allInfo);
                    }
                    else
                    {
                        MessageBox.Show(Properties.Resources.ODBCNeededForHealthDrillDown, "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    }
                }
            }
        }
    }
}
