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
    /// Objects implementing this interface can be displayed in the OptionsDialog
    /// </summary>
    public interface IOptionObject
    {
        /// <summary>
        /// Called when the user clicks the Ok button on the options panel.
        /// </summary>
        void OnOptionsChanged();

        /// <summary>
        /// This method shall be called by the framework to set the options that
        /// have obtained from the persistance framework. The persisted options
        /// are obtained using the IOptionProvider's name and the Option title.
        /// </summary>
        /// <param name="persistedObject"></param>
        void LoadedFromPersistence(Object persistedObject);

        /// <summary>
        /// This method to clone self image for the purpose of display the dialog
        /// </summary>
        /// <returns></returns>
        IOptionObject Clone();

        /// <summary>
        /// This is the copy method.
        /// </summary>
        /// <param name="obj"></param>
        void Copy(IOptionObject obj);
    }
}
