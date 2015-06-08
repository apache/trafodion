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
using System.Drawing;
using System.Windows.Forms;

namespace Trafodion.Manager.Framework.Controls
{
    public partial class TrafodionStatusLightUserControl : UserControl
    {
        public delegate void ChangingHandler(object sender, EventArgs args);
        public event ChangingHandler MouseClickLight;
        private int _state = NOT_SET_STATE;
        private string _label = "";
        private string _key = "";
        private string _toolTipText = "";
        private object _tag = null;
        private const int NOT_SET_STATE = -1;

        public string Key
        {
            get { return _key; }
            set { _key = value; }
        }

        public int State
        {
            get { return _state; }
            set
            {
                _state = value;
                SetState(_state);
            }
        }

        public string Label
        {
            get { return _label; }
            set
            {
                _label = value;
                this.lightLabel_label.Text = value;
                if (string.IsNullOrEmpty(_label))
                {
                    tableLayoutPanel1.Visible = false;
                    panel6.Visible = false;
                }
            }
        }

        public string ToolTipText
        {
            get {

                return this._toolTipText;
            }
            set {
                this._toolTipText = value;

                //this.toolTip1.SetToolTip(this.panel1, this._toolTipText);
                this.toolTip1.SetToolTip(this.statusLight_pictureBox, this._toolTipText);
                this.toolTip1.SetToolTip(this.lightLabel_label, this._toolTipText);
            }
        }

        public object TagObject
        {
            get { return _tag; }
            set { _tag = value; }
        }

        public TrafodionStatusLightUserControl()
        {
            InitializeComponent();
        }

        public void SetState(int aState)
        {
            this.statusLight_pictureBox.Image = GetLightColor(aState);
        }

        public bool IsAbnormal(int previousState)
        {
            return (previousState == 0 && (_state == 1 || _state == 2 || _state == 3))
                || (previousState == NOT_SET_STATE && (_state == 1 || _state == 2))
                || (previousState == 3 && (_state == 1 || _state == 2));
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
            if (3 == SystemStat)
            {
                return global::Trafodion.Manager.Properties.Resources.UnknownIcon;
            }

            return global::Trafodion.Manager.Properties.Resources.GrayIcon;
        }

        public void statusLight_pictureBox_Click(object sender, EventArgs e)
        {
            if (null != MouseClickLight)
            MouseClickLight(this, e);
        }
    }
}
