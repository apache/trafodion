//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
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
using System.Collections;
using System.Data;
using System.Data.Odbc;
using System.Windows.Forms;
using System.IO;
using System.Xml;
using System.Xml.Serialization;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using Trafodion.Manager.DatabaseArea.Queries.Controls;
using Trafodion.Manager.Framework.Queries;

namespace Trafodion.Manager.OverviewArea.Models
{
    /// <summary>
    /// Object to save filter values
    /// </summary>
    [Serializable]
    [XmlRoot("EventFilter")]
    [XmlInclude(typeof(NameValuePair))]
    [XmlInclude(typeof(TimeRangeHandler))]
    public class EventFilterModel
    {
        // Define event display column id
        public const int EVENT_GENERATION_TIME_TS_COL_ID = 0;
        public const int EVENT_SUBSYSTEM_COL_ID = 1;
        public const int EVENT_PROCESS_COL_ID = 2;
        public const int EVENT_EVENT_ID_COL_ID = 3;
        public const int EVENT_SEVERITY_COL_ID = 4;
        public const int EVENT_ROLE_COL_ID = 5;
        public const int EVENT_MESSAGE_COL_ID = 6;
        public const int EVENT_NODE_COL_ID = 7;
        public const int EVENT_PROCESS_NAME_COL_ID = 8;
        public const int EVENT_TOKENIZED_EVENT_TABLE_COL_ID = 9;

        // Define data grid column names
        public const string EVENT_ID = "Event ID";
        public const string PROCESS_ID = "Process ID";
        public const string SUBSYSTEM = "Subsystem";
        public const string SEVERITY = "Severity";
        public const string EVENT_TIME = "Event Time (server local time)";
        public const string ROLE_NAME = "Role Name";
        public const string MESSAGE = "Message";
        public const string NODE_ID = "Node ID";
        public const string PROCESS_NAME = "Process Name";
        public const string TOKENIZED_EVENT_TABLE = "Tokenized Event Table";

        // Define Live Feed column names
        public const string LV_EVENT_GENERATION_TIME_TS_COL_NAME = "info_header_info_generation_time_ts_lct";
        public const string LV_EVENT_SUBSYSTEM_COL_NAME = "event_header_event_id";
        public const string LV_EVENT_PROCESS_COL_NAME = "info_header_info_process_id";
        public const string LV_EVENT_EVENT_ID_COL_NAME = "info_header_info_component_id";
        public const string LV_EVENT_SEVERITY_COL_NAME = "event_header_event_severity";
        public const string LV_EVENT_ROLE_COL_NAME = "qpid_header_process_name";
        public const string LV_EVENT_MESSAGE_COL_NAME = "text";
        public const string LV_EVENT_NODE_COL_NAME = "info_header_info_node_id";
        public const string LV_EVENT_PROCESS_NAME_COL_NAME = "info_header_info_process_name";

        // Define event filter persistence key
        public const string EventFilterPersistenceKey = "EventFilterPersistenceKey";
        public const string LiveEventFilterPersistenceKey = "LiveEventFilterPersistenceKey";

        #region Member variables
        List<NameValuePair> _theSubSystemFilters = new List<NameValuePair>();
        List<NameValuePair> _theSeverityFilters = new List<NameValuePair>();
        string _theEventIds = "";
        string _theProcessIds = "";
        string _theProcessNames = "";
        TimeRangeHandler.Range _theTimeRange;
        bool _theCurrentTime = true;
        DateTime _theCustomEndTime;
        DateTime _theCustomStartTime;
        string _theMessaegeFilter;
        bool _theCaseSensitive;
        NameValuePair _theMessageFilterCondition;

        [NonSerialized]
        private bool _theSubSystemAllChecked = false;

        [NonSerialized]
        private bool _theSeverityAllChecked = false;

        //[NonSerialized]
        //private List<string> _eventIds = new List<string>();
        //[NonSerialized]
        //private List<string> _processIds = new List<string>();
        //[NonSerialized]
        //private List<string> _processNames = new List<string>();
        #endregion
        #region Constructors

        [XmlArray("SubSystemFilters")]
        [XmlArrayItem("SubSystemFilter")]
        public List<NameValuePair> SubSystemFilters
        {
            get { return _theSubSystemFilters; }
            set { _theSubSystemFilters = value; }
        }

        [XmlArray("SeverityFilters")]
        [XmlArrayItem("SeverityFilter")]
        public List<NameValuePair> SeverityFilters
        {
            get { return _theSeverityFilters; }
            set { _theSeverityFilters = value; }
        }

        [XmlElement("EventIds")]
        public string EventIds
        {
            get 
            {
                return getFormattedListString(GetIntIds(_theEventIds));
                //return _theEventIds; 
            }
            set { _theEventIds = getFormattedListString(GetIntIds(value)); }
        }
        [XmlElement("ProcessIds")]
        public string ProcessIds
        {
            get 
            {
                return getFormattedListString(GetIntIds(_theProcessIds));
                //return _theProcessIds; 
            }
            set { _theProcessIds = getFormattedListString(GetIntIds(value)); }
        }
        [XmlElement("ProcessNames")]
        public string ProcessNames
        {
            get { return getFormattedListString(GetIds(_theProcessNames)); }
            set { _theProcessNames = getFormattedListString(GetIds(value)); }
        }
        [XmlElement("TimeRange")]
        public TimeRangeHandler.Range TimeRange
        {
            get { return _theTimeRange; }
            set { _theTimeRange = value; }
        }
        [XmlElement("CurrentTime")]
        public bool CurrentTime
        {
            get { return _theCurrentTime; }
            set { _theCurrentTime = value; }
        }
        [XmlElement("TheEndTime")]
        public DateTime TheEndTime
        {
            get { return _theCustomEndTime; }
            set { _theCustomEndTime = value; }
        }
        [XmlElement("TheStartTime")]
        public DateTime TheStartTime
        {
            get { return _theCustomStartTime; }
            set { _theCustomStartTime = value; }
        }
        [XmlElement("MessaegeFilter")]
        public string MessaegeFilter
        {
            get { return _theMessaegeFilter; }
            set { _theMessaegeFilter = value; }
        }
        [XmlElement("CaseSensitive")]
        public bool CaseSensitive
        {
            get { return _theCaseSensitive; }
            set { _theCaseSensitive = value; }
        }
        [XmlElement("MessageFilterCondition")]
        public NameValuePair MessageFilterCondition
        {
            get { return _theMessageFilterCondition; }
            set { _theMessageFilterCondition = value; }
        }

        [XmlIgnore()]
        public bool SubSystemAllChecked
        {
            get { return _theSubSystemAllChecked; }
            set { _theSubSystemAllChecked = value; }
        }

        [XmlIgnore()]
        public bool SeverityAllChecked
        {
            get { return _theSeverityAllChecked; }
            set { _theSeverityAllChecked = value; }
        }

        #endregion

        #region Public Methods
        
