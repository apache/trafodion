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
    public partial class MultilineTextUserControl : UserControl
    {
        #region Fields

        #endregion Fields

        #region Properties

        /// <summary>
        /// ImportButton: the import button. This is for the container to register for button events. 
        /// </summary>
        public Button ImportButton
        {
            get { return _theImportButton; }
        }

        /// <summary>
        /// Content: the text content. 
        /// </summary>
        public String Content
        {
            get { return _theTextBox.Text.Trim(); }
            set { _theTextBox.Text = value; }
        }

        /// <summary>
        /// ContentFontColor: the text box's fore color. 
        /// </summary>
        public Color ContentFontColor
        {
            get { return _theTextBox.ForeColor; }
            set { _theTextBox.ForeColor = value; }
        }

        /// <summary>
        /// ContentBackColor: the text box's background color. 
        /// </summary>
        public Color ContentBackColor
        {
            get { return _theTextBox.BackColor; }
            set { _theTextBox.BackColor = value; }
        }

        /// <summary>
        /// TextBox: the text box control
        /// </summary>
        public TextBox TextBox
        {
            get { return _theTextBox; }
        }

        /// <summary>
        /// ImportButtonToolTipText: Setting the tooltip of the import button. 
        /// </summary>
        public String ImportButtonToolTipText
        {
            set
            {
                this.toolTip1.SetToolTip(this._theImportButton, value);
            }
        }

        #endregion Properties

        #region Constructor

        /// <summary>
        /// Constructor
        /// </summary>
        public MultilineTextUserControl()
        {
            InitializeComponent();
        }

        #endregion Constructor
    }
}
