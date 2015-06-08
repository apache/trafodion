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
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    public partial class MVSampledStatsFreqValuesControl : UserControl
    {
        public MVSampledStatsFreqValuesControl()
        {
            InitializeComponent();
            //Export buttons to the datagridviews
            _frequentValuesDataGridView.AddButtonControlToParent(DockStyle.Bottom);
        }

        public void InitializeControl()
        {
            _frequentValuesDataGridView.Columns.Clear();
            _frequentValuesDataGridView.Rows.Clear();
            _frequentValuesDataGridView.Columns.Add("Value", Properties.Resources.Value);
            _frequentValuesDataGridView.Columns.Add("Cardinality", Properties.Resources.Cardinality);
            _frequentValuesDataGridView.Columns[1].DefaultCellStyle.Format = "N0"; //Cardinality
        }

        public void Populate(TrafodionMaterializedViewColumn sqlMxMVColumn)
        {
            List<MVColumnSampledStats.FrequentValue> frequentValues = sqlMxMVColumn.SampledStatistics.FrequentValues;
            if (frequentValues != null)
            {
                foreach (MVColumnSampledStats.FrequentValue frequentValue in frequentValues)
                {
                    _frequentValuesDataGridView.Rows.Add(new object[] {
                        frequentValue.ColumnValue,
                        frequentValue.Count
                        });
                }
            }
        }
    }
}