        //Compares the passed EventFilterModel to the current EventFilterModel
        public override bool Equals(Object obj)
        {
            EventFilterModel newModel = obj as EventFilterModel;
            if (newModel != null)
            {
                bool ret = true;

                //has _theSubSystemFilters changed
                ret = ret && AreSubSystemFiltersSame(newModel);

                //has _theSeverityFilters changed
                ret = ret && AreSeverityFiltersSame(newModel);

                //has _theEventIds changed
                ret = ret && AreEventIdsSame(newModel);

                //has _theProcessIds changed
                ret = ret && AreProcessIdsSame(newModel);

                //has _theProcessNames changed
                ret = ret && AreProcessNamesSame(newModel);

                //has _theTimeRange changed
                ret = ret && _theTimeRange.Equals(newModel.TimeRange);

                //has _theCurrentTime flag changed
                ret = ret && (_theCurrentTime == newModel.CurrentTime);

                //has _theStartTime  changed
                ret = ret && _theCustomEndTime.Equals(newModel.TheEndTime);

                //has _theEndTime  changed
                ret = ret && _theCustomStartTime.Equals(newModel.TheStartTime);

                //has _theMessaegeFilter  changed
                ret = ret && _theMessaegeFilter.Equals(newModel.MessaegeFilter);

                //has _theCaseSensitive flag changed
                ret = ret && (_theCaseSensitive == newModel.CaseSensitive);

                //has _theMessageFilterCondition changed
                ret = ret && (_theMessageFilterCondition.Equals(newModel.MessageFilterCondition));

                return ret;
            }
            return false;
        }

        public override int GetHashCode()
        {
            return base.GetHashCode();
        }

        public static List<string> GetIntIds(string eventIdsStr)
        {
            List<string> eventIds = new List<string>();
            if (eventIdsStr != null)
            {
                string[] ids = eventIdsStr.Split(new string[] { "," }, StringSplitOptions.RemoveEmptyEntries);
                foreach (string id in ids)
                {
                    try
                    {
                        Int32.Parse(id.Trim());
                        eventIds.Add(id.Trim());
                    }
                    catch (Exception)
                    {
                    }
                }
            }
            return eventIds;
        }

        public static List<string> GetIds(string eventIdsStr)
        {
            List<string> eventIds = new List<string>();
            if (eventIdsStr != null)
            {
                string[] ids = eventIdsStr.Split(new string[] { "," }, StringSplitOptions.RemoveEmptyEntries);
                foreach (string id in ids)
                {

                    eventIds.Add(id.Trim());
                }
            }
            return eventIds;
        }

        /// <summary>
        /// Get filter string for regular Event viewer. This string is to be used as the predicates for the SQL text
        /// fetching the event table in the instance repository. 
        /// </summary>
        /// <returns></returns>
        public string GetFilterSQL()
        {
            string ret = "";
            //get sub-system filter string            
            if (_theSubSystemFilters.Count > 0)
            {
                String SubSystemFilter = string.Format("{0} IN (", "component_id");
                for (int i = 0; i < _theSubSystemFilters.Count; i++)
                {
                    SubSystemFilter = SubSystemFilter + _theSubSystemFilters[i].Value + ((i < (_theSubSystemFilters.Count - 1)) ? ", " : "");
                }
                SubSystemFilter = SubSystemFilter + ")";
                ret = addToCondition(ret, SubSystemFilter);
            }

            //get severity filter string            
            if (_theSeverityFilters.Count > 0)
            {
                String SeverityFilter = string.Format("{0} IN (", "SEVERITY");
                for (int i = 0; i < _theSeverityFilters.Count; i++)
                {
                    SeverityFilter = SeverityFilter + _theSeverityFilters[i].Value + ((i < (_theSeverityFilters.Count - 1)) ? ", " : "");
                }
                SeverityFilter = SeverityFilter + ")";
                ret = addToCondition(ret, SeverityFilter);
            }

            //get event id filter string
            List<string> eventIds = GetIds(_theEventIds);
            if (eventIds.Count > 0)
            {
                String EventIdFilter = string.Format("{0} IN (", "event_id");
                for (int i = 0; i < eventIds.Count; i++)
                {
                    EventIdFilter = EventIdFilter + eventIds[i] + ((i < (eventIds.Count - 1)) ? ", " : "");
                }
                EventIdFilter = EventIdFilter + ")";
                ret = addToCondition(ret, EventIdFilter);
            }

            //get process id filter string
            List<string> processIds = GetIds(_theProcessIds);
            if (processIds.Count > 0)
            {
                String ProcessIdFilter = string.Format("{0} IN (", "process_id");
                for (int i = 0; i < processIds.Count; i++)
                {
                    ProcessIdFilter = ProcessIdFilter + processIds[i] + ((i < (processIds.Count - 1)) ? ", " : "");
                }
                ProcessIdFilter = ProcessIdFilter + ")";
                ret = addToCondition(ret, ProcessIdFilter);
            }

            //get process name filter string
            List<string> processNames = GetIds(_theProcessNames);
            if (processNames.Count > 0)
            {
                String ProcessNameFilter = string.Format("{0} IN (", "PROCESS_NAME");
                for (int i = 0; i < processNames.Count; i++)
                {
                    string processName = processNames[i].Trim().Replace("'", "''");
                    ProcessNameFilter = ProcessNameFilter + string.Format("'{0}'", processName) + ((i < (processNames.Count - 1)) ? ", " : "");
                }
                ProcessNameFilter = ProcessNameFilter + ")";
                ret = addToCondition(ret, ProcessNameFilter);
            }

            //get time range filter
            if (_theTimeRange != TimeRangeHandler.Range.AllTimes)
            {
                String TimeFilter = "GEN_TS_LCT";
                if (_theTimeRange == TimeRangeHandler.Range.CustomRange)
                {
                    if (_theCurrentTime)
                    {
                        TimeFilter = string.Format("{0} >= {1}", TimeFilter, getDateForQuery(_theCustomStartTime));
                    }
                    else
                    {
                        TimeFilter = string.Format("({0} >= {1}) AND ({2} <= {3}) ", TimeFilter, getDateForQuery(_theCustomStartTime), TimeFilter, getDateForQuery(_theCustomEndTime));
                    }
                }
                else
                {
                    TimeFilter = string.Format("{0} {1}", TimeFilter, TimeRangeInput.ComputeSQLTimeRange(new TimeRangeHandler(_theTimeRange).GetTimeRangeString(_theTimeRange)));
                }
                ret = addToCondition(ret, TimeFilter);
            }

            //get message filter
            if (_theMessaegeFilter.Trim().Length > 0)
            {
                String MessageFilter = (!_theCaseSensitive) ? "UPSHIFT(text)" : "text";
                String filterMessage = (!_theCaseSensitive) ? _theMessaegeFilter.Trim().ToUpper() : _theMessaegeFilter.Trim();
                filterMessage = filterMessage.Replace("'", "''");
                if (_theMessageFilterCondition.Value.Equals("Contains"))
                {
                    MessageFilter = string.Format("{0} LIKE '%{1}%'", MessageFilter, filterMessage);
                }
                else if (_theMessageFilterCondition.Value.Equals("Equals"))
                {
                    MessageFilter = string.Format("{0} = '{1}'", MessageFilter, filterMessage);
                }
                else if (_theMessageFilterCondition.Value.Equals("Does Not Contain"))
                {
                    MessageFilter = string.Format("{0} NOT LIKE '%{1}%'", MessageFilter, filterMessage);
                }
                ret = addToCondition(ret, MessageFilter);
            }

            return ret;

        }

