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

namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// Object to store Database Area options
    /// </summary>
    [Serializable]
    public class DatabaseAreaOptions : IOptionObject
    {
        #region Member variables


        private bool _enableExplainColorProcessBoundaries = true;
        private bool _enableSortExplainGridByLevels = true;

        public const string OptionsKey = "Database Options";

        //[field: NonSerialized]
        //public event EventHandler DatabaseOptionsChanged;

        [field: NonSerialized]
        private static EventHandlerList _theEventHandlers = new EventHandlerList();

        [field: NonSerialized]
        private const string _theOptionsChangedKey = "OptionsChanged";

        #endregion

        #region Constructor

        /// <summary>
        /// Constructs the default DatabaseAreaOptions
        /// </summary>
        public DatabaseAreaOptions()
        {
            //ShowSystemViews is disable by default
            _enableExplainColorProcessBoundaries = true;
            _enableSortExplainGridByLevels = true;
        }

        /// <summary>
        /// Copy constructor
        /// </summary>
        /// <param name="options"></param>
        public DatabaseAreaOptions(DatabaseAreaOptions options)
        {
            _enableExplainColorProcessBoundaries = options.EnableExplainColorProcessBoundaries;
            _enableSortExplainGridByLevels = options.EnableSortExplainGirdByLevels;
        }

        #endregion

        #region Properties

        /// <summary>
        /// Option changed Event handler
        /// </summary>
        public event EventHandler DatabaseOptionsChanged
        {
            add { _theEventHandlers.AddHandler(_theOptionsChangedKey, value); }
            remove { _theEventHandlers.RemoveHandler(_theOptionsChangedKey, value); }
        }

        /// <summary>
        /// Booelan value to enable Color Process Boundaries for Explain plans
        /// </summary>
        [Category("Application Settings")]
        [Description("Specifying if an Explain plan should be shown with color process boundaries.")]
        [DisplayName("Explain Plan Color Process Boundaries")]
        public bool EnableExplainColorProcessBoundaries
        {
            get { return _enableExplainColorProcessBoundaries; }
            set { _enableExplainColorProcessBoundaries = value; }
        }

        /// <summary>
        /// Booelan value to enable Sort Grid by Levels for Explain plans
        /// </summary>
        [Category("Application Settings")]
        [Description("Specifying whether or not an Explain plan grid is always sorted by levels.")]
        [DisplayName("Sort Explain Grid By Levels")]
        public bool EnableSortExplainGirdByLevels
        {
            get { return _enableSortExplainGridByLevels; }
            set { _enableSortExplainGridByLevels = value; }
        }

        #endregion

        #region Public Methods

        /// <summary>
        /// Gets the current instance of DatabaseAreaOptions
        /// </summary>
        /// <returns></returns>
        public static DatabaseAreaOptions GetOptions()
        {

            //Load DatabaseAreaOptions from the OptionStore persistence
            DatabaseAreaOptions options = OptionStore.GetOptionValues(Properties.Resources.Database, OptionsKey) as DatabaseAreaOptions;
            if (options == null)
            {
                //If DatabaseAreaOptions not available in OptionStore, create a default DatabaseAreaOptions
                options = new DatabaseAreaOptions();
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
        /// Save the current state of the DatabaseAreaOptions into the OptionStore
        /// </summary>
        public void Save()
        {
            OptionStore.SaveOptionValues(Properties.Resources.Database, OptionsKey, this);
            OnOptionsChanged();
        }

        public void OnOptionsChanged()
        {
            OnDatabaseOptionsChanged();
        }

       /// <summary>
       /// Handle the case when Options is loaded from persistence
       /// </summary>
       /// <param name="optionObject"></param>
        public void LoadedFromPersistence(Object optionObject)
        {
            DatabaseAreaOptions loadedOptions = optionObject as DatabaseAreaOptions;
            if(loadedOptions != null)
            {
                this.EnableExplainColorProcessBoundaries = loadedOptions.EnableExplainColorProcessBoundaries;
                this.EnableSortExplainGirdByLevels = loadedOptions.EnableSortExplainGirdByLevels;
            }
        }

        protected virtual void OnDatabaseOptionsChanged()
        {
            //EventHandler handler = DatabaseOptionsChanged;

            //if (handler != null)
            //{
            //    handler(this, new EventArgs());
            //}
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
            return (IOptionObject)(new DatabaseAreaOptions(this));
        }

        /// <summary>
        /// To copy options from another DatabaseAreaOptions object.
        /// </summary>
        /// <param name="obj"></param>
        public void Copy(IOptionObject obj)
        {
            DatabaseAreaOptions options = obj as DatabaseAreaOptions;
            if (obj != null)
            {
                this.EnableExplainColorProcessBoundaries = options.EnableExplainColorProcessBoundaries;
                this.EnableSortExplainGirdByLevels = options.EnableSortExplainGirdByLevels;
            }
        }

        #endregion Public Methods
    }
}
