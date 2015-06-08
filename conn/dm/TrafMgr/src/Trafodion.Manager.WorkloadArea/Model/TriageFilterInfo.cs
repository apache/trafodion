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
using System.ComponentModel;
using System.Windows.Forms;
using System.Drawing;
using System.Drawing.Design;

using System.Xml;
using System.Xml.Serialization;
using Trafodion.Manager.WorkloadArea.Controls;

namespace Trafodion.Manager.WorkloadArea.Model
{
	#region Enums

	[CategoryAttribute("Enums"), TypeConverter(typeof(WorkloadEnumConverter)) ]
	public enum States { Any, Completed, Error, Running, 
						 [Description("Abnormally Terminated")] Killed_Or_Beyond12HourLimit };

	public enum Errors { Any, Yes, No };
	public enum Types	{ Any, Select, Insert, Delete, Update, Call, DDL, Management, Control, Other };


	[CategoryAttribute("Enums"), TypeConverter(typeof(WorkloadEnumConverter))]
	public enum FilterTimePeriod {
		[DescriptionAttribute("Any")] Any,
        [DescriptionAttribute("Last 15 Minutes")] Last_15_Minutes,
        [DescriptionAttribute("Last 30 Minutes")] Last_30_Minutes,
		[DescriptionAttribute("Last 1 Hour")] Last_Hour,
		[DescriptionAttribute("Last 2 Hours")] Last_2_Hours,
		[DescriptionAttribute("Last 4 Hours")] Last_4_Hours,
		[DescriptionAttribute("Last 8 Hours")] Last_8_Hours,
		[DescriptionAttribute("Last 12 Hours")] Last_12_Hours,
		[DescriptionAttribute("Today")] Today,
		[DescriptionAttribute("Yesterday")] Yesterday,
		[DescriptionAttribute("Last 1 Day")] Last_1_Day,
		[DescriptionAttribute("Last 2 Days")] Last_2_Days,
		[DescriptionAttribute("Last 5 Days")] Last_5_Days,
		[DescriptionAttribute("Last 7 Days")] Last_Week,
		[DescriptionAttribute("Custom Date and Time")] Custom
	};

	#endregion Enums

	[XmlRoot(ElementName="NCCFilterInfo", Namespace="", IsNullable=true)]
	public class TriageFilterInfo
	{
		#region Members
		private string[] m_applicationID = null;
		private string[] m_clientID = null;
		private string[] m_userName = null;
		private Types m_sqlType = Types.Any;
		private Errors m_error = Errors.Any;
		private States m_state = States.Any;
		private DateTime m_startTime = new DateTime();
		private DateTime m_endTime = new DateTime();
		private TimeSpan m_elapsedTime = new TimeSpan();
		private TriageHelper _theTriageHelper = null;
		private String _queryTextLike = null;
		private FilterTimePeriod _timePeriod = FilterTimePeriod.Any;
		private String _trafodionSQLPredicate = null;
		private int _rowsAccessed = 0;
		private int _rowsRetrieved = 0;
		private int _rowsIUD = 0;
		private double _estimatedCost = 0.0;
		private String _queryID = null;
		private String _sessionID = null;
        //private bool _enablePrompts = false;

		#endregion Members

		#region Properties
        //public bool EnablePrompts
        //{
        //    get { return _enablePrompts; }
        //    set { _enablePrompts = value; }
        //}

		[CategoryAttribute("Facts"), MergableProperty(true),
		 XmlElement(Namespace = "", ElementName = "SQLStatementTypes"),
	     DescriptionAttribute("Indicates the SQL statement type [Client-side Filter].")]
		public Types Type
		{
			get { return m_sqlType; }
			set { m_sqlType = value; }
		}

		[CategoryAttribute("Facts"),
		 XmlElement(Namespace = "", ElementName = "Errors"),
	     DescriptionAttribute("Indicates whether or not to filter statements with errors [Client-Side Filter].")]
		public Errors Error
		{
			get { return m_error; }
			set { m_error = value; }
		}

		[CategoryAttribute("Facts"), XmlElement(Namespace="", ElementName="QueryTextLike"),
		 DescriptionAttribute("Filter on SQL Query Text. \n" +
						      "Wildcards supported are % or * and  _ or ?. \n" +
							  "E.g. SELECT%COUNT implies contains 'SELECT' and 'COUNT'.")]
		public String Text {
			get { return _queryTextLike; }
			set { _queryTextLike = value; }
		}


        [Editor(typeof(System.ComponentModel.Design.MultilineStringEditor), typeof(UITypeEditor))]
        [CategoryAttribute("Facts"), XmlElement(Namespace = "", ElementName = "CustomTrafodionFilter"),
         DescriptionAttribute("Custom Trafodion Qualifier [Trafodion Platform Filter].")]
		public String Custom_Filter {
			get { return _trafodionSQLPredicate; }
			set { _trafodionSQLPredicate = value; }
		}