        /// <summary>
        /// Get the sql fiterling string for live feed. This will be used as the predicates for filtering the cached event data table
        /// in the Cached Event Data provider.  
        /// </summary>
        /// <returns></returns>
        public string GetFilterLiveFeedSQL()
        {
            string ret = "";
            //get sub-system filter string
            //do not filter since the backend will do the filtering
            //if (_theSubSystemFilters.Count > 0)
            //{
            //    String SubSystemFilter = string.Format("{0} IN (", "info_header_info_component_id");
            //    for (int i = 0; i < _theSubSystemFilters.Count; i++)
            //    {
            //        SubSystemFilter = SubSystemFilter + _theSubSystemFilters[i].Value + ((i < (_theSubSystemFilters.Count - 1)) ? ", " : "");
            //    }
            //    SubSystemFilter = SubSystemFilter + ")";
            //    ret = addToCondition(ret, SubSystemFilter);
            //}

            //get severity filter string
            //do not filter since the backend will do the filtering
            //if (_theSeverityFilters.Count > 0)
            //{
            //    String SeverityFilter = string.Format("{0} IN (", "event_header_event_severity");
            //    for (int i = 0; i < _theSeverityFilters.Count; i++)
            //    {
            //        SeverityFilter = SeverityFilter + _theSeverityFilters[i].Value + ((i < (_theSeverityFilters.Count - 1)) ? ", " : "");
            //    }
            //    SeverityFilter = SeverityFilter + ")";
            //    ret = addToCondition(ret, SeverityFilter);
            //}

            //get event id filter string
            //do not filter since the backend will do the filtering
            //List<string> eventIds = GetIds(_theEventIds);
            //if (eventIds.Count > 0)
            //{
            //    String EventIdFilter = string.Format("{0} IN (", "event_header_event_id");
            //    for (int i = 0; i < eventIds.Count; i++)
            //    {
            //        EventIdFilter = EventIdFilter + eventIds[i] + ((i < (eventIds.Count - 1)) ? ", " : "");
            //    }
            //    EventIdFilter = EventIdFilter + ")";
            //    ret = addToCondition(ret, EventIdFilter);
            //}

            //get process id filter string
            //do not filter since the backend will do the filtering
            //List<string> processIds = GetIds(_theProcessIds);
            //if (processIds.Count > 0)
            //{
            //    String ProcessIdFilter = string.Format("{0} IN (", "info_header_info_process_id");
            //    for (int i = 0; i < processIds.Count; i++)
            //    {
            //        ProcessIdFilter = ProcessIdFilter + processIds[i] + ((i < (processIds.Count - 1)) ? ", " : "");
            //    }
            //    ProcessIdFilter = ProcessIdFilter + ")";
            //    ret = addToCondition(ret, ProcessIdFilter);
            //}

            //get process name filter string
            //List<string> processNames = GetIds(_theProcessNames);
            //if (processNames.Count > 0)
            //{
            //    String ProcessNameFilter = string.Format("{0} IN (", "qpid_header_process_name");
            //    for (int i = 0; i < processNames.Count; i++)
            //    {
            //        string processName = processNames[i].Trim().Replace("'", "''");
            //        ProcessNameFilter = ProcessNameFilter + string.Format("'{0}'", processName) + ((i < (processNames.Count - 1)) ? ", " : "");
            //    }
            //    ProcessNameFilter = ProcessNameFilter + ")";
            //    ret = addToCondition(ret, ProcessNameFilter);
            //}

            // not supporting time range.
            ////get time range filter
            //if (_theTimeRange != TimeRangeHandler.Range.AllTimes)
            //{
            //    String TimeFilter = "GEN_TS_LCT";
            //    if (_theTimeRange == TimeRangeHandler.Range.CustomRange)
            //    {
            //        if (_theCurrentTime)
            //        {
            //            TimeFilter = string.Format("{0} >= {1}", TimeFilter, getDateForQuery(_theCustomStartTime));
            //        }
            //        else
            //        {
            //            TimeFilter = string.Format("({0} >= {1}) AND ({2} <= {3}) ", TimeFilter, getDateForQuery(_theCustomStartTime), TimeFilter, getDateForQuery(_theCustomEndTime));
            //        }
            //    }
            //    else
            //    {
            //        TimeFilter = string.Format("{0} {1}", TimeFilter, TimeRangeInput.ComputeSQLTimeRange(new TimeRangeHandler(_theTimeRange).GetTimeRangeString(_theTimeRange)));
            //    }
            //    ret = addToCondition(ret, TimeFilter);
            //}

            //get message filter
            if (_theMessaegeFilter.Trim().Length > 0)
            {
                String MessageFilter = "text";
                String filterMessage = (!_theCaseSensitive) ? _theMessaegeFilter.Trim().ToUpper() : _theMessaegeFilter.Trim();
                filterMessage = filterMessage.Replace("'", "''");
                if (_theMessageFilterCondition.Value.Equals("Contains"))
                {
                    MessageFilter = string.Format("{0} LIKE '%{1}%'", MessageFilter, filterMessage);
                }
                else if (_theMessageFilterCondition.Value.Equals("Equals"))
                {
                    MessageFilter = string.Format("{0} = '{1}'", MessageFilter, filterMessage);
                }
                else if (_theMessageFilterCondition.Value.Equals("Does Not Contain"))
                {
                    MessageFilter = string.Format("{0} NOT LIKE '%{1}%'", MessageFilter, filterMessage);
                }
                ret = addToCondition(ret, MessageFilter);
            }

            return ret;

        }

        /// <summary>
        /// The LiveFeed filter api allows only one field at a time. Therefore, we'll need to have an array to 
        /// handle all of the filtering fields.
        /// </summary>
        /// <returns></returns>
        public string[] GetFilterLiveFeed()
        {
            List<string> ret = new List<string>();

            //get sub-system filter string            
            if (_theSubSystemFilters.Count > 0 && !SubSystemAllChecked)
            {
                String SubSystemFilter = string.Format("event.common.instance.public.gpb.text_event.header.header.info_component_id");
                for (int i = 0; i < _theSubSystemFilters.Count; i++)
                {
                    SubSystemFilter += " = " + _theSubSystemFilters[i].Value + ((i < (_theSubSystemFilters.Count - 1)) ? " ||" : "");
                }

                ret.Add(SubSystemFilter);
            }

            //get severity filter string            
            if (_theSeverityFilters.Count > 0 && !SeverityAllChecked)
            {
                String SeverityFilter = string.Format("event.common.instance.public.gpb.text_event.header.event_severity");
                for (int i = 0; i < _theSeverityFilters.Count; i++)
                {
                    SeverityFilter += " = " + _theSeverityFilters[i].Value + ((i < (_theSeverityFilters.Count - 1)) ? " ||" : "");
                }

                ret.Add(SeverityFilter);
            }

            //get event id filter string
            List<string> eventIds = GetIds(_theEventIds);
            if (eventIds.Count > 0)
            {
                String EventIdFilter = string.Format("event.common.instance.public.gpb.text_event.header.event_id");
                for (int i = 0; i < eventIds.Count; i++)
                {
                    EventIdFilter += " = " + eventIds[i] + ((i < (eventIds.Count - 1)) ? " ||" : "");
                }

                ret.Add(EventIdFilter);
            }

            //get process id filter string
            List<string> processIds = GetIds(_theProcessIds);
            if (processIds.Count > 0)
            {
                String ProcessIdFilter = string.Format("event.common.instance.public.gpb.text_event.header.header.info_process_id");
                for (int i = 0; i < processIds.Count; i++)
                {
                    ProcessIdFilter += " = " + processIds[i] + ((i < (processIds.Count - 1)) ? " ||" : "");
                }

                ret.Add(ProcessIdFilter);
            }

            //get process name filter string
            List<string> processNames = GetIds(_theProcessNames);
            if (processNames.Count > 0)
            {
                String ProcessNameFilter = string.Format("event.common.instance.public.gpb.text_event.header.header.info_process_name");
                for (int i = 0; i < processNames.Count; i++)
                {
                    ProcessNameFilter += " = " + processNames[i] + ((i < (processNames.Count - 1)) ? " ||" : "");
                }

                ret.Add(ProcessNameFilter);
            }

            // M6 backend not supporting the following criteria.
            ////get time range filter
            //if (_theTimeRange != TimeRangeHandler.Range.AllTimes)
            //{
            //    String TimeFilter = "GEN_TS_LCT";
            //    if (_theTimeRange == TimeRangeHandler.Range.CustomRange)
            //    {
            //        if (_theCurrentTime)
            //        {
            //            TimeFilter = string.Format("{0} >= {1}", TimeFilter, getDateForQuery(_theCustomStartTime));
            //        }
            //        else
            //        {
            //            TimeFilter = string.Format("({0} >= {1}) AND ({2} <= {3}) ", TimeFilter, getDateForQuery(_theCustomStartTime), TimeFilter, getDateForQuery(_theCustomEndTime));
            //        }
            //    }
            //    else
            //    {
            //        TimeFilter = string.Format("{0} {1}", TimeFilter, TimeRangeInput.ComputeSQLTimeRange(new TimeRangeHandler(_theTimeRange).GetTimeRangeString(_theTimeRange)));
            //    }
            //    //ret = addToCondition(ret, TimeFilter);
            //}

            ////get message filter
            //if (_theMessaegeFilter.Trim().Length > 0)
            //{
            //    String MessageFilter = (_theCaseSensitive) ? "UPSHIFT(text)" : "text";
            //    String filterMessage = (_theCaseSensitive) ? _theMessaegeFilter.Trim().ToUpper() : _theMessaegeFilter.Trim();
            //    filterMessage = filterMessage.Replace("'", "''");
            //    if (_theMessageFilterCondition.Value.Equals("Contains"))
            //    {
            //        MessageFilter = string.Format("{0} LIKE '%{1}%'", MessageFilter, filterMessage);
            //    }
            //    else if (_theMessageFilterCondition.Value.Equals("Equals"))
            //    {
            //        MessageFilter = string.Format("{0} = '{1}'", MessageFilter, filterMessage);
            //    }
            //    else if (_theMessageFilterCondition.Value.Equals("Does Not Contain"))
            //    {
            //        MessageFilter = string.Format("{0} NOT LIKE '%{1}%'", MessageFilter, filterMessage);
            //    }
            //    //ret = addToCondition(ret, MessageFilter);
            //}

            return ret.ToArray();

        }

