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
using System.Collections;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Queries.Controls;

namespace Trafodion.Manager.DatabaseArea.Queries
{
    [Serializable]
    public class SimpleReportDefinition : ReportDefinition, IComparable
    {
        private string _theName;

        private Dictionary<string, object> _theProperties = new Dictionary<string, object>();

        [NonSerialized]
        private const int THE_MAX_ONE_LINE_LENGTH = 120;

        [NonSerialized]
        private Control _theResultControl = null;

        [NonSerialized]
        private Control _thePlanControl = null;

        [NonSerialized]
        public const string ORIGINAL_NAME = "ORGINIAL_NAME";

        [NonSerialized]
        private string _theGroup = Trafodion.Manager.DatabaseArea.Properties.Resources.PersistenceFile; // default to persistence store

        /** Creates a new instance of SimpleReportDefinition */
        public SimpleReportDefinition(string aName)
        {
            _theName = (aName.Length > 0) ? aName : SimpleReportDefinition.CreateNewName("");
            SetProperty(ReportDefinition.TYPE, ReportDefinition.TEXT_REPORT);
        }

        public Dictionary<string, object> Properties
        {
            get { return _theProperties; }
        }

        public override Control ResultContainer
        {
            get { return _theResultControl; }
            set 
            {
                _theResultControl = value;
               
                if (_theResultControl != null)
                {
                    ((QueryResultContainer)_theResultControl).ReportDefinition = this;
                }
            }
        }

        public override Control PlanContainer
        {
            get { return _thePlanControl; }
            set
            {
                _thePlanControl = value;

                if (_thePlanControl != null)
                {
                    ((QueryPlanContainer)_thePlanControl).ReportDefinition = this;
                }
            }
        }

        public override string Name
        {
            get { return _theName; }
            set 
            {
                _theName = value;
                FireChanged(Reason.NameChanged);
            }
        }

        /// <summary>
        /// Property: Group - the group which the report is belong to.
        /// </summary>
        public string Group
        {
            get { return _theGroup; }
            set { _theGroup = value; }
        }

        public override string OneLineSummary 
        {
            get
            {
                string theString = QueryText;
                int newLineIndex = theString.IndexOf("\n");
                if (newLineIndex > 0)
                {
                    if (newLineIndex <= THE_MAX_ONE_LINE_LENGTH && theString.Length >= THE_MAX_ONE_LINE_LENGTH)
                    {
                        theString = theString.Substring(0, THE_MAX_ONE_LINE_LENGTH).Insert(THE_MAX_ONE_LINE_LENGTH, "...");
                    }
                    else
                    {
                        theString = theString.Substring(0, newLineIndex).Insert(newLineIndex, "...");
                    }
                }
                theString = theString.Substring(0, Math.Min(THE_MAX_ONE_LINE_LENGTH, theString.Length));
                theString = theString.Replace("\n", " ");
                theString = theString.Replace("\t", " ");
                return theString;
            }
        }

        public override string ToString()
        {
            return Name;
        }

        public string Version
        {
            get { return GetProperty(VERSION) as string; }
        }

        public string Type
        {
            get { return GetProperty(TYPE) as string; }
        }

        public string QueryText
        {
            get 
            {
                string theQueryText = (string)GetProperty(DEFINITION);
                return (theQueryText == null) ? "" : theQueryText;
            }
            set { SetProperty(DEFINITION, value); }
        }

        public override object GetProperty(string key)
        {
            return _theProperties.ContainsKey(key) ? _theProperties[key] : null;
        }

        public override void SetProperty(string key, object value)
        {
            _theProperties[key] = value;
            if (key.Equals(DEFINITION))
            {
                FireChanged(Reason.StatementChanged);
            }
        }

        /// <summary>
        /// Clear all properties except the essenital ones. 
        /// </summary>
        public override void  ResetProperty()
        {
            object defintion = GetProperty(ReportDefinition.DEFINITION);
            object paramsList = GetProperty(ReportDefinition.PARAMETERS);
            object controls = GetProperty(ReportDefinition.CONTROL_STATEMENTS);
            object originalName = GetProperty(SimpleReportDefinition.ORIGINAL_NAME);
            object fullRawText = GetProperty(ReportDefinition.FULL_RAW_TEXT);

            _theProperties.Clear();
            SetProperty(ReportDefinition.TYPE, ReportDefinition.TEXT_REPORT);
            SetProperty(ReportDefinition.DEFINITION, defintion);
            SetProperty(ReportDefinition.PARAMETERS, paramsList);
            SetProperty(ReportDefinition.CONTROL_STATEMENTS, controls);
            SetProperty(SimpleReportDefinition.ORIGINAL_NAME, (originalName != null) ? originalName : Name);
            SetProperty(ReportDefinition.FULL_RAW_TEXT, fullRawText);
        }

        /**Used to do a case-insensitive sort based on the report name
         */
        public int compareTo(object obj)
        {
            return (Name.ToUpper().CompareTo(((ReportDefinition)obj).Name.ToUpper()));
        }

        #region IComparable Members

        public int CompareTo(Object aSimpleReportDefinition)
        {
            if (aSimpleReportDefinition == null)
            {
                return -1;
            }
            else 
            {
                return Name.CompareTo(((SimpleReportDefinition)aSimpleReportDefinition).Name);
            }
        }

        #endregion

        /// <summary>
        /// Description
        /// </summary>
        public override string Description
        {
            get
            {
                string comments = "";
                string queryText = (string)GetProperty(DEFINITION);
                if (queryText != null && queryText.Length > 0)
                {
                    comments = ReportParameterProcessor.Instance.getBlockComment(queryText);
                }

                return comments;
            }
        }

        /// <summary>
        /// Returns a new name using the original name
        /// </summary>
        /// <returns></returns>
        public string GetNewName()
        {
            string OriginalName = null;
            if (_theProperties.ContainsKey(ORIGINAL_NAME))
            {
                OriginalName = GetProperty(ORIGINAL_NAME) as string;
            }

            if (OriginalName == null)
            {
                OriginalName = Name;
            }

            return OriginalName + (OriginalName.Length > 0 ? "_" : "") + DateTime.Now.ToString("yyyyMMddHHmmssfffffff");
        }

        /// <summary>
        /// Create a brand new report name with a given prefix.
        /// </summary>
        /// <param name="prefix"></param>
        /// <returns></returns>
        public static string CreateNewName(string prefix)
        {
            return prefix + (prefix.Length > 0 ? "_" : "") + DateTime.Now.ToString("yyyyMMddHHmmssfffffff");
        }

        /// <summary>
        /// Equals method
        /// </summary>
        /// <param name="obj"></param>
        /// <returns></returns>
        public override bool Equals(object obj)
        {
            if (!(obj is ReportDefinition))
            {
                return false;
            }

            if (String.Compare(this.Group, ((SimpleReportDefinition)obj).Group, true) != 0)
            {
                return false;
            }

            if (  this.Name.Equals(((ReportDefinition)obj).Name, StringComparison.OrdinalIgnoreCase))
            {
                return true;
            }

            return false;
        }

        /// <summary>
        /// if two objects are equal then they must both have the same hash code. Or,
        /// if two objects have different hash codes then they must be unequal.
        /// </summary>
        /// <returns></returns>
        public override int GetHashCode()
        {
            return this.Name.GetHashCode();
        }
    }
}
