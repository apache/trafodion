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
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.WorkloadArea.Model;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class WMSOffenderStatusCommand : Trafodion.Manager.Framework.Controls.TrafodionForm
    {
        #region Members
        private WMSOffenderStatusOptions m_statusOptions = null;
        #endregion

        #region Properties
        public WMSOffenderStatusOptions WMSOffenderStatusOptions
        {
            get { return m_statusOptions; }
        }
        #endregion

        public WMSOffenderStatusCommand(WMSOffenderStatusOptions statusOptions)
        {
            InitializeComponent();
            m_statusOptions = statusOptions;
            this.Text = "Status Command";
            this.cpuRadioButton.CheckedChanged += new EventHandler(handleInfoChanged);
            this.memRadioButton.CheckedChanged += new EventHandler(handleInfoChanged);
            this.statusLinkLabel.MouseHover += new EventHandler(statusLinkLabel_MouseHover);
            this.useCPUCheckBox.CheckedChanged += new EventHandler(handleInfoChanged);
            this.useSegmentCheckBox.CheckedChanged += new EventHandler(handleInfoChanged);
            this.cpuNumericUpDown.ValueChanged += new EventHandler(handleInfoChanged);
            this.segmentNumericUpDown.ValueChanged += new EventHandler(handleInfoChanged);
            this.sqlRadioButton.CheckedChanged += new EventHandler(handleInfoChanged);
            this.allRadioButton.CheckedChanged += new EventHandler(handleInfoChanged);
            this.processLinkLabel.MouseHover += new EventHandler(processLinkLabel_MouseHover);
            this.syntaxLinkLabel.MouseHover += new EventHandler(syntaxLinkLabel_MouseHover);

            setStatusCommandControls();
        }

        private void setStatusCommandControls()
        {
            if (m_statusOptions.StatusCpu)
            {
                cpuRadioButton.Checked = true;
            }
            else
            {
                memRadioButton.Checked = true;
            }

            if (m_statusOptions.UseCpu)
            {
                useCPUCheckBox.Checked = true;
                cpuNumericUpDown.Enabled = true;
            }
            else
            {
                useCPUCheckBox.Checked = false;
                cpuNumericUpDown.Enabled = false;
            }
            cpuNumericUpDown.Value = m_statusOptions.CpuNumber;

            if (m_statusOptions.UseSegment)
            {
                useSegmentCheckBox.Checked = true;
                segmentNumericUpDown.Enabled = true;
            }
            else
            {
                useSegmentCheckBox.Checked = false;
                segmentNumericUpDown.Enabled = false;
            }
            segmentNumericUpDown.Value = m_statusOptions.SegmentNumber;

            if (m_statusOptions.SQLProcess)
            {
                sqlRadioButton.Checked = true;
            }
            else
            {
                allRadioButton.Checked = true;
            }

            commandPreviewTextBox.Text = m_statusOptions.StatusCommand;
        }

        void syntaxLinkLabel_MouseHover(object sender, EventArgs e)
        {
            string caption =
                "Syntax:\r\n" +
                "   STATUS <CPU|MEM> [cpu-num] [SEGMENT <seg-num>] [PROC[ESS] <SQL|ALL>]\r\n" +
                "Example:\r\n" +
                "   STATUS CPU 1 SEGMENT 2 PROCESS ALL";
                this.toolTip1.IsBalloon = true;
            this.toolTip1.AutoPopDelay = 30 * 1000;
            this.toolTip1.SetToolTip(syntaxLinkLabel, caption);
        }

        void statusLinkLabel_MouseHover(object sender, EventArgs e)
        {
         string caption =
                "Select one of these options:\r\n" +
                "- CPU to display processes consuming the most CPU resources\r\n" +
                "- Mem to display processes consuming the most memory";
            this.toolTip1.IsBalloon = true;
            this.toolTip1.AutoPopDelay = 30 * 1000;
            this.toolTip1.SetToolTip(statusLinkLabel, caption);
        }

        void processLinkLabel_MouseHover(object sender, EventArgs e)
        {
            string caption =
                "Select one of these options:\r\n" +
                "- ALL to display all processes\r\n" +
                "- SQL to display SQL processes such as MXUDR, MXESP, MXCI, and MXOSRVR\r\n" +
                "The default value is SQL."; 
            this.toolTip1.IsBalloon = true;
            this.toolTip1.AutoPopDelay = 30 * 1000;
            this.toolTip1.SetToolTip(processLinkLabel, caption);
        }

        void handleInfoChanged(object sender, EventArgs e)
        {
            string sql = getCommandPreview();
            this.commandPreviewTextBox.Text = sql;
        }

        private string getCommandPreview()
        {
            StringBuilder sb = new StringBuilder();
            sb.Append("STATUS ").Append(this.cpuRadioButton.Checked ? "CPU" : "MEM");
            sb.Append(this.useCPUCheckBox.Checked ? (" " + this.cpuNumericUpDown.Value.ToString()) : "");
            sb.Append(this.useSegmentCheckBox.Checked ? " SEGMENT " + this.segmentNumericUpDown.Value.ToString() : "");
            sb.Append(" PROCESS " + (this.sqlRadioButton.Checked ? "SQL" : "ALL"));
            return sb.ToString();
        }

        private bool validSyntax(string command)
        {
            StringSplitOptions options = StringSplitOptions.RemoveEmptyEntries;
            char[] splitter = {' '};
            string[] tokens = command.Split(splitter, options);
            bool result = false;
            if (tokens.Length >= 2)
            {
                if (tokens[0].ToUpper().Equals("STATUS"))
                {
                    if (tokens[1].ToUpper().Equals("CPU") || tokens[1].ToUpper().Equals("MEM"))
                    {
                        result = true;
                    }
                }
            }
            return result;
        }

        private void okButton_Click(object sender, EventArgs e)
        {
            m_statusOptions.StatusCpu = cpuRadioButton.Checked ? true : false;
            m_statusOptions.UseCpu = useCPUCheckBox.Checked ? true : false;
            m_statusOptions.CpuNumber = (int)cpuNumericUpDown.Value;
            m_statusOptions.UseSegment = useSegmentCheckBox.Checked ? true : false;
            m_statusOptions.SegmentNumber = (int)segmentNumericUpDown.Value;
            m_statusOptions.SQLProcess = sqlRadioButton.Checked ? true : false;
            m_statusOptions.StatusCommand = commandPreviewTextBox.Text;
            if (validSyntax(m_statusOptions.StatusCommand))
            {
                DialogResult = DialogResult.OK;
                Close();
            }
            else 
            {
                MessageBox.Show("Error: Check the STATUS <CPU|MEM> command syntax", Properties.Resources.SystemOffender, MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void cancelButton_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        private void useCPUCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            cpuNumericUpDown.Enabled = useCPUCheckBox.Checked ? true : false;
        }

        private void useSegmentCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            segmentNumericUpDown.Enabled = useSegmentCheckBox.Checked ? true : false;
        }

    }
}
