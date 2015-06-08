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
using System.Data.Odbc;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Model
{
	/// <summary>
	/// Model object for Synonym
	/// </summary>
	public class TrafodionSynonym
		: TrafodionSchemaObject
	{
        /// <summary>
        /// Synonym's name space
        /// </summary>
        public const string ObjectNameSpace = "TA";

        /// <summary>
        /// Synonym's object type
        /// </summary>
        public const string ObjectType = "SY";

        private TrafodionSchemaObject _referencedObject = null;
        private string[] ddlPatterns = { "CREATE SYNONYM" };

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aTrafodionSchema">Schema</param>
        /// <param name="anInternalName">Name of Synonym</param>
        /// <param name="aUID">UID of synonym</param>
        /// <param name="aCreateTime">Creation Time</param>
        /// <param name="aRedefTime">Redefinition time</param>
        /// <param name="aSecurityClass">Security class</param>
        /// <param name="anOwner">The owner of the object.</param>
        public TrafodionSynonym(TrafodionSchema aTrafodionSchema, string anInternalName, long aUID, long aCreateTime, long aRedefTime, string aSecurityClass, string anOwner)
			: base(aTrafodionSchema, anInternalName, aUID, aCreateTime, aRedefTime, aSecurityClass, anOwner)
		{
		}

        /// <summary>
        /// Object type
        /// </summary>
		override public string SchemaObjectType
		{ 
			get
			{
                return ObjectType;
			}
		}

        /// <summary>
        /// Displayable Object type for the synonym
        /// </summary>
        override public string DisplayObjectType
        {
            get
            {
                return Properties.Resources.Synonym;
            }
        }

        /// <summary>
        /// Object name space
        /// </summary>
		override public string SchemaObjectNameSpace
		{ 
			get
			{
                return ObjectNameSpace;
			}
		}

        /// <summary>
        /// Resets the synonym model
        /// </summary>
        override public void Refresh()
        {
            //create a temp model
            TrafodionSynonym aSynonym = this.TheTrafodionSchema.LoadSynonymByName(this.InternalName);

            //If temp model is null, the object has been removed
            //So cleanup and notify the UI
            if (aSynonym == null)
            {
                this.TheTrafodionSchema.TrafodionSynonyms.Remove(this);
                OnModelRemovedEvent();
                return;
            }
            if (this.CompareTo(aSynonym) != 0)
            {
                //If sql object has been recreated, attach the new sql model to the parent.
                this.TheTrafodionSchema.TrafodionSynonyms.Remove(this);
                this.TheTrafodionSchema.TrafodionSynonyms.Add(aSynonym);
                this.OnModelReplacedEvent(aSynonym);
            }
            else
            {
                base.Refresh();
                _referencedObject = null;
            }
        }

        /// <summary>
        /// The object for which the synonym is defined
        /// </summary>
        public TrafodionSchemaObject ReferencedObject
        {
            get
            {
                if (_referencedObject == null)
                {
                  LoadReferencedObject();
                }
                return _referencedObject;
            }
            set { _referencedObject = value; }
        }

        /// <summary>
        /// Load the referenced object 
        /// </summary>
        private void LoadReferencedObject()
        {
            Connection theConnection = null;

            try
            {
                theConnection = GetConnection();

                OdbcDataReader odbcReader = Queries.ExecuteGetSynonymReferenceObject(theConnection, this);
                if (odbcReader.Read())
                {
                    //Synonym could reference an object from a difference schema. So get the schema of the reference object
                    Int64 schemaUID = odbcReader.GetInt64(0);

                    //Synonym could reference a table or mv or view. So get the referenced object's name and type.
                    string objectName = odbcReader.GetString(1).TrimEnd();
                    string objectType = odbcReader.GetString(2).Trim();

                    //Find the model schema of the referenced object
                    TrafodionSchema sqlMxSchema = this.TheTrafodionCatalog.FindSchema(schemaUID);

                    //Based on the object type, call the appropriate method on the model schema to find the 
                    //referenced object
                    switch (objectType)
                    {
                        case TrafodionTable.ObjectType:
                            {
                                _referencedObject = sqlMxSchema.FindTable(objectName);
                                break;
                            }
                        case TrafodionMaterializedView.ObjectType:
                            {
                                _referencedObject = sqlMxSchema.FindMaterializedView(objectName);
                                break;
                            }
                        case TrafodionView.ObjectType:
                            {
                                try
                                {
                                    _referencedObject = sqlMxSchema.FindView(objectName);
                                }
                                catch (Exception ex)
                                {
                                    //If view is not found, it could be that the view is a system view
                                    //and show system views is disabled. So the view model is not in cache
                                    _referencedObject = sqlMxSchema.LoadViewByName(objectName);
                                }
                                break;
                            }
                        default:
                            {
                                _referencedObject = null;
                                break;
                            }
                          
                    }
                }
            }
            finally
            {
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }
        }

        /// <summary>
        /// DDL text for the Synonym
        /// </summary>
        public override string DDLText
        {
            get
            {
                if (_ddlText == null || _ddlText.Length == 0)
                {
                    //There is bug in showddl where the showddl on mv does not return its synonyms
                    //So the implementation that invokes showddl is commented out and instead we explicitly 
                    //construct the DDL. When ShowDDL fixes this bug, we will have uncomment this code.
                    /*if (ReferencedObject != null)
                    {
                        //DDL is extracted from the referenced object's DDL.
                        _ddlText = TrafodionObjectDDLLoader.FindChildDDL(ReferencedObject.DDLText, VisibleAnsiName,
                            ddlPatterns);
                    }*/

                    //Temporary workaround for showddl bug.
                    _ddlText = String.Format("CREATE SYNONYM {0} FOR {1};", VisibleAnsiName, ReferencedObject.VisibleAnsiName);
                }
                return _ddlText;
            }
        }
    }

}


