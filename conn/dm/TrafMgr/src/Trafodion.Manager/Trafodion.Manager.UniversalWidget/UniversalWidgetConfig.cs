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
using System.Collections;
using System.Collections.Generic;
using System.Runtime.Serialization;
using System.Xml;
using System.Xml.Serialization;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.UniversalWidget
{
    [Serializable]
    [XmlRoot("WidgetConfig")]
    [XmlInclude(typeof(DatabaseDataProviderConfig))]
    //[XmlInclude(typeof(LineChartConfig))]
    public class UniversalWidgetConfig : IColumnMappingInfo
    {
        public enum ShowStatusStripEnum { Show, Hide, ShowDuringOperation };
        public enum ChartPositions { TOP, BOTTOM, LEFT, RIGHT };
        public static readonly string UniversalWidgetPersistenceKey = "UniversalWidgetPersistence";

        private string _theName;
        private string _theTitle;
        private string _theDescription;
        private string _theAuthor;
        private string _theVersion;
        private string _theServerVersion;
        private string _theLastExecutionTime;
        private string _theHelpTopic = null;
        private string _theSerializedChart = null;

        private string _theReportID;
        [NonSerialized]
        private string _theReportPath;

        private string _theReportFileName;

        private DataProviderConfig _theDataProviderConfig;

        [NonSerialized]
        private DataProvider _theDataProvider;

        private ChartConfig _theChartConfig;
        private int _theChartSplitterDistance = -1;

        [NonSerialized]
        private ChartRenderer _theChartRenderer;

        private bool _theSupportCharts = false;

        private bool _theShowToolBar = true;

        private bool _theShowChartToolBarButton = false;
        private bool _theShowPropertiesToolBarButton = false;
        private bool _theShowProviderToolBarButton = false;

        private bool _theShowTable = true;
        private bool _theShowChart = false;
        [NonSerialized]
        private ChartPositions _theChartPosition = ChartPositions.RIGHT;

        private bool _theShowProperties = false;
        private bool _theShowProviderStatus = false;

        private bool _theShowConnections = false;
        private bool _theShowCatalogs = false;
        private bool _theShowSchemas = false;
        private bool _theShowRowCount = false;
        private bool _theShowTimerSetupButton = true;
        private ShowStatusStripEnum _theShowStatusStrip = ShowStatusStripEnum.Show;
        private bool _theShowExportButtons = true;
        private bool _theShowHelpButton = true;
        private bool _theShowRefreshButton = true;

        [NonSerialized]
        private bool _theShowRefreshTimerButton = true;
        

        //Contains the names of the widgets that will be linked to this widget
        //User actions will invoke these widgets
        private List<AssociatedWidgetConfig> _theAssociatedWidgetConfigs = new List<AssociatedWidgetConfig>();

        //Parametrs that can be passed to this widget        
        private Hashtable _thePassedParameters;

        //name of selected Catalog combobox item,use for restore selected item
        private string _theCmbCatalogName;
        //name of selected Schema combobox item,use for restore selected item
        private string _theCmbSchemaName;

        [XmlElement("Name")]
        public string Name
        {
            get { return _theName; }
            set { _theName = value; }
        }

        [XmlElement("ReportID")]
        public string ReportID
        {
            get { return _theReportID; }
            set { _theReportID = value; }
        }

        [XmlIgnore]
        public string ReportPath
        {
            get { return _theReportPath; }
            set { _theReportPath = value; }
        }

        [XmlElement("Serializedchart")]
        public string SerializedChart
        {
            get { return _theSerializedChart; }
            set { _theSerializedChart = value; }
        }

        [XmlElement("ReportFileName")]
        public string ReportFileName
        {
            get { return _theReportFileName; }
            set { _theReportFileName = value; }
        }

        /// <summary>
        /// This property is not used any more. It has been lefy here to make the 
        /// persistence file compatible between 2.4.2 and 2.5 release. Please use 
        /// the AssociatedWidgetConfigs property get get a list of associated 
        /// widgets.
        /// </summary>
        //[XmlArray("AssociatedWidgets")]
        //[XmlArrayItem("AssociatedWidget")]
        //public List<string> AssociatedWidgets
        //{
        //    get { return null; }
        //    set {  }
        //}

        [XmlArray("AssociatedWidgets")]
        [XmlArrayItem("AssociatedWidgets")]
        public List<AssociatedWidgetConfig> AssociatedWidgets
        {
            get { return _theAssociatedWidgetConfigs; }
            set { _theAssociatedWidgetConfigs = value; }
        }


        [XmlElement("Description")]
        public string Description
        {
            get { return _theDescription; }
            set { _theDescription = value; }
        }

        [XmlElement("Author")]
        public string Author
        {
            get { return _theAuthor; }
            set { _theAuthor = value; }
        }

        [XmlElement("WidgetVersion")]
        public string WidgetVersion
        {
            get { return _theVersion; }
            set { _theVersion = value; }
        }

        [XmlElement("ServerVersion")]
        public string ServerVersion
        {
            get { return _theServerVersion; }
            set { _theServerVersion = value; }
        }

        [XmlElement("LastExecutionTime")]
        public string LastExecutionTime
        {
            get { return _theLastExecutionTime; }
            set { _theLastExecutionTime = value; }
        }

        [XmlElement("SupportCharts")]
        public bool SupportCharts
        {
            get { return _theSupportCharts; }
            set 
            { 
                _theSupportCharts = value;
                if (ChartConfig == null)
                {
                    ChartConfig = new ChartConfig();
                }
            }
        }

        [XmlElement("ShowRowCount")]
        public bool ShowRowCount
        {
            get { return _theShowRowCount; }
            set { _theShowRowCount = value; }
        }

        [XmlElement("ShowSchemas")]
        public bool ShowSchemas
        {
            get { return _theShowSchemas; }
            set { _theShowSchemas = value; }
        }

        [XmlElement("ShowCatalogs")]
        public bool ShowCatalogs
        {
            get { return _theShowCatalogs; }
            set { _theShowCatalogs = value; }
        }

        [XmlElement("ShowConnections")]
        public bool ShowConnections
        {
            get { return _theShowConnections; }
            set { _theShowConnections = value; }
        }

        [XmlElement("ShowChart")]
        public bool ShowChart
        {
            get { return _theShowChart; }
            set { _theShowChart = value; }
        }

        [XmlElement("ShowTable")]
        public bool ShowTable
        {
            get { return _theShowTable; }
            set { _theShowTable = value; }
        }

        [XmlElement("ShowProviderStatus")]
        public bool ShowProviderStatus
        {
            get { return _theShowProviderStatus; }
            set { _theShowProviderStatus = value; }
        }

        [XmlElement("ShowProperties")]
        public bool ShowProperties
        {
            get { return _theShowProperties; }
            set { _theShowProperties = value; }
        }

        [XmlIgnore]
        public DataProvider DataProvider
        {
            //get { return _theDataProvider; }
            set { _theDataProvider = value; }
        }

        [XmlElement("CmbCatalogName")]
        public string CmbCatalogName
        {
            get { return _theCmbCatalogName; }
            set { _theCmbCatalogName = value; }
        }

        [XmlElement("CmbSchemaName")]
        public string CmbSchemaName
        {
            get { return _theCmbSchemaName; }
            set { _theCmbSchemaName = value; }
        }

        [XmlElement("DataProviderConfig")]
        public DataProviderConfig DataProviderConfig
        {
            get
            {
                if (_theDataProvider == null)
                {
                    return _theDataProviderConfig;
                }
                else
                {
                    return _theDataProvider.DataProviderConfig;
                }
            }

            set
            {
                if (_theDataProvider == null)
                {
                    _theDataProviderConfig = value;
                }
                else
                {
                    throw new Exception("The DataProvider has already been created for this configuration."
                        + " If you choose to change the DataProvider configuration, please obtain the reference of the DataProvider"
                        + " from the universal widget and change its configuration.");
                }
            }
        }

        [XmlElement("ChartConfig")]
        public ChartConfig ChartConfig
        {
            get { return _theChartConfig; }
            set { _theChartConfig = value; }
        }

        [XmlElement("ChartSplitterDistance")]
        public int ChartSplitterDistance
        {
            get { return _theChartSplitterDistance; }
            set { _theChartSplitterDistance = value; }
        }

        [XmlElement("ChartPosition")]
        public ChartPositions ChartPosition
        {
            get { return _theChartPosition; }
            set { _theChartPosition = value; }
        }

        [XmlIgnore]
        public ChartRenderer ChartRenderer
        {
            get { return _theChartRenderer; }
            set { _theChartRenderer = value; }
        }

        [XmlElement("Title")]
        public string Title
        {
            get { return _theTitle; }
            set { _theTitle = value; }
        }

        [XmlElement("ShowToolBar")]
        public bool ShowToolBar
        {
            get { return _theShowToolBar; }
            set { _theShowToolBar = value; }
        }

        [XmlElement("ShowChartToolBarButton")]
        public bool ShowChartToolBarButton
        {
            get { return _theShowChartToolBarButton; }
            set { _theShowChartToolBarButton = value; }
        }

        [XmlElement("ShowPropertiesToolBarButton")]
        public bool ShowPropertiesToolBarButton
        {
            get { return _theShowPropertiesToolBarButton; }
            set { _theShowPropertiesToolBarButton = value; }
        }

        [XmlElement("ShowProviderToolBarButton")]
        public bool ShowProviderToolBarButton
        {
            get { return _theShowProviderToolBarButton; }
            set { _theShowProviderToolBarButton = value; }
        }

        [XmlIgnore]
        public Hashtable PassedParameters
        {
            get { return _thePassedParameters; }
            set { _thePassedParameters = value; }
        }

        [XmlElement("ShowTimerSetupButton")]
        public bool ShowTimerSetupButton
        {
            get { return _theShowTimerSetupButton; }
            set { _theShowTimerSetupButton = value; }
        }

        [XmlElement("ShowStatusStrip")]
        public ShowStatusStripEnum ShowStatusStrip
        {
            get { return _theShowStatusStrip; }
            set { _theShowStatusStrip = value; }
        }

        [XmlElement("ShowExportButtons")]
        public bool ShowExportButtons
        {
            get { return _theShowExportButtons; }
            set { _theShowExportButtons = value; }
        }

        // Try to keep this as the last one
        [XmlElement("ShowHelpButton")]
        public bool ShowHelpButton
        {
            get { return (_theShowHelpButton && !String.IsNullOrEmpty(_theHelpTopic)); }
            set { _theShowHelpButton = value; }
        }

        [XmlElement("HelpTopic")]
        public string HelpTopic
        {
            get { return _theHelpTopic; }
            set { _theHelpTopic = value; }
        }

        [XmlElement("ShowRefreshButton")]
        public bool ShowRefreshButton
        {
            get { return _theShowRefreshButton; }
            set { _theShowRefreshButton = value; }
        }

        [XmlIgnore()]
        public bool ShowRefreshTimerButton
        {
            get { return _theShowRefreshTimerButton; }
            set { _theShowRefreshTimerButton = value; }
        }

        public void Persist()
        {
            if (this.Name != null && this.Name.Trim().Length > 0)
            {
                try
                {
                    Hashtable uwPersistence = Trafodion.Manager.Framework.Persistence.Get(UniversalWidgetConfig.UniversalWidgetPersistenceKey) as Hashtable;

                    //This is the first time when a UW is being persisted
                    if (uwPersistence == null)
                    {
                        uwPersistence = new Hashtable();
                        Trafodion.Manager.Framework.Persistence.Put(UniversalWidgetConfig.UniversalWidgetPersistenceKey, uwPersistence);
                    }

                    lock (uwPersistence)
                    {
                        uwPersistence[this.Name] = this;
                    }
                }
                catch{}
            }
        }

        public AssociatedWidgetConfig GetAssociationByID(string aCallingWidgetID, string aCalledWidgetID)
        {
            AssociatedWidgetConfig ret = null;
            if (this.AssociatedWidgets != null)
            {
                foreach (AssociatedWidgetConfig ac in this.AssociatedWidgets)
                {
                    if (stringsEqual(ac.CallingWidgetID, aCallingWidgetID) && (stringsEqual(ac.CalledWidgetID, aCalledWidgetID)))
                    {
                        return ac;
                    }
                }
            }
            return null;
        }


        private bool stringsEqual(string str1, string str2)
        {
            return ((str1 != null) && (str2 != null) && (str1.Equals(str2)));
        }

        [OnDeserializing]
        void OnDeserializing(StreamingContext c) 
        {
            _theShowRefreshTimerButton = true;
        }

        [OnDeserialized]
        void OnDeserialized(StreamingContext c)
        {
            if (_theDataProviderConfig != null)
            {
                if (_theDataProviderConfig.PrefetchColumnNameList == null || _theDataProviderConfig.PrefetchColumnNameList.Count == 0)
                {
                    if (_theDataProviderConfig.CurrentVisibleColumnNames != null)
                    {
                        _theDataProviderConfig.PrefetchColumnNameList = new List<string>(_theDataProviderConfig.CurrentVisibleColumnNames);
                    }
                }
            }
        }

        #region IColumnMappingInfo Members

        /// <summary>
        /// Given an internal name, returns the column mapping info for that name
        /// </summary>
        /// <param name="internalName"></param>
        /// <param name="externalName"></param>
        /// <param name="columnWidth"></param>
        public void GetColumnMappingInfo(string internalName, out string externalName, out int columnWidth)
        {
            externalName = null;
            columnWidth = 0;
            if ((this.DataProviderConfig.ColumnMappings != null) && (internalName != null))
            {
                foreach (ColumnMapping columnMapping in this.DataProviderConfig.ColumnMappings)
                {
                    if (columnMapping.InternalName.Equals(internalName))
                    {
                        externalName = columnMapping.ExternalName;
                        columnWidth = columnMapping.ColumnWidth;
                        break;
                    }
                }
            }
        }

        #endregion
    }
}
