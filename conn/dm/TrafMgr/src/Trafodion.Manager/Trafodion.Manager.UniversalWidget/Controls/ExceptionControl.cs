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
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace Trafodion.Manager.UniversalWidget.Controls
{
    public partial class ExceptionControl : UserControl
    {
        DataProvider _theDataProvider = null;
        public delegate void UpdateStatus(Object obj, EventArgs e);
        public DataProvider DataProvider
        {
            get { return _theDataProvider; }
            set 
            {
                if (_theDataProvider != null)
                {
                    RemoveHandlers();
                }
                _theDataProvider = value;
                AddHandlers();
            }
        }


        public ExceptionControl()
        {
            InitializeComponent();
            this._theExceptionText.Multiline = true;
            this._theExceptionText.WordWrap = false;
            this._theExceptionText.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theExceptionText.Font = new System.Drawing.Font("Courier New", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));

        }

        private void AddHandlers()
        {
            if (_theDataProvider != null)
            {
                //Associate the event handlers
                _theDataProvider.OnErrorEncountered += InvokeHandleError;
                _theDataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;
                _theDataProvider.OnFetchingData += InvokeHandleFetchingData;
            }
        }
        private void RemoveHandlers()
        {
            if (_theDataProvider != null)
            {
                //Remove the event handlers
                _theDataProvider.OnErrorEncountered -= InvokeHandleError;
                _theDataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
                _theDataProvider.OnFetchingData -= InvokeHandleFetchingData;
            }
        }
        //Do the dispose
        private void MyDispose(bool disposing)
        {
            if (disposing)
            {
                //Remove the event handlers
                this.RemoveHandlers();
            }
        }
        private void InvokeHandleError(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new UpdateStatus(HandleError), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.UW,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }
        private void HandleError(Object obj, EventArgs e)
        {
            DataProviderEventArgs evtArgs = (DataProviderEventArgs)e;
            if(!this._theExceptionText.IsDisposed)
            {
                this._theExceptionText.Text = evtArgs.Exception.Message;
            }
            

        }

        private void InvokeHandleNewDataArrived(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new UpdateStatus(HandleNewDataArrived), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.UW,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        private void HandleNewDataArrived(Object obj, EventArgs e)
        {
            //Clear the errors
            if (!this._theExceptionText.IsDisposed)
            {
                this._theExceptionText.Text = "";
            }
            
        }


        private void InvokeHandleFetchingData(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new UpdateStatus(HandleFetchingData), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.UW,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }


        private void HandleFetchingData(Object obj, EventArgs e)
        {
            //Do Nothing
        }


    }
}
