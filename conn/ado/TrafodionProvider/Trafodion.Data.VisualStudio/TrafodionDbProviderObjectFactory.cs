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

using System;
using System.Runtime.InteropServices;
using Microsoft.VisualStudio.Data;
using Microsoft.VisualStudio.Data.AdoDotNet;

namespace Trafodion.Data.VisualStudio
{
    [Guid("113C3AC2-17B0-4099-9175-54E32292A5BF")]
    public sealed class TrafodionDbProviderObjectFactory : AdoDotNetProviderObjectFactory
    {
        public override object CreateObject(Type objType)
        {
            if (objType == typeof(DataConnectionPromptDialog))
            {
                return new TrafodionDbDataConnectionPromptDialog();
            }
            if (objType == typeof(DataConnectionProperties))
            {
                return new TrafodionDbConnectionProperties();
            }
            if (objType == typeof(DataConnectionSupport))
            {
                return new TrafodionDbConnectionSupport();
            }
            if (objType == typeof(DataConnectionUIControl))
            {
                return new TrafodionDbConnectionUIControl();
            }
            if (objType == typeof(DataSourceSpecializer))
            {
                return new TrafodionDbDataSourceSpecializer();
            }

            return base.CreateObject(objType);
        }
    }
}
