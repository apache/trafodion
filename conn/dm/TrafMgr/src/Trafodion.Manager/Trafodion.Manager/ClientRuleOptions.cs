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
    public class ClientRuleOptions : IOptionObject
    {
        #region Fields
        /// <summary>
        /// ClientRule option Key
        /// </summary>
        public const string ClientRuleOptionsKey = "Client Rule Folder";

        // Private members
        private string clientRuleFolderName = "";        

        #endregion Fields

        #region Constructors

        /// <summary>
        /// Constructors
        /// </summary>
        private ClientRuleOptions()
        {
            clientRuleFolderName = Persistence.HomeDirectory + "\\" + Trafodion.Manager.Properties.Resources.DefaultClientRuleFolder;           
        }

        /// <summary>
        /// Copy constructor
        /// </summary>
        /// <param name="options"></param>
        public ClientRuleOptions(ClientRuleOptions options)
        {
            clientRuleFolderName = options.ClientRuleFolderName;            
        }

        #endregion Constructors

        #region Properties
        /// <summary>
        /// Error Client Rule Folder Name
        /// </summary>
        [Category("Application Settings")]
        [Description("Specifying the Client Rule folder name. This attribute will be persisted.")]
        [DisplayName("Client Rule Folder Name")]
        public String ClientRuleFolderName
        {
            get
            {
                if (String.IsNullOrEmpty(clientRuleFolderName))
                {
                    clientRuleFolderName = Persistence.HomeDirectory + "\\" + Trafodion.Manager.Properties.Resources.DefaultClientRuleFolder;   
                }

                return clientRuleFolderName;
            }
            set
            {
                if (!string.IsNullOrEmpty(value))
                {
                    if (Utilities.IsValidFileName(value))
                    {
                        clientRuleFolderName = value;
                    }
                }
            }
        }
                
        #endregion

        /// <summary>
        /// Gets the current instance of ClientRuleOptions
        /// </summary>
        /// <returns></returns>
        public static ClientRuleOptions GetOptions()
        {

            //Load ClientRuleOptions from the OptionStore persistence
            ClientRuleOptions options = OptionStore.GetOptionValues("General", ClientRuleOptionsKey) as ClientRuleOptions;
            if (options == null)
            {
                //If ClientRuleOptions not available in OptionStore, create a default ClientRuleOptions
                options = new ClientRuleOptions();
                try
                {
                    //Save the ClientRuleOptions to the OptionStore, so a subsequent GetOptions call would find it
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
            OptionStore.SaveOptionValues("General", ClientRuleOptionsKey, this);
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
        /// Close self for display
        /// </summary>
        /// <returns></returns>
        public IOptionObject Clone()
        {
            return (IOptionObject)(new ClientRuleOptions(this));
        }

        /// <summary>
        /// To copy options from another ClientRuleOptions object.
        /// </summary>
        /// <param name="obj"></param>
        public void Copy(IOptionObject obj)
        {
            ClientRuleOptions options = obj as ClientRuleOptions;
            if (options != null)
            {
                this.ClientRuleFolderName = options.ClientRuleFolderName;
                
            }
        }
    }
}

