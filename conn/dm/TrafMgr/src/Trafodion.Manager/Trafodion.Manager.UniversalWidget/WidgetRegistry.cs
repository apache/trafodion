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
using System.IO;
using System.Runtime.Serialization.Formatters.Binary;
using System.Xml.Serialization;
//using ZedGraph;
//using Trafodion.Manager.Framework.Queries;

namespace Trafodion.Manager.UniversalWidget
{
    //It loads all of the widgets from the widget location
    public class WidgetRegistry
    {
        private static WidgetRegistry _theInstance = new WidgetRegistry();
        private Dictionary<string, WidgetRegistryEntry> _theWidgets = new Dictionary<string, WidgetRegistryEntry>();
        private WidgetRegistry()
        {

        }
        /// <summary>
        /// Returns the entire regitry
        /// </summary>
        public Dictionary<string, UniversalWidgetConfig> Widgets
        {
            get
            {
                Dictionary<string, UniversalWidgetConfig> widgets = new Dictionary<string, UniversalWidgetConfig>();
                lock (this)
                {
                    foreach (KeyValuePair<string, WidgetRegistryEntry> kv in _theWidgets)
                    {
                        widgets.Add(kv.Key, kv.Value.Config);
                    }
                }
                return widgets;
            }
        }

        /// <summary>
        /// WidgetRegistry is a singleton. This method retuns the instance to that singleton
        /// </summary>
        /// <returns></returns>
        public static WidgetRegistry GetInstance()
        {
            return _theInstance;
        }

        public UniversalWidgetConfig GetUniversalWidgetConfig(string reportID)
        {
            if (Widgets.ContainsKey(reportID))
            {
                return _theWidgets[reportID].Config;
            }
            return null;
        }

        private string ParseReportDefinitionName(string fileName)
        {
            string name = Path.GetFileNameWithoutExtension(fileName);
            //Path.GetFileNameWithoutExtension
            //int extensionIndex = fileName.LastIndexOf(".");
            //if (extensionIndex > 0)
            //{
            //    fileName = fileName.Substring(0, extensionIndex);
            //}
            return name;
        }

        /// <summary>
        /// if widget configuration has been change,update it to registry.
        /// </summary>
        /// <param name="config"></param>
        public void UpdateWidgetToRegistry(UniversalWidgetConfig config)
        {
            lock (this)
            {
                if (_theWidgets.ContainsKey(config.ReportID))
                {
                    WidgetRegistryEntry registryEntry = _theWidgets[config.ReportID];
                    registryEntry.Config = config;
                    _theWidgets.Remove(config.ReportID);
                    _theWidgets.Add(config.ReportID, registryEntry);

                }
            }
        }

        /// <summary>
        /// Adds the widget config to the registry. If there is an existing configuration in the 
        /// registry with the same name, it will be replaced with the new widget
        /// </summary>
        /// <param name="config"></param>
        public void AddWidgetToRegistry(WidgetRegistryEntry registryEntry)
        {
            //if (_theWidgets.ContainsKey(registryEntry.Config.Name))
            //{
            //    //for now we will only keep one instance of the widget for a given name.                                    
            //    _theWidgets.Remove(registryEntry.Config.Name);
            //}
            //_theWidgets.Add(registryEntry.Config.Name, registryEntry);
            try
            {
                //the report Id of old version is null, so in order to compatible with old version need to add report id
                if (registryEntry.Config.ReportID == null)
                {
                    registryEntry.Config.ReportID = GetWidgetID();
                }
                lock (this)
                {
                    if (_theWidgets.ContainsKey(registryEntry.Config.ReportID))
                    {
                        _theWidgets.Remove(registryEntry.Config.ReportID);
                    }
                    _theWidgets.Add(registryEntry.Config.ReportID, registryEntry);
                }
            }
            catch (Exception ex)
            {
            }
        }

        /// <summary>
        /// Adds the widget config to the registry. If there is an existing configuration in the 
        /// registry with the same name, it will be replaced with the new widget
        /// </summary>
        /// <param name="config"></param>
        public void RemoveWidgetFromRegistry(UniversalWidgetConfig config)
        {
            //if (_theWidgets.ContainsKey(config.Name))
            //{
            //    //for now we will only keep one instance of the widget for a given name.                                    
            //    _theWidgets.Remove(config.Name);
            //}
            lock (this)
            {
                if (_theWidgets.ContainsKey(config.ReportID))
                {
                    _theWidgets.Remove(config.ReportID);
                }
            }
        }

