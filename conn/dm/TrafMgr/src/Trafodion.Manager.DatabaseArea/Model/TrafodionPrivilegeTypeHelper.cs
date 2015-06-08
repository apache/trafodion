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
using System.Collections;
using System.Text;

namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// All privilege types defined in Trafodion
    /// </summary>
    public class TrafodionPrivilegeTypeHelper
    {
        #region SQL Privilege Types

        /// <summary>
        /// All privilege type literals defined in Trafodion.
        /// </summary>
        public const string TYPE_ADMINISTRATOR = "AD";
        /// <summary>
        /// Alter All
        /// </summary>
        public const string TYPE_ALTER = "A";
        /// <summary>
        /// Alter Table
        /// </summary>
        public const string TYPE_ALTER_TABLE = "AB";
        /// <summary>
        /// Alter MV
        /// </summary>
        public const string TYPE_ALTER_MV = "AM";
        /// <summary>
        /// Alter MV group
        /// </summary>
        public const string TYPE_ALTER_MV_GROUP = "AG";
        /// <summary>
        /// Alter Synonym
        /// </summary>
        public const string TYPE_ALTER_SYNONYM = "AS";
        /// <summary>
        /// Alter Trigger
        /// </summary>
        public const string TYPE_ALTER_TRIGGER = "AT";
        /// <summary>
        /// Alter View
        /// </summary>
        public const string TYPE_ALTER_VIEW = "AV";
        /// <summary>
        /// Alter Library
        /// </summary>
        public const string TYPE_ALTER_LIBRARY = "AL";
        /// <summary>
        /// Create All
        /// </summary>
        public const string TYPE_CREATE = "C";
        /// <summary>
        /// Create Table
        /// </summary>
        public const string TYPE_CREATE_TABLE = "CB";
        /// <summary>
        /// Create MV
        /// </summary>
        public const string TYPE_CREATE_MV = "CM";
        /// <summary>
        /// Create MV Group
        /// </summary>
        public const string TYPE_CREATE_MV_GROUP = "CG";
        /// <summary>
        /// Create Procedure
        /// </summary>
        public const string TYPE_CREATE_PROCEDURE = "CP";
        /// <summary>
        /// Create Synonym
        /// </summary>
        public const string TYPE_CREATE_SYNONYM = "CS"; 
        /// <summary>
        /// Create Trigger
        /// </summary>
        public const string TYPE_CREATE_TRIGGER = "CT";
        /// <summary>
        /// Create View
        /// </summary>
        public const string TYPE_CREATE_VIEW = "CV";
        /// <summary>
        /// Create Library
        /// </summary>
        public const string TYPE_CREATE_LIBRARY = "CL";
        /// <summary>
        /// Delete 
        /// </summary>
        public const string TYPE_DELETE = "D";
        /// <summary>
        /// Drop All object
        /// </summary>
        public const string TYPE_DROP = "DR";
        /// <summary>
        /// Drop Table
        /// </summary>
        public const string TYPE_DROP_TABLE = "DB";
        /// <summary>
        /// Drop MV
        /// </summary>
        public const string TYPE_DROP_MV = "DM";
        /// <summary>
        /// Drop MV Group
        /// </summary>
        public const string TYPE_DROP_MV_GROUP = "DG";
        /// <summary>
        /// Drop Procedure
        /// </summary>
        public const string TYPE_DROP_PROCEDURE = "DP";
        /// <summary>
        /// Drop Synonym
        /// </summary>
        public const string TYPE_DROP_SYNONYM = "DS";
        /// <summary>
        /// Drop Trigger
        /// </summary>
        public const string TYPE_DROP_TRIGGER = "DT";
        /// <summary>
        /// Drop View
        /// </summary>
        public const string TYPE_DROP_VIEW = "DV";
        /// <summary>
        /// Drop Library
        /// </summary>
        public const string TYPE_DROP_LIBRARY = "DL";
        /// <summary>
        /// Execute
        /// </summary>
        public const string TYPE_EXECUTE = "E";
        /// <summary>
        /// Insert
        /// </summary>
        public const string TYPE_INSERT = "I"; 
        /// <summary>
        /// Maintain
        /// </summary>
        public const string TYPE_MAINTAIN = "M";
        /// <summary>
        /// Reference
        /// </summary>
        public const string TYPE_REFERENCE = "R";
        /// <summary>
        /// Refresh MV
        /// </summary>
        public const string TYPE_REFRESH = "RF";
        /// <summary>
        /// Reorg 
        /// </summary>
        public const string TYPE_REORG = "RO"; 
        /// <summary>
        /// Select
        /// </summary>
        public const string TYPE_SELECT = "S"; 
        /// <summary>
        /// Trigger
        /// </summary>
        public const string TYPE_TRIGGER = "T"; 
        /// <summary>
        /// Update
        /// </summary>
        public const string TYPE_UPDATE = "U";

        /// <summary>
        /// Usage
        /// </summary>
        public const string TYPE_USAGE = "Y";

        /// <summary>
        /// Privilege of REPLICATE
        /// </summary>
        public const string TYPE_REPLICATE = "RP";

        
        /// <summary>
        /// Update Stats
        /// </summary>
        public const string TYPE_UPDATE_STATS = "US";
        public const string TYPE_ALL = "ALL";
        public const string TYPE_ALL_DDL = "ALL_DDL";
        public const string TYPE_ALL_DML = "ALL_DML";
        public const string TYPE_ALL_UTILITY = "ALL_UTILITY";

        public const string TYPE_CREATE_ROUTINE_ACTION = "CA";
        public const string TYPE_DROP_ROUTINE_ACTION = "DA";
        public const string TYPE_ALTER_ROUTINE_ACTION = "AA";
        public const string TYPE_CREATE_ROUTINE = "CR";
        public const string TYPE_DROP_ROUTINE = "DD";
        public const string TYPE_ALTER_ROUTINE = "AR";

        #endregion SQL Privilege Types

        #region Private Variables

        private static readonly Hashtable PrivTypes = new Hashtable();
        private static readonly Hashtable SQLPrivClauses = new Hashtable();
        private static readonly List<string> ObjectPrivTypes = new List<string>();
        private static readonly List<string> SchemaPrivTypes = new List<string>();
        private static readonly List<string> DDLPrivTypes = new List<string>();
        private static readonly List<string> DMLPrivTypes = new List<string>();
        private static readonly List<string> UtilityPrivTypes = new List<string>();
        private static readonly List<string> AlterPrivTypes = new List<string>();
        private static readonly List<string> CreatePrivTypes = new List<string>();
        private static readonly List<string> DropPrivTypes = new List<string>();
        private static readonly List<string> CommonPrivTypes = new List<string>();
        private static readonly List<string> TablePrivTypes = new List<string>();
        private static readonly List<string> MVPrivTypes = new List<string>();
        private static readonly List<string> ViewPrivTypes = new List<string>();
        private static readonly List<string> RoutinePrivTypes = new List<string>();
        private static readonly List<string> LibraryPrivTypes = new List<string>();

        static TrafodionPrivilegeTypeHelper()
        {
            /*
             * All priv types added here
             * PrivTypes stores all the displaying text for each privilege
            */
            PrivTypes.Add(TYPE_ADMINISTRATOR, Properties.Resources.AllPrivileges);
            PrivTypes.Add(TYPE_ALTER, Properties.Resources.Alter);
            PrivTypes.Add(TYPE_ALTER_TABLE, Properties.Resources.AlterTable);
            PrivTypes.Add(TYPE_ALTER_MV, Properties.Resources.AlterMV);
            PrivTypes.Add(TYPE_ALTER_MV_GROUP, Properties.Resources.AlterMVGroup);
            PrivTypes.Add(TYPE_ALTER_SYNONYM, Properties.Resources.AlterSynonym);
            PrivTypes.Add(TYPE_ALTER_TRIGGER, Properties.Resources.AlterTrigger);
            PrivTypes.Add(TYPE_ALTER_VIEW, Properties.Resources.AlterView);
            PrivTypes.Add(TYPE_CREATE, Properties.Resources.Create);
            PrivTypes.Add(TYPE_CREATE_TABLE, Properties.Resources.CreateTable);
            PrivTypes.Add(TYPE_CREATE_MV, Properties.Resources.CreateMV);
            PrivTypes.Add(TYPE_CREATE_MV_GROUP, Properties.Resources.CreateMVGroup);
            PrivTypes.Add(TYPE_CREATE_PROCEDURE, Properties.Resources.CreateProcedure);
            PrivTypes.Add(TYPE_CREATE_SYNONYM, Properties.Resources.CreateSynonym);
            PrivTypes.Add(TYPE_CREATE_TRIGGER, Properties.Resources.CreateTrigger);
            PrivTypes.Add(TYPE_CREATE_VIEW, Properties.Resources.CreateView);
            PrivTypes.Add(TYPE_DELETE, Properties.Resources.Delete);
            PrivTypes.Add(TYPE_DROP, Properties.Resources.Drop);
            PrivTypes.Add(TYPE_DROP_TABLE, Properties.Resources.DropTable);
            PrivTypes.Add(TYPE_DROP_MV, Properties.Resources.DropMV);
            PrivTypes.Add(TYPE_DROP_MV_GROUP, Properties.Resources.DropMVGroup);
            PrivTypes.Add(TYPE_DROP_PROCEDURE, Properties.Resources.DropProcedure);
            PrivTypes.Add(TYPE_DROP_SYNONYM, Properties.Resources.DropSynonym);
            PrivTypes.Add(TYPE_DROP_TRIGGER, Properties.Resources.DropTrigger);
            PrivTypes.Add(TYPE_DROP_VIEW, Properties.Resources.DropView);
            PrivTypes.Add(TYPE_EXECUTE, Properties.Resources.ExecutePrivilege);
            PrivTypes.Add(TYPE_INSERT, Properties.Resources.Insert);
            PrivTypes.Add(TYPE_MAINTAIN, Properties.Resources.Maintain);
            PrivTypes.Add(TYPE_REFERENCE, Properties.Resources.Reference);
            PrivTypes.Add(TYPE_REFRESH, Properties.Resources.Refresh);
            PrivTypes.Add(TYPE_REORG, Properties.Resources.Reorg);
            PrivTypes.Add(TYPE_SELECT, Properties.Resources.Select);
            PrivTypes.Add(TYPE_TRIGGER, Properties.Resources.Trigger);
            PrivTypes.Add(TYPE_UPDATE, Properties.Resources.Update);
            PrivTypes.Add(TYPE_UPDATE_STATS, Properties.Resources.UpdateStats);
            PrivTypes.Add(TYPE_CREATE_ROUTINE_ACTION, Properties.Resources.CreateFunctionAction);
            PrivTypes.Add(TYPE_DROP_ROUTINE_ACTION, Properties.Resources.DropFunctionAction);
            PrivTypes.Add(TYPE_ALTER_ROUTINE_ACTION, Properties.Resources.AlterFunctionAction);
            PrivTypes.Add(TYPE_CREATE_ROUTINE, Properties.Resources.CreateFunction);
            PrivTypes.Add(TYPE_DROP_ROUTINE, Properties.Resources.DropFunction);
            PrivTypes.Add(TYPE_ALTER_ROUTINE, Properties.Resources.AlterFunction);
            PrivTypes.Add(TYPE_USAGE, Properties.Resources.Usage);
            PrivTypes.Add(TYPE_ALL_UTILITY, Properties.Resources.AllUtility);
            PrivTypes.Add(TYPE_REPLICATE, Properties.Resources.Replicate);
            PrivTypes.Add(TYPE_CREATE_LIBRARY, Properties.Resources.CreateLibrary);
            PrivTypes.Add(TYPE_ALTER_LIBRARY, Properties.Resources.AlterLibrary);
            PrivTypes.Add(TYPE_DROP_LIBRARY, Properties.Resources.DropLibrary);

            //Add the list of possible privilege clauses
            SQLPrivClauses.Add(TYPE_ALTER, "ALTER");
            SQLPrivClauses.Add(TYPE_ALTER_TABLE, "ALTER_TABLE");
            SQLPrivClauses.Add(TYPE_ALTER_MV, "ALTER_MV");
            SQLPrivClauses.Add(TYPE_ALTER_MV_GROUP, "ALTER_MV_GROUP");
            SQLPrivClauses.Add(TYPE_ALTER_SYNONYM, "ALTER_SYNONYM");
            SQLPrivClauses.Add(TYPE_ALTER_TRIGGER, "ALTER_TRIGGER");
            SQLPrivClauses.Add(TYPE_ALTER_VIEW, "ALTER_VIEW");
            SQLPrivClauses.Add(TYPE_CREATE, "CREATE");
            SQLPrivClauses.Add(TYPE_CREATE_TABLE, "CREATE_TABLE");
            SQLPrivClauses.Add(TYPE_CREATE_MV, "CREATE_MV");
            SQLPrivClauses.Add(TYPE_CREATE_MV_GROUP, "CREATE_MV_GROUP");
            SQLPrivClauses.Add(TYPE_CREATE_PROCEDURE, "CREATE_PROCEDURE");
            SQLPrivClauses.Add(TYPE_CREATE_SYNONYM, "CREATE_SYNONYM");
            SQLPrivClauses.Add(TYPE_CREATE_TRIGGER, "CREATE_TRIGGER");
            SQLPrivClauses.Add(TYPE_CREATE_VIEW, "CREATE_VIEW");
            SQLPrivClauses.Add(TYPE_DELETE, "DELETE");
            SQLPrivClauses.Add(TYPE_DROP, "DROP");
            SQLPrivClauses.Add(TYPE_DROP_TABLE, "DROP_TABLE");
            SQLPrivClauses.Add(TYPE_DROP_MV, "DROP_MV");
            SQLPrivClauses.Add(TYPE_DROP_MV_GROUP, "DROP_MV_GROUP");
            SQLPrivClauses.Add(TYPE_DROP_PROCEDURE, "DROP_PROCEDURE");
            SQLPrivClauses.Add(TYPE_DROP_SYNONYM, "DROP_SYNONYM");
            SQLPrivClauses.Add(TYPE_DROP_TRIGGER, "DROP_TRIGGER");
            SQLPrivClauses.Add(TYPE_DROP_VIEW, "DROP_VIEW");
            SQLPrivClauses.Add(TYPE_EXECUTE, "EXECUTE");
            SQLPrivClauses.Add(TYPE_INSERT, "INSERT");
            SQLPrivClauses.Add(TYPE_MAINTAIN, "MAINTAIN");
            SQLPrivClauses.Add(TYPE_REFERENCE, "REFERENCES");
            SQLPrivClauses.Add(TYPE_REFRESH, "REFRESH");
            SQLPrivClauses.Add(TYPE_REORG, "REORG");
            SQLPrivClauses.Add(TYPE_SELECT, "SELECT");
            SQLPrivClauses.Add(TYPE_TRIGGER, "TRIGGER");
            SQLPrivClauses.Add(TYPE_UPDATE, "UPDATE");
            SQLPrivClauses.Add(TYPE_UPDATE_STATS, "UPDATE_STATS");
            SQLPrivClauses.Add(TYPE_CREATE_ROUTINE_ACTION, "CREATE_ROUTINE_ACTION");
            SQLPrivClauses.Add(TYPE_DROP_ROUTINE_ACTION, "DROP_ROUTINE_ACTION");
            SQLPrivClauses.Add(TYPE_ALTER_ROUTINE_ACTION, "ALTER_ROUTINE_ACTION");
            SQLPrivClauses.Add(TYPE_CREATE_ROUTINE, "CREATE_ROUTINE");
            SQLPrivClauses.Add(TYPE_DROP_ROUTINE, "DROP_ROUTINE");
            SQLPrivClauses.Add(TYPE_ALTER_ROUTINE, "ALTER_ROUTINE");
            SQLPrivClauses.Add(TYPE_ALL, "ALL");
            SQLPrivClauses.Add(TYPE_ALL_DDL, "ALL_DDL");
            SQLPrivClauses.Add(TYPE_ALL_DML, "ALL_DML");
            SQLPrivClauses.Add(TYPE_ALL_UTILITY, "ALL_UTILITY");
            SQLPrivClauses.Add(TYPE_USAGE, "USAGE");
            SQLPrivClauses.Add(TYPE_REPLICATE, "REPLICATE");
            SQLPrivClauses.Add(TYPE_CREATE_LIBRARY, "CREATE_LIBRARY");
            SQLPrivClauses.Add(TYPE_ALTER_LIBRARY, "ALTER_LIBRARY");
            SQLPrivClauses.Add(TYPE_DROP_LIBRARY, "DROP_LIBRARY");


            // Only Schema priv types added here
            SchemaPrivTypes.Add(TYPE_ADMINISTRATOR);
            SchemaPrivTypes.Add(TYPE_ALTER);
            SchemaPrivTypes.Add(TYPE_ALTER_TABLE);
            SchemaPrivTypes.Add(TYPE_ALTER_MV);
            SchemaPrivTypes.Add(TYPE_ALTER_MV_GROUP);
            SchemaPrivTypes.Add(TYPE_ALTER_SYNONYM);
            SchemaPrivTypes.Add(TYPE_ALTER_TRIGGER);
            SchemaPrivTypes.Add(TYPE_ALTER_VIEW);
            SchemaPrivTypes.Add(TYPE_CREATE);
            SchemaPrivTypes.Add(TYPE_CREATE_TABLE);
            SchemaPrivTypes.Add(TYPE_CREATE_MV);
            SchemaPrivTypes.Add(TYPE_CREATE_MV_GROUP);
            SchemaPrivTypes.Add(TYPE_CREATE_PROCEDURE);
            SchemaPrivTypes.Add(TYPE_CREATE_SYNONYM);
            SchemaPrivTypes.Add(TYPE_CREATE_TRIGGER);
            SchemaPrivTypes.Add(TYPE_CREATE_VIEW);
            SchemaPrivTypes.Add(TYPE_DELETE);
            SchemaPrivTypes.Add(TYPE_DROP);
            SchemaPrivTypes.Add(TYPE_DROP_TABLE);
            SchemaPrivTypes.Add(TYPE_DROP_MV);
            SchemaPrivTypes.Add(TYPE_DROP_MV_GROUP);
            SchemaPrivTypes.Add(TYPE_DROP_PROCEDURE);
            SchemaPrivTypes.Add(TYPE_DROP_SYNONYM);
            SchemaPrivTypes.Add(TYPE_DROP_TRIGGER);
            SchemaPrivTypes.Add(TYPE_DROP_VIEW);
            SchemaPrivTypes.Add(TYPE_EXECUTE);
            SchemaPrivTypes.Add(TYPE_INSERT);
            SchemaPrivTypes.Add(TYPE_MAINTAIN);
            SchemaPrivTypes.Add(TYPE_REFERENCE);
            SchemaPrivTypes.Add(TYPE_REFRESH);
            SchemaPrivTypes.Add(TYPE_REORG);
            SchemaPrivTypes.Add(TYPE_SELECT);
            SchemaPrivTypes.Add(TYPE_UPDATE);
            SchemaPrivTypes.Add(TYPE_UPDATE_STATS);
            SchemaPrivTypes.Add(TYPE_CREATE_ROUTINE_ACTION);
            SchemaPrivTypes.Add(TYPE_CREATE_ROUTINE);
            SchemaPrivTypes.Add(TYPE_DROP_ROUTINE);
            SchemaPrivTypes.Add(TYPE_ALTER_ROUTINE);
            SchemaPrivTypes.Add(TYPE_ALL_UTILITY);
            SchemaPrivTypes.Add(TYPE_REPLICATE);
            SchemaPrivTypes.Add(TYPE_USAGE);
            SchemaPrivTypes.Add(TYPE_CREATE_LIBRARY);
            SchemaPrivTypes.Add(TYPE_ALTER_LIBRARY);
            SchemaPrivTypes.Add(TYPE_DROP_LIBRARY);

            // Only object priv types added here
            ObjectPrivTypes.Add(TYPE_ADMINISTRATOR);
            ObjectPrivTypes.Add(TYPE_ALTER_TABLE);
            ObjectPrivTypes.Add(TYPE_ALTER_MV);
            ObjectPrivTypes.Add(TYPE_ALTER_MV_GROUP);
            ObjectPrivTypes.Add(TYPE_ALTER_SYNONYM);
            ObjectPrivTypes.Add(TYPE_ALTER_VIEW);
            ObjectPrivTypes.Add(TYPE_DELETE);
            ObjectPrivTypes.Add(TYPE_EXECUTE);
            ObjectPrivTypes.Add(TYPE_INSERT);
            ObjectPrivTypes.Add(TYPE_MAINTAIN);
            ObjectPrivTypes.Add(TYPE_REFERENCE);
            ObjectPrivTypes.Add(TYPE_REPLICATE);
            ObjectPrivTypes.Add(TYPE_REFRESH);
            ObjectPrivTypes.Add(TYPE_REORG);
            ObjectPrivTypes.Add(TYPE_SELECT);
            ObjectPrivTypes.Add(TYPE_TRIGGER);
            ObjectPrivTypes.Add(TYPE_UPDATE);
            ObjectPrivTypes.Add(TYPE_UPDATE_STATS);
            ObjectPrivTypes.Add(TYPE_CREATE_ROUTINE_ACTION);
            ObjectPrivTypes.Add(TYPE_DROP_ROUTINE_ACTION);
            ObjectPrivTypes.Add(TYPE_ALTER_ROUTINE_ACTION);
            ObjectPrivTypes.Add(TYPE_USAGE);
            ObjectPrivTypes.Add(TYPE_ALTER_LIBRARY);

            // Create a list for DML types
            DMLPrivTypes.Add(TYPE_DELETE);
            DMLPrivTypes.Add(TYPE_EXECUTE);
            DMLPrivTypes.Add(TYPE_INSERT);
            DMLPrivTypes.Add(TYPE_REFERENCE);
            DMLPrivTypes.Add(TYPE_SELECT);
            DMLPrivTypes.Add(TYPE_UPDATE);
            DMLPrivTypes.Add(TYPE_USAGE);

            // Create a list for DDL types
            DDLPrivTypes.Add(TYPE_ALTER);
            DDLPrivTypes.Add(TYPE_ALTER_TABLE);
            DDLPrivTypes.Add(TYPE_ALTER_MV);
            DDLPrivTypes.Add(TYPE_ALTER_MV_GROUP);
            DDLPrivTypes.Add(TYPE_ALTER_SYNONYM);
            DDLPrivTypes.Add(TYPE_ALTER_TRIGGER);
            DDLPrivTypes.Add(TYPE_ALTER_VIEW);
            DDLPrivTypes.Add(TYPE_CREATE);
            DDLPrivTypes.Add(TYPE_CREATE_TABLE);
            DDLPrivTypes.Add(TYPE_CREATE_MV);
            DDLPrivTypes.Add(TYPE_CREATE_MV_GROUP);
            DDLPrivTypes.Add(TYPE_CREATE_PROCEDURE);
            DDLPrivTypes.Add(TYPE_CREATE_SYNONYM);
            DDLPrivTypes.Add(TYPE_CREATE_TRIGGER);
            DDLPrivTypes.Add(TYPE_CREATE_VIEW);
            DDLPrivTypes.Add(TYPE_DROP);
            DDLPrivTypes.Add(TYPE_DROP_TABLE);
            DDLPrivTypes.Add(TYPE_DROP_MV);
            DDLPrivTypes.Add(TYPE_DROP_MV_GROUP);
            DDLPrivTypes.Add(TYPE_DROP_PROCEDURE);
            DDLPrivTypes.Add(TYPE_DROP_SYNONYM);
            DDLPrivTypes.Add(TYPE_DROP_TRIGGER);
            DDLPrivTypes.Add(TYPE_DROP_VIEW);
            DDLPrivTypes.Add(TYPE_CREATE_LIBRARY);
            DDLPrivTypes.Add(TYPE_ALTER_LIBRARY);
            DDLPrivTypes.Add(TYPE_DROP_LIBRARY);

            DDLPrivTypes.Add(TYPE_MAINTAIN);

            DDLPrivTypes.Add(TYPE_REFRESH);
            DDLPrivTypes.Add(TYPE_REORG);

            DDLPrivTypes.Add(TYPE_UPDATE_STATS);

            DDLPrivTypes.Add(TYPE_CREATE_ROUTINE_ACTION);
            DDLPrivTypes.Add(TYPE_DROP_ROUTINE_ACTION);
            DDLPrivTypes.Add(TYPE_ALTER_ROUTINE_ACTION);
            DDLPrivTypes.Add(TYPE_CREATE_ROUTINE);
            DDLPrivTypes.Add(TYPE_DROP_ROUTINE);
            DDLPrivTypes.Add(TYPE_ALTER_ROUTINE);

            // Create a list for Alter
            AlterPrivTypes.Add(TYPE_ALTER_TABLE);
            AlterPrivTypes.Add(TYPE_ALTER_MV);
            AlterPrivTypes.Add(TYPE_ALTER_MV_GROUP);
            AlterPrivTypes.Add(TYPE_ALTER_SYNONYM);
            AlterPrivTypes.Add(TYPE_ALTER_TRIGGER);
            AlterPrivTypes.Add(TYPE_ALTER_VIEW);
            AlterPrivTypes.Add(TYPE_ALTER_ROUTINE_ACTION);
            AlterPrivTypes.Add(TYPE_ALTER_ROUTINE);
            AlterPrivTypes.Add(TYPE_ALTER_LIBRARY);

            // Create a list for Create
            CreatePrivTypes.Add(TYPE_CREATE_TABLE);
            CreatePrivTypes.Add(TYPE_CREATE_MV);
            CreatePrivTypes.Add(TYPE_CREATE_MV_GROUP);
            CreatePrivTypes.Add(TYPE_CREATE_PROCEDURE);
            CreatePrivTypes.Add(TYPE_CREATE_SYNONYM);
            CreatePrivTypes.Add(TYPE_CREATE_TRIGGER);
            CreatePrivTypes.Add(TYPE_CREATE_VIEW);
            CreatePrivTypes.Add(TYPE_CREATE_ROUTINE_ACTION);
            CreatePrivTypes.Add(TYPE_CREATE_ROUTINE);
            CreatePrivTypes.Add(TYPE_CREATE_LIBRARY);

            // Create a list for Drop
            DropPrivTypes.Add(TYPE_DROP_TABLE);
            DropPrivTypes.Add(TYPE_DROP_MV);
            DropPrivTypes.Add(TYPE_DROP_MV_GROUP);
            DropPrivTypes.Add(TYPE_DROP_PROCEDURE);
            DropPrivTypes.Add(TYPE_DROP_SYNONYM);
            DropPrivTypes.Add(TYPE_DROP_TRIGGER);
            DropPrivTypes.Add(TYPE_DROP_VIEW);
            DropPrivTypes.Add(TYPE_DROP_ROUTINE_ACTION);
            DropPrivTypes.Add(TYPE_DROP_ROUTINE);
            DropPrivTypes.Add(TYPE_DROP_LIBRARY);
            
            // Create a list for Utility 
            UtilityPrivTypes.Add(TYPE_REPLICATE);

            // Create a list for Common object type
            CommonPrivTypes.Add(TYPE_DELETE);
            CommonPrivTypes.Add(TYPE_INSERT);
            CommonPrivTypes.Add(TYPE_REFERENCE);
            CommonPrivTypes.Add(TYPE_SELECT);
            CommonPrivTypes.Add(TYPE_UPDATE);

            // Create a list for Table object type
            TablePrivTypes.Add(TYPE_ALTER);
            TablePrivTypes.Add(TYPE_ALTER_TABLE);
            TablePrivTypes.Add(TYPE_ALTER_SYNONYM);
            TablePrivTypes.Add(TYPE_TRIGGER);
            TablePrivTypes.Add(TYPE_MAINTAIN);
            TablePrivTypes.Add(TYPE_UPDATE_STATS);
            TablePrivTypes.Add(TYPE_REORG);

            // Create a list for MV object type
            MVPrivTypes.Add(TYPE_ALTER);
            MVPrivTypes.Add(TYPE_ALTER_SYNONYM);
            MVPrivTypes.Add(TYPE_ALTER_MV);
            MVPrivTypes.Add(TYPE_ALTER_MV_GROUP);
            MVPrivTypes.Add(TYPE_MAINTAIN);
            MVPrivTypes.Add(TYPE_REFRESH);
            MVPrivTypes.Add(TYPE_REORG);
            MVPrivTypes.Add(TYPE_UPDATE_STATS);

            // Create a list for View object type
            ViewPrivTypes.Add(TYPE_ALTER);
            ViewPrivTypes.Add(TYPE_ALTER_SYNONYM);
            ViewPrivTypes.Add(TYPE_ALTER_VIEW);

            // Create a list for Procedure object type
            RoutinePrivTypes.Add(TYPE_EXECUTE);
            RoutinePrivTypes.Add(TYPE_CREATE_ROUTINE_ACTION);
            RoutinePrivTypes.Add(TYPE_DROP_ROUTINE_ACTION);
            RoutinePrivTypes.Add(TYPE_ALTER_ROUTINE_ACTION);
            RoutinePrivTypes.Add(TYPE_CREATE_ROUTINE);
            RoutinePrivTypes.Add(TYPE_DROP_ROUTINE);
            RoutinePrivTypes.Add(TYPE_ALTER_ROUTINE);

            // Create a list for Library object type
            LibraryPrivTypes.Add(TYPE_UPDATE);
            LibraryPrivTypes.Add(TYPE_USAGE);
            LibraryPrivTypes.Add(TYPE_ALTER_LIBRARY);
        }

        #endregion Private Variables

        #region Public Methods

        /// <summary>
        /// Is this a DML privilege type?
        /// </summary>
        /// <param name="privType"></param>
        /// <returns></returns>
        public static bool IsDMLPrivilege(string privType)
        {
            return DMLPrivTypes.Contains(privType);
        }

        /// <summary>
        /// Is this a DDL privilege type?
        /// </summary>
        /// <param name="privType"></param>
        /// <returns></returns>
        public static bool IsDDLPrivilege(string privType)
        {
            return DDLPrivTypes.Contains(privType);
        }

        /// <summary>
        /// Check if this a Utility privilege type?
        /// </summary>
        /// <param name="privType"></param>
        /// <returns></returns>
        public static bool IsUtilityPrivilege(string privType)
        {
            return UtilityPrivTypes.Contains(privType);
        }

        /// <summary>
        /// Is this the Administrator privilege?
        /// </summary>
        /// <param name="privType"></param>
        /// <returns></returns>
        public static bool IsAdministrator(string privType)
        {
            return (TYPE_ADMINISTRATOR.Equals(privType, StringComparison.OrdinalIgnoreCase));
        }

        /// <summary>
        /// Is this the Alter All Object privilege?
        /// </summary>
        /// <param name="privType"></param>
        /// <returns></returns>
        public static bool IsAlterAllPrivilege(string privType)
        {
            return (TYPE_ALTER.Equals(privType, StringComparison.OrdinalIgnoreCase));
        }

        /// <summary>
        /// Is this the Create All privilege?
        /// </summary>
        /// <param name="privType"></param>
        /// <returns></returns>
        public static bool IsCreateAllPrivilege(string privType)
        {
            return (TYPE_CREATE.Equals(privType, StringComparison.OrdinalIgnoreCase));
        }

        /// <summary>
        /// Is this the Drop All privilege?
        /// </summary>
        /// <param name="privType"></param>
        /// <returns></returns>
        public static bool IsDropAllPrivilege(string privType)
        {
            return (TYPE_DROP.Equals(privType, StringComparison.OrdinalIgnoreCase));
        }

        /// <summary>
        /// Is this an Alter privilege?
        /// </summary>
        /// <param name="privType"></param>
        /// <returns></returns>
        public static bool IsAlterPrivilege(string privType)
        {
            return AlterPrivTypes.Contains(privType);
        }

        /// <summary>
        /// Is this a Create privilege?
        /// </summary>
        /// <param name="privType"></param>
        /// <returns></returns>
        public static bool IsCreatePrivilege(string privType)
        {
            return CreatePrivTypes.Contains(privType);
        }

        /// <summary>
        /// Is this a Drop privilege?
        /// </summary>
        /// <param name="privType"></param>
        /// <returns></returns>
        public static bool IsDropPrivilege(string privType)
        {
            return DropPrivTypes.Contains(privType);
        }

        /// <summary>
        /// Is this an object privilege type?
        /// </summary>
        /// <param name="privType"></param>
        /// <returns></returns>
        public static bool IsObjectPrivilege(string privType)
        {
            return ObjectPrivTypes.Contains(privType);
        }

        /// <summary>
        /// Is this a schema privilege type?
        /// </summary>
        /// <param name="privType"></param>
        /// <returns></returns>
        public static bool IsSchemaPrivilege(string privType)
        {
            return SchemaPrivTypes.Contains(privType);
        }

        /// <summary>
        /// Is this a common privilege type?
        /// </summary>
        /// <param name="privType"></param>
        /// <returns></returns>
        public static bool IsCommonPrivilege(string privType)
        {
            return CommonPrivTypes.Contains(privType);
        }

        /// <summary>
        /// Is this a table privilege?
        /// </summary>
        /// <param name="privType"></param>
        /// <returns></returns>
        public static bool IsTablePrivilege(string privType)
        {
            return IsCommonPrivilege(privType) || TablePrivTypes.Contains(privType);
        }

        /// <summary>
        /// Is this an MV privilege?
        /// </summary>
        /// <param name="privType"></param>
        /// <returns></returns>
        public static bool IsMVPrivilege(string privType)
        {
            return IsCommonPrivilege(privType) || MVPrivTypes.Contains(privType);
        }

        /// <summary>
        /// Is this a view privilege?
        /// </summary>
        /// <param name="privType"></param>
        /// <returns></returns>
        public static bool IsViewPrivilege(string privType)
        {
            return IsCommonPrivilege(privType) || ViewPrivTypes.Contains(privType);
        }

        /// <summary>
        /// Is this a procedure privilege?
        /// </summary>
        /// <param name="privType"></param>
        /// <returns></returns>
        public static bool IsProcedurePrivilege(string privType)
        {
            return RoutinePrivTypes.Contains(privType);
        }

        /// <summary>
        /// Is this a library privilege?
        /// </summary>
        /// <param name="privType"></param>
        /// <returns></returns>
        public static bool IsLibraryPrivilege(string privType)
        {
            return LibraryPrivTypes.Contains(privType);
        }

        /// <summary>
        /// Display string of the privilege
        /// </summary>
        /// <param name="privType"></param>
        /// <returns></returns>
        public static string DisplayPrivilegeType(string privType)
        {
            if (PrivTypes.ContainsKey(privType))
            {
                return (string)PrivTypes[privType];
            }
            else
            {
                return "";
            }
        }

        /// <summary>
        /// Gets the SQL Privilege clause for the matching privilege type
        /// </summary>
        /// <param name="privType"></param>
        /// <returns></returns>
        public static string GetSQLPrivilegeClause(string privType)
        {
            if (SQLPrivClauses.ContainsKey(privType))
            {
                return (string)SQLPrivClauses[privType];
            }
            else
            {
                return "";
            }
        }



        /// <summary>
        /// Build privilege sequence by connecting all kinds of privilege sequence
        /// </summary>
        /// <param name="ddlPrivilege"></param>
        /// <param name="dmlPrivilege"></param>
        /// <param name="utilityPrivilege"></param>
        /// <returns></returns>
        public static string BuildPrivilegeSequence(string ddlPrivilege, string dmlPrivilege, string utilityPrivilege)
        {
            StringBuilder sbPrivilege = new StringBuilder();

            if (!String.IsNullOrEmpty(ddlPrivilege))
            {
                sbPrivilege.AppendFormat("DDL Privileges : ({0})", ddlPrivilege);
            }

            if (!String.IsNullOrEmpty(dmlPrivilege))
            {
                if (sbPrivilege.Length > 0)
                {
                    sbPrivilege.Append(Environment.NewLine);
                }
                sbPrivilege.AppendFormat("DML Privileges : ({0})", dmlPrivilege);
            }

            if (!String.IsNullOrEmpty(utilityPrivilege))
            {
                if (sbPrivilege.Length > 0)
                {
                    sbPrivilege.Append(Environment.NewLine);
                }
                sbPrivilege.AppendFormat("Utility Privileges : ({0})", utilityPrivilege);
            }

            return sbPrivilege.ToString();
        }

        #endregion Public Methods
    }
}