		[CategoryAttribute("Facts"),
		 XmlElement(DataType="int", Namespace = "", ElementName = "RowsAccessed"),
		 DescriptionAttribute("Minimum number of rows accessed.\n" +
						      "Filter on rows accessed >= the specified value.")]
		public int Rows_Accessed {
			get { return _rowsAccessed; }
			set { _rowsAccessed = value; }
		}

		[CategoryAttribute("Facts"),
		 XmlElement(DataType="int", Namespace = "", ElementName = "RowsRetrieved"),
		 DescriptionAttribute("Minimum number of rows retrieved. \n" +
                              "Filter on row count >= the specified value.")]
		public int Row_Count {
			get { return _rowsRetrieved; }
			set 
            {
                try 
                {
                    int theNewValue= value;
                    _rowsRetrieved=theNewValue;
                }
                catch(Exception ex)
                {
                    _rowsRetrieved = 0;
                }                
            }
		}

		[CategoryAttribute("Facts"),
		 XmlElement(DataType="int", Namespace = "", ElementName = "RowsInsertedUpdatedDeleted"),
		 DescriptionAttribute("Filter on number of insert, updates and deletes. \n" +
							  "Filter on >= the specified value.")]
		public int IUD_Count {
			get { return _rowsIUD; }
			set { _rowsIUD= value; }
		}

		[CategoryAttribute("Facts"),
		 XmlElement(DataType="double", Namespace = "", ElementName = "EstimatedCost"),
		 DescriptionAttribute("Lower limit for estimated query cost. \n" +
							  "E.g. 1000 means estimated cost >= 1000 ")]
		public double Estimated_Cost {
			get { return _estimatedCost; }
			set { _estimatedCost= value; }
		}


		[CategoryAttribute("Facts"),
		 XmlElement(Namespace = "", ElementName = "QueryState"),
		 DescriptionAttribute("Filter based on the query state [Client-Side Filter].")]
		public States State
		{
			get { return m_state; }
			set { m_state = value; }
		}


		[CategoryAttribute("Facts"),
		 XmlIgnore(),
		 DescriptionAttribute("Lower limit for the elapsed time. [Client-Side Filter]\n" +
							  "Filter on  >=  value in hh:mm:ss format.")]
		public TimeSpan Elapsed_Time {
			get { return m_elapsedTime; }
			set { m_elapsedTime = value; }
		}

		[CategoryAttribute("Facts"), Browsable(false),
		 XmlElement(DataType = "duration", Namespace = "", ElementName = "ElapsedTime"),
		 DescriptionAttribute("The elapsed time in XML format for save and load support.")]
		public String ElapsedTimeXML {
			get {
				if (TimeSpan.FromSeconds(0) > m_elapsedTime)
					m_elapsedTime = TimeSpan.FromSeconds(0);

				return XmlConvert.ToString(m_elapsedTime);
			}

			set {
				String theNewValue = value;
				if (null == theNewValue)
					theNewValue = "0";

				m_elapsedTime = XmlConvert.ToTimeSpan(theNewValue);
			}
		}



		[CategoryAttribute("Dimensions"),
		 XmlElement(Namespace = "", ElementName = "TimePeriod"),
		 DescriptionAttribute("Filter based on the specified time period.")]
		public FilterTimePeriod TimePeriod {
			get { return _timePeriod; }
			set 
            {
				_timePeriod = value;
                //if (this._enablePrompts  &&  (FilterTimePeriod.Custom == value))
                //    getStartAndEndDateTimes();
			}
		}

		[CategoryAttribute("Dimensions"), Browsable(false),
		 XmlElement(DataType="dateTime", Namespace = "", ElementName = "FromTime"),
	    DescriptionAttribute("The starting time in mm/dd/yyyy hh:mm AM|PM format.\n   04/27/2007 11:30 AM")]
		public DateTime From
		{
			get { return m_startTime; }
			set { m_startTime = value; }
		}

		[CategoryAttribute("Dimensions"), Browsable(false),
		 XmlElement(DataType="dateTime", Namespace = "", ElementName = "ToTime"),
   	     DescriptionAttribute("The ending time in mm/dd/yyyy hh:mm AM|PM format.\n     04/27/2007 11:30 AM")]
		public DateTime To
		{
			get { return m_endTime; }
			set { m_endTime = value; }
		}

		[Editor(typeof(TriageFilterSelectionEditor), typeof(UITypeEditor))]
		[CategoryAttribute("Dimensions"), XmlArray(Namespace = "", IsNullable = true, ElementName = "UserNames"),
		 DescriptionAttribute("Select the User(s) from the list.")]
		public string[] Users
		{
			get { return m_userName; }
			set { m_userName = value; }
		}

