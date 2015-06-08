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

using System.Data.Odbc;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.DatabaseArea.NCC;

namespace Trafodion.Manager.DatabaseArea.Queries
{
    /// <summary>
    /// This class is used to execute a SqlCommand
    /// </summary>
    public class SqlCommandObject
    {
        #region Fields

        private ReportDefinition _theReportDefinition;
        private ConnectionDefinition _theConnectionDefinition;
        private OdbcCommand _odbcCommandObject;
        private string _theStatement;
        private int _theMaxRows;
        private int _rowsPerPage;
        private string _defaultCatalogName;
        private string _defaultSchemaName;
        private ReportDefinition.Operation _operation;
        private NCCQueryPlan _nccQueryPlan = null;

        #endregion Fields

        /// <summary>
        /// Constructor for SqlCommandObject
        /// </summary>
        /// <param name="aReportDefinition"></param>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="aStatement"></param>
        /// <param name="aMaxRows"></param>
        public SqlCommandObject(ReportDefinition aReportDefinition, ConnectionDefinition aConnectionDefinition, ReportDefinition.Operation anOperation, string aStatement, int aMaxRows, int rowsPerPage)
        {
            _theReportDefinition = aReportDefinition;
            _theConnectionDefinition = aConnectionDefinition;
            _operation = anOperation;
            _theStatement = aStatement;
            _theMaxRows = aMaxRows;
            _rowsPerPage = rowsPerPage;
        }

        #region Properties

        /// <summary>
        /// The odbc command associated with this sql command
        /// </summary>
        public OdbcCommand OdbcCommandObject
        {
            get { return _odbcCommandObject; }
            set { _odbcCommandObject = value; }
        }

        /// <summary>
        /// Get the Report Definition
        /// </summary>
        public ReportDefinition TheReportDefinition
        {
            get { return _theReportDefinition; }
        }

        /// <summary>
        /// Get the Connection Definition
        /// </summary>
        public ConnectionDefinition TheConnectionDefinition
        {
            get { return _theConnectionDefinition; }
        }

        /// <summary>
        /// Get the Operation
        /// </summary>
        public ReportDefinition.Operation TheOperation
        {
            get { return _operation; }
        }

        /// <summary>
        /// Get the Statement
        /// </summary>
        public string TheStatement
        {
            get { return _theStatement; }
        }

        /// <summary>
        /// Get the Max Row number
        /// </summary>
        public int TheMaxRows
        {
            get { return _theMaxRows; }
        }

        /// <summary>
        /// Rows per page of query results
        /// </summary>
        public int RowsPerPage
        {
            get { return _rowsPerPage; }
        }

        /// <summary>
        /// Catalog Name
        /// </summary>
        public string DefaultCatalogName
        {
            get { return _defaultCatalogName; }
            set { _defaultCatalogName = value; }
        }
        /// <summary>
        /// Schema Name
        /// </summary>
        public string DefaultSchemaName
        {
            get { return _defaultSchemaName; }
            set { _defaultSchemaName = value; }
        }

        /// <summary>
        /// Property: NCCQueryPlan - the model object responsible for accessing query plan from backend. 
        /// </summary>
        public NCCQueryPlan NCCQueryPlan
        {
            get { return _nccQueryPlan; }
            set { _nccQueryPlan = value; }
        }

        #endregion Properties
    }
}
