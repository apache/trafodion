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
using System.IO;
using System.Collections;
using System.Collections.Generic;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.Main
{
    /// <summary>
    /// This class will manage the launching of the NCI tool
    /// </summary>
    class NCIHandler : IOptionsProvider
    {
        #region public methods
        /// <summary>
        /// Code to launch NCI
        /// </summary>
        /// <param name="aCurrentConnectionDefinition"></param>
        public void LaunchNCI(ConnectionDefinition aCurrentConnectionDefinition)
        {
            try
            {
                NCIOptionControl optionControl = new NCIOptionControl();
                NCIOptions Options = OptionStore.GetOptionValues(global::Trafodion.Manager.Properties.Resources.NCI,
                    optionControl.OptionTitle) as NCIOptions;

                optionControl.Dispose();
                Options = (Options == null) ? new NCIOptions() : Options;
                String fileName = Options.NCIExecutableFile;

                System.Text.StringBuilder nciArgs = new System.Text.StringBuilder("");


                //Set up autologon items
                if (aCurrentConnectionDefinition != null && Options.AutoLogon)
                {
                    String userName = aCurrentConnectionDefinition.UserName;
                    String passWord = aCurrentConnectionDefinition.Password;
                    String dataSource = aCurrentConnectionDefinition.ClientDataSource;
                    String host = aCurrentConnectionDefinition.Host + ":" + aCurrentConnectionDefinition.Port;
                    String schema = aCurrentConnectionDefinition.SchemaForConnectionString;
                    String role = aCurrentConnectionDefinition.UserSpecifiedRole;

                    if (String.IsNullOrEmpty(dataSource))
                    {
                        dataSource = "TDM_Default_DataSource";
                    }

                    if (String.IsNullOrEmpty(role))
                    {
                        role = "";
                    }


                    string path = Path.Combine(Persistence.HomeDirectory, "NciStartup.txt");
                    if (File.Exists(path))
                    {
                        File.Delete(path);
                    }

                    // Create a file to write to. The "using" statement here insures
                    //that the dispose function is called in all cases even an exception.  
                    //It also ensures that the stream writer is closed when out of scope..DS
                    using (StreamWriter sw = File.CreateText(path))
                    {
                        //If controlled schema access is enabled, do not do a set schema. This will throw a sql error
                        if (!String.IsNullOrEmpty(schema) && !aCurrentConnectionDefinition.IsControlledSchemaAccessEnabled)
                        {
                            sw.WriteLine("Set Schema " + schema + ";");
                        }
                        if (Options.UseCustomPrompt)
                        {
                            sw.WriteLine("Set Prompt " + Options.CustomPromptString);
                        }
                        else
                        {
                            sw.WriteLine("Set Prompt " + Options.GetPromptString());
                        }
                    }

                    //Do not use using autologon if there is no password.  Nci needs all arguments in order to
                    //auto launch.  
                    if (!String.IsNullOrEmpty(passWord))
                    {
                        nciArgs.AppendFormat("-u \"{0}\" -p \"{1}\" -h {2} -dsn {3} -s \"{4}\" -r \"{5}\" ", 
                                    new string[] {userName , passWord, host, dataSource, path, role});
                    }
                }

                System.Diagnostics.Process nciProcess = null;
                if (Utilities.LaunchProcess(ref fileName, nciArgs.ToString(), global::Trafodion.Manager.Properties.Resources.NCI, out nciProcess))
                {
                    Options.NCIExecutableFile = fileName;
                    OptionStore.SaveOptionValues(global::Trafodion.Manager.Properties.Resources.NCI, global::Trafodion.Manager.Properties.Resources.NCIOptions, Options);
                }

                if (nciProcess != null && nciProcess.HasExited)
                {
                    if (MessageBox.Show(global::Trafodion.Manager.Properties.Resources.NCIArgumentErrorText, global::Trafodion.Manager.Properties.Resources.NCIStartupError, MessageBoxButtons.YesNo, MessageBoxIcon.Error) == DialogResult.Yes)
                    {
                        Utilities.LaunchProcess(ref fileName, "", global::Trafodion.Manager.Properties.Resources.NCI, out nciProcess);
                    }
                }

            }
            catch (Exception ex)
            {
                MessageBox.Show(Utilities.GetForegroundControl(),
                    Properties.Resources.Error + "\n" + ex.Message,
                    global::Trafodion.Manager.Properties.Resources.NCIStartupError,
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
            }
        }
        #endregion

        #region IOptionsProvider

        /// <summary>
        /// Property that the framework reads to get the options control
        /// </summary>
        public List<IOptionControl> OptionControls
        {
            get
            {
                List<IOptionControl> list = new List<IOptionControl>();
                list.Add(new NCIOptionControl());
                return list;
            }
        }

        public Dictionary<String, IOptionObject> OptionObjects
        {
            get
            {
                return null;
            }
        }
        #endregion

    }
}