        /*public string GetLiveFeedFormattedFilterString(LiveFeedEventsDataHandler aLiveFeedEventsDataHandler)
        {
            string ret = "";

            //get sub-system filter string            
            if (_theSubSystemFilters.Count > 0)
            {
                ColumnDetails cd = aLiveFeedEventsDataHandler.getColumnDetailsForName("info_header_info_component_id");
                if (cd != null)
                {
                    String SubSystemFilter = string.Format("{0} IN (", cd.DisplayName);
                    for (int i = 0; i < _theSubSystemFilters.Count; i++)
                    {
                        SubSystemFilter = SubSystemFilter + _theSubSystemFilters[i].Name + ((i < (_theSubSystemFilters.Count - 1)) ? ", " : "");
                    }
                    SubSystemFilter = SubSystemFilter + ")";
                    ret = addToCondition(ret, SubSystemFilter);
                }
            }

            //get severity filter string            
            if (_theSeverityFilters.Count > 0)
            {
                ColumnDetails cd = aLiveFeedEventsDataHandler.getColumnDetailsForName("event_header_event_severity");
                if (cd != null)
                {
                    String SeverityFilter = string.Format("{0} IN (", cd.DisplayName);
                    for (int i = 0; i < _theSeverityFilters.Count; i++)
                    {
                        SeverityFilter = SeverityFilter + _theSeverityFilters[i].Name + ((i < (_theSeverityFilters.Count - 1)) ? ", " : "");
                    }
                    SeverityFilter = SeverityFilter + ")";
                    ret = addToCondition(ret, SeverityFilter);
                }
            }

            //get event id filter string
            List<string> eventIds = GetIds(_theEventIds);
            if (eventIds.Count > 0)
            {
                ColumnDetails cd = aLiveFeedEventsDataHandler.getColumnDetailsForName("event_header_event_id");
                if (cd != null)
                {
                    String EventIdFilter = string.Format("{0} IN (", cd.DisplayName);
                    for (int i = 0; i < eventIds.Count; i++)
                    {
                        EventIdFilter = EventIdFilter + eventIds[i] + ((i < (eventIds.Count - 1)) ? ", " : "");
                    }
                    EventIdFilter = EventIdFilter + ")";
                    ret = addToCondition(ret, EventIdFilter);
                }
            }

            //get process id filter string
            List<string> processIds = GetIds(_theProcessIds);
            if (processIds.Count > 0)
            {
                ColumnDetails cd = aLiveFeedEventsDataHandler.getColumnDetailsForName("info_header_info_process_id");
                if (cd != null)
                {
                    String ProcessIdFilter = string.Format("{0} IN (", cd.DisplayName);
                    for (int i = 0; i < processIds.Count; i++)
                    {
                        ProcessIdFilter = ProcessIdFilter + processIds[i] + ((i < (processIds.Count - 1)) ? ", " : "");
                    }
                    ProcessIdFilter = ProcessIdFilter + ")";
                    ret = addToCondition(ret, ProcessIdFilter);
                }
            }

            //get process name filter string
            List<string> processNames = GetIds(_theProcessNames);
            if (processNames.Count > 0)
            {
                ColumnDetails cd = aLiveFeedEventsDataHandler.getColumnDetailsForName("info_header_info_process_name");
                if (cd != null)
                {

                    String ProcessNameFilter = string.Format("{0} IN (", cd.DisplayName);
                    for (int i = 0; i < processNames.Count; i++)
                    {
                        ProcessNameFilter = ProcessNameFilter + string.Format("'{0}'", processNames[i].Trim()) + ((i < (processNames.Count - 1)) ? ", " : "");
                    }
                    ProcessNameFilter = ProcessNameFilter + ")";
                    ret = addToCondition(ret, ProcessNameFilter);
                }
            }
            
            // For M6, LiveFeed does not support time range filter.
            ////get time range filter
            //if (_theTimeRange != TimeRangeHandler.Range.AllTimes)
            //{
            //    ColumnDetails cd = aLiveFeedEventsDataHandler.getColumnDetailsForName("info_header_info_generation_time_ts_lct");
            //    if (cd != null)
            //    {
            //        String TimeFilter = cd.DisplayName;
            //        if (_theTimeRange == TimeRangeHandler.Range.CustomRange)
            //        {
            //            if (_theCurrentTime)
            //            {
            //                TimeFilter = string.Format("{0} >= {1}", TimeFilter, _theCustomStartTime);
            //            }
            //            else
            //            {
            //                TimeFilter = string.Format("({0} >= {1}) AND ({2} <= {3}) ", TimeFilter, _theCustomStartTime, TimeFilter, _theCustomEndTime);
            //            }
            //        }
            //        else
            //        {
            //            TimeFilter = string.Format("{0} IN {1}", TimeFilter, new TimeRangeHandler(_theTimeRange).ToString());
            //        }
            //        ret = addToCondition(ret, TimeFilter);
            //    }
            //}
            //get message filter
            if (_theMessaegeFilter.Trim().Length > 0)
            {
                ColumnDetails cd = aLiveFeedEventsDataHandler.getColumnDetailsForName("text");
                if (cd != null)
                {
                    String MessageFilter = (!_theCaseSensitive) ? string.Format("UPSHIFT({0})", cd.DisplayName) : cd.DisplayName;
                    String filterMessage = (!_theCaseSensitive) ? _theMessaegeFilter.Trim().ToUpper() : _theMessaegeFilter.Trim();
                    if (_theMessageFilterCondition.Value.Equals("Contains"))
                    {
                        MessageFilter = string.Format("{0} LIKE '%{1}%'", MessageFilter, filterMessage);
                    }
                    else if (_theMessageFilterCondition.Value.Equals("Equals"))
                    {
                        MessageFilter = string.Format("{0} = '{1}'", MessageFilter, filterMessage);
                    }
                    else if (_theMessageFilterCondition.Value.Equals("Does Not Contain"))
                    {
                        MessageFilter = string.Format("{0} NOT LIKE '%{1}%'", MessageFilter, filterMessage);
                    }
                    ret = addToCondition(ret, MessageFilter);
                }
            }

            return ret;
        }*/

