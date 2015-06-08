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
using System.Data;
using System.Data.Odbc;
using System.Linq;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// Model for Schema
    /// </summary>
    public class TrafodionSchema : TrafodionObject, IHasTrafodionCatalog, IHasTrafodionSchema, IHasTrafodionPrivileges
    {
        #region Fields

        private TrafodionCatalog _sqlMxCatalog;
        private int _owner = 0;
        private string _ownerName;
        private int _version = 0;
        private string _subvolName = null;

        private List<TrafodionTable> _sqlMxTables = null;
        private List<TrafodionMaterializedView> _sqlMxMaterializedViews = null;
        private List<TrafodionMaterializedViewGroup> _sqlMxMaterializedViewGroups = null;
        private List<SchemaPrivilege> _schemaPrivileges = null;
        private List<TrafodionProcedure> _sqlMxProcedures = null;
        private List<TrafodionLibrary> _sqlMxLibraries = null;
        private List<TrafodionSynonym> _sqlMxSynonyms = null;
        private List<TrafodionView> _sqlMxViews = null;
        private List<TrafodionIndex> _sqlMxIndexes = null;
        private List<TrafodionUDFunction> _sqlMxUDFunctions = null;
        private List<TrafodionFunctionAction> _sqlMxFunctionActions = null;
        private List<TrafodionTableMappingFunction> _sqlMxTableMappingFunctions = null;
        private List<TrafodionSequence> _sqlMxSequences = null;

        private readonly object syncRootForTables = new object();

        #endregion

        #region Properties

        /// <summary>
        /// The tables that reside under this schema.
        /// </summary>
        public List<TrafodionTable> TrafodionTables
        {
            get
            {
                lock (this.syncRootForTables)
                {
                    if (_sqlMxTables == null)
                    {
                        _sqlMxTables = new TrafodionTablesLoader().Load(this);
                    }
                }

                return _sqlMxTables;
            }
            set
            {
                _sqlMxTables = null;
            }
        }

        /// <summary>
        /// The list of indexes under the schema.
        /// </summary>
        public List<TrafodionIndex> TrafodionIndexes
        {
            get
            {
                if (_sqlMxIndexes == null)
                {
                    RefreshIndexes();
                }

                return _sqlMxIndexes;
            }
        }

        /// <summary>
        /// The Materialized Views that reside under this schema.
        /// </summary>
        public List<TrafodionMaterializedView> TrafodionMaterializedViews
        {
            get
            {
                if (_sqlMxMaterializedViews == null)
                {
                    _sqlMxMaterializedViews = new TrafodionMaterializedViewsLoader().Load(this);
                }
                return _sqlMxMaterializedViews;
            }
            set
            {
                _sqlMxMaterializedViews = null;
            }
        }

        /// <summary>
        /// The Materialized View Groups that reside under this schema.
        /// </summary>
        public List<TrafodionMaterializedViewGroup> TrafodionMaterializedViewGroups
        {
            get
            {
                if (_sqlMxMaterializedViewGroups == null)
                {
                    _sqlMxMaterializedViewGroups = new TrafodionMaterializedViewGroupsLoader().Load(this);
                }
                return _sqlMxMaterializedViewGroups;
            }
            set
            {
                _sqlMxMaterializedViewGroups = null;
            }
        }

        /// <summary>
        /// The privileges of this schema.
        /// </summary>
        public List<SchemaPrivilege> Privileges
        {
            get
            {
                if (_schemaPrivileges == null)
                {
                    _schemaPrivileges = PrivilegesLoader.LoadSchemaPrivileges(this);
                }
                return _schemaPrivileges;
            }
        }

        /// <summary>
        /// The procedures that resided under this schema.
        /// </summary>
        public List<TrafodionProcedure> TrafodionProcedures
        {
            get
            {
                if (_sqlMxProcedures == null)
                {
                    _sqlMxProcedures = new TrafodionProceduresLoader().Load(this);
                }
                return _sqlMxProcedures;
            }
            set
            {
                _sqlMxProcedures = null;
            }
        }

        

        /// <summary>
        /// The Libraries that resided under this schema.
        /// </summary>
        public List<TrafodionLibrary> TrafodionLibraries
        {
            get
            {
                if (_sqlMxLibraries == null)
                {
                    _sqlMxLibraries = new TrafodionLibrariesLoader().Load(this);
                }
                return _sqlMxLibraries;
            }
            set
            {
                _sqlMxLibraries = null;
            }
        }

        /// <summary>
        /// The user defined functions that resided under this schema.
        /// </summary>
        public List<TrafodionUDFunction> TrafodionUDFunctions
        {
            get
            {
                if (_sqlMxUDFunctions == null)
                {
                    _sqlMxUDFunctions = new TrafodionUDFunctionsLoader().Load(this);
                }
                return _sqlMxUDFunctions;
            }
            set
            {
                _sqlMxUDFunctions = null;
            }
        }

        /// <summary>
        /// The user defined function actions that resided under this schema.
        /// </summary>
        public List<TrafodionFunctionAction> TrafodionFunctionActions
        {
            get
            {
                if (_sqlMxFunctionActions == null)
                {
                    _sqlMxFunctionActions = new TrafodionFunctionActionsLoader().Load(this);
                }
                return _sqlMxFunctionActions;
            }
            set
            {
                _sqlMxFunctionActions = null;
            }
        }

        public List<TrafodionTableMappingFunction> TrafodionTableMappingFunctions
        {
            get
            {
                if (_sqlMxTableMappingFunctions == null)
                {
                    _sqlMxTableMappingFunctions = new TrafodionTableMappingFunctionsLoader().Load(this);
                }
                return _sqlMxTableMappingFunctions;
            }
            set
            {
                _sqlMxTableMappingFunctions = null;
            }
        }
        
        /// <summary>
        /// The synonyms that reside under this schema.
        /// </summary>
        public List<TrafodionSynonym> TrafodionSynonyms
        {
            get
            {
                if (_sqlMxSynonyms == null)
                {
                    _sqlMxSynonyms = new TrafodionSynonymsLoader().Load(this);
                }
                return _sqlMxSynonyms;
            }
            set
            {
                _sqlMxSynonyms = null;
            }
        }

        /// <summary>
        ///  The views that reside under this schema.
        /// </summary>
        public List<TrafodionView> TrafodionViews
        {
            get
            {
                if (_sqlMxViews == null)
                {
                    _sqlMxViews = new TrafodionViewsLoader().Load(this);
                }
                return _sqlMxViews;
            }
            set
            {
                _sqlMxViews = null;
            }
        }

        /// <summary>
        ///  The views that reside under this schema.
        /// </summary>
        public List<TrafodionSequence> TrafodionSequences
        {
            get
            {
                if (_sqlMxSequences == null)
                {
                    _sqlMxSequences = new TrafodionSequencesLoader().Load(this);
                }
                return _sqlMxSequences;
            }
            set
            {
                _sqlMxSequences = null;
            }
        }
        /// <summary>
        /// The user id of the owner of the schema.
        /// </summary>
        public int Owner
        {
            get
            {
                return _owner;
            }
        }

        /// <summary>
        /// The owner's name.
        /// </summary>
        public string OwnerName
        {
            get
            {
                return _ownerName;
            }
        }

        /// <summary>
        /// The schema's version.
        /// </summary>
        public int Version
        {
            get
            {
                return _version;
            }
        }

        /// <summary>
        /// Sub volume name
        /// </summary>
        public string SubvolName
        {
            get
            {
                return _subvolName;
            }
        }

        /// <summary>
        /// Location
        /// </summary>
        public string Location
        {
            get
            {
                return _sqlMxCatalog.VolumeName + "." + SubvolName;
            }
        }

        /// <summary>
        /// This Schema object's TrafodionCatalog object
        /// </summary>
        public TrafodionCatalog TheTrafodionCatalog
        {
            get
            {
                return _sqlMxCatalog;
            }
            set
            {
                _sqlMxCatalog = value;
            }
        }

        /// <summary>
        /// The schema model
        /// </summary>
        public TrafodionSchema TheTrafodionSchema
        {
            get
            {
                return this;
            }
        }

        #endregion

        /// <summary>
        /// Creates a new Schema with no information.
        /// </summary>
        /// <summary>
        /// Creates a new Schema object.
        /// </summary>
        /// <param name="aTrafodionCatalog">The schema's catalog.</param>
        /// <param name="anInternalName">The schema's internal name.</param>
        /// <param name="anOwner">The user id of the owner of the schema.</param>
        /// <param name="aVersion">The version of the schema.</param>
        /// <param name="aSubvolName">The schema's subvolume.</param>
        /// <param name="aUID">The object UID of the schema.</param>
        public TrafodionSchema(TrafodionCatalog aTrafodionCatalog, string anInternalName, int anOwner, string anOwnerName, int aVersion, string aSubvolName, long aUID)
            : base(anInternalName, aUID)
        {
            TheTrafodionCatalog = aTrafodionCatalog;
            _owner = anOwner;
            _ownerName = anOwnerName;
            _version = aVersion;
            _subvolName = aSubvolName;
        }

        /// <summary>
        /// Finds a TrafodionSchemaObject object from the TrafodionTables list using an internal name
        /// </summary>
        /// <param name="anInternalName">The internal name of the table</param>
        /// <returns>TrafodionSchemaObject</returns>
        public TrafodionTable FindTable(string anInternalName)
        {
            TrafodionTable sqlMxTable = FindSchemaObjectByName(anInternalName, TrafodionTables);

            if (sqlMxTable == null)
            {
                //If table does not exist, reload the table list again
                _sqlMxTables = new TrafodionTablesLoader().Load(this);
                sqlMxTable = FindSchemaObjectByName(anInternalName, TrafodionTables);
                // TODO: Deal with exception if table is not found even after table list is reloaded
                //If table still does not exist, throw an exception
                if (sqlMxTable == null)
                    throw new Exception("Table " + anInternalName + " not found in schema " + ExternalName);
            }
            
            return sqlMxTable;
        }

        /// <summary>
        /// Finds a TrafodionMaterializedView object from the TrafodionMaterializedViews list using an internal name
        /// </summary>
        /// <param name="anInternalName">The internal name of the materialized view</param>
        /// <returns>TrafodionMaterializedView</returns>
        public TrafodionMaterializedView FindMaterializedView(string anInternalName)
        {
            TrafodionMaterializedView sqlMxMaterializedView = FindSchemaObjectByName(anInternalName, TrafodionMaterializedViews);

            if (sqlMxMaterializedView == null)
            {
                //If MV does not exist, reload the MV list again
                _sqlMxMaterializedViews = new TrafodionMaterializedViewsLoader().Load(this);
                sqlMxMaterializedView = FindSchemaObjectByName(anInternalName, TrafodionMaterializedViews);
                // TODO: Deal with exception if MV is not found even after mv list is reloaded
                //If materialized view still does not exist, throw an exception
                if (sqlMxMaterializedView == null)
                    throw new Exception("Materialized View" + anInternalName + " not found in schema " + ExternalName);
            }

            return sqlMxMaterializedView;
        }

        /// <summary>
        /// Finds a TrafodionMaterializedViewGroup object from the TrafodionMaterializedViewGroups list using an internal name
        /// </summary>
        /// <param name="anInternalName">The internal name of the materialized view group</param>
        /// <returns>TrafodionMaterializedViewGroup</returns>
        public TrafodionMaterializedViewGroup FindMaterializedViewGroup(string anInternalName)
        {
            TrafodionMaterializedViewGroup sqlMxMaterializedViewGroup = FindSchemaObjectByName(anInternalName, TrafodionMaterializedViewGroups);

            if (sqlMxMaterializedViewGroup == null)
            {
                //If mv group does not exist, reload the MV group list again
                _sqlMxMaterializedViewGroups = new TrafodionMaterializedViewGroupsLoader().Load(this);
                sqlMxMaterializedViewGroup = FindSchemaObjectByName(anInternalName, TrafodionMaterializedViewGroups);
                // TODO: Deal with exception if MV group is not found even after mv group list is reloaded
                //If materialized view group still does not exist, throw an exception
                if (sqlMxMaterializedViewGroup == null)
                    throw new Exception("Materialized View Group " + anInternalName + " not found in schema " + ExternalName);
            } 
            return sqlMxMaterializedViewGroup;
        }

        /// <summary>
        /// Finds a TrafodionView object from the TrafodionViews list using an internal name
        /// </summary>
        /// <param name="anInternalName">The internal name of the view</param>
        /// <returns>TrafodionView</returns>
        public TrafodionView FindView(string anInternalName)
        {
            TrafodionView sqlMxView = FindSchemaObjectByName(anInternalName, TrafodionViews);

            if (sqlMxView == null)
            {
                //If view does not exist, reload the views list again
                _sqlMxViews = new TrafodionViewsLoader().Load(this);
                sqlMxView = FindSchemaObjectByName(anInternalName, TrafodionViews);
                // TODO: Deal with exception if view is not found even after view list is reloaded
                //If view  still does not exist, throw an exception
                if (sqlMxView == null)
                    throw new Exception("View  " + anInternalName + " not found in schema " + ExternalName);
            } 
            return sqlMxView;
        }

        /// <summary>
        /// Finds a TrafodionIndex object from the TrafodionIndexes list using the internal name of an index
        /// </summary>
        /// <param name="anInternalName"></param>
        /// <returns></returns>
        public TrafodionIndex FindIndex(string anInternalName)
        {
            TrafodionIndex sqlMxIndex = FindSchemaObjectByName(anInternalName, TrafodionIndexes);

            if (sqlMxIndex == null)
            {
                //If index does not exist, reload the indexes list again
                _sqlMxIndexes = new TrafodionIndexesLoader().Load(this);
                sqlMxIndex = FindSchemaObjectByName(anInternalName, TrafodionIndexes);
                // TODO: Deal with exception if index is not found even after indexes list is reloaded
                //If index  still does not exist, throw an exception
                if (sqlMxIndex == null)
                    throw new Exception("Index  " + anInternalName + " not found in schema " + ExternalName);
            }
            return sqlMxIndex;
        }
        /// <summary>
        /// Finds a TrafodionSynonym object from the TrafodionSynonyms list using an internal name
        /// </summary>
        /// <param name="anInternalName">The internal name of the synonym</param>
        /// <returns>TrafodionSynonym</returns>
        public TrafodionSynonym FindSynonym(string anInternalName)
        {
            TrafodionSynonym sqlMxSynonym = FindSchemaObjectByName(anInternalName, TrafodionSynonyms);

            if (sqlMxSynonym == null)
            {
                //If synonym does not exist, reload the synonyms list again
                _sqlMxSynonyms = new TrafodionSynonymsLoader().Load(this);
                sqlMxSynonym = FindSchemaObjectByName(anInternalName, TrafodionSynonyms);
                // TODO: Deal with exception if synonym is not found even after synonym list is reloaded
                //If synonym  still does not exist, throw an exception
                if (sqlMxSynonym == null)
                    throw new Exception("Synonym  " + anInternalName + " not found in schema " + ExternalName);
            } 

            return sqlMxSynonym;
        }

        /// <summary>
        /// Finds a TrafodionProcedure object from the TrafodionProcedures list using an internal name
        /// </summary>
        /// <param name="anInternalName">The internal name of the procedure</param>
        /// <returns>TrafodionProcedure</returns>
        public TrafodionProcedure FindProcedure(string anInternalName)
        {
            TrafodionProcedure sqlMxProcedure = FindSchemaObjectByName(anInternalName, TrafodionProcedures);

            if (sqlMxProcedure == null)
            {
                //If procedure does not exist, reload the procedures list again
                _sqlMxProcedures = new TrafodionProceduresLoader().Load(this);
                sqlMxProcedure = FindSchemaObjectByName(anInternalName, TrafodionProcedures);
                // TODO: Deal with exception if procedure is not found even after procedure list is reloaded
                //If procedure still does not exist, throw an exception
                if (sqlMxProcedure == null)
                    throw new Exception("Procedure  " + anInternalName + " not found in schema " + ExternalName);
            } 

            return sqlMxProcedure;
        }

        /// <summary>
        /// Finds a TrafodionLibrary object from the TrafodionLibrary list using an internal name
        /// </summary>
        /// <param name="anInternalName"></param>
        /// <returns></returns>
        public TrafodionLibrary FindLibrary(string anInternalName)
        {
            TrafodionLibrary sqlMxLibrary = FindSchemaObjectByName(anInternalName, TrafodionLibraries);

            if (sqlMxLibrary == null)
            {
                //If Library does not exist, reload the Library list again
                _sqlMxLibraries = new TrafodionLibrariesLoader().Load(this);
                sqlMxLibrary = FindSchemaObjectByName(anInternalName, TrafodionLibraries);
                // TODO: Deal with exception if Library is not found even after Library list is reloaded
                //If Library still does not exist, throw an exception
                if (sqlMxLibrary == null)
                    throw new Exception("Library  " + anInternalName + " not found in schema " + ExternalName);
            }

            return sqlMxLibrary;
        }

        /// <summary>
        /// Finds a TrafodionLibrary object from the TrafodionLibrary list using UID
        /// </summary>
        /// <param name="anInternalName"></param>
        /// <returns></returns>
        public TrafodionLibrary FindLibrary(long objUID)
        {
            TrafodionLibrary sqlMxLibrary = FindSchemaObjectByUID(objUID, TrafodionLibraries);

            if (sqlMxLibrary == null)
            {
                //If Library does not exist, reload the Library list again
                _sqlMxLibraries = new TrafodionLibrariesLoader().Load(this);
                sqlMxLibrary = FindSchemaObjectByUID(objUID, TrafodionLibraries);
                // TODO: Deal with exception if Library is not found even after Library list is reloaded
                //If Library still does not exist, throw an exception
                if (sqlMxLibrary == null)
                    throw new Exception("Library UID " + objUID + " not found in schema " + ExternalName);
            }

            return sqlMxLibrary;
        }

        /// <summary>
        /// Finds a TrafodionUDFunction object from the TrafodionUDFunction list using an internal name
        /// </summary>
        /// <param name="anInternalName">The internal name of the user defined function</param>
        /// <returns>TrafodionUDFunction</returns>
        public TrafodionUDFunction FindFunction(string anInternalName)
        {
            TrafodionUDFunction sqlMxUDFunction = FindSchemaObjectByName(anInternalName, TrafodionUDFunctions);

            if (sqlMxUDFunction == null)
            {
                //If procedure does not exist, reload the functions list again
                _sqlMxUDFunctions = new TrafodionUDFunctionsLoader().Load(this);
                sqlMxUDFunction = FindSchemaObjectByName(anInternalName, TrafodionUDFunctions);
                // TODO: Deal with exception if function is not found even after functions list is reloaded
                //If function still does not exist, throw an exception
            }

            return sqlMxUDFunction;
        }
        /// <summary>
        /// Finds a TrafodionUDFunction object from the TrafodionUDFunction list using an function UID
        /// </summary>
        /// <param name="aFunctionUID">The function uid of the user defined function</param>
        /// <returns>TrafodionUDFunction</returns>
        public TrafodionUDFunction FindFunction(long aFunctionUID)
        {
            TrafodionUDFunction theTrafodionUDFunction = TrafodionUDFunctions.Find(delegate(TrafodionUDFunction aTrafodionUDFunction)
            {
                return aTrafodionUDFunction.UID == aFunctionUID;
            });

            if (theTrafodionUDFunction == null)
            {
                //If procedure does not exist, reload the functions list again
                _sqlMxUDFunctions = new TrafodionUDFunctionsLoader().Load(this);
                theTrafodionUDFunction = TrafodionUDFunctions.Find(delegate(TrafodionUDFunction aTrafodionUDFunction)
                {
                    return aTrafodionUDFunction.UID == aFunctionUID;
                });
            }

            return theTrafodionUDFunction;
        }

        /// <summary>
        /// Finds a TrafodionUDFunctionAction object from the TrafodionUDFunctionAction list using an object name
        /// </summary>
        /// <param name="anInternalName"></param>
        /// <returns>TrafodionFunctionAction</returns>
        public TrafodionFunctionAction FindFunctionAction(String anInternalName)
        {
            TrafodionFunctionAction functionAction = FindSchemaObjectByName(anInternalName, TrafodionFunctionActions);

            if (functionAction == null)
            {
                //If procedure does not exist, reload the functions list again
                _sqlMxFunctionActions = new TrafodionFunctionActionsLoader().Load(this);
                functionAction = FindSchemaObjectByName(anInternalName, TrafodionFunctionActions);
            }

            return functionAction;
        }

        public TrafodionFunctionAction FindFunctionAction(long  aCatalogUID, long aSchemaUID, long aFunctionActionUID)
        {
            TrafodionFunctionAction theTrafodionFunctionAction = null;

            foreach (TrafodionFunctionAction aTrafodionFunctionAction in TrafodionFunctionActions)
            {
                if (aCatalogUID == aTrafodionFunctionAction.TheTrafodionCatalog.UID && aSchemaUID == aTrafodionFunctionAction.TheTrafodionSchema.UID && aFunctionActionUID == aTrafodionFunctionAction.UID)
                {
                    theTrafodionFunctionAction = aTrafodionFunctionAction;
                    break;
                }
            }

            return theTrafodionFunctionAction;
        }

        /// <summary>
        /// Finds a TrafodionTableMappingFunction object from the TrafodionTableMappingFunction list using an object name
        /// </summary>
        /// <param name="anInternalName">object name</param>
        /// <returns>TrafodionTableMappingFunction</returns>
        public TrafodionTableMappingFunction FindTableMappingFunction(string anInternalName)
        {
            TrafodionTableMappingFunction sqlMxTableMappingFunction = FindSchemaObjectByName(anInternalName, TrafodionTableMappingFunctions);

            if (sqlMxTableMappingFunction == null)
            {
                //If table mappting fuction does not exist, reload the table mappting fuction list again
                _sqlMxTableMappingFunctions = new TrafodionTableMappingFunctionsLoader().Load(this);
                sqlMxTableMappingFunction = FindSchemaObjectByName(anInternalName, TrafodionTableMappingFunctions);
            }

            return sqlMxTableMappingFunction;
        }

        /// <summary>
        /// A Generic method to find a TrafodionSchemaObject from the corresponding TrafodionSchemaObject list using an internal name
        /// </summary>
        /// <typeparam name="T">Type of sql object</typeparam>
        /// <param name="anInternalName">The internal name of the TrafodionSchemaObject</param>
        /// <param name="aTrafodionSchemaObjectList">The list to search</param>
        /// <returns></returns>
        public T FindSchemaObjectByName<T>(string anInternalName, List<T> aTrafodionSchemaObjectList) where T : TrafodionSchemaObject
        {
            T theTrafodionSchemaObject = aTrafodionSchemaObjectList.Find(delegate(T aTrafodionSchemaObject)
            {
                return aTrafodionSchemaObject.InternalName.Equals(anInternalName);
            });
            return theTrafodionSchemaObject;
        }

        /// <summary>
        /// A Generic method to find a TrafodionSchemaObject from the corresponding TrafodionSchemaObject list using an UID
        /// </summary>
        /// <typeparam name="T">Type of sql object</typeparam>
        /// <param name="objUID">UID of object</param>
        /// <param name="aTrafodionSchemaObjectList">The list to search</param>
        /// <returns></returns>
        public T FindSchemaObjectByUID<T>(long objUID, List<T> aTrafodionSchemaObjectList) where T : TrafodionSchemaObject
        {
            T theTrafodionSchemaObject = aTrafodionSchemaObjectList.Find(delegate(T aTrafodionSchemaObject)
            {
                return aTrafodionSchemaObject.UID==objUID;
            });
            return theTrafodionSchemaObject;
        }

        public T GetSchemaObjectByName<T>(string anInternalName) where T : TrafodionSchemaObject
        {
            TrafodionSchemaObject schemaObject = null;
            Type objectType = typeof(T);

            if (objectType.IsAssignableFrom(typeof(TrafodionTable)))
            {
                if (TrafodionObject.Exists<TrafodionTable>(TrafodionTables, anInternalName))
                {
                    schemaObject = FindTable(anInternalName);
                }
                else
                {
                    schemaObject = LoadTableByName(anInternalName);
                }
            }
            else
            if (objectType.IsAssignableFrom(typeof(TrafodionMaterializedView)))
            {
                if (TrafodionObject.Exists<TrafodionMaterializedView>(TrafodionMaterializedViews, anInternalName))
                {
                    schemaObject = FindMaterializedView(anInternalName);
                }
                else
                {
                    schemaObject = LoadMVByName(anInternalName);
                }
            }
            else
            if (objectType.IsAssignableFrom(typeof(TrafodionView)))
            {
                if (TrafodionObject.Exists<TrafodionView>(TrafodionViews, anInternalName))
                {
                    schemaObject = FindView(anInternalName);
                }
                else
                {
                    schemaObject = LoadViewByName(anInternalName);
                }
            }
            else
                if (objectType.IsAssignableFrom(typeof(TrafodionLibrary)))
                {
                    if (TrafodionObject.Exists<TrafodionLibrary>(TrafodionLibraries, anInternalName))
                    {
                        schemaObject = FindLibrary(anInternalName);
                    }
                    else
                    {
                        schemaObject = LoadLibraryByName(anInternalName);
                    }
                }
            else
            if (objectType.IsAssignableFrom(typeof(TrafodionProcedure)))
            {
                if (TrafodionObject.Exists<TrafodionProcedure>(TrafodionProcedures, anInternalName))
                {
                    schemaObject = FindProcedure(anInternalName);
                }
                else
                {
                    schemaObject = LoadProcedureByName(anInternalName);
                }
            }
            else
            if (objectType.IsAssignableFrom(typeof(TrafodionSynonym)))
            {
                if (TrafodionObject.Exists<TrafodionSynonym>(TrafodionSynonyms, anInternalName))
                {
                    schemaObject = FindSynonym(anInternalName);
                }
                else
                {
                    schemaObject = LoadSynonymByName(anInternalName);
                }
            }
            return (T)schemaObject;
        }
        /// <summary>
        /// Creates a table model
        /// </summary>
        /// <param name="anInternalName">Internal name of the table</param>
        /// <returns></returns>
        public TrafodionTable LoadTableByName(string anInternalName)
        {
            return new TrafodionTablesLoader().LoadObjectByName(this, anInternalName);
        }

        /// <summary>
        /// Creates a MV model
        /// </summary>
        /// <param name="anInternalName">Internal name of the MV</param>
        /// <returns></returns>
        public TrafodionMaterializedView LoadMVByName(string anInternalName)
        {
            return new TrafodionMaterializedViewsLoader().LoadObjectByName(this, anInternalName);
        }

        /// <summary>
        /// Creates a MVGroup model
        /// </summary>
        /// <param name="anInternalName">Internal name of the MVGroup</param>
        /// <returns></returns>
        public TrafodionMaterializedViewGroup LoadMVGroupByName(string anInternalName)
        {
            return new TrafodionMaterializedViewGroupsLoader().LoadObjectByName(this, anInternalName);
        }

        /// <summary>
        /// Creates a view model
        /// </summary>
        /// <param name="anInternalName">Internal name of the view</param>
        /// <returns></returns>
        public TrafodionView LoadViewByName(string anInternalName)
        {
            return new TrafodionViewsLoader().LoadObjectByName(this, anInternalName);
        }

        /// <summary>
        /// Creates a sequence model
        /// </summary>
        /// <param name="anInternalName">Internal name of the sequence</param>
        /// <returns></returns>
        public TrafodionSequence LoadSequenceByName(string anInternalName)
        {
            return new TrafodionSequencesLoader().LoadObjectByName(this, anInternalName);
        }

        /// <summary>
        /// Creates a synonym model
        /// </summary>
        /// <param name="anInternalName">Internal name of the synonym</param>
        /// <returns></returns>
        public TrafodionSynonym LoadSynonymByName(string anInternalName)
        {
            return new TrafodionSynonymsLoader().LoadObjectByName(this, anInternalName);
        }
        
        /// <summary>
        /// Creates a procedure model
        /// </summary>
        /// <param name="anInternalName">Internal name of the procedure</param>
        /// <returns></returns>
        public TrafodionProcedure LoadProcedureByName(string anInternalName)
        {
            return new TrafodionProceduresLoader().LoadObjectByName(this, anInternalName);
        }
        
        /// <summary>
        /// Creates a Library model
        /// </summary>
        /// <param name="internalName">Internal name of the Library</param>
        /// <returns></returns>
        public TrafodionLibrary LoadLibraryByName(string internalName)
        {
            return new TrafodionLibrariesLoader().LoadObjectByName(this, internalName);
        }

        /// <summary>
        /// Creates a function model
        /// </summary>
        /// <param name="anInternalName">Internal name of the function</param>
        /// <returns></returns>
        public TrafodionUDFunction LoadUDFunctionByName(string anInternalName)
        {
            return new TrafodionUDFunctionsLoader().LoadObjectByName(this, anInternalName);
        }
        /// <summary>
        /// Creates a function action model
        /// </summary>
        /// <param name="anInternalName">Internal name of the function action</param>
        /// <returns></returns>
        public TrafodionFunctionAction LoadFunctionActionByName(string anInternalName)
        {
            return new TrafodionFunctionActionsLoader().LoadObjectByName(this, anInternalName);
        }

        /// <summary>
        /// Returns the connection that was used to retrieve information about this schema.
        /// </summary>
        /// <returns></returns>
        override public Connection GetConnection()
        {
            return _sqlMxCatalog.GetConnection();
        }

        /// <summary>
        /// Checks if the given user has the specificed privilege type on this object
        /// </summary>
        /// <param name="userName">User name</param>
        /// <param name="privilegeType">Privilege Type</param>
        /// <returns>True or False</returns>
        public bool DoesUserHavePrivilege(string userName, string privilegeType)
        {
            SchemaPrivilege[] userPrivileges = (from priv in Privileges where (priv.GranteeName.Equals(userName) || priv.GranteeName.ToUpper().Equals("PUBLIC")) select priv).Distinct().ToArray();
            foreach (SchemaPrivilege priv in userPrivileges)
            {
                if (priv.HasPrivilege(privilegeType))
                    return true;
            }
            return false;
        }

        /// <summary>
        /// Internal Ansi name of the schema
        /// </summary>
        override public string RealAnsiName
        {
            get
            {
                return _sqlMxCatalog.ExternalName + "." + ExternalName;
            }
        }

        /// <summary>
        /// Visible ansi name of the schema
        /// </summary>
        override public string VisibleAnsiName
        {
            get
            {
                return _sqlMxCatalog.ExternalName + "." + ExternalName;
            }
        }

        /// <summary>
        /// Refreshes the list of indexes from the objects under this schema.
        /// </summary>
        public void RefreshIndexes()
        {
            if (_sqlMxIndexes == null)
            {
                _sqlMxIndexes = new List<TrafodionIndex>();
            }
            else
            {
                _sqlMxIndexes.Clear();
            }
            _sqlMxIndexes = new TrafodionIndexesLoader().Load(this);
        }

        /// <summary>
        /// Clears the information about this schema.
        /// </summary>
        override public void Refresh()
        {
            //Create a temp model
            TrafodionSchema aSchema = TheTrafodionCatalog.LoadTrafodionSchema(this.InternalName);

            //If temp model is null, the object has been removed
            //So cleanup and notify the UI
            if (aSchema == null)
            {
                TheTrafodionCatalog.TrafodionSchemas.Remove(this);
                OnModelRemovedEvent();
                return;
            }
            if (this.CompareTo(aSchema) != 0)
            {
                //If catalog object has been recreated, attach the new catalog model to the parent.
                TheTrafodionCatalog.TrafodionSchemas.Remove(this);
                TheTrafodionCatalog.TrafodionSchemas.Add(aSchema);
                this.OnModelReplacedEvent(aSchema);
            }
            else
            {
                base.Refresh();
                _sqlMxTables = null;
                _sqlMxMaterializedViews = null;
                _sqlMxMaterializedViewGroups = null;
                _schemaPrivileges = null;
                _sqlMxLibraries = null;
                _sqlMxProcedures = null;
                _sqlMxSynonyms = null;
                _sqlMxViews = null;
                _sqlMxIndexes = null;
                _sqlMxUDFunctions = null;
            }
        }

        /// <summary>
        /// Adds a procedure model to its list of procedures
        /// Raises an event to notify its listeners that a new procedure has been created
        /// </summary>
        /// <param name="aTrafodionProcedure"></param>
        public void AddProcedure(TrafodionProcedure aTrafodionProcedure)
        {
            TrafodionProcedures.Add(aTrafodionProcedure);
            OnModelChangedEvent(new TrafodionModelChangeEventArgs(ChangeEvent.ProcedureCreated, aTrafodionProcedure));
        }

        public void DropProcedure(TrafodionProcedure aTrafodionProcedure)
        {
            TrafodionProcedures.Remove(aTrafodionProcedure);
            OnModelChangedEvent(new TrafodionModelChangeEventArgs(ChangeEvent.ProcedureDropped, aTrafodionProcedure));
        }



        /// <summary>
        /// Adds a Library model to its list of libraries
        /// Notify its listeners that a new Library has been created
        /// </summary>
        /// <param name="aTrafodionLibrary"></param>
        public void AddLibrary(TrafodionLibrary aTrafodionLibrary)
        {
            TrafodionLibraries.Add(aTrafodionLibrary);
            OnModelChangedEvent(new TrafodionModelChangeEventArgs(ChangeEvent.LibraryCreated, aTrafodionLibrary));
        }

        /// <summary>
        /// Drop a Library model to its list of libraries
        /// Notify its listeners that a new Library has been dropped
        /// </summary>
        /// <param name="aTrafodionLibrary"></param>
        public void DropLibrary(TrafodionLibrary aTrafodionLibrary)
        {            
            TrafodionLibraries.Remove(aTrafodionLibrary);
            OnModelChangedEvent(new TrafodionModelChangeEventArgs(ChangeEvent.LibraryDropped, aTrafodionLibrary));
            OnModelChangedEvent(new TrafodionModelChangeEventArgs(ChangeEvent.ProcedureDropped, aTrafodionLibrary));
        }

        /// <summary>
        /// Removes the altered library from the list and forces a model changed event which causes
        /// the library list and models to be refreshed.
        /// </summary>
        /// <param name="aTrafodionLibrary"></param>
        public void AlterLibrary(TrafodionLibrary aTrafodionLibrary)
        {
            TrafodionLibraries.Remove(aTrafodionLibrary);
            OnModelChangedEvent(new TrafodionModelChangeEventArgs(ChangeEvent.LibraryAltered, aTrafodionLibrary));
        }

        /// <summary>
        /// Returns the connection definition used in the connection that retrieved information
        /// about this schema.
        /// </summary>
        override public ConnectionDefinition ConnectionDefinition
        {
            get { return _sqlMxCatalog.ConnectionDefinition; }
        }

        #region IHasTrafodionPrivileges Members


        public void GrantRevoke(TrafodionObject.PrivilegeAction action, ConnectionDefinition aConnectionDefinition, string grantRevokeCommandString, out DataTable sqlWarnings)
        {
            sqlWarnings = new DataTable();
            if (SupportsPrivileges(this))
            {
                Trafodion.Manager.DatabaseArea.Model.Queries.ExecuteGrantRevoke(aConnectionDefinition, grantRevokeCommandString, out sqlWarnings);
                if (_schemaPrivileges != null)
                {
                    _schemaPrivileges.Clear();
                    _schemaPrivileges = null;
                }
                if (action == PrivilegeAction.GRANT)
                {
                    OnModelChangedEvent(new TrafodionModelChangeEventArgs(ChangeEvent.PrivilegeGranted, this));
                }
                else
                {
                    OnModelChangedEvent(new TrafodionModelChangeEventArgs(ChangeEvent.PrivilegeRevoked, this));
                }
            }
        }

        #endregion

        /// <summary>
        /// Do Multipe Views Valdation Operations
        /// </summary>
        /// <param name="sqlMxViews"></param>
        /// <param name="isCascade"></param>
        /// <param name="multipleWarningDataTable"></param>
        /// <returns>A datatable that contains multiple warnings</returns>
        public DataTable ValidateViews(List<TrafodionView> sqlMxViews, bool isCascade, DataTable multipleWarningDataTable)
        {
            if (sqlMxViews.Count > 1) 
            {
                //Sort the sqlMxViews
                sqlMxViews.Sort((a, b) => { return a.TheCreateTime.CompareTo(b.TheCreateTime); }); 
            }
            DataTable sqlwarnings = new DataTable();
            foreach (var item in sqlMxViews)
            {                
                item.ValidateView(isCascade, out sqlwarnings);
                if (multipleWarningDataTable != null && multipleWarningDataTable.Columns.Count != 0)
                    multipleWarningDataTable = sqlwarnings.Clone();
                multipleWarningDataTable.Merge(sqlwarnings);
            }
            return multipleWarningDataTable;            
        }


        public void FireChangedEventAtSchemaLevel()
        {
            OnModelChangedEvent(new TrafodionModelChangeEventArgs(ChangeEvent.SchemaViewValidated, this));
        }

        public void FireChangedEventAtViewLevel()
        {
            OnModelChangedEvent(new TrafodionModelChangeEventArgs(ChangeEvent.ViewValidated, this));
        }
    }

    /// <summary>
    /// The object loader for TrafodionSchemaObject objects.
    /// </summary>
    class TrafodionTablesLoader : TrafodionObjectsLoader<TrafodionSchema, TrafodionTable>
    {
        override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionSchema aTrafodionSchema)
        {
            return Queries.ExecuteSelectSchemaObjectNames(aConnection, aTrafodionSchema.TheTrafodionCatalog, aTrafodionSchema.InternalName, aTrafodionSchema.Version, TrafodionTable.ObjectType, TrafodionTable.ObjectNameSpace);
        }

        override protected void LoadOne(List<TrafodionTable> aList, TrafodionSchema aTrafodionSchema, OdbcDataReader aReader)
        {
            aList.Add(new TrafodionTable(aTrafodionSchema, aReader.GetString(0).TrimEnd(), aReader.GetInt64(1), aReader.GetInt64(2), aReader.GetInt64(3), aReader.GetString(4), aReader.GetString(5).Trim().Replace("\0", "")));
        }

        override protected OdbcDataReader GetReaderForSqlObject(Connection aConnection, TrafodionSchema aTrafodionSchema, string aTableName)
        {
            return Queries.ExecuteSelectSchemaObjectByName(aConnection, aTrafodionSchema.TheTrafodionCatalog, aTrafodionSchema.InternalName, aTrafodionSchema.Version, TrafodionTable.ObjectType, aTableName, TrafodionTable.ObjectNameSpace);
        }
        protected override TrafodionTable ReadSqlObject(TrafodionSchema aTrafodionSchema, OdbcDataReader aReader)
        {
            return new TrafodionTable(aTrafodionSchema, aReader.GetString(0).TrimEnd(), aReader.GetInt64(1), aReader.GetInt64(2), aReader.GetInt64(3), aReader.GetString(4), aReader.GetString(5).Trim().Replace("\0", ""));
        }
    }

    /// <summary>
    /// The object loader for TrafodionMaterializedView objects.
    /// </summary>
    class TrafodionMaterializedViewsLoader : TrafodionObjectsLoader<TrafodionSchema, TrafodionMaterializedView>
    {
        override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionSchema aTrafodionSchema)
        {
            return Queries.ExecuteSelectSchemaObjectNames(aConnection, aTrafodionSchema.TheTrafodionCatalog, aTrafodionSchema.InternalName, aTrafodionSchema.Version, TrafodionMaterializedView.ObjectType, TrafodionMaterializedView.ObjectNameSpace);
        }

        override protected void LoadOne(List<TrafodionMaterializedView> aList, TrafodionSchema aTrafodionSchema, OdbcDataReader aReader)
        {
            aList.Add(new TrafodionMaterializedView(aTrafodionSchema, aReader.GetString(0).TrimEnd(), aReader.GetInt64(1), aReader.GetInt64(2), aReader.GetInt64(3), aReader.GetString(4), aReader.GetString(5).Trim().Replace("\0", "")));
        }
        override protected OdbcDataReader GetReaderForSqlObject(Connection aConnection, TrafodionSchema aTrafodionSchema, string objectName)
        {
            return Queries.ExecuteSelectSchemaObjectByName(aConnection, aTrafodionSchema.TheTrafodionCatalog, aTrafodionSchema.InternalName, aTrafodionSchema.Version, TrafodionMaterializedView.ObjectType, objectName, TrafodionMaterializedView.ObjectNameSpace);
        }
        protected override TrafodionMaterializedView ReadSqlObject(TrafodionSchema aTrafodionSchema, OdbcDataReader aReader)
        {
            return new TrafodionMaterializedView(aTrafodionSchema, aReader.GetString(0).TrimEnd(), aReader.GetInt64(1), aReader.GetInt64(2), aReader.GetInt64(3), aReader.GetString(4), aReader.GetString(5).Trim().Replace("\0", ""));
        }
    }

    /// <summary>
    /// The object loader for TrafodionMaterializedViewGroup objects.
    /// </summary>
    class TrafodionMaterializedViewGroupsLoader : TrafodionObjectsLoader<TrafodionSchema, TrafodionMaterializedViewGroup>
    {
        override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionSchema aTrafodionSchema)
        {
            return Queries.ExecuteSelectSchemaObjectNames(aConnection, aTrafodionSchema.TheTrafodionCatalog, aTrafodionSchema.InternalName, aTrafodionSchema.Version, TrafodionMaterializedViewGroup.ObjectType, TrafodionMaterializedViewGroup.ObjectNameSpace);
        }

        override protected void LoadOne(List<TrafodionMaterializedViewGroup> aList, TrafodionSchema aTrafodionSchema, OdbcDataReader aReader)
        {
            aList.Add(new TrafodionMaterializedViewGroup(aTrafodionSchema, aReader.GetString(0).TrimEnd(), aReader.GetInt64(1), aReader.GetInt64(2), aReader.GetInt64(3), aReader.GetString(4), aReader.GetString(5).Trim().Replace("\0","")));
        }
        override protected OdbcDataReader GetReaderForSqlObject(Connection aConnection, TrafodionSchema aTrafodionSchema, string objectName)
        {
            return Queries.ExecuteSelectSchemaObjectByName(aConnection, aTrafodionSchema.TheTrafodionCatalog, aTrafodionSchema.InternalName, aTrafodionSchema.Version, TrafodionMaterializedViewGroup.ObjectType, objectName, TrafodionMaterializedViewGroup.ObjectNameSpace);
        }
        protected override TrafodionMaterializedViewGroup ReadSqlObject(TrafodionSchema aTrafodionSchema, OdbcDataReader aReader)
        {
            return new TrafodionMaterializedViewGroup(aTrafodionSchema, aReader.GetString(0).TrimEnd(), aReader.GetInt64(1), aReader.GetInt64(2), aReader.GetInt64(3), aReader.GetString(4), aReader.GetString(5).Trim().Replace("\0",""));
        }
    }

    /// <summary>
    /// The object loader for TrafodionProcedure objects.
    /// </summary>
    class TrafodionProceduresLoader : TrafodionObjectsLoader<TrafodionSchema, TrafodionProcedure>
    {
        override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionSchema aTrafodionSchema)
        {
            return Queries.ExecuteSelectRoutines(aConnection, aTrafodionSchema, TrafodionProcedure.ObjectNameSpace);
        }

        override protected void LoadOne(List<TrafodionProcedure> aList, TrafodionSchema aTrafodionSchema, OdbcDataReader aReader)
        {
            string udrType = aReader.GetString(5).TrimEnd();
            if (udrType.Equals("P"))
            {
                aList.Add(new TrafodionProcedure(aTrafodionSchema, aReader.GetString(0).TrimEnd(), aReader.GetInt64(1), aReader.GetInt64(2), aReader.GetInt64(3), aReader.GetString(4), aReader.GetString(6).Trim().Replace("\0", "")));
            }
        }
        override protected OdbcDataReader GetReaderForSqlObject(Connection aConnection, TrafodionSchema aTrafodionSchema, string objectName)
        {
            return Queries.ExecuteSelectRoutineByName(aConnection, aTrafodionSchema, objectName, TrafodionProcedure.ObjectNameSpace);
        }
        protected override TrafodionProcedure ReadSqlObject(TrafodionSchema aTrafodionSchema, OdbcDataReader aReader)
        {
            string udrType = aReader.GetString(5).TrimEnd();
            if (udrType.Equals("P"))
            {
                return new TrafodionProcedure(aTrafodionSchema, aReader.GetString(0).TrimEnd(), aReader.GetInt64(1), aReader.GetInt64(2), aReader.GetInt64(3), aReader.GetString(4), aReader.GetString(6).Trim().Replace("\0", ""));
            }
            return null;
        }
    }


    /// <summary>
    /// The object loader for TrafodionLibrary objects.
    /// </summary>
    class TrafodionLibrariesLoader : TrafodionObjectsLoader<TrafodionSchema, TrafodionLibrary>
    {
        override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionSchema aTrafodionSchema)
        {
            return Queries.ExecuteSelectLibraries(aConnection, aTrafodionSchema, TrafodionLibrary.ObjectType);
        }

        override protected void LoadOne(List<TrafodionLibrary> libraryList, TrafodionSchema aTrafodionSchema, OdbcDataReader aReader)
        {
            libraryList.Add(new TrafodionLibrary(aTrafodionSchema, aReader.GetString(0).TrimEnd(), aReader.GetInt64(1), aReader.GetInt64(2), aReader.GetInt64(3), aReader.GetString(4), aReader.GetString(5).Trim().Replace("\0", ""), aReader.GetString(6).Trim(), aReader.GetString(7).Trim(), aReader.GetString(8).Trim()));
        }
        override protected OdbcDataReader GetReaderForSqlObject(Connection aConnection, TrafodionSchema aTrafodionSchema, string objectName)
        {
            return Queries.ExecuteSelectLibraryByName(aConnection, aTrafodionSchema, objectName, TrafodionLibrary.ObjectType);
        }
        protected override TrafodionLibrary ReadSqlObject(TrafodionSchema aTrafodionSchema, OdbcDataReader aReader)
        {
            return new TrafodionLibrary(aTrafodionSchema, aReader.GetString(0).TrimEnd(), aReader.GetInt64(1), aReader.GetInt64(2), aReader.GetInt64(3), aReader.GetString(4), aReader.GetString(5).Trim().Replace("\0", ""), aReader.GetString(6).Trim(), aReader.GetString(7).Trim(), aReader.GetString(8).Trim());
        }
    }

    /// <summary>
    /// The object loader for TrafodionUDFunction objects.
    /// </summary>
    class TrafodionUDFunctionsLoader : TrafodionObjectsLoader<TrafodionSchema, TrafodionUDFunction>
    {
        override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionSchema aTrafodionSchema)
        {
            return Queries.ExecuteSelectRoutines(aConnection, aTrafodionSchema, TrafodionUDFunction.ObjectNameSpace);
        }

        override protected void LoadOne(List<TrafodionUDFunction> aList, TrafodionSchema aTrafodionSchema, OdbcDataReader aReader)
        {
            TrafodionUDFunction udFunction = new TrafodionUDFunction(aTrafodionSchema, aReader.GetString(0).TrimEnd(), aReader.GetInt64(1), aReader.GetInt64(2), aReader.GetInt64(3), aReader.GetString(4), aReader.GetString(6).Trim().Replace("\0", "") );
            string IsUniversal = aReader.GetString(7).TrimEnd();
            udFunction.IsUniversal = IsUniversal.Equals("Y");
            aList.Add(udFunction);
        }
        override protected OdbcDataReader GetReaderForSqlObject(Connection aConnection, TrafodionSchema aTrafodionSchema, string objectName)
        {
            return Queries.ExecuteSelectRoutineByName(aConnection, aTrafodionSchema, objectName, TrafodionUDFunction.ObjectNameSpace);
        }

        protected override TrafodionUDFunction ReadSqlObject(TrafodionSchema aTrafodionSchema, OdbcDataReader aReader)
        {
            TrafodionUDFunction udFunction = new TrafodionUDFunction(aTrafodionSchema, aReader.GetString(0).TrimEnd(), aReader.GetInt64(1), aReader.GetInt64(2), aReader.GetInt64(3), aReader.GetString(4), aReader.GetString(6).Trim().Replace("\0", ""));
            string IsUniversal = aReader.GetString(7).TrimEnd();
            udFunction.IsUniversal = IsUniversal.Equals("Y");
            return udFunction;
        }
    }
    /// <summary>
    /// The object loader for TrafodionFunction action objects.
    /// </summary>
    class TrafodionFunctionActionsLoader : TrafodionObjectsLoader<TrafodionSchema, TrafodionFunctionAction>
    {
        override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionSchema aTrafodionSchema)
        {
            return Queries.ExecuteSelectFunctionActions(aConnection, aTrafodionSchema);
        }

        override protected void LoadOne(List<TrafodionFunctionAction> aList, TrafodionSchema aTrafodionSchema, OdbcDataReader aReader)
        {
            TrafodionFunctionAction functionAction = new TrafodionFunctionAction(aTrafodionSchema, aReader.GetString(0).TrimEnd(), aReader.GetInt64(1), aReader.GetInt64(2), aReader.GetInt64(3), aReader.GetString(4), aReader.GetString(7).Trim().Replace("\0", ""));
            long functionSchemaUID = aReader.GetInt64(5);
            long functionUID = aReader.GetInt64(6);
            try
            {
                TrafodionSchema functionSchema = aTrafodionSchema.TheTrafodionCatalog.FindSchema(functionSchemaUID);
                TrafodionUDFunction function = functionSchema.FindFunction(functionUID);
                if (function != null)
                {
                    functionAction.TrafodionUDFunction = function;
                    function.AddAction(functionAction);
                }
                aList.Add(functionAction);
            }
            catch (Exception ex)
            {
            }
        }

        override protected OdbcDataReader GetReaderForSqlObject(Connection aConnection, TrafodionSchema aTrafodionSchema, string objectName)
        {
            return Queries.ExecuteSelectActionByName(aConnection, aTrafodionSchema, objectName); //Queries.ExecuteSelectSchemaObjectByName(aConnection, aTrafodionSchema.TheTrafodionCatalog, aTrafodionSchema.UID, aTrafodionSchema.Version, TrafodionFunctionAction.ObjectType, objectName, TrafodionFunctionAction.ObjectNameSpace);
        }
        protected override TrafodionFunctionAction ReadSqlObject(TrafodionSchema aTrafodionSchema, OdbcDataReader aReader)
        {
            TrafodionFunctionAction functionAction = new TrafodionFunctionAction(aTrafodionSchema, aReader.GetString(0).TrimEnd(), aReader.GetInt64(1), aReader.GetInt64(2), aReader.GetInt64(3), aReader.GetString(4), aReader.GetString(7).Trim().Replace("\0", ""));

            long functionSchemaUID = aReader.GetInt64(5);
            long functionUID = aReader.GetInt64(6);
            TrafodionSchema functionSchema = aTrafodionSchema.TheTrafodionCatalog.FindSchema(functionSchemaUID);
            TrafodionUDFunction function = functionSchema.FindFunction(functionUID);
            if (function != null)
            {
                functionAction.TrafodionUDFunction = function;
                function.AddAction(functionAction);
            }
            return functionAction;
        }
    }

    /// <summary>
    /// The object loader for TrafodionTableMappingFunction objects.
    /// </summary>
    class TrafodionTableMappingFunctionsLoader : TrafodionObjectsLoader<TrafodionSchema, TrafodionTableMappingFunction>
    {
        override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionSchema aTrafodionSchema)
        {
            return Queries.ExecuteSelectRoutines(aConnection, aTrafodionSchema, TrafodionTableMappingFunction.ObjectNameSpace);
        }

        override protected void LoadOne(List<TrafodionTableMappingFunction> aList, TrafodionSchema aTrafodionSchema, OdbcDataReader aReader)
        {
            string udrType = aReader.GetString(5).TrimEnd();
            if (udrType.Equals("T"))
            {
                aList.Add(new TrafodionTableMappingFunction(aTrafodionSchema, aReader.GetString(0).TrimEnd(), aReader.GetInt64(1), aReader.GetInt64(2), aReader.GetInt64(3), aReader.GetString(4), aReader.GetString(6).Trim().Replace("\0", "")));
            }
        }
        override protected OdbcDataReader GetReaderForSqlObject(Connection aConnection, TrafodionSchema aTrafodionSchema, string objectName)
        {
            return Queries.ExecuteSelectRoutineByName(aConnection, aTrafodionSchema, objectName, TrafodionTableMappingFunction.ObjectNameSpace);
        }
        protected override TrafodionTableMappingFunction ReadSqlObject(TrafodionSchema aTrafodionSchema, OdbcDataReader aReader)
        {
            string udrType = aReader.GetString(5).TrimEnd();
            if (udrType.Equals("T"))
            {
                return new TrafodionTableMappingFunction(aTrafodionSchema, aReader.GetString(0).TrimEnd(), aReader.GetInt64(1), aReader.GetInt64(2), aReader.GetInt64(3), aReader.GetString(4), aReader.GetString(6).Trim().Replace("\0", ""));
            }
            return null;
        }
    }

    /// <summary>
    /// The object loader for TrafodionSynonym objects.
    /// </summary>
    class TrafodionSynonymsLoader : TrafodionObjectsLoader<TrafodionSchema, TrafodionSynonym>
    {
        override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionSchema aTrafodionSchema)
        {
            return Queries.ExecuteSelectSchemaObjectNames(aConnection, aTrafodionSchema.TheTrafodionCatalog, aTrafodionSchema.InternalName, aTrafodionSchema.Version, TrafodionSynonym.ObjectType, TrafodionSynonym.ObjectNameSpace);
        }

        override protected void LoadOne(List<TrafodionSynonym> aList, TrafodionSchema aTrafodionSchema, OdbcDataReader aReader)
        {
            aList.Add(new TrafodionSynonym(aTrafodionSchema, aReader.GetString(0).TrimEnd(), aReader.GetInt64(1), aReader.GetInt64(2), aReader.GetInt64(3), aReader.GetString(4), aReader.GetString(5).Trim().Replace("\0","")));
        }
        override protected OdbcDataReader GetReaderForSqlObject(Connection aConnection, TrafodionSchema aTrafodionSchema, string objectName)
        {
            return Queries.ExecuteSelectSchemaObjectByName(aConnection, aTrafodionSchema.TheTrafodionCatalog, aTrafodionSchema.InternalName, aTrafodionSchema.Version, TrafodionSynonym.ObjectType, objectName, TrafodionSynonym.ObjectNameSpace);
        }
        protected override TrafodionSynonym ReadSqlObject(TrafodionSchema aTrafodionSchema, OdbcDataReader aReader)
        {
            return new TrafodionSynonym(aTrafodionSchema, aReader.GetString(0).TrimEnd(), aReader.GetInt64(1), aReader.GetInt64(2), aReader.GetInt64(3), aReader.GetString(4), aReader.GetString(5).Trim().Replace("\0", ""));
        }
    }

    /// <summary>
    /// The object loader for TrafodionView objects.
    /// </summary>
    class TrafodionViewsLoader : TrafodionObjectsLoader<TrafodionSchema, TrafodionView>
    {
        override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionSchema aTrafodionSchema)
        {
            return Queries.ExecuteSelectViewsInSchema(aConnection, aTrafodionSchema);
            //return Queries.ExecuteSelectSchemaObjectNames(aConnection, aTrafodionSchema.TheTrafodionCatalog, aTrafodionSchema.InternalName, aTrafodionSchema.Version, TrafodionView.ObjectType, TrafodionSynonym.ObjectNameSpace);
        }

        override protected void LoadOne(List<TrafodionView> aList, TrafodionSchema aTrafodionSchema, OdbcDataReader aReader)
        {
            TrafodionView aTrafodionView = null;
            if (aTrafodionSchema.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140)
            {
                aTrafodionView = new TrafodionView(aTrafodionSchema, aReader.GetString(0).TrimEnd(), aReader.GetInt64(1), aReader.GetInt64(2), aReader.GetInt64(3), aReader.GetString(4), aReader.GetString(6).Trim().Replace("\0", ""), aReader.GetString(7).TrimEnd());
            }
            else 
            {
                aTrafodionView = new TrafodionView(aTrafodionSchema, aReader.GetString(0).TrimEnd(), aReader.GetInt64(1), aReader.GetInt64(2), aReader.GetInt64(3), aReader.GetString(4), aReader.GetString(6).Trim().Replace("\0", ""));
            }
            
            if (aTrafodionSchema.Version >= 2500)
            {
                aTrafodionView.ViewType = aReader.GetString(5);
            }
            else
            {
                string textPreview = aReader.GetString(5);
                aTrafodionView.ViewType = textPreview.StartsWith("CREATE SYSTEM VIEW") ? "SV" : "UV";
            } 
            aList.Add(aTrafodionView);
        }
        override protected OdbcDataReader GetReaderForSqlObject(Connection aConnection, TrafodionSchema aTrafodionSchema, string objectName)
        {
            return Queries.ExecuteSelectViewByName(aConnection, aTrafodionSchema, objectName);
        }
        protected override TrafodionView ReadSqlObject(TrafodionSchema aTrafodionSchema, OdbcDataReader aReader)
        {
            TrafodionView aTrafodionView = null;
            if (aTrafodionSchema.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140)
            {
                aTrafodionView = new TrafodionView(aTrafodionSchema, aReader.GetString(0).TrimEnd(), aReader.GetInt64(1), aReader.GetInt64(2), aReader.GetInt64(3), aReader.GetString(4), aReader.GetString(6).Trim().Replace("\0", ""), aReader.GetString(7).TrimEnd());
            }
            else
            {
                aTrafodionView = new TrafodionView(aTrafodionSchema, aReader.GetString(0).TrimEnd(), aReader.GetInt64(1), aReader.GetInt64(2), aReader.GetInt64(3), aReader.GetString(4), aReader.GetString(6).Trim().Replace("\0", ""));
            }
            if (aTrafodionSchema.Version >= 2500)
            {
                aTrafodionView.ViewType = aReader.GetString(5);
            }
            else
            {
                string textPreview = aReader.GetString(5);
                aTrafodionView.ViewType = textPreview.StartsWith("CREATE SYSTEM VIEW") ? "SV" : "UV";
            } 
            return aTrafodionView;
        }
    }

    /// <summary>
    /// The object loader for TrafodionSequence objects.
    /// </summary>
    class TrafodionSequencesLoader : TrafodionObjectsLoader<TrafodionSchema, TrafodionSequence>
    {
        override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionSchema aTrafodionSchema)
        {
            return Queries.ExecuteSelectSchemaObjectNames(aConnection, aTrafodionSchema.TheTrafodionCatalog, aTrafodionSchema.InternalName, aTrafodionSchema.Version, TrafodionSequence.ObjectType, TrafodionSequence.ObjectNameSpace);
        }

        override protected void LoadOne(List<TrafodionSequence> aList, TrafodionSchema aTrafodionSchema, OdbcDataReader aReader)
        {
            aList.Add(new TrafodionSequence(aTrafodionSchema, aReader.GetString(0).TrimEnd(), aReader.GetInt64(1), aReader.GetInt64(2), aReader.GetInt64(3), aReader.GetString(4), aReader.GetString(5).Trim().Replace("\0", "")));
        }
        override protected OdbcDataReader GetReaderForSqlObject(Connection aConnection, TrafodionSchema aTrafodionSchema, string objectName)
        {
            return Queries.ExecuteSelectSchemaObjectByName(aConnection, aTrafodionSchema.TheTrafodionCatalog, aTrafodionSchema.InternalName, aTrafodionSchema.Version, TrafodionSequence.ObjectType, objectName, TrafodionSequence.ObjectNameSpace);
        }
        protected override TrafodionSequence ReadSqlObject(TrafodionSchema aTrafodionSchema, OdbcDataReader aReader)
        {
            return new TrafodionSequence(aTrafodionSchema, aReader.GetString(0).TrimEnd(), aReader.GetInt64(1), aReader.GetInt64(2), aReader.GetInt64(3), aReader.GetString(4), aReader.GetString(5).Trim().Replace("\0", ""));
        }
    }

    /// <summary>
    /// The object loader for TrafodionIndex objects.
    /// </summary>
    class TrafodionIndexesLoader : TrafodionObjectsLoader<TrafodionSchema, TrafodionIndex>
    {
        override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionSchema aTrafodionSchema)
        {
            return Queries.ExecuteSelectIndexesForSchema(aConnection, aTrafodionSchema);
        }

        override protected void LoadOne(List<TrafodionIndex> aList, TrafodionSchema aTrafodionSchema, OdbcDataReader aReader)
        {
            //Get the parent object name (eg. table or MV)
            string aBaseTableName = aReader.GetString(0).TrimEnd();
            string aBaseTableType = aReader.GetString(1).TrimEnd();

            IndexedSchemaObject anIndexedSchemaObject = null;
            //Find the model for the parent table
            if (aBaseTableType.Equals(TrafodionTable.ObjectType))
            {
                anIndexedSchemaObject = aTrafodionSchema.FindTable(aBaseTableName);
            }
            else
            {
                anIndexedSchemaObject = aTrafodionSchema.FindMaterializedView(aBaseTableName);
            }

            if (anIndexedSchemaObject != null)
            {
                //Create a model for the index, link it to the parent object's model
                //Then add the index model to the index list
                aList.Add(new TrafodionIndex(anIndexedSchemaObject, aReader.GetString(2).TrimEnd(), aReader.GetInt64(3),
                    aReader.GetInt64(4), aReader.GetInt64(5), anIndexedSchemaObject.TheSecurityClass, aReader.GetString(6).Trim().Replace("\0", "")));
            }
        }
    }
}
