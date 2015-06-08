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

namespace Trafodion.Manager.DatabaseArea.Queries
{
    [Serializable]
    public class ReportParameter : IComparable<ReportParameter>
    {
        private string paramName;           //The actual name of the param
        private string displayName;         //The param name as displayed in the label
        private string sqlDataType;         //The SQL data type
        private string description;         //Used for the tool tip if available
        private object value;               //The value provided by the user
        private ArrayList possibleValues = new ArrayList();   //List of of values provided by the author/user
        private bool isInternal = false;
        public ReportParameter()
        {
        }

        public ReportParameter(ReportParameter aReportParameter)
        {
            Set(aReportParameter);
        }

        public void Set(ReportParameter aReportParameter)
        {
            ParamName = aReportParameter.ParamName;
            DisplayName = aReportParameter.DisplayName;
            SqlDataType = aReportParameter.SqlDataType;
            Value = aReportParameter.Value;
            PossibleValues = new ArrayList(aReportParameter.PossibleValues);
        }


        //Checks if two params as equal
        public override bool Equals(object obj)
        {
            if (obj == null)
            {
                return false;
            }
            ReportParameter compareTo = obj as ReportParameter;
            if (compareTo != null)
            {
                if ((paramName == null)  || (compareTo.ParamName == null))
                {
                    return false;
                }
                if (paramName.Equals(compareTo.ParamName))
                {
                    return true;
                }
            }
            return false;
        }//Equals

        public override int GetHashCode()
        {
            return paramName.GetHashCode();
        }

        public override string ToString()
        {
            string ret = "";
            ret  += "paramName       : " + paramName + "\n";
            ret  += "displayName     : " + displayName + "\n";
            ret  += "sqlDataType     : " + sqlDataType + "\n";
            ret  += "description     : " + description + "\n";
            ret  += "value           : " + value + "\n";
            ret  += "possibleValues  : " + possibleValues + "\n";
            return ret;
        }



        public bool IsInternal
        {
            get { return isInternal; }
            set { isInternal = value; }
        }

        public string ParamName
        {
            get { return paramName; }
            set { paramName = value; }
        }

        public string DisplayName
        {
            get { return displayName; }
            set { displayName = value; }
        }

        public string SqlDataType
        {
            get { return sqlDataType; }
            set { sqlDataType = value; }
        }

        public string Description
        {
            get { return description; }
            set { description = value; }
        }

        public object Value
        {
            get { return this.value; }
            set { this.value = value; }
        }

        public ArrayList PossibleValues
        {
            get { return possibleValues; }
            set { possibleValues = value; }
        }



        #region IComparable<ReportParameter> Members

        public int CompareTo(ReportParameter aReportParameter)
        {
            return ParamName.CompareTo(aReportParameter.ParamName);
        }

        #endregion
    }
}
