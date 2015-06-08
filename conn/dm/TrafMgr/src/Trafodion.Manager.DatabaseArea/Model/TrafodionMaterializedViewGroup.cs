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

using System.Collections.Generic;
using System.Data.Odbc;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Model
{
	/// <summary>
	/// The "Model"  class for MV groups to get data via ODBC and store it for access by the UI ["View"]
	/// </summary>
	public class TrafodionMaterializedViewGroup
		: TrafodionSchemaObject
	{
        public const string ObjectNameSpace = "RG";
        public const string ObjectType = "RG";
        
        private List<TrafodionMaterializedView> _sqlMxMaterializedViews = null;

        /// <summary>
        /// The Model for MV Group
        /// </summary>
        /// <param name="aTrafodionSchema"></param>
        /// <param name="anInternalName"></param>
        /// <param name="aUID"></param>
        /// <param name="aCreateTime"></param>
        /// <param name="aRedefTime"></param>
        /// <param name="aSecurityClass"></param>
        /// <param name="anOwner">The owner of the object.</param>
        public TrafodionMaterializedViewGroup(TrafodionSchema aTrafodionSchema, string anInternalName, long aUID, long aCreateTime, long aRedefTime, string aSecurityClass, string anOwner)
			: base(aTrafodionSchema, anInternalName, aUID, aCreateTime, aRedefTime, aSecurityClass, anOwner)
		{
		}

        /// <summary>
        /// Object Type for the MV used for Sql Query
        /// </summary>
		override public string SchemaObjectType
		{ 
			get
			{
                return ObjectType; 
			}
		}

        /// <summary>
        /// Displayable Object type for the MV group
        /// </summary>
        override public string DisplayObjectType
        {
            get
            {
                return Properties.Resources.MaterializedViewGroup;
            }
        }

        /// <summary>
        /// The naemspace fro the MV Group used for Sql query 
        /// </summary>
		override public string SchemaObjectNameSpace
		{ 
			get
			{
                return ObjectNameSpace; 
			}
		}

        /// <summary>
        /// Lets the model reset its state
        /// </summary>
        public override void Refresh()
        {
            //Create a temp MV Group model
            TrafodionMaterializedViewGroup aMVGroup = this.TheTrafodionSchema.LoadMVGroupByName(this.InternalName);

            //IF temp model is null, the object has been removed
            //So cleanup and notify the UI
            if (aMVGroup == null)
            {
                this.TheTrafodionSchema.TrafodionMaterializedViewGroups.Remove(this);
                OnModelRemovedEvent();
                return;
            }
            if (this.CompareTo(aMVGroup) != 0)
            {
                //If sql object has been recreated, attach the new sql model to the parent.
                this.TheTrafodionSchema.TrafodionMaterializedViewGroups.Remove(this);
                this.TheTrafodionSchema.TrafodionMaterializedViewGroups.Add(aMVGroup);
                //Notify the UI
                this.OnModelReplacedEvent(aMVGroup);
            }
            else
            {
                base.Refresh();
                _sqlMxMaterializedViews = null;
            }
        }

        /// <summary>
        /// DDL for the MV group
        /// </summary>
        public override string DDLText
        {
            get
            {
                //Generate the DDL since this is not available through SHOWDDL
                return "CREATE MVGROUP " + VisibleAnsiName + " ;";
            }
        }

        /// <summary>
        /// List of materialized views in this MV Group
        /// </summary>
        public List<TrafodionMaterializedView> TheTrafodionMaterializedViews
        {
            get
            {
                if (_sqlMxMaterializedViews == null)
                {
                    _sqlMxMaterializedViews = new MVGroupMembersLoader().Load(this);
                }
                return _sqlMxMaterializedViews;
            }
        }

        /// <summary>
        /// The class todo the SQL query via ODBC and retrieve the results/rows and store them for UI layer to use.
        /// </summary>
        class MVGroupMembersLoader : TrafodionObjectsLoader<TrafodionMaterializedViewGroup, TrafodionMaterializedView>
        {
            override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionMaterializedViewGroup aTrafodionMaterializedViewGroup)
            {
                // Query returns MV-name, sch-UID,  cat-UID
                return Queries.ExecuteSelectMVGroupMembers(aConnection, aTrafodionMaterializedViewGroup.TheTrafodionCatalog.ExternalName, aTrafodionMaterializedViewGroup.TheTrafodionSchema.Version, aTrafodionMaterializedViewGroup.UID);
            }

            /// <summary>
            /// Retrieve the Trafodion MaterializedView objects from their respective schemas and make a  list
            /// </summary>
            /// <param name="aList"></param>
            /// <param name="aTrafodionMaterializedViewGroup"></param>
            /// <param name="aReader"></param>
            /// 
            override protected void LoadOne(List<TrafodionMaterializedView> aList, TrafodionMaterializedViewGroup aTrafodionMaterializedViewGroup, OdbcDataReader aReader)
            {
                string mvName    = aReader.GetString(0).TrimEnd();
                long   schemaUID = aReader.GetInt64(1);

                TrafodionSchema sqlMxSchema = aTrafodionMaterializedViewGroup.TheTrafodionCatalog.FindSchema(schemaUID);
                
                // Find the MV object from the schema and add to the list.
                TrafodionMaterializedView mv = sqlMxSchema.FindMaterializedView(mvName);
                aList.Add(mv);

            }
        }

	}

}


