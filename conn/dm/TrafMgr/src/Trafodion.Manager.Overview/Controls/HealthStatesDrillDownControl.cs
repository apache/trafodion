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
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework;
using System.Data.Odbc;
using Trafodion.Manager.OverviewArea.Models;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.OverviewArea.Controls
{

    public partial class HealthStatesDrillDownControl : TrafodionForm
    {
        TrafodionProgressUserControl _progressUserControl;
        EventHandler<TrafodionProgressCompletedArgs> _progressHandler;
        public string SubLayer { get; set; }
        public bool AllInfo { get; set; }
        public const string LIVEFEED_INFRA_VALUE = "LIVE FEED INFRASTRUCTURE";
        public const string LIVEFEED_INFRA_DISPLAY = "Live Feed Infrastructure";

        public HealthStatesDrillDownControl()
        {
            InitializeComponent();
        }

        public HealthStatesDrillDownControl(string aSubLayer, bool allInfo) : this()
        {
            this.SubLayer = aSubLayer;
            this.AllInfo = allInfo;
            CenterToScreen();
            this.Text += aSubLayer + " Details";
            TrafodionLabelSubjectArea.Text = string.Format(Properties.Resources.HealthStateDrillDownLabel, allInfo ? "All" : "Failure") + " " + aSubLayer;
            InitializeProgressControl(aSubLayer, allInfo);
        }

        private void InitializeProgressControl(string aSubLayer, bool allInfo)
        {
            _theContentPanel.Visible = false;
            _theProgressPanel.Visible = true;

            Object[] parameters = new Object[] { aSubLayer, allInfo };

            string progressMessage = "";
            bool isLiveFeedInfrastructure = aSubLayer.ToUpper().Equals(LIVEFEED_INFRA_VALUE);
            if (isLiveFeedInfrastructure)
            {
                this.SubLayer = aSubLayer.ToUpper();
                progressMessage = AllInfo ? Properties.Resources.LiveFeedInfrastructureHealthCheckProgressMsg_All : Properties.Resources.LiveFeedInfrastructureHealthCheckProgressMsg_Failure;
            }
            else
            {
                if (AllInfo)
                    progressMessage = string.Format(Properties.Resources.HealthStatesDrillDownProgressMessage_All, "All", SubLayer);
                else
                    progressMessage = string.Format(Properties.Resources.HealthStatesDrillDownProgressMessage_Failure, "Failure", SubLayer);
            }


            TrafodionProgressArgs args = new TrafodionProgressArgs(progressMessage, this, "DrillDownInformation", parameters);
            _progressUserControl = new TrafodionProgressUserControl(args);
            if (isLiveFeedInfrastructure)
            {
                _progressUserControl.Height = _progressUserControl.Height + 20;
                _theProgressPanel.Height = _theProgressPanel.Height + 20;
            }

            _progressUserControl.Dock = DockStyle.Top;
            _progressHandler = ProgressCompletedEvent;
            _progressUserControl.ProgressCompletedEvent += _progressHandler;
            _theProgressPanel.Controls.Clear();
            _theProgressPanel.Controls.Add(_progressUserControl);
        }

        public OdbcDataReader DrillDownInformation(string aSubLayer, bool allInfo) 
        {
            
            Connection connection = new Connection(TrafodionContext.Instance.CurrentConnectionDefinition);  
            
            OdbcDataReader reader = Queries.DrillDownHealthStateInfo(connection, SubLayer, allInfo);
            
            return reader;
        }

        private void ProgressCompletedEvent(object sender, TrafodionProgressCompletedArgs e)
        {

            if (e.Error != null)
            {
                MessageBox.Show(e.Error.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                _theProgressPanel.Controls.Clear();
            }
            else
            {                
                OdbcDataReader reader = (OdbcDataReader)_progressUserControl.ReturnValue;
                DisplayDrillDownDetails(reader);
                CompleteDrillDown();
            }
        }

        private void CompleteDrillDown() 
        {
            _theProgressPanel.Visible = false;
            _theProgressPanel.Controls.Clear();
            _progressUserControl.ProgressCompletedEvent -= ProgressCompletedEvent;            
            
            _theContentPanel.Visible = true;
            
        }

        private void DisplayDrillDownDetails(OdbcDataReader reader) 
        {
            try
            {
                DataTable result = GetResultDataTableFromReader(reader);

                _theGrid.BeginUpdate();                
                _theGrid.FillWithData(result);
                if (_theGrid.Cols.Count ==2)
                {
                    _theGrid.Cols[0].CellStyle.TextFormatFlags = iGStringFormatFlags.WordWrap;
                    _theGrid.Cols[1].CellStyle.TextFormatFlags = iGStringFormatFlags.WordWrap;
                }

                _theGrid.Cols.AutoWidth();
                _theGrid.Rows.AutoHeight();
                _theGrid.SelectionMode = iGSelectionMode.MultiExtended;
                _theGrid.EndUpdate();
                //Make the iGrid Not Sortable
                foreach (iGRow row in _theGrid.Rows)
                {
                    row.Sortable = false;
                }
                _theGrid.Visible = true;

                if (_theGrid.Rows.Count == 0)
                    TrafodionLabelSubjectArea.Text = string.Format(Properties.Resources.HealthStateDrillDownNoFailure, SubLayer);
            }
            catch (Exception ex)
            {
                if (!reader.IsClosed)
                    reader.Close();
                _theGrid.Visible = false;

                OdbcDataReader newreader=DrillDownInformation(SubLayer, AllInfo);
                string result = GetResultStringForReader(newreader);

                TrafodionTextBox tbxDetails = new TrafodionTextBox();
                tbxDetails.Name = "DrillDownDetails";
                tbxDetails.Text = result;
                
                tbxDetails.Dock = DockStyle.Fill;
                tbxDetails.Enabled = false;
                tbxDetails.Multiline = true;

                //Change Dock priority of control by adding them intern, the latter one has higher priority
                _theContentPanel.Controls.Remove(TrafodionPanel1);
                _theContentPanel.Controls.Add(tbxDetails);
                _theContentPanel.Controls.Add(TrafodionPanel1);
            }
            
        }

        private string GetResultStringForReader(OdbcDataReader reader) 
        {
            StringBuilder temp = new StringBuilder();
            while (reader.Read()) 
            {
                for (int colNum = 0; colNum < reader.FieldCount; colNum++) 
                {
                    temp.Append(reader.GetValue(colNum));
                }
                temp.AppendLine();
            }
            return temp.ToString();
        }

        private DataTable GetResultDataTableFromReader(OdbcDataReader reader)
        {
            DataTable resultDT = new DataTable();
            resultDT.Columns.Add("Name", typeof(System.String));
            resultDT.Columns.Add("Value", typeof(System.String));

            //add temp variable to store name and value
            string fieldName=string.Empty;
            string fieldValue=string.Empty;
            int iSpitterPos = 0;
            //The reader only contains one column, which contains key/value pair
            bool isFirstLine = true;//the first line should not have blank spitter row.
            while (reader.Read())
            {
                string val = (string)reader.GetValue(0);
                if (val.Equals(""))
                {
                    resultDT.Rows.Add(new object[] { string.Empty, string.Empty });
                }
                else
                {
                    if (val.ToLower().Contains("rownum"))
                    {
                        if (!isFirstLine)
                        {
                            resultDT.Rows.Add(new object[] { string.Empty, string.Empty });
                        }
                    }
                    else
                    {
                        fieldName=string.Empty;
                        fieldValue=string.Empty;
                        iSpitterPos = val.IndexOf(':');
                        fieldName = val.Substring(0, iSpitterPos);
                        fieldValue = val.Substring(iSpitterPos+1);
                        resultDT.Rows.Add(new object[] { fieldName.Trim(), fieldValue.Trim() });
                    }
                }
                isFirstLine = false;
            }  
            reader.Close();
            return resultDT;
        }

        private void _helpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.HealthStateDetails);
        } 
    }
}
