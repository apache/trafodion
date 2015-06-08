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
using System.Text;

namespace Trafodion.Manager.UniversalWidget
{
    /// <summary>
    /// Class to allow default logging in the universal widget
    /// </summary>
    public class DefaultLogger : ILogger
    {
        public delegate void LogData(object sender, LogEventArgs e);
        public event LogData OnLogData;

       /// <summary>
       /// Will log message at info level
       /// </summary>
       /// <param name="sender"></param>
       /// <param name="aMessage"></param>
        public virtual void Log(object sender, string aMessage)
        {
            LogEventArgs eventArgs = new LogEventArgs();
            eventArgs.Message = aMessage;
            eventArgs.Level = LogEventArgs.LogLevel.Info;
            if (OnLogData != null)
            {
                OnLogData(sender, eventArgs);
            }
        }

        /// <summary>
        /// Pass all the params required to log a message
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="aLevel"></param>
        /// <param name="aSubSystem"></param>
        /// <param name="aMessage"></param>
        public virtual void Log(object sender, LogEventArgs.LogLevel aLevel, string aSubSystem, string aMessage)
        {
            LogEventArgs eventArgs = new LogEventArgs();
            eventArgs.Level = aLevel;
            eventArgs.SubSystem = aSubSystem;
            eventArgs.Message = aMessage;
            if (OnLogData != null)
            {
                OnLogData(sender, eventArgs);
            }
        }
    }

    /// <summary>
    /// Log event parameters
    /// </summary>
    public class LogEventArgs: EventArgs
    {
        public enum LogLevel { Trace, Info, Warning, Error };
        public LogEventArgs()
        {
        }

        private LogLevel _theLogLevel;
        private string _theSubSystem;
        private string _theMessage;

        public LogLevel Level
        {
            get { return _theLogLevel; }
            set { _theLogLevel = value; }
        }

        public string SubSystem
        {
            get { return _theSubSystem; }
            set { _theSubSystem = value; }
        }

        public string Message
        {
            get { return _theMessage; }
            set { _theMessage = value; }
        }

    }
}
