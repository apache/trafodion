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
using System.Windows.Forms;
using System.Drawing;

namespace Trafodion.Manager.Framework.Controls
{
    /// <summary>
    ///  Progress control that executes the user specified method in background
    /// </summary>
    public partial class TrafodionProgressUserControl : UserControl
    {
        TrafodionProgressHelper _progressHelper;
        public event EventHandler<TrafodionProgressCompletedArgs> ProgressCompletedEvent;

        public TrafodionProgressUserControl()
        {
            InitializeComponent();
        }

        public TrafodionProgressUserControl(TrafodionProgressArgs aProgressArgs)
        {
            InitializeComponent();
            _progressTextLabel.Text = aProgressArgs.Text;
            _progressHelper = new TrafodionProgressHelper(aProgressArgs);
            _progressHelper.ProgressCompletedEvent += _progressHelper_ProgressCompletedEvent;
        }

        void _progressHelper_ProgressCompletedEvent(object sender, TrafodionProgressCompletedArgs e)
        {
            OnProgressCompletedEvent(e);
        }

        /// <summary>
        /// Raise the model changed event
        /// </summary>
        protected virtual void OnProgressCompletedEvent(TrafodionProgressCompletedArgs completedEvent)
        {
            EventHandler<TrafodionProgressCompletedArgs> handler = ProgressCompletedEvent;
            if (handler != null)
            {
                handler(this, completedEvent);
            }
        }
        
        /// <summary>
        /// Return value of the method that is invoked in the background
        /// </summary>
        public Object ReturnValue
        {
            get 
            {
                if (_progressHelper != null)
                {
                    return _progressHelper.ReturnValue;
                }
                return null;
            }
        }

        /// <summary>
        /// Exception if any from the background method invocation
        /// </summary>
        public Exception Error
        {
            get
            {
                if (_progressHelper != null)
                {
                    return _progressHelper.Error;
                }
                return null;
            }
        }

        public void AdaptCompactSize()
        {
            this._progressTextLabel.Location = new Point(0, 0);
            this._progressBar.Location = new Point(0, 0);
            this.TrafodionPanel1.Size = new Size(193, 20);
            this.TrafodionPanel1.Margin = new Padding(0);
            this.TrafodionPanel2.Margin = new Padding(0);
        }

        private void MyDispose(bool disposing)
        {
            if (_progressHelper != null)
            {
                _progressHelper.ProgressCompletedEvent -= _progressHelper_ProgressCompletedEvent;
                _progressHelper.Cancel();
            }
        }
    }
}
