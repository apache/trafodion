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

namespace Trafodion.Manager.DatabaseArea.Queries
{
    [Serializable]
    public class ReportControlStatement : IComparable<ReportControlStatement>
    {
        private bool _disable;               //Whether the statement is disabled
        private string _type;                //The type of the control statement
        private string _attribute;           //The attribute of the statement
        private string _value;               //The value of the statement
        private bool _isInternal = false;

        public ReportControlStatement()
        {
        }

        public ReportControlStatement(ReportControlStatement aReportControlStatement)
        {
            Set(aReportControlStatement);
        }

        public void Set(ReportControlStatement aReportControlStatement)
        {
            Disable = aReportControlStatement.Disable;
            CQType = aReportControlStatement.CQType;
            Attribute = aReportControlStatement.Attribute;
            Value = aReportControlStatement.Value;
        }

        public override string ToString()
        {
            string ret = "";
            ret  += "disable       : " + _disable + "\n";
            ret  += "type          : " + _type + "\n";
            ret  += "attribute     : " + _attribute + "\n";
            ret  += "value         : " + _value + "\n";
            return ret;
        }

        public bool IsInternal
        {
            get { return _isInternal; }
            set { _isInternal = value; }
        }

        public bool Disable
        {
            get { return _disable; }
            set { _disable = value; }
        }

        public string CQType
        {
            get { return _type; }
            set { _type = value; }
        }

        public string Attribute
        {
            get { return _attribute; }
            set { _attribute = value; }
        }

        public string Value
        {
            get { return _value; }
            set { _value = value; }
        }

        /// <summary>
        /// The Equals method
        /// </summary>
        /// <param name="controlStatement"></param>
        /// <returns></returns>
        public bool Equals(ReportControlStatement controlStatement)
        {
            if (Disable == controlStatement.Disable &&
                CQType == controlStatement.CQType &&
                Attribute == controlStatement.Attribute &&
                Value == controlStatement.Value)
            {
                return true;
            }

            return false;
        }

        #region IComparable<ReportControlStatement> Members

        public int CompareTo(ReportControlStatement aReportControlStatement)
        {
            if (CQType != aReportControlStatement.CQType)
            {
                return CQType.CompareTo(aReportControlStatement.CQType);
            }
            else if (Attribute != aReportControlStatement.Attribute)
            {
                return Attribute.CompareTo(aReportControlStatement.Attribute);
            }
            else if (Value != aReportControlStatement.Value)
            {
                return Value.CompareTo(aReportControlStatement.Value);
            }

            return 0;
        }

        #endregion
    }
}
