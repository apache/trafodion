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

using Microsoft.VisualStudio.Data;
using Microsoft.VisualStudio.Data.AdoDotNet;

namespace Trafodion.Data.VisualStudio
{
    public class TrafodionDbDataSourceInformation : AdoDotNetDataSourceInformation
    {
        public TrafodionDbDataSourceInformation(DataConnection connection)
            : base(connection)
        {
            AddProperty(SupportsAnsi92Sql, true);
            //AddProperty(IdentifierPartsCaseSensitive, false);
            AddProperty(SupportsQuotedIdentifierParts, true);
            AddProperty(IdentifierOpenQuote, "\"");
            AddProperty(IdentifierCloseQuote, "\"");
            AddProperty(QuotedIdentifierPartsStorageCase, "M");
            AddProperty(QuotedIdentifierPartsCaseSensitive, true);
            
            AddProperty(ServerSeparator, ".");
            AddProperty(CatalogSupported, true);
            AddProperty(CatalogSupportedInDml, true);
            AddProperty(CatalogSeparator, ".");
            AddProperty(SchemaSupported, true);
            AddProperty(SchemaSupportedInDml, true);
            AddProperty(SchemaSeparator, ".");
           
        }

        public override object this[string propertyName]
        {
            get
            {
                return base[propertyName];
            }
        }
    }
}
