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
using System.Reflection;
namespace Trafodion.Manager.UniversalWidget
{
    /// <summary>
    /// This class shall read an XML configuration file and populate the appropriate 
    /// configuration objects
    /// </summary>
    public class XMLConfigurationReader
    {
        public UniversalWidgetConfig GetConfig(string xmlFileName)
        {
            UniversalWidgetConfig returnValue = new UniversalWidgetConfig();
            XmlDocument dom = new XmlDocument();
            dom.Load(xmlFileName);
            //Populate the properties
            XmlNodeList nl = dom.DocumentElement.ChildNodes;
            for (int i = 0; i <= nl.Count - 1; i++)
            {
                string itemName = nl.Item(i).Name;
                if ((nl.Item(i).NodeType == XmlNodeType.Element) && (nl.Item(i).FirstChild.NodeType == XmlNodeType.Text))
                {
                    string value = nl.Item(i).FirstChild.InnerText;
                    PopulateProperty(returnValue, itemName, value);
                    Console.WriteLine(string.Format("{0} - {1}", itemName, value));
                }
            }

            //Populate the Data provider config
            returnValue.DataProviderConfig = GetDataProviderConfig(dom);

            //Populate the associated widgets
            PopulateAssociatedWidgets(returnValue, dom);
            
            return returnValue;
        }

        public static void PopulateProperties(Object anObject, XmlNode aParentNode)
        {
            XmlNodeList nodes = aParentNode.ChildNodes;
            for (int j = 0; j <= nodes.Count - 1; j++)
            {
                XmlNode node = nodes.Item(j);
                if ((node.NodeType == XmlNodeType.Element) && (node.FirstChild.NodeType == XmlNodeType.Text))
                {
                    string value = node.FirstChild.InnerText;
                    PopulateProperty(anObject, node.Name, value);
                }
            }
        }

        public static void PopulateProperty(Object anObject, string aProperty, string aValue)
        {
            Type objectType = anObject.GetType();
            PropertyInfo propInfo = null;
            try
            {
                propInfo = objectType.GetProperty(aProperty);
                if (propInfo != null)
                {
                    if (propInfo.PropertyType.FullName.Equals("System.String"))
                    {
                        propInfo.SetValue(anObject, aValue, null);
                    }
                    if (propInfo.PropertyType.FullName.Equals("System.Boolean"))
                    {
                        propInfo.SetValue(anObject, Boolean.Parse(aValue), null);
                    }
                    if (propInfo.PropertyType.FullName.Equals("System.Int32"))
                    {
                        propInfo.SetValue(anObject, Int32.Parse(aValue), null);
                    }
                }
            }
            catch (Exception ex)
            {
                //do nothing
            }
        }


        private DataProviderConfig GetDataProviderConfig(XmlDocument dom)
        {
            XmlNodeList nodes = dom.SelectNodes("/WidgetConfig/DataProviderConfig/ProviderName");
            DataProviderConfig dpconfig = null;
            if ((nodes != null) && (nodes.Count > 0))
            {
                dpconfig = GetDataProviderConfigForName((nodes[0].FirstChild.NodeType == XmlNodeType.Text) ? nodes[0].FirstChild.Value : null); 
                if (dpconfig != null)
                {
                    dpconfig.PopulateFromXML(dom);
                }
            }
            return dpconfig;
        }

        private DataProviderConfig GetDataProviderConfigForName(string name)
        {
            DataProviderConfig config = null;
            if (name == null) return config;

            if (name.Equals("DB", StringComparison.InvariantCultureIgnoreCase))
            {
                name = "Trafodion.Manager.UniversalWidget.DatabaseDataProviderConfig";
            }

            try
            {
                Type configType = Type.GetType(name);
                ConstructorInfo[] cInfos = configType.GetConstructors();
                if ((cInfos != null) && (cInfos.Length > 0))
                {
                    config = cInfos[0].Invoke(null) as DataProviderConfig;
                }
                else
                {
                    config = System.Activator.CreateInstance(configType) as DataProviderConfig;
                }
            }
            catch (Exception ex)
            {

            }
            return config;
        }


        private void PopulateAssociatedWidgets(UniversalWidgetConfig aConfig, XmlDocument dom)
        {
            //XmlNodeList nodes = dom.SelectNodes("/WidgetConfig/AssociatedWidgets");
            //if ((nodes != null) && (nodes.Count > 0))
            //{
            //    XmlNode node = null;
            //    for (int i = 0; i < nodes[0].ChildNodes.Count; i++)
            //    {
            //        node = nodes[0].ChildNodes[i];
            //        if (node.Name.Equals("AssociatedWidget"))
            //        {
            //            aConfig.AssociatedWidgets.Add(node.FirstChild.Value);
            //        }
            //    }
            //}
        }
    }
}
