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
    /// A wrapper class on Persistence to manage the persistance of the options.
    /// </summary>
    public class OptionStore
    {
        public const string OptionsKey = "OPTIONS_KEY";
        public OptionStore()
        {
        }

        /// <summary>
        /// Gets the Option object from persistence 
        /// </summary>
        /// <param name="optionProviderName"></param>
        /// <param name="optionName"></param>
        /// <returns></returns>
        public static Object GetOptionValues(string optionProviderName, string optionName)
        {
            Dictionary<string, Dictionary<string, Object>> storedOptions = getStoredOptions();
            Dictionary<string, Object> providerOptions = null;
            if (storedOptions.ContainsKey(optionProviderName))
            {
                providerOptions = storedOptions[optionProviderName] as Dictionary<string, Object>;
                if (providerOptions.ContainsKey(optionName))
                {
                    return providerOptions[optionName];
                }
            }
            return null;
        }

        /// <summary>
        /// Saves the options object
        /// </summary>
        /// <param name="optionProviderName"></param>
        /// <param name="optionName"></param>
        /// <param name="optionValue"></param>
        public static void SaveOptionValues(string optionProviderName, string optionName, Object optionValue)
        {
            Dictionary<string, Dictionary<string, Object>> storedOptions = getStoredOptions();
            Dictionary<string, Object> savedOptions = null;
            if (storedOptions.ContainsKey(optionProviderName))
            {
                savedOptions = storedOptions[optionProviderName] as Dictionary<string, Object>;
            }
            else
            {
                savedOptions = new Dictionary<string, Object>();
                storedOptions.Add(optionProviderName, savedOptions);
            }

            //if (savedOptions.ContainsKey(optionName)) savedOptions.Remove(optionName);
            //savedOptions.Add(optionName, optionValue);
            if (!savedOptions.ContainsKey(optionName))
            {
                savedOptions.Add(optionName, optionValue);
            }
            else if (optionValue is IOptionObject)
            {
                IOptionObject obj = (IOptionObject)savedOptions[optionName];
                obj.Copy((IOptionObject)optionValue);
            }
            else
            {
                if (savedOptions.ContainsKey(optionName)) savedOptions.Remove(optionName);
                savedOptions.Add(optionName, optionValue);
            }

            Persistence.SaveAllToDefault();
        }

        private static Dictionary<string, Dictionary<string, Object>> getStoredOptions()
        {
            Dictionary<string, Dictionary<string, Object>> storedOptions = 
                Persistence.Get(OptionsKey) as Dictionary<string, Dictionary<string, Object>>;

            if (storedOptions == null)
            {
                storedOptions = new Dictionary<string, Dictionary<string, Object>>();
                Persistence.Put(OptionsKey, storedOptions);
            }
            return storedOptions;
        }



    }
}
