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
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    public partial class QueryParametersUserControl : UserControl
    {
        Dictionary<string, ReportParameterInput> _theReportParameterInputs = new Dictionary<string, ReportParameterInput>();

        public QueryParametersUserControl()
        {
            InitializeComponent();
        }

        public void LoadParameters(List<ReportParameter> aReportParameterList, bool isEditable)
        {
            _theReportParameterInputs.Clear();
            ReportParameter startParam = null;
            ReportParameter endParam = null;
            bool showTimeRange = false;
            if (isEditable)
            {
                //find if both start and end time parameters are needed        
                foreach (ReportParameter parameter in aReportParameterList)
                {
                    if (parameter.ParamName.Equals(ReportParameterProcessor.FROM_TIME))
                    {
                        startParam = parameter;
                    }
                    else if (parameter.ParamName.Equals(ReportParameterProcessor.TO_TIME))
                    {
                        endParam = parameter;
                    }
                }
                showTimeRange = (startParam != null) && (endParam != null);

                foreach (ReportParameter parameter in aReportParameterList)
                {
                    if ((((parameter.ParamName.Equals(ReportParameterProcessor.FROM_TIME)) ||
                            (parameter.ParamName.Equals(ReportParameterProcessor.TO_TIME)))
                            &&
                            (showTimeRange))
                          || (parameter.ParamName.Equals(ReportParameterProcessor.CONNECTED_DSN))    //Connected DSN will be populated from the environment 
                          || (parameter.ParamName.Equals(ReportParameterProcessor.LOGGEDON_USER_ID)) //the user id will be populated from the environment 
                        )
                    {
                        //dont show the parameter separately as we will take care of that in the time range or internally
                    }
                    else
                    {
                        ReportParameterInput paramInput = null;
                        if ((parameter.SqlDataType != null) && (parameter.SqlDataType.Equals("java.sql.Timestamp")))
                        {
                            paramInput = new TimeReportParameterInput();
                        }
                        else
                        {
                            paramInput = new DefaultReportParameterInput();
                        }
                        paramInput.setParameter(parameter);

                        //add to panel and to local map
                        addToPanel(paramInput);
                        _theReportParameterInputs[parameter.ParamName] = paramInput;
                    }

                }

                if (showTimeRange)
                {
                    TimeRangeInput rangeInput = new TimeRangeInput();
                    ReportParameter timeRangeKey = new ReportParameter();
                    timeRangeKey.ParamName = ReportParameterProcessor.TIME_RANGE_KEY;
                    ReportParameterProcessor.Instance.prepopulateParam(timeRangeKey);

                    ((ReportParameterInput)rangeInput).setParameter(startParam);
                    ((ReportParameterInput)rangeInput).setParameter(endParam);
                    ((ReportParameterInput)rangeInput).setParameter(timeRangeKey);

                    //add to panel and to local map
                    addToPanel(rangeInput);
                    _theReportParameterInputs[startParam.ParamName] = rangeInput;
                    _theReportParameterInputs[endParam.ParamName] = rangeInput;
                    _theReportParameterInputs[timeRangeKey.ParamName] = rangeInput;
                }
            }
            else
            {
                foreach (ReportParameter parameter in aReportParameterList)
                {
                }
                //for (Iterator i = parameters.iterator();i.hasNext();)
                //{
                //    ReportParameter parameter = (ReportParameter)i.next();
                //    ReportParameterInput paramInput = null;
                //    paramInput = new DisplayonlyReportParameterInput();
                //    paramInput.setParameter(parameter);               

                //    //add to panel and to local map
                //    addToPanel(paramInput, row);
                //    paramMap[parameter] =  paramInput;

                //    row++;
                //}
            }

            this.Refresh();
        }

        //Helper method to add a component dynamically to the UI
        private void addToPanel(ReportParameterInput aReportParameterInput)
        {
            Control theControl = aReportParameterInput as Control;
            if (theControl != null)
            {
                theControl.Dock = System.Windows.Forms.DockStyle.Top;
                theControl.Name = "defaultReportParameterInput" + Controls.Count;
                theControl.TabIndex = Controls.Count;
                Controls.Add(theControl);
            }
        }

    }
}