        public string GetFormattedFilterString(TextEventsDataHandler aTextEventsDataHandler)
        {
            string ret = "";

            if (aTextEventsDataHandler == null)
                return "Filter string is ...";

            //get sub-system filter string            
            if (_theSubSystemFilters.Count > 0)
            {
                ColumnDetails cd = aTextEventsDataHandler.getColumnDetailsForName("component_id");
                if (cd != null)
                {
                    String SubSystemFilter = string.Format("{0} IN (", cd.DisplayName);
                    for (int i = 0; i < _theSubSystemFilters.Count; i++)
                    {
                        SubSystemFilter = SubSystemFilter + _theSubSystemFilters[i].Name + ((i < (_theSubSystemFilters.Count - 1)) ? ", " : "");
                    }
                    SubSystemFilter = SubSystemFilter + ")";
                    ret = addToCondition(ret, SubSystemFilter);
                }
            }

            //get severity filter string            
            if (_theSeverityFilters.Count > 0)
            {
                ColumnDetails cd = aTextEventsDataHandler.getColumnDetailsForName("SEVERITY");
                if (cd != null)
                {
                    String SeverityFilter = string.Format("{0} IN (", cd.DisplayName);
                    for (int i = 0; i < _theSeverityFilters.Count; i++)
                    {
                        SeverityFilter = SeverityFilter + _theSeverityFilters[i].Name + ((i < (_theSeverityFilters.Count - 1)) ? ", " : "");
                    }
                    SeverityFilter = SeverityFilter + ")";
                    ret = addToCondition(ret, SeverityFilter);
                }
            }

            //get event id filter string
            List<string> eventIds = GetIds(_theEventIds);
            if (eventIds.Count > 0)
            {
                ColumnDetails cd = aTextEventsDataHandler.getColumnDetailsForName("event_id");
                if (cd != null)
                {
                    String EventIdFilter = string.Format("{0} IN (", cd.DisplayName);
                    for (int i = 0; i < eventIds.Count; i++)
                    {
                        EventIdFilter = EventIdFilter + eventIds[i] + ((i < (eventIds.Count - 1)) ? ", " : "");
                    }
                    EventIdFilter = EventIdFilter + ")";
                    ret = addToCondition(ret, EventIdFilter);
                }
            }

            //get process id filter string
            List<string> processIds = GetIds(_theProcessIds);
            if (processIds.Count > 0)
            {
                ColumnDetails cd = aTextEventsDataHandler.getColumnDetailsForName("process_id");
                if (cd != null)
                {
                    String ProcessIdFilter = string.Format("{0} IN (", cd.DisplayName);
                    for (int i = 0; i < processIds.Count; i++)
                    {
                        ProcessIdFilter = ProcessIdFilter + processIds[i] + ((i < (processIds.Count - 1)) ? ", " : "");
                    }
                    ProcessIdFilter = ProcessIdFilter + ")";
                    ret = addToCondition(ret, ProcessIdFilter);
                }
            }

            //get process name filter string
            List<string> processNames = GetIds(_theProcessNames);
            if (processNames.Count > 0)
            {
                ColumnDetails cd = aTextEventsDataHandler.getColumnDetailsForName("PROCESS_NAME");
                if (cd != null)
                {

                    String ProcessNameFilter = string.Format("{0} IN (", cd.DisplayName);
                    for (int i = 0; i < processNames.Count; i++)
                    {
                        ProcessNameFilter = ProcessNameFilter + string.Format("'{0}'", processNames[i].Trim()) + ((i < (processNames.Count - 1)) ? ", " : "");
                    }
                    ProcessNameFilter = ProcessNameFilter + ")";
                    ret = addToCondition(ret, ProcessNameFilter);
                }
            }

            //get time range filter
            if (_theTimeRange != TimeRangeHandler.Range.AllTimes)
            {
                ColumnDetails cd = aTextEventsDataHandler.getColumnDetailsForName("GEN_TS_LCT");
                if (cd != null)
                {
                    String TimeFilter = cd.DisplayName;
                    if (_theTimeRange == TimeRangeHandler.Range.CustomRange)
                    {
                        if (_theCurrentTime)
                        {
                            TimeFilter = string.Format("{0} >= {1}", TimeFilter, _theCustomStartTime);
                        }
                        else
                        {
                            TimeFilter = string.Format("({0} >= {1}) AND ({2} <= {3}) ", TimeFilter, _theCustomStartTime, TimeFilter, _theCustomEndTime);
                        }
                    }
                    else
                    {
                        TimeFilter = string.Format("{0} IN {1}", TimeFilter, new TimeRangeHandler(_theTimeRange).ToString());
                    }
                    ret = addToCondition(ret, TimeFilter);
                }
            }
            //get message filter
            if (_theMessaegeFilter.Trim().Length > 0)
            {
                ColumnDetails cd = aTextEventsDataHandler.getColumnDetailsForName("text");
                if (cd != null)
                {
                    String MessageFilter = (!_theCaseSensitive) ? string.Format("UPSHIFT({0})",cd.DisplayName) : cd.DisplayName;
                    String filterMessage = (!_theCaseSensitive) ? _theMessaegeFilter.Trim().ToUpper() : _theMessaegeFilter.Trim();
                    if (_theMessageFilterCondition.Value.Equals("Contains"))
                    {
                        MessageFilter = string.Format("{0} LIKE '%{1}%'", MessageFilter, filterMessage);
                    }
                    else if (_theMessageFilterCondition.Value.Equals("Equals"))
                    {
                        MessageFilter = string.Format("{0} = '{1}'", MessageFilter, filterMessage);
                    }
                    else if (_theMessageFilterCondition.Value.Equals("Does Not Contain"))
                    {
                        MessageFilter = string.Format("{0} NOT LIKE '%{1}%'", MessageFilter, filterMessage);
                    }
                    ret = addToCondition(ret, MessageFilter);
                }
            }

            return ret;
        }

        public string AddEventId(string eventId)
        {
            if (eventId != null)
            {
                eventId = eventId.Trim();
                List<string> eventIds = GetIds(_theEventIds);
                if (!eventIds.Contains(eventId))
                {
                    eventIds.Add(eventId);
                }
                _theEventIds = getFormattedListString(eventIds);
            }
            return _theEventIds;
        }
        
        public string AddProcessId(string processId)
        {
            if (processId != null)
            {
                processId = processId.Trim();
                List<string> processes = GetIds(_theProcessIds);
                if (!processes.Contains(processId))
                {
                    processes.Add(processId);
                }
                _theProcessIds = getFormattedListString(processes);
            }
            return _theProcessIds;
        }
        
        public string AddProcessName(string processName)
        {
            if (processName != null)
            {
                processName = processName.Trim();
                List<string> processes = GetIds(_theProcessNames);
                if (!processes.Contains(processName))
                {
                    processes.Add(processName);
                }
                _theProcessNames = getFormattedListString(processes);
            }
            return _theProcessNames;
        }
        
