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

using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.Framework
{

    /// <summary>
    /// Each area must implement this interface
    /// </summary>
    public interface ITrafodionArea
    {

        /// <summary>
        /// Property that the framework reads to find the area's name for its button and the active area label
        /// </summary>
        string AreaName { get; }

        /// <summary>
        /// Property that the framework reads to get the area's navigator control
        /// </summary>
        Control Navigator { get; }

        /// <summary>
        /// Property that the framework reads to get the area's right pane control
        /// </summary>
        Control RightPane { get; }

        /// <summary>
        /// Image that is displayed in the Area Button
        /// </summary>
        Image Image { get; }

        /// <summary>
        /// A callback mathod from Trafodion main to notify the area that it's now the active area
        /// </summary>
        void OnActivate();

    }

}
