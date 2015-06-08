//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2012-2015 Hewlett-Packard Development Company, L.P.
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
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    public partial class DropConfirmDialog :  Trafodion.Manager.Framework.Controls.TrafodionForm
    {
        #region Fields

        private string _title = string.Empty;
        private string _message = string.Empty;
        private string _icon = string.Empty;
        private string _option = string.Empty;
        private bool _OptionValue = false;
        private string _note = string.Empty;

        #endregion
        #region Properties
        public bool OptionValue
        {
            get { return _OptionValue; }           
        }

        #endregion
        #region Constructors

        public DropConfirmDialog()
        {
            InitializeComponent();
            _thePicBox.Image = Trafodion.Manager.Properties.Resources.YellowIcon;
        }

        public DropConfirmDialog(string aTitle, string aMessage, Image icon,string aOptionText,bool optionValue)
            :this()
        {
            Text = aTitle;
            _theCheckOption.Text = aOptionText;
            _theMessageLabel.Text = aMessage;
            _thePicBox.Image = icon;
            _theCheckOption.Checked = optionValue;
        }

        public DropConfirmDialog(string aTitle, string aMessage, Image icon, string aOptionText, bool optionValue, string aNote) : this(aTitle, aMessage, icon, aOptionText, optionValue) 
        {
             _theNoteLabel.Text= aNote;
        }

        #endregion


      

        #region Events        
        private void _theCheckOption_CheckedChanged(object sender, EventArgs e)
        {
            _OptionValue = _theCheckOption.Checked;
        }
        #endregion

        
    }
}
