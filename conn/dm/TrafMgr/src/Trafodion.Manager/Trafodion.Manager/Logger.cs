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
using System.IO;
using System.Text;

// Configure logging for this assembly using the 'SimpleApp.exe.log4net' file
//[assembly: log4net.Config.XmlConfigurator(ConfigFileExtension="log4net",Watch=true)]

namespace Trafodion.Manager.Framework
{
    public static class Logger
    {
        #region member variables
        private static bool clearErrorLog = true;
        private static bool appendLog = true;
        private static bool _isTracing = false;
        private static string logFileName = null;
        private static string errorLogFileName = null;
        private static object lockObject = new object();
        private static int maxTraceFileSize = 10; // in MB
        private static int maxTraceBackupFiles = 10;
        private static string logFileExtension = null;
        private static bool appendDateToTraceFileName = false;

        private static readonly log4net.ILog log = log4net.LogManager.GetLogger(System.Reflection.MethodBase.GetCurrentMethod().DeclaringType);

        #endregion

        #region Properties

        public static bool IsTracing
        {
            get { return _isTracing; }
            set { _isTracing = value; }
        }

        /// <summary>
        /// Returns the name of the error log file name. By default the name from the properties file is returmed
        /// </summary>
        public static string DefaultErrorLogFilename
        {
            get
            {
                if (errorLogFileName == null)
                {
                    return Trafodion.Manager.Properties.Resources.DefaultErrorLogFile + "." + Trafodion.Manager.Properties.Resources.DefaultLogFileExtension;
                }
                else
                {
                    return errorLogFileName;
                }
            }

            //set
            //{
            //    errorLogFileName = value;
            //}
        }

        /// <summary>
        /// Returns the name of the trace log file name. By default the name from the properties file is returmed
        /// </summary>
        public static string TraceLogFilename
        {
            get { return logFileName; }
            set {
                if (String.CompareOrdinal(logFileName, value) != 0)
                {
                    logFileName = value;
                    log4net.GlobalContext.Properties["LogName"] = value;
                }
            }
        }

        /// <summary>
        /// Returns the trace file extension
        /// </summary>
        public static string TraceLogFileExtension
        {

            get { return logFileExtension; }
            set
            {
                if (string.CompareOrdinal(logFileExtension, value) != 0)
                {
                    logFileExtension = value;
                }
            }
        }

        /// <summary>
        /// Property: ErrorLogFilename
        /// Returns the name of the error log file name. By default the name from the properties file is returned
        /// Note: this is only used by the LogOptions.  Any other reference to the current Log file should use ErrorLog property.
        /// </summary>
        public static string ErrorLogFilename
        {
            get { return errorLogFileName; }
            set
            {
                if (String.CompareOrdinal(errorLogFileName, value) != 0)
                {                 
                    Logger.SwitchErrorLogFile(value);
                    errorLogFileName = value;
                }
            }
        }

        /// <summary>
        /// indicates if tracing has been enabled
        /// </summary>
        public static bool IsTracingEnabled
        {
            get { return TraceOptions.GetOptions().TraceActive(TraceOptions.TraceOption.ANY); }
        }

        /// <summary>
        /// Property: ErrorLog
        /// Returns the full file name of the error log. It's a read only property
        /// </summary>
        public static string ErrorLog
        {
            get
            {
                if (String.IsNullOrEmpty(errorLogFileName))
                {
                    errorLogFileName = LogOptions.GetOptions().ErrorLogFileName;
                }
                return errorLogFileName;
            }
        }

        /// <summary>
        /// Log4net logger
        /// </summary>
        public static log4net.ILog Log
        {
            get { return log; }
        }

        /// <summary>
        /// Property: MaxTraceFileSize - the maximum trace file size in MB
        /// </summary>
        public static int MaxTraceFileSize
        {
            get { return maxTraceFileSize; }
            set 
            {
                if (maxTraceBackupFiles != value)
                {
                    maxTraceFileSize = value;
                }
            }
        }

        /// <summary>
        /// Property: MaxTraceBackupFiles - the maximum number of backup trace files
        /// </summary>
        public static int MaxTraceBackupFiles
        {
            get { return maxTraceBackupFiles; }
            set 
            {
                if (maxTraceBackupFiles != value)
                {
                    maxTraceBackupFiles = value;
                }
            }
        }

        /// <summary>
        /// Property: AppendToTraceFile - to append to an existing trace file or to overwrite it.
        /// </summary>
        public static bool AppendToTraceFile
        {
            get { return appendLog; }
            set 
            {
                if (appendLog != value)
                {
                    appendLog = value;
                }
            }
        }

