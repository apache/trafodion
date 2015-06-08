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

namespace Trafodion.Manager.OverviewArea.Controls
{
    public partial class SystemMonitorAggregationBreadcrumbs : UserControl
    {
        public SystemMonitorAggregationBreadcrumbs()
        {
            InitializeComponent();
        }

        public LinkLabel FullLinkLabel
        {
            get {
                return this.full_linkLabel;
            }
        }

        public void SetFullLinkLabel(string LinkLabelText)
        {
            this.full_linkLabel.Text = LinkLabelText;
        }

        public void SetDrillLinkLabel(string LinkLabelText)
        {

            bool drilled = (LinkLabelText != "");
            if (!drilled)
            { this.tableLayoutPanel1.ColumnCount = 1; }
            else
            {
                this.tableLayoutPanel1.ColumnCount = 4; 
            }
            this.segNum_label.Text = LinkLabelText;
            this.segment_label.Visible = drilled;
            this.arrows_label.Visible = drilled;
            this.full_linkLabel.Enabled = drilled;



        }

    }
}
