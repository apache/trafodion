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
using System.ComponentModel;
using System.Reflection;

namespace Trafodion.Manager.Framework.Controls
{
    /// <summary>
    /// Progress dialog that executes the user specified method in background
    /// </summary>
    public partial class TrafodionProgressDialog : TrafodionForm
    {
        TrafodionProgressUserControl _progressControl;

        /// <summary>
        /// Constructs the progress dialog
        /// </summary>
        /// <param name="aProgressArgs">Specifies the arguments for the background method</param>
        public TrafodionProgressDialog(TrafodionProgressArgs aProgressArgs)
        {
            InitializeComponent();
            _progressControl = new TrafodionProgressUserControl(aProgressArgs);
            _progressControl.ProgressCompletedEvent += _progressControl_ProgressCompletedEvent;
            _progressControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this.Controls.Add(_progressControl);
            this.ControlBox = false;
            Text = "Progress Dialog";
            CenterToParent();
        }

        void _progressControl_ProgressCompletedEvent(object sender, TrafodionProgressCompletedArgs e)
        {
            Close();
        }

        /// <summary>
        /// Return value of the method that is invoked in the background
        /// </summary>
        public Object ReturnValue
        {
            get { return _progressControl.ReturnValue; }
        }

        /// <summary>
        /// Exception if any from the background method invocation
        /// </summary>
        public Exception Error
        {
            get { return _progressControl.Error; }
        }

        /// <summary>
        /// Cancel any background work if any
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void ProgressDialog_FormClosing(object sender, System.Windows.Forms.FormClosingEventArgs e)
        {
            if (_progressControl != null)
            {
                _progressControl.ProgressCompletedEvent -= _progressControl_ProgressCompletedEvent;
            }
        }
    }
}
