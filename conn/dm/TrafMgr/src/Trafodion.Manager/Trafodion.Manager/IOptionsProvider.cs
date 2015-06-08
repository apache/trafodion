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

namespace Trafodion.Manager.Framework
{
    /// <summary>
    /// Must be implemented by Areas/Tools/Widgets that want to expose
    /// options so that the framework can display it.
    /// </summary>
    public interface IOptionsProvider
    {
        /// <summary>
        /// Property that the framework reads to get the options control
        /// </summary>
        List<IOptionControl> OptionControls { get; }

        /// <summary>
        /// Objects that shall contain option values. Since no UI is being provided,
        /// a property grid shall be used to display the options
        /// </summary>
        Dictionary<String, IOptionObject> OptionObjects { get; }
    }
}
