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
using System.ComponentModel;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager
{
    [Serializable]
    public class TraceOptions : IOptionObject
    {
        #region Fields
        /// <summary>
        /// Trace option Key
        /// </summary>
        public const string TraceOptionsKey = "Trace";
        public const string DefaultTraceFileExtension = "log";
        public const string DefaultTraceFileName = "TrafodionTrace";

        /// <summary>
        /// Trace options
        /// </summary>
        public enum TraceOption { ANY = 0, SQL, DEBUG, LIVEFEEDDATA, PERF };

        /// <summary>
        /// Trace areas
        /// </summary>
        public enum TraceArea { Framework, Connection, Database, Security, Monitoring, Connectivity, UW, SQLWhiteboard, MetricMinner, WMS, Overview, AMQPFramework, UserManagement, LiveFeedFramework, SQMonotioring, LiveFeedMonitoring, BDRDashboard };

        // Private members
        private string _traceFileName = "";
        private string _tracefileExtension = DefaultTraceFileExtension;

        private int _maxNumberBackFiles = 10;

        private int _maxFileSize = 10;

        private bool _appendToTraceFile = false;

        private bool _addDateToTraceFileName = false;

        [NonSerialized]
        private bool _sqlTraceActive = false;

        [NonSerialized]
        private bool _debugTraceActive = false;

        [NonSerialized]
        private bool _livefeedDataTraceActive = false;

        [NonSerialized]
        private bool _queryExecTimeTraceActive = false;

        #endregion Fields

        #region Constructors

        /// <summary>
        /// Constructors
        /// </summary>
        private TraceOptions()
        {
            _sqlTraceActive = false;
            _debugTraceActive = false;
            _livefeedDataTraceActive = false;
            _queryExecTimeTraceActive = false;
            _traceFileName = Path.Combine(Persistence.HomeDirectory, DefaultTraceFileName);
            _tracefileExtension = DefaultTraceFileExtension;
        }

        /// <summary>
        /// Copy constructor
        /// </summary>
        /// <param name="options"></param>
        public TraceOptions(TraceOptions options)
        {
            _sqlTraceActive = options.SQLTraceActive;
            _debugTraceActive = options.DEBUGTraceActive;
            _livefeedDataTraceActive = options.LiveFeedDataTraceActive;
            _queryExecTimeTraceActive = options.QueryExecutionTimeTraceActive;
            _traceFileName = options.TraceFileName;
            _maxNumberBackFiles = options.MaxNumberBackupFiles;
            _maxFileSize = options.MaxTraceFileSize;
            _appendToTraceFile = options.AppendToTraceFile;
            _tracefileExtension = options.TraceFileExtension;
            _addDateToTraceFileName = options.AppendDateToTraceFileName;
        }

        #endregion Constructors

        #region Properties
        /// <summary>
        /// Trace file name
        /// </summary>
        [Category("Application Settings")]
        [Description("Specifying the trace file name. Note the trace file extension is a separate property. This property will be persisted.")]
        [DisplayName("Trace File Name")]
        public String TraceFileName
        {
            get 
            {
                if (String.IsNullOrEmpty(_traceFileName))
                {
                    _traceFileName = Path.Combine(Persistence.HomeDirectory, DefaultTraceFileName);
                }

                return _traceFileName;
            }
            set 
            {
                if (!string.IsNullOrEmpty(value))
                {
                    if (Utilities.IsValidFileName(value))
                    {
                        _traceFileName = value;
                    }
                }
            }
        }

        /// <summary>
        /// Booelan value to turn on or off traces
        /// </summary>
        [Category("Application Settings")]
        [Description("Specifying whether the SQL trace is active. This property will not be persisted.")]
        [DisplayName("Trace SQL Statement")]
        public bool SQLTraceActive
        {
            get { return _sqlTraceActive; }
            set { _sqlTraceActive = value; }
        }

        /// <summary>
        /// Booelan value to turn on or off traces
        /// </summary>
        [Category("Application Settings")]
        [Description("Specifying whether the DEBUG trace is active. This property will not be persisted.")]
        [DisplayName("Trace Internal Debug Message")]
        public bool DEBUGTraceActive
        {
            get { return _debugTraceActive; }
            set { _debugTraceActive = value; }
        }

        /// <summary>
        /// Booelan value to turn on or off traces
        /// </summary>
        [Category("Application Settings")]
        [Description("Specifying whether the LiveFeed Data trace is active. This property will not be persisted. [Note]: This may collect a large amount of data. Therefore, select only a subset of metrics when enabling the trace.")]
        [DisplayName("Trace LiveFeed Data")]
        public bool LiveFeedDataTraceActive
        {
            get { return _livefeedDataTraceActive; }
            set { _livefeedDataTraceActive = value; }
        }

        /// <summary>
        /// Booelan value to turn on or off traces
        /// </summary>
        [Category("Application Settings")]
        [Description("Specifying whether the TrafodionManager internal query execution time trace is active. This property will not be persisted.")]
        [DisplayName("Trace Internal Query Execution Time")]
        public bool QueryExecutionTimeTraceActive
        {
            get { return _queryExecTimeTraceActive; }
            set { _queryExecTimeTraceActive = value; }
        }
        
        /// <summary>
        /// Maximum trace file size 
        /// </summary>
        [Category("Application Settings")]
        [Description("Specifying the maximum size in MB that the output file is allowed to reach before being rolled over to backup files. Valid file size must range from 1 to 9999 MB. This property will be persisted.")]
        [DisplayName("Maximum Trace File Size (MB)")]
        public int MaxTraceFileSize
        {
            get { return _maxFileSize; }
            set { if (value > 0 && value < 10000) { _maxFileSize = value; } }
        }

        /// <summary>
        /// Maximum number of backup files 
        /// </summary>
        [Category("Application Settings")]
        [Description("Specifying the maximum number of backup files that are kept before the oldest is erased. Valid number of backup files must be range from 1 to 999. This property will be persisted.")]
        [DisplayName("Number of Backup Files")]
        public int MaxNumberBackupFiles
        {
            get { return _maxNumberBackFiles; }
            set { if (value > 0 && value < 1000) { _maxNumberBackFiles = value; } }
        }

        /// <summary>
        /// Clear trace file when trace start 
        /// </summary>
        [Category("Application Settings")]
        [Description("Specifying either to append trace records to an existing file or to overwrite it. If this property is not enabled, the existing file will be rolled over. This property will be persisted.")]
        [DisplayName("Append To Existing Trace File")]
        public bool AppendToTraceFile
        {
            get { return _appendToTraceFile; }
            set { _appendToTraceFile = value; }
        }

        /// <summary>
        /// Append date to the trace file name 
        /// </summary>
        [Category("Application Settings")]
        [Description("Specifying whether to append date to the trace file names. For example, the trace file name of TrafodionTrace.log will become TrafodionTrace-2012-2-26.log. If this property is set, rollover may occur when date changes. This property will be persisted.")]
        [DisplayName("Append Date to Trace File Name")]
        public bool AppendDateToTraceFileName
        {
            get { return _addDateToTraceFileName; }
            set { _addDateToTraceFileName = value; }
        }

        /// <summary>
        /// Trace file extension
        /// </summary>
        [Category("Application Settings")]
        [Description("Specifying the trace file extension. Trace file extension allows only alpha numeric characters. This property will be persisted.")]
        [DisplayName("Trace File Extension")]
        public String TraceFileExtension
        {
            get
            {
                if (String.IsNullOrEmpty(_tracefileExtension))
                {
                    _tracefileExtension = DefaultTraceFileExtension;
                }

                return _tracefileExtension;
            }
            set
            {
                if (!string.IsNullOrEmpty(value))
                {
                    if (Utilities.ContainOnlyAlphaNumericCharacters(value))
                    {
                        _tracefileExtension = value;
                    }
                }
            }
        }
        #endregion

        /// <summary>
        /// Gets the current instance of TraceOptions
        /// </summary>
        /// <returns></returns>
        public static TraceOptions GetOptions()
        {

            //Load TraceOptions from the OptionStore persistence
            TraceOptions options = OptionStore.GetOptionValues("General", TraceOptionsKey) as TraceOptions;
            if (options == null)
            {
                //If TraceOptions not available in OptionStore, create a default TraceOptions
                options = new TraceOptions();
                try
                {
                    //Save the TraceOptions to the OptionStore, so a subsequent GetOptions call would find it
                    options.Save();
                }
                catch (Exception)
                {
                }
            }
            else
            {
                if (options.MaxTraceFileSize <= 0) options.MaxTraceFileSize = 10;
                if (options.MaxNumberBackupFiles <= 0) options.MaxNumberBackupFiles = 10;
                if (string.IsNullOrEmpty(options.TraceFileExtension)) options.TraceFileExtension = DefaultTraceFileExtension;
                if (string.IsNullOrEmpty(options.TraceFileName)) options.TraceFileName = DefaultTraceFileName;
            }

            return options;
        }

        /// <summary>
        /// To save the options
        /// </summary>
        public void Save()
        {
            OptionStore.SaveOptionValues("General", TraceOptionsKey, this);
            OnOptionsChanged();
        }

        /// <summary>
        /// Called when the user clicks the Ok button on the options panel.
        /// </summary>
        public void OnOptionsChanged()
        {
            bool loggerPropertyChanged = false;

            if (String.CompareOrdinal(_traceFileName, Logger.TraceLogFilename) != 0)
            {
                Logger.TraceLogFilename = TraceFileName;
                loggerPropertyChanged = true;
            }

            if (String.CompareOrdinal(_tracefileExtension, Logger.TraceLogFileExtension) != 0)
            {
                Logger.TraceLogFileExtension = TraceFileExtension;
                loggerPropertyChanged = true;
            }

            if (MaxNumberBackupFiles != Logger.MaxTraceBackupFiles)
            {
                Logger.MaxTraceBackupFiles = MaxNumberBackupFiles;
                loggerPropertyChanged = true;
            }

            if (MaxTraceFileSize != Logger.MaxTraceFileSize)
            {
                Logger.MaxTraceFileSize = MaxTraceFileSize;
                loggerPropertyChanged = true;
            }

            if (AppendToTraceFile != Logger.AppendToTraceFile)
            {
                Logger.AppendToTraceFile = AppendToTraceFile;
                loggerPropertyChanged = true;
            }

            if (AppendDateToTraceFileName != Logger.AppendDateToFileName)
            {
                Logger.AppendDateToFileName = AppendDateToTraceFileName;
                loggerPropertyChanged = true;
            }

            if (TraceActive(TraceOption.ANY))
            {
                Logger.EnableLogging(loggerPropertyChanged);
            }
            else
            {
                Logger.DisableLogging();
            }
        }

        /// <summary>
        /// This method shall be called by the framework to set the options that
        /// have obtained from the persistance framework. The persisted options
        /// are obtained using the IOptionProvider's name and the Option title.
        /// </summary>
        /// <param name="persistedObject"></param>
        public void LoadedFromPersistence(Object persistedObject)
        {
        }

        public bool TraceActive(TraceOption opt)
        {
            switch (opt)
            {
                case TraceOption.ANY:
                    return (SQLTraceActive || DEBUGTraceActive || LiveFeedDataTraceActive);

                case TraceOption.SQL:
                    return SQLTraceActive;

                case TraceOption.DEBUG:
                    return DEBUGTraceActive;

                case TraceOption.LIVEFEEDDATA:
                    return LiveFeedDataTraceActive;

                case TraceOption.PERF:
                    return QueryExecutionTimeTraceActive;

                default:
                    return false;
            }
        }

        /// <summary>
        /// Close self for display
        /// </summary>
        /// <returns></returns>
        public IOptionObject Clone()
        {
            return (IOptionObject)(new TraceOptions(this));
        }

        /// <summary>
        /// To copy options from another TraceOptions object.
        /// </summary>
        /// <param name="obj"></param>
        public void Copy(IOptionObject obj)
        {
            TraceOptions options = obj as TraceOptions;
            if (options != null)
            {
                this.TraceFileName = options.TraceFileName;
                this.TraceFileExtension = options.TraceFileExtension;
                this.SQLTraceActive = options.SQLTraceActive;
                this.DEBUGTraceActive = options.DEBUGTraceActive;
                this.LiveFeedDataTraceActive = options.LiveFeedDataTraceActive;
                this.QueryExecutionTimeTraceActive = options.QueryExecutionTimeTraceActive;
                this.MaxNumberBackupFiles = options.MaxNumberBackupFiles;
                this.MaxTraceFileSize = options.MaxTraceFileSize;
                this.AppendToTraceFile = options.AppendToTraceFile;
                this.AppendDateToTraceFileName = options.AppendDateToTraceFileName;
            }
        }
    }
}
