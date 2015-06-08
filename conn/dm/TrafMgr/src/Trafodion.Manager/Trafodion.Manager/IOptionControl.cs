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
    /// This interface shall be implemented by the user controls that would 
    /// be displayed in the options dialog.
    /// </summary>
    public interface IOptionControl
    {
        /// <summary>
        /// The name for the control that will be displayed in the options
        /// panel and also shall be used as a Key during serialization.
        /// </summary>
        String OptionTitle { get; }

        /// <summary>
        /// Called when the user clicks the Ok button on the options panel.
        /// </summary>
        Object OnOptionsChanged();

        /// <summary>
        /// This method shall be called by the framework to set the options that
        /// have been obtained from the persistance framework. The persisted options
        /// are obtained using the IOptionProvider's name and the Option title.
        /// </summary>
        /// <param name="persistedObject"></param>
        void LoadedFromPersistence(Object persistedObject);

    }
}
