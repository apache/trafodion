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

using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.Framework.Controls
{

    /// <summary>
    /// Interface supported by controls that can be cloned into managed windows.
    /// </summary>
    public interface ICloneToWindow
    {

        /// <summary>
        /// Get a clone suitable for embedding in a managed window.
        /// </summary>
        /// <returns>The control</returns>
        Control Clone();

        /// <summary>
        /// Read only property that supplies a suitable base title for the managed window.
        /// </summary>
        string WindowTitle { get; }

        /// <summary>
        /// ConnectionDefinition property that needs be in all Cloned-Windows e.g. to enable its closing
        /// </summary>
        ConnectionDefinition ConnectionDefn { get;  }

    }
}
