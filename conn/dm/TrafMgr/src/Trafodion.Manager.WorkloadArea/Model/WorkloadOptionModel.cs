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
using System.Globalization;

namespace Trafodion.Manager.WorkloadArea.Model
{
	[Serializable]
	public class WorkloadOptionModel
	{
		#region Constants
		public const int MIN_TIMELINE_INTERVAL_SECS = 6 * 60 * 60;  // 6 hours
		public const int MIN_METRIC_INTERVAL_SECS = 30 * 60;  // 30 minutes	
        public const int MIN_WMS_GRAPH_POINTS = 60; // 30 minutes at 30 second refresh rates.

        public const int DEFAULT_WMS_GRAPH_POINTS = 120;
        public const String CURRENT_OPTION_MODEL_VERSION = "1.7";

		#endregion


		#region Members
		private int m_connectionTimeout = 30;   // ODBC default 15 seconds 
		private int m_commandTimeout = 180;     // ODBC default 30 seconds
		private int m_fetchLimit = 5000;        // Max rows to fetch in Workbench. 
		private bool _allowQueryExecution = true;  // Allow Query Execution in Workbench.
		private bool _autoVersionQueries = true;  // Automatically version queries if text is changed.
		private bool _colorizeProcessBoundaries = true; //Turn on process boundary coloring
		private bool _sortByOperatorLevel = true; // Turn on Query Plan IGrid SortByLevel.

		private String _selectedTimeLineInterval = "1 day";
		private int _timeLineInterval = 24 * 60 * 60;  // 1 day in seconds

		private int _triageFetchLimit = 20000;  // Triage Space fetch limit. 

		private String _selectedMetricInterval = "1 hour";
		private int _metricInterval = 60 * 60;  // 1 hour in seconds
		private bool _showTimesLocally = true;

        //private NCCClientSkin _clientSkin = new NCCClientSkin();
		private bool _hideTransactionBoundaries = false;
		private bool _hideRecentlyCompletedQueries = false;
		private bool _hideInternalQueries = true;
		private bool _downloadTimeLineDetails = false;

        private int _maxWMSGraphPoints = DEFAULT_WMS_GRAPH_POINTS;
        private String _repositorySchema = Trafodion.Manager.WorkloadArea.Controls.WorkloadOption.SYSTEM_REPOSITORY_NAME;
        private String _optionsVersion = CURRENT_OPTION_MODEL_VERSION;

		#endregion

		#region Properties
		public int ConnectionTimeout
		{
			get { return this.m_connectionTimeout; }
			set { this.m_connectionTimeout = value; }
		}

		public int CommandTimeout
		{
			get { return this.m_commandTimeout; }
			set { this.m_commandTimeout = value; }
		}

		public int TriageFetchLimit {
			get { return this._triageFetchLimit; }
			set { this._triageFetchLimit = value; }
		}

		public int FetchLimit
		{
			get { return this.m_fetchLimit; }
			set { this.m_fetchLimit = value; }
		}

		public bool AllowQueryExecution {
			get { return this._allowQueryExecution; }
			set { this._allowQueryExecution = value; }
		}

		public bool ColorizeProcessBoundaries{
			get { return this._colorizeProcessBoundaries; }
			set { this._colorizeProcessBoundaries = value; }
		}

		public bool SortByOperatorLevel {
			get { return this._sortByOperatorLevel; }
			set { this._sortByOperatorLevel = value; }
		}

		public bool AutoVersionQueries {
			get { return this._autoVersionQueries; }
			set { this._autoVersionQueries = value; }
		}


		public String TimeLineIntervalSelection
		{
			get { return this._selectedTimeLineInterval; }
			set { this._selectedTimeLineInterval = value; }
		}

		public int TimeLineIntervalSeconds
		{
			get { return this._timeLineInterval; }
			set { this._timeLineInterval = value; }
		}

		public String MetricGraphIntervalSelection
		{
			get { return this._selectedMetricInterval; }
			set { this._selectedMetricInterval = value; }
		}

		public int MetricGraphIntervalSeconds
		{
			get { return this._metricInterval; }
			set { this._metricInterval = value; }
		}


		public bool DownloadTimeLineInfo {
			get { return this._downloadTimeLineDetails; }
			set { this._downloadTimeLineDetails = value; }
		}


		public bool ShowTimesLocally
		{
			get { return this._showTimesLocally; }
			set { this._showTimesLocally = value; }
		}

        //public SKIN ClientSkin {
        //    get { return this._clientSkin.Skin; }
        //    set { this._clientSkin.Skin = value; }
        //}

		public bool HideTransactionBoundaries {
			get { return this._hideTransactionBoundaries; }
			set { this._hideTransactionBoundaries = value; }
		}

		public bool HideRecentlyCompletedQueries {
			get { return this._hideRecentlyCompletedQueries; }
			set { this._hideRecentlyCompletedQueries = value; }
		}

		public bool HideInternalQueries {
			get { return this._hideInternalQueries; }
			set { this._hideInternalQueries = value; }
		}

        public int MaxWMSGraphPoints {
            get { return this._maxWMSGraphPoints; }
            set { this._maxWMSGraphPoints = value; }
        }

        public String RepositorySchema {
            get { return this._repositorySchema; }
            set {
                if (null != value)
                    this._repositorySchema = value.Trim();
                else
                    this._repositorySchema = value;

            }
        }

		#endregion

