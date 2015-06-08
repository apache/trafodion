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
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.SecurityArea.Model;

namespace Trafodion.Manager.SecurityArea.Controls
{
    public partial class LongOperationStatusDialog : TrafodionForm
    {
        #region Member variables
        private string _theMessage = "";
        #endregion

        #region Constructors
        public LongOperationStatusDialog()
        {
            InitializeComponent();
            CenterToParent();
        }
        public LongOperationStatusDialog(string aCaption, string aMessage) : this()
        {
            this.Text = aCaption;
            _theMessage = aMessage;
            _theStatusBox.Text = _theMessage;
        }
        #endregion

        #region Properties
        public int ProgressMaxValue
        {
            get
            {
                return _theProgressBar.Maximum;
            }
            set
            {
                _theProgressBar.Maximum = value;
            }
        }

        public int ProgressValue
        {
            get
            {
                return _theProgressBar.Value;
            }
            set
            {
                _theProgressBar.Value = value;
            }
        }
        #endregion

        #region Public methods
        public void ShowProgress(string message)
        {
            _theStatusBox.Text = _theMessage + "\n" + message;
            _theProgressBar.Value = _theProgressBar.Value + 1;
        }
        #endregion

        private void LongOperationStatusDialog_FormClosing(object sender, FormClosingEventArgs e)
        {
            //e.Cancel = true;
        }

    }

    public class LongOperationHandler
    {
        LongOperationStatusDialog _theStatusDialog = null;

        public LongOperationStatusDialog StatusDialog
        {
            get { return _theStatusDialog; }
            set { _theStatusDialog = value; }
        }

        public void OnSecurityBackendOperation(object sender, EventArgs e)
        {
            UserEventArgs eventArgs = e as UserEventArgs;
            if (eventArgs != null)
            {
                Console.WriteLine(String.Format("{0} - {1} {2}", eventArgs.Message, eventArgs.Type, eventArgs.EventCount));
                switch (eventArgs.Type)
                {
                    case UserEventArgs.EventType.EventStart:
                        {
                            //show the dialog
                            _theStatusDialog.ProgressMaxValue = eventArgs.EventCount;
                            _theStatusDialog.Show();
                        }
                        break;
                    case UserEventArgs.EventType.EventProgress:
                        {
                            _theStatusDialog.ShowProgress(eventArgs.Operation);
                        }
                        break;
                    case UserEventArgs.EventType.EventEnd:
                        {
                            _theStatusDialog.Close();
                        }
                        break;
                }
            }
        }
    }
}
