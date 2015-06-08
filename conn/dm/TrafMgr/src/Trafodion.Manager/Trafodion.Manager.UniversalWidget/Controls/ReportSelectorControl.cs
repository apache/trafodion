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
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Queries;
using Trafodion.Manager.DatabaseArea.Queries.Controls;
namespace Trafodion.Manager.UniversalWidget.Controls
{
    public partial class ReportSelectorControl : UserControl
    {

        WidgetLinkerObject _theWidgetLinker;
        UniversalWidgetConfig _theConfig;
        List<ReportParameter> reportParams = null;
        ReportParamDialog paramDialog = null;
        int initialWidth = 550;
        int initialHeight = 140;
        public ReportSelectorControl()
        {
            InitializeComponent();
        }
        
        public void PopulateUI(UniversalWidgetConfig aConfig, WidgetLinkerObject aWidgetLinkerObject)
        {
            _theWidgetLinker = aWidgetLinkerObject;
            _theConfig = aConfig;
            populateLinkedReports();            
        }
        
        public AssociatedWidgetConfig GetSelectedReport()
        {
            return _theWidgetMappingListBox.SelectedItem as AssociatedWidgetConfig;
        }

        public List<ReportParameter> GetUpdatedParameters()
        {
            return reportParams;
        }

        public void UpdateParamsFromUserInput()
        {
            paramDialog.UpdateParamsFromUserInput();
        }

        private void populateLinkedReports()
        {
            this._theWidgetMappingListBox.Items.Clear();
            foreach (AssociatedWidgetConfig associatedWidget in _theConfig.AssociatedWidgets)
            {
                this._theWidgetMappingListBox.Items.Add(associatedWidget);
            }
            this._theWidgetMappingListBox.SelectedIndex = 0;
        }

        private void DisplayParameters(List<ReportParameter> parameters)
        {
            _theParameterInputPanel.Controls.Clear();
            paramDialog = new ReportParamDialog();
            paramDialog.populatePanel(_theParameterInputPanel, parameters, true);
            this.ParentForm.ClientSize = new Size(initialWidth, initialHeight + paramDialog.PanelHeight);
        }

        private void _theWidgetMappingListBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            AssociatedWidgetConfig associatedConfig = _theWidgetMappingListBox.SelectedItem as AssociatedWidgetConfig;
            if (associatedConfig != null)
            {
                if (_theConfig != null)
                {
                    UniversalWidgetConfig calledWidgetConfig = WidgetRegistry.GetInstance().GetUniversalWidgetConfig(associatedConfig.CalledWidgetName);
                    if ((calledWidgetConfig != null) && (calledWidgetConfig.DataProviderConfig is DatabaseDataProviderConfig))
                    {
                        DatabaseDataProviderConfig dbConfig = calledWidgetConfig.DataProviderConfig as DatabaseDataProviderConfig;

                        //Set the called report name based on the user selection
                        _theWidgetLinker.CalledWidget = associatedConfig.CalledWidgetName;

                        //Create a temporary report def object so that we can parse the parameters
                        SimpleReportDefinition reportDef = new SimpleReportDefinition(_theConfig.Name);
                        reportDef.QueryText = dbConfig.SQLText;
                        
                        //Use the report param processor to parse the SQL text to get the list of parameters
                        ReportParameterProcessor paramProcessor = ReportParameterProcessor.Instance;
                        reportParams =  paramProcessor.getReportParams(reportDef);

                        //Get the values that will be mapped to the parameters based on the parameter mapping
                        Hashtable mappedParams = _theWidgetLinker.MappedParameters;

                        //Populate the parameters with the value returned by the row selected by the user
                        paramProcessor.populateDefinedParameters(reportParams, mappedParams);

                        //Display the populated parameters to the user
                        DisplayParameters(reportParams);
                    }
                }
            }
        }

        private void _theAddButton_Click(object sender, EventArgs e)
        {

        }
    }
}
