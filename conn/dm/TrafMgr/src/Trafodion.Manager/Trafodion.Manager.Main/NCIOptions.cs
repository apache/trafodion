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
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.Main
{    
    /// <summary>
    /// Used to store TrafodionManager options for the NCI tool
    /// </summary>
    
    [Serializable]
    public class NCIOptions : IOptionObject
    {
        #region Member variables
        bool autoLogon;
        bool user;
        bool server;
        bool schema;
        bool customPrompt;
        String customPromptString;
        String _NCIExecutableFile;
        #endregion

        #region Constructor
        public NCIOptions() 
        {
            this.AutoLogon = Boolean.Parse(global::Trafodion.Manager.Properties.Resources.NCIAutoLogon);
            this.SetPromptUser = Boolean.Parse(global::Trafodion.Manager.Properties.Resources.NCIPromptUser);
            this.SetPromptServer = Boolean.Parse(global::Trafodion.Manager.Properties.Resources.NCIPromptHost);
            this.SetPromptSchema = Boolean.Parse(global::Trafodion.Manager.Properties.Resources.NCIPromptSchema);
            this.UseCustomPrompt = Boolean.Parse(global::Trafodion.Manager.Properties.Resources.NCIUseCustomPrompt);
            this.CustomPromptString = global::Trafodion.Manager.Properties.Resources.NCICustomPrompt;
            this.NCIExecutableFile = Utilities.Is64bitOS() ? global::Trafodion.Manager.Properties.Resources.NCIFilename64 :
                global::Trafodion.Manager.Properties.Resources.NCIFilename32;
        }

        /// <summary>
        /// Copy constructor
        /// </summary>
        /// <param name="options"></param>
        public NCIOptions(NCIOptions options)
        {
            this.AutoLogon = options.AutoLogon;
            this.SetPromptUser = options.SetPromptUser;
            this.SetPromptServer = options.SetPromptServer;
            this.SetPromptSchema = options.SetPromptSchema;
            this.UseCustomPrompt = options.UseCustomPrompt;
            this.CustomPromptString = options.CustomPromptString;
            this.NCIExecutableFile = options.NCIExecutableFile;
        }
        #endregion

        #region Properties
        public bool AutoLogon
        {
            get { return autoLogon; }
            set { autoLogon = value; }
        }

        public bool SetPromptUser
        {
            get { return user; }
            set { user = value; }
        }

        public bool SetPromptServer
        {
            get { return server; }
            set { server = value; }
        }

        public bool SetPromptSchema
        {
            get { return schema; }
            set { schema = value; }
        }


        public bool UseCustomPrompt
        {
            get { return customPrompt; }
            set { customPrompt = value; }
        }

        public String CustomPromptString
        {
            get { return customPromptString; }
            set { customPromptString = value; }
        }

        public String NCIExecutableFile
        {
            get { return _NCIExecutableFile; }
            set { _NCIExecutableFile = value; }
        }
        #endregion

        #region Public Methods
        /// <summary>
        /// Given a NCIOptions, updates the current object with the passed properties
        /// </summary>
        /// <param name="aModel"></param>
        public void SetModel(NCIOptions aModel)
        {
            if (aModel != null)
            {
                this.autoLogon = aModel.AutoLogon;
                this.user = aModel.SetPromptUser;
                this.server = aModel.SetPromptServer;
                this.schema = aModel.SetPromptSchema;
                this.customPrompt = aModel.UseCustomPrompt;
                this.customPromptString = aModel.CustomPromptString;
                this._NCIExecutableFile = aModel.NCIExecutableFile;
            }
        }

        /// <summary>
        /// Returns the prompt string for NCI
        /// </summary>
        /// <returns></returns>
        public String GetPromptString()
        {
            StringBuilder promptStr = new StringBuilder("\"");

            if (SetPromptUser)
                promptStr.Append("%User ");
            if (SetPromptServer)
                promptStr.Append("%Server ");
            if (SetPromptSchema)
                promptStr.Append("%Schema ");

            promptStr.Append("SQL>\"");

            return promptStr.ToString();
        }

        /// <summary>
        /// Just to be compatible with interface
        /// </summary>
        public void OnOptionsChanged()
        {
        }

        /// <summary>
        /// Just to be compatible with interface; the control does the persistence anyway.
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
            return (IOptionObject)(new NCIOptions(this));
        }

        /// <summary>
        /// To copy options from another TraceOptions object.
        /// </summary>
        /// <param name="obj"></param>
        public void Copy(IOptionObject obj)
        {
            SetModel(obj as NCIOptions);
        }

        #endregion
        
    }

}
