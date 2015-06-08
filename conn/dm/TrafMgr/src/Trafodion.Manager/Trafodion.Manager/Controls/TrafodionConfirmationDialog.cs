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

using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.Framework.Controls
{
    public partial class TrafodionConfirmationDialog : TrafodionForm
    {
        #region Private Member Variables
        
        private string _promptText = "";
        private bool _ApplyToAll = false;
        #endregion Private Member Variables

        #region Public Properties

        /// <summary>
        /// The message text for user to response
        /// </summary>
        public string PromptText
        {
            get { return this.prompt_TrafodionLabel.Text; }
            set { this.prompt_TrafodionLabel.Text = value; }
        }

        /// <summary>
        /// Use this ApplyToAll flag to skip the confirmation prompts
        /// </summary>
        public bool ApplyToAll
        {
            get { return _ApplyToAll; }
            set { _ApplyToAll = value; }
        }

        #endregion Public Properties

        /// <summary>
        /// default constructor
        /// </summary>
        public TrafodionConfirmationDialog()
        {
            InitializeComponent();
            setupDefaultProperties();
        }

        public TrafodionConfirmationDialog(string aPromptText, string aTitleText)
        {
            //Apply the title text
            this.Text = aTitleText;
            InitializeComponent();

            //Apply prompt text to the control.
            this.prompt_TrafodionLabel.Text = aPromptText;
            setupDefaultProperties();
        }

        private void setupDefaultProperties()
        {
            this.FormClosing += new FormClosingEventHandler(StopOperationDialog_FormClosing);
            
            // Center the dialog on the parent
            StartPosition = FormStartPosition.CenterParent;
    }

        private void _yesToAllButton_Click(object sender, System.EventArgs e)
        {
            this.ApplyToAll = true;
            this.Close();
        }

        private void _yesButton_Click(object sender, System.EventArgs e)
        {
            this.ApplyToAll = false;
            this.Close();
        }


        /// <summary>
        /// Catch the form closing event so that the dialog remains open if exceptions are thrown
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void StopOperationDialog_FormClosing(object sender, FormClosingEventArgs e)
        {
            // TODO: handle user click the "X" button here
            //e.Cancel = true;
        }

    }
}