        /// <summary>
        /// Property: AppendDateToFileName - to append date to trace file name.
        /// </summary>
        public static bool AppendDateToFileName
        {
            get { return appendDateToTraceFileName; }
            set 
            {
                if (Logger.AppendDateToFileName != value)
                {
                    appendDateToTraceFileName = value;
                }
            }
        }

        #endregion

        #region public methods

        /// <summary>
        /// Method to enable logging
        /// </summary>
        public static void EnableLogging(bool loggerPropertyChanged)
        {
            if (string.IsNullOrEmpty(logFileName))
            {
                return;
            }

            if (loggerPropertyChanged || !_isTracing)
            {
                ConfigureLog4Net();
            }
            else if (_isTracing)
            {
                return;
            }

            _isTracing = true;

            try
            {
                WriteTraceLog(TraceOptions.TraceArea.Framework.ToString(),
                              null,
                              TraceOptions.TraceOption.ANY.ToString(),
                              "@@@@@@@@@@@@@@@@@@@@@@ TrafodionManager 3 Tracing Enabled");
            }
            catch (Exception ex)
            {
                System.Console.WriteLine(ex.Message + " - " + ex.StackTrace);
            }
        }

        /// <summary>
        /// Perform log4net configuration
        /// Note: those commented lines were used if the external configuration file is used.
        /// </summary>
        private static void ConfigureLog4Net()
        {
            //StreamWriter sw = new StreamWriter(Path.Combine(Application.StartupPath, "Trafodion.Manager.dll.log4net"), false);
            StringBuilder sb = new StringBuilder();
            try
            {
                sb.AppendLine("<?xml version=\"1.0\" encoding=\"utf-8\" ?>");
                sb.AppendLine("   <!-- This section contains the log4net configuration settings -->");
                sb.AppendLine("      <log4net>");
                sb.AppendLine("         <!-- Define some output appenders -->");
                sb.AppendLine("         <appender name=\"RollingTraceAppender\" type=\"log4net.Appender.RollingFileAppender\">");
                if (appendDateToTraceFileName)
                {
                    sb.AppendLine(string.Format("            <file type=\"log4net.Util.PatternString\" value=\"{0}.{1}\" />", logFileName + "-%date{yyyy-MM-dd}", logFileExtension));
                }
                else
                {
                    sb.AppendLine(string.Format("            <file type=\"log4net.Util.PatternString\" value=\"{0}.{1}\" />", logFileName, logFileExtension));
                }

                sb.AppendLine(string.Format("            <appendToFile value=\"{0}\" />", appendLog));
                sb.AppendLine("            <rollingStyle value=\"Composite\" />");
                sb.AppendLine(string.Format("            <maxSizeRollBackups value=\"{0}\" />", MaxTraceBackupFiles));
                sb.AppendLine(string.Format("            <maximumFileSize value=\"{0}MB\" />", MaxTraceFileSize));
                sb.AppendLine("            <PreserveLogFileNameExtension value=\"true\" />");
                sb.AppendLine("            <layout type=\"log4net.Layout.PatternLayout\">");
                sb.AppendLine("               <conversionPattern value=\"%date{yyyy-MM-dd HH:mm:ss.fff}&#9;[%thread] %-5level %logger [%property{AREA}] [%property{SUB_AREA}] [%property{TRACE_OPTION}] - %message%newline\" />");
                sb.AppendLine("            </layout>");
                //sb.AppendLine("            <filter type=\"log4net.Filter.LevelRangeFilter\">");
                //sb.AppendLine("               <levelMax value=\"DEBUG\" />");
                //sb.AppendLine("            </filter>");
                sb.AppendLine("         </appender>");

                //sb.AppendLine("         <appender name=\"RollingErrorLogAppender\" type=\"log4net.Appender.RollingFileAppender\">");
                //sb.AppendLine(string.Format("            <file type=\"log4net.Util.PatternString\" value=\"{0}\" />", ErrorLog));
                //sb.AppendLine(string.Format("            <appendToFile value=\"true\" />", clearLog));
                //sb.AppendLine("            <rollingStyle value=\"Size\" />");
                //sb.AppendLine(string.Format("            <maxSizeRollBackups value=\"{0}\" />", -1));
                //sb.AppendLine(string.Format("            <maximumFileSize value=\"{0}B\" />", "10K"));
                //sb.AppendLine("            <PreserveLogFileNameExtension value=\"true\" />");
                //sb.AppendLine("            <layout type=\"log4net.Layout.PatternLayout\">");
                //sb.AppendLine("               <conversionPattern value=\"%date [%thread] %-5level %logger [%property{AREA}] [%property{SUB_AREA}] [%property{TRACE_OPTION}] - %message%newline\" />");
                //sb.AppendLine("            </layout>");
                //sb.AppendLine("            <filter type=\"log4net.Filter.LevelRangeFilter\">");
                //sb.AppendLine("               <levelMin value=\"ERROR\" />");
                //sb.AppendLine("            </filter>");
                //sb.AppendLine("         </appender>");

                sb.AppendLine("         <!-- Setup the root category, add the appenders and set the default priority -->");
                sb.AppendLine("         <root>");
                sb.AppendLine(string.Format("            <level value=\"{0}\" />", "ALL"));
                sb.AppendLine("            <appender-ref ref=\"RollingTraceAppender\" />");
                //sb.AppendLine("            <appender-ref ref=\"RollingErrorLogAppender\" />");
                sb.AppendLine("         </root>");
                sb.AppendLine("      </log4net>");
                //sw.WriteLine(sb.ToString());
                //sw.Flush();
            }
            catch (Exception ex)
            {
                System.Console.WriteLine(ex.Message + " - " + ex.StackTrace);
            }
            //finally
            //{
            //    sw.Close();
            //    sw.Dispose();
            //}

            ConfigureLog4Net(sb.ToString());
        }