		#region Constructors
        public WorkloadOptionModel(Trafodion.Manager.WorkloadArea.Controls.WorkloadOption options)
        {
            if (options != null)
            {
                //this._clientSkin.Skin = options.ClientSkin;

                this.m_commandTimeout = options.CommandTimeout;
                this.m_connectionTimeout = options.ConnectionTimeout;

                this._triageFetchLimit = options.TriageFetchLimit;
                this._hideTransactionBoundaries = options.HideTransactionBoundaries;
                this._hideRecentlyCompletedQueries = options.HideRecentlyCompletedQueries;
                this._hideInternalQueries = options.HideInternalQueries;

                this._repositorySchema = options.RepositorySchema;

                this.m_fetchLimit = options.FetchLimit;
                this._maxWMSGraphPoints = options.MaxWMSGraphPoints;
                this._allowQueryExecution = options.AllowQueryExecution;
                this._autoVersionQueries = options.AutoVersionQueries;

                this._selectedTimeLineInterval = options.TimeLineIntervalSelection;
                this._timeLineInterval = options.TimeLineIntervalSeconds;
                this._selectedMetricInterval = options.MetricGraphIntervalSelection;
                this._metricInterval = options.MetricGraphIntervalSeconds;
                this._downloadTimeLineDetails = options.DownloadTimeLineInfo;
                this._showTimesLocally = options.ShowTimesLocally;
            }
        }
		#endregion

        //public void doVersionChecks(SKIN theSkin) {
        //    int clzVersion = getClassVersion();

        //    if (null == this._clientSkin)
        //        this._clientSkin = new NCCClientSkin(theSkin);

        //    this._clientSkin.Skin = theSkin;

        //    switch (clzVersion) {
        //        case 0: 
        //            //  Old pre 1.0 version - set auto versioning and allow query executions to true.
        //            this._allowQueryExecution = true;
        //            this._autoVersionQueries = true;
        //            this._colorizeProcessBoundaries = true;
        //            this._sortByOperatorLevel = true;
        //            this._hideTransactionBoundaries = false;
        //            this._hideRecentlyCompletedQueries = false;
        //            this._hideInternalQueries = true;
        //            this._downloadTimeLineDetails = false;
        //            this._maxWMSGraphPoints = DEFAULT_WMS_GRAPH_POINTS;
        //            this._repositorySchema = NCCOption.SYSTEM_REPOSITORY_NAME;
        //            break;


        //        case 10:
        //            this._colorizeProcessBoundaries = true;
        //            this._sortByOperatorLevel = true;
        //            this._hideTransactionBoundaries = false;
        //            this._hideRecentlyCompletedQueries = false;
        //            this._hideInternalQueries = true;
        //            this._downloadTimeLineDetails = false;
        //            this._maxWMSGraphPoints = DEFAULT_WMS_GRAPH_POINTS;
        //            this._repositorySchema = NCCOption.SYSTEM_REPOSITORY_NAME;
        //            break;

        //        case 11:
        //            this._sortByOperatorLevel = true;
        //            this._hideTransactionBoundaries = false;
        //            this._hideRecentlyCompletedQueries = false;
        //            this._hideInternalQueries = true;
        //            this._downloadTimeLineDetails = false;
        //            this._maxWMSGraphPoints = DEFAULT_WMS_GRAPH_POINTS;
        //            this._repositorySchema = NCCOption.SYSTEM_REPOSITORY_NAME;
        //            break;

        //        case 12:
        //            this._hideTransactionBoundaries = false;
        //            this._hideRecentlyCompletedQueries = false;
        //            this._hideInternalQueries = true;
        //            this._downloadTimeLineDetails = false;
        //            this._maxWMSGraphPoints = DEFAULT_WMS_GRAPH_POINTS;
        //            this._repositorySchema = NCCOption.SYSTEM_REPOSITORY_NAME;
        //            break;

        //        case 13:
        //            this._hideInternalQueries = true;
        //            this._downloadTimeLineDetails = false;
        //            this._maxWMSGraphPoints = DEFAULT_WMS_GRAPH_POINTS;
        //            this._repositorySchema = NCCOption.SYSTEM_REPOSITORY_NAME;
        //            break;

        //        case 14:
        //            this._maxWMSGraphPoints = DEFAULT_WMS_GRAPH_POINTS;
        //            this._downloadTimeLineDetails = false;
        //            this._repositorySchema = NCCOption.SYSTEM_REPOSITORY_NAME;
        //            break;

        //        case 15:
        //            this._repositorySchema = NCCOption.SYSTEM_REPOSITORY_NAME;
        //            this._downloadTimeLineDetails = false;
        //            break;

        //        case 16:
        //            this._repositorySchema = NCCOption.SYSTEM_REPOSITORY_NAME;
        //            this._downloadTimeLineDetails = false;
        //            break;

        //        default:
        //            break;
        //    }

        //    //  All done w/ checks, now set the option model version to the current one.
        //    this._optionsVersion = CURRENT_OPTION_MODEL_VERSION;
        //}


        //public String getSkinName() {
        //    return this._clientSkin.getName();
        //}

        //public bool isNPASkin() {
        //    return this._clientSkin.isNPASkin;
        //}

        //public bool isQueryListViewSkin() {
        //    return this._clientSkin.isQueryListViewSkin;
        //}

        //public bool isQueryWorkbenchSkin() {
        //    return this._clientSkin.isQueryWorkbenchSkin;
        //}

		private int getClassVersion() {
			int clzVersion = 0;

			try {
				NumberFormatInfo nFormatInfo = Trafodion.Manager.WorkloadArea.Controls.TriageHelper.getNumberFormatInfoForParsing();
				if (null != this._optionsVersion)
					clzVersion = (int)(double.Parse(this._optionsVersion, nFormatInfo) * 10);

			} catch (Exception) {
				clzVersion = 0;
			}

			return clzVersion;
		}

	}
}
