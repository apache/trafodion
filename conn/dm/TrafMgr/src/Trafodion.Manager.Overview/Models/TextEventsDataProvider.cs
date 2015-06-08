#region Copyright info
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
#endregion Copyright info
using System;
using System.Collections;
using Trafodion.Manager.DatabaseArea;
using Trafodion.Manager.Framework;
using Trafodion.Manager.UniversalWidget;

namespace Trafodion.Manager.OverviewArea.Models
{
    public class TextEventsDataProvider : DatabaseDataProvider
    {
        #region Fields
        EventFilterModel _theEventFilterModel;
        int rowCount = 1000;
        String _theSQLText = "SELECT {0} (GEN_TS_LCT) AS GEN_TS_LCT,(COMPONENT_ID) as component_id,(PROCESS_ID) as process_id,(EVENT_ID) as event_id,SEVERITY ,(ROLE_NAME) as role_name,(TEXT) as text, (NODE_ID) as node_id, (PROCESS_NAME) as PROCESS_NAME, TOKENIZED_EVENT_TABLE from MANAGEABILITY.INSTANCE_REPOSITORY.EVENT_TEXT_1 {1} order by GEN_TS_LCT DESC for read uncommitted access;";
        private ArrayList _Columns = new ArrayList();

        #endregion Fields

        #region Properties

        public EventFilterModel EventFilterModel
        {
            get { return _theEventFilterModel; }
            set 
            {
                _theEventFilterModel = value;
                Persistence.Put(EventFilterModel.EventFilterPersistenceKey, _theEventFilterModel);
            }
        }

        public ArrayList ColumnsToFilterOn
        {
            get { return _Columns; }
            set { _Columns = value; }
        }

        #endregion Properties

        #region Constructors

        public TextEventsDataProvider(DataProviderConfig aDataProviderConfig)
            : base(aDataProviderConfig)
        {
        }

        #endregion Constructors

        //Overrides the default DoPrefetchSetup and sets up the query based on the number of rows requested and the 
        //filter criteria.
        public override void DoPrefetchSetup(Hashtable predefinedParametersHash)
        {
            TextEventDataProviderConfig dbConfig = DataProviderConfig as TextEventDataProviderConfig;
            rowCount = dbConfig.MaxRowCount;
            string rowCountString = (rowCount > 0) ? string.Format("[First {0}]", rowCount) : "";
            string filterStr = (_theEventFilterModel != null) ? _theEventFilterModel.GetFilterSQL() : "";
            string whereClause = ((filterStr != null) && (filterStr.Trim().Length > 0)) ? ("WHERE " + filterStr) : "";
            dbConfig.SQLText = string.Format(_theSQLText, rowCountString, whereClause);
            base.DoPrefetchSetup(predefinedParametersHash);
        }  
    }

    [Serializable]
    public class TextEventDataProviderConfig : DatabaseDataProviderConfig
    {
        int _CacheSize = 1000;
        //[XmlIgnore]
        [NonSerialized]
        protected TextEventsDataProvider _theDataProvider = null;
        //[XmlIgnore]
        public override DataProviderTypes DataProviderType
        {
            get { return DataProviderTypes.DB; }
        }

        /// <summary>
        /// Default constructor
        /// </summary>
        public TextEventDataProviderConfig()
        {
            base.TimerContinuesOnError = true;
        }

        /// <summary>
        /// Returns a new DatabaseDataProvider using this config
        /// </summary>
        /// <returns></returns>
        public override DataProvider GetDataProvider()
        {
            if (_theDataProvider == null)
            {
                _theDataProvider = new TextEventsDataProvider(this);
            }
            return _theDataProvider;
        }


        public int CacheSize
        {
            get { return _CacheSize; }
            set { _CacheSize = value; }
        }
    }
}
