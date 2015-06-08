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

namespace Trafodion.Manager.Framework
{
    /// <summary>
    /// To record a statement
    /// </summary>
    public class StatementDefinition
    {
        #region Fields

        private string _name = null;
        private string _queryID = null;
        private string _queryText = null;
        private DateTime _startTime = DateTime.Now;

        #endregion 

        #region Properties

        /// <summary>
        /// Property: Name
        /// </summary>
        public string Name
        {
            get { return _name; }
            set { _name = value; }
        }

        /// <summary>
        /// Property: Query ID
        /// </summary>
        public string QueryID
        {
            get { return _queryID; }
            set { _queryID = value; }
        }

        /// <summary>
        /// Property: Query Text
        /// </summary>
        public string QueryText
        {
            get { return _queryText; }
            set { _queryText = value; }
        }
        
        /// <summary>
        /// Property: Start Time
        /// </summary>
        public DateTime StartTime
        {
            get { return _startTime; }
            set { _startTime = value; }
        }

        #endregion Properties

        #region Constructor

        public StatementDefinition(string aName, string aQueryID, string aQueryText)
        {
            Name = aName;
            QueryID = aQueryID;
            QueryText = aQueryText;
        }

        public StatementDefinition(string aQueryID, string aQueryText)
        {
            Name = QueryID = aQueryID;
            QueryText = aQueryText;
        }

        public StatementDefinition(string aQueryText)
        {
            QueryText = aQueryText;
        }

        public StatementDefinition()
        {
        }

        #endregion Constructor

        #region Public methods

        #endregion Public methods

        #region Private methods

        #endregion Private methods
    }
}