		[Editor(typeof(TriageFilterSelectionEditor), typeof(UITypeEditor))]
		[CategoryAttribute("Dimensions"), XmlArray(Namespace = "", IsNullable = true, ElementName = "ClientIDs"),
		 DescriptionAttribute("Select the Client ID(s) from the list.")]
		public string[] Clients
		{
			get { return m_clientID; }
			set { m_clientID = value; }
		}

		[Editor(typeof(TriageFilterSelectionEditor), typeof(UITypeEditor))]
		[CategoryAttribute("Dimensions"), XmlArray(Namespace = "", IsNullable = true, ElementName = "ApplicationIDs"),
 		 DescriptionAttribute("Select the Application ID(s) from the list.")]
		public string[] Applications
		{
			get { return m_applicationID; }
			set { m_applicationID = value; }
		}


		[CategoryAttribute("Identifiers"), XmlElement(Namespace = "", ElementName = "QueryID"),
		 DescriptionAttribute("Filter based on a Query ID. \n")]
		public String Query_ID {
			get { return _queryID; }
			set { _queryID = value; }
		}

		[CategoryAttribute("Identifiers"), XmlElement(Namespace = "", ElementName = "SessionID"),
		 DescriptionAttribute("Filter based on a Session ID. \n")]
		public String Session_ID {
			get { return _sessionID; }
			set { _sessionID = value; }
		}



        [CategoryAttribute("Miscellaneous"),
         Browsable(false), XmlIgnore(),
         DescriptionAttribute("The helper class to logically hold the functions")]
        public TriageHelper TriageHelper
        {
            get { return _theTriageHelper; }
            set { _theTriageHelper = value; }
        }

		#endregion Properties

		public TriageFilterInfo() {
			//  Need to set the default filter time period to the last hour. Its set to ANY in the initializer
			//  code so that we can determine whether or not this object was deserialized (on a load) or 
			//  constructed -- in which case this constructor would be invoked.
			this._timePeriod = FilterTimePeriod.Last_Hour;
		}

        //public void enablePrompts(bool flag) {
        //    this._enablePrompts = flag;
        //}

		public void resetFilters() {
			this.m_applicationID = null;
			this.m_clientID = null;
			this.m_userName = null;
			this.m_sqlType = Types.Any;
			this.m_error = Errors.Any;
			this.m_state = States.Any;
			this.m_startTime = new DateTime();
			this.m_endTime = new DateTime();
			this.m_elapsedTime = TimeSpan.Zero;
			this._queryTextLike = null;
			this._timePeriod = FilterTimePeriod.Last_Hour;
			this._trafodionSQLPredicate = null;
			this._rowsAccessed = 0;
			this._rowsRetrieved = 0;
			this._rowsIUD = 0;
			this._estimatedCost = 0.0;
			this._queryID = null;
			this._sessionID = null;
		}


		public void copyFilterInformation(TriageFilterInfo copyFrom) {
			this.resetFilters();
			this.m_applicationID = copyFrom.Applications;
			this.m_clientID = copyFrom.Clients;
			this.m_userName = copyFrom.Users;
			this.m_sqlType = copyFrom.Type;
			this.m_error = copyFrom.Error;
			this.m_state = copyFrom.State;
			this.m_startTime = copyFrom.From;
			this.m_endTime = copyFrom.To;
			this.m_elapsedTime = copyFrom.Elapsed_Time;
			this._queryTextLike = copyFrom.Text;
			this._timePeriod = copyFrom.TimePeriod;

			if (FilterTimePeriod.Any == copyFrom.TimePeriod) {
				// Check on older saved versions and set custom accordingly.
				if ((DateTime.MinValue < this.m_startTime)  ||
					(DateTime.MinValue < this.m_endTime) )
					this._timePeriod = FilterTimePeriod.Custom;
			}

			this._trafodionSQLPredicate = copyFrom.Custom_Filter;
			this._rowsAccessed = copyFrom.Rows_Accessed;
			this._rowsRetrieved = copyFrom.Row_Count;
			this._rowsIUD = copyFrom.IUD_Count;
			this._estimatedCost = copyFrom.Estimated_Cost;
			this._queryID = copyFrom.Query_ID;
			this._sessionID = copyFrom.Session_ID;
		}

        /// <summary>
        /// File up the custom DateTime dialog to get Start and End Datetime.
        /// </summary>
        /// <returns></returns>
		public bool GetStartAndEndDateTimes() {
			TriageCustomDateTimeEntry customForm = new TriageCustomDateTimeEntry();

			if (DateTime.MinValue < m_startTime)
				customForm.StartDateTime = m_startTime;

			if (DateTime.MinValue < m_endTime)
				customForm.EndDateTime = m_endTime;


			DialogResult result = customForm.ShowDialog();

			if (DialogResult.OK == result) {
				m_startTime = customForm.StartDateTime;
				m_endTime = customForm.EndDateTime;
				return true;
			}

			return false;
		}


	}
}
