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

using Trafodion.Manager.DatabaseArea.Controls.Tree;
using Trafodion.Manager.DatabaseArea.Model;

namespace Trafodion.Manager.DatabaseArea.Controls
{

    /// <summary>
    /// A hyperlink to a SQL object and the name is displayed in ansi format
    /// This class overrides its base class to provide an ansi name for the linked object
    /// </summary>
    public class DatabaseAreaObjectsVisibleAnsiLink : DatabaseAreaObjectsDataGridViewLink
    {
        /// <summary>
        /// Default constructor
        /// </summary>
        public DatabaseAreaObjectsVisibleAnsiLink()
        {
        }

        /// <summary>
        /// Constructs a DatabaseAreaObjectsVisibleAnsiLink
        /// </summary>
        /// <param name="aDatabaseTreeView">A reference to Database Navigation tree</param>
        /// <param name="aTrafodionObject"> A reference the TrafodionObject to which the Link will point to</param>
        public DatabaseAreaObjectsVisibleAnsiLink(DatabaseTreeView aDatabaseTreeView, TrafodionObject aTrafodionObject)
            : base(aDatabaseTreeView, aTrafodionObject)
        {
        }

        /// <summary>
        /// If the logged on user is a Trafodion user, then link name is the 2 part 
        /// ansi name of the Sql object. If the logged on user is a Super group 
        /// user, then link name is the 3 ansi part name of the Sql object
        /// </summary>
        /// <returns></returns>
        override public string ToString()
        {
            if (TheTrafodionObject == null)
            {
                return base.ToString();
            }

            return TheTrafodionObject.VisibleAnsiName;
        }

    }

}
