//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
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
using System.Xml;
using System.Xml.Serialization;

namespace Trafodion.Manager.LiveFeedFramework.Models
{

    [Serializable]
    public class LiveFeedDataProviderConfig : DataProviderConfig
    {
        #region Fields

        public enum LiveFeedDataFormat { DataTable = 0, ProtoBuf };

        [NonSerialized]
        protected LiveFeedDataProvider _theDataProvider = null;

        [NonSerialized]
        // This list keeps all of the configured subscriptions from the widget.
        // the key is the publcation name, the value is the array of filters
        private Dictionary<string, string[]> _theSubscriptionConfiguration = null;

        //[NonSerialized]
        private LiveFeedDataFormat _theDataFormat = LiveFeedDataFormat.DataTable;

        #endregion Fields

        #region Properties

        //[XmlIgnore]
        public override DataProviderTypes DataProviderType
        {
            get { return DataProviderTypes.MDD; }
        }

        /// <summary>
        /// Property: DataProvider
        /// </summary>
        public LiveFeedDataProvider TheDataProvider
        {
            get { return _theDataProvider; }
            set { _theDataProvider = value; }
        }

        /// <summary>
        /// Property: DataFormat
        /// </summary>
        public LiveFeedDataFormat TheDataFormat
        {
            get { return _theDataFormat; }
            set { _theDataFormat = value; }
        }

        /// <summary>
        /// Porperty: SubscriptionConfiguration - the list of subscriptions for the data provider
        /// </summary>
        public Dictionary<string, string[]> SubscriptionConfiguration
        {
            get { return _theSubscriptionConfiguration; }
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
        public LiveFeedDataProviderConfig()
        {
            ProviderName = "LiveFeed";
            _theSubscriptionConfiguration = new Dictionary<string, string[]>();
            _theDataFormat = LiveFeedDataFormat.DataTable;
        }

        /// <summary>
        /// The constructor 
        /// </summary>
        /// <param name="aDataFormat"></param>
        public LiveFeedDataProviderConfig(LiveFeedDataFormat aDataFormat)
            : this()
        {
            _theDataFormat = aDataFormat;
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
                if (TheDataFormat == LiveFeedDataFormat.DataTable)
                {
                    _theDataProvider = new LiveFeedDataProvider(this);
                }
                else
                {
                    _theDataProvider = new CachedLiveFeedProtoBufDataProvider(this);
                }
            }
            return _theDataProvider;
        }

        /// <summary>
        /// To configure a single publication 
        /// </summary>
        /// <param name="aPublication"></param>
        public void Configure(string aPublication)
        {
            if (_theSubscriptionConfiguration == null)
            {
                _theSubscriptionConfiguration = new Dictionary<string, string[]>();
            }

            _theSubscriptionConfiguration.Add(aPublication, null);
        }

        /// <summary>
        /// to configure an array of publcations
        /// </summary>
        /// <param name="aPublicationArray"></param>
        public void Configure(string[] aPublicationArray)
        {
            foreach (string pub in aPublicationArray)
            {
                Configure(pub);
            }
        }

        /// <summary>
        /// To configure a single publication with an array of filters
        /// </summary>
        /// <param name="aPublication"></param>
        /// <param name="aFilters"></param>
        public void Configure(string aPublication, string[] aFilters)
        {
            _theSubscriptionConfiguration.Add(aPublication, aFilters);
        }

        /// <summary>
        /// To cleanup existing subscription first and then add the provided list
        /// </summary>
        /// <param name="aPublcationArray"></param>
        public void ReConfigure(string[] aPublicationArray)
        {
            _theSubscriptionConfiguration.Clear();
            Configure(aPublicationArray);
        }

        #endregion Public methods
    }
}

