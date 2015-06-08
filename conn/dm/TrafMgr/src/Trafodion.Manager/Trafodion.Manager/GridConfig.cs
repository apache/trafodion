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

namespace Trafodion.Manager.Framework
{
    [Serializable]
    public class GridConfig : IColumnMappingInfo
    {
        private GridConfig()
        {
            this.DefaultVisibleColumns = new List<string>();
            this.ColumnMappingInfos = new List<ColumnMappingInfo>();
            this.CurrentVisibleColumns = new List<string>();
            this.ColumnSorts = new List<ColumnSort>();
        }

        public GridConfig(string id) : this()
        {
            if (id == null || id.Trim().Length == 0)
            {
                throw new ArgumentNullException("id", "'id' cannot be null or empty!");
            }

            this.ID = id.Trim();
        }

        [XmlElement("ID")]
        public string ID
        {
            get;
            private set;
        }

        [XmlArray("ColumnMappingInfos")]
        [XmlArrayItem("ColumnMappingInfo")]
        public List<ColumnMappingInfo> ColumnMappingInfos
        {
            get;
            set;
        }

        [XmlArray("DefaultVisibleColumns")]
        [XmlArrayItem("DefaultVisibleColumnName")]
        public List<string> DefaultVisibleColumns
        {
            get;
            set;
        }

        [XmlArray("ColumnSorts")]
        [XmlArrayItem("ColumnSort")]
        public List<ColumnSort> ColumnSorts
        {
            get;
            set;
        }
        
        [XmlArray("CurrentVisibleColumns")]
        [XmlArrayItem("CurrentVisibleColumns")]
        public List<string> CurrentVisibleColumns
        {
            get;
            set;
        }

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
            if ((ColumnMappingInfos != null) && (internalName != null))
            {
                foreach (ColumnMappingInfo columnMapping in ColumnMappingInfos)
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

        /// <summary>
        /// Given an internal name, returns the column mapping info for that name
        /// </summary>
        /// <param name="internalName"></param>
        public ColumnMappingInfo GetColumnMapping(string internalName)
        {
            if ((ColumnMappingInfos != null) && (internalName != null))
            {
                foreach (ColumnMappingInfo columnMappingInfo in ColumnMappingInfos)
                {
                    if (columnMappingInfo.InternalName.Equals(internalName))
                    {
                        return columnMappingInfo;
                    }
                }
            }
            return null;
        }
    }

    /// <summary>
    /// This class shall be used to save the column name/attribute information
    /// from an internal and external class
    /// </summary>
    [Serializable]
    public class ColumnMappingInfo
    {
        public ColumnMappingInfo()
        {
        }

        public ColumnMappingInfo(string internalName, string externalName, int columnWidth)
        {
            this.InternalName = internalName;
            this.ExternalName = externalName;
            this.ColumnWidth = columnWidth;
        }

        [XmlElement("InternalName")]
        public string InternalName
        {
            get;
            set;
        }

        [XmlElement("ExternalName")]
        public string ExternalName
        {
            get;
            set;
        }

        [XmlElement("ColumnWidth")]
        public int ColumnWidth
        {
            get;
            set;
        }
    }

    [Serializable]
    public class ColumnSort
    {
        public ColumnSort()
        {
        }

        public ColumnSort(int columnIndex, int index, int sortOrder)
        {
            this.ColIndex = columnIndex;
            this.Index = index;
            this.SortOrder = sortOrder;
        }

        [XmlElement("ColIndex")]
        public int ColIndex
        {
            get;
            set;
        }

        [XmlElement("Index")]
        public int Index
        {
            get;
            set;
        }

        [XmlElement("SortOrder")]
        public int SortOrder
        {
            get; 
            set;
        }        
    }

    public interface IColumnMappingInfo
    {
        void GetColumnMappingInfo(string columnName, out string externalName, out int columnWidth);
    }
}
