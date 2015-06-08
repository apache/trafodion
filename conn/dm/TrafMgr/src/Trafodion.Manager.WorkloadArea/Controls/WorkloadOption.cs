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
using System.Collections;
using System.Text.RegularExpressions;
using Trafodion.Manager.WorkloadArea.Model;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.WorkloadArea.Controls
{
	public partial class WorkloadOption : TrafodionForm
	{
        public const String SYSTEM_REPOSITORY_NAME = "System Repository";
        
		private WorkloadOptionModel m_optionModel;

		public WorkloadOptionModel OptionModel
		{
			get { return m_optionModel; }
			set { m_optionModel = value; }
		}

		public int ConnectionTimeout
		{
			get { return m_optionModel.ConnectionTimeout; }
			set { m_optionModel.ConnectionTimeout = value; }
		}

		public int CommandTimeout
		{
			get { return m_optionModel.CommandTimeout; }
			set { m_optionModel.CommandTimeout = value; }
		}

		public int TriageFetchLimit {
			get { return m_optionModel.TriageFetchLimit; }
			set { m_optionModel.TriageFetchLimit = value; }
		}

		public int FetchLimit
		{
			get { return m_optionModel.FetchLimit; }
			set { m_optionModel.FetchLimit = value; }
		}

		public bool AllowQueryExecution {
			get { return m_optionModel.AllowQueryExecution; }
			set { m_optionModel.AllowQueryExecution = value; }
		}

		public bool ColorizeProcessBoundaries
		{
			get { return this.m_optionModel.ColorizeProcessBoundaries; }
			set { this.m_optionModel.ColorizeProcessBoundaries = value; }
		}

		public bool SortByOperatorLevel {
			get { return this.m_optionModel.SortByOperatorLevel; }
			set { this.m_optionModel.SortByOperatorLevel = value; }
		}


		public bool AutoVersionQueries {
			get { return m_optionModel.AutoVersionQueries; }
			set { m_optionModel.AutoVersionQueries = value; }
		}

		public String TimeLineIntervalSelection
		{
			get { return m_optionModel.TimeLineIntervalSelection; }
			set { m_optionModel.TimeLineIntervalSelection = value; }
		}

		public int TimeLineIntervalSeconds
		{
			get { return m_optionModel.TimeLineIntervalSeconds; }
			set { m_optionModel.TimeLineIntervalSeconds = value; }
		}

		public String MetricGraphIntervalSelection
		{
			get { return m_optionModel.MetricGraphIntervalSelection; }
			set { m_optionModel.MetricGraphIntervalSelection = value; }
		}

		public int MetricGraphIntervalSeconds
		{
			get { return m_optionModel.MetricGraphIntervalSeconds; }
			set { m_optionModel.MetricGraphIntervalSeconds = value; }
		}

		public bool DownloadTimeLineInfo {
			get { return m_optionModel.DownloadTimeLineInfo; }
			set { m_optionModel.DownloadTimeLineInfo = value; }
		}


		public bool ShowTimesLocally
		{
			get { return m_optionModel.ShowTimesLocally; }
			set { m_optionModel.ShowTimesLocally = value; }
		}

        //public SKIN ClientSkin {
        //    get { return m_optionModel.ClientSkin; }
        //    set { m_optionModel.ClientSkin = value; }
        //}

        //public bool NPASkin{
        //    get { return m_optionModel.isNPASkin(); }
        //    set { if (value)  m_optionModel.ClientSkin = SKIN.TrafodionPerformanceAnalyzer; }
        //}

        //public bool QueryListViewSkin {
        //    get { return m_optionModel.isQueryListViewSkin(); }
        //    set {if (value)  m_optionModel.ClientSkin = SKIN.QueryListView; }
        //}

        //public bool QueryWorkbenchSkin {
        //    get { return m_optionModel.isQueryWorkbenchSkin(); }
        //    set { if (value)  m_optionModel.ClientSkin = SKIN.QueryWorkbench; }
        //}


		public bool HideTransactionBoundaries {
			get { return m_optionModel.HideTransactionBoundaries; }
			set { m_optionModel.HideTransactionBoundaries = value; }
		}

		public bool HideRecentlyCompletedQueries  {
			get { return m_optionModel.HideRecentlyCompletedQueries; }
			set { m_optionModel.HideRecentlyCompletedQueries = value; }
		}

		public bool HideInternalQueries {
			get { return m_optionModel.HideInternalQueries; }
			set { m_optionModel.HideInternalQueries = value; }
		}

        public int MaxWMSGraphPoints {
            get { return m_optionModel.MaxWMSGraphPoints; }
            set { m_optionModel.MaxWMSGraphPoints = value; }
        }

        public String RepositorySchema {
            get { return m_optionModel.RepositorySchema; }
            set { m_optionModel.RepositorySchema = value; }
        }


		public WorkloadOption()
		{
			InitializeComponent();
			this.m_optionModel = new WorkloadOptionModel(null);
			setupComponent();
		}


		public WorkloadOption(WorkloadOption options)
		{
			InitializeComponent();
			this.m_optionModel = new WorkloadOptionModel(options);
			setupComponent();
		}


		private void setupComponent() {
			setupOptionSelections();
		}




		private void setupOptionSelections() {
#if DEBUG
			this.generalSettingsGroupBox.Visible = true;
#endif

			this.nccOptionsDownloadTimeLineCheckBox.Checked = m_optionModel.DownloadTimeLineInfo;
			this.nccOptionsDisplayTimesInLocalTimezoneCheckBox.Checked = m_optionModel.ShowTimesLocally;

			//this.nccOptionsClientSkinComboBox.Text = m_optionModel.getSkinName();

			this.nccOptionsHideTransactionBoundariesCheckBox.Checked = m_optionModel.HideTransactionBoundaries;

			this.nccHideRecentlyCompletedQueries.Checked          = m_optionModel.HideRecentlyCompletedQueries;
			this.nccOptionsHideInternalQueries.Checked            = m_optionModel.HideInternalQueries;

			this.nccOptionsAllowQueryExecutionCheckBox.Checked    = m_optionModel.AllowQueryExecution;
			this.nccOptionsAutoVersionQueriesCheckBox.Checked     = m_optionModel.AutoVersionQueries;
			this.nccOptionsColorizeProcBoundariesCheckBox.Checked = m_optionModel.ColorizeProcessBoundaries;
			this.nccOptionsSortByOperatorLevelCheckBox.Checked    = m_optionModel.SortByOperatorLevel;

			int idx = -1;
			try
			{
				idx = this.nccOptionsTimelineIntervalComboBox.FindString(m_optionModel.TimeLineIntervalSelection);
			}
			catch (Exception)
			{
				idx = -1;
			}

			if (-1 == idx)
				idx = this.nccOptionsTimelineIntervalComboBox.Items.Add(m_optionModel.TimeLineIntervalSelection);

			this.nccOptionsTimelineIntervalComboBox.SelectedIndex = Math.Max(0, idx);


			idx = -1;
			try
			{
				idx = this.nccOptionsMetricGraphIntervalComboBox.FindString(m_optionModel.MetricGraphIntervalSelection);
			}
			catch (Exception)
			{
				idx = -1;
			}

			if (-1 == idx)
				idx = this.nccOptionsMetricGraphIntervalComboBox.Items.Add(m_optionModel.MetricGraphIntervalSelection);

			this.nccOptionsMetricGraphIntervalComboBox.SelectedIndex = Math.Max(0, idx);


			this.connectionTimeoutTextBox.Text = Convert.ToInt32(m_optionModel.ConnectionTimeout).ToString();
			this.commandTimeoutTextBox.Text = Convert.ToInt32(m_optionModel.CommandTimeout).ToString();
			this.triageMaxRowsLimitTextBox.Text = Convert.ToInt32(m_optionModel.TriageFetchLimit).ToString();
			this.maxRowsLimitTextBox.Text = Convert.ToInt32(m_optionModel.FetchLimit).ToString();
            this.maxWMSGraphPointsTextBox.Text = Convert.ToInt32(m_optionModel.MaxWMSGraphPoints).ToString();

            idx = -1;
            try {
                idx = this.nccRepositorySchemaComboBox.FindStringExact(m_optionModel.RepositorySchema);
            } catch (Exception) {
                idx = -1;
            }

            if (-1 == idx)
                idx = this.nccRepositorySchemaComboBox.Items.Add(m_optionModel.RepositorySchema);

            this.nccRepositorySchemaComboBox.SelectedIndex = Math.Max(0, idx);
		}


		public DialogResult showOptionsDialog() {
			setupOptionSelections();
			return base.ShowDialog();
		}


		public void selectDisplayTimesInLocalTimeZoneOption(bool flag) {
			this.nccOptionsDisplayTimesInLocalTimezoneCheckBox.Checked = flag;
			m_optionModel.ShowTimesLocally = flag;
		}


		// Cancel pushbutton
		private void nccOptionsCancelButton_Click(object sender, EventArgs e) {
			this.DialogResult = DialogResult.Cancel;
			this.Hide();
		}

		// OK pushbutton     
		private void nccOptionsOKButton_Click(object sender, EventArgs e) {
			try
			{
				int connTimeout = Int32.Parse(this.connectionTimeoutTextBox.Text.Trim());
				int cmdTimeout = Int32.Parse(this.commandTimeoutTextBox.Text.Trim());
				int triageFetchLimit = Int32.Parse(this.triageMaxRowsLimitTextBox.Text.Trim());
				int fetchLimit = Int32.Parse(this.maxRowsLimitTextBox.Text.Trim());

				if (!validateTimelineInterval(this.nccOptionsTimelineIntervalComboBox.Text))
					return;

				if (!validateMetricGraphInterval(this.nccOptionsMetricGraphIntervalComboBox.Text))
					return;

                if (!validateWMSGraphPoints(this.maxWMSGraphPointsTextBox.Text))
                    return;


				// Only copy once all options have been validated.
				m_optionModel.ConnectionTimeout = connTimeout;
				m_optionModel.CommandTimeout = cmdTimeout;
				m_optionModel.TriageFetchLimit = triageFetchLimit;
				m_optionModel.FetchLimit = fetchLimit;
				m_optionModel.DownloadTimeLineInfo = this.nccOptionsDownloadTimeLineCheckBox.Checked;
				m_optionModel.ShowTimesLocally = this.nccOptionsDisplayTimesInLocalTimezoneCheckBox.Checked;
				m_optionModel.AllowQueryExecution = this.nccOptionsAllowQueryExecutionCheckBox.Checked;
				m_optionModel.AutoVersionQueries = this.nccOptionsAutoVersionQueriesCheckBox.Checked;
				m_optionModel.ColorizeProcessBoundaries = this.nccOptionsColorizeProcBoundariesCheckBox.Checked;
				m_optionModel.SortByOperatorLevel = this.nccOptionsSortByOperatorLevelCheckBox.Checked;

				//m_optionModel.ClientSkin = NCCClientSkin.mapName(this.nccOptionsClientSkinComboBox.Text);
				m_optionModel.HideTransactionBoundaries = this.nccOptionsHideTransactionBoundariesCheckBox.Checked;
				m_optionModel.HideRecentlyCompletedQueries = this.nccHideRecentlyCompletedQueries.Checked;
				m_optionModel.HideInternalQueries = this.nccOptionsHideInternalQueries.Checked;
                m_optionModel.RepositorySchema = this.nccRepositorySchemaComboBox.Text;

				this.DialogResult = DialogResult.OK;
				this.Hide();
			}
			catch (Exception ex)
			{
			    MessageBox.Show("\nError: Options input validation error.\n\n" +
							   "Problem: \t The values specified for one or more of the options are invalid. \n\n" +
							   "Solution: \t Please specify a valid numeric value.\n\n" +
							   "Details: \t " + ex.Message + "\n\n",
							   "Invalid Option Values",
							   MessageBoxButtons.OK, MessageBoxIcon.Error);

			}

		}

		private void NCCOption_Load(object sender, EventArgs e) {
		}



		private int getConversionFactor(String key) {
			if (null == key)
				return -1;

			Hashtable factorHT = new Hashtable();
			factorHT.Add("SECOND", 1);
			factorHT.Add("SECONDS", 1);
			factorHT.Add("MINUTE", 60);
			factorHT.Add("MINUTES", 60);
			factorHT.Add("HOUR", 60 * 60);
			factorHT.Add("HOURS", 60 * 60);
			factorHT.Add("DAY", 24 * 60 * 60);
			factorHT.Add("DAYS", 24 * 60 * 60);
			factorHT.Add("WEEK", 7 * 24 * 60 * 60);
			factorHT.Add("WEEKS", 7 * 24 * 60 * 60);

			key = key.Trim().ToUpper();
			if (factorHT.ContainsKey(key) ) {
				return (int) factorHT[key];
			}

			return -1;
		}


		private bool isValidTimeInterval(String timeIntervalStr, out int nSecs) {
			char[] wordDelimiters = {' ', '\t' };
			String  valueRegExpPattern = @"(SECOND|SECONDS|MINUTE|MINUTES|HOUR|HOURS|DAY|DAYS|" +
									"WEEK|WEEKS)";

			nSecs = -1;
			if ((null != timeIntervalStr) && (0 < timeIntervalStr.Trim().Length)) {
				String[] strArray = timeIntervalStr.Split(wordDelimiters, StringSplitOptions.RemoveEmptyEntries);
				if ((0 >= strArray.Length) || (2 < strArray.Length))
					return false;

				int value = -1;
				try {
					value = short.Parse(strArray[0]);
				} catch (Exception) {
					return false;
				}

				if (0 >= value)
					return false;

				if (2 > strArray.Length) {
					nSecs = value;
					return true;
				}

				int multFactor = 1;
				MatchCollection matches = Regex.Matches(strArray[1], valueRegExpPattern, RegexOptions.IgnoreCase);
				if (0 < matches.Count) {
					// Just take the first one.
					multFactor = getConversionFactor(matches[0].Value);
					if (0 < multFactor) {
						nSecs = multFactor * value;
						return true;
					}
				}

				return false;
			}

			return true;

		}


		private bool validateTimelineInterval(String val) {
			int timeVal;
			if (isValidTimeInterval(val, out timeVal))
			{
				if (timeVal >= WorkloadOptionModel.MIN_TIMELINE_INTERVAL_SECS)
				{
					m_optionModel.TimeLineIntervalSelection = val;
					m_optionModel.TimeLineIntervalSeconds = timeVal;
					return true;
				}
			}
			MessageBox.Show("\nError: Invalid Timeline Interval value [" + val + "]. \n\n" +
							"Problem: \t The time interval value specified is invalid or is < 6 hours. \n\n" +
							"Solution: \t Please specify a valid time interval [>= 6 hours] of the form: \n\n" +
							"          \t\t  <timeValue> [ second[s] | minute[s] | hour[s] | week[s] ] \n\n",
							"Invalid Timeline Interval Value", MessageBoxButtons.OK, MessageBoxIcon.Error);

			return false;
		}



		private bool validateMetricGraphInterval(String val) {
			int timeVal;
			if (isValidTimeInterval(val, out timeVal)) {
				if (timeVal >= WorkloadOptionModel.MIN_METRIC_INTERVAL_SECS)
				{
					m_optionModel.MetricGraphIntervalSelection = val;
					m_optionModel.MetricGraphIntervalSeconds = timeVal;
					return true;
				}
			}


			MessageBox.Show("\nError: Invalid Graph Interval value [" + val + "]. \n" +
							"Problem: \t The time interval value specified is invalid or is < 30 minutes. \n\n" +
							"Solution: \t Please specify a valid time interval [>= 30 minutes] of the form: \n\n" +
							"          \t\t  <timeValue> [ second[s] | minute[s] | hour[s] | week[s] ] \n\n",
							"Invalid Graph Interval Value", MessageBoxButtons.OK, MessageBoxIcon.Error);

			return false;
		}


        private bool validateWMSGraphPoints(String val) {
            int  wmsGraphPoints = 0;

            try {
                wmsGraphPoints = Int32.Parse(val.Trim() );

            } catch (Exception) {
            }

            if (wmsGraphPoints >= WorkloadOptionModel.MIN_WMS_GRAPH_POINTS) {
                m_optionModel.MaxWMSGraphPoints = wmsGraphPoints;
                return true;
            }


            MessageBox.Show("\nError: Invalid Live Range (number of graph points) value [" + val + "]. \n" +
                            "Problem: \t The live  range value specified is invalid or is < 60. \n\n" +
                            "Solution: \t Please specify a valid range [>= 60]. \n\n",
                            "Invalid Live Range Value", MessageBoxButtons.OK, MessageBoxIcon.Error);

            return false;
        }


        //private void showHideSettingsForSkin(SKIN theSkin) {
        //    int timeOptionsHeight = 65;
        //    int triageOptionsSizeChange = 25;
        //    bool showTriageSettings = true;
        //    bool showWorkbenchSettings = true;
			
        //    if (SKIN.QueryListView == theSkin)
        //        showWorkbenchSettings = false;
        //    else if (SKIN.QueryWorkbench == theSkin)
        //        showTriageSettings = false;


        //    triageSettingsGroupBox.Text = "Query Viewer Settings";
        //    triageMaxRowsLimitLabel.Visible = true;
        //    triageMaxRowsLimitTextBox.Visible = true;

        //    triageSettingsGroupBox.Visible = showTriageSettings;
        //    workbenchSettingsGroupBox.Visible = showWorkbenchSettings;
        //    repositorySettingsGroupBox.Visible = true;

        //    if (SKIN.TrafodionPerformanceAnalyzer != theSkin) {
        //        nccOptionsDownloadTimeLineCheckBox.Visible = false;
        //        nccOptionsTimelineIntervalLabel.Visible = false;
        //        nccOptionsTimelineIntervalComboBox.Visible = false;
        //        nccOptionsMetricGraphIntervalLabel.Visible = false;
        //        nccOptionsMetricGraphIntervalComboBox.Visible = false;

        //        timeLineSettingsGroupBox.Height = timeOptionsHeight;
        //        int yPos = timeOptionsHeight - 10 - nccOptionsDisplayTimesInLocalTimezoneCheckBox.Height;
        //        nccOptionsDisplayTimesInLocalTimezoneCheckBox.Location = new Point(34, Math.Max(28, yPos) );

        //        repositorySettingsGroupBox.Visible = false;

        //        this.Size = new Size(this.Width, 400);

        //        if (SKIN.QueryListView == theSkin) {
        //            triageSettingsGroupBox.Text = "Query Viewer Settings";
        //            triageMaxRowsLimitLabel.Visible = false;
        //            triageMaxRowsLimitTextBox.Visible = false;

        //            triageSettingsGroupBox.Size = new Size(triageSettingsGroupBox.Width,
        //                                                   triageSettingsGroupBox.Height - triageOptionsSizeChange);
        //            moveControlUpDown(triageSettingsGroupBox, triageOptionsSizeChange);
        //            moveControlUpDown(nccOptionsHideInternalQueries, 0 - triageOptionsSizeChange);
        //            moveControlUpDown(nccOptionsHideTransactionBoundariesCheckBox, 0 - triageOptionsSizeChange);
        //            moveControlUpDown(nccHideRecentlyCompletedQueries, 0 - triageOptionsSizeChange);

        //            moveControlUpDown(maxWMSGraphPointsLabel, 0 - triageOptionsSizeChange);
        //            moveControlUpDown(maxWMSGraphPointsTextBox, 0 - triageOptionsSizeChange);

        //        } else if (SKIN.QueryWorkbench == theSkin) {
        //            workbenchSettingsGroupBox.Location = new Point(workbenchSettingsGroupBox.Location.X,
        //                                                           timeoutGroupBox.Location.Y);
        //            workbenchSettingsGroupBox.Height = triageSettingsGroupBox.Height;
        //        }


        //    }

        //}


        private void moveControlUpDown(Control c, int yOffset) {
            int xPos = c.Location.X;
            int yPos = c.Location.Y + yOffset;
            yPos = Math.Max(0, yPos);

            c.Location = new Point(xPos, yPos);
        }


		private void nccOptionsClientSkinComboBox_SelectedIndexChanged(object sender, EventArgs e) {
			try {
                //SKIN skin = NCCClientSkin.mapName(nccOptionsClientSkinComboBox.Text);
                //showHideSettingsForSkin(skin);

			} catch(Exception) {
			}
		}

		private void nccOptionsEnableTimeLineCheckBox_CheckedChanged(object sender, EventArgs e) {
			bool enableTimeSettings = nccOptionsDownloadTimeLineCheckBox.Checked;
			nccOptionsTimelineIntervalLabel.Enabled = enableTimeSettings;
			nccOptionsTimelineIntervalComboBox.Enabled = enableTimeSettings;
			nccOptionsMetricGraphIntervalLabel.Enabled = enableTimeSettings;
			nccOptionsMetricGraphIntervalComboBox.Enabled = enableTimeSettings;
		}

	}
}
