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
using System.Windows.Forms;
using System.Data;
using System.Drawing;

namespace Trafodion.Manager.Framework.Controls
{
    public partial class TrafodionMultipleMessageDialog : Trafodion.Manager.Framework.Controls.TrafodionForm
    {
        private readonly Size fullMinimumSize = new Size(600, 300);
        private const int MIMINUM_HEIGHT = 130;
        private int detailHeight = 0;

        /// <summary>
        /// 
        /// </summary>
        /// <param name="summaryMessage"></param>
        /// <param name="dataTable"></param>
        /// <param name="anIcon"></param>
        public TrafodionMultipleMessageDialog(string summaryMessage, DataTable dataTable, Icon displayIcon)
        {
            InitializeComponent();
            summaryMessageLabel.Text = summaryMessage;

            if (displayIcon != null)
            {
                pictureBox1.Image = displayIcon.ToBitmap();
            }

            messagesGridView.AutoSizeRowsMode = DataGridViewAutoSizeRowsMode.AllCells;
            messagesGridView.DefaultCellStyle.WrapMode = DataGridViewTriState.True;
            messagesGridView.DataSource = dataTable;

            messagesGridView.AddButtonControlToParent(DockStyle.Bottom);

            CenterToScreen();
            ShowDetails();
        }

        public TrafodionDataGridView GridView
        {
            get
            {
                return this.messagesGridView;
            }
        }

        private void okButton_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        private void showDetails_Click(object sender, EventArgs e)
        {
            if (showDetailsButton.Text.Equals("Show Details"))
            {
                ShowDetails();
            }
            else
            {
                HideDetails();
            }
        }

        private void ShowDetails()
        {
            showDetailsButton.Text = "Hide Details";
            gbxDetail.Visible = true;
            this.MaximumSize = Size.Empty;
            this.Height += this.detailHeight;
            this.MinimumSize = this.fullMinimumSize;
        }

        private void HideDetails()
        {
            showDetailsButton.Text = "Show Details";
            this.detailHeight = this.Height - MIMINUM_HEIGHT;
            gbxDetail.Visible = false;
            this.MinimumSize = this.MaximumSize = new Size(this.Width, MIMINUM_HEIGHT);
        }
    }
}
