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
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;

namespace Trafodion.Manager.MetricMiner.Controls
{
    public partial class DrillDownMappingDialog : TrafodionForm
    {
        UniversalWidgetConfig _theConfig;
        DialogResult _theSelectedOption = DialogResult.None;
        AssociatedWidgetConfig _theAssociatedWidgetConfig = null;
        public DialogResult SelectedOption
        {
            get { return _theSelectedOption; }
        }

        public AssociatedWidgetConfig ConfiguredAssociatedWidgetConfig
        {
            get { return _theAssociatedWidgetConfig; }
        }

        public DrillDownMappingDialog()
        {
            InitializeComponent();
            Text = "Link Reports";
            StartPosition = FormStartPosition.CenterParent;
            _theAssociatedWidgetConfig = null;
        }

        public DrillDownMappingDialog(UniversalWidgetConfig aConfig) :this()
        {
            _theConfig = aConfig;
            this._theDrillDownMappingUserControl.Config =  _theConfig;
        }


        private void _theSaveButton_Click(object sender, EventArgs e)
        {
            _theSelectedOption = DialogResult.OK;
            //_theAssociatedWidgetConfig = _theDrillDownMappingUserControl.getAssociatedWidgetConfig();
            if (_theAssociatedWidgetConfig != null)
            {
                _theConfig.AssociatedWidgets.Add(_theAssociatedWidgetConfig);
            }

            this.Hide();
        }

        private void _theCancelButton_Click(object sender, EventArgs e)
        {
            this.Hide();
        }


    }
}
