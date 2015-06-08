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
using Trafodion.Manager.Framework.Queries;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.UniversalWidget.Controls
{
    public partial class OutputControl : UserControl
    {
        DataProvider _theDataProvider = null;
        public delegate void UpdateStatus(Object obj, EventArgs e);
        //We might have to keep the actual messages to take care of color customization 
        //based on message types. For now we will stick to red for error and the rest are 
        //black
        //private Stack<MessageItem> _theMessageStack = new Stack<MessageItem>();
        private string header = "";
        private string footer = "";
        private string body = "";

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


        public OutputControl()
        {
            InitializeComponent();

            this._theOutputText.Multiline = true;
            this._theOutputText.WordWrap = false;
            this._theOutputText.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theOutputText.Font = new System.Drawing.Font("Courier New", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            createDefaultRtf();
            _theOutputText.Rtf = getRtf();
        }

        public void LogData(object sender, LogEventArgs e)
        {
            StringBuilder sb = new StringBuilder();
            AddToSbWIthTime(sb, e.Message);
            sb.AppendLine(this._theOutputText.Text);
            this._theOutputText.Text = sb.ToString();
        }

        private void AddHandlers()
        {
            if (_theDataProvider != null)
            {
                //Associate the event handlers
                _theDataProvider.OnErrorEncountered += InvokeHandleError;
                _theDataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;
                _theDataProvider.OnFetchingData += InvokeHandleFetchingData;
                _theDataProvider.OnFetchCancelled += InvokeHandleCancel;
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
                _theDataProvider.OnFetchCancelled -= InvokeHandleCancel;
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

        private void InvokeHandleCancel(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new UpdateStatus(HandleCancel), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.UW,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }
        private void HandleCancel(Object obj, EventArgs e)
        {
            StringBuilder sb = new StringBuilder();
            AddToSbWIthTime(sb, "Fetch Cancelled. ");

            DataProviderEventArgs evtArgs = (DataProviderEventArgs)e;
            if (evtArgs.Exception != null)
            {
                sb.AppendLine(evtArgs.Exception.Message);
            }
            //_theMessageStack.Push(new MessageItem(MessageItem.Messagetype.error, sb.ToString()));
            this._theOutputText.Rtf = getRtf(new MessageItem(MessageItem.Messagetype.error, sb.ToString()));
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
            StringBuilder sb = new StringBuilder();
            AddToSbWIthTime(sb, "Error Encountered: Exception Message ");

            DataProviderEventArgs evtArgs = (DataProviderEventArgs)e;
            if (evtArgs.Exception != null)
            {
                sb.AppendLine(evtArgs.Exception.Message);
            }
            //_theMessageStack.Push(new MessageItem(MessageItem.Messagetype.error, sb.ToString()));
            this._theOutputText.Rtf = getRtf(new MessageItem(MessageItem.Messagetype.error, sb.ToString()));
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
            StringBuilder sb = new StringBuilder();
            DataProviderEventArgs evtArgs = (DataProviderEventArgs)e;
            long elapsedTicks = 0;
            if (evtArgs.GetEventProperty("elapsed_time") != null)
            {
                elapsedTicks = (long)evtArgs.GetEventProperty("elapsed_time");
            }

            if (elapsedTicks > 0)
            {
                TimeSpan elapsedSpan = new TimeSpan(elapsedTicks);
                AddToSbWIthTime(sb, string.Format("Fetch Successful...Time taken {0:N2} secs", elapsedSpan.TotalSeconds));
            }
            else
            {
                AddToSbWIthTime(sb, "Fetch Successful");
            }
            //_theMessageStack.Push(new MessageItem(MessageItem.Messagetype.info, sb.ToString()));
            this._theOutputText.Rtf = getRtf(new MessageItem(MessageItem.Messagetype.info, sb.ToString()));
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
            StringBuilder sb = new StringBuilder();
            AddToSbWIthTime(sb, "Fetching Data Using Query....");
            DataProviderEventArgs evtArgs = (DataProviderEventArgs)e;
            string query = evtArgs.GetEventProperty("query_text") as string;
            if (!string.IsNullOrEmpty(query))
            {
                sb.Append(query);
            }
            //SimpleReportDefinition reportDef = evtArgs.GetEventProperty("report_definition") as SimpleReportDefinition;
            //if (reportDef != null)
            //{
            //    string query = reportDef.GetProperty(ReportDefinition.ACTUAL_QUERY) as String;
            //    sb.AppendLine(query);
            //}

            this._theOutputText.Rtf = getRtf(new MessageItem(MessageItem.Messagetype.info, sb.ToString()));
        }

        private void AddToSbWIthTime(StringBuilder sb, string message)
        {
            sb.Append(Utilities.CurrentFormattedDateTime);
            sb.Append(" : ");
            sb.AppendLine(message);

        }

        private void _theClearButton_Click(object sender, EventArgs e)
        {
            //_theMessageStack.Clear();
            body = "";
            _theOutputText.Rtf = getRtf();

        }

        private string getRtf()
        {
            return getRtf(null);
        }
        /// <summary>
        /// Returns the RTF for the messages being displayed
        /// </summary>
        /// <param name="messageItem"></param>
        /// <returns></returns>
        private string getRtf(MessageItem messageItem)
        {
            StringBuilder sb = new StringBuilder();
            sb.AppendLine(header);
            if (messageItem != null)
            {
                //create the body of the message
                StringBuilder sbBody = new StringBuilder();
                if (messageItem.TheMessageType == MessageItem.Messagetype.error)
                {
                    sbBody.Append(" \\pard\\f0\\cf1\\fs17 ");
                }
                else
                {
                    sbBody.Append(" \\pard\\f0\\cf0\\fs17 ");
                }
                sbBody.Append(messageItem.Message);
                sbBody.AppendLine(" \\par ");
                //We don't want to keep prepending to the existing messages.                
                //sbBody.AppendLine(body);

                //update the body
                body = sbBody.ToString();
                sb.AppendLine(body);
            }
            sb.AppendLine(footer);
            return sb.ToString();
        }

        /// <summary>
        /// Creates the pieces that we need over and over again
        /// </summary>
        private void createDefaultRtf()
        {
            StringBuilder sb = new StringBuilder();
            sb.AppendLine("{\\rtf1\\ansi\\ansicpg1252\\deff0\\deflang1033");
            sb.AppendLine("{\\fonttbl {\\f0\\fnil\\fcharset1 Courier New;}}");
            sb.AppendLine("{\\colortbl ;\\red255\\green0\\blue0;\\red0\\green0\\blue0;}");
            sb.AppendLine("{\\viewkind4\\uc1\\pard\\f0\\fs24");

            header = sb.ToString();

            footer = " }";
        }

    }

    public class MessageItem
    {
        public enum Messagetype { trace, info, warning, error };
        private string _theMessage;

        public string Message
        {
            get { return _theMessage; }
            set { _theMessage = value; }
        }
        private Messagetype _theMessageType;

        public Messagetype TheMessageType
        {
            get { return _theMessageType; }
            set { _theMessageType = value; }
        }
        public MessageItem(Messagetype aMessagetype, string aMessage)
        {
            _theMessageType = aMessagetype;
            _theMessage = aMessage;
        }
    }
}
