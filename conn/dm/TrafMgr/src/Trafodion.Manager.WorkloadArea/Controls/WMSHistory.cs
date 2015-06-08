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
using System.Collections;
using System.Windows.Forms;
using Trafodion.Manager.WorkloadArea.Model;
using TenTec.Windows.iGridLib;
using Trafodion.Manager.Framework;
namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class WMSHistory : Trafodion.Manager.Framework.Controls.TrafodionForm
    {
        #region Members
        private Form m_parent = null;
        private ArrayList m_commands = null;
        private string m_command = null;
        private int m_rowIndex = -1;
        #endregion

        #region Properties 
        public string WMSCommand
        {
            get { return m_command; }
        }
        #endregion

        public WMSHistory(Form parent, ref ArrayList commands)
        {
            InitializeComponent();
            m_parent = parent;
            m_commands = commands;
            historyIGrid.AutoWidthColMode = iGAutoWidthColMode.HeaderAndCells;
            populateHistory();
            //this.historyIGrid.SelectionChanged += new EventHandler(iGridHistory_SelectionChanged);
            this.historyIGrid.CellClick += new iGCellClickEventHandler(historyIGrid_SelectionChanged);

            CenterToParent();
        }

        void historyIGrid_SelectionChanged(object sender, iGCellClickEventArgs e)
        {
            m_rowIndex = e.RowIndex;
            this.okButton.Enabled = (m_rowIndex >= 0) ? true : false;
        }

        private void populateHistory()
        {
            historyIGrid.Rows.Clear();
            historyIGrid.Cols.Clear();
            iGColPattern intColPattern = new iGColPattern();
            intColPattern.CellStyle.ValueType = System.Type.GetType("System.Int16");
            intColPattern.CellStyle.TextAlign = iGContentAlignment.MiddleRight;
            iGColPattern stringColPattern = new iGColPattern();
            stringColPattern.CellStyle.ValueType = System.Type.GetType("System.String");
            stringColPattern.CellStyle.TextAlign = iGContentAlignment.MiddleLeft;
            historyIGrid.Cols.Add("NUMBER", 80, intColPattern);
            historyIGrid.Cols.Add("TYPE", 80, stringColPattern);
            historyIGrid.Cols.Add("COMMAND", 250, stringColPattern);
            historyIGrid.Cols.Add("EXECUTED TIME", 150, stringColPattern);

            try
            {
                int size = m_commands.Count;
                for (int i = 0; i < size; i++)
                {
                    string sqlCommand = ((WMSHistoryModel)m_commands[i]).SQLCommand;
                    historyIGrid.Rows.Add();
                    historyIGrid.Cells[i, 0].Value = ((WMSHistoryModel)m_commands[i]).Number + 1;
                    historyIGrid.Cells[i, 1].Value = ((WMSHistoryModel)m_commands[i]).SQLType;
                    historyIGrid.Cells[i, 2].Value = sqlCommand.Replace(Environment.NewLine, "");
                    DateTime dt = ((WMSHistoryModel)m_commands[i]).ExecutedTime;
                    dt = dt.ToLocalTime();
                    string timeStamp = String.Format("{0:0000}-{1:00}-{2:00} {3:00}:{4:00}:{5:00}",
                                              dt.Year, dt.Month, dt.Day, dt.Hour, dt.Minute, dt.Second);
                    historyIGrid.Cells[i, 3].Value = timeStamp;
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show("Exception: " + ex.Message, Properties.Resources.SystemOffender, MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            finally
            {
                historyIGrid.Cols.AutoWidth();
            }
        }

        private void buttonOK_Click(object sender, EventArgs e)
        {
            iGRow row = historyIGrid.Rows[m_rowIndex];
            iGRowCellCollection coll = row.Cells;
            this.m_command = (string)coll[2].Value;
            DialogResult = DialogResult.OK;
            this.Close();
        }

        private void buttonRefresh_Click(object sender, EventArgs e)
        {
            this.okButton.Enabled = false;
            populateHistory();
        }

        private void clearButton_Click(object sender, EventArgs e)
        {
            m_commands.Clear();
            this.historyIGrid.Rows.Clear();
            this.okButton.Enabled = false;
        }

        private void cancelButton_Click(object sender, EventArgs e)
        {
            Close();
        }

        private void helpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.OffenderHistory);
        }

    }
}
