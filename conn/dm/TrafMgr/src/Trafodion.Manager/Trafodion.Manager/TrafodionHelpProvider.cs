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
using System.Windows.Forms;

namespace Trafodion.Manager.Framework
{
    public class TrafodionHelpProvider
    {
        public const string TrafodionHelpFile = "TrafodionHelp_user.chm";
        //public const string TrafodionHelpFileSupport = "TrafodionHelp_Support.chm";
        public const string SQQueryGuideFile = "SQ_QG.chm";

        private static TrafodionHelpProvider _instance;

        private TrafodionHelpProvider()
        {            
        }

        public static TrafodionHelpProvider Instance
        {
            get 
            { 
                if (_instance == null)
                {
                    _instance = new TrafodionHelpProvider();
                }
                return _instance;
            }
        }

        /// <summary>
        /// ShowHelp launches the correct help for support or trafodion users
        /// </summary>
        /// <param name="hn"></param>
        /// <param name="searchStr">Empty string place holder for "Find" Helpnavigator</param>
        public void ShowHelp(HelpNavigator hn, String searchStr)
        {
            //The help window is hosted inside a separated Form _helpContainerForm to make it non modal
            //with respect to the main TrafodionManager application
            String helpFile = GetHelpFile();
            Help.ShowHelp(TrafodionContext.Instance.TheTrafodionMain.HelpContainerForm, helpFile, hn, searchStr);
        }

        /// <summary>
        /// ShowTrafodionHelp launches the correct help for the specified help file
        /// </summary>
        /// <param name="hn"></param>
        /// <param name="helpFile">a specific help file</param>
        /// <param name="searchStr">Empty string place holder for "Find" Helpnavigator</param>
        public void ShowHelp(HelpNavigator hn, String helpFile, String searchStr)
        {
            //The help window is hosted inside a separated Form _helpContainerForm to make it non modal
            //with respect to the main TrafodionManager application
            Help.ShowHelp(TrafodionContext.Instance.TheTrafodionMain.HelpContainerForm, helpFile, hn, searchStr);
        }

        /// <summary>
        /// Displays a help topic
        /// </summary>
        /// <param name="fullTopicName"> The full topic name is the fully qualied topic name and would have a format
        ///                              like "auto_logon.html" or "i1004209.html#some_topic"
        /// </param>
        public void ShowHelpTopic(string fullTopicName)
        {
            ShowHelp(HelpNavigator.Topic, fullTopicName);
        }

        /// <summary>
        /// Displays a help topic from a specified help file
        /// </summary>
        /// <param name="helpFile">a specific help file</param>
        /// <param name="fullTopicName"> The full topic name is the fully qualied topic name and would have a format
        ///                              like "auto_logon.html" or "i1004209.html#some_topic"
        /// </param>
        public void ShowHelpTopic(string helpFile, string fullTopicName)
        {
            ShowHelp(HelpNavigator.Topic, helpFile, fullTopicName);
        }

        /// <summary>
        /// Gets the help file based on the user role.
        /// </summary>
        /// <returns></returns>
        private string GetHelpFile()
        {
            String helpFile = TrafodionHelpFile;
            return helpFile;
        }
    }
}
