/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2009-2015 Hewlett-Packard Development Company, L.P.
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
********************************************************************/

namespace Trafodion.Data
{
    using System.Reflection;
    using System.Resources;

    /// <summary>
    /// Collection of static methods used to interact with bundled resources.
    /// </summary>
    internal class TrafodionDBResources
    {
        private static readonly ResourceManager Messages = new ResourceManager("Trafodion.Data.Properties.Messages", Assembly.GetExecutingAssembly());

        internal static string FormatMessage(TrafodionDBMessage msg, params object [] p)
        {
            return string.Format(TrafodionDBResources.Messages.GetString(msg.ToString()), p);;
        }

        internal static string GetMessage(TrafodionDBMessage msg)
        {
            return TrafodionDBResources.Messages.GetString(msg.ToString());
        }

        /*internal static string GetMessage(string msg)
        {
            return TrafodionDBResources.Messages.GetString(msg);
        }*/
    }
}
