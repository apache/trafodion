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
using System.Xml;
using System.Xml.Serialization;
namespace Trafodion.Manager.UniversalWidget
{
    /// <summary>
    /// Configuration data needed for fetching data from a DB
    /// </summary>
    [Serializable]
    public class DatabaseDataProviderConfig : DataProviderConfig
    {
        private string _theSchemaName;
        private string _theCatalogName;
        private string _theOpenCommand;
        private string _theSQLText;
        private string _theCloseCommand;
        private int _theCommandTimeout = -1;

        public DatabaseDataProviderConfig()
        {
            ProviderName = "DB";
        }


        public string CatalogName
        {
            get { return _theCatalogName; }
            set { _theCatalogName = value; }
        }

        public string SchemaName
        {
            get { return _theSchemaName; }
            set { _theSchemaName = value; }
        }

        [XmlIgnore]
        public override DataProviderTypes DataProviderType
        {
            get { return DataProviderTypes.DB; }
        }

        [XmlElement("OpenCommand")]
        public string OpenCommand
        {
            get 
            {
                if ((_theOpenCommand != null) && (_theOpenCommand.Trim().Length > 0))
                {
                    return _theOpenCommand.Trim();
                }
                return null;
            }
            set { _theOpenCommand = value; }
        }

        [XmlElement("SQLText")]
        public string SQLText
        {
            get 
            { 
                return _theSQLText; 
            }
            set { _theSQLText = value; }
        }
        [XmlElement("CloseCommand")]
        public string CloseCommand
        {
            get
            {
                if ((_theCloseCommand != null) && (_theCloseCommand.Trim().Length > 0))
                {
                    return _theCloseCommand.Trim();
                }
                return null;
            }
            set { _theCloseCommand = value; }
        }

        [XmlElement("CommandTimeout")]
        public int CommandTimeout
        {
            get { return _theCommandTimeout; }
            set { _theCommandTimeout = value; }
        }

        /// <summary>
        /// Adds additional functionality to read the SQL from the config
        /// </summary>
        /// <param name="dom"></param>
        public override void PopulateFromXML(XmlDocument dom)
        {
            base.PopulateFromXML(dom);
            PopulateSQL(dom);
        }

        /// <summary>
        /// Returns a new DatabaseDataProvider using this config
        /// </summary>
        /// <returns></returns>
        public override DataProvider GetDataProvider()
        {
            //return new DatabaseDataProvider(this);
            //return null;
            throw new Exception("Internal Error: The DataProvider has to be manually created before a UniversalWidgetConfiguration can be assigned to the Widget."
                                    + " Please assign a DataProvider to the widget right after the widget is created");
        }
        /// <summary>
        /// Gets the SQL text from the XML and populates the config
        /// </summary>
        /// <param name="dom"></param>
        private void PopulateSQL(XmlDocument dom)
        {
            XmlNodeList nodes = dom.SelectNodes("/WidgetConfig/DataProviderConfig/SQLText");
            if ((nodes != null) && (nodes.Count > 0))
            {
                XmlNode sqlNode = nodes[0];
                if (sqlNode.FirstChild.NodeType == XmlNodeType.CDATA)
                {
                    this.SQLText = sqlNode.FirstChild.Value;
                }
            }
        }

    }
}
