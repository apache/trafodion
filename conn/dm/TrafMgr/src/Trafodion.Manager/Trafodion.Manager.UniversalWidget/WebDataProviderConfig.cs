//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2012-2015 Hewlett-Packard Development Company, L.P.
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
using System.Xml;
using Trafodion.Manager.UniversalWidget;
using System.Collections.Generic;
using Trafodion.Manager.Framework.Connections;
using System.Xml.Serialization;

namespace Trafodion.Manager.UniversalWidget
{
    [Serializable]
    public class WebDataProviderConfig : DataProviderConfig
    {
        #region Fields

        [NonSerialized]
        protected WebDataProvider _theDataProvider = null;
        private string _theRootURL = null;
        private string _theURL = null;
        private string _methodType = null;
        private string _contentType = null;

        //private Dictionary<string, string> _parameters = new Dictionary<string, string>();
        private string postParameters = "";

        public string PostParameters
        {
            get { return postParameters; }
            set { postParameters = value; }
        }


        #endregion Fields

        #region Properties

        public override DataProviderTypes DataProviderType
        {
            get { return DataProviderTypes.MDD; }
        }

        /// <summary>
        /// Property: DataProvider
        /// </summary>
        public WebDataProvider TheDataProvider
        {
            get { return _theDataProvider; }
            set { _theDataProvider = value; }
        }

        public string TheRootURL
        {
            get { return _theRootURL; }
            set { _theRootURL = value; }
        }

        /// <summary>
        /// Property: The URL for the web request
        /// </summary>
        public string TheURL
        {
            get { return _theURL; }
            set { _theURL = value; }
        }

        public string MethodType
        {
            get { return string.IsNullOrEmpty(_methodType) ? "GET" : _methodType; }
            set { _methodType = value; }
        }

        public string ContentType
        {
            get { return string.IsNullOrEmpty(_methodType) ? "application/json;" : _contentType; }
            set { _contentType = value; }
        }

        /// <summary>
        /// Property: ConnectionDefinition - the connection definition
        /// </summary>
        [XmlIgnore]
        public override ConnectionDefinition ConnectionDefinition
        {
            get { return base._theConnectionDefinition; }
            set
            {
                base._theConnectionDefinition = value;
            }
        }
        #endregion Properties

        #region Constructors

        /// <summary>
        /// The constructor
        /// </summary>
        public WebDataProviderConfig()
        {
            ProviderName = "Web";
        }

        #endregion Constructors

        #region Public methods

        /// <summary>
        /// Adds additional functionality to read the SQL from the config
        /// </summary>
        /// <param name="dom"></param>
        public override void PopulateFromXML(XmlDocument dom)
        {
            base.PopulateFromXML(dom);
        }

        /// <summary>
        /// Returns a new DatabaseDataProvider using this config
        /// </summary>
        /// <returns></returns>
        public override DataProvider GetDataProvider()
        {
            if (_theDataProvider == null)
            {
                _theDataProvider = new WebDataProvider(this);
            }
            return _theDataProvider;
        }


        #endregion Public methods
    }
}

