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
    /// This is a well-known singleton that lets independent components publish objects that might be of interest
    /// to other components.
    /// </summary>
    static public class GlobalRegistry
    {
        static public void Insert(string aKey, object aValue)
        {
            if (theGlobalRegistry.ContainsKey(aKey))
            {
                throw new ApplicationException("Insert: Key " + aKey + " already in use in " + Properties.Resources.ProductName + " Global Resistry");
            }
            theGlobalRegistry.Add(aKey, aValue);
        }

        static public void Update(string aKey, object aValue)
        {
            if (!theGlobalRegistry.ContainsKey(aKey))
            {
                throw new ApplicationException("Update: Key " + aKey + " not found in " + Properties.Resources.ProductName + " Global Resistry");
            }
            theGlobalRegistry.Remove(aKey);
            theGlobalRegistry.Add(aKey, aValue);
        }

        static public void Delete(string aKey)
        {
            if (!theGlobalRegistry.ContainsKey(aKey))
            {
                throw new ApplicationException("Delete: Key " + aKey + " not found in " + Properties.Resources.ProductName + " Global Resistry");
            }
            theGlobalRegistry.Remove(aKey);
        }

        static public object Lookup(string aKey)
        {
            if (!theGlobalRegistry.ContainsKey(aKey))
            {
                throw new ApplicationException("Lookup: Key " + aKey + " not found in use in " + Properties.Resources.ProductName + " Global Resistry");
            }
            return theGlobalRegistry[aKey];
        }

        static public bool Exists(string aKey)
        {
            return (theGlobalRegistry.ContainsKey(aKey));
        }

        static private Dictionary<string, object> theGlobalRegistry = new Dictionary<string, object>();
    }

}