        public void RemoveWidgetFromRegistry(string reportPath)
        {
            List<UniversalWidgetConfig> tmp = new List<UniversalWidgetConfig>();
            lock (this)
            {
                foreach (KeyValuePair<string, WidgetRegistryEntry> kv in _theWidgets)
                {
                    WidgetRegistryEntry registryEntry = kv.Value;
                    if (registryEntry.Config.ReportPath == reportPath)
                    {
                        tmp.Add(registryEntry.Config);
                    }
                }
            }
            for (int i = 0; i < tmp.Count; i++)
            {
                RemoveWidgetFromRegistry(tmp[i]);
            }
        }


        /// <summary>
        /// Adds the widget config to the registry. If there is an existing configuration in the 
        /// registry with the same name, it will be replaced with the new widget
        /// </summary>
        /// <param name="config"></param>
        public void DeleteWidget(UniversalWidgetConfig config)
        {
            //if (_theWidgets.ContainsKey(config.Name))
            //{
            //    //for now we will only keep one instance of the widget for a given name.  
            //    WidgetRegistryEntry registryEntry = _theWidgets[config.Name];                  

            //    //delete the file
            //    File.Delete(registryEntry.FullFileName);

            //    //remove the entry from the dictionary
            //    _theWidgets.Remove(config.Name);
            //}
            if (_theWidgets.ContainsKey(config.ReportID))
            {
                WidgetRegistryEntry registryEntry = _theWidgets[config.ReportID];

                File.Delete(registryEntry.FullFileName);

                lock (this)
                {
                    _theWidgets.Remove(config.ReportID);
                }
            }
        }

        /// <summary>
        /// Given a path reads the widget configs from XML and loads them to the 
        /// registry and returns them in the list
        /// </summary>
        /// <param name="widgetDir"></param>
        /// <returns></returns>
        public List<UniversalWidgetConfig> LoadWidgets(string widgetDir)
        {
            List<UniversalWidgetConfig> widgets = new List<UniversalWidgetConfig>();
            Dictionary<string, UniversalWidgetConfig> filterArea = new Dictionary<string, UniversalWidgetConfig>();
            if (widgetDir != null)
            {
                DirectoryInfo dir = new DirectoryInfo(widgetDir);
                if (dir.Exists)
                {
                    FileInfo[] files = dir.GetFiles();
                    XmlSerializer s = new XmlSerializer(typeof(UniversalWidgetConfig));
                    TextReader reader = null;
                    for (int i = 0; i < files.Length; i++)
                    {
                        if (files[i].FullName.EndsWith(".widget", StringComparison.InvariantCultureIgnoreCase))
                        {
                            UniversalWidgetConfig config = LoadWidgetFromFile(files[i].FullName);
                            if (config != null)
                            {
                                if (filterArea.ContainsKey(config.Name))
                                {
                                    filterArea.Remove(config.Name);
                                }
                                filterArea.Add(config.Name, config);
                            }
                        }
                    }
                }
            }


            foreach (KeyValuePair<string, UniversalWidgetConfig> kv in filterArea)
            {
                widgets.Add(kv.Value);
            }
            return widgets;
        }

        public UniversalWidgetConfig LoadWidgetFromFile(string fullFilePath)
        {
            XmlSerializer s = new XmlSerializer(typeof(UniversalWidgetConfig));
            TextReader reader = null;
            UniversalWidgetConfig config = null;
            try
            {
                reader = new StreamReader(fullFilePath);
                config = (UniversalWidgetConfig)s.Deserialize(reader);

                WidgetRegistryEntry registryEntry = new WidgetRegistryEntry();
                registryEntry.FullFileName = fullFilePath;
                registryEntry.Config = config;

                AddWidgetToRegistry(registryEntry);
                reader.Close();
            }
            catch (Exception ex)
            {
                Console.WriteLine("Error in loading from file " + fullFilePath + ". " + ex.Message);
            }
            return config;
        }

        //return configuration without registry
        public UniversalWidgetConfig LoadWidgetFromFileWithoutRegistry(string fullFilePath)
        {
            XmlSerializer s = new XmlSerializer(typeof(UniversalWidgetConfig));
            TextReader reader = null;
            UniversalWidgetConfig config = null;
            try
            {
                reader = new StreamReader(fullFilePath);
                config = (UniversalWidgetConfig)s.Deserialize(reader);

                reader.Close();
            }
            catch (Exception ex)
            {
                Console.WriteLine("Error in loading from file " + fullFilePath + ". " + ex.Message);
            }
            return config;
        }

        public static string GetWidgetID()
        {
            long i = 1;
            foreach (byte b in Guid.NewGuid().ToByteArray())
            {
                i *= ((int)b + 1);
            }
            return string.Format("{0:x}", i - DateTime.Now.Ticks);
        }

