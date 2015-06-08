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

namespace Trafodion.Manager
{
    [Serializable]
    public class GeneralOptions : IOptionObject
    {
        public const string GeneralOptionsKey = "Framework";

        Int32 _connectionTimeOut = 180;
        Int32 _commandTimeout = 180;


        public GeneralOptions()
        {
            _connectionTimeOut = 180;
            _commandTimeout = 180;
        }

        /// <summary>
        /// Copy constructor
        /// </summary>
        /// <param name="options"></param>
        public GeneralOptions(GeneralOptions options)
        {
            _connectionTimeOut = options.ConnectionTimeOut;
            _commandTimeout = options.CommandTimeOut;
        }

        #region Properties

        /// <summary>
        /// Booelan value to show or hide system views
        /// </summary>
        [Category("Application Settings")]
        [Description("An integer value from 0 to 2,147,483,647 specifying the odbc connection timeout in seconds. Overrides the connection timeout specified in the client datasource.")]
        [DisplayName("Connection Time Out")]
        public Int32 ConnectionTimeOut
        {
            get { return _connectionTimeOut; }
            set 
            {
                if (value >= 0)
                {
                    _connectionTimeOut = value;
                }
            }
        }

        /// <summary>
        /// Integer value to control the ODBC command timeout.
        /// </summary>
        [Category("Application Settings")]
        [Description("An integer value from 0 to 2,147,483,647 specifying the odbc command timeout in seconds. Overrides the query timeout specified in the client datasource. Value of 0 indicates no timeout.")]
        [DisplayName("Command Time Out")]
        public Int32 CommandTimeOut
        {
            get { return _commandTimeout; }
            set 
            {
                if (value >= 0)
                {
                    _commandTimeout = value;
                }
            }
        }

        #endregion

        /// <summary>
        /// Gets the current instance of DatabaseAreaOptions
        /// </summary>
        /// <returns></returns>
        public static GeneralOptions GetOptions()
        {

            //Load GeneralOptions from the OptionStore persistence
            GeneralOptions options = OptionStore.GetOptionValues("General", GeneralOptionsKey) as GeneralOptions;
            if (options == null)
            {
                //If GeneralOptions not available in OptionStore, create a default GeneralOptions
                options = new GeneralOptions();
                try
                {
                    //Save the GeneralOptions to the OptionStore, so a subsequent GetOptions call would find it
                    options.Save();
                }
                catch (Exception ex)
                {
                }
            }
            return options;
        }

        public void Save()
        {
            OptionStore.SaveOptionValues("General", GeneralOptionsKey, this);
            OnOptionsChanged();
        }

        /// <summary>
        /// Called when the user clicks the Ok button on the options panel.
        /// </summary>
        public void OnOptionsChanged()
        {
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
        /// To clone self for option display
        /// </summary>
        /// <returns></returns>
        public IOptionObject Clone()
        {
            return (IOptionObject)(new GeneralOptions(this));
        }

        /// <summary>
        /// To copy all of the options from another option object.
        /// </summary>
        /// <param name="obj"></param>
        public void Copy(IOptionObject obj)
        {
            GeneralOptions options = obj as GeneralOptions;
            if (options != null)
            {
                this.CommandTimeOut = options.CommandTimeOut;
                this.ConnectionTimeOut = options.ConnectionTimeOut;
            }
        }
    }
}
