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


namespace Trafodion.Manager.Framework
{
    
    /// <summary>
    /// Used to store TrafodionManager options
    /// </summary>
    public class Options
    {
       
        public bool AutoLogon
        {
            get { return Properties.Settings.Default.NCIAutoLogon2; }
            set 
            {
 
                Properties.Settings.Default.NCIAutoLogon2 = value;
                Properties.Settings.Default.Save();
            }
        }
    
        public bool SetPromptUser
        {
            get{return Properties.Settings.Default.NCIPromptUser;}
            set { 
                Properties.Settings.Default.NCIPromptUser = value;
                Properties.Settings.Default.Save();
            }
        }
        
        public bool SetPromptServer
        {
            get { return Properties.Settings.Default.NCIPromptHost; }
            set { 
                Properties.Settings.Default.NCIPromptHost = value;
                Properties.Settings.Default.Save();
            }
        }
        public bool SetPromptSchema
        {
            get { return Properties.Settings.Default.NCIPromptSchema; }
            set { 
                Properties.Settings.Default.NCIPromptSchema = value;
                Properties.Settings.Default.Save();
            }
        }


        public String GetPromptString()
        {
            StringBuilder promptStr = new StringBuilder("\"");
            
            if(SetPromptUser)
                promptStr.Append("%User ");
            if (SetPromptServer)
                promptStr.Append("%Server ");
            if (SetPromptSchema)
                promptStr.Append("%Schema ");

            promptStr.Append("SQL>\"");

            return promptStr.ToString();
            
            
        }

        public bool UseCustomPrompt
        {
            get { return Properties.Settings.Default.NCIUseCustomPrompt; }
            set
            {
                Properties.Settings.Default.NCIUseCustomPrompt = value;
                Properties.Settings.Default.Save();
            }

        }

        public String CustomPromptString
        {
            get { return Properties.Settings.Default.NCICustomPrompt; }
            set 
            {
              Properties.Settings.Default.NCICustomPrompt = value;
              Properties.Settings.Default.Save();
            }
        }
    
    }

}
