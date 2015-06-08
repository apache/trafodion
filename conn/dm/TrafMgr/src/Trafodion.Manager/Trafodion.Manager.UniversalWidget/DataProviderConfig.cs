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
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.UniversalWidget
{
    /// <summary>
    /// Super class for different DataProviderConfigs
    /// </summary>
    [Serializable]
    public abstract class DataProviderConfig
    {
        public  enum DataProviderTypes { DB, MDD };

        #region Member variables
        private string _theProviderName;
        private List<ColumnMapping> _theColumnMappings;
        private List<string> _theDefaultVisibleColumnNames;
        private List<ColumnSortObject> _theColumnSortObjects;
        //The column mappings as saved by the user
        private List<ColumnMapping> _theCurrentColumnMappings;
        private List<string> _theCurrentVisibleColumnNames;
        private int _theMaxRowCount = 500;
        private List<string> _prefetchColumnNameList;

        [NonSerialized]
        private string _theDefaultRefreshRates;
        
        [NonSerialized]
        protected ConnectionDefinition _theConnectionDefinition;
        private int _theRefreshRate;
        private bool _theTimerPaused;
        private bool _theTimerContinuesOnError = false;
        #endregion

        #region Properties

        [XmlIgnore]
        public string DefaultRefreshRates
        {
            get { return _theDefaultRefreshRates; }
            set
            {
                _theDefaultRefreshRates = value;
            }
        }

        [XmlElement("TimerPaused")]
        public bool TimerPaused
        {
            get { return _theTimerPaused; }
            set { _theTimerPaused = value; }
        }

        [XmlElement("RefreshRate")]
        public int RefreshRate
        {
            get { return _theRefreshRate; }
            set { _theRefreshRate = value; }
        }

        [XmlElement("MaxRowCount")]
        public int MaxRowCount
        {
            get { return _theMaxRowCount; }
            set { _theMaxRowCount = value; }
        }


        [XmlIgnore]
        public virtual ConnectionDefinition ConnectionDefinition
        {
            get { return _theConnectionDefinition; }
            set 
            {
                _theConnectionDefinition = (value != null) ? new ScratchConnectionDefinition(value) : null;
            }
        }
        public List<string> PrefetchColumnNameList
        {
            get 
            {
                if (_prefetchColumnNameList == null)
                {
                    _prefetchColumnNameList = new List<string>();
                }

                return _prefetchColumnNameList; 
            }
            set
            {
                _prefetchColumnNameList = value;
            }
        }
        [XmlArray("ColumnMappings")]
        [XmlArrayItem("ColumnMapping")]
        public List<ColumnMapping> ColumnMappings
        {
            get { return _theColumnMappings; }
            set { _theColumnMappings = value; }
        }

        [XmlArray("DefaultVisibleColumnNames")]
        [XmlArrayItem("DefaultVisibleColumnName")]
        public List<string> DefaultVisibleColumnNames
        {
            get { return _theDefaultVisibleColumnNames; }
            set 
            {
                _theDefaultVisibleColumnNames = value;
                if (_theCurrentVisibleColumnNames == null)
                {
                    _theCurrentVisibleColumnNames = new List<string>();
                    if (_theDefaultVisibleColumnNames != null)
                    {
                        _theCurrentVisibleColumnNames.AddRange(_theDefaultVisibleColumnNames.ToArray());
                    }
                }
            }
        }

        [XmlArray("ColumnSortObjects")]
        [XmlArrayItem("ColumnSortObject")]
        public List<ColumnSortObject> ColumnSortObjects
        {
            get { return _theColumnSortObjects; }
            set { _theColumnSortObjects = value; }
        }

        [XmlArray("CurrentColumnMappings")]
        [XmlArrayItem("CurrentColumnMappings")]
        public List<ColumnMapping> CurrentColumnMappings
        {
            get { return _theCurrentColumnMappings; }
            set { _theCurrentColumnMappings = value; }
        }

        [XmlArray("CurrentVisibleColumnNames")]
        [XmlArrayItem("CurrentVisibleColumnNames")]
        public List<string> CurrentVisibleColumnNames
        {
            get { return _theCurrentVisibleColumnNames; }
            set { _theCurrentVisibleColumnNames = value; }
        }

        [XmlElement("ProviderName")]
        public string ProviderName
        {
            get { return _theProviderName; }
            set { _theProviderName = value; }
        }

        [XmlIgnore]
        public abstract DataProviderTypes DataProviderType
        {
            get;
        }

        [XmlElement("TimerContinuesOnError")]
        public bool TimerContinuesOnError
        {
            get { return _theTimerContinuesOnError; }
            set { _theTimerContinuesOnError = value; }
        }

        #endregion

        #region Public methods
        
        
        /// <summary>
        /// Populates the data provider config from the XMLDocument object.
        /// Subclasses can extend this method to provide custom behavior
        /// </summary>
        /// <param name="dom"></param>
        public virtual void PopulateFromXML(XmlDocument dom)
        {
            //Populate the properties
             XmlNodeList nodes = dom.SelectNodes("/WidgetConfig/DataProviderConfig");
             if ((nodes != null) && (nodes.Count > 0))
             {
                 PopulatePropertiesFromXML(nodes[0]);
             }
            
            //Populate the column Mappings
             PopulateColumnMappings(dom);
        }
        
        /// <summary>
        /// Given a node, uses its children to populate all the properties of the 
        /// DataProviderConfig. 
        /// 
        /// This method can be extended by subclasses of this class to provide 
        /// custom property reading
        /// </summary>
        /// <param name="aParentNode"></param>
        public virtual void PopulatePropertiesFromXML(XmlNode aParentNode)
        {
            XMLConfigurationReader.PopulateProperties(this, aParentNode);
        }
        
        /// <summary>
        /// Populates any column mappings for the configuration
        /// </summary>
        /// <param name="dom"></param>
        public virtual void PopulateColumnMappings(XmlDocument dom)
        {
        }

        /// <summary>
        /// Subclasses must override this method to return appropriate data provider
        /// </summary>
        /// <returns></returns>
        public abstract DataProvider GetDataProvider();

        /// <summary>
        /// Given an internal name, returns the column mapping for that name
        /// </summary>
        /// <param name="internalName"></param>
        /// <returns></returns>
        public ColumnMapping GetColumnMappingForInternalName(string internalName)
        {
            if ((_theColumnMappings != null) && (internalName != null))
            {
                foreach (ColumnMapping cm in _theColumnMappings)
                {
                    if (cm.InternalName.Equals(internalName))
                    {
                        return cm;
                    }
                }
            }
            return null;
        }

        public void AddColumnMapping(ColumnMapping cm)
        {
            if (cm != null)
            {
                if (_theColumnMappings == null)
                {
                    _theColumnMappings = new List<ColumnMapping>();
                }
                _theColumnMappings.Add(cm);
            }
        }

        #endregion
    }

    /// <summary>
    /// This class shall be used to save the column name/attribute information
    /// from an internal and external class
    /// </summary>
    [Serializable]
    public class ColumnMapping
    {
        public ColumnMapping()
        {
        }

        public ColumnMapping(string internalName, string externalName, int columnWidth)
        {
            this._theInternalName = internalName;
            this._theExternalName = externalName;
            this._theColumnWidth = columnWidth;
        }

        private string _theInternalName;
        [XmlElement("InternalName")]
        public string InternalName
        {
            get { return _theInternalName; }
            set { _theInternalName = value; }
        }

        private string _theExternalName;
        [XmlElement("ExternalName")]
        public string ExternalName
        {
            get { return _theExternalName; }
            set { _theExternalName = value; }
        }

        private int _theColumnWidth;
        [XmlElement("ColumnWidth")]
        public int ColumnWidth
        {
            get { return _theColumnWidth; }
            set { _theColumnWidth = value; }
        }

        public static void Synchronize(List<ColumnMapping> definedMappings, List<ColumnMapping> persistedMappings)
        {
            if (definedMappings == null || persistedMappings == null) return;

            foreach (ColumnMapping persistedMapping in persistedMappings)
            {
                int index = -1;
                for (int i = 0; i < definedMappings.Count; i++ )
                {
                    if (0 == string.Compare(definedMappings[i].InternalName, persistedMapping.InternalName, true))
                    {
                        index = i;
                        break;
                    }
                }

                if (index >= 0)
                {
                    definedMappings.RemoveAt(index);
                    definedMappings.Add(persistedMapping);
                }
            }

        }
    }

    [Serializable]
    public class ColumnSortObject
    {

        public ColumnSortObject(){}
        public ColumnSortObject(int aColumnIndex, int aIndex, int aSortOrder)
        {
            _theColumnIndex = aColumnIndex;
            _theIndex = aIndex;
            _theSortOrder = aSortOrder;
        }

        private int _theColumnIndex;
        [XmlElement("ColIndex")]
        public int ColIndex
        {
            get { return _theColumnIndex; }
            set { _theColumnIndex = value; }
        }

        private int _theIndex;
        [XmlElement("Index")]
        public int Index
        {
            get { return _theIndex; }
            set { _theIndex = value; }
        }

        private int _theSortOrder;
        [XmlElement("SortOrder")]
        public int SortOrder
        {
            get { return _theSortOrder; }
            set { _theSortOrder = value; }
        }
 
        
    }

}
