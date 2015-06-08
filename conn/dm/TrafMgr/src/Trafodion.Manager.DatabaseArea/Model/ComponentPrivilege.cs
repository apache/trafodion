//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
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

namespace Trafodion.Manager.DatabaseArea.Model
{
    public class ComponentPrivilege : Privilege, IComparable<ComponentPrivilege>
    {
        private string _privType;
        private long _componentUID;

        public string PrivType
        {
            get { return _privType; }
        }
        public long ComponentUID
        {
            get { return _componentUID; }
        }

        public ComponentPrivilege(long componentUID, string type, bool grantable, int grantor, string grantorName, string grantorType,
            int grantee, string granteeName, string granteeType)
            : base(grantable, grantor, grantorName, grantorType, grantee, granteeName, granteeType)
        {
            _componentUID = componentUID;
            _privType = type;
        }

        #region IComparable<ComponentPrivilege> Members

        public int CompareTo(ComponentPrivilege other)
        {
            int order = _privType.CompareTo(other.PrivType);
            return (order == 0 && _componentUID == other.ComponentUID) ? CompareTo(other as Privilege) : order;
        }

        #endregion IComparable<ComponentPrivilege> Member
    }
}
