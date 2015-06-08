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
using System.Reflection;
using Trafodion.Manager.Framework;

namespace PluginLoader
{
    /// <summary>
    /// Used to load plugins in separate appDomain so they can be unloaded due to incompatibilities or errors
    /// </summary>
    public class Loader : MarshalByRefObject
    {
        // Status returned upon load
        public const int Ok = 0;
        public const int Incompatible = 1;
        public const int Trafodionarea_not_found = 2;
        public const int Exception = 3;

        /// <summary>
        /// Default constructor
        /// </summary>
        public Loader()
        {
        }

        /// <summary>
        /// Test Loads the plugin and returns status of the load
        /// </summary>
        /// <param name="aPluginPath">full path to the plugin file</param>
        /// <param name="aFrameworkVersion">Version of the framework that is loading the dll</param>
        /// <returns>status of the load</returns>
        public int LoadInAppDomain(string aPluginPath, Version aFrameworkVersion)
        {            
            Assembly pluginAssembly = Assembly.LoadFrom(aPluginPath);
            
            // Check the version of the framework for compatibility
            if (aFrameworkVersion != null)
            {
                foreach (AssemblyName theAN in pluginAssembly.GetReferencedAssemblies())
                {
                    if (theAN.Name.Equals("Trafodion.Manager"))
                    {
                        Version pluginVersion = theAN.Version;
                        if ((pluginVersion.Major != aFrameworkVersion.Major) ||
                            (pluginVersion.Minor != aFrameworkVersion.Minor)
                            )
                        {
                            return Incompatible;
                        }
                        
                        break;
                    }
                }
            }            

            foreach (Type currType in pluginAssembly.GetTypes())
            {
                // Try to find the type that implements ITrafodionArea
                if (currType.GetInterface("ITrafodionArea") != null)
                {
                    ITrafodionArea theITrafodionArea = Activator.CreateInstance(currType) as ITrafodionArea;
                    return Ok;                    
                }
            }

            // No interface that implements ITrafodionArea was found
            return Trafodionarea_not_found;
        }
    }
}
