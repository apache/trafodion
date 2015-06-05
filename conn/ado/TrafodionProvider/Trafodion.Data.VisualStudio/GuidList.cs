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

namespace Trafodion.Data.VisualStudio
{
    internal class GuidList
    {
        //its not worth storing these as actual Guid objects as we only want the String values for the registry.

        //abstract
        public const string TrafodionDbDatasource = "{5D308DC4-BED8-4093-BEE7-FDA96AFFB376}";
        public const string TrafodionDbProvider = "{77E965BD-B4E2-4365-827C-EB11724C1CF1}";
        
        //classes
        public const string TrafodionDbProviderObjectFactory = "{113C3AC2-17B0-4099-9175-54E32292A5BF}";
        public const string TrafodionDbDataPackage = "{F60A05D1-9D3B-42F3-B7E3-3D4924399FAB}";
    }
}
