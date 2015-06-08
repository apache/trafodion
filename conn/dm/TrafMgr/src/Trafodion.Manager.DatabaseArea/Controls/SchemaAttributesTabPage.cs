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

using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;

namespace Trafodion.Manager.DatabaseArea.Controls
{

    public class SchemaAttributesTabPage : Trafodion.Manager.Framework.Controls.DelayedPopulateTabPage, IHasTrafodionSchema
    {

        private TrafodionSchema theTrafodionSchema;
        private DatabaseObjectsControl theDatabaseObjectsControl;

        public SchemaAttributesTabPage(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionSchema aTrafodionSchema)
            : base(Properties.Resources.Attributes) 
        {
            TheDatabaseObjectsControl = aDatabaseObjectsControl;
            TheTrafodionSchema = aTrafodionSchema;
        }

        override protected void Populate()
        {
            //Console.WriteLine("schema attributes tab enter : " + DateTime.Now.ToString("yyyy-MM-dd hh':'mm':'ss.FFFFFF"));
            Controls.Clear();
            SchemaAttributesDataGridView theDataGridView = new SchemaAttributesDataGridView(TheDatabaseObjectsControl, TheTrafodionSchema);
            theDataGridView.Dock = DockStyle.Fill;
            Controls.Add(theDataGridView);
            //Console.WriteLine("schema attributes tab leave : " + DateTime.Now.ToString("yyyy-MM-dd hh':'mm':'ss.FFFFFF"));
        }

        public TrafodionSchema TheTrafodionSchema
        {
            get { return theTrafodionSchema; }
            set { theTrafodionSchema = value; }
        }

        public DatabaseObjectsControl TheDatabaseObjectsControl
        {
            get { return theDatabaseObjectsControl; }
            set { theDatabaseObjectsControl = value; }
        }

    }

}
