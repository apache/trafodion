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
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.Main
{
    public partial class OptionsDialog : Trafodion.Manager.Framework.Controls.TrafodionForm
    {
        ConnectionDefinition _connectionDef;
        NCIOptions model = null;

        public OptionsDialog()
        {
            model = new NCIOptions();
            InitializeComponent();

            TrafodionCheckBox1.Checked = model.AutoLogon;
            userCheckBox.Checked = model.SetPromptUser;
            serverCheckBox.Checked = model.SetPromptServer;
            schemaCheckBox.Checked = model.SetPromptSchema;
            customCheckBox.Checked = model.UseCustomPrompt;
            customTextBox.Text = model.CustomPromptString;

            if ((userCheckBox.Checked | serverCheckBox.Checked | schemaCheckBox.Checked) & userCheckBox.Enabled)
            {
                UpdateCustom(false);
            }
            else
            {
                UpdateCustom(true);
            }
                    
        }

        public ConnectionDefinition ConnectionDef
        {
            set { _connectionDef = value; }
            get { return _connectionDef; }
        }

        private void TrafodionOptionsDoneButton_Click(object sender, EventArgs e)
        {
            if (!String.IsNullOrEmpty(customTextBox.Text))
            {
                model.CustomPromptString = customTextBox.Text;
            }

            this.Close();
        }

        private void TrafodionCheckBox1_CheckedChanged(object sender, EventArgs e)
        {
            model.AutoLogon = this.TrafodionCheckBox1.Checked;
            if (this.TrafodionCheckBox1.Checked)
            {
                customCheckBox.Enabled = true;

                if (customCheckBox.Checked)
                {
                    UpdateControls(false);
                    UpdateCustom(true);
                }
                else
                {
                    UpdateControls(true);
                }                            
            }
            else
            {
                UpdateControls(false);
                customCheckBox.Enabled = false;
                UpdateCustom(false);
                
            }


        }

        
        private void userCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            model.SetPromptUser = userCheckBox.Checked;
        }

        private void serverCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            model.SetPromptServer = serverCheckBox.Checked;
        }

        private void schemaCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            model.SetPromptSchema = schemaCheckBox.Checked;
        }

        private void customCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            model.UseCustomPrompt = customCheckBox.Checked;

            if (customCheckBox.Checked)
            {
                UpdateControls(false);
                UpdateCustom(true);
                //if (!String.IsNullOrEmpty(customCheckBox.Text))
                //{
                //    model.CustomPromptString = customCheckBox.Text;
                //}
            }
            else
            {
                UpdateControls(true);
                UpdateCustom(false);
                               
            }
        }

        private void UpdateControls(bool enableFlag)
        {
            userCheckBox.Enabled = enableFlag;
            serverCheckBox.Enabled = enableFlag;
            schemaCheckBox.Enabled = enableFlag;
           

        }

        private void UpdateCustom(bool enableFlag)
        {
            customTextBox.Enabled = enableFlag;
            
        }

    }
}