        public ArrayList IsValid()
        {
            ArrayList ret = new ArrayList();            
            if (_theTimeRange == TimeRangeHandler.Range.CustomRange)
            {
                //we only need to check Start time must be less than or equal to the End time if the 
                //current time flag has not been set.
                if ((! CurrentTime) && (TheEndTime.CompareTo(TheStartTime) < 0))
                {
                    ret.Add("The Start time must be less than or equal to the End time");
                }
            }
            return ret;
        }
        #endregion

        #region Private Methods



        private string getFormattedListString(List<string> list)
        {
            string ret = "";
            int count = list.Count;
            for (int i = 0; i < count; i++)
            {
                ret += (i < (count - 1)) ? list[i].Trim() + ", " : list[i].Trim();
            }
            return ret;
        }

        //Helper to create the date string that is needed in the SQL statement
        public string getDateForQuery(DateTime aDateTime)
        {
            string timestamp = aDateTime.ToString("yyyy-MM-dd HH:mm:ss");
            return string.Format("TIMESTAMP '{0}'", timestamp);
        }

        //Helper method to add the filter conditions
        public string addToCondition(String existingFilter, string newFilter)
        {
            if ((newFilter != null) && (newFilter.Trim().Length > 0))
            {
                if (existingFilter.Trim().Length == 0)
                {
                    existingFilter = string.Format("({0})", newFilter);
                }
                else
                {
                    existingFilter = string.Format("{0} AND ({1})", existingFilter, newFilter.Trim());
                }
            }
            return existingFilter;
        }

        private bool AreSubSystemFiltersSame(EventFilterModel newModel)
        {
            if (_theSubSystemFilters.Count == newModel.SubSystemFilters.Count)
            {
                foreach (NameValuePair nvp in _theSubSystemFilters)
                {
                    if (!(newModel.SubSystemFilters.IndexOf(nvp) >= 0))
                    {
                        return false;
                    }
                }
                return true;
            }
            else
            {
                return false;
            }
        }

        private bool AreSeverityFiltersSame(EventFilterModel newModel)
        {
            if (_theSeverityFilters.Count == newModel.SeverityFilters.Count)
            {
                foreach (NameValuePair nvp in _theSeverityFilters)
                {
                    if (!(newModel.SeverityFilters.IndexOf(nvp) >= 0))
                    {
                        return false;
                    }
                }
                return true;
            }
            else
            {
                return false;
            }
        }
        private bool AreEventIdsSame(EventFilterModel newModel)
        {
            return AreStringListsSame(_theEventIds, newModel.EventIds);
        }

        private bool AreProcessIdsSame(EventFilterModel newModel)
        {
            return AreStringListsSame(_theProcessIds, newModel.ProcessIds);
        }

        private bool AreProcessNamesSame(EventFilterModel newModel)
        {
            return AreStringListsSame(_theProcessNames, newModel.ProcessNames);
        }


        private bool AreStringListsSame(string oldList, string newList)
        {
            List<string> oldIds = GetIds(oldList);
            List<string> newIds = GetIds(newList);

            if (oldIds.Count == newIds.Count)
            {
                foreach (string id in oldIds)
                {
                    if (!(newIds.IndexOf(id) >= 0))
                    {
                        return false;
                    }
                }
                return true;
            }
            else
            {
                return false;
            }
        }       
        

        #endregion
    }

    /// <summary>
    /// Class to save name value pairs in the UI componenets
    /// </summary>
    [Serializable]
    [XmlRoot("EventFilter")]
    public class NameValuePair
    {
        string _name = "";
        [XmlElement("Name")]
        public string Name
        {
            get { return _name; }
            set { _name = value; }
        }
        object _value = null;
        [XmlElement("Value")]
        public object Value
        {
            get { return _value; }
            set { _value = value; }
        }
        public NameValuePair(string aName, object aValue)
        {
            _name = aName;
            _value = aValue;
        }
        public override string ToString()
        {
            return _name;
        }

        public override bool Equals(object obj)
        {
            NameValuePair nvp = obj as NameValuePair;
            return (nvp != null) && (nvp.Name.Equals(_name)) && (nvp.Value.Equals(_value));
        }

        public override int GetHashCode()
        {
            return base.GetHashCode();
        }
    }

    /// <summary>
    /// Helper class to manage the various default filters
    /// </summary>
    public class EventDetails
    {
        #region Member Variables
        private ConnectionDefinition _theConnectionDefinition;
        private Connection _theCurrentConnection = null;
        private Hashtable _theSubSystemsTable =  new Hashtable();
        private Hashtable _theSeveritiesTable =  new Hashtable();

        private ArrayList _theSubSystems;
        private ArrayList _theSeverities;
        static private string[] _theMessageFilters = { "Contains", "Does Not Contain" };
        static private TimeRangeHandler[] _theTimeRanges = { new TimeRangeHandler()
                                                           , new TimeRangeHandler(TimeRangeHandler.Range.CustomRange) 
                                                           , new TimeRangeHandler(TimeRangeHandler.Range.Last10Minutes) 
                                                           , new TimeRangeHandler(TimeRangeHandler.Range.Last20Minutes)
                                                           , new TimeRangeHandler(TimeRangeHandler.Range.Last30Minutes)
                                                           , new TimeRangeHandler(TimeRangeHandler.Range.Last1Hour)
                                                           , new TimeRangeHandler(TimeRangeHandler.Range.Today)
                                                           , new TimeRangeHandler(TimeRangeHandler.Range.Last24Hours)
                                                           , new TimeRangeHandler(TimeRangeHandler.Range.Last7Days)};

        private const string TRACE_SUB_AREA_NAME = "Event Filter";
        #endregion

        #region Constructor
        public EventDetails(ConnectionDefinition aConnectionDefinition)
        {
            _theConnectionDefinition = aConnectionDefinition;
        }
        #endregion

        #region Properties
        public string[] MessageFilters
        {
            get { return EventDetails._theMessageFilters; }
            set { EventDetails._theMessageFilters = value; }
        }

        public TimeRangeHandler[] TimeRanges
        {
            get { return _theTimeRanges; }
            set { _theTimeRanges = value; }
        }

        public ArrayList SubSystems
        {
            get 
            {
                return _theSubSystems; 
            }
            set 
            { 
                _theSubSystems = value;
                _theSubSystemsTable.Clear();
                foreach (ComponentModel cm in _theSubSystems)
                {
                    _theSubSystemsTable.Add(cm.Component, cm);
                }
            }
        }

        public ArrayList Severities
        {
            get
            {
                return _theSeverities;
            }
            set
            {
                _theSeverities = value;
                _theSeveritiesTable.Clear();
                foreach (SeverityModel sm in _theSeverities)
                {
                    _theSeveritiesTable.Add(sm.Severity, sm);
                }
            }
        }        
        #endregion

        #region Public Methods

        /// <summary>
        /// Get a list of defined components (either from compoent dimention table or from TrafodionManager's own hard coded list).
        /// </summary>
        /// <returns></returns>
        public ArrayList GetComponentList()
        {
            ArrayList componentList = null;

            //Just get it from the hard list
            //if (_theConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
            //{
            //    try
            //    {
            //        componentList = GetComponentsFromDB();
            //    }
            //    catch (Exception ex)
            //    { }
            //}

            if (componentList == null)
            {
                componentList = GetComponentsFromHardList();
            }

            return componentList;
        }

