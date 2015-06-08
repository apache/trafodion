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
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    public partial class MVSampledStatsSummaryControl : UserControl
    {
        private string _columnName = "";

        /// <summary>
        /// Constructs the summary control
        /// </summary>
        public MVSampledStatsSummaryControl()
        {
            InitializeComponent();
        }

        /// <summary>
        /// Initializes the summary control
        /// </summary>
        /// <param name="sqlMxMVColumn"></param>
        public void InitializeControl(TrafodionMaterializedViewColumn sqlMxMVColumn)
        {
            _sampleTimeStamp.Text = Properties.Resources.SampleProgress;
            _columnNameLabel.Text = Properties.Resources.ColumnName + " : " + sqlMxMVColumn.ExternalName;
            _columnName = sqlMxMVColumn.TrafodionMaterializedView.VisibleAnsiName;

            headerLabel.Text = String.Format(Properties.Resources.SampledMVName, _columnName);

            _summaryDataGridView.Columns.Clear();
            _summaryDataGridView.Rows.Clear();
            _summaryDataGridView.Columns.Add("Datatype", Properties.Resources.DataType);
            _summaryDataGridView.Columns.Add("NumberOfNulls", Properties.Resources.NumberOfNulls);
            _summaryDataGridView.Columns.Add("Skew", Properties.Resources.Skew);
            _summaryDataGridView.Columns[1].DefaultCellStyle.Format = "N0"; //#nulls
            _summaryDataGridView.Columns[2].DefaultCellStyle.Format = "N2"; //skew
            _progressBar.Visible = true;
        }

        /// <summary>
        /// Populates the summary details from the sample
        /// </summary>
        /// <param name="sqlMxMVColumn"></param>
        public void Populate(TrafodionMaterializedViewColumn sqlMxMVColumn)
        {
            _progressBar.Visible = false;
            _sampleTimeStamp.Text = string.Format(Properties.Resources.SampledAtTimeStamp,
                Trafodion.Manager.Framework.Utilities.CurrentFormattedDateTime);

            _summaryDataGridView.Rows.Add(new object[] {
                            sqlMxMVColumn.FormattedDataType(),
                            sqlMxMVColumn.SampledStatistics.NumberOfNulls, 
                            sqlMxMVColumn.SampledStatistics.Skew
                        });

            //In the header lable, display the sample percent and the source MV from which the sample was taken
            headerLabel.Text = String.Format(Properties.Resources.SampledPercentMVText,
                sqlMxMVColumn.SampledStatistics.FormattedSampledPercent,
                sqlMxMVColumn.TrafodionSchemaObject.VisibleAnsiName);

        }

        /// <summary>
        /// Resets the summary status if an error occurs
        /// </summary>
        public void SetStatusOnError()
        {
            _progressBar.Visible = false;
            headerLabel.Text = String.Format(Properties.Resources.SampledMVName, _columnName);
            _sampleTimeStamp.Text = Properties.Resources.SamplingCancelled;
        }
    }
}
