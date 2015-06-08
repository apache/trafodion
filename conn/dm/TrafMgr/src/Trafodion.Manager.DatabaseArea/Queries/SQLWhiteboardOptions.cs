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
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.DatabaseArea.Queries
{
    /// <summary>
    /// Object to store SQL Whiteboard options
    /// </summary>
    [Serializable]
    public class SQLWhiteboardOptions : IOptionObject
    {
        #region Member variables

        private int  _defaultMaxRowCount = 500;
        private bool _stopBatchOperationOnExceptions = true;

        [field: NonSerialized]
        public const string OptionsKey = "SQLWhiteboard Options";

        [field: NonSerialized]
        private static EventHandlerList _theEventHandlers = new EventHandlerList();

        [field: NonSerialized]
        private const string _theOptionsChangedKey = "OptionsChanged";

        /// <summary>
        /// Option changed Event handler
        /// </summary>
        public event EventHandler SQLWhiteboardOptionsChanged
        {
            add { _theEventHandlers.AddHandler(_theOptionsChangedKey, value); }
            remove { _theEventHandlers.RemoveHandler(_theOptionsChangedKey, value); }
        }

        #endregion

        #region Constructor

        /// <summary>
        /// Constructs the default SQLWhiteboardOptions
        /// </summary>
        public SQLWhiteboardOptions()
        {
            //setup the default values
            _defaultMaxRowCount = 500;
            _stopBatchOperationOnExceptions = true;
        }

        /// <summary>
        /// Copy constructor
        /// </summary>
        /// <param name="options"></param>
        public SQLWhiteboardOptions(SQLWhiteboardOptions options)
        {
            _defaultMaxRowCount = options.DefaultMaxRowCount;
            _stopBatchOperationOnExceptions = options.StopBatchOperationOnExceptions;
        }

        #endregion

        #region Properties

        /// <summary>
        /// Default maximum row counts
        /// </summary>
        [Category("Application Settings")]
        [Description("An integer value greater than 0 specifying the default maximum row count when you start SQL Whiteboard.")]
        [DisplayName("Default Max Rows")]
        public int DefaultMaxRowCount
        {
            get { return _defaultMaxRowCount; }
            set { if (value > 0) { _defaultMaxRowCount = value; }  }
        }

        /// <summary>
        /// Booelan value to stop batch execution when an exception is reported
        /// </summary>
        [Category("Application Settings")]
        [Description("Specifying if batch execution (or batch explain) operation should be discontinued when an exception is encountered.")]
        [DisplayName("Stop Batch Operation On Exceptions")]
        public bool StopBatchOperationOnExceptions
        {
            get { return _stopBatchOperationOnExceptions; }
            set { _stopBatchOperationOnExceptions = value; }
        }

        #endregion

        #region Public Methods

        /// <summary>
        /// Gets the current instance of SQLWhiteboardOptions
        /// </summary>
        /// <returns></returns>
        public static SQLWhiteboardOptions GetOptions()
        {

            //Load DatabaseAreaOptions from the OptionStore persistence
            SQLWhiteboardOptions options = OptionStore.GetOptionValues(Properties.Resources.SQLWhiteboard, OptionsKey) as SQLWhiteboardOptions;
            if (options == null)
            {
                //If DatabaseAreaOptions not available in OptionStore, create a default DatabaseAreaOptions
                options = new SQLWhiteboardOptions();
                try
                {
                    //Save the DatabaseAreaOptions to the OptionStore, so a subsequent GetOptions call would find it
                    options.Save();
                }
                catch (Exception ex)
                {
                }
            }
            return options;
        }

        /// <summary>
        /// Save the current state of the SQLWhiteboardOptions into the OptionStore
        /// </summary>
        public void Save()
        {
            OptionStore.SaveOptionValues(Properties.Resources.SQLWhiteboard, OptionsKey, this);
            OnOptionsChanged();
        }

        /// <summary>
        /// Fire up Option changed event
        /// </summary>
        public void OnOptionsChanged()
        {
            OnSQLWhiteboardOptionsChanged();
        }

       /// <summary>
       /// Handle the case when Options is loaded from persistence
       /// </summary>
       /// <param name="optionObject"></param>
        public void LoadedFromPersistence(Object optionObject)
        {
        }

        /// <summary>
        /// Multicast events to all listeners
        /// </summary>
        protected virtual void OnSQLWhiteboardOptionsChanged()
        {
            // Get all of the listeners to the changed event
            EventHandler theOptionsChangedHandlers = (EventHandler)_theEventHandlers[_theOptionsChangedKey];

            // Check to see if there are any
            if (theOptionsChangedHandlers != null)
            {
                // Multicast to them
                theOptionsChangedHandlers(this, new EventArgs());
            }
        }

        /// <summary>
        /// Clone the options for display purposes
        /// </summary>
        /// <returns></returns>
        public IOptionObject Clone()
        {
            return (IOptionObject)(new SQLWhiteboardOptions(this));
        }

        /// <summary>
        /// To Copy the options from another SQLWhiteboard options object.
        /// </summary>
        /// <param name="obj"></param>
        public void Copy(IOptionObject obj)
        {
            SQLWhiteboardOptions options = obj as SQLWhiteboardOptions;
            if (obj != null)
            {
                this.DefaultMaxRowCount = options.DefaultMaxRowCount;
                this.StopBatchOperationOnExceptions = options.StopBatchOperationOnExceptions;
            }
        }

        #endregion Public Methods
    }
}