        /// <summary>
        /// Returns the components configured in the component_dimension table
        /// </summary>
        /// <returns></returns>
        public ArrayList GetComponentsFromDB()
        {
            ArrayList componentList = new ArrayList();
            if (GetConnection())
            {
                try
                {
                    string sql = "SELECT component_id , component_name , component_description FROM manageability.access.dimension_component_1 ORDER BY component_name ASC";
                    OdbcCommand theQuery = new OdbcCommand(sql);
                    theQuery.Connection = _theCurrentConnection.OpenOdbcConnection;
                    OdbcDataReader reader = Utilities.ExecuteReader(theQuery, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Overview, TRACE_SUB_AREA_NAME, false);
                    while (reader.Read())
                    {
                        ComponentModel cm = new ComponentModel();
                        cm.Component = reader.GetInt16(0);
                        cm.ComponentName = reader.GetString(1).Trim();
                        cm.ComponentDescription = reader.GetString(2).Trim();
                        componentList.Add(cm);
                    }
                }
                finally
                {
                    CloseConnection(); 
                }
            }
            return componentList;
        }

        /// <summary>
        /// Returns the components configured in the component_dimension table
        /// </summary>
        /// <returns></returns>
        public ArrayList GetComponentsFromHardList()
        {
            string[][] cmps = new string[][] { 
                                                new string[] { "6", "AMP", "Audit Management Process" },
                                                new string[] { "5", "ASE", "Audit Storage Engine" },
                                                new string[] { "3", "DTM", "Distributed Transaction Manager" }, 
                                                new string[] { "17", "IODRV", "I/O Driver layer"  },
                                                new string[] { "10", "LUNMGR", "LUN Manager"  },
                                                new string[] { "1", "Monitor", "Monitor component" }, 
                                                new string[] { "8", "HPDCS", "Connectivity services"  },
                                                new string[] { "7", "Recovery", "Transaction recovery component"  },
                                                new string[] { "9", "SQL", "SQL database engine"  },
                                                new string[] { "2", "SeaBed", "SeaBed library"  },
                                                new string[] { "14", "SeaBridge", "Database manageability layer"  },
                                                new string[] { "11", "Security", "Security component"  },
                                                new string[] { "4", "TSE", "Table Storage Engine"  },
                                                new string[] { "15", "TSvc", "Transporter Services"  },
                                                new string[] { "13", "Transducer", "Manageability infrastructure"  },
                                                new string[] { "16", "WMS", "Workload Management Services" }
                                             };

            ArrayList componentList = new ArrayList();
            foreach (string[] cmp in cmps)
            {
                ComponentModel cm = new ComponentModel();
                cm.Component = int.Parse(cmp[0]);
                cm.ComponentName = cmp[1];
                cm.ComponentDescription = cmp[2];
                componentList.Add(cm);
            }

            return componentList;
        }

        /// <summary>
        /// Get a list of defined severities (either from compoent dimention table or from TrafodionManager's own hard coded list).
        /// </summary>
        /// <returns></returns>
        public ArrayList GetSeverityList()
        {
            ArrayList severityList = null;

            //Just get it from the hard list
            //if (_theConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
            //{
            //    try
            //    {
            //        severityList = GetSeveritiesFromDB();
            //    }
            //    catch (Exception ex)
            //    { }
            //}

            if (severityList == null)
            {
                severityList = GetSeveritiesFromHardList();
            }

            return severityList;
        }

        /// <summary>
        /// Returns all the severities configured in the DB
        /// </summary>
        /// <returns></returns>
        public ArrayList GetSeveritiesFromDB()
        {
            ArrayList list = new ArrayList();
            if (GetConnection())
            {
                try
                {
                    string sql = "SELECT severity , severity_name , severity_description  FROM manageability.access.dimension_severity_1  ORDER BY severity ASC";
                    OdbcCommand theQuery = new OdbcCommand(sql);
                    theQuery.Connection = _theCurrentConnection.OpenOdbcConnection;
                    OdbcDataReader reader = Utilities.ExecuteReader(theQuery, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Overview, TRACE_SUB_AREA_NAME, false);
                    while (reader.Read())
                    {
                        SeverityModel sm = new SeverityModel();
                        sm.Severity = reader.GetInt16(0);
                        sm.SeverityName = reader.GetString(1).Trim();
                        sm.SeverityDescription = reader.GetString(2).Trim();
                        list.Add(sm);
                    }
                }
                finally
                {
                    CloseConnection();
                }
            }
            return list;
        }

        /// <summary>
        /// Returns all the severities configured in the DB
        /// </summary>
        /// <returns></returns>
        public ArrayList GetSeveritiesFromHardList()
        {
            string[][] sevs = new string[][] { 
                                             new string[] { "0","Emergency","System unusable" },
                                             new string[] { "1" ,"Immediate", "Immediate action required" }, 
                                             new string[] { "2", "Critical", "Critical condition" }, 
                                             new string[] { "3", "Error", "Error condition" },  
                                             new string[] { "4", "Warning", "Warning condition" }, 
                                             new string[] { "5","Notice","Normal but significant condition" }, 
                                             new string[] { "6", "Informational", "Informational" },
                                             new string[] { "7", "Debug", "Debug-level message" }
                                            };

            ArrayList list = new ArrayList();
            foreach (string[] sev in sevs)
            {
                SeverityModel sm = new SeverityModel();
                sm.Severity = int.Parse(sev[0]);
                sm.SeverityName = sev[1];
                sm.SeverityDescription = sev[2];
                list.Add(sm);
            }

            return list;
        }

        /// <summary>
        /// Given a sub-system id retturns the ComponentModel for that sub-system
        /// </summary>
        /// <param name="subsystem"></param>
        /// <returns></returns>
        public ComponentModel SubSystem(int subsystem)
        {
            if (_theSubSystemsTable.ContainsKey(subsystem))
            {
                return (ComponentModel)_theSubSystemsTable[subsystem];
            }

            ComponentModel undefined = new ComponentModel();
            undefined.Component = subsystem;
            undefined.ComponentName = string.Format("{0} ({1})", Properties.Resources.Undefined, subsystem);
            return undefined;
        }

        /// <summary>
        /// Given a severity id returns the SeverityModel for that severity
        /// </summary>
        /// <param name="severity"></param>
        /// <returns></returns>
        public SeverityModel Severity(int severity)
        {
            if (_theSeveritiesTable.ContainsKey(severity))
            {
                return (SeverityModel)_theSeveritiesTable[severity];
            }

            SeverityModel undefined = new SeverityModel();
            undefined.Severity = severity;
            undefined.SeverityName = string.Format("{0} ({1})", Properties.Resources.Undefined, severity);
            return undefined;
        }

        /// <summary>
        /// Gets a new connection object
        /// </summary>
        /// <returns></returns>
        public bool GetConnection()
        {
            if (this._theCurrentConnection == null && this._theConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
            {
                _theCurrentConnection = new Connection(_theConnectionDefinition);
                return true;
            }
            else
            {
                return false;
            }
        }

        /// <summary>
        /// Closes a connecttion
        /// </summary>
        public void CloseConnection()
        {
            if (this._theCurrentConnection != null)
            {
                _theCurrentConnection.Close();
                _theCurrentConnection = null;
            }
        }
        #endregion
    }


    /// <summary>
    /// Helper Class used to render the DateTime filters in the UI 
    /// </summary>
    [Serializable]
    public class TimeRangeHandler
    {
        static private string[] _theTimeRanges = { "All Times", "Custom Range", "Last 10 Minutes", "Last 20 Minutes", "Last 30 Minutes", "Last 1 Hour", "Today", "Last 24 Hours", "Last 7 Days", "Last 14 Days", "Last 30 Days" };
        public enum Range { AllTimes, CustomRange, Last10Minutes, Last20Minutes, Last30Minutes, Last1Hour, Today, Last24Hours, Last7Days, Last14Days, Last30Days };
        private Range _theRange = Range.AllTimes;
        private bool _isAllTimes = true;
        private bool _isCustomRange = false;
        public Hashtable rangeTable = new Hashtable();


