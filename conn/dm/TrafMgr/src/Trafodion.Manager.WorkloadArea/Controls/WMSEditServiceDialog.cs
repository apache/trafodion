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
using System.Linq;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.WorkloadArea.Model;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class WMSEditServiceDialog : TrafodionForm
    {
        public WMSEditServiceDialog(WmsCommand.WMS_ACTION operation, WmsSystem wmsSystem, WmsService wmsService)
        {
            InitializeComponent();

            WMSEditServiceUserControl editServicesUserControl = new WMSEditServiceUserControl(operation, wmsSystem, wmsService);
            editServicesUserControl.Dock = DockStyle.Fill;
            Controls.Add(editServicesUserControl);
            //this.Height = editServicesUserControl.Height + 10;
            this.Height = 695;
            if (operation == WmsCommand.WMS_ACTION.ADD_SERVICE)
            {
                Text = "Add Service";
            }
            else if (wmsSystem.ConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_ALTER.ToString()))
            {
                Text = "Alter Service";
                //Enter key to default to OK button
                this.AcceptButton = editServicesUserControl.OKButton;
            }
            else 
            {
                Text = "Service Details";
                editServicesUserControl.OKButton.Visible = false;
                //Enter key to default to Cancel button
                this.AcceptButton = editServicesUserControl.CancelButton;
            }
            this.CancelButton = editServicesUserControl.CancelButton;

           

            CenterToParent();
        }
    }
}
