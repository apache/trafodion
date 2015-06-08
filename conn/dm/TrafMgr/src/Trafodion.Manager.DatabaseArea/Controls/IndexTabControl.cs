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

using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// Control that contains tabs for an index
    /// </summary>
    public class IndexTabControl : TrafodionTabControl
    {
        /// <summary>
        /// Constructor for IndexTabControl
        /// </summary>
        /// <param name="aDatabaseObjectsControl"></param>
        /// <param name="aTrafodionIndex"></param>
        public IndexTabControl(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionIndex aTrafodionIndex)
        {
            TrafodionIndex = aTrafodionIndex;
        }

        /// <summary>
        /// Get and Set the Index
        /// </summary>
        public TrafodionIndex TrafodionIndex
        {
            get 
            { 
                return _sqlMxIndex; 
            }
            set 
            { 
                _sqlMxIndex = value;
                TabPages.Clear();
                TabPages.Add(new IndexColumnsTabPage(TrafodionIndex));
                //TabPages.Add(new DivisionByKeyTabPage(TrafodionIndex));
                TabPages.Add(new IndexAttributesTabPage(TrafodionIndex));
                TabPages.Add(new TrafodionObjectDDLTabPage(TrafodionIndex));
                //TabPages.Add(new PartitionsTabPage(TrafodionIndex));


            }
        }

        private TrafodionIndex _sqlMxIndex;


    }
}
