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
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.Framework.Connections.Controls
{

    /// <summary>
    /// A panel that shows a connection definition's description and allows the user to enter a password for that
    /// connection definition.  A Test button may also be shown if desired.
    /// </summary>
    public class EditPasswordPanel : UserControl
    {

        /// <summary>
        /// Designer-generated variables
        /// </summary>
        private TrafodionTextBox thePasswordTextBox;
        private TrafodionLabel theConnectionDefinitionLabel;
        private TrafodionButton theTestButton;
        private TrafodionLabel thePasswordLabel;

        /// <summary>
        /// Create a panel
        /// </summary>
        /// <param name="aConnectionDefinition">The connection definition</param>
        /// <param name="hideTestButton">true to hide the test button</param>
        public EditPasswordPanel(ConnectionDefinition aConnectionDefinition, bool hideTestButton)
        {
            ConnectionDefinition = aConnectionDefinition;
            InitializeComponent();
            theConnectionDefinitionLabel.Text = ConnectionDefinition.Description;
            theTestButton.Visible = !hideTestButton;
            thePasswordTextBox.MaxLength = ConnectionDefinition.PASSWORD_MAX_LENGTH;

            // The test button is assumed to be immediately to the right of the password field.  If the test
            // button is hidden, make the password field wider to include the area that would have been used
            // by the test button.
            if (hideTestButton)
            {
                thePasswordTextBox.Width = theTestButton.Location.X + theTestButton.Width - thePasswordTextBox.Location.X;
            }

        }

        private ConnectionDefinition theConnectionDefinition;

        public ConnectionDefinition ConnectionDefinition
        {
            get { return theConnectionDefinition; }
            set { theConnectionDefinition = value; }
        }
    
        /// <summary>
        /// Designer-generated code
        /// </summary>
        private void InitializeComponent()
        {
            this.thePasswordTextBox = new TrafodionTextBox();
            this.theConnectionDefinitionLabel = new TrafodionLabel();
            this.thePasswordLabel = new TrafodionLabel();
            this.theTestButton = new TrafodionButton();
            this.SuspendLayout();
            // 
            // thePasswordTextBox
            // 
            this.thePasswordTextBox.AcceptsReturn = true;
            this.thePasswordTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.thePasswordTextBox.Location = new System.Drawing.Point(74, 38);
            this.thePasswordTextBox.Name = "thePasswordTextBox";
            this.thePasswordTextBox.PasswordChar = '*';
            this.thePasswordTextBox.Size = new System.Drawing.Size(265, 20);
            this.thePasswordTextBox.TabIndex = 3;
            this.thePasswordTextBox.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.thePasswordTextBox_KeyPress);
            this.thePasswordTextBox.TextChanged += new System.EventHandler(this.thePasswordTextBox_TextChanged);
            // 
            // theConnectionDefinitionLabel
            // 
            this.theConnectionDefinitionLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.theConnectionDefinitionLabel.AutoEllipsis = true;
            //this.theConnectionDefinitionLabel.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.theConnectionDefinitionLabel.Location = new System.Drawing.Point(12, 15);
            this.theConnectionDefinitionLabel.Name = "theConnectionDefinitionLabel";
            this.theConnectionDefinitionLabel.Size = new System.Drawing.Size(409, 13);
            this.theConnectionDefinitionLabel.TabIndex = 1;
            this.theConnectionDefinitionLabel.Text = "<connection description>";
            // 
            // thePasswordLabel
            // 
            this.thePasswordLabel.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.thePasswordLabel.AutoSize = true;
            //this.thePasswordLabel.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.thePasswordLabel.Location = new System.Drawing.Point(12, 40);
            this.thePasswordLabel.Name = "thePasswordLabel";
            this.thePasswordLabel.Size = new System.Drawing.Size(56, 13);
            this.thePasswordLabel.TabIndex = 2;
            this.thePasswordLabel.Text = "Password:";
            // 
            // theTestButton
            // 
            this.theTestButton.Anchor = System.Windows.Forms.AnchorStyles.Right;
            //this.theTestButton.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.theTestButton.Location = new System.Drawing.Point(345, 36);
            this.theTestButton.Name = "theTestButton";
            this.theTestButton.Size = new System.Drawing.Size(75, 23);
            this.theTestButton.TabIndex = 4;
            this.theTestButton.Text = "Test";
            this.theTestButton.Click += new System.EventHandler(this.theTestButton_Click);
            // 
            // EnterPasswordPanel
            // 
            this.Controls.Add(this.theTestButton);
            this.Controls.Add(this.thePasswordLabel);
            this.Controls.Add(this.theConnectionDefinitionLabel);
            this.Controls.Add(this.thePasswordTextBox);
            this.Name = "EnterPasswordPanel";
            this.Size = new System.Drawing.Size(428, 71);
            this.Enter += new System.EventHandler(this.EnterPasswordPanel_Enter);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        /// <summary>
        /// Called when the text in the password field changes
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void thePasswordTextBox_TextChanged(object sender, EventArgs e)
        {

            // Note that the user has changed something
            Changed = true;

        }

        /// <summary>
        /// Called when the user clicks the Test button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void theTestButton_Click(object sender, EventArgs e)
        {

            // Do the test and show a message box even if the test succeeds
            DoTest(false);
        }

        /// <summary>
        /// Does a test of our connection definition
        /// </summary>
        /// <param name="suppressSuccessMessageBox">true to suppress the message box if the test succeeds</param>
        private void DoTest(bool suppressSuccessMessageBox)
        {

            // Collect the password from the control and set it in the connection definition
            ConnectionDefinition.Password = thePasswordTextBox.Text;

            // Perform the test with message box as caller dersires
            ConnectionDefinition.DoTest(suppressSuccessMessageBox);
            Changed = true;
        }

        private void EnterPasswordPanel_Enter(object sender, EventArgs e)
        {

            // Note that the user has changed something
            thePasswordTextBox.Focus();

        }

        /// <summary>
        /// True if the user has changed something
        /// </summary>
        private bool changed = false;

        /// <summary>
        /// Property that is true if the user has changed something
        /// </summary>
        public bool Changed
        {
            get { return changed; }
            set { changed = value; }
        }

        /// <summary>
        /// Read only property that returns the current password text
        /// </summary>
        public string Password
        {
            get { return thePasswordTextBox.Text; }
        }

        /// <summary>
        /// Called when the user presses a key in the password field
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void thePasswordTextBox_KeyPress(object sender, KeyPressEventArgs e)
        {

            // If the test button is visible and the user hits enter, we will pretend that the
            // the user clicked the Test button
            if (theTestButton.Visible && (e.KeyChar == '\r'))
            {

                // Perform the test, suppressing the message box on success
                DoTest(true);

            }
        }

    }

}