        /// <summary>
        /// use the input string as an log4net configuration stream for configuring the logger
        /// </summary>
        /// <param name="xmlConfigString"></param>
        private static void ConfigureLog4Net(string xmlConfigString)
        {
            MemoryStream ms = new MemoryStream(ASCIIEncoding.Default.GetBytes(xmlConfigString));
            log4net.Config.XmlConfigurator.Configure(ms);
            ms.Close();
            ms.Dispose();
        }

        /// <summary>
        /// Note: This should be called before the log file has been switch.
        /// </summary>
        /// <param name="newErrorLogFile"></param>
        private static void SwitchErrorLogFile(string newErrorLogFile)
        {
            // reporting switch to new error log file
            StreamWriter errsw = null;
            string message1 = 
                string.Format(Properties.Resources.ErrorLogFileSwitchMessage, DateTime.Now.ToString());
            string message2 =
                string.Format(Properties.Resources.ErrorLogNewFileMessage, newErrorLogFile);
            string message3 =
                string.Format(Properties.Resources.ErrorLogStartMessage, DateTime.Now.ToString());

            try
            {
                if (File.Exists(ErrorLog))
                {
                    errsw = File.AppendText(ErrorLog);

                    clearErrorLog = false;
                    errsw.WriteLine("=======================================================================");
                    errsw.WriteLine(message1);
                    errsw.WriteLine(message2);
                    errsw.WriteLine("=======================================================================");
                    errsw.WriteLine();
                    errsw.Flush();
                    errsw.Close();
                }

                // Now, log to new file.
                if (LogOptions.GetOptions().ClearNewLogFileAtSwitch)
                {
                    errsw = new StreamWriter(newErrorLogFile, false);
                }
                else
                {
                    errsw = File.AppendText(newErrorLogFile);
                }

                clearErrorLog = false;
                errsw.WriteLine("=======================================================================");
                errsw.WriteLine(message3);
                errsw.WriteLine("=======================================================================");
                errsw.WriteLine(); 
            }
            catch (Exception ex)
            {

            }
            finally
            {
                if (errsw != null)
                {
                    errsw.Close();
                }
            }
#if DEBUG
            log.Info(message1);
            log.Info(message2);
#endif
        }