        public Range TheRange
        {
            get { return _theRange; }
            set { _theRange = value; }
        }
        public bool IsAllTimes
        {
            get { return _isAllTimes; }
        }

        public bool IsCustomRange
        {
            get { return _isCustomRange; }
        }

        public TimeRangeHandler()
        {
            rangeTable.Add(Range.Last10Minutes, TimeRangeInputBase.LAST_10_MINS);
            rangeTable.Add(Range.Last20Minutes, TimeRangeInputBase.LAST_20_MINS);
            rangeTable.Add(Range.Last30Minutes, TimeRangeInputBase.LAST_30_MINS);
            rangeTable.Add(Range.Last1Hour, TimeRangeInputBase.LAST_HOUR);
            rangeTable.Add(Range.Today, TimeRangeInputBase.TODAY);
            rangeTable.Add(Range.Last24Hours, TimeRangeInputBase.LAST_24_HOURS);
            rangeTable.Add(Range.Last7Days, TimeRangeInputBase.LAST_7_DAYS);
            rangeTable.Add(Range.Last14Days, TimeRangeInputBase.LAST_14_DAYS);
            rangeTable.Add(Range.Last30Days, TimeRangeInputBase.LAST_30_DAYS);
        }

        public TimeRangeHandler(Range aRange):this()
        {
            _theRange = aRange;
            _isCustomRange = (aRange.Equals(Range.CustomRange));
            _isAllTimes = (aRange.Equals(Range.AllTimes));
        }

        public DateTime[] GetDateTimeRange()
        {
            return GetDateTimeRange(DateTime.Now);
        }
        public String GetTimeRangeString(Range aRange)
        {
            if (rangeTable.ContainsKey(aRange))
            {
                return rangeTable[aRange] as String;
            }
            return rangeTable[Range.Last1Hour] as String; ;
        }
        public DateTime[] GetDateTimeRange(DateTime serverTime)
        {
            if (_isCustomRange)
            {
                return null;
            }

            if (_isAllTimes)
            {
                return null;
            }

            DateTime[] returnValue = new DateTime[2];
            returnValue[1] = serverTime;
            switch (_theRange)
            {
                case Range.Last10Minutes:
                    returnValue[0] = serverTime.AddMinutes(-10);
                    break;
                case Range.Last20Minutes:
                    returnValue[0] = serverTime.AddMinutes(-20);
                    break;
                case Range.Last30Minutes:
                    returnValue[0] = serverTime.AddMinutes(-30);
                    break;
                case Range.Last1Hour:
                    returnValue[0] = serverTime.AddMinutes(-60);
                    break;
                case Range.Today:
                    returnValue[0] = DateTime.Today;
                    break;
                case Range.Last24Hours:
                    returnValue[0] = serverTime.AddHours(-24);
                    break;
                case Range.Last7Days:
                    returnValue[0] = serverTime.AddDays(-7);
                    break;
                case Range.Last14Days:
                    returnValue[0] = serverTime.AddDays(-14);
                    break;
                case Range.Last30Days:
                    returnValue[0] = serverTime.AddDays(-30);                    
                    break;

            }
            return returnValue;
        }

        public override string ToString()
        {
            return _theTimeRanges[(int)_theRange];
        }

        public override bool Equals(object obj)
        {
            TimeRangeHandler trh = obj as TimeRangeHandler;
            if (trh != null)
            {
                return (_theRange == trh.TheRange);
            }
            return false;
        }

        public override int GetHashCode()
        {
            return base.GetHashCode();
        }

    }

    /// <summary>
    /// Helper class to encapsulate the mapping of the internal and external name, data type and
    /// display conversion logic
    /// </summary>
    public class ColumnDetails
    {
        int _ColumnNumber = -1;
        string _ColumnName = "";
        string _DisplayName = "";
        Type _ColumnType;
        public delegate object GetFormattedValue(object value);
        GetFormattedValue _GetFormattedValueImpl;

        public GetFormattedValue GetFormattedValueImpl
        {
            get { return _GetFormattedValueImpl; }
            set { _GetFormattedValueImpl = value; }
        }

        public int ColumnNumber
        {
            get { return _ColumnNumber; }
            set { _ColumnNumber = value; }
        }

        public string ColumnName
        {
            get { return _ColumnName; }
            set { _ColumnName = value; }
        }

        public string DisplayName
        {
            get { return _DisplayName; }
            set { _DisplayName = value; }
        }

        public Type ColumnType
        {
            get { return _ColumnType; }
            set { _ColumnType = value; }
        }

        public ColumnDetails(int aColNum, string aColName, string aDisplayName, Type aColType)
        {
            _ColumnNumber = aColNum;
            _ColumnName = aColName;
            _DisplayName = aDisplayName;
            _ColumnType = aColType;
            _GetFormattedValueImpl = GetDefaultFormattedValueImpl;
        }

        public ColumnDetails(int aColNum, string aColName, string aDisplayName, Type aColType, GetFormattedValue aFormatter)
            : this(aColNum, aColName, aDisplayName, aColType)
        {
            _GetFormattedValueImpl = aFormatter;
        }


        private object GetDefaultFormattedValueImpl(object obj)
        {
            return obj;
        }

        public override bool Equals(object obj)
        {
            if (obj is string)
            {
                return ((string)obj).Equals(_ColumnName, StringComparison.InvariantCultureIgnoreCase);
            }
            else if (obj is ColumnDetails)
            {
                return ((ColumnDetails)obj).ColumnName.Equals(_ColumnName);
            }
            return false;
        }

        public override int GetHashCode()
        {
            return base.GetHashCode();
        }
    }

    /// <summary>
    /// Class to encapsulate severity information
    /// </summary>
    public class SeverityModel
    {
        private int _severity;
        private string _severityName;
        private string _severityDescription;

        public int Severity
        {
            get { return _severity; }
            set { _severity = value; }
        }

        public string SeverityName
        {
            get { return _severityName; }
            set { _severityName = value; }
        }

        public string SeverityDescription
        {
            get { return _severityDescription; }
            set { _severityDescription = value; }
        }

        public override bool Equals(object obj)
        {
            if (obj is SeverityModel)
            {
                SeverityModel sm = obj as SeverityModel;
                if (sm.Severity == _severity)
                {
                    return true;
                }
                else if (sm._severityName.Equals(_severityName, StringComparison.InvariantCultureIgnoreCase))
                {
                    return true;
                }
            }
            return false;
        }

        public override string ToString()
        {
            return _severityName;
        }

        public override int GetHashCode()
        {
            return base.GetHashCode();
        }

    }

    /// <summary>
    /// class to encapsulate sub-system information
    /// </summary>
    public class ComponentModel
    {
        private int _component;
        private string _componentName;
        private string _componentDescription;

        public int Component
        {
            get { return _component; }
            set { _component = value; }
        }

        public string ComponentName
        {
            get { return _componentName; }
            set { _componentName = value; }
        }

        public string ComponentDescription
        {
            get { return _componentDescription; }
            set { _componentDescription = value; }
        }

        public override bool Equals(object obj)
        {
            if (obj is ComponentModel)
            {
                ComponentModel cm = obj as ComponentModel;
                if (cm.Component == _component)
                {
                    return true;
                }
                else if (cm._componentName.Equals(_componentName, StringComparison.InvariantCultureIgnoreCase))
                {
                    return true;
                }
            }
            return false;
        }

        public override string ToString()
        {
            return _componentName;
        }

        public override int GetHashCode()
        {
            return base.GetHashCode();
        }
    }
}
