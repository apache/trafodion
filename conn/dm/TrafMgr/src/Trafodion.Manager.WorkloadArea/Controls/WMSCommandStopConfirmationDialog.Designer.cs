// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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
namespace Trafodion.Manager.WorkloadArea.Controls
{
    partial class WMSCommandStopConfirmationDialog
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(WMSCommandStopConfirmationDialog));
            this.servicesTextBox = new System.Windows.Forms.TextBox();
            this.notAllServicesActiveLabel = new System.Windows.Forms.Label();
            this.systemServicesWarningLabel = new System.Windows.Forms.Label();
            this.okButton = new System.Windows.Forms.Button();
            this.cancelButton = new System.Windows.Forms.Button();
            this.cmdStopConfirmationHeadingLabel = new System.Windows.Forms.Label();
            this.immediateStopCheckBox = new System.Windows.Forms.CheckBox();
            this.nccWMSCommandStopToolTip = new System.Windows.Forms.ToolTip(this.components);
            this.cautionPictureBox = new System.Windows.Forms.PictureBox();
            this.systemServicesWarningPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.notAllServicesPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.confirmationPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.listPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.buttonPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.padderPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            ((System.ComponentModel.ISupportInitialize)(this.cautionPictureBox)).BeginInit();
            this.systemServicesWarningPanel.SuspendLayout();
            this.notAllServicesPanel.SuspendLayout();
            this.confirmationPanel.SuspendLayout();
            this.listPanel.SuspendLayout();
            this.buttonPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // servicesTextBox
            // 
            this.servicesTextBox.Location = new System.Drawing.Point(25, 1);
            this.servicesTextBox.Multiline = true;
            this.servicesTextBox.Name = "servicesTextBox";
            this.servicesTextBox.ReadOnly = true;
            this.servicesTextBox.ScrollBars = System.Windows.Forms.ScrollBars.Both;
            this.servicesTextBox.Size = new System.Drawing.Size(425, 120);
            this.servicesTextBox.TabIndex = 0;
            this.nccWMSCommandStopToolTip.SetToolTip(this.servicesTextBox, "List of WMS Services to be stopped");
            // 
            // notAllServicesActiveLabel
            // 
            this.notAllServicesActiveLabel.AutoSize = true;
            this.notAllServicesActiveLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.notAllServicesActiveLabel.ForeColor = System.Drawing.Color.ForestGreen;
            this.notAllServicesActiveLabel.Location = new System.Drawing.Point(90, 3);
            this.notAllServicesActiveLabel.Name = "notAllServicesActiveLabel";
            this.notAllServicesActiveLabel.Size = new System.Drawing.Size(337, 26);
            this.notAllServicesActiveLabel.TabIndex = 0;
            this.notAllServicesActiveLabel.Text = "Some of the selected services are already in STOPPED state.\r\nOnly the ACTIVE or o" +
                "n HOLD or TRANSIENT services will be stopped.";
            this.notAllServicesActiveLabel.Visible = false;
            // 
            // systemServicesWarningLabel
            // 
            this.systemServicesWarningLabel.AutoSize = true;
            this.systemServicesWarningLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.systemServicesWarningLabel.ForeColor = System.Drawing.Color.Red;
            this.systemServicesWarningLabel.Location = new System.Drawing.Point(90, 6);
            this.systemServicesWarningLabel.Name = "systemServicesWarningLabel";
            this.systemServicesWarningLabel.Size = new System.Drawing.Size(253, 26);
            this.systemServicesWarningLabel.TabIndex = 0;
            this.systemServicesWarningLabel.Text = "Stopping of System defined services is not allowed.\r\nIgnoring stop request for al" +
                "l system services ...";
            this.systemServicesWarningLabel.TextAlign = System.Drawing.ContentAlignment.TopCenter;
            this.systemServicesWarningLabel.Visible = false;
            // 
            // okButton
            // 
            this.okButton.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.okButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.okButton.Location = new System.Drawing.Point(253, 26);
            this.okButton.Name = "okButton";
            this.okButton.Size = new System.Drawing.Size(90, 25);
            this.okButton.TabIndex = 3;
            this.okButton.Text = "&Yes";
            this.nccWMSCommandStopToolTip.SetToolTip(this.okButton, "Stop the WMS services");
            this.okButton.UseVisualStyleBackColor = true;
            this.okButton.Click += new System.EventHandler(this.okButton_Click);
            // 
            // cancelButton
            // 
            this.cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.cancelButton.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.cancelButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.cancelButton.Location = new System.Drawing.Point(363, 26);
            this.cancelButton.Name = "cancelButton";
            this.cancelButton.Size = new System.Drawing.Size(90, 25);
            this.cancelButton.TabIndex = 2;
            this.cancelButton.Text = "&No";
            this.nccWMSCommandStopToolTip.SetToolTip(this.cancelButton, "Cancel the operation");
            this.cancelButton.UseVisualStyleBackColor = true;
            // 
            // cmdStopConfirmationHeadingLabel
            // 
            this.cmdStopConfirmationHeadingLabel.AutoSize = true;
            this.cmdStopConfirmationHeadingLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.cmdStopConfirmationHeadingLabel.Location = new System.Drawing.Point(90, 13);
            this.cmdStopConfirmationHeadingLabel.Name = "cmdStopConfirmationHeadingLabel";
            this.cmdStopConfirmationHeadingLabel.Size = new System.Drawing.Size(295, 13);
            this.cmdStopConfirmationHeadingLabel.TabIndex = 0;
            this.cmdStopConfirmationHeadingLabel.Text = "Please confirm that you wish to stop the following services?";
            // 
            // immediateStopCheckBox
            // 
            this.immediateStopCheckBox.AutoSize = true;
            this.immediateStopCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.immediateStopCheckBox.Location = new System.Drawing.Point(25, 6);
            this.immediateStopCheckBox.Name = "immediateStopCheckBox";
            this.immediateStopCheckBox.Size = new System.Drawing.Size(109, 17);
            this.immediateStopCheckBox.TabIndex = 1;
            this.immediateStopCheckBox.Text = "Stop Immediately";
            this.nccWMSCommandStopToolTip.SetToolTip(this.immediateStopCheckBox, global::Trafodion.Manager.WorkloadArea.Properties.Resources.immediateStopCheckBoxToolTip);
            this.immediateStopCheckBox.UseVisualStyleBackColor = true;
            // 
            // nccWMSCommandStopToolTip
            // 
            this.nccWMSCommandStopToolTip.AutoPopDelay = 8000;
            this.nccWMSCommandStopToolTip.InitialDelay = 500;
            this.nccWMSCommandStopToolTip.ReshowDelay = 100;
            // 
            // cautionPictureBox
            // 
            this.cautionPictureBox.Image = System.Drawing.SystemIcons.Question.ToBitmap();
            this.cautionPictureBox.Location = new System.Drawing.Point(25, 0);
            this.cautionPictureBox.Name = "cautionPictureBox";
            this.cautionPictureBox.Size = new System.Drawing.Size(32, 32);
            this.cautionPictureBox.SizeMode = System.Windows.Forms.PictureBoxSizeMode.AutoSize;
            this.cautionPictureBox.TabIndex = 5;
            this.cautionPictureBox.TabStop = false;
            // 
            // systemServicesWarningPanel
            // 
            this.systemServicesWarningPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.systemServicesWarningPanel.Controls.Add(this.systemServicesWarningLabel);
            this.systemServicesWarningPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this.systemServicesWarningPanel.Location = new System.Drawing.Point(0, 10);
            this.systemServicesWarningPanel.Name = "systemServicesWarningPanel";
            this.systemServicesWarningPanel.Size = new System.Drawing.Size(484, 35);
            this.systemServicesWarningPanel.TabIndex = 6;
            // 
            // notAllServicesPanel
            // 
            this.notAllServicesPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.notAllServicesPanel.Controls.Add(this.notAllServicesActiveLabel);
            this.notAllServicesPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this.notAllServicesPanel.Location = new System.Drawing.Point(0, 45);
            this.notAllServicesPanel.Name = "notAllServicesPanel";
            this.notAllServicesPanel.Size = new System.Drawing.Size(484, 32);
            this.notAllServicesPanel.TabIndex = 7;
            // 
            // confirmationPanel
            // 
            this.confirmationPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.confirmationPanel.Controls.Add(this.cautionPictureBox);
            this.confirmationPanel.Controls.Add(this.cmdStopConfirmationHeadingLabel);
            this.confirmationPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this.confirmationPanel.Location = new System.Drawing.Point(0, 77);
            this.confirmationPanel.Name = "confirmationPanel";
            this.confirmationPanel.Size = new System.Drawing.Size(484, 41);
            this.confirmationPanel.TabIndex = 8;
            // 
            // listPanel
            // 
            this.listPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.listPanel.Controls.Add(this.servicesTextBox);
            this.listPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this.listPanel.Location = new System.Drawing.Point(0, 118);
            this.listPanel.Name = "listPanel";
            this.listPanel.Size = new System.Drawing.Size(484, 124);
            this.listPanel.TabIndex = 9;
            // 
            // buttonPanel
            // 
            this.buttonPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.buttonPanel.Controls.Add(this.immediateStopCheckBox);
            this.buttonPanel.Controls.Add(this.okButton);
            this.buttonPanel.Controls.Add(this.cancelButton);
            this.buttonPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.buttonPanel.Location = new System.Drawing.Point(0, 242);
            this.buttonPanel.Name = "buttonPanel";
            this.buttonPanel.Size = new System.Drawing.Size(484, 57);
            this.buttonPanel.TabIndex = 10;
            // 
            // padderPanel
            // 
            this.padderPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.padderPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this.padderPanel.Location = new System.Drawing.Point(0, 0);
            this.padderPanel.Name = "padderPanel";
            this.padderPanel.Size = new System.Drawing.Size(484, 10);
            this.padderPanel.TabIndex = 1;
            // 
            // WMSCommandStopConfirmationDialog
            // 
            this.AcceptButton = this.okButton;
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.CancelButton = this.cancelButton;
            this.ClientSize = new System.Drawing.Size(484, 299);
            this.Controls.Add(this.buttonPanel);
            this.Controls.Add(this.listPanel);
            this.Controls.Add(this.confirmationPanel);
            this.Controls.Add(this.notAllServicesPanel);
            this.Controls.Add(this.systemServicesWarningPanel);
            this.Controls.Add(this.padderPanel);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "WMSCommandStopConfirmationDialog";
            this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
            this.Text = "Trafodion Database Manager - Stop Service(s)";
            this.Load += new System.EventHandler(this.WMSCommandStopConfirmationDialog_Load);
            ((System.ComponentModel.ISupportInitialize)(this.cautionPictureBox)).EndInit();
            this.systemServicesWarningPanel.ResumeLayout(false);
            this.systemServicesWarningPanel.PerformLayout();
            this.notAllServicesPanel.ResumeLayout(false);
            this.notAllServicesPanel.PerformLayout();
            this.confirmationPanel.ResumeLayout(false);
            this.confirmationPanel.PerformLayout();
            this.listPanel.ResumeLayout(false);
            this.listPanel.PerformLayout();
            this.buttonPanel.ResumeLayout(false);
            this.buttonPanel.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TextBox servicesTextBox;
        private System.Windows.Forms.Label notAllServicesActiveLabel;
        private System.Windows.Forms.Label systemServicesWarningLabel;
        private System.Windows.Forms.Button okButton;
        private System.Windows.Forms.Button cancelButton;
        private System.Windows.Forms.Label cmdStopConfirmationHeadingLabel;
        private System.Windows.Forms.CheckBox immediateStopCheckBox;
        private System.Windows.Forms.ToolTip nccWMSCommandStopToolTip;
        private System.Windows.Forms.PictureBox cautionPictureBox;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel systemServicesWarningPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel notAllServicesPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel confirmationPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel listPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel buttonPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel padderPanel;
    }
}