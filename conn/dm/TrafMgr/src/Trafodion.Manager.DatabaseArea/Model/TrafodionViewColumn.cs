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
using System.Text;

namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// A column that resides in a View. It contains special information that only
    /// pertains to a View column such as primary key information.
    /// </summary>
    public class TrafodionViewColumn : TrafodionColumn 
    {

        
        #region Properties

        /// <summary>
        /// The View to which this column belongs.
        /// </summary>
        public TrafodionView TrafodionView
        {
            get { return TrafodionSchemaObject as TrafodionView; }
            set { TrafodionSchemaObject = value; }
        }

        #endregion

        /// <summary>
        /// Constructs a new View column.
        /// </summary>
        public TrafodionViewColumn()
        {
        }

        public TrafodionObject GetUsedTable(int columnNum)
        {
            return TrafodionView.GetUsedTable(columnNum); 
        }


    }
}
