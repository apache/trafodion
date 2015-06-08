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
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Trafodion.Manager.DatabaseArea.Model
{
    public class ComponentPrivilegeUsage
    {
        private long _componentUID;
        private string _privType;
        private string _privName;
        private string _privDescription;

        public long ComponentUID
        {
            get { return _componentUID; }
        }

        public string PrivType
        {
            get { return _privType; }
        }

        public string PrivName
        {
            get { return _privName; }
        }

        public string PrivDescription
        {
            get { return _privDescription; }
        }

        public ComponentPrivilegeUsage(long UID, string type, string name, string description)
        {
            _componentUID = UID;
            _privType = type;
            _privName = name;
            _privDescription = description;
        }
    }
}
