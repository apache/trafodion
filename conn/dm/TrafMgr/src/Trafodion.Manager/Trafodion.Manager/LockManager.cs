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
using System.ComponentModel;

namespace Trafodion.Manager.Framework
{
    static public class  LockManager
    {

        /// <summary>
        /// It is either Lock or Unlock.
        /// </summary>
        public enum LockOperation
        {
            Lock,
            Unlock
        }
  

        /// <summary>
        /// Flag to indicate the whole TrafodionManager is locked or not.
        /// </summary>
        static private bool _locked = false;

        // List of all components having lock operations
        static private EventHandlerList theEventHandlers = new EventHandlerList();

        static private readonly string theLockEventKey = "LockManager";


        /// <summary>
        /// Default constructor.
        /// </summary>
        static LockManager()
        {

        }


        #region  Properties

        /// <summary>
        /// Read only property.
        /// </summary>
        static public bool Locked
        {
            get { return _locked; }
        }

        #endregion  /*  End of region  Properties.  */


        #region  Public Methods

        /// <summary>
        /// To perform a lock operation on the entire TrafodionManager.  This includes 
        /// firing a lock event.
        /// </summary>
        static public void DoLock()
        {
            _locked = true;
            FireLockEvent(LockOperation.Lock);
        }


        /// <summary>
        /// To perform a unlock operation on the entire TrafodionManager.  This includes 
        /// firing a unlock event.
        /// </summary>
        static public void DoUnlock()
        {
            _locked = false;
            FireLockEvent(LockOperation.Unlock);
        }


        /// <summary>
        /// A Lock event handler
        /// </summary>
        /// <param name="aLockOperation">Lock or Unlock</param>
        public delegate void LockHandler(LockOperation aLockOperation);


        /// <summary>
        /// The list of all components having lock operations
        /// </summary>
        static public event LockHandler LockHandlers
        {
            add { theEventHandlers.AddHandler(theLockEventKey, value); }
            remove { theEventHandlers.RemoveHandler(theLockEventKey, value); }
        }

        #endregion /* end of Public Methods region */


        #region  Private Methods

        /// <summary>
        /// Call to tell all interested components that they are to lock or unlock
        /// </summary>
        /// <param name="aLockOperation">Whether to lock or unlock</param>
        static private void FireLockEvent(LockOperation aLockOperation)
        {
            LockHandler theLockHandlers = (LockHandler)theEventHandlers[theLockEventKey];

            if (theLockHandlers != null)
            {
                theLockHandlers(aLockOperation);
            }
        }

        #endregion /* end of Private Methods region */


    }
}
