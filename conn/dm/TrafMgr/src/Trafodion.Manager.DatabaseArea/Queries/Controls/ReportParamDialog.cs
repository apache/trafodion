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
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Queries;

namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    public partial class ReportParamDialog : Trafodion.Manager.Framework.Controls.TrafodionForm
    {
        public delegate void OnParametersChanged();
        public event OnParametersChanged OnParametersChangedImpl;
        private Hashtable paramMap = new Hashtable();
        int idx = 0;
        int panelHeight = 36;


        public bool IsParametersValid
        {
            get { return okBtn.Enabled; }
        }

        public int PanelHeight
        {
            get { return panelHeight; }
        }

        public ReportParamDialog(List<ReportParameter> parameters, bool editable)
        {
            InitializeComponent();
            populatePanel(_theParametersPanel, parameters, editable);
            ValidateInput();
            StartPosition = FormStartPosition.CenterParent;
        }
        public ReportParamDialog()
        {
            InitializeComponent();
            StartPosition = FormStartPosition.CenterParent;
        }

       //Creates a panel on the fly get the parameters
        public void populatePanel(Trafodion.Manager.Framework.Controls.TrafodionPanel aPanel,List<ReportParameter> parameters, bool editable)
        {
            int row = 0;
            ReportParameter startParam = null;
            ReportParameter endParam = null;
            ReportParameter timeRangeKey = null;
            bool showTimeRange = false;
            if (editable)
            {
                //find if both start and end time parameters are needed        
                foreach (ReportParameter parameter in parameters)
                {
                       if (parameter.ParamName.Equals(ReportParameterProcessor.FROM_TIME))
                      {
                           startParam = parameter;
                      }
                       else if (parameter.ParamName.Equals(ReportParameterProcessor.TO_TIME))
                       {
                           endParam = parameter;
                       }
                       else if (parameter.ParamName.Equals(ReportParameterProcessor.TIME_RANGE_KEY))
                       {
                           timeRangeKey = parameter;
                       }
                }
                showTimeRange = (startParam != null) && (endParam != null);

                foreach (ReportParameter parameter in parameters)
                {
                    if ((((parameter.ParamName.Equals(ReportParameterProcessor.FROM_TIME)) ||
                            (parameter.ParamName.Equals(ReportParameterProcessor.TO_TIME)) ||
                            (parameter.ParamName.Equals(ReportParameterProcessor.TIME_RANGE_KEY))) 
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
                        //Register an event handle for the parameter input control.
                        paramInput.InputChanged += parameters_InputChanged;

                        //add to panel and to local map
                        addToPanel(aPanel, paramInput, row);
                        paramMap[parameter] = paramInput;
                    }            
                     row++;
                }

                if (showTimeRange)
                {
                    TimeRangeInput rangeInput = new TimeRangeInput();
                    if (timeRangeKey == null)
                    {
                        timeRangeKey = new ReportParameter();
                        timeRangeKey.ParamName = ReportParameterProcessor.TIME_RANGE_KEY;
                        ReportParameterProcessor.Instance.prepopulateParam(timeRangeKey);
                    }

                    //For custom range we have to set the range value first as setting the range value after
                    //setting the start and end values will alter their values
                    if ((timeRangeKey.Value != null) &&  (timeRangeKey.Value.Equals(TimeRangeInputBase.CUSTOM_RANGE)))
                    {
                        ((ReportParameterInput)rangeInput).setParameter(timeRangeKey);
                        ((ReportParameterInput)rangeInput).setParameter(startParam);
                        ((ReportParameterInput)rangeInput).setParameter(endParam);
                    }
                    else
                    {
                        ((ReportParameterInput)rangeInput).setParameter(startParam);
                        ((ReportParameterInput)rangeInput).setParameter(endParam);
                        ((ReportParameterInput)rangeInput).setParameter(timeRangeKey);
                    }
                    //Register an event handle for the parameter input control.
                    rangeInput.InputChanged += parameters_InputChanged;

                    //add to panel and to local map
                    addToPanel(aPanel, rangeInput, row);
                    paramMap[startParam]    =  rangeInput;
                    paramMap[endParam] = rangeInput;
                    paramMap[timeRangeKey] = rangeInput;
                    row++;
                }            
            }
            else
            {
                //for (Iterator i = parameters.iterator();i.hasNext();)
                //{
                //    ReportParameter parameter = (ReportParameter)i.next();
                //    ReportParameterInput paramInput = null;
                //    paramInput = new DisplayonlyReportParameterInput();
                //    paramInput.setParameter(parameter);               
                    
                //    //add to panel and to local map
                //    addToPanel(aPanel, paramInput, row);
                //    paramMap[parameter] =  paramInput;

                //    row++;
                //}
            }
            //don't show the cancek button in the non-editable mode. The OK should close the dialog
            cancelBtn.Visible = editable;
            this.ClientSize = new System.Drawing.Size(this.Width, panelHeight);
            this.Refresh();
            
        }
        /// <summary>
        ///  To response user input on the parameters user control
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
       private void parameters_InputChanged(object sender, EventArgs e)
        {
            ValidateInput();
            if (OnParametersChangedImpl != null)
            {
                OnParametersChangedImpl();
            }
        }

        /// <summary>
        /// Validate input
        /// </summary>
        private void ValidateInput()
        {
            foreach (DictionaryEntry de in paramMap)
            {
                if (!((ReportParameterInput)de.Value).IsValidInput())
                {
                    okBtn.Enabled = false;
                    return;
                }
            }
            okBtn.Enabled = true;
        }

        /// <summary>
        /// Un-register event for the parameters controls.
        /// </summary>
        private void ParameterControlDispose()
        {
            foreach (DictionaryEntry de in paramMap)
            {
                ((ReportParameterInput)de.Value).InputChanged-=parameters_InputChanged;
            }
        }

        public void UpdateParamsFromUserInput()
        {
            foreach (ReportParameter key in paramMap.Keys)
            {
                ReportParameterInput paramInput = (ReportParameterInput)paramMap[key];
                key.Value = paramInput.getParamValue(key);
            }
        }
        //Helper method to add a component dynamically to the UI
        private void addToPanel(Trafodion.Manager.Framework.Controls.TrafodionPanel aPanel, ReportParameterInput paramInput, int row)
        {
            const int theTabOrderBaseForParameters = 1000;  // Keep these after the buttons in the tab order
            this.SuspendLayout();

            UserControl paramInputControl = paramInput as UserControl;
            if (paramInputControl != null)
            {
                idx++;

                //paramInputControl.Location = new System.Drawing.Point(0, idx * controlHeight);
                paramInputControl.Name = "defaultReportParameterInput" + idx;
                paramInputControl.Size = new System.Drawing.Size(buttonPanel.Width, paramInputControl.Height+10);
                panelHeight += paramInputControl.Height;
                paramInputControl.TabIndex = theTabOrderBaseForParameters + idx;
                //paramInputControl.Padding = new System.Windows.Forms.Padding(0,0,10,10);

                // By adding the sorted parameters to the bottom of the the panel filling the center,
                // they stay sorted and by using an inner panel, we keep the buttons at the bottom of it all.
                paramInputControl.Dock = System.Windows.Forms.DockStyle.Bottom;
                aPanel.Controls.Add(paramInputControl);   
             
            }
            this.ResumeLayout(false);
        }


        private void okBtn_Click(object sender, EventArgs e)
        {
            UpdateParamsFromUserInput();
        }

        private void cancelBtn_Click(object sender, EventArgs e)
        {

        }
    }
}
