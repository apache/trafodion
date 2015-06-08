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
using System.Runtime.CompilerServices;

namespace Trafodion.Manager
{
    /// <summary>
    /// This class shall extended by all objects that wants its listeners 
    /// to be notified of any change in the model. 
    /// </summary>
    [Serializable]
    public class DefaultChangeEventFiringModel
    {
        [NonSerialized]
        private EventHandler _ModelChanged;
        public event EventHandler ModelChanged
        {
            [MethodImpl(MethodImplOptions.Synchronized)]
            add
            {
                _ModelChanged = (EventHandler)Delegate.Combine(_ModelChanged, value);
            }

            [MethodImpl(MethodImplOptions.Synchronized)]
            remove
            {
                _ModelChanged = (EventHandler)Delegate.Remove(_ModelChanged, value);
            }
        }

        public void fireModelChangedEvent()
        {
            if (_ModelChanged != null)
            {
                _ModelChanged(this, null);
            }
        }

    }
}
