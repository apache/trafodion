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
    public partial class WMSCommandStopConfirmationDialog : TrafodionForm
    {
        #region  Constructors
        public WMSCommandStopConfirmationDialog()
        {
            InitializeComponent();
            CenterToScreen();
        }

        /*
         *  Constructor to create WMS stop confirmation dialog.
         */
        public WMSCommandStopConfirmationDialog(bool supportsImmediateStop) {
            InitializeComponent();

            this.Text = Properties.Resources.StopWMS;
            cmdStopConfirmationHeadingLabel.Text = Properties.Resources.ConfirmStopWMS; ;

            this.immediateStopCheckBox.Visible = supportsImmediateStop;
            this.systemServicesWarningLabel.Visible = this.systemServicesWarningPanel.Visible = false;
            this.notAllServicesActiveLabel.Visible  = this.notAllServicesPanel.Visible = false;

            String immediateStopToolTip = "Stopping the WMS environment will gracefully stop the " +
                                          "environment - allowing all executing queries to complete.\n" +
                                          "The \"Stop Immediately\" option brings down down the environment " +
                                          "immediately and all executing queries are cancelled. \n" +
                                          "Please note that in either case, all waiting queries will be rejected " +
                                          "and no new query requests will be allowed into the environment.";

            this.nccWMSCommandStopToolTip.SetToolTip(this.immediateStopCheckBox, immediateStopToolTip);
            this.nccWMSCommandStopToolTip.SetToolTip(this.okButton, "Stop the WMS environment");

            servicesTextBox.Visible = listPanel.Visible = false;
            this.Height -= (listPanel.Height + notAllServicesPanel.Height + systemServicesWarningPanel.Height);
            CenterToScreen();
        }  /*  End of  NCCWMSCommandStopConfirmationDialog(bool)  constructor.  */


        /*
         *  Constructor to create a services stop confirmation dialog.
         */
        public WMSCommandStopConfirmationDialog(List<WmsService> wmsServices, bool supportsImmediateStop,
                                                   bool showSysServicesWarning, bool showNotAllActiveWarning) {
            InitializeComponent();

            this.immediateStopCheckBox.Visible = supportsImmediateStop;
            this.systemServicesWarningLabel.Visible = this.systemServicesWarningPanel.Visible = showSysServicesWarning;
            this.notAllServicesActiveLabel.Visible = this.notAllServicesPanel.Visible = showNotAllActiveWarning;

            addServiceNamesToDisplay(wmsServices);
            if (!showNotAllActiveWarning)
                this.Height -= notAllServicesPanel.Height;
            if (!showSysServicesWarning)
                this.Height -= systemServicesWarningPanel.Height;
            CenterToScreen();

        }  /*  End of  NCCWMSCommandStopConfirmationDialog(List, bool, bool)  constructor.  */


        #endregion  /*  End of  region  Constructors.  */



        #region  Properties

        /*
         *  Property indicates whether or not the service(s) should be stopped immediately.
         *  Option that the user selected in this stop service(s) confirmation dialog.
         */
        public bool StopImmediately {
            get { return this.immediateStopCheckBox.Checked; }

        }  /*  End of  StopImmediately  Property.  */


        #endregion  /*  End of  region  Properties.  */



        #region  Private Methods

        /*
         *  Adds services names to the list displayed in the WMS services stop confirmation dialog. 
         */
        private void addServiceNamesToDisplay(List<WmsService> wmsServices) {
            StringBuilder  sb = new StringBuilder(); 
            foreach (WmsService aService in wmsServices)
                sb.Append(aService.Name).Append(Environment.NewLine);

            servicesTextBox.Text = sb.ToString();
            servicesTextBox.Select(0, 0);

        }  /*  End of  addServiceNamesToDisplay  method.  */



        /*
         *  Handle ok button click. 
         */
        private void okButton_Click(object sender, EventArgs e) {
            this.DialogResult = DialogResult.Yes;
            this.Close();

        }

        private void WMSCommandStopConfirmationDialog_Load(object sender, EventArgs e)
        {

        }  /*  End of  okButton_Click  method.  */


        #endregion  /*  End of  region  Private Methods.  */
    }
}
