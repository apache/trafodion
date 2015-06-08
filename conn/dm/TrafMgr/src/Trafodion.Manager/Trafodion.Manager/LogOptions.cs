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
    public class LogOptions : IOptionObject
    {
        #region Fields
        /// <summary>
        /// Log option Key
        /// </summary>
        public const string LogOptionsKey = "Logging";

        // Private members
        private string _errorLogFileName = "";
        private bool _clearNewLogFileAtSwitch = true;

        #endregion Fields

        #region Constructors

        /// <summary>
        /// Constructors
        /// </summary>
        private LogOptions()
        {
            _errorLogFileName = Path.Combine(Persistence.HomeDirectory, Trafodion.Manager.Properties.Resources.DefaultErrorLogFile) + "." + Trafodion.Manager.Properties.Resources.DefaultLogFileExtension;
            _clearNewLogFileAtSwitch = true;
        }

        /// <summary>
        /// Copy constructor
        /// </summary>
        /// <param name="options"></param>
        public LogOptions(LogOptions options)
        {
            _errorLogFileName = options.ErrorLogFileName;
            _clearNewLogFileAtSwitch = options.ClearNewLogFileAtSwitch;
        }

        #endregion Constructors

        #region Properties
        /// <summary>
        /// Error Log File Name
        /// </summary>
        [Category("Application Settings")]
        [Description("Specifying the error log file name. This attribute will be persisted.")]
        [DisplayName("Error Log File Name")]
        public String ErrorLogFileName
        {
            get 
            {
                if (String.IsNullOrEmpty(_errorLogFileName))
                {
                    _errorLogFileName = Path.Combine(Persistence.HomeDirectory, Trafodion.Manager.Properties.Resources.DefaultErrorLogFile) + "." + Trafodion.Manager.Properties.Resources.DefaultLogFileExtension;
                }

                return _errorLogFileName;
            }
            set
            {
                if (!string.IsNullOrEmpty(value))
                {
                    if (Utilities.IsValidFileName(value))
                    {
                        _errorLogFileName = value;
                    }
                }
            }
        }

        /// <summary>
        /// Clear New Log File When 
        /// </summary>
        [Category("Application Settings")]
        [Description("Specifying whether or not to clear all entries in the new error log file when the file name is changed.")]
        [DisplayName("Clear New Log File")]
        public bool ClearNewLogFileAtSwitch
        {
            get { return _clearNewLogFileAtSwitch; }
            set { _clearNewLogFileAtSwitch = value; }
        }

        #endregion

        /// <summary>
        /// Gets the current instance of LogOptions
        /// </summary>
        /// <returns></returns>
        public static LogOptions GetOptions()
        {

            //Load LogOptions from the OptionStore persistence
            LogOptions options = OptionStore.GetOptionValues("General", LogOptionsKey) as LogOptions;
            if (options == null)
            {
                //If LogOptions not available in OptionStore, create a default LogOptions
                options = new LogOptions();
                try
                {
                    //Save the LogOptions to the OptionStore, so a subsequent GetOptions call would find it
                    options.Save();
                }
                catch (Exception)
                {
                }
            }
            return options;
        }

        /// <summary>
        /// Save the options to option store.
        /// </summary>
        public void Save()
        {
            OptionStore.SaveOptionValues("General", LogOptionsKey, this);
            OnOptionsChanged();
        }

        /// <summary>
        /// Called when the user clicks the Ok button on the options panel.
        /// </summary>
        public void OnOptionsChanged()
        {
            if (String.CompareOrdinal(_errorLogFileName, Logger.ErrorLogFilename) != 0)
            {
                Logger.ErrorLogFilename = ErrorLogFileName;
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

        /// <summary>
        /// Close self for display
        /// </summary>
        /// <returns></returns>
        public IOptionObject Clone()
        {
            return (IOptionObject)(new LogOptions(this));
        }

        /// <summary>
        /// To copy options from another LogOptions object.
        /// </summary>
        /// <param name="obj"></param>
        public void Copy(IOptionObject obj)
        {
            LogOptions options = obj as LogOptions;
            if (options != null)
            {
                this.ErrorLogFileName = options.ErrorLogFileName;
                this.ClearNewLogFileAtSwitch = options.ClearNewLogFileAtSwitch;
            }
        }
    }
}