        /// <summary>
        /// Saves the widget configuration after any changes have been made to the config
        /// </summary>
        /// <param name="widget"></param>
        public void SaveWidget(UniversalWidgetConfig widget)
        {
            WidgetRegistryEntry registryEntry = GetWidgetRegistryEntry(widget.ReportID);
            if ((registryEntry != null) && (registryEntry.FullFileName != null))
            {
                FileInfo fileInfo = new FileInfo(registryEntry.FullFileName);
                SaveWidget(widget, fileInfo.DirectoryName, fileInfo.Name);
            }
        }

        /// <summary>
        /// Saves the widget config to the location as an XML in the file specified
        /// </summary>
        /// <param name="widget"></param>
        /// <param name="widgetDir"></param>
        /// <param name="widgetFileName"></param>
        public void SaveWidget(UniversalWidgetConfig widget, String widgetDir, String widgetFileName)
        {
            XmlSerializer s = new XmlSerializer(typeof(UniversalWidgetConfig));
            TextWriter w = new StreamWriter(widgetDir + "\\" + widgetFileName);
            s.Serialize(w, widget);
            w.Close();
        }


        /// <summary>
        /// Obtains a widget config for the file name provided
        /// </summary>
        /// <param name="widget"></param>
        /// <param name="widgetDir"></param>
        /// <param name="widgetFileName"></param>
        public UniversalWidgetConfig GetWidgetForFileName(String widgetFileName)
        {
            lock (this)
            {
                foreach (KeyValuePair<string, WidgetRegistryEntry> kvp in _theWidgets)
                {
                    //we should use semantic equals, c:\\\\a.txt ==c:a.txt
                    if (Path.GetFullPath(kvp.Value.FullFileName).Equals(Path.GetFullPath(widgetFileName), StringComparison.InvariantCultureIgnoreCase))
                    {
                        return kvp.Value.Config;
                    }
                }
            }
            return null;
        }

        public UniversalWidgetConfig GetWidgetForDisplayNameInFolder(String fileFolder,String widgetDisplayName)
        {
            foreach (KeyValuePair<string, UniversalWidgetConfig> kv in Widgets)
            {
                UniversalWidgetConfig widgetConfig = kv.Value;
                if (Path.GetFullPath(fileFolder+"\\").Equals(Path.GetFullPath(widgetConfig.ReportPath+"\\"), 
                    StringComparison.InvariantCultureIgnoreCase) &&
                    string.Compare(widgetConfig.Name,widgetDisplayName,true)==0)
                {
                    return widgetConfig;
                }
            }
            return null;
        }

        /// <summary>
        /// Helper method to display the name of the widget without the logical directory info
        /// </summary>
        /// <param name="config"></param>
        /// <returns></returns>
        public static string GetWidgetDisplayName(UniversalWidgetConfig config)
        {
            return GetWidgetDisplayName(config.Name);
        }


        /// <summary>
        /// Helper method to display the name of the widget without the logical directory info
        /// </summary>
        /// <param name="config"></param>
        /// <returns></returns>
        public static string GetWidgetDisplayName(string configName)
        {
            string name = configName;
            string[] parts = name.Split(new char[] { '@' });
            return parts[parts.Length - 1];
        }

        /// <summary>
        /// Helper method to display the name of the widget without the logical directory info
        /// </summary>
        /// <param name="config"></param>
        /// <returns></returns>
        public static string GetWidgetPath(string configName)
        {
            string name = configName;
            string[] parts = name.Split(new char[] { '@' });
            string ret = "";
            if (parts.Length > 1)
            {
                for (int i = 0; i < (parts.Length - 1); i++)
                {
                    ret += parts[i] + "@";
                }
            }
            return ret;
        }

