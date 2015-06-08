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
using System.Data.Odbc;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Model
{
	/// <summary>
	/// 
	/// </summary>
	public class TrafodionCatalog : TrafodionObject, IHasTrafodionCatalog
	{

		public TrafodionCatalog(TrafodionSystem aTrafodionSystem, string anInternalName)
            : base(anInternalName, 0)
		{
            theTrafodionSystem = aTrafodionSystem;
		}

		public TrafodionCatalog(TrafodionSystem aTrafodionSystem, string anInternalName, long aUID, string aVolumeName)
			: base(anInternalName, aUID)
		{
            theTrafodionSystem = aTrafodionSystem;
			theVolumeName = aVolumeName;
		}

        /// <summary>
        /// Finds a TrafodionSchema object from the TrafodionSchemas list using the external name
        /// </summary>
        /// <param name="anExternalName">The external name of the schema</param>
        /// <returns></returns>
        public TrafodionSchema FindSchema(string anExternalName)
        {
            TrafodionSchema theTrafodionSchema = TrafodionSchemas.Find(delegate(TrafodionSchema aTrafodionSchema)
            {
                return aTrafodionSchema.ExternalName.Equals(anExternalName);

            });

            if (theTrafodionSchema == null)
            {
                //If schema cannot be resolved, read the database and reload all schemas for catalog 
                LoadTrafodionSchemas();
                theTrafodionSchema = TrafodionSchemas.Find(delegate(TrafodionSchema aTrafodionSchema)
                {
                    return aTrafodionSchema.ExternalName.Equals(anExternalName);
                });
            }

            //If schema still cannot be resolved after the reload, throw an error
            if (theTrafodionSchema == null)
                throw new Exception("Schema " + anExternalName + " not found in catalog " + ExternalName + ".");

            return theTrafodionSchema;
        }

        /// <summary>
        /// Finds a TrafodionSchema object from the TrafodionSchemas list using the schema UID
        /// </summary>
        /// <param name="aSchemaUID">The UID of the schema</param>
        /// <returns></returns>
        public TrafodionSchema FindSchema(long aSchemaUID)
        {
            TrafodionSchema theTrafodionSchema = TrafodionSchemas.Find(delegate(TrafodionSchema aTrafodionSchema)
            {
                return aTrafodionSchema.UID == aSchemaUID;
            });

            if (theTrafodionSchema == null)
            {
                //If schema cannot be resolved, read the database and reload all schemas for catalog 
                LoadTrafodionSchemas();
                theTrafodionSchema = TrafodionSchemas.Find(delegate(TrafodionSchema aTrafodionSchema)
                {
                    return aTrafodionSchema.UID == aSchemaUID;
                });
            }

            //If schema still cannot be resolved after the reload, throw an error
            if (theTrafodionSchema == null)
                throw new Exception("Schema not found in catalog " + ExternalName + ".");

            return theTrafodionSchema;
        }

        override public Connection GetConnection()
        {
            return TrafodionSystem.GetConnection();
        }

        public override bool AllowShowDDL
        {
            get
            {
                return false; //We dont display DDL for catalogs
            }
        }

        public bool LoadTrafodionSchemas()
		{
            Connection theConnection = null;

            try
            {
                theConnection = GetConnection();

                theTrafodionSchemas = new List<TrafodionSchema>();

                OdbcDataReader theReader = Queries.ExecuteSelectSchemaAttributes(theConnection, InternalName);
                while (theReader.Read())
                {
                    string schemaName = theReader.GetString(0).TrimEnd();

                    //If trafodion user and schema is a system schema then ignore it
                    if (TrafodionName.IsAnInternalSchemaName(schemaName))
                    {
                        continue;
                    }
                    else
                    {
                        theTrafodionSchemas.Add(new TrafodionSchema(this,
                            schemaName,
                            theReader.GetInt32(1),
                            theReader.GetString(2).Trim().Replace("\0",""),
                            theReader.GetInt16(3),
                            theReader.GetString(4).Trim(),
                            schemaName.GetHashCode()));
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
            return true;
        }

        public TrafodionSchema LoadTrafodionSchema(string anInternalSchemaName)
        {
            Connection theConnection = null;
            TrafodionSchema sqlMxSchema = null;

            try
            {
                theConnection = GetConnection();

                OdbcDataReader theReader = Queries.ExecuteSelectSchemaAttributes(theConnection, this.InternalName, anInternalSchemaName);
                if (theReader.Read())
                {
                    string schemaName = theReader.GetString(0).TrimEnd();

                    //If internal schema, ignore it.
                    if (TrafodionName.IsAnInternalSchemaName(schemaName))
                    {
                        sqlMxSchema = null;
                    }
                    else
                    {
                        sqlMxSchema = new TrafodionSchema(this,
                            schemaName,
                            theReader.GetInt32(1),
                            theReader.GetString(2).Replace("\0",""),
                            theReader.GetInt16(3),
                            theReader.GetString(4).Trim(),
                            schemaName.GetHashCode());
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
            return sqlMxSchema;
        }


		public string VolumeName
		{
			get
			{
				return theVolumeName;
			}
			set
			{
				theVolumeName = value;
			}
		}

		public List<TrafodionSchema> TrafodionSchemas
		{
			get
			{
				if (theTrafodionSchemas == null)
				{
					if (!LoadTrafodionSchemas())
					{
						return new List<TrafodionSchema>();
					}
				}
				return theTrafodionSchemas;
			}
            set
            {
               theTrafodionSchemas  = null;
            }
		}

        public List<TrafodionSchema> UserTrafodionSchemas
        {
            get
            {
                List<TrafodionSchema> userTrafodionSchemas = new List<TrafodionSchema>();
                foreach (TrafodionSchema sqlMxSchema in TheTrafodionCatalog.TrafodionSchemas)
                {
                    if (TrafodionName.IsAnInternalSchemaName(sqlMxSchema.InternalName))
                    {
                        continue;
                    }
                    else
                    {
                        userTrafodionSchemas.Add(sqlMxSchema);
                    }
                }
                return userTrafodionSchemas;
            }
        }

        override public void Refresh()
        {
            TrafodionCatalog aCatalog = TrafodionSystem.LoadTrafodionCatalog(this.InternalName);

            if (aCatalog == null)
            {
                TrafodionSystem.TrafodionCatalogs.Remove(this);
                OnModelRemovedEvent();
                return;
            }
            if (this.CompareTo(aCatalog) != 0)
            {
                //If catalog object has been recreated, attach the new catalog model to the parent.
                TrafodionSystem.TrafodionCatalogs.Remove(this);
                TrafodionSystem.TrafodionCatalogs.Add(aCatalog);
                OnModelReplacedEvent(aCatalog);
            }
            else
            {
                base.Refresh();
                theTrafodionSchemas = null;
                theTrafodionCatalogRegistrations = null;
            }
        }

        private string theVolumeName = null;

		private List<TrafodionCatalogRegistration> theTrafodionCatalogRegistrations = null;

		private List<TrafodionSchema> theTrafodionSchemas = null;

        private TrafodionSystem theTrafodionSystem;

        public TrafodionSystem TrafodionSystem
        {
            get { return theTrafodionSystem; }
            set { theTrafodionSystem = value; }
        }

        /// <summary>
        /// A catalog's catalog is itself.  The code asking the question of an arbitrary SQL object expects an answer.
        /// </summary>
        public TrafodionCatalog TheTrafodionCatalog
        {
            get { return this; }
        }

        override public ConnectionDefinition ConnectionDefinition
        {
            get { return TrafodionSystem.ConnectionDefinition; }
        }

        /// <summary>
        /// DDL Text for catalog
        /// </summary>
        public override string DDLText
        {
            get
            {
                //Generate the DDL for catalog, since this is not available through ShowDDL
                return "CREATE CATALOG " + ExternalName + " LOCATION " + VolumeName + " ;";
            }
        }

        /// <summary>
        /// check the jar file whether has been uploaded to server to create library.
        /// </summary>
        /// <param name="clientName"></param>
        /// <param name="clientFileName"></param>
        /// <returns></returns>
        public bool ClientJarFileExists(string libraryName, string clientName, string clientFileName)
        {
            foreach (TrafodionSchema sch in this.TrafodionSchemas)
            {
                foreach (TrafodionLibrary lib in sch.TrafodionLibraries)
                {
                    if (String.Compare(lib.ClientFileName, clientFileName, StringComparison.OrdinalIgnoreCase) == 0 &&
                        String.Compare(lib.ClientName, clientName, StringComparison.OrdinalIgnoreCase) == 0)
                    {
                        if (string.Compare(lib.ExternalName, libraryName, StringComparison.OrdinalIgnoreCase) != 0)
                        {
                            return true; //If the jar file is used by another library name, then return true.
                        }
                    }
                }
            }
            return false;
        }
    }
}
