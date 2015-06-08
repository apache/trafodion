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
using System;
using System.Collections.Generic;
using System.Text;
using System.Xml.Serialization;

namespace Trafodion.Manager.UniversalWidget
{
    /// <summary>
    /// This class shall be used to add drill down mapping from one widget 
    /// to the next
    /// </summary>
    [Serializable]
    public class AssociatedWidgetConfig
    {
        string _theCallingWidgetName;
        string _theCalledWidgetName;
        string _theAssociationReason;

        string _theCallingWidgetID;
        string _theCalledWidgetID;

        List<ParameterMapping> _theParameterMappings;
        
        public AssociatedWidgetConfig()
        {
        }
        
        //public AssociatedWidgetConfig(string aCallingWidgetName, string aCalledWidgetName) : this(aCalledWidgetName)
        //{
        //    CallingWidgetName = aCallingWidgetName;
        //}
        public AssociatedWidgetConfig(string aCallingWidgetID, string aCalledWidgetID)
            : this(aCalledWidgetID)
        {
            CallingWidgetID = aCallingWidgetID;
        }

        //public AssociatedWidgetConfig(string aCalledWidgetName): this()
        //{
        //    CalledWidgetName = aCalledWidgetName;
        //}

        public AssociatedWidgetConfig(string aCalledWidgetID)
            : this()
        {
            CalledWidgetID = aCalledWidgetID;
        }

        /// <summary>
        /// The widget from which the drill down is being invoked
        /// </summary>
        [XmlElement("CallingWidgetID")]
        public string CallingWidgetID
        {
            get { return _theCallingWidgetID; }
            set { _theCallingWidgetID = value; }
        }

        /// <summary>
        /// The widget that is being called as a result of the drill down
        /// </summary>
        [XmlElement("CalledWidgetID")]
        public string CalledWidgetID
        {
            get { return _theCalledWidgetID; }
            set { _theCalledWidgetID = value; }
        }


        /// <summary>
        /// The widget from which the drill down is being invoked
        /// </summary>
        [XmlElement("CallingWidgetName")]
        public string CallingWidgetName
        {
            get { return _theCallingWidgetName; }
            set { _theCallingWidgetName = value; }
        }

        /// <summary>
        /// The widget that is being called as a result of the drill down
        /// </summary>
        [XmlElement("CalledWidgetName")]
        public string CalledWidgetName
        {
            get { return _theCalledWidgetName; }
            set { _theCalledWidgetName = value; }
        }

        /// <summary>
        /// The reason why this association was created
        /// </summary>
        [XmlElement("AssociationReason")]
        public string AssociationReason
        {
            get { return _theAssociationReason; }
            set { _theAssociationReason = value; }
        }

        [XmlArray("ParameterMappings")]
        [XmlArrayItem("ParameterMapping")]
        public List<ParameterMapping> ParameterMappings
        {
            get { return _theParameterMappings; }
            set { _theParameterMappings = value; }
        }

        public override string ToString()
        {
            return CalledWidgetName;
        }
    }


    /// <summary>
    /// Contains the mapping and transformations of the column values of the table
    /// to the pameter of the next widget
    /// </summary>
    [Serializable]
    public class ParameterMapping
    {
        string _theColumnName;
        string _theParamName;

        /// <summary>
        /// The name of the column that will be passed as a parameter to the next widget
        /// </summary>
        [XmlElement("ColumnName")]
        public string ColumnName
        {
            get { return _theColumnName; }
            set { _theColumnName = value; }
        }

        /// <summary>
        /// The parameter name that will be mapped to from the column name
        /// </summary>
        [XmlElement("ParamName")]
        public string ParamName
        {
            get { return _theParamName; }
            set { _theParamName = value; }
        }
    }
}
