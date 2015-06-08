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

namespace Trafodion.Manager.SecurityArea.Controls
{
    public partial class LargeTextBoxDialog : Form
    {
        #region Fields

        private string _title = null;
        private Color _theOriginalFontColor;

        #endregion Fields

        #region Properties

        public String WindowTitle
        {
            get { return String.Format(Properties.Resources.SecurityWindowTitle, _title); }
            set 
            { 
                _title = value;
                this.Text = String.Format(Properties.Resources.SecurityWindowTitle, _title);
            }
        }

        public Color OriginalFontColor
        {
            get { return _theOriginalFontColor; }
            set { _theOriginalFontColor = value; }
        }

        public String Content
        {
            get { return _theTextBox.Text.Trim(); }
            set { _theTextBox.Text = value; }
        }

        public Color ContentFontColor
        {
            get { return _theTextBox.ForeColor; }
            set { _theTextBox.ForeColor = value; }
        }

        #endregion Properties

        public LargeTextBoxDialog(string aTitle, Color anOriginalFontColor)
        {
            InitializeComponent();
            CenterToParent();
            WindowTitle = aTitle;
            _theOriginalFontColor = anOriginalFontColor;
        }

        public void ResetFontColor()
        {
            ContentFontColor = _theOriginalFontColor;
        }

        private void _theApplyButton_Click(object sender, EventArgs e)
        {
            Close();
        }

        private void _theCancelButton_Click(object sender, EventArgs e)
        {
            Close();
        }

        private void _theTextBox_TextChanged(object sender, EventArgs e)
        {
            ResetFontColor();
        }
    }
}