        /// <summary>
        /// Disable logging
        /// </summary>
        public static void DisableLogging()
        {
            try
            {
                WriteTraceLog(TraceOptions.TraceArea.Framework.ToString(),
                              null,
                              TraceOptions.TraceOption.ANY.ToString(),
                              "@@@@@@@@@@@@@@@@@@@@@@ Tracing Disabled");
            }
            catch (Exception ex)
            {
                System.Console.WriteLine(ex.Message + " - " + ex.StackTrace);
            }

            // note: cannot shut off the log4net, otherwise, it'll turn it off entire and can no longer turn back on. 
            //       also, we could not just leave the appender around since the log4net will remain using the file
            //       so, just redirect the appender to console. 
            StringBuilder sb = new StringBuilder();
            sb.AppendLine("<?xml version=\"1.0\" encoding=\"utf-8\" ?>");
            sb.AppendLine("   <!-- This section contains the log4net configuration settings in order to stop tracing -->");
            sb.AppendLine("      <log4net>");
            sb.AppendLine("         <appender name=\"Console\" type=\"log4net.Appender.ConsoleAppender\">");
            //sb.AppendLine("            <layout type=\"log4net.Layout.PatternLayout\">");
            //sb.AppendLine("            <conversionPattern value=\"%date [%thread] %-5level %logger [%property{AREA}] [%property{SUB_AREA}] [%property{TRACE_OPTION}] - %message%newline\" />");
            //sb.AppendLine("            </layout>");
            sb.AppendLine("         </appender>");	
	        sb.AppendLine("         <root>");
            //sb.AppendLine("            <level value=\"ALL\" />");
	        sb.AppendLine("            <appender-ref ref=\"Console\" />");
	        sb.AppendLine("         </root>");
            sb.AppendLine("      </log4net>");

            ConfigureLog4Net(sb.ToString());

            _isTracing = false;
        }

        //Note: this method is used if we want to have individual logger in different classes. This will allow individual class name and even line number to be 
        //      shown in the trace records.
        //public static void OutputToLog(ILog log, TraceOptions.TraceOption opt, TraceOptions.TraceArea area, string subArea, string outputString)
        //{
        //      log4net.ThreadContext.Properties["TRACE_OPTION"] = opt;
        //      log4net.ThreadContext.Properties["AREA"] = area;
        //      log4net.ThreadContext.Properties["SUB_AREA"] = subArea;
        //      log.Debug(outputString);
        //      OutputToLog(opt, area, subArea, outputString);
        //}

        /// <summary>
        /// Writes out the string passed to the trace log
        /// </summary>
        /// <param name="outputString"></param>
        public static void OutputToLog(string outputString)
        {
            OutputToLog(TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Framework, outputString);
        }

        /// <summary>
        /// Writes out the string passed to the trace log
        /// </summary>
        /// <param name="opt"></param>
        /// <param name="area"></param>
        /// <param name="outputString"></param>
        public static void OutputToLog(TraceOptions.TraceOption opt, TraceOptions.TraceArea area, string outputString)
        {
            OutputToLog(opt, area, null, outputString);
        }

        /// <summary>
        /// Write traces.
        /// </summary>
        /// <param name="opt"></param>
        /// <param name="area"></param>
        /// <param name="subArea"></param>
        /// <param name="outputString"></param>
        public static void OutputToLog(TraceOptions.TraceOption opt, TraceOptions.TraceArea area, string subArea, string outputString)
        {
            if (TraceOptions.GetOptions().TraceActive(opt))
            {
                lock (lockObject)
                {
                    try
                    {
                        if (!_isTracing)
                        {
                            EnableLogging(true);
                        }

                        WriteTraceLog(area.ToString(), subArea, opt.ToString(), outputString);
                    }
                    catch (Exception ex)
                    {
                        System.Console.WriteLine(ex.Message + " - " + ex.StackTrace);
                    }
                }
            }
        }

        /// <summary>
        /// This method start the error log by writing a start message to the error log file.
        /// Note: this will in fact, setting up the error log file name correctly.
        /// </summary>
        /// <param name="outputString"></param>
        public static void StartErrorLog()
        {
            string message = string.Format(Properties.Resources.ErrorLogStartMessage, DateTime.Now.ToString());
            //ConfigErrorLog();
            StreamWriter errsw = null;

            try
            {
                if (!File.Exists(ErrorLog))
                    errsw = new StreamWriter(ErrorLog, false);
                else
                    errsw = File.AppendText(ErrorLog);

                clearErrorLog = false;
                errsw.WriteLine("=======================================================================");
                errsw.WriteLine(message);
                errsw.WriteLine("=======================================================================");
                errsw.WriteLine();
            }
            catch (Exception ex)
            {

            }
            finally
            {
                if (errsw != null)
                {
                    errsw.Close();
                }
            }

            log.Error("=======================================================================");
            log.Error(message);
            log.Error("=======================================================================");
        }

        //Note: This is to configure error logging to use log4net as well. This will allow a single integrated error and trace file. 
        //      The down side is log file may be re-created when trace options get changed. Also, if the user desires to have different 
        //      log and trace files, filters need to be configured and this could potential impact the performance. 
        //public static void ConfigErrorLog()
        //{
        //    StreamWriter sw = new StreamWriter(Path.Combine(Application.StartupPath, "Trafodion.Manager.dll.log4net"), false);

