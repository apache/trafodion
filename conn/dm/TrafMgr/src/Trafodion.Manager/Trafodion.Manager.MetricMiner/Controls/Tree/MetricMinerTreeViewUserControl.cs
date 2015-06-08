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

namespace Trafodion.Manager.MetricMiner.Controls.Tree
{
    public partial class MetricMinerTreeViewUserControl : UserControl
    {
        private MetricMinerWidgetsTreeView _theTree;
        ImageList il = new ImageList();

        public MetricMinerTreeViewUserControl()
        {
            InitializeComponent();

            this._theTree = new Trafodion.Manager.MetricMiner.Controls.Tree.MetricMinerWidgetsTreeView();

            /*
             * Fix the issue that report tree doesn't refresh automatically after RE-LAUNCH( open, close, and then reopen ) a metric minter window.
             * The reason is that when a node of MetricMinerReportFolder type is added, a one-to-one mapping FileSystemWatcher will be created and working, 
             * which will watch the file change of the report folder, and update the tree accordingly.
             * And when closing the Metric Miner window, the FileSystemWatcher should be disposed also.
             * The defect is the disposing is missing. So when Metric Miner is relaunched, a new FileSystemWatcher is created and is watch one same report folder.
             * I.e. several FileSystemWatchers are watching one same report folder. In such case, when new report file is created, all of them will try to add a node to the tree, and the tree will work abnormal without refreshing
             */
            this.Load += (s, arg) =>
                {
                    this.ParentForm.FormClosing += (sender, e) =>
                        {
                            this._theTree.Dispose();
                        };
                };

            this.SuspendLayout();
            // 
            // _theTree
            // 
            this._theTree.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theTree.Location = new System.Drawing.Point(0, 0);
            this._theTree.Name = "_theTree";
            this._theTree.Size = new System.Drawing.Size(150, 535);
            this._theTree.ImageIndex = -1;
            this._theTree.SelectedImageIndex = -1;
            // 
            // MetricMinerTreeViewUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theTree);
            this.Name = "MetricMinerTreeViewUserControl";
            this.Size = new System.Drawing.Size(150, 535);
            this.ResumeLayout(false);

            this.Load +=new EventHandler(MetricMinerTreeViewUserControl_Load);
            //_theTree.ImageList = _theImageList;
           

        }

        private void MetricMinerTreeViewUserControl_Load(object sender, System.EventArgs e)
        {
            _theTree.BeginUpdate();
            _theTree.Populate();

            _theTree.EndUpdate();
            _theTree.Invalidate();
        }
        public MetricMinerWidgetsTreeView TheTree
        {
            get { return _theTree; }
            set { _theTree = value; }
        }

    }
}
