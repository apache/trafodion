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
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.Framework.Controls
{
    public partial class TrafodionAdvancedStatusLightUserControl : UserControl
    {
        public delegate void ChangingHandler(object sender, EventArgs args);
        public event ChangingHandler MouseClickLight;
        private int _state = 0;
        private string _label = "";
        public int State
        {
            get { return _state; }
            set {                
                    _state = value;
                    SetState(_state);
                }
        }       

        public TrafodionAdvancedStatusLightUserControl()
        {
            InitializeComponent();

        }

        public TrafodionAdvancedStatusLightUserControl(string aLabel)
        {
            InitializeComponent();
            //this.lightLabel_label.Text = aLabel;
        }

        public TrafodionAdvancedStatusLightUserControl(int initialState)
        {
            InitializeComponent();
            //this.lightLabel_label.Text = "";
            this.State = initialState;
        }

        public TrafodionAdvancedStatusLightUserControl(int aInitialState, string aControlText)
        {
            InitializeComponent();
            this.State = aInitialState;
            //this.lightLabel_label.Text = aControlText;
        }

        public TrafodionAdvancedStatusLightUserControl(int aInitialState, string aControlText, string aGBoxTitle)
        {
            //Suspend layout
            InitializeComponent();
            
            this.State = aInitialState;
            //this.lightLabel_label.Text = aControlText;
            this.groupBox_TrafodionGroupBox.Text = aGBoxTitle;
        }


        public void SetState(int aState)
        {
            this.statusLight_pictureBox.Image = GetLightColor(aState);
        }


        public void AddDetail(string aName, string aValue)
        {
            iGRow row = this.detailsIG_TrafodionIGrid.Rows.Add();//Rows.Add(row);
            row.Cells["Name"].Value = aName;
            row.Cells["Value"].Value = aValue;
        }

        private Image GetLightColor(int SystemStat)
        {
            if (0 == SystemStat)
            {
                return global::Trafodion.Manager.Properties.Resources.GreenIcon;
            }
            if (1 == SystemStat)
            {
                return global::Trafodion.Manager.Properties.Resources.YellowIcon;
            }
            if (2 == SystemStat)
            {
                return global::Trafodion.Manager.Properties.Resources.RedIcon;
            }

            return global::Trafodion.Manager.Properties.Resources.GrayIcon;
        }

        public void statusLight_pictureBox_Click(object sender, EventArgs e)
        {
            try
            {
                MouseClickLight(sender, e);
            }
            catch (Exception ex)
            {

            }
        }
    }
}
