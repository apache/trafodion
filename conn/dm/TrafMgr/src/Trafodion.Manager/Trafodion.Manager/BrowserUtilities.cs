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

using System.Diagnostics;
using Trafodion.Manager.Framework.Connections;
using System;
using System.Windows.Forms;

namespace Trafodion.Manager.Framework
{
    /// <summary>
    /// Utility class for launching browser windows
    /// </summary>
    static public class BrowserUtilities
    {

        private const string dialOutEventsURL = ":9991/?AS=$ALT&ANALYSIS=OPVIEW&GE=5000&BRST=";
        private const string databaseEventsURL = ":9991/?AS=$DBA&ANALYSIS=OPVIEW&GE=5000&BRST=";

        /// <summary>
        /// Reads path of default browser from registry
        /// </summary>
        /// <returns></returns>
        static public string GetDefaultBrowserPath()
        {
            string key = @"http\shell\open\command";
            Microsoft.Win32.RegistryKey registryKey =
            Microsoft.Win32.Registry.ClassesRoot.OpenSubKey(key, false);
            string path = "";
            if (registryKey != null)
            {
                // get default browser path
               path = ((string)registryKey.GetValue(null, null)).Split('"')[1];
               registryKey.Close();
            }
            return path;
        }

        /// <summary>
        /// Launches URL in the default browser
        /// </summary>
        /// <param name="url"></param>
        static public void LaunchURL(string url)
        {
            try
            {
                //Process.Start(GetDefaultBrowserPath(), url);
                System.Diagnostics.Process.Start(@url);
            }
            catch (Exception ex)
            {
                MessageBox.Show(Properties.Resources.ErrorOpeningBrowser + ex.ToString());
            }
        }

        /// <summary>
        /// Launch Database Events page
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        static public void LaunchDatabaseEvents(ConnectionDefinition aConnectionDefinition)
        {
            LaunchEvents(aConnectionDefinition, true);
        }

        /// <summary>
        /// Launch Dial Out Events page
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        static public void LaunchDialOutEvents(ConnectionDefinition aConnectionDefinition)
        {
            LaunchEvents(aConnectionDefinition, false);
        }

        static private void LaunchEvents(ConnectionDefinition aConnectionDefinition, bool aDatabaseEvent)
        {
            string theTempFileName = System.IO.Path.GetTempFileName() + ".html";
            string theTemplate = System.IO.File.ReadAllText(Application.StartupPath + "/OSMEventViewerTemplate.html");
            string theURL = String.Format("https://{0}:9991/?Events={1}", 
                                          aConnectionDefinition.Server, 
                                          (aDatabaseEvent ? "Database" : "DialOut"));
            string theRole = aDatabaseEvent ? @"$DBA" : @"$ALT";
            string theText = String.Format(theTemplate, @theURL, @aConnectionDefinition.UserName, @theRole, "{", "}");
            System.IO.File.WriteAllText(theTempFileName, theText);
            string url = String.Format("file://{0}", theTempFileName);
            LaunchURL(url);
        }
    }
}
