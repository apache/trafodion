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
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.Framework.Controls;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    class CatalogSpaceUsageSummaryControl : SpaceUsageSummaryUserControl, ICloneToWindow
    {
        protected override string ObjectType
        {
            get
            {
                return Properties.Resources.Schemas;
            }
        }

        public override string HeaderText
        {
            get
            {
                return "Catalog has {0} schemas and uses {1} of total SQL space {2}";
            }
        }

        protected override IDataDisplayHandler TheDataDisplayHandler
        {
            get
            {
                if (_dataDisplayHandler == null)
                {
                    _dataDisplayHandler = new SystemSpaceUsageDataHandler(this);
                }
                return _dataDisplayHandler;
            }
        }

        protected override string HelpTopicName
        {
            get
            {
                return HelpTopics.CatalogSpaceUsage;
            }
        }

        protected override string PersistenceKey
        {
            get
            {
                return "CatalogSpaceUsageSummaryPersistenceKey";
            }
        }

        public CatalogSpaceUsageSummaryControl(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionCatalog aTrafodionCatalog)
            : base(aDatabaseObjectsControl, aTrafodionCatalog)
        {
            this.DataProvider = new SystemSpaceDataProvider(this.DataProvider.DataProviderConfig);
            SQLText = Model.Queries.GetCatalogLevelSpaceUsageQueryString(aTrafodionCatalog);
        }

        public CatalogSpaceUsageSummaryControl(CatalogSpaceUsageSummaryControl clone)
            : this(null, (TrafodionCatalog)clone.TheTrafodionObject)
        {           
        }

        protected override void HandleHyperLink(int rowIndex, int colIndex)
        {
            string schemaName = TheDataGrid.Rows[rowIndex].Cells[colIndex].Value as string;
            if (!string.IsNullOrEmpty(schemaName))
            {
                TrafodionSchema schema = ((TrafodionCatalog)_theTrafodionObject).FindSchema(schemaName.Trim());
                if (schema != null && TheDatabaseObjectsControl != null)
                {
                    TheDatabaseObjectsControl.TheDatabaseTreeView.SelectTrafodionObject(schema);
                }
            }
        }

        #region ICloneToWindow Members
        public Control Clone()
        {
            return new CatalogSpaceUsageSummaryControl(this);
        }

        public string WindowTitle
        {
            get { return _theTrafodionObject.VisibleAnsiName+" "+Properties.Resources.SpaceUsage; }
        }

        public ConnectionDefinition ConnectionDefn
        {
            get { return _theTrafodionObject.ConnectionDefinition; }
        }

        #endregion ICloneToWindow Members
    }
}
