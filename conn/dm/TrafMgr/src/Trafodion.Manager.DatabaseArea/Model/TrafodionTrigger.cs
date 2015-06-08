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
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Model
{
	/// <summary>
	/// 
	/// </summary>
	public class TrafodionTrigger
		: TrafodionSchemaObject
    {
        #region Member Variables

        /// <summary>
        /// Object name space
        /// </summary>
        public const string ObjectNameSpace = "TR";
        /// <summary>
        /// Object type
        /// </summary>
        public const string ObjectType = "TR";
        private string[] ddlPatterns = { "CREATE TRIGGER" };

        // Optimization: Trigger attributes are loaded when the trigger is created.
        private bool _areAttributesLoaded = true;
        private string _isEnabled;
        private string _activationTime; 
        private string _operation; 
        private string _granularity;
        private TrafodionTable _sqlMxTable;

        #endregion

        public TrafodionTrigger(TrafodionTable aTrafodionTable, string anInternalName, long aUID, long aCreateTime, long aRedefTime, string enabled, string activation, string operation, string granularity, string aSecurityClass, string anOwner)
            : base(aTrafodionTable.TheTrafodionSchema, anInternalName, aUID, aCreateTime, aRedefTime, aSecurityClass, anOwner)
		{
            _sqlMxTable = aTrafodionTable;
            _isEnabled = enabled;
            _activationTime = activation;
            _operation = operation;
            _granularity = granularity;
		}

		override public string SchemaObjectType
		{ 
			get
			{
                return ObjectType;
			}
		}

        /// <summary>
        /// Displayable Object type for the trigger
        /// </summary>
        override public string DisplayObjectType
        {
            get
            {
                return Properties.Resources.Trigger;
            }
        }
        override public string SchemaObjectNameSpace
		{ 
			get
			{
                return ObjectNameSpace;
			}
		}

        /// <summary>
        /// Load the attributes of the trigger
        /// </summary>
        /// <returns></returns>
        public bool LoadAttributes()
        {
            if (_areAttributesLoaded)
            {
                return true;
            }

            Connection theConnection = null;

            try
            {
                theConnection = GetConnection();

                OdbcDataReader theReader = Queries.ExecuteSelectTriggerNames(theConnection, _sqlMxTable.TheTrafodionCatalog, _sqlMxTable.TheTrafodionSchema.Version, _sqlMxTable.UID);
                theReader.Read();
                SetAttributes(theReader);
            }
            finally
            {
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }
            _areAttributesLoaded = true;
            return true;
        }

        /// <summary>
        /// Store the data you get back from the query in LoadAttributes
        /// </summary>
        /// <param name="aReader"></param>
        public void SetAttributes(OdbcDataReader aReader)
        {
            _isEnabled = aReader.GetString(4).Trim();
            _activationTime = aReader.GetString(5).Trim();
            _operation = aReader.GetString(6).Trim();
            _granularity = aReader.GetString(7).Trim();
        }

    #region Properties  
    
        /// <summary>
        /// The parent table on which this trigger has been created
        /// </summary>
        public TrafodionTable TrafodionTable
        {
            get { return _sqlMxTable; }
        }

        /// <summary>
        /// DDL text for the Trigger
        /// </summary>
        public override string DDLText
        {
            get
            {
                if (_ddlText == null || _ddlText.Length == 0)
                {
                    //Get the DDL text from the referenced object's DDL.
                    _ddlText = TrafodionObjectDDLLoader.FindChildDDL(_sqlMxTable.DDLText, VisibleAnsiName,
                        ddlPatterns);
                    if (string.IsNullOrEmpty(_ddlText))
                    {
                        _ddlText = TrafodionObjectDDLLoader.FindChildDDL(_sqlMxTable.DDLText, TheTrafodionSchema.ExternalName + "." + ExternalName,
                        ddlPatterns);
                    }
                }
                return _ddlText;
            }
        }

        /// <summary>
        /// Returns a string that can be used to display is enabled intelligently
        /// </summary>
        public string FormattedIsEnabled
        {
            get 
            {
                LoadAttributes();
                return Utilities.YesNo(_isEnabled);
            }
        }

        /// <summary>
        /// Returns a string that can be used to display Activation Time intelligently
        /// </summary>
        public string FormattedActivationTime
        {
            get 
            {
                string activationTime;
                LoadAttributes();
                if (_activationTime.Equals("B") )
                {
                    activationTime = Properties.Resources.Before;
                }
                else if (_activationTime.Equals("A") )
                {
                    activationTime = Properties.Resources.After;
                }
                else
                    activationTime = Properties.Resources.Unknown;

                return activationTime;
            }
        }

        /// <summary>
        /// Returns a string that can be used to display Operation intelligently
        /// </summary>
        public string FormattedOperation
        {
            get 
            {
                string operation;

                LoadAttributes();

                if (_operation.Equals("I"))
                {
                    operation = Properties.Resources.Insert;
                }
                else if (_operation.Equals("D") )
                {
                    operation = Properties.Resources.Delete;
                }
                else if (_operation.Equals("U") )
                {
                    operation = Properties.Resources.Update;
                }
                else
                    operation = Properties.Resources.Unknown;

                return operation; 
    
           }
        }

        /// <summary>
        /// Returns a string that can be used to display Granularity intelligently
        /// </summary>
        public string FormattedGranularity
        {
            get
            {
                string granularity;

                LoadAttributes();

                if (_granularity.Equals("R"))
                {
                    granularity = Properties.Resources.Row;
                }
                else if (_granularity.Equals("S"))
                {
                    granularity = Properties.Resources.Statement;
                }
                else
                    granularity = Properties.Resources.Unknown;

                return granularity;
            }

        }

    #endregion

        /// <summary>
        /// Returns a string that can be used to display Operation intelligently
        /// </summary>
        public static string UsageFormattedOperation (string operation)
        {
                string aOperation;
                if (operation.Equals("I"))
                {
                    aOperation = Properties.Resources.Insert;
                }
                else if (operation.Equals("D"))
                {
                    aOperation = Properties.Resources.Delete;
                }
                else if (operation.Equals("U"))
                {
                    aOperation = Properties.Resources.Update;
                }
                else if (operation.Equals("S"))
                {
                    aOperation = Properties.Resources.Select;
                }
                else if (operation.Equals("R"))
                {
                    aOperation = Properties.Resources.Routine;
                }
                else
                    aOperation = Properties.Resources.Unknown;

                return aOperation;
        }

        /// <summary>
        /// Returns a string that can be used to display Is Subject intelligently
        /// </summary>
        public static string UsageFormattedIsSubjectTable(string isSubjectTable)
        {
            
            return Utilities.TrueFalse(isSubjectTable);
     
        }

        private List<TrafodionTriggerUsage> _usesViewTrafodionTrigger = null;
        /// <summary>
        /// List of materialized views using this MV
        /// </summary>
        public List<TrafodionTriggerUsage> TheUsesViewsTrafodionTrigger
        {
            get
            {
                if (_usesViewTrafodionTrigger == null)
                {
                   new usageTrafodionTriggerLoader().Load(this);
                }
                return _usesViewTrafodionTrigger;
            }
        }

        
        private List<TrafodionTriggerUsage> _usesTableTrafodionTrigger = null;
        /// <summary>
        /// List of Table using this MV
        /// </summary>
        public List<TrafodionTriggerUsage> TheUsesTablesTrafodionTrigger
        {
            get
            {
                if (_usesTableTrafodionTrigger == null)
                {
                   new usageTrafodionTriggerLoader().Load(this);
                }
                return _usesTableTrafodionTrigger;
            }
        }

        private List<TrafodionTriggerUsage> _usesRoutinesTrafodionTrigger = null;
        /// <summary>
        /// List of Table using this MV
        /// </summary>
        public List<TrafodionTriggerUsage> TheUsesRoutinesTrafodionTrigger
        {
            get
            {
                if (_usesRoutinesTrafodionTrigger == null)
                {
                   new TriggerRoutinesUsageLoader().Load(this);
                }
                return _usesRoutinesTrafodionTrigger;
            }
        }

       class usageTrafodionTriggerLoader : TrafodionObjectsLoader<TrafodionTrigger, TrafodionTriggerUsage>
        {
            override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionTrigger aTrafodionTrigger)
            {
                return Queries.ExecuteSelectTriggerUsages(aConnection, aTrafodionTrigger.TheTrafodionCatalog.ExternalName, aTrafodionTrigger.TheTrafodionSchema.Version, aTrafodionTrigger.UID);
            }

            /// <summary>
            /// Retrieve the Tables from their respective schemas.
            /// </summary>
            /// <param name="aList"></param>
            /// <param name="aTrafodionTrigger"></param>
            /// <param name="aReader"></param>
            override protected void LoadOne(List<TrafodionTriggerUsage> aList, TrafodionTrigger aTrafodionTrigger, OdbcDataReader aReader)
            {
                string objectName = aReader.GetString(0).TrimEnd();
                string objectType = aReader.GetString(1).Trim();
                long schemaUID = aReader.GetInt64(2);
                string operation = aReader.GetString(3).Trim();
                string isSubjectTable = aReader.GetString(4).Trim();
                List<TrafodionTriggerUsage> theList = new List<TrafodionTriggerUsage>();

                if (aTrafodionTrigger._usesTableTrafodionTrigger == null)
                    aTrafodionTrigger._usesTableTrafodionTrigger = new List<TrafodionTriggerUsage>();
                if (aTrafodionTrigger._usesViewTrafodionTrigger == null)
                    aTrafodionTrigger._usesViewTrafodionTrigger = new List<TrafodionTriggerUsage>();
                //if (aTrafodionTrigger._usesRoutinesTrafodionTrigger == null)
                //    aTrafodionTrigger._usesRoutinesTrafodionTrigger = new List<TrafodionTriggerUsage>();

                if (objectType.Equals("BT"))
                {
                    TrafodionSchema sqlMxSchema = aTrafodionTrigger.TheTrafodionCatalog.FindSchema(schemaUID);
                    TrafodionTable table = sqlMxSchema.FindTable(objectName);
                    if (table != null)
                    {
                        TrafodionTriggerUsage theTrafodionTriggerUsage = new TrafodionTriggerUsage(sqlMxSchema, table, UsageFormattedOperation(operation), UsageFormattedIsSubjectTable(isSubjectTable));
                        aTrafodionTrigger._usesTableTrafodionTrigger.Add(theTrafodionTriggerUsage);
                    }
                }
                if (objectType.Equals("VI"))
                {
                    TrafodionSchema sqlMxSchema = aTrafodionTrigger.TheTrafodionCatalog.FindSchema(schemaUID);
                    TrafodionView view = sqlMxSchema.FindView(objectName);
                    if (view != null)
                    {
                        TrafodionTriggerUsage theTrafodionTriggerUsage = new TrafodionTriggerUsage(sqlMxSchema, view, UsageFormattedOperation(operation), UsageFormattedIsSubjectTable(isSubjectTable));
                        aTrafodionTrigger._usesViewTrafodionTrigger.Add(theTrafodionTriggerUsage);
                    }
                }
            }
        }
        
        /// <summary>
        /// Loads all the routines used by the trigger
        /// </summary>
       class TriggerRoutinesUsageLoader : TrafodionObjectsLoader<TrafodionTrigger, TrafodionTriggerUsage>
       {
           override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionTrigger aTrafodionTrigger)
           {
               return Queries.ExecuteSelectRoutinesUsedByTrigger(aConnection, aTrafodionTrigger);
           }

           /// <summary>
           /// 
           /// </summary>
           /// <param name="aList"></param>
           /// <param name="aTrafodionTrigger"></param>
           /// <param name="aReader"></param>
           override protected void LoadOne(List<TrafodionTriggerUsage> aList, TrafodionTrigger aTrafodionTrigger, OdbcDataReader aReader)
           {
               string objectName = aReader.GetString(0).TrimEnd();
               long schemaUID = aReader.GetInt64(1);
               string operation = aReader.GetString(2).Trim();
               string isSubjectTable = aReader.GetString(3).Trim();
               string udrType = aReader.GetString(4).Trim();
               List<TrafodionTriggerUsage> theList = new List<TrafodionTriggerUsage>();

               if (aTrafodionTrigger._usesRoutinesTrafodionTrigger == null)
                   aTrafodionTrigger._usesRoutinesTrafodionTrigger = new List<TrafodionTriggerUsage>();

               if (udrType.Equals("P"))
               {
                   TrafodionSchema sqlMxSchema = aTrafodionTrigger.TheTrafodionCatalog.FindSchema(schemaUID);
                   TrafodionProcedure procedure = sqlMxSchema.FindProcedure(objectName);
                   if (procedure != null)
                   {
                       TrafodionTriggerUsage theTrafodionTriggerUsage = new TrafodionTriggerUsage(sqlMxSchema, procedure, UsageFormattedOperation(operation), UsageFormattedIsSubjectTable(isSubjectTable));
                       aTrafodionTrigger._usesRoutinesTrafodionTrigger.Add(theTrafodionTriggerUsage);
                   }
               }
               else if (udrType.Equals("F"))
               {
                   TrafodionSchema sqlMxSchema = aTrafodionTrigger.TheTrafodionCatalog.FindSchema(schemaUID);
                   TrafodionUDFunction function = sqlMxSchema.FindFunction(objectName);
                   if (function != null)
                   {
                       TrafodionTriggerUsage theTrafodionTriggerUsage = new TrafodionTriggerUsage(sqlMxSchema, function, UsageFormattedOperation(operation), UsageFormattedIsSubjectTable(isSubjectTable));
                       aTrafodionTrigger._usesRoutinesTrafodionTrigger.Add(theTrafodionTriggerUsage);
                   }
               }else if (udrType.Equals("AC"))
               {
                   TrafodionSchema sqlMxSchema = aTrafodionTrigger.TheTrafodionCatalog.FindSchema(schemaUID);
                   TrafodionFunctionAction functionAction = sqlMxSchema.FindFunctionAction(objectName);
                   if (functionAction != null)
                   {
                       TrafodionTriggerUsage theTrafodionTriggerUsage = new TrafodionTriggerUsage(sqlMxSchema, functionAction, UsageFormattedOperation(operation), UsageFormattedIsSubjectTable(isSubjectTable));
                       aTrafodionTrigger._usesRoutinesTrafodionTrigger.Add(theTrafodionTriggerUsage);
                   }
               }
               else if (udrType.Equals("T"))
               {
                   TrafodionSchema sqlMxSchema = aTrafodionTrigger.TheTrafodionCatalog.FindSchema(schemaUID);
                   TrafodionTableMappingFunction tableMappingFunction = sqlMxSchema.FindTableMappingFunction(objectName);
                   if (tableMappingFunction != null)
                   {
                       TrafodionTriggerUsage theTrafodionTriggerUsage = new TrafodionTriggerUsage(sqlMxSchema, tableMappingFunction, UsageFormattedOperation(operation), UsageFormattedIsSubjectTable(isSubjectTable));
                       aTrafodionTrigger._usesRoutinesTrafodionTrigger.Add(theTrafodionTriggerUsage);
                   }
               }
           }
       }

       /// <summary>
        /// Resets the table model
        /// </summary>
        override public void Refresh()
       {
            //create a temp model
           TrafodionTrigger aTrigger = this.TrafodionTable.LoadTriggerByName(this.InternalName);

           //If temp model is null, the object has been removed
           //So cleanup and notify the UI
           if (aTrigger == null)
           {
               this.TrafodionTable.TrafodionTriggers.Remove(this);
               OnModelRemovedEvent();
               return;
           }
           if (this.CompareTo(aTrigger) != 0)
           {
               //If sql object has been recreated, attach the new sql model to the parent.
               this.TrafodionTable.TrafodionTriggers.Remove(this);
               this.TrafodionTable.TrafodionTriggers.Add(aTrigger);
               this.OnModelReplacedEvent(aTrigger);
           }
           else
           {
               // Clear attributes
               base.Refresh();
               _areAttributesLoaded = false;
               _isEnabled = "";
               _activationTime = "";
               _operation = "";
               _granularity = "";

               //Clear Usage lists
               _usesViewTrafodionTrigger = null;
               _usesTableTrafodionTrigger = null;
               _usesRoutinesTrafodionTrigger = null;
           }
       }
	}
}


