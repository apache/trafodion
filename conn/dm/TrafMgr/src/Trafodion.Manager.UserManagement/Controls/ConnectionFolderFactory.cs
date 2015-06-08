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
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.UserManagement.Controls.Tree;

namespace Trafodion.Manager.UserManagement.Controls
{
   public class ConnectionFolderFactory : NavigationTreeConnectionFolderFactory
    {
        override public NavigationTreeConnectionFolder NewNavigationTreeConnectionFolder(ConnectionDefinition aConnectionDefinition)
        {
            return new SystemConnectionFolder(aConnectionDefinition);
        }

    }

}
