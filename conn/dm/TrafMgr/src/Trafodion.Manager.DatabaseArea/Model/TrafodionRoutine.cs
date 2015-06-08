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
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// Model for a sql routine
    /// </summary>
    abstract public class TrafodionRoutine
        : TrafodionSchemaObject
    {
        #region Public variables

        /// <summary>
        /// Literal identifier to denote the procedure modifies sql data
        /// </summary>
        public static readonly string MODIFIES_SQL_DATA_KEY = "M";
        /// <summary>
        /// Literal identifier to denote the procedure modifies does not access sql
        /// </summary>
        public static readonly string NO_SQL_KEY = "";

        #endregion Public variables

        #region Private Member Variables

        private bool _areAttributesLoaded = false;

        /// Procedure Atributes
        #region Procedure fields
        private string _UDRType = "";
        private string _languageType = "";
        private string _deterministicBool = "";
        private string _sqlAccess = "";
        private string _isolateBool = "";
        private string _parameterStyle = "";
        private string _transactionAttributes = "";
        private int _maxResults;
        private string _UDRAttributes = "";
        private string _externalPath = "";
        private string _libraryName = null;
        private Int64 _libraryUID = 0;
        private string _externalClassFileName = "";
        private string _externalMethodName = "";
        protected string _signatureText = "";
        protected string _formattedSignatureText = null;
        protected bool _isUniversal = false;

        //newly added columns in version 2500
        private string _callOnNull = "";
        private int _paramStyleVersion;
        private int _initialCPUCost;
        private int _initialIOCost;
        private int _initialMSGCost;
        private int _normalCPUCost;
        private int _normalIOCost;
        private int _normalMSGCost;
        //private string _isUniversal;
        private string _parallelism = "";
        private string _canCollapse = "";
        private string _userVersion = "";
        private string _externalSecurity = "";
        private int _actionPosition;
        private string _executionMode = "";
        #endregion
        /// The procedure parameters are represented as columns by SQL
        private DefaultHasTrafodionColumns<TrafodionProcedureColumn> _theColumns = null;
        protected string[] _params;
        /// <summary>
        /// Literal string to denote the procedure modifies sql data
        /// </summary>
        private static readonly string MODIFIES_SQL_DATA = "MODIFIES SQL DATA";
        /// <summary>
        /// Literal string to denote the procedure modifies does not access sql
        /// </summary>
        private static readonly string NO_SQL = "NO SQL";

        private List<TrafodionSchemaObject> _sqlMxSchemaObjects = null;


        #endregion Private Member Variables

        #region Public Properties

        /// <summary>
        /// Object type for the procedure
        /// </summary>
        override public string SchemaObjectType
        {
            get
            {
                return "UR";
            }
        }

        /// <summary>
        /// Object name space for the procedure
        /// </summary>
        override public string SchemaObjectNameSpace
        {
            get
            {
                return "TA";
            }
        }

        /// <summary>
        /// Displayable Object type for the procedure
        /// </summary>
        override public string DisplayObjectType
        {
            get
            {
                return Properties.Resources.Procedure;
            }
        }

        /// <summary>
        /// The procedure parameters are represented as columns by SQL
        /// </summary>
        public List<TrafodionColumn> Columns
        {
            get
            {
                return _theColumns.Columns;
            }
        }
        /// <summary>
        /// The max number of dynamic result sets
        /// </summary>
        public int MaxResults
        {
            get { return _maxResults; }
            set { _maxResults = value; }
        }
        /// <summary>
        /// SQL access identifier string
        /// </summary>
        public string SQLAccess
        {
            get { return _sqlAccess; }
            set { _sqlAccess = value; }
        }
        /// <summary>
        /// External path of the java code file
        /// </summary>
        public string ExternalPath
        {
            get { return _externalPath; }
            set { _externalPath = value; }
        }

        /// <summary>
        /// External path of the java code file
        /// </summary>
        public string LibraryName
        {
            get { return _libraryName; }
            set { _libraryName = value; }
        }

        public Int64 LibraryUID
        {
            get { return _libraryUID; }
        }
        /// <summary>
        /// The java class used by the procedure
        /// </summary>
        public string ExternalClassFileName
        {
            get { return _externalClassFileName; }
            set { _externalClassFileName = value; }
        }

        /// <summary>
        /// The java method name used by the procedure
        /// </summary>
        public string ExternalMethodName
        {
            get { return _externalMethodName; }
            set { _externalMethodName = value; }
        }

        /// <summary>
        /// The java signature of the method
        /// </summary>
        public string SignatureText
        {
            get { return _signatureText; }
            set { _signatureText = value; }
        }

        public bool IsUniversal
        {
            get { return _isUniversal; }
            set { _isUniversal = value; }
        }

        public string UDRType
        {
            get
            {
                return _UDRType;
            }
        }

        public string CallOnNull
        {
            get
            {
                return _callOnNull;
            }
        }
        public int ParamStyleVersion
        {
            get
            {
                return _paramStyleVersion;
            }
        }
        public int InitialCPUCost
        {
            get
            {
                return _initialCPUCost;
            }
        }
        public int InitialIOCost
        {
            get
            {
                return _initialIOCost;
            }
        }
        public int InitialMSGCost
        {
            get
            {
                return _initialMSGCost;
            }
        }

        public int NormalCPUCost
        {
            get
            {
                return _normalCPUCost;
            }
        }
        public int NormalIOCost
        {
            get
            {
                return _normalIOCost;
            }
        }
        public int NormalMSGCost
        {
            get
            {
                return _normalMSGCost;
            }
        }
        public string Parallelism
        {
            get
            {
                return _parallelism;
            }
        }
        public string CanCollapse
        {
            get
            {
                return _canCollapse;
            }
        }
        public string UserVersion
        {
            get
            {
                return _userVersion;
            }
        }
        public string ExternalSecurity
        {
            get
            {
                return _externalSecurity;
            }
            set
            {
                _externalSecurity = value;
            }
        }
        public string NewTransactionAttributes
        {
            get
            {
                return _transactionAttributes;
            }
            set
            {
                _transactionAttributes = value;
            }
        }
        public int ActionPosition
        {
            get
            {
                return _actionPosition;
            }
        }
        public string ExecutionMode
        {
            get
            {
                return _executionMode;
            }
        }

           
        #endregion Public Properties

        #region Formatted Public Properties

        /// <summary>
        /// Format Dynamic Result Set
        /// </summary>
        public string FormattedDynamicResultSets
        {
            get
            {
                LoadAttributes();

                return _maxResults.ToString();
            }
        }

        /// <summary>
        /// Format SQL access type
        /// </summary>
        public string FormattedSQLAccess
        {
            get
            {
                if (_sqlAccess == null)
                {
                    LoadAttributes();
                }

                string aSQLAccess;

                if (_sqlAccess.Equals(TrafodionProcedure.MODIFIES_SQL_DATA_KEY))
                {
                    aSQLAccess = TrafodionRoutine.MODIFIES_SQL_DATA;
                }
                else if (_sqlAccess.Equals("R"))
                {
                    aSQLAccess = Properties.Resources.ReadsSQL;
                }
                else if (_sqlAccess.Equals("C"))
                {
                    aSQLAccess = Properties.Resources.ContainsSQL;
                }
                else
                    aSQLAccess = Properties.Resources.NoSQL;

                return aSQLAccess;
            }
        }

        /// <summary>
        /// Format External path
        /// </summary>
        public string FormattedLibraryName
        {
            get
            {
                if (_libraryName == null)
                {
                    LoadAttributes();
                }
                return _libraryName;
            }
        }

        /// <summary>
        /// Format External path
        /// </summary>
        public string FormattedExternalPath
        {
            get
            {
                if (_externalPath == null)
                {
                    LoadAttributes();
                }
                return _externalPath;
            }
        }

        /// <summary>
        /// Format External File
        /// </summary>
        public string FormattedExternaClassFileName
        {
            get
            {
                if (_externalClassFileName == null)
                {
                    LoadAttributes();
                }

                return _externalClassFileName;
            }
        }

        /// <summary>
        /// Format External Name
        /// </summary>
        public string FormattedMethodName
        {
            get
            {
                if (_externalMethodName == null)
                {
                    LoadAttributes();
                }
                return _externalMethodName;
            }
        }

        /// <summary>
        /// Format Language Type, only java right now
        /// </summary>
        public string FormattedLanguageType
        {
            get
            {
                LoadAttributes();

                string aLanguageType;
                if ("J".Equals(_languageType))
                {
                    aLanguageType = Properties.Resources.Java;
                }
                else if ("C".Equals(_languageType))
                {
                    aLanguageType = _languageType;
                }else
                {
                    aLanguageType = _languageType;
                }
                return aLanguageType;
            }
        }

        /// <summary>
        /// Format parameter style, only java right now
        /// </summary>
        public string FormattedParameterStyle
        {
            get
            {
                LoadAttributes();

                string aParamStyle;
                if (_parameterStyle.Equals("J"))
                {
                    aParamStyle = Properties.Resources.Java;
                }
                else if (_parameterStyle.Equals("G"))
                {
                    aParamStyle = Properties.Resources.GeneralStyle;
                }
                else if (_parameterStyle.Equals("S"))
                {
                    aParamStyle = Properties.Resources.SQL;
                }
                else if (_parameterStyle.Equals("SR"))
                {
                    aParamStyle = Properties.Resources.SQLRow;
                }
                else
                    aParamStyle = _parameterStyle;


                return aParamStyle;
            }
        }

        /// <summary>
        /// Format parameter Deterministic, yes/no
        /// </summary>
        public string FormattedDeterministic
        {
            get
            {
                LoadAttributes();

                return Utilities.YesNo(_deterministicBool);
            }
        }

        /// <summary>
        /// Format parameter Isolate, yes/no
        /// </summary>
        public string FormattedIsolate
        {
            get
            {
                LoadAttributes();

                return Utilities.YesNo(_isolateBool);
            }
        }

        /// <summary>
        /// Format Transaction Attribute
        /// </summary>
        public string TransactionAttributes
        {
            get
            {
                LoadAttributes();

                string aTransactionAttributes;
                if (_transactionAttributes.Equals("RQ"))
                {
                    aTransactionAttributes = Properties.Resources.TransactionRequired;
                }
                else
                    aTransactionAttributes = Properties.Resources.NoTransaction;

                return aTransactionAttributes;
            }
        }

        /// <summary>
        /// Format External Security Attribute
        /// </summary>
        public string FormattedExternalSecurity
        {
            get
            {
                LoadAttributes();

                string aExternalSecurity = "";
                if (_externalSecurity.Equals("D"))
                {
                    aExternalSecurity = Properties.Resources.ExternalSecurityDefiner;
                }
                else if (_externalSecurity.Equals("I"))
                {
                    aExternalSecurity = Properties.Resources.ExternalSecurityInvoker;
                }

                return aExternalSecurity;
            }
        }

        /// <summary>
        /// fortmat udr type
        /// </summary>
        public string FormattedUDRType
        {
            get
            {
                if ("P".Equals(UDRType))
                {
                    return Properties.Resources.Procedure;
                }else if ("F".Equals(UDRType))
                {
                    return Properties.Resources.ScalarFunction;
                }
                else if ("U".Equals(UDRType))
                {
                    return Properties.Resources.UniversalFunction;
                }
                else if ("AC".Equals(UDRType))
                {
                    return Properties.Resources.FunctionAction;
                }
                else if ("T".Equals(UDRType))
                {
                    return Properties.Resources.TableMappingFunction;
                }
                else
                {
                    return UDRType;
                }
            }
        }

        #endregion Formatted Public Properties

        /// <summary>
        /// Constructs a model for the procedure
        /// </summary>
        /// <param name="aTrafodionSchema"></param>
        /// <param name="anInternalName"></param>
        /// <param name="aUID"></param>
        /// <param name="aCreateTime"></param>
        /// <param name="aRedefTime"></param>
        /// <param name="aSecurityClass"></param>
        /// <param name="anOwner">The owner of the object.</param>
        public TrafodionRoutine(TrafodionSchema aTrafodionSchema, string anInternalName, long aUID, long aCreateTime, long aRedefTime, string aSecurityClass, string anOwner)
            : base(aTrafodionSchema, anInternalName, aUID, aCreateTime, aRedefTime, aSecurityClass, anOwner)
        {
            _theColumns = new DefaultHasTrafodionColumns<TrafodionProcedureColumn>(this);
        }

        /// <summary>
        /// Load the attributes of the index
        /// </summary>
        /// <returns></returns>
        public bool LoadAttributes()
        {
            if (_areAttributesLoaded)
            {
                return true;
            }

            Connection theConnection = null;
            OdbcDataReader theReader = null;
            try
            {
                theConnection = GetConnection();
                theReader = Queries.ExecuteSelectRoutineAttributes(theConnection, this);

                int rowNumber = 1;
                while(theReader.Read())
                {
                    if (rowNumber == 1)
                    {
                        _UDRType = theReader.GetString(0).Trim();
                        _languageType = theReader.GetString(1).Trim();
                        _deterministicBool = theReader.GetString(2).Trim();
                        _sqlAccess = theReader.GetString(3).Trim();
                        _isolateBool = theReader.GetString(4).Trim();
                        _parameterStyle = theReader.GetString(5).Trim();
                        _transactionAttributes = theReader.GetString(6).Trim();
                        _maxResults = theReader.GetInt16(7);
                        _UDRAttributes = theReader.GetString(8).Trim();
                        _externalPath = theReader.GetString(9).Trim();
                        _externalClassFileName = theReader.GetString(10).Trim();
                        _externalMethodName = theReader.GetString(11).Trim();
                        _signatureText = theReader.GetString(12).Trim();

                        if (TheTrafodionSchema.Version >= 2500)
                        {
                            //newly added columns
                            _callOnNull = theReader.GetString(13).Trim();
                            _paramStyleVersion = theReader.GetInt32(14);
                            _initialCPUCost = theReader.GetInt32(15);
                            _initialIOCost = theReader.GetInt32(16);
                            _initialMSGCost = theReader.GetInt32(17);
                            _normalCPUCost = theReader.GetInt32(18);
                            _normalIOCost = theReader.GetInt32(19);
                            _normalMSGCost = theReader.GetInt32(20);
                            _isUniversal = theReader.GetString(21).Trim().Equals("Y");
                            _parallelism = theReader.GetString(22).Trim();
                            _canCollapse = theReader.GetString(23).Trim();
                            _userVersion = theReader.GetString(24).Trim();
                            _externalSecurity = theReader.GetString(25).Trim();
                            _actionPosition = theReader.GetInt32(26); ;
                            _executionMode = theReader.GetString(27).Trim();
                        }
                        _formattedSignatureText = null;
                        if (ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ120)
                        {
                            //M7
                            _libraryUID = theReader.GetInt64(28);
                            if (_libraryUID > 0)
                            {
                                TrafodionLibrary libraryModel = this.TheTrafodionSchema.FindLibrary(_libraryUID);
                                _libraryName = string.Empty;
                                if (libraryModel != null)
                                {
                                    _libraryName = libraryModel.RealAnsiName;
                                }
                            }
                        }

                    }
                    else
                    {
                        _formattedSignatureText += theReader.GetString(12).Trim();
                    }
                }
            }
            finally
            {
                if (theReader != null)
                {
                    theReader.Close();
                }
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }
            _areAttributesLoaded = true;
            return true;
        }

        /// <summary>
        /// Resets the Procedure model
        /// </summary>
        override public void Refresh()
        {
            base.Refresh();
            _areAttributesLoaded = false;
            _formattedSignatureText = null;
            _UDRType = null;
            _languageType = null;
            _deterministicBool = null;
            _sqlAccess = null;
            _isolateBool = null;
            _parameterStyle = null;
            _transactionAttributes = null;
            _maxResults = 0;
            _UDRAttributes = null;
            _externalPath = null;
            _externalClassFileName = null;
            _externalMethodName = null;
            _signatureText = null;
            _externalSecurity = null;
            _libraryUID = 0;
            _libraryName = null;
        }

        public List<TrafodionSchemaObject> LoadTrafodionSchemaObjects(TrafodionRoutine aTrafodionRoutine)
        {
            if (_sqlMxSchemaObjects == null)
            {
                _sqlMxSchemaObjects = new TrafodionSchemaObjectsLoader().Load(this);
            }
            return _sqlMxSchemaObjects;
        }

        internal class TrafodionSchemaObjectsLoader : TrafodionObjectsLoader<TrafodionRoutine, TrafodionSchemaObject>
        {
            override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionRoutine aTrafodionRoutine)
            {
                return Queries.ExecuteSelectTrafodionSchemaObjectsUsedByRoutine(aConnection, aTrafodionRoutine);
            }

            override protected void LoadOne(List<TrafodionSchemaObject> aList, TrafodionRoutine aTrafodionRoutine, OdbcDataReader aReader)
            {
                long catalogUID = aReader.GetInt64(0);
                long schemaUID = aReader.GetInt64(1);
                long objectUID = aReader.GetInt64(2);
                string objectType = aReader.GetString(3).Trim();

                TrafodionCatalog catalog = aTrafodionRoutine.TheTrafodionCatalog.TrafodionSystem.FindCatalog(catalogUID);
                if (catalog == null)
                {
                    return;
                }
                TrafodionSchema schema = catalog.FindSchema(schemaUID);
                if (schema == null)
                {
                    return;
                }

                if ("VI".Equals(objectType))
                {
                    foreach (TrafodionView view in schema.TrafodionViews)
                    {
                        if (objectUID == view.UID)
                        {
                            aList.Add(view);
                        }
                    }
                }
                else if ("MV".Equals(objectType))
                {
                    foreach (TrafodionMaterializedView mv in schema.TrafodionMaterializedViews)
                    {
                        if (objectUID == mv.UID)
                        {
                            aList.Add(mv);
                        }
                    }
                }
                else if ("TR".Equals(objectType))
                {
                    foreach (TrafodionTable table in schema.TrafodionTables)
                    {
                        foreach (TrafodionTrigger trigger in table.TrafodionTriggers)
                        {
                            if (objectUID == trigger.UID)
                            {
                                aList.Add(trigger);
                            }
                        }
                    }
                }
                else if ("LB".Equals(objectType))
                {
                    foreach (TrafodionLibrary lib in schema.TrafodionLibraries)
                    {
                        if (objectUID == lib.UID)
                        {
                            aList.Add(lib);
                        }
                    }
                }  
            }
        }
    }
}
