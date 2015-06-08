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

namespace Trafodion.Manager.SecurityArea.Model
{
    /// <summary>
    /// Object to store Security Area options
    /// </summary>
    [Serializable]
    public class SecurityAreaOptions : IOptionObject
    {
        #region Member variables

        private bool _defaultAddLocalDBUser;
        public const string OptionsKey = "Security Options";

        [field: NonSerialized]
        public event EventHandler SecurityAreaOptionsChanged;

        #endregion

        #region Constructor

        /// <summary>
        /// Constructs the default SecurityArea Options
        /// </summary>
        public SecurityAreaOptions()
        {
            _defaultAddLocalDBUser = true;
        }

        /// <summary>
        /// Copy constructor
        /// </summary>
        /// <param name="options"></param>
        public SecurityAreaOptions(SecurityAreaOptions options)
        {
            _defaultAddLocalDBUser = options._defaultAddLocalDBUser;
        }

        #endregion

        #region Properties

        /// <summary>
        /// Booelan value to indicate if Add user dialog should default to locally authenticated user type
        /// </summary>
        [Category("Display")]
        [Description("If set to true, the Add Database User dialog would default to locally authenticated user type. If set to false, it would default to remotely authenticated user type.")]
        public bool DefaultAddDatabaseUserLocally
        {
            get { return _defaultAddLocalDBUser; }
            set { _defaultAddLocalDBUser = value; }
        }

        #endregion

        #region Public Methods

        /// <summary>
        /// Gets the current instance of SecurityAreaOptions
        /// </summary>
        /// <returns></returns>
        public static SecurityAreaOptions GetOptions()
        {

            //Load SecurityAreaOptions from the OptionStore persistence
            SecurityAreaOptions options = OptionStore.GetOptionValues(Properties.Resources.SecurityAreaName, OptionsKey) as SecurityAreaOptions;
            if (options == null)
            {
                //If SecurityAreaOptions not available in OptionStore, create a default SecurityAreaOptions
                options = new SecurityAreaOptions();
                try
                {
                    //Save the SecurityAreaOptions to the OptionStore, so a subsequent GetOptions call would find it
                    options.Save();
                }
                catch (Exception ex)
                {
                }
            }
            return options;
        }

        /// <summary>
        /// Save the current state of the SecurityAreaOptions into the OptionStore
        /// </summary>
        public void Save()
        {
            OptionStore.SaveOptionValues(Properties.Resources.SecurityAreaName, OptionsKey, this);
            OnOptionsChanged();
        }

        public void OnOptionsChanged()
        {
            OnSecurityAreaOptionsChanged();
        }

        /// <summary>
        /// Handle the case when Options is loaded from persistence
        /// </summary>
        /// <param name="optionObject"></param>
        public void LoadedFromPersistence(Object optionObject)
        {
            SecurityAreaOptions loadedOptions = optionObject as SecurityAreaOptions;
            if (loadedOptions != null)
            {
                _defaultAddLocalDBUser = loadedOptions._defaultAddLocalDBUser;
            }
        }

        protected virtual void OnSecurityAreaOptionsChanged()
        {
            EventHandler handler = SecurityAreaOptionsChanged;

            if (handler != null)
            {
                handler(this, new EventArgs());
            }
        }

        /// <summary>
        /// Clone the options for display purposes
        /// </summary>
        /// <returns></returns>
        public IOptionObject Clone()
        {
            return (IOptionObject)(new SecurityAreaOptions(this));
        }

        /// <summary>
        /// To copy options from another options object
        /// </summary>
        /// <param name="obj"></param>
        public void Copy(IOptionObject obj)
        {
            SecurityAreaOptions options = obj as SecurityAreaOptions;
            if (options != null)
            {
                this.DefaultAddDatabaseUserLocally = options.DefaultAddDatabaseUserLocally;
            }
        }

        #endregion Public Methods
    }
}
