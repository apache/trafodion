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
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.Main
{
    /// <summary>
    /// This form shall be used to display all options for TrafodionManager
    /// </summary>
    public partial class MainOptionsDialog : TrafodionForm
    {
        #region Constructor
        public MainOptionsDialog()
        {
            InitializeComponent();
            this.theOptionsPanel.TheOkButton.Click += new System.EventHandler(this.theOkButton_Click);
            this.theOptionsPanel.TheCancelButton.Click += new System.EventHandler(this.theCancelButton_Click);
            this.theOptionsPanel.TheApplyButton.Click += new System.EventHandler(this.theApplyButton_Click);
        }
        #endregion

        #region Public methods
        public void showOptions(Dictionary<string, IOptionsProvider> options)
        {
            this.theOptionsPanel.showOptions(options);
        }
        #endregion

        #region Private methods
        private void theOkButton_Click(object sender, EventArgs e)
        {
            // This is to force a completion of the grid changes in case short-cut key is used.
            this.theOptionsPanel.TheOkButton.Select();

            if (this.theOptionsPanel.PersistOptions())
            {
                this.Close();
            }
        }
        private void theCancelButton_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        private void theApplyButton_Click(object sender, EventArgs e)
        {
            // This is to force a completion of the grid changes in case short-cut key is used.
            this.theOptionsPanel.TheApplyButton.Select();

            this.theOptionsPanel.PersistOptions();
        }
        #endregion
    }
}
