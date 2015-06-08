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
using Trafodion.Manager.Framework;
using System;
using System.Data;

namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// 
    /// </summary>
    public class TrafodionView : TrafodionSchemaObject, IHasTrafodionColumns
    {

        #region Fields
        
        /// <summary>
        /// Object name space
        /// </summary>
        public const string ObjectNameSpace = "TA";
        /// <summary>
        /// Object type
        /// </summary>
        public const string ObjectType = "VI";
        private string _viewType = "";
        private string _valid_def = "";
        private List<TrafodionSynonym> _synonyms = null;
        private List<TrafodionTable>   _tables   = null;
        private List<TrafodionMaterializedView> _mvs = null;
        private List<TrafodionView> _views = null;
        private List<TrafodionView> _viewsUsing = null;

        private List<TrafodionRoutine> _routine = null;
 
        private List<TrafodionObject> _objects = null;
        private DefaultHasTrafodionColumns<TrafodionViewColumn> _columnsDelegate;

        

        #endregion


        /// <summary>
        /// Creates a new TrafodionView object. The parent of instances of this object are TrafodionSchema objects.
        /// </summary>
        /// <param name="aTrafodionSchema">The parent object of this instance.</param>
        /// <param name="anInternalName">The internal name of the view.</param>
        /// <param name="aUID">The unique identifer of the object.</param>
        /// <param name="aCreateTime">The creation time of the object.</param>
        /// <param name="aRedefTime">The redefinition time of the object.</param>
        /// <param name="aSecurityClass">The security class of the object.</param>
        /// <param name="anOwner">The owner of the object.</param>
        public TrafodionView(TrafodionSchema aTrafodionSchema, string anInternalName, long aUID, long aCreateTime, long aRedefTime, string aSecurityClass, string anOwner)
            : base(aTrafodionSchema, anInternalName, aUID, aCreateTime, aRedefTime, aSecurityClass, anOwner)
        {
            _columnsDelegate = new DefaultHasTrafodionColumns<TrafodionViewColumn>(this);
        }

        /// <summary>
        /// Creates a new TrafodionView object. The parent of instances of this object are TrafodionSchema objects.
        /// </summary>
        /// <param name="aTrafodionSchema">The parent object of this instance.</param>
        /// <param name="anInternalName">The internal name of the view.</param>
        /// <param name="aUID">The unique identifer of the object.</param>
        /// <param name="aCreateTime">The creation time of the object.</param>
        /// <param name="aRedefTime">The redefinition time of the object.</param>
        /// <param name="aSecurityClass">The security class of the object.</param>
        /// <param name="anOwner">The owner of the object.</param>
        /// <param name="aValidDef">The validation information of the view, starting from M9</param>
        public TrafodionView(TrafodionSchema aTrafodionSchema, string anInternalName, long aUID, long aCreateTime, long aRedefTime, string aSecurityClass, string anOwner, string aValidDef)
            : this(aTrafodionSchema, anInternalName, aUID, aCreateTime, aRedefTime, aSecurityClass, anOwner)            
        {
            _valid_def = aValidDef;
        }

        /// <summary>
        /// The number of columns in this view.
        /// </summary>
        public int ColumnCount
        {
            get { return _columnsDelegate.ColumnCount; }
        }

        /// <summary>
        /// List of columns
        /// </summary>
        public List<TrafodionColumn> Columns
        {
            get { return _columnsDelegate.Columns; }
        }

        /// <summary>
        /// List of this view's column privileges.
        /// </summary>
        public List<ColumnPrivilege> ColumnPrivileges
        {
            get { return _columnsDelegate.ColumnPrivileges; }
        }

        /// <summary>
        /// The internal object type string for a view.
        /// </summary>
        override public string SchemaObjectType
        { 
            get
            {
                return ObjectType; 
            }
        }

        /// <summary>
        /// Displayable Object type for the view
        /// </summary>
        override public string DisplayObjectType
        {
            get
            {
                return Properties.Resources.View;
            }
        }
        
        /// <summary>
        /// The internal object name space for a view.
        /// </summary>
        override public string SchemaObjectNameSpace
        { 
            get
            {
                return ObjectNameSpace; 
            }
        }

        /// <summary>
        /// The view type
        /// </summary>
        public string ViewType
        {
            get { return _viewType; }
            set { _viewType = value; }
        }
        
        /// <summary>
        /// The view valid definition
        /// </summary>
        public string Valid_Def
        {
            get { return _valid_def; }
            set { _valid_def = value; }
        }


        /// <summary>
        /// A list of synonyms that point to the view. The synonyms all belong to the same catalog as the view.
        /// </summary>
        public List<TrafodionSynonym> TrafodionSynonyms
        {
            get
            {
                if (_synonyms == null)
                {
                    _synonyms = new TrafodionSynonymsLoader().Load(this);
                }
                return _synonyms;
            }
            set
            {
                _synonyms = null;
            }
        }

        /// <summary>
        /// Is this is a system view
        /// </summary>
        public override bool IsMetadataObject
        {
            get
            {
                return _viewType != null && _viewType.Trim().Equals("SV");
            }
        }    

        /// <summary>
        /// The object loader for TrafodionSynonym objects.
        /// </summary>
        class TrafodionSynonymsLoader : TrafodionObjectsLoader<TrafodionView, TrafodionSynonym>
        {
            override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionView aTrafodionView)
            {
                return Queries.ExecuteSelectSynonymsOnObjects(aConnection, aTrafodionView.TheTrafodionCatalog.ExternalName, aTrafodionView.TheTrafodionSchema.Version, aTrafodionView.UID);
            }

            /// <summary>
            /// Retrive the synonym objects from their respective schemas.
            /// </summary>
            /// <param name="aList"></param>
            /// <param name="aTrafodionView"></param>
            /// <param name="aReader"></param>
            override protected void LoadOne(List<TrafodionSynonym> aList, TrafodionView aTrafodionView, OdbcDataReader aReader)
            {
                string synonymInternalName = aReader.GetString(0).TrimEnd();
                long schemaUID = aReader.GetInt64(1);

                //Synonyms can be defined in other schemas to use this table. 
                //Using the uid of the schema in which the Synonym is located, get the TrafodionSchema object
                TrafodionSchema schema = null;
                try
                {
                    schema = aTrafodionView.TheTrafodionCatalog.FindSchema(schemaUID);
                }
                catch (Exception ex)
                {
                    //if schema is system schema or internal schema, we will get an error.
                }
                if (schema == null)
                {
                    return;
                }

                //Find the instance of the TrafodionSynonym from this TrafodionSchema object and add it to the list
                TrafodionSynonym synonym = schema.FindSynonym(synonymInternalName);

                if (synonym != null)
                {
                    aList.Add(synonym);
                }
            }
        }

        /// <summary>
        /// A list of tables that are used by this View. 
        /// </summary>
        public List<TrafodionTable> TheTrafodionTablesUsedBy
        {
            get
            {
                if (_tables == null)
                {
                    _tables = new TrafodionTablesUsedByLoader().Load(this);
                }
                return _tables;
            }
            set
            {
                _tables = null;
            }
        }



        /// <summary>
        /// The object loader for TrafodionTable objects.
        /// </summary>
        class TrafodionTablesUsedByLoader : TrafodionObjectsLoader<TrafodionView, TrafodionTable>
        {
            override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionView aTrafodionView)
            {
                return Queries.ExecuteSelectTablesUsedByView(aConnection, aTrafodionView.TheTrafodionCatalog.ExternalName, aTrafodionView.TheTrafodionSchema.Version, aTrafodionView.UID);
            }

            /// <summary>
            /// Retrive the table  objects from their respective schemas.
            /// </summary>
            /// <param name="aList"></param>
            /// <param name="aTrafodionView"></param>
            /// <param name="aReader"></param>
            override protected void LoadOne(List<TrafodionTable> aList, TrafodionView aTrafodionView, OdbcDataReader aReader)
            {
                string tableInternalName = aReader.GetString(0).TrimEnd();
                long schemaUID = aReader.GetInt64(1);

                //Table can be defined in other schemas to use this View. 
                //Using the uid of the schema in which the Table is located, get the TrafodionSchema object
                TrafodionSchema schema = null;
                try
                {
                    schema = aTrafodionView.TheTrafodionCatalog.FindSchema(schemaUID);
                }
                catch (Exception ex)
                {
                    //if schema is system schema or internal schema, we will get an error.
                }
                if (schema == null)
                {
                    return;
                }

                //Find the instance of the TrafodionTable from this TrafodionSchema object and add it to the list
                TrafodionTable table = schema.FindTable(tableInternalName);

                if (table != null)
                {
                    aList.Add(table);
                }
            }
        }


        /// <summary>
        /// A list of routines that are used by this View. 
        /// </summary>
        public List<TrafodionRoutine> TheTrafodionRoutinesUsedBy
        {
            get
            {
                if (_routine == null)
                {
                    _routine = new TrafodionRoutinesUsedByLoader().Load(this);
                }
                return _routine;
            }
            set
            {
                _routine = null;
            }
        }

        /// <summary>
        /// The object loader for TrafodionTable objects.
        /// </summary>
        class TrafodionRoutinesUsedByLoader : TrafodionObjectsLoader<TrafodionView, TrafodionRoutine>
        {
            override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionView aTrafodionView)
            {
                return Queries.ExecuteSelectRoutinesUsedByView(aConnection, aTrafodionView);
            }

            /// <summary>
            /// Retrive the routine  objects from their respective schemas.
            /// </summary>
            /// <param name="aList"></param>
            /// <param name="aTrafodionView"></param>
            /// <param name="aReader"></param>
            override protected void LoadOne(List<TrafodionRoutine> aList, TrafodionView aTrafodionView, OdbcDataReader aReader)
            {
                string routineName = aReader.GetString(0).Trim();
                long schemaUID = aReader.GetInt64(1);
                string udrType = aReader.GetString(2).Trim();

                TrafodionSchema schema = null;
                try
                {
                    schema = aTrafodionView.TheTrafodionCatalog.FindSchema(schemaUID);
                }
                catch (Exception ex)
                {
                    //if schema is system schema or internal schema, we will get an error.
                }
                if (schema == null)
                {
                    return;
                }
                TrafodionRoutine sqlMxRoutine = null;
                if ("F".Equals(udrType))
                {
                    sqlMxRoutine = schema.FindFunction(routineName);
                }
                else if ("AC".Equals(udrType))
                {
                    sqlMxRoutine = schema.FindFunctionAction(routineName);
                }
                else if ("T".Equals(udrType))
                {
                    sqlMxRoutine = schema.FindTableMappingFunction(routineName);
                }

                if (sqlMxRoutine != null)
                {
                    aList.Add(sqlMxRoutine);
                }
            }
        }

        /// <summary>
        /// A list of MVs that are used by this View. 
        /// </summary>
        public List<TrafodionMaterializedView> TheTrafodionMVsUsedBy
        {
            get
            {
                if (_mvs == null)
                {
                    _mvs = new TrafodionMVsUsedByLoader().Load(this);
                }
                return _mvs;
            }
            set
            {
                _mvs = null;
            }
        }



        /// <summary>
        /// The object loader for TrafodionMaterializedView objects.
        /// </summary>
        class TrafodionMVsUsedByLoader : TrafodionObjectsLoader<TrafodionView, TrafodionMaterializedView>
        {
            override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionView aTrafodionView)
            {
                return Queries.ExecuteSelectMVsUsedByView(aConnection, aTrafodionView.TheTrafodionCatalog.ExternalName, aTrafodionView.TheTrafodionSchema.Version, aTrafodionView.UID);
            }

            /// <summary>
            /// Retrive the MV objects from their respective schemas.
            /// </summary>
            /// <param name="aList"></param>
            /// <param name="aTrafodionView"></param>
            /// <param name="aReader"></param>
            override protected void LoadOne(List<TrafodionMaterializedView> aList, TrafodionView aTrafodionView, OdbcDataReader aReader)
            {
                string mvInternalName = aReader.GetString(0).TrimEnd();
                long schemaUID = aReader.GetInt64(1);

                //MV can be defined in other schemas to use this View. 
                //Using the uid of the schema in which the MV is located, get the TrafodionSchema object
                TrafodionSchema schema = null;
                try
                {
                    schema = aTrafodionView.TheTrafodionCatalog.FindSchema(schemaUID);
                }
                catch (Exception ex)
                {
                    //if schema is system schema or internal schema, we will get an error.
                }
                if (schema == null)
                {
                    return;
                }

                //Find the instance of the TrafodionMaterializedView from this TrafodionSchema object and add it to the list
                TrafodionMaterializedView mv = schema.FindMaterializedView(mvInternalName);

                if (mv != null)
                {
                    aList.Add(mv);
                }
            }
        }



        /// <summary>
        /// A list of other Views that are used by this View. 
        /// </summary>

        public List<TrafodionView> TheTrafodionViewsUsedBy
        {
            get
            {
                if (_views == null)
                {
                    _views = new TrafodionViewsUsedByLoader().Load(this);
                }
                return _views;
            }
            set
            {
                _views = null;
            }
        }



        /// <summary>
        /// The object loader for TrafodionView objects.
        /// </summary>
        class TrafodionViewsUsedByLoader : TrafodionObjectsLoader<TrafodionView, TrafodionView>
        {
            override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionView aTrafodionView)
            {
                return Queries.ExecuteSelectViewsUsedByView(aConnection, aTrafodionView.TheTrafodionCatalog.ExternalName, aTrafodionView.TheTrafodionSchema.Version, aTrafodionView.UID);
            }

            /// <summary>
            /// Retrive the Views objects from their respective schemas.
            /// </summary>
            /// <param name="aList"></param>
            /// <param name="aTrafodionView"></param>
            /// <param name="aReader"></param>
            override protected void LoadOne(List<TrafodionView> aList, TrafodionView aTrafodionView, OdbcDataReader aReader)
            {
                string viewInternalName = aReader.GetString(0).TrimEnd();
                long schemaUID = aReader.GetInt64(1);

                //MV can be defined in other schemas to use this View. 
                //Using the uid of the schema in which the MV is located, get the TrafodionSchema object
                TrafodionSchema schema = null;
                try
                {
                    schema = aTrafodionView.TheTrafodionCatalog.FindSchema(schemaUID);
                }
                catch (Exception ex)
                {
                    //if schema is system schema or internal schema, we will get an error.
                }
                if (schema == null)
                {
                    return;
                }

                //Find the instance of the TrafodionView from this TrafodionSchema object and add it to the list
                TrafodionView view = schema.FindView(viewInternalName);

                if (view != null)
                {
                    aList.Add(view);
                }
            }
        }



        /// <summary>
        /// A list of other Views that are used by this View. 
        /// </summary>

        public List<TrafodionView> TheTrafodionViewsUsing
        {
            get
            {
                if (_viewsUsing == null)
                {
                    _viewsUsing = new TrafodionViewsUsingLoader().Load(this);
                }
                return _viewsUsing;
            }
            set
            {
                _viewsUsing = null;
            }
        }



        /// <summary>
        /// The object loader for TrafodionView objects.
        /// </summary>
        class TrafodionViewsUsingLoader : TrafodionObjectsLoader<TrafodionView, TrafodionView>
        {
            override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionView aTrafodionView)
            {
                return Queries.ExecuteSelectViewsUsingView(aConnection, aTrafodionView.TheTrafodionCatalog.ExternalName, aTrafodionView.TheTrafodionSchema.Version, aTrafodionView.UID);
            }

            /// <summary>
            /// Retrive the Views objects from their respective schemas.
            /// </summary>
            /// <param name="aList"></param>
            /// <param name="aTrafodionView"></param>
            /// <param name="aReader"></param>
            override protected void LoadOne(List<TrafodionView> aList, TrafodionView aTrafodionView, OdbcDataReader aReader)
            {
                string viewInternalName = aReader.GetString(0).TrimEnd();
                long schemaUID = aReader.GetInt64(1);

                //MV can be defined in other schemas to use this View. 
                //Using the uid of the schema in which the MV is located, get the TrafodionSchema object
                TrafodionSchema schema = null;
                try
                {
                    schema = aTrafodionView.TheTrafodionCatalog.FindSchema(schemaUID);
                }
                catch (Exception ex)
                {
                    //if schema is system schema or internal schema, we will get an error.
                }
                if (schema == null)
                {
                    return;
                }

                //Find the instance of the TrafodionView from this TrafodionSchema object and add it to the list
                TrafodionView view = schema.FindView(viewInternalName);

                if (view != null)
                {
                    aList.Add(view);
                }
            }
        }





        /// <summary>
        /// Get the table that the views column came from
        /// </summary>
        /// <param name="columnNum">column number</param>
        /// <returns></returns>
 
        public TrafodionObject GetUsedTable(int columnNum)
        {

            if (_objects == null)
            {
                _objects = new TrafodionTablesLoader().Load(this);
            }

            // Object were fetched in column order
            if (_objects.Count > 0 && _objects.Count > columnNum )
              return _objects[columnNum];

            return null;
        }

        /// <summary>
        /// The object loader for used objects.
        /// </summary>
        class TrafodionTablesLoader : TrafodionObjectsLoader<TrafodionView, TrafodionObject>
        {
            override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionView aTrafodionView)
            {
                return Queries.ExecuteSelectColUsage(aConnection, aTrafodionView.TheTrafodionCatalog, aTrafodionView.TheTrafodionSchema.Version, aTrafodionView);
            }

            /// <summary>
            /// Retrieve the used objects from their respective schemas.
            /// </summary>
            /// <param name="aList"></param>
            /// <param name="aTrafodionView"></param>
            /// <param name="aReader"></param>
            override protected void LoadOne(List<TrafodionObject> aList, TrafodionView aTrafodionView, OdbcDataReader aReader)
            {
                TrafodionObject aObject = null;

                string objectInternalName = aReader.GetString(0).TrimEnd();
                long schemaUID = aReader.GetInt64(1);

                // Get the schema of the used table
                TrafodionSchema schema = null;
                try
                {
                    schema = aTrafodionView.TheTrafodionCatalog.FindSchema(schemaUID);
                }
                catch (Exception ex)
                {
                    //if schema is system schema or internal schema, we will get an error.
                }

                if (schema == null)
                {
                    return;
                }

                // Find the instance of the used object from this TrafodionSchema object and add it to the list
                // could be a Table, View or MV
                aObject = schema.FindSchemaObjectByName(objectInternalName, schema.TrafodionTables);

                if (aObject == null)
                {
                  aObject = schema.FindSchemaObjectByName(objectInternalName, schema.TrafodionViews);
                  if (aObject == null)
                      aObject = schema.FindSchemaObjectByName(objectInternalName, schema.TrafodionMaterializedViews);
                  }

                if (aObject != null)
                {
                    aList.Add(aObject);
                }
            }
        }

        /// <summary>
        /// The view model resets it state
        /// </summary>
        public override void Refresh()
        {
            //Create a temp view model
            TrafodionView aView = this.TheTrafodionSchema.LoadViewByName(this.InternalName);

            if (aView == null)
            {
                //IF temp model is null, the object has been removed
                //So cleanup and notify the UI
                this.TheTrafodionSchema.TrafodionViews.Remove(this);
                OnModelRemovedEvent();
                return;
            }
            if (this.CompareTo(aView) != 0)
            {
                //If sql object has been recreated, attach the new sql model to the parent.
                this.TheTrafodionSchema.TrafodionViews.Remove(this);
                this.TheTrafodionSchema.TrafodionViews.Add(aView);
                //Notify the UI
                this.OnModelReplacedEvent(aView);
            }
            else
            {
                base.Refresh();
                _columnsDelegate.ClearColumns();
                _synonyms = null;
                _objects = null;
                _tables = null;
                _mvs = null;
                _views = null;
                _viewsUsing = null;
                ResetColumnPrivileges();
            }
        }

        //protected override void OnModelReplacedEvent(TrafodionObject newTrafodionObject)
        //{            
        //    //ModelReplacedEvent(this, new TrafodionModelEventArgs(ReplaceEvent.ViewValidated, newTrafodionObject);
        //}


        public void ValidateView(bool isCascade, out DataTable sqlWarnings) 
        {
            Model.Queries.ExecuteValidationView(this.GetConnection(), this, isCascade, out sqlWarnings);

            //OnModelChangedEvent(new TrafodionModelChangeEventArgs(ChangeEvent.ViewValidated, this));
            //theTrafodionView.Refresh(); 
            //if (raiseEvent)
            //{
                //OnModelChangedEvent(new TrafodionModelChangeEventArgs(ChangeEvent.ViewValidated, this));
            //}
        }

        public void FireChangedEvent() 
        {
            OnModelChangedEvent(new TrafodionModelChangeEventArgs(ChangeEvent.ViewValidated, this));
        }


        public void ResetColumnPrivileges()
        {
            _columnsDelegate.ResetColumnPrivileges();
        }

        public static string DisplayValidState(string aValue)
        {
            return aValue.Equals("Y") ? "Valid" : "Invalid";
        }

        #region IHasTrafodionColumns Members


        public bool DoesUserHaveColumnPrivilege(string userName, string columnName, string privilegeType)
        {
            return _columnsDelegate.DoesUserHaveColumnPrivilege(userName, columnName, privilegeType);
        }

        #endregion
    }
}
