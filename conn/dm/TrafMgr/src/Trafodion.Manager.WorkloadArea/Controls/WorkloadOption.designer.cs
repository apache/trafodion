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
    partial class WorkloadOption
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(WorkloadOption));
            this.nccOptionsCancelButton = new System.Windows.Forms.Button();
            this.nccOptionsOKButton = new System.Windows.Forms.Button();
            this.timeoutGroupBox = new System.Windows.Forms.GroupBox();
            this.connectionTimeoutLabel = new System.Windows.Forms.Label();
            this.connectionTimeoutTextBox = new System.Windows.Forms.TextBox();
            this.commandTimeoutLabel = new System.Windows.Forms.Label();
            this.commandTimeoutTextBox = new System.Windows.Forms.TextBox();
            this.workbenchSettingsGroupBox = new System.Windows.Forms.GroupBox();
            this.nccOptionsSortByOperatorLevelCheckBox = new System.Windows.Forms.CheckBox();
            this.nccOptionsColorizeProcBoundariesCheckBox = new System.Windows.Forms.CheckBox();
            this.nccOptionsAutoVersionQueriesCheckBox = new System.Windows.Forms.CheckBox();
            this.nccOptionsAllowQueryExecutionCheckBox = new System.Windows.Forms.CheckBox();
            this.maxRowsLimitTextBox = new System.Windows.Forms.TextBox();
            this.maxRowsLimitLabel = new System.Windows.Forms.Label();
            this.timeLineSettingsGroupBox = new System.Windows.Forms.GroupBox();
            this.nccOptionsTimelineIntervalLabel = new System.Windows.Forms.Label();
            this.nccOptionsTimelineIntervalComboBox = new System.Windows.Forms.ComboBox();
            this.nccOptionsMetricGraphIntervalLabel = new System.Windows.Forms.Label();
            this.nccOptionsMetricGraphIntervalComboBox = new System.Windows.Forms.ComboBox();
            this.nccOptionsDownloadTimeLineCheckBox = new System.Windows.Forms.CheckBox();
            this.nccOptionsDisplayTimesInLocalTimezoneCheckBox = new System.Windows.Forms.CheckBox();
            this.triageSettingsGroupBox = new System.Windows.Forms.GroupBox();
            this.triageMaxRowsLimitTextBox = new System.Windows.Forms.TextBox();
            this.maxWMSGraphPointsTextBox = new System.Windows.Forms.TextBox();
            this.maxWMSGraphPointsLabel = new System.Windows.Forms.Label();
            this.triageMaxRowsLimitLabel = new System.Windows.Forms.Label();
            this.nccOptionsHideTransactionBoundariesCheckBox = new System.Windows.Forms.CheckBox();
            this.nccHideRecentlyCompletedQueries = new System.Windows.Forms.CheckBox();
            this.nccOptionsHideInternalQueries = new System.Windows.Forms.CheckBox();
            this.nccOptionsToolTip = new System.Windows.Forms.ToolTip(this.components);
            this.nccOptionsClientSkinComboBox = new System.Windows.Forms.ComboBox();
            this.nccOptionsClientSkinLabel = new System.Windows.Forms.Label();
            this.nccRepositorySchemaLabel = new System.Windows.Forms.Label();
            this.nccRepositorySchemaComboBox = new System.Windows.Forms.ComboBox();
            this.generalSettingsGroupBox = new System.Windows.Forms.GroupBox();
            this.repositorySettingsGroupBox = new System.Windows.Forms.GroupBox();
            this.timeoutGroupBox.SuspendLayout();
            this.workbenchSettingsGroupBox.SuspendLayout();
            this.timeLineSettingsGroupBox.SuspendLayout();
            this.triageSettingsGroupBox.SuspendLayout();
            this.generalSettingsGroupBox.SuspendLayout();
            this.repositorySettingsGroupBox.SuspendLayout();
            this.SuspendLayout();
            // 
            // nccOptionsCancelButton
            // 
            this.nccOptionsCancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.nccOptionsCancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.nccOptionsCancelButton.Font = new System.Drawing.Font("Lucida Sans Unicode", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.nccOptionsCancelButton.Image = global::Trafodion.Manager.WorkloadArea.Properties.Resources.cancelImage;
            this.nccOptionsCancelButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.nccOptionsCancelButton.Location = new System.Drawing.Point(395, 405);
            this.nccOptionsCancelButton.Name = "nccOptionsCancelButton";
            this.nccOptionsCancelButton.Padding = new System.Windows.Forms.Padding(3, 0, 0, 0);
            this.nccOptionsCancelButton.Size = new System.Drawing.Size(90, 25);
            this.nccOptionsCancelButton.TabIndex = 100;
            this.nccOptionsCancelButton.Text = "    &Cancel";
            this.nccOptionsToolTip.SetToolTip(this.nccOptionsCancelButton, "Discard all the changes");
            this.nccOptionsCancelButton.UseVisualStyleBackColor = true;
            this.nccOptionsCancelButton.Click += new System.EventHandler(this.nccOptionsCancelButton_Click);
            // 
            // nccOptionsOKButton
            // 
            this.nccOptionsOKButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.nccOptionsOKButton.Font = new System.Drawing.Font("Lucida Sans Unicode", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.nccOptionsOKButton.Image = global::Trafodion.Manager.WorkloadArea.Properties.Resources.ApplyIcon;
            this.nccOptionsOKButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.nccOptionsOKButton.Location = new System.Drawing.Point(290, 405);
            this.nccOptionsOKButton.Name = "nccOptionsOKButton";
            this.nccOptionsOKButton.Padding = new System.Windows.Forms.Padding(3, 0, 0, 0);
            this.nccOptionsOKButton.Size = new System.Drawing.Size(80, 25);
            this.nccOptionsOKButton.TabIndex = 99;
            this.nccOptionsOKButton.Text = "&OK";
            this.nccOptionsToolTip.SetToolTip(this.nccOptionsOKButton, "Accept all the changes made on this options dialog");
            this.nccOptionsOKButton.UseVisualStyleBackColor = true;
            this.nccOptionsOKButton.Click += new System.EventHandler(this.nccOptionsOKButton_Click);
            // 
            // timeoutGroupBox
            // 
            this.timeoutGroupBox.Controls.Add(this.connectionTimeoutLabel);
            this.timeoutGroupBox.Controls.Add(this.connectionTimeoutTextBox);
            this.timeoutGroupBox.Controls.Add(this.commandTimeoutLabel);
            this.timeoutGroupBox.Controls.Add(this.commandTimeoutTextBox);
            this.timeoutGroupBox.Font = new System.Drawing.Font("Lucida Sans Unicode", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.timeoutGroupBox.Location = new System.Drawing.Point(15, 15);
            this.timeoutGroupBox.Name = "timeoutGroupBox";
            this.timeoutGroupBox.Size = new System.Drawing.Size(250, 100);
            this.timeoutGroupBox.TabIndex = 1;
            this.timeoutGroupBox.TabStop = false;
            this.timeoutGroupBox.Text = "Connection Settings (in secs)";
            // 
            // connectionTimeoutLabel
            // 
            this.connectionTimeoutLabel.AutoSize = true;
            this.connectionTimeoutLabel.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.connectionTimeoutLabel.Location = new System.Drawing.Point(17, 28);
            this.connectionTimeoutLabel.Name = "connectionTimeoutLabel";
            this.connectionTimeoutLabel.Size = new System.Drawing.Size(108, 16);
            this.connectionTimeoutLabel.TabIndex = 0;
            this.connectionTimeoutLabel.Text = "Co&nnection Timeout";
            this.nccOptionsToolTip.SetToolTip(this.connectionTimeoutLabel, "Connection timeout in seconds");
            // 
            // connectionTimeoutTextBox
            // 
            this.connectionTimeoutTextBox.Font = new System.Drawing.Font("Arial", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.connectionTimeoutTextBox.Location = new System.Drawing.Point(130, 25);
            this.connectionTimeoutTextBox.Name = "connectionTimeoutTextBox";
            this.connectionTimeoutTextBox.Size = new System.Drawing.Size(100, 21);
            this.connectionTimeoutTextBox.TabIndex = 1;
            this.nccOptionsToolTip.SetToolTip(this.connectionTimeoutTextBox, "Enter the connection timeout in seconds");
            // 
            // commandTimeoutLabel
            // 
            this.commandTimeoutLabel.AutoSize = true;
            this.commandTimeoutLabel.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.commandTimeoutLabel.Location = new System.Drawing.Point(27, 63);
            this.commandTimeoutLabel.Name = "commandTimeoutLabel";
            this.commandTimeoutLabel.Size = new System.Drawing.Size(98, 16);
            this.commandTimeoutLabel.TabIndex = 2;
            this.commandTimeoutLabel.Text = "Co&mmand Timeout";
            this.nccOptionsToolTip.SetToolTip(this.commandTimeoutLabel, "Timeout value used when executing a command\r\non the Trafodion platform");
            // 
            // commandTimeoutTextBox
            // 
            this.commandTimeoutTextBox.Font = new System.Drawing.Font("Arial", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.commandTimeoutTextBox.Location = new System.Drawing.Point(130, 60);
            this.commandTimeoutTextBox.Name = "commandTimeoutTextBox";
            this.commandTimeoutTextBox.Size = new System.Drawing.Size(100, 21);
            this.commandTimeoutTextBox.TabIndex = 3;
            this.nccOptionsToolTip.SetToolTip(this.commandTimeoutTextBox, "Enter the run time limit [maximum allowable] for \r\nexecuting commands on the Neov" +
        "iew platform");
            // 
            // workbenchSettingsGroupBox
            // 
            this.workbenchSettingsGroupBox.Controls.Add(this.nccOptionsSortByOperatorLevelCheckBox);
            this.workbenchSettingsGroupBox.Controls.Add(this.nccOptionsColorizeProcBoundariesCheckBox);
            this.workbenchSettingsGroupBox.Controls.Add(this.nccOptionsAutoVersionQueriesCheckBox);
            this.workbenchSettingsGroupBox.Controls.Add(this.nccOptionsAllowQueryExecutionCheckBox);
            this.workbenchSettingsGroupBox.Controls.Add(this.maxRowsLimitTextBox);
            this.workbenchSettingsGroupBox.Controls.Add(this.maxRowsLimitLabel);
            this.workbenchSettingsGroupBox.Font = new System.Drawing.Font("Lucida Sans Unicode", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.workbenchSettingsGroupBox.Location = new System.Drawing.Point(285, 200);
            this.workbenchSettingsGroupBox.Name = "workbenchSettingsGroupBox";
            this.workbenchSettingsGroupBox.Size = new System.Drawing.Size(190, 160);
            this.workbenchSettingsGroupBox.TabIndex = 41;
            this.workbenchSettingsGroupBox.TabStop = false;
            this.workbenchSettingsGroupBox.Text = "Query Workbench Settings";
            // 
            // nccOptionsSortByOperatorLevelCheckBox
            // 
            this.nccOptionsSortByOperatorLevelCheckBox.AutoSize = true;
            this.nccOptionsSortByOperatorLevelCheckBox.Checked = true;
            this.nccOptionsSortByOperatorLevelCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.nccOptionsSortByOperatorLevelCheckBox.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.nccOptionsSortByOperatorLevelCheckBox.Location = new System.Drawing.Point(25, 135);
            this.nccOptionsSortByOperatorLevelCheckBox.Name = "nccOptionsSortByOperatorLevelCheckBox";
            this.nccOptionsSortByOperatorLevelCheckBox.Size = new System.Drawing.Size(159, 20);
            this.nccOptionsSortByOperatorLevelCheckBox.TabIndex = 45;
            this.nccOptionsSortByOperatorLevelCheckBox.Text = "Sort Explain Grid by Levels";
            this.nccOptionsToolTip.SetToolTip(this.nccOptionsSortByOperatorLevelCheckBox, "Sort explain plan grid rows by operator level.\r\nDisable to sort according to grid" +
        " values.");
            this.nccOptionsSortByOperatorLevelCheckBox.UseVisualStyleBackColor = true;
            // 
            // nccOptionsColorizeProcBoundariesCheckBox
            // 
            this.nccOptionsColorizeProcBoundariesCheckBox.AutoSize = true;
            this.nccOptionsColorizeProcBoundariesCheckBox.Checked = true;
            this.nccOptionsColorizeProcBoundariesCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.nccOptionsColorizeProcBoundariesCheckBox.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.nccOptionsColorizeProcBoundariesCheckBox.Location = new System.Drawing.Point(25, 110);
            this.nccOptionsColorizeProcBoundariesCheckBox.Name = "nccOptionsColorizeProcBoundariesCheckBox";
            this.nccOptionsColorizeProcBoundariesCheckBox.Size = new System.Drawing.Size(153, 20);
            this.nccOptionsColorizeProcBoundariesCheckBox.TabIndex = 44;
            this.nccOptionsColorizeProcBoundariesCheckBox.Text = "Color &Process Boundaries";
            this.nccOptionsToolTip.SetToolTip(this.nccOptionsColorizeProcBoundariesCheckBox, "Color process boundaries for explain plans");
            this.nccOptionsColorizeProcBoundariesCheckBox.UseVisualStyleBackColor = true;
            // 
            // nccOptionsAutoVersionQueriesCheckBox
            // 
            this.nccOptionsAutoVersionQueriesCheckBox.AutoSize = true;
            this.nccOptionsAutoVersionQueriesCheckBox.Checked = true;
            this.nccOptionsAutoVersionQueriesCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.nccOptionsAutoVersionQueriesCheckBox.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.nccOptionsAutoVersionQueriesCheckBox.Location = new System.Drawing.Point(25, 60);
            this.nccOptionsAutoVersionQueriesCheckBox.Name = "nccOptionsAutoVersionQueriesCheckBox";
            this.nccOptionsAutoVersionQueriesCheckBox.Size = new System.Drawing.Size(133, 20);
            this.nccOptionsAutoVersionQueriesCheckBox.TabIndex = 42;
            this.nccOptionsAutoVersionQueriesCheckBox.Text = "&Auto Version Queries";
            this.nccOptionsToolTip.SetToolTip(this.nccOptionsAutoVersionQueriesCheckBox, "Automatically add and version statements if \r\nit is executed or explained");
            this.nccOptionsAutoVersionQueriesCheckBox.UseVisualStyleBackColor = true;
            // 
            // nccOptionsAllowQueryExecutionCheckBox
            // 
            this.nccOptionsAllowQueryExecutionCheckBox.AutoSize = true;
            this.nccOptionsAllowQueryExecutionCheckBox.Checked = true;
            this.nccOptionsAllowQueryExecutionCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.nccOptionsAllowQueryExecutionCheckBox.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.nccOptionsAllowQueryExecutionCheckBox.Location = new System.Drawing.Point(25, 85);
            this.nccOptionsAllowQueryExecutionCheckBox.Name = "nccOptionsAllowQueryExecutionCheckBox";
            this.nccOptionsAllowQueryExecutionCheckBox.Size = new System.Drawing.Size(146, 20);
            this.nccOptionsAllowQueryExecutionCheckBox.TabIndex = 43;
            this.nccOptionsAllowQueryExecutionCheckBox.Text = "Enable Query &Execution";
            this.nccOptionsToolTip.SetToolTip(this.nccOptionsAllowQueryExecutionCheckBox, "Enable or disable query execution buttons");
            this.nccOptionsAllowQueryExecutionCheckBox.UseVisualStyleBackColor = true;
            // 
            // maxRowsLimitTextBox
            // 
            this.maxRowsLimitTextBox.Font = new System.Drawing.Font("Arial", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.maxRowsLimitTextBox.Location = new System.Drawing.Point(75, 25);
            this.maxRowsLimitTextBox.Name = "maxRowsLimitTextBox";
            this.maxRowsLimitTextBox.Size = new System.Drawing.Size(100, 21);
            this.maxRowsLimitTextBox.TabIndex = 41;
            this.nccOptionsToolTip.SetToolTip(this.maxRowsLimitTextBox, "Enter the maximum number of rows to fetch for any statement executed \r\nin the wor" +
        "kbench. You can override this value on a per-query basis in the \r\nquery workbenc" +
        "h.");
            // 
            // maxRowsLimitLabel
            // 
            this.maxRowsLimitLabel.AutoSize = true;
            this.maxRowsLimitLabel.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.maxRowsLimitLabel.Location = new System.Drawing.Point(12, 28);
            this.maxRowsLimitLabel.Name = "maxRowsLimitLabel";
            this.maxRowsLimitLabel.Size = new System.Drawing.Size(56, 16);
            this.maxRowsLimitLabel.TabIndex = 40;
            this.maxRowsLimitLabel.Text = "Max Rows";
            this.nccOptionsToolTip.SetToolTip(this.maxRowsLimitLabel, "The maximum number of rows to fetch for any statement \r\nexecuted in  the workbenc" +
        "h. This value can be overriden\r\non a per-query basis.");
            // 
            // timeLineSettingsGroupBox
            // 
            this.timeLineSettingsGroupBox.Controls.Add(this.nccOptionsTimelineIntervalLabel);
            this.timeLineSettingsGroupBox.Controls.Add(this.nccOptionsTimelineIntervalComboBox);
            this.timeLineSettingsGroupBox.Controls.Add(this.nccOptionsMetricGraphIntervalLabel);
            this.timeLineSettingsGroupBox.Controls.Add(this.nccOptionsMetricGraphIntervalComboBox);
            this.timeLineSettingsGroupBox.Controls.Add(this.nccOptionsDownloadTimeLineCheckBox);
            this.timeLineSettingsGroupBox.Controls.Add(this.nccOptionsDisplayTimesInLocalTimezoneCheckBox);
            this.timeLineSettingsGroupBox.Font = new System.Drawing.Font("Lucida Sans Unicode", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.timeLineSettingsGroupBox.Location = new System.Drawing.Point(15, 125);
            this.timeLineSettingsGroupBox.Name = "timeLineSettingsGroupBox";
            this.timeLineSettingsGroupBox.Size = new System.Drawing.Size(250, 155);
            this.timeLineSettingsGroupBox.TabIndex = 11;
            this.timeLineSettingsGroupBox.TabStop = false;
            this.timeLineSettingsGroupBox.Text = "Time Settings";
            // 
            // nccOptionsTimelineIntervalLabel
            // 
            this.nccOptionsTimelineIntervalLabel.AutoSize = true;
            this.nccOptionsTimelineIntervalLabel.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.nccOptionsTimelineIntervalLabel.Location = new System.Drawing.Point(34, 28);
            this.nccOptionsTimelineIntervalLabel.Name = "nccOptionsTimelineIntervalLabel";
            this.nccOptionsTimelineIntervalLabel.Size = new System.Drawing.Size(91, 16);
            this.nccOptionsTimelineIntervalLabel.TabIndex = 4;
            this.nccOptionsTimelineIntervalLabel.Text = "Timeline Interval";
            this.nccOptionsToolTip.SetToolTip(this.nccOptionsTimelineIntervalLabel, "The period of time to display on the timeline");
            // 
            // nccOptionsTimelineIntervalComboBox
            // 
            this.nccOptionsTimelineIntervalComboBox.AutoCompleteCustomSource.AddRange(new string[] {
            "hours",
            "day",
            "days",
            "week",
            "weeks",
            "month",
            "months"});
            this.nccOptionsTimelineIntervalComboBox.DropDownWidth = 5;
            this.nccOptionsTimelineIntervalComboBox.Font = new System.Drawing.Font("Arial", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.nccOptionsTimelineIntervalComboBox.FormattingEnabled = true;
            this.nccOptionsTimelineIntervalComboBox.Items.AddRange(new object[] {
            "6 hours",
            "12 hours",
            "1 day",
            "2 days",
            "5 days",
            "1 week",
            "2 weeks"});
            this.nccOptionsTimelineIntervalComboBox.Location = new System.Drawing.Point(130, 25);
            this.nccOptionsTimelineIntervalComboBox.Name = "nccOptionsTimelineIntervalComboBox";
            this.nccOptionsTimelineIntervalComboBox.Size = new System.Drawing.Size(100, 22);
            this.nccOptionsTimelineIntervalComboBox.TabIndex = 5;
            this.nccOptionsToolTip.SetToolTip(this.nccOptionsTimelineIntervalComboBox, "Select or enter the period of time to display on the timeline. \r\nAllows you to en" +
        "ter a numeric value followed by a time unit :  \r\n       second(s), minute(s), ho" +
        "ur(s) or day(s).");
            // 
            // nccOptionsMetricGraphIntervalLabel
            // 
            this.nccOptionsMetricGraphIntervalLabel.AutoSize = true;
            this.nccOptionsMetricGraphIntervalLabel.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.nccOptionsMetricGraphIntervalLabel.Location = new System.Drawing.Point(8, 63);
            this.nccOptionsMetricGraphIntervalLabel.Name = "nccOptionsMetricGraphIntervalLabel";
            this.nccOptionsMetricGraphIntervalLabel.Size = new System.Drawing.Size(117, 16);
            this.nccOptionsMetricGraphIntervalLabel.TabIndex = 6;
            this.nccOptionsMetricGraphIntervalLabel.Text = "Metric Graph Interval";
            this.nccOptionsToolTip.SetToolTip(this.nccOptionsMetricGraphIntervalLabel, "The default time interval to show on all \r\nthe workload and system metric graphs");
            // 
            // nccOptionsMetricGraphIntervalComboBox
            // 
            this.nccOptionsMetricGraphIntervalComboBox.AutoCompleteCustomSource.AddRange(new string[] {
            "minute",
            "minutes",
            "hour",
            "hours",
            "day",
            "days",
            "week",
            "weeks",
            "month",
            "months"});
            this.nccOptionsMetricGraphIntervalComboBox.DropDownWidth = 100;
            this.nccOptionsMetricGraphIntervalComboBox.Font = new System.Drawing.Font("Arial", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.nccOptionsMetricGraphIntervalComboBox.FormattingEnabled = true;
            this.nccOptionsMetricGraphIntervalComboBox.Items.AddRange(new object[] {
            "30 minutes",
            "45 minutes",
            "1 hour",
            "2 hours",
            "5 hours ",
            "6 hours",
            "12 hours"});
            this.nccOptionsMetricGraphIntervalComboBox.Location = new System.Drawing.Point(130, 60);
            this.nccOptionsMetricGraphIntervalComboBox.Name = "nccOptionsMetricGraphIntervalComboBox";
            this.nccOptionsMetricGraphIntervalComboBox.Size = new System.Drawing.Size(100, 22);
            this.nccOptionsMetricGraphIntervalComboBox.TabIndex = 7;
            this.nccOptionsToolTip.SetToolTip(this.nccOptionsMetricGraphIntervalComboBox, resources.GetString("nccOptionsMetricGraphIntervalComboBox.ToolTip"));
            // 
            // nccOptionsDownloadTimeLineCheckBox
            // 
            this.nccOptionsDownloadTimeLineCheckBox.AutoSize = true;
            this.nccOptionsDownloadTimeLineCheckBox.Checked = true;
            this.nccOptionsDownloadTimeLineCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.nccOptionsDownloadTimeLineCheckBox.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.nccOptionsDownloadTimeLineCheckBox.Location = new System.Drawing.Point(66, 100);
            this.nccOptionsDownloadTimeLineCheckBox.Name = "nccOptionsDownloadTimeLineCheckBox";
            this.nccOptionsDownloadTimeLineCheckBox.Size = new System.Drawing.Size(161, 20);
            this.nccOptionsDownloadTimeLineCheckBox.TabIndex = 13;
            this.nccOptionsDownloadTimeLineCheckBox.Text = "Fetch &Timeline Information";
            this.nccOptionsToolTip.SetToolTip(this.nccOptionsDownloadTimeLineCheckBox, "Downloads or fetches timeline information at connection time and at regular refre" +
        "sh intervals");
            this.nccOptionsDownloadTimeLineCheckBox.UseVisualStyleBackColor = true;
            this.nccOptionsDownloadTimeLineCheckBox.CheckedChanged += new System.EventHandler(this.nccOptionsEnableTimeLineCheckBox_CheckedChanged);
            // 
            // nccOptionsDisplayTimesInLocalTimezoneCheckBox
            // 
            this.nccOptionsDisplayTimesInLocalTimezoneCheckBox.AutoSize = true;
            this.nccOptionsDisplayTimesInLocalTimezoneCheckBox.Checked = true;
            this.nccOptionsDisplayTimesInLocalTimezoneCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.nccOptionsDisplayTimesInLocalTimezoneCheckBox.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.nccOptionsDisplayTimesInLocalTimezoneCheckBox.Location = new System.Drawing.Point(66, 125);
            this.nccOptionsDisplayTimesInLocalTimezoneCheckBox.Name = "nccOptionsDisplayTimesInLocalTimezoneCheckBox";
            this.nccOptionsDisplayTimesInLocalTimezoneCheckBox.Size = new System.Drawing.Size(168, 20);
            this.nccOptionsDisplayTimesInLocalTimezoneCheckBox.TabIndex = 14;
            this.nccOptionsDisplayTimesInLocalTimezoneCheckBox.Text = "&Show times in local timezone";
            this.nccOptionsToolTip.SetToolTip(this.nccOptionsDisplayTimesInLocalTimezoneCheckBox, "Display all times in the local client timezone");
            this.nccOptionsDisplayTimesInLocalTimezoneCheckBox.UseVisualStyleBackColor = true;
            // 
            // triageSettingsGroupBox
            // 
            this.triageSettingsGroupBox.Controls.Add(this.triageMaxRowsLimitTextBox);
            this.triageSettingsGroupBox.Controls.Add(this.maxWMSGraphPointsTextBox);
            this.triageSettingsGroupBox.Controls.Add(this.maxWMSGraphPointsLabel);
            this.triageSettingsGroupBox.Controls.Add(this.triageMaxRowsLimitLabel);
            this.triageSettingsGroupBox.Controls.Add(this.nccOptionsHideTransactionBoundariesCheckBox);
            this.triageSettingsGroupBox.Controls.Add(this.nccHideRecentlyCompletedQueries);
            this.triageSettingsGroupBox.Controls.Add(this.nccOptionsHideInternalQueries);
            this.triageSettingsGroupBox.Font = new System.Drawing.Font("Lucida Sans Unicode", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.triageSettingsGroupBox.Location = new System.Drawing.Point(285, 15);
            this.triageSettingsGroupBox.Name = "triageSettingsGroupBox";
            this.triageSettingsGroupBox.Size = new System.Drawing.Size(190, 175);
            this.triageSettingsGroupBox.TabIndex = 31;
            this.triageSettingsGroupBox.TabStop = false;
            this.triageSettingsGroupBox.Text = "Triage Space Settings";
            // 
            // triageMaxRowsLimitTextBox
            // 
            this.triageMaxRowsLimitTextBox.Font = new System.Drawing.Font("Arial", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.triageMaxRowsLimitTextBox.Location = new System.Drawing.Point(75, 25);
            this.triageMaxRowsLimitTextBox.Name = "triageMaxRowsLimitTextBox";
            this.triageMaxRowsLimitTextBox.Size = new System.Drawing.Size(100, 21);
            this.triageMaxRowsLimitTextBox.TabIndex = 24;
            this.nccOptionsToolTip.SetToolTip(this.triageMaxRowsLimitTextBox, "Enter the maximum number of rows to fetch when loading the Triage Space");
            // 
            // maxWMSGraphPointsTextBox
            // 
            this.maxWMSGraphPointsTextBox.Font = new System.Drawing.Font("Arial", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.maxWMSGraphPointsTextBox.Location = new System.Drawing.Point(75, 60);
            this.maxWMSGraphPointsTextBox.Name = "maxWMSGraphPointsTextBox";
            this.maxWMSGraphPointsTextBox.Size = new System.Drawing.Size(100, 21);
            this.maxWMSGraphPointsTextBox.TabIndex = 26;
            this.nccOptionsToolTip.SetToolTip(this.maxWMSGraphPointsTextBox, "The maximum number of points to graph for statement, service and platform status " +
        "indicators in live monitoring mode");
            // 
            // maxWMSGraphPointsLabel
            // 
            this.maxWMSGraphPointsLabel.AutoSize = true;
            this.maxWMSGraphPointsLabel.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.maxWMSGraphPointsLabel.Location = new System.Drawing.Point(6, 63);
            this.maxWMSGraphPointsLabel.Name = "maxWMSGraphPointsLabel";
            this.maxWMSGraphPointsLabel.Size = new System.Drawing.Size(62, 16);
            this.maxWMSGraphPointsLabel.TabIndex = 25;
            this.maxWMSGraphPointsLabel.Text = "&Live Range";
            this.nccOptionsToolTip.SetToolTip(this.maxWMSGraphPointsLabel, "The maximum number of points to graph for statement, service and platform status " +
        "indicators in live monitoring mode");
            // 
            // triageMaxRowsLimitLabel
            // 
            this.triageMaxRowsLimitLabel.AutoSize = true;
            this.triageMaxRowsLimitLabel.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.triageMaxRowsLimitLabel.Location = new System.Drawing.Point(6, 28);
            this.triageMaxRowsLimitLabel.Name = "triageMaxRowsLimitLabel";
            this.triageMaxRowsLimitLabel.Size = new System.Drawing.Size(65, 16);
            this.triageMaxRowsLimitLabel.TabIndex = 23;
            this.triageMaxRowsLimitLabel.Text = "&Fetch Limit";
            this.nccOptionsToolTip.SetToolTip(this.triageMaxRowsLimitLabel, "The maximum number of rows to fetch when loading the Triage Space");
            // 
            // nccOptionsHideTransactionBoundariesCheckBox
            // 
            this.nccOptionsHideTransactionBoundariesCheckBox.AutoSize = true;
            this.nccOptionsHideTransactionBoundariesCheckBox.Checked = true;
            this.nccOptionsHideTransactionBoundariesCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.nccOptionsHideTransactionBoundariesCheckBox.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.nccOptionsHideTransactionBoundariesCheckBox.Location = new System.Drawing.Point(25, 120);
            this.nccOptionsHideTransactionBoundariesCheckBox.Name = "nccOptionsHideTransactionBoundariesCheckBox";
            this.nccOptionsHideTransactionBoundariesCheckBox.Size = new System.Drawing.Size(150, 20);
            this.nccOptionsHideTransactionBoundariesCheckBox.TabIndex = 34;
            this.nccOptionsHideTransactionBoundariesCheckBox.Text = "Hide T&ransaction Bounds";
            this.nccOptionsToolTip.SetToolTip(this.nccOptionsHideTransactionBoundariesCheckBox, "Hides the transaction boundaries or scope - start and end of transactions.");
            this.nccOptionsHideTransactionBoundariesCheckBox.UseVisualStyleBackColor = true;
            // 
            // nccHideRecentlyCompletedQueries
            // 
            this.nccHideRecentlyCompletedQueries.AutoSize = true;
            this.nccHideRecentlyCompletedQueries.Checked = true;
            this.nccHideRecentlyCompletedQueries.CheckState = System.Windows.Forms.CheckState.Checked;
            this.nccHideRecentlyCompletedQueries.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.nccHideRecentlyCompletedQueries.Location = new System.Drawing.Point(25, 145);
            this.nccHideRecentlyCompletedQueries.Name = "nccHideRecentlyCompletedQueries";
            this.nccHideRecentlyCompletedQueries.Size = new System.Drawing.Size(146, 20);
            this.nccHideRecentlyCompletedQueries.TabIndex = 35;
            this.nccHideRecentlyCompletedQueries.Text = "Hide Completed Queries";
            this.nccOptionsToolTip.SetToolTip(this.nccHideRecentlyCompletedQueries, "Hides recently completed queries in the live triage view");
            this.nccHideRecentlyCompletedQueries.UseVisualStyleBackColor = true;
            // 
            // nccOptionsHideInternalQueries
            // 
            this.nccOptionsHideInternalQueries.AutoSize = true;
            this.nccOptionsHideInternalQueries.Checked = true;
            this.nccOptionsHideInternalQueries.CheckState = System.Windows.Forms.CheckState.Checked;
            this.nccOptionsHideInternalQueries.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.nccOptionsHideInternalQueries.Location = new System.Drawing.Point(25, 95);
            this.nccOptionsHideInternalQueries.Name = "nccOptionsHideInternalQueries";
            this.nccOptionsHideInternalQueries.Size = new System.Drawing.Size(134, 20);
            this.nccOptionsHideInternalQueries.TabIndex = 33;
            this.nccOptionsHideInternalQueries.Text = "Hide &Internal Queries";
            this.nccOptionsToolTip.SetToolTip(this.nccOptionsHideInternalQueries, "Hides internal queries in the Triage Space");
            this.nccOptionsHideInternalQueries.UseVisualStyleBackColor = true;
            this.nccOptionsHideInternalQueries.Visible = false;
            // 
            // nccOptionsClientSkinComboBox
            // 
            this.nccOptionsClientSkinComboBox.AutoCompleteCustomSource.AddRange(new string[] {
            "hours",
            "day",
            "days",
            "week",
            "weeks",
            "month",
            "months"});
            this.nccOptionsClientSkinComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.nccOptionsClientSkinComboBox.DropDownWidth = 100;
            this.nccOptionsClientSkinComboBox.Font = new System.Drawing.Font("Arial", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.nccOptionsClientSkinComboBox.FormattingEnabled = true;
            this.nccOptionsClientSkinComboBox.Items.AddRange(new object[] {
            "Trafodion Performance Analyzer",
            "Trafodion Query Viewer",
            "Trafodion Query Workbench"});
            this.nccOptionsClientSkinComboBox.Location = new System.Drawing.Point(87, 28);
            this.nccOptionsClientSkinComboBox.Name = "nccOptionsClientSkinComboBox";
            this.nccOptionsClientSkinComboBox.Size = new System.Drawing.Size(150, 22);
            this.nccOptionsClientSkinComboBox.TabIndex = 18;
            this.nccOptionsToolTip.SetToolTip(this.nccOptionsClientSkinComboBox, "Change the client skin or view. Selecting the skin also governs what features the" +
        " client provides.\r\nOnly available for debugging right now.");
            this.nccOptionsClientSkinComboBox.SelectedIndexChanged += new System.EventHandler(this.nccOptionsClientSkinComboBox_SelectedIndexChanged);
            // 
            // nccOptionsClientSkinLabel
            // 
            this.nccOptionsClientSkinLabel.AutoSize = true;
            this.nccOptionsClientSkinLabel.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.nccOptionsClientSkinLabel.Location = new System.Drawing.Point(12, 30);
            this.nccOptionsClientSkinLabel.Name = "nccOptionsClientSkinLabel";
            this.nccOptionsClientSkinLabel.Size = new System.Drawing.Size(60, 16);
            this.nccOptionsClientSkinLabel.TabIndex = 17;
            this.nccOptionsClientSkinLabel.Text = "Client Skin";
            this.nccOptionsToolTip.SetToolTip(this.nccOptionsClientSkinLabel, "The client skin or view");
            // 
            // nccRepositorySchemaLabel
            // 
            this.nccRepositorySchemaLabel.AutoSize = true;
            this.nccRepositorySchemaLabel.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.nccRepositorySchemaLabel.Location = new System.Drawing.Point(12, 30);
            this.nccRepositorySchemaLabel.Name = "nccRepositorySchemaLabel";
            this.nccRepositorySchemaLabel.Size = new System.Drawing.Size(45, 16);
            this.nccRepositorySchemaLabel.TabIndex = 15;
            this.nccRepositorySchemaLabel.Text = "Schema";
            this.nccOptionsToolTip.SetToolTip(this.nccRepositorySchemaLabel, resources.GetString("nccRepositorySchemaLabel.ToolTip"));
            // 
            // nccRepositorySchemaComboBox
            // 
            this.nccRepositorySchemaComboBox.DropDownWidth = 100;
            this.nccRepositorySchemaComboBox.Font = new System.Drawing.Font("Arial", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.nccRepositorySchemaComboBox.FormattingEnabled = true;
            this.nccRepositorySchemaComboBox.Items.AddRange(new object[] {
            "System Repository",
            "TRAFODION_METRICS_ARCHIVE"});
            this.nccRepositorySchemaComboBox.Location = new System.Drawing.Point(70, 28);
            this.nccRepositorySchemaComboBox.Name = "nccRepositorySchemaComboBox";
            this.nccRepositorySchemaComboBox.Size = new System.Drawing.Size(160, 22);
            this.nccRepositorySchemaComboBox.TabIndex = 16;
            this.nccOptionsToolTip.SetToolTip(this.nccRepositorySchemaComboBox, resources.GetString("nccRepositorySchemaComboBox.ToolTip"));
            // 
            // generalSettingsGroupBox
            // 
            this.generalSettingsGroupBox.Controls.Add(this.nccOptionsClientSkinLabel);
            this.generalSettingsGroupBox.Controls.Add(this.nccOptionsClientSkinComboBox);
            this.generalSettingsGroupBox.Font = new System.Drawing.Font("Lucida Sans Unicode", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.generalSettingsGroupBox.Location = new System.Drawing.Point(15, 365);
            this.generalSettingsGroupBox.Name = "generalSettingsGroupBox";
            this.generalSettingsGroupBox.Size = new System.Drawing.Size(250, 66);
            this.generalSettingsGroupBox.TabIndex = 21;
            this.generalSettingsGroupBox.TabStop = false;
            this.generalSettingsGroupBox.Text = "General Settings";
            this.generalSettingsGroupBox.Visible = false;
            // 
            // repositorySettingsGroupBox
            // 
            this.repositorySettingsGroupBox.Controls.Add(this.nccRepositorySchemaLabel);
            this.repositorySettingsGroupBox.Controls.Add(this.nccRepositorySchemaComboBox);
            this.repositorySettingsGroupBox.Font = new System.Drawing.Font("Lucida Sans Unicode", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.repositorySettingsGroupBox.Location = new System.Drawing.Point(15, 290);
            this.repositorySettingsGroupBox.Name = "repositorySettingsGroupBox";
            this.repositorySettingsGroupBox.Size = new System.Drawing.Size(250, 70);
            this.repositorySettingsGroupBox.TabIndex = 22;
            this.repositorySettingsGroupBox.TabStop = false;
            this.repositorySettingsGroupBox.Text = "Repository Views Settings";
            // 
            // WorkloadOption
            // 
            this.AcceptButton = this.nccOptionsOKButton;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.CancelButton = this.nccOptionsCancelButton;
            this.ClientSize = new System.Drawing.Size(504, 443);
            this.Controls.Add(this.repositorySettingsGroupBox);
            this.Controls.Add(this.generalSettingsGroupBox);
            this.Controls.Add(this.triageSettingsGroupBox);
            this.Controls.Add(this.timeoutGroupBox);
            this.Controls.Add(this.timeLineSettingsGroupBox);
            this.Controls.Add(this.workbenchSettingsGroupBox);
            this.Controls.Add(this.nccOptionsOKButton);
            this.Controls.Add(this.nccOptionsCancelButton);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "WorkloadOption";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Trafodion Database Manager - Options";
            this.Load += new System.EventHandler(this.NCCOption_Load);
            this.timeoutGroupBox.ResumeLayout(false);
            this.timeoutGroupBox.PerformLayout();
            this.workbenchSettingsGroupBox.ResumeLayout(false);
            this.workbenchSettingsGroupBox.PerformLayout();
            this.timeLineSettingsGroupBox.ResumeLayout(false);
            this.timeLineSettingsGroupBox.PerformLayout();
            this.triageSettingsGroupBox.ResumeLayout(false);
            this.triageSettingsGroupBox.PerformLayout();
            this.generalSettingsGroupBox.ResumeLayout(false);
            this.generalSettingsGroupBox.PerformLayout();
            this.repositorySettingsGroupBox.ResumeLayout(false);
            this.repositorySettingsGroupBox.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        //private System.Windows.Forms.CheckedListBox checkedListBox1;
        private System.Windows.Forms.Button nccOptionsCancelButton;
		private System.Windows.Forms.Button nccOptionsOKButton;
        private System.Windows.Forms.GroupBox timeoutGroupBox;
        private System.Windows.Forms.Label connectionTimeoutLabel;
        private System.Windows.Forms.TextBox commandTimeoutTextBox;
        private System.Windows.Forms.Label commandTimeoutLabel;
        private System.Windows.Forms.TextBox connectionTimeoutTextBox;
        private System.Windows.Forms.GroupBox workbenchSettingsGroupBox;
        private System.Windows.Forms.TextBox maxRowsLimitTextBox;
		private System.Windows.Forms.Label maxRowsLimitLabel;
		private System.Windows.Forms.GroupBox timeLineSettingsGroupBox;
		private System.Windows.Forms.Label nccOptionsMetricGraphIntervalLabel;
		private System.Windows.Forms.CheckBox nccOptionsDisplayTimesInLocalTimezoneCheckBox;
		private System.Windows.Forms.Label nccOptionsTimelineIntervalLabel;
		private System.Windows.Forms.ComboBox nccOptionsMetricGraphIntervalComboBox;
		private System.Windows.Forms.ComboBox nccOptionsTimelineIntervalComboBox;
		private System.Windows.Forms.GroupBox triageSettingsGroupBox;
		private System.Windows.Forms.TextBox triageMaxRowsLimitTextBox;
		private System.Windows.Forms.Label triageMaxRowsLimitLabel;
		private System.Windows.Forms.CheckBox nccOptionsAllowQueryExecutionCheckBox;
		private System.Windows.Forms.CheckBox nccOptionsAutoVersionQueriesCheckBox;
		private System.Windows.Forms.CheckBox nccOptionsColorizeProcBoundariesCheckBox;
		private System.Windows.Forms.ToolTip nccOptionsToolTip;
		private System.Windows.Forms.CheckBox nccOptionsSortByOperatorLevelCheckBox;
		private System.Windows.Forms.CheckBox nccOptionsHideInternalQueries;
		private System.Windows.Forms.GroupBox generalSettingsGroupBox;
		private System.Windows.Forms.Label nccOptionsClientSkinLabel;
		private System.Windows.Forms.ComboBox nccOptionsClientSkinComboBox;
		private System.Windows.Forms.CheckBox nccOptionsHideTransactionBoundariesCheckBox;
		private System.Windows.Forms.CheckBox nccHideRecentlyCompletedQueries;
		private System.Windows.Forms.CheckBox nccOptionsDownloadTimeLineCheckBox;
        private System.Windows.Forms.TextBox maxWMSGraphPointsTextBox;
        private System.Windows.Forms.Label maxWMSGraphPointsLabel;
        private System.Windows.Forms.GroupBox repositorySettingsGroupBox;
        private System.Windows.Forms.Label nccRepositorySchemaLabel;
        private System.Windows.Forms.ComboBox nccRepositorySchemaComboBox;

    }
}