        /// <summary>
        /// Returns a default UniversalWidgetConfig with database data provider
        /// </summary>
        /// <returns></returns>
        public static UniversalWidgetConfig GetDefaultDBConfig()
        {
            UniversalWidgetConfig config = new UniversalWidgetConfig();
            config.Title = "Title - " + System.DateTime.Now.Ticks;
            config.Name = "Dummy Name - " + System.DateTime.Now.Ticks;
            //Create the chart configuration
            //LineChartConfig lineChartConfig = new LineChartConfig();
            //lineChartConfig.Title = "My Test Date Graph";
            //lineChartConfig.XaxisTitle = "Date";
            //lineChartConfig.XaxisType = AxisType.Date;
            //lineChartConfig.YAxisTitle = "Y Axis";
            //lineChartConfig.ChartLines = new List<ChartLine>();

            ////Create the definition of a curve in the chart
            //ChartLine chartLine1 = new ChartLine();
            //chartLine1.ChartLineLabel = "My Curve 1";
            //chartLine1.ChartLineColor = Color.Blue;
            //chartLine1.ChartLineFill = new Fill(Color.White, Color.Red, 45F);

            //chartLine1.TheChartLineSymbol = SymbolType.Circle;
            //chartLine1.SymbolFill = new Fill(Color.White);

            //PointConfig aPointConfig = new PointConfig();
            //aPointConfig.XColName = "X_date_column";
            //aPointConfig.YColName = "Y_column1";
            //chartLine1.PointConfig = aPointConfig;


            //ChartLine chartLine2 = new ChartLine();
            //chartLine2.ChartLineLabel = "My Curve 2";
            //chartLine2.ChartLineColor = Color.MediumVioletRed;
            //chartLine2.ChartLineFill = new Fill(Color.White, Color.Green);

            //chartLine2.TheChartLineSymbol = SymbolType.None;
            ////chartLine2.SymbolFill = new Fill(Color.White);

            //PointConfig pointConfig2 = new PointConfig();
            //pointConfig2.XColName = "X_date_column";
            //pointConfig2.YColName = "Y_column2";
            //chartLine2.PointConfig = pointConfig2;

            ////Add the configuration of the curve to the chart configuration
            //lineChartConfig.ChartLines.Add(chartLine1);
            //lineChartConfig.ChartLines.Add(chartLine2);

            DatabaseDataProviderConfig dbProviderConfig = new DatabaseDataProviderConfig();
            dbProviderConfig.SQLText = "Select * from TRAFODION.SCH.REPORT_TEST order by X_date_column ASC";
            config.DataProviderConfig = dbProviderConfig;

            //Just show the graph
            config.ShowTable = true;
            config.ShowChart = false;
            config.ShowProperties = false;
            config.ShowProviderStatus = false;

            config.ShowConnections = false;
            config.ShowCatalogs = false;
            config.ShowSchemas = false;
            config.ShowRowCount = false;

            return config;
        }

        /// <summary>
        /// Given a config name, returns the config from persistence
        /// </summary>
        /// <param name="name"></param>
        /// <returns></returns>
        public static UniversalWidgetConfig GetConfigFromPersistence(string name)
        {
            try
            {
                Hashtable uwPersistence = Trafodion.Manager.Framework.Persistence.Get(UniversalWidgetConfig.UniversalWidgetPersistenceKey) as Hashtable;

                //This is the first time when a UW is being persisted
                if (uwPersistence == null)
                {
                    return null;
                }
                return DeepCopy(uwPersistence[name] as UniversalWidgetConfig);
            }
            catch (Exception ex)
            {
            }
            return null;
        }

        /// <summary>
        /// Returns a copy of the widget
        /// </summary>
        /// <param name="aConfig"></param>
        /// <returns></returns>
        public static UniversalWidgetConfig DeepCopy(UniversalWidgetConfig aConfig)
        {
            UniversalWidgetConfig result = null;

            using (MemoryStream ms = new MemoryStream())
            {
                BinaryFormatter formatter = new BinaryFormatter();
                formatter.Serialize(ms, aConfig);
                ms.Position = 0;

                result = formatter.Deserialize(ms) as UniversalWidgetConfig;
                ms.Close();
            }
            return result;
        }

        /// <summary>
        /// Given a widget name retuns the WidgetRegistryEntry from the registry. 
        /// </summary>
        /// <param name="widgetName"></param>
        /// <returns></returns>
        //public WidgetRegistryEntry GetWidgetRegistryEntry(string widgetName)
        //{
        //    if (Widgets.ContainsKey(widgetName))
        //    {
        //        return _theWidgets[widgetName];
        //    }
        //    return null;
        //}

        public WidgetRegistryEntry GetWidgetRegistryEntry(string reportID)
        {
            try
            {
                if (Widgets.ContainsKey(reportID))
                {
                    return _theWidgets[reportID];
                }
            }
            catch (Exception ex)
            {
            }
            return null;
        }

        public UniversalWidgetConfig GetWidgetByName(string widgetName)
        {
            foreach (KeyValuePair<string, UniversalWidgetConfig> kv in Widgets)
            {
                UniversalWidgetConfig widgetConfig = kv.Value;
                if (widgetConfig.Name == widgetName)
                {
                    return widgetConfig;
                }
            }
            return null;
        }

    }


    public class WidgetRegistryEntry
    {
        private UniversalWidgetConfig _theConfig;
        private String _theFullFileName;

        public String FullFileName
        {
            get { return _theFullFileName; }
            set { _theFullFileName = value; }
        }

        public UniversalWidgetConfig Config
        {
            get { return _theConfig; }
            set { _theConfig = value; }
        }

    }
}