        //    try
        //    {
        //        StringBuilder sb = new StringBuilder();
        //        sb.AppendLine("<?xml version=\"1.0\" encoding=\"utf-8\" ?>");
        //        sb.AppendLine("   <!-- This section contains the log4net configuration settings -->");
        //        sb.AppendLine("      <log4net>");
        //        sb.AppendLine("         <!-- Define some output appenders -->");
        //        sb.AppendLine("         <appender name=\"RollingErrorLogAppender\" type=\"log4net.Appender.RollingFileAppender\">");
        //        sb.AppendLine(string.Format("            <file type=\"log4net.Util.PatternString\" value=\"{0}\" />", ErrorLog));
        //        sb.AppendLine(string.Format("            <appendToFile value=\"true\" />", clearLog));
        //        sb.AppendLine("            <rollingStyle value=\"Size\" />");
        //        sb.AppendLine(string.Format("            <maxSizeRollBackups value=\"{0}\" />", -1));
        //        sb.AppendLine(string.Format("            <maximumFileSize value=\"{0}B\" />", "10M"));
        //        sb.AppendLine("            <PreserveLogFileNameExtension value=\"true\" />");
        //        sb.AppendLine("            <layout type=\"log4net.Layout.PatternLayout\">");
        //        sb.AppendLine("               <conversionPattern value=\"%date [%thread] %-5level %logger [%property{AREA}] [%property{SUB_AREA}] [%property{TRACE_OPTION}] - %message%newline\" />");
        //        sb.AppendLine("            </layout>");
        //        sb.AppendLine("            <filter type=\"log4net.Filter.LevelRangeFilter\">");
        //        sb.AppendLine("               <levelMin value=\"ERROR\" />");
        //        sb.AppendLine("            </filter>");
        //        sb.AppendLine("         </appender>");

        //        sb.AppendLine("         <!-- Setup the root category, add the appenders and set the default priority -->");
        //        sb.AppendLine("         <root>");
        //        sb.AppendLine(string.Format("            <level value=\"{0}\" />", "ALL"));
        //        sb.AppendLine("            <appender-ref ref=\"RollingErrorLogAppender\" />");
        //        sb.AppendLine("         </root>");
        //        sb.AppendLine("      </log4net>");
        //        sw.WriteLine(sb.ToString());
        //        sw.Flush();
        //    }
        //    catch (Exception ex)
        //    {
        //        System.Console.WriteLine(ex.Message + " - " + ex.StackTrace);
        //    }
        //    finally
        //    {
        //        sw.Close();
        //        sw.Dispose();
        //    }

        //}

        /// <summary>
        /// This method writes the string passsed to the error log
        /// </summary>
        /// <param name="outputString"></param>
        public static void OutputErrorLog(string outputString)
        {
            StreamWriter errsw = null;

            try
            {
                if (!File.Exists(ErrorLog))
                    errsw = new StreamWriter(ErrorLog, false);
                else
                    errsw = File.AppendText(ErrorLog);

                clearErrorLog = false;
                errsw.WriteLine("@@@ Exception : " + DateTime.Now.ToString());
                errsw.WriteLine("----------------------------------------------------");
                if (null != errsw)
                {
                    errsw.WriteLine(DateTime.Now.ToString() + ": " + outputString);
                }

                OutputToLog("@@@ An Exception has occured. Details logged in: " + errorLogFileName);
                errsw.WriteLine();
            }
            catch (Exception ex)
            {

            }
            finally
            {
                if (errsw != null)
                {
                    errsw.Close();
                }
            }
//#if DEBUG
//            log.Error(outputString);
//#endif
        }

        /// <summary>
        /// Check if the specified trace option is on?
        /// </summary>
        /// <param name="opt"></param>
        /// <returns></returns>
        public static bool IsTraceActive(TraceOptions.TraceOption opt)
        {
            return TraceOptions.GetOptions().TraceActive(opt);
        }

        #endregion

        #region Private methods

        /// <summary>
        /// To write out trace records via log4net logger
        /// </summary>
        /// <param name="area"></param>
        /// <param name="subarea"></param>
        /// <param name="opt"></param>
        /// <param name="message"></param>
        private static void WriteTraceLog(string area, string subarea, string opt, string message)
        {
            log4net.ThreadContext.Properties["TRACE_OPTION"] = opt;
            log4net.ThreadContext.Properties["AREA"] = area;
            log4net.ThreadContext.Properties["SUB_AREA"] = subarea;
            log.Debug(message);
        }

        #endregion Private methods
    }

}
