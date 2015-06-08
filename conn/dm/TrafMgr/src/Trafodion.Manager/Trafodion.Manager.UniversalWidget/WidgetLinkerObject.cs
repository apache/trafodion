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
using System.Data;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using Trafodion.Manager.Framework.Queries;
//using Trafodion.Manager.Framework.Queries.Controls;
namespace Trafodion.Manager.UniversalWidget
{
    /// <summary>
    /// Will contain all information that a widget can pass to another widget 
    /// when it's invoked
    /// </summary>
    public class WidgetLinkerObject
    {
        public const string TimePattern = "yyyy-MM-dd HH:mm:ss";
        //The name of the widget that the user is doing an operation
        private string _theCallingWidget;

        public string CallingWidget
        {
            get { return _theCallingWidget; }
            set { _theCallingWidget = value; }
        } 
        //The name of the widget that will be displayed based on the user action
        private string _theCalledWidget;

        public string CalledWidget
        {
            get { return _theCalledWidget; }
            set { _theCalledWidget = value; }
        }
        //This will conatin the values of the cells or the points on a graph that the user clicked on 
        private Hashtable _theSelectedObjects;

        public Hashtable SelectedObjects
        {
            get { return _theSelectedObjects; }
            set { _theSelectedObjects = value; }
        }
        //This will have the column name/value for each cell of the row that the user clicked on
        private Hashtable _theRowHashTable;

        public Hashtable RowHashTable
        {
            get { return _theRowHashTable; }
            set { _theRowHashTable = value; }
        }        
        //Will have the entire dataTable as is displayed on the calling widget
        private DataTable _theCallingWidgetDataTable;

        public DataTable CallingWidgetDataTable
        {
            get { return _theCallingWidgetDataTable; }
            set { _theCallingWidgetDataTable = value; }
        }
        //The catch all bucket for additional parameters that the calling widget might want to pass
        private Hashtable _theAdditionalParameters;

        public Hashtable AdditionalParameters
        {
            get { return _theAdditionalParameters; }
            set { _theAdditionalParameters = value; }
        }

        // [Note] These functions have been moved to the class where these functions are used.  The purpose is to make the universal widget project
        //        not to reference anything in DatabaseArea.  
        // 
        //public void UpdateFromUserInput(List<ReportParameter> userInputs)
        //{
        //    if (userInputs != null)
        //    {
        //        if ((CallingWidget != null) && (CalledWidget != null))
        //        {
        //            UniversalWidgetConfig callingConfig = WidgetRegistry.GetInstance().GetUniversalWidgetConfig(CallingWidget);
        //            //AssociatedWidgetConfig assocConfig = callingConfig.GetAssociationByName(CallingWidget, CalledWidget);
        //            AssociatedWidgetConfig assocConfig = callingConfig.GetAssociationByID(CallingWidget, CalledWidget);
        //            if ((assocConfig != null) && (assocConfig.ParameterMappings != null) && (assocConfig.ParameterMappings.Count > 0))
        //            {
        //                foreach (ParameterMapping pm in assocConfig.ParameterMappings)
        //                {
        //                    ReportParameter reportParam = getReportParameterForName(userInputs, pm.ParamName);
        //                    if (reportParam != null)
        //                    {
        //                        if (this.RowHashTable.ContainsKey(pm.ParamName))
        //                        {
        //                            this.RowHashTable.Remove(pm.ParamName);
        //                        }
        //                        this.RowHashTable.Add(pm.ParamName, reportParam.Value);                                
        //                    }
        //                }
        //            }
        //        }
        //    }
        //}

        //private ReportParameter getReportParameterForName(List<ReportParameter> userInputs, string aParamName)
        //{
        //    foreach (ReportParameter reportParam in userInputs)
        //    {
        //        if (reportParam.ParamName.Equals(aParamName))
        //        {
        //            return reportParam;
        //        }
        //    }
        //    return null;
        //}

        public Hashtable MappedParameters
        {
            get
            {
                bool hasFrom = false;
                bool hasTo = false;
                if ((CallingWidget != null) && (CalledWidget != null))
                {
                    UniversalWidgetConfig callingConfig = WidgetRegistry.GetInstance().GetUniversalWidgetConfig(CallingWidget);
                    //AssociatedWidgetConfig assocConfig = callingConfig.GetAssociationByName(CallingWidget, CalledWidget);
                    AssociatedWidgetConfig assocConfig = callingConfig.GetAssociationByID(CallingWidget, CalledWidget);
                    if ((assocConfig != null) && (assocConfig.ParameterMappings != null) && (assocConfig.ParameterMappings.Count > 0))
                    {
                        Hashtable paramMappings = new Hashtable();
                        //map columns appropritaely
                        foreach (ParameterMapping pm in assocConfig.ParameterMappings)
                        {
                            if (_theRowHashTable.ContainsKey(pm.ColumnName))
                            {
                                hasFrom = (hasFrom || pm.ParamName.Equals(ReportParameterProcessorBase.FROM_TIME)) ? true : false;
                                hasTo = (hasTo || pm.ParamName.Equals(ReportParameterProcessorBase.TO_TIME)) ? true : false;
                                if (!paramMappings.ContainsKey(pm.ParamName))
                                {                                    
                                    Object value = _theRowHashTable[pm.ColumnName];
                                    if (value is DateTime)
                                    {
                                        paramMappings.Add(pm.ParamName, ((DateTime)value).ToString(TimePattern));
                                    }
                                    else
                                    {
                                        paramMappings.Add(pm.ParamName, _theRowHashTable[pm.ColumnName]);
                                    }
                                }
                            }
                            /* Fix issue 5546 */
                            else if (pm.ColumnName.Trim().Length == 0)
                            {
                                string additionalParameterName = pm.ParamName;
                                if (_theRowHashTable.ContainsKey(additionalParameterName) && !paramMappings.ContainsKey(additionalParameterName))
                                {
                                    paramMappings.Add(additionalParameterName, _theRowHashTable[additionalParameterName]);
                                }
                            }
                        }

                        //Since we have from and to time set we are setting the custom date range
                        if (hasFrom && hasTo)
                        {
                            paramMappings.Add(ReportParameterProcessorBase.TIME_RANGE_KEY, TimeRangeInputBase.CUSTOM_RANGE);
                        }
                        return paramMappings;
                    }
                }
                return _theRowHashTable;
            }
        }
    }
}
