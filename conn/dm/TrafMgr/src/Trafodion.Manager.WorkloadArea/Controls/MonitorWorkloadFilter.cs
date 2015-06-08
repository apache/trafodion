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
using Trafodion.Manager.WorkloadArea.Model;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class MonitorWorkloadFilter : Trafodion.Manager.Framework.Controls.TrafodionForm
    {
        #region Members

        private MonitorWorkloadOptions m_options = null;
        private Color m_tempWarnHigh = Color.WhiteSmoke;
        private Color m_tempWarnMedium = Color.WhiteSmoke;
        private Color m_tempWarnLow = Color.WhiteSmoke;
        private Color m_tempNoWarn = Color.WhiteSmoke;

        #endregion

        public MonitorWorkloadFilter(MonitorWorkloadOptions options)
        {
            InitializeComponent();
            m_options = options;
            setFilterStates();
            this._theToolTip.SetToolTip(this.maxWMSGraphPoints, Properties.Resources.MaxGraphPointsToolTipText);
            this._theToolTip.SetToolTip(this.maxWMSGraphPointsLabel, Properties.Resources.MaxGraphPointsToolTipText);
            maxWMSGraphPoints.Maximum = Int32.MaxValue;
        }

        private void setFilterStates()
        {
            executingCheckBox.Checked = m_options.QueryStates.Contains(executingCheckBox.Text.ToUpper());
            waitingCheckBox.Checked = m_options.QueryStates.Contains(waitingCheckBox.Text.ToUpper());
            holdingCheckBox.Checked = m_options.QueryStates.Contains(holdingCheckBox.Text.ToUpper());
            suspendedCheckBox.Checked = m_options.QueryStates.Contains(suspendedCheckBox.Text.ToUpper());
            rejectedCheckBox.Checked = m_options.QueryStates.Contains(rejectedCheckBox.Text.ToUpper());
            completedCheckBox.Checked = m_options.QueryStates.Contains(completedCheckBox.Text.ToUpper());
            executingButton.Enabled = executingCheckBox.Checked;
            waitingButton.Enabled = waitingCheckBox.Checked;
            holdingButton.Enabled = holdingCheckBox.Checked;
            suspendedButton.Enabled = suspendedCheckBox.Checked;
            rejectedButton.Enabled = rejectedCheckBox.Checked;
            completedButton.Enabled = completedCheckBox.Checked;
            executingButton.ForeColor = m_options.ExecutingColor;
            waitingButton.ForeColor = m_options.WaitingColor;
            holdingButton.ForeColor = m_options.HoldingColor;
            suspendedButton.ForeColor = m_options.SuspendedColor;
            rejectedButton.ForeColor = m_options.RejectedColor;
            completedButton.ForeColor = m_options.CompletedColor;

            warnHighCheckBox.Checked = m_options.WarnLevels.Contains(warnHighCheckBox.Text.ToUpper());
            warnMediumCheckBox.Checked = m_options.WarnLevels.Contains(warnMediumCheckBox.Text.ToUpper());
            warnLowCheckBox.Checked = m_options.WarnLevels.Contains(warnLowCheckBox.Text.ToUpper());
            noWarnCheckBox.Checked = m_options.WarnLevels.Contains(noWarnCheckBox.Text.ToUpper());
            warnHighButton.Enabled = warnHighCheckBox.Checked;
            warnMediumButton.Enabled = warnMediumCheckBox.Checked;
            warnLowButton.Enabled = warnLowCheckBox.Checked;
            noWarnButton.Enabled = noWarnCheckBox.Checked;
            warnHighButton.BackColor = m_options.WarnHighColor;
            warnMediumButton.BackColor = m_options.WarnMediumColor;
            warnLowButton.BackColor = m_options.WarnLowColor;
            noWarnButton.BackColor = m_options.NoWarnColor;

            sqlPreviewCheckBox.Checked = m_options.SQLPreview;
            highLightCheckBox.Checked = m_options.HighLightChanges;
            sqlPreviewButton.Enabled = sqlPreviewCheckBox.Checked;
            highLightButton.Enabled = highLightCheckBox.Checked;
            sqlPreviewButton.ForeColor = m_options.SQLPreviewColor;
            highLightButton.ForeColor = m_options.HighLightChangesColor;

            showManageabilityCheckBox.Checked = m_options.ShowManageabilityQueries;
            maxWMSGraphPoints.Text = (m_options.MaxGraphPoints != 0 ? m_options.MaxGraphPoints.ToString() : MonitorWorkloadOptions.DefaultMaxGraphPoints.ToString());
        }

        private void resetFilterStates()
        {
            executingCheckBox.Checked = true;
            waitingCheckBox.Checked = true;
            holdingCheckBox.Checked = true;
            suspendedCheckBox.Checked = true;
            rejectedCheckBox.Checked = true;
            completedCheckBox.Checked = true;
            executingButton.ForeColor = Color.Green;
            waitingButton.ForeColor = Color.DimGray;
            holdingButton.ForeColor = Color.DimGray;
            suspendedButton.ForeColor = Color.DimGray;
            rejectedButton.ForeColor = Color.Sienna;
            completedButton.ForeColor = Color.Blue;

            warnHighCheckBox.Checked = true;
            warnMediumCheckBox.Checked = true;
            warnLowCheckBox.Checked = true;
            noWarnCheckBox.Checked = true;
            warnHighButton.BackColor = Color.WhiteSmoke;
            warnMediumButton.BackColor = Color.WhiteSmoke;
            warnLowButton.BackColor = Color.WhiteSmoke;
            noWarnButton.BackColor = Color.WhiteSmoke;

            sqlPreviewCheckBox.Checked = false;
            highLightCheckBox.Checked = true;
            sqlPreviewButton.ForeColor = Color.DimGray;
            highLightButton.ForeColor = Color.Blue;

            showManageabilityCheckBox.Checked = true;
        }

        private void okButton_Click(object sender, EventArgs e)
        {
            m_options.QueryStates.Clear();
            if (executingCheckBox.Checked)
            {
                m_options.QueryStates.Add(executingCheckBox.Text.ToUpper());
            }
            if (waitingCheckBox.Checked)
            {
                m_options.QueryStates.Add(waitingCheckBox.Text.ToUpper());
            }
            if (holdingCheckBox.Checked)
            {
                m_options.QueryStates.Add(holdingCheckBox.Text.ToUpper());
            }
            if (suspendedCheckBox.Checked)
            {
                m_options.QueryStates.Add(suspendedCheckBox.Text.ToUpper());
            }
            if (rejectedCheckBox.Checked)
            {
                m_options.QueryStates.Add(rejectedCheckBox.Text.ToUpper());
            }
            if (completedCheckBox.Checked)
            {
                m_options.QueryStates.Add(completedCheckBox.Text.ToUpper());
            }

            m_options.WarnLevels.Clear();
            if (warnHighCheckBox.Checked)
            {
                m_options.WarnLevels.Add(warnHighCheckBox.Text.ToUpper());
            }
            if (warnMediumCheckBox.Checked)
            {
                m_options.WarnLevels.Add(warnMediumCheckBox.Text.ToUpper());
            }
            if (warnLowCheckBox.Checked)
            {
                m_options.WarnLevels.Add(warnLowCheckBox.Text.ToUpper());
            }
            if (noWarnCheckBox.Checked)
            {
                m_options.WarnLevels.Add(noWarnCheckBox.Text.ToUpper());
            }

            m_options.SQLPreview = sqlPreviewCheckBox.Checked;
            m_options.HighLightChanges = highLightCheckBox.Checked;

            m_options.ExecutingColor = executingButton.ForeColor;
            m_options.WaitingColor = waitingButton.ForeColor;
            m_options.HoldingColor = holdingButton.ForeColor;
            m_options.SuspendedColor = suspendedButton.ForeColor;
            m_options.RejectedColor = rejectedButton.ForeColor;
            m_options.CompletedColor = completedButton.ForeColor;

            m_options.WarnHighColor = warnHighButton.BackColor;
            m_options.WarnMediumColor = warnMediumButton.BackColor;
            m_options.WarnLowColor = warnLowButton.BackColor;
            m_options.NoWarnColor = noWarnButton.BackColor;

            m_options.SQLPreviewColor = sqlPreviewButton.ForeColor;
            m_options.HighLightChangesColor = highLightButton.ForeColor;

            m_options.ShowManageabilityQueries = showManageabilityCheckBox.Checked;
            m_options.MaxGraphPoints = Int32.Parse(maxWMSGraphPoints.Text);

            m_options.SaveIntoPersistence();

            DialogResult = DialogResult.OK;
            this.Close();
        }

        private void resetButton_Click(object sender, EventArgs e)
        {
            resetFilterStates();
            maxWMSGraphPoints.Text = MonitorWorkloadOptions.DefaultMaxGraphPoints.ToString();
        }

        private void cancelButton_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        private void warnHighButton_Click(object sender, EventArgs e)
        {
            ColorDialog colorDLg = new ColorDialog();
            DialogResult result = colorDLg.ShowDialog();
            if (result == DialogResult.OK)
            {
                m_tempWarnHigh = warnHighButton.BackColor = colorDLg.Color;
            }
        }

        private void warnMediumButton_Click(object sender, EventArgs e)
        {
            ColorDialog colorDLg = new ColorDialog();
            DialogResult result = colorDLg.ShowDialog();
            if (result == DialogResult.OK)
            {
                m_tempWarnMedium = warnMediumButton.BackColor = colorDLg.Color;
            }
        }

        private void warnLowButton_Click(object sender, EventArgs e)
        {
            ColorDialog colorDLg = new ColorDialog();
            DialogResult result = colorDLg.ShowDialog();
            if (result == DialogResult.OK)
            {
                m_tempWarnLow = warnLowButton.BackColor = colorDLg.Color;
            }
        }

        private void noWarnButton_Click(object sender, EventArgs e)
        {
            ColorDialog colorDLg = new ColorDialog();
            DialogResult result = colorDLg.ShowDialog();
            if (result == DialogResult.OK)
            {
                m_tempNoWarn = noWarnButton.BackColor = colorDLg.Color;
            }
        }

        private void executingButton_Click(object sender, EventArgs e)
        {
            ColorDialog colorDLg = new ColorDialog();
            DialogResult result = colorDLg.ShowDialog();
            if (result == DialogResult.OK)
            {
                executingButton.ForeColor = colorDLg.Color;
            }
        }

        private void waitingButton_Click(object sender, EventArgs e)
        {
            ColorDialog colorDLg = new ColorDialog();
            DialogResult result = colorDLg.ShowDialog();
            if (result == DialogResult.OK)
            {
                waitingButton.ForeColor = colorDLg.Color;
            }
        }

        private void holdingButton_Click(object sender, EventArgs e)
        {
            ColorDialog colorDLg = new ColorDialog();
            DialogResult result = colorDLg.ShowDialog();
            if (result == DialogResult.OK)
            {
                holdingButton.ForeColor = colorDLg.Color;
            }
        }

        private void suspendedButton_Click(object sender, EventArgs e)
        {
            ColorDialog colorDLg = new ColorDialog();
            DialogResult result = colorDLg.ShowDialog();
            if (result == DialogResult.OK)
            {
                suspendedButton.ForeColor = colorDLg.Color;
            }
        }

        private void rejectedButton_Click(object sender, EventArgs e)
        {
            ColorDialog colorDLg = new ColorDialog();
            DialogResult result = colorDLg.ShowDialog();
            if (result == DialogResult.OK)
            {
                rejectedButton.ForeColor = colorDLg.Color;
            }
        }

        private void completedButton_Click(object sender, EventArgs e)
        {
            ColorDialog colorDLg = new ColorDialog();
            DialogResult result = colorDLg.ShowDialog();
            if (result == DialogResult.OK)
            {
                completedButton.ForeColor = colorDLg.Color;
            }
        }

        private void executingCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            executingButton.Enabled = executingCheckBox.Checked;
        }

        private void waitingCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            waitingButton.Enabled = waitingCheckBox.Checked;
        }

        private void holdingCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            holdingButton.Enabled = holdingCheckBox.Checked;
        }

        private void suspendedCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            suspendedButton.Enabled = suspendedCheckBox.Checked;
        }

        private void rejectedCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            rejectedButton.Enabled = rejectedCheckBox.Checked;
        }

        private void completedCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            completedButton.Enabled = completedCheckBox.Checked;
        }

        private void warnHighCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            warnHighButton.Enabled = warnHighCheckBox.Checked;
            warnHighButton.BackColor = warnHighCheckBox.Checked ? m_tempWarnHigh : Color.WhiteSmoke;
        }

        private void warnMediumCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            warnMediumButton.Enabled = warnMediumCheckBox.Checked;
            warnMediumButton.BackColor = warnMediumCheckBox.Checked ? m_tempWarnMedium : Color.WhiteSmoke;
        }

        private void warnLowCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            warnLowButton.Enabled = warnLowCheckBox.Checked;
            warnLowButton.BackColor = warnLowCheckBox.Checked ? m_tempWarnLow : Color.WhiteSmoke;
        }

        private void noWarnCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            noWarnButton.Enabled = noWarnCheckBox.Checked;
            noWarnButton.BackColor = noWarnCheckBox.Checked ? m_tempNoWarn : Color.WhiteSmoke;
        }

        private void sqlPreviewButton_Click(object sender, EventArgs e)
        {
            ColorDialog colorDLg = new ColorDialog();
            DialogResult result = colorDLg.ShowDialog();
            if (result == DialogResult.OK)
            {
                sqlPreviewButton.ForeColor = colorDLg.Color;
            }
        }

        private void highLightButton_Click(object sender, EventArgs e)
        {
            ColorDialog colorDLg = new ColorDialog();
            DialogResult result = colorDLg.ShowDialog();
            if (result == DialogResult.OK)
            {
                highLightButton.ForeColor = colorDLg.Color;
            }
        }

        private void sqlPreviewCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            sqlPreviewButton.Enabled = sqlPreviewCheckBox.Checked;
        }

        private void highLightCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            highLightButton.Enabled = highLightCheckBox.Checked;
        }

        private void helpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.QueryMonitoringOptions);
        }

    }
}
