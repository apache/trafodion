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

namespace Trafodion.Manager.Framework.Controls
{
    public partial class TrafodionMultipleErrorDialog : TrafodionForm
    {
        
        /// <summary>
        /// Dialog box that displays invalid assembly information
        /// </summary>
        /// <param name="anIncompatibleAssemblyList"></param>
        /// <param name="anExceptionsDictionary"></param>
        public TrafodionMultipleErrorDialog(string windowHeader, string errorHeader, string mainMessage, string anExceptions)
        {
            InitializeComponent();
            this.Text = windowHeader;
            this.errorHeader_TrafodionLabel.Text = errorHeader;
            this.errorMainText_TrafodionTextBox.Text = mainMessage;
            //this.Height = this.TrafodionPanel1.Height + mainMessageLabel + 200; 
            this.exceptions_TrafodionTextBox.Text = anExceptions;
            this.OkButton.Select();

        }

        private void okButton_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        private void showDetails_Click(object sender, EventArgs e)
        {
            if (showDetailsButton.Text.Equals("Show Details"))
            {
                showDetailsButton.Text = "Hide Details";
                splitContainer1.Panel2Collapsed = false;
                this.Height += 228; //+ pluginDataGridView.Height;
            }
            else
            {
                showDetailsButton.Text = "Show Details";
                splitContainer1.Panel2Collapsed = true;
                this.Height = 228;
            }
        }
    }
}
