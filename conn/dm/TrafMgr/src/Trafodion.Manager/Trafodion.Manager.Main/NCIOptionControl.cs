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
using System.IO;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.Main
{
    /// <summary>
    /// User control to display all of the NCI options
    /// </summary>
    public partial class NCIOptionControl : UserControl, IOptionControl
    {
        ConnectionDefinition _connectionDef;
        NCIOptions model;

        #region constructor
        public NCIOptionControl()
        {
            model = new NCIOptions();
            InitializeComponent();
            UpdateUIFromModel();
        }
        #endregion

        #region Properties
        public ConnectionDefinition ConnectionDef
        {
            set { _connectionDef = value; }
            get { return _connectionDef; }
        }
        #endregion

        #region IOptionControl
        public String OptionTitle
        {
            get { return global::Trafodion.Manager.Properties.Resources.NCIOptions; }
        }


        public Object OnOptionsChanged()
        {
            //if (!String.IsNullOrEmpty(customTextBox.Text))
            //{
                model.CustomPromptString = customTextBox.Text;
            //}
            if (!String.IsNullOrEmpty(nciExecutableText.Text))
            {
                model.NCIExecutableFile = nciExecutableText.Text;
            }
            return model;
        }

        public void LoadedFromPersistence(Object persistedObject)
        {
            model.SetModel(persistedObject as NCIOptions);
            this.UpdateUIFromModel();
        }
        #endregion

        #region private methods
        private void UpdateUIFromModel()
        {
            TrafodionCheckBox1.Checked = model.AutoLogon;
            userCheckBox.Checked = model.SetPromptUser;
            serverCheckBox.Checked = model.SetPromptServer;
            schemaCheckBox.Checked = model.SetPromptSchema;
            customCheckBox.Checked = model.UseCustomPrompt;
            customTextBox.Text = model.CustomPromptString;
            this.nciExecutableText.Text = model.NCIExecutableFile;

            if ((userCheckBox.Checked | serverCheckBox.Checked | schemaCheckBox.Checked) & userCheckBox.Enabled)
            {
                UpdateCustom(false);
            }
            else
            {
                UpdateCustom(true);
            }

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


        private void browseButton_Click(object sender, EventArgs e)
        {
            OpenFileDialog openFileDialog = new OpenFileDialog();
            try
            {
                openFileDialog.FileName = Path.GetFileName(nciExecutableText.Text);
                //Setup an initial directory to display in openfiledialog if possible.
                if (openFileDialog.InitialDirectory == "")
                {
                    openFileDialog.InitialDirectory = Path.GetDirectoryName(nciExecutableText.Text);
                }
            }
            catch (Exception ex2)
            {
                //Not a big deal if this gets hit. Used to keep users from seeing an
                //exception.  
#if DEBUG
                System.Console.WriteLine(ex2.Message);
#endif
            }

            //Could not find the given directory so just default it to C:
            if (!Directory.Exists(openFileDialog.InitialDirectory))
            {
                openFileDialog.InitialDirectory = ("C:");
            }


            //Set up some file filters for better usability.
            openFileDialog.Filter = global::Trafodion.Manager.Properties.Resources.FileFilter;

            if (openFileDialog.ShowDialog() == DialogResult.OK)
            {
                nciExecutableText.Text = openFileDialog.FileName;

            }
        }
        #endregion
    }
}
