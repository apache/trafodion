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
using System.Reflection;
using System.Windows.Forms;

namespace Trafodion.Manager
{
    class TrafodionLookAndFeelChangeHandler : IDisposable
    {
        Object _control;
        TrafodionLookAndFeelManager lookAndFeelManager;

        private bool _disposed = false;

        public TrafodionLookAndFeelManager LookAndFeelManager
        {
            get { return lookAndFeelManager; }
            set { lookAndFeelManager = value; }
        }

        /// <summary>
        /// Constructor that takes in an object whose look and feel has to be managed.
        /// </summary>
        /// <param name="aControl"></param>
        public TrafodionLookAndFeelChangeHandler(Object aControl)
        {
            this._control = aControl;

            //Get the look and feel manager from the context
            lookAndFeelManager = TrafodionContext.Instance.LookAndFeelManager;

            //set the current theme of the passed control
            setCurrentTheme();
        }

        /// <summary>
        /// Destructor to call the dispose to detach it from the lookAndFeelManager
        /// </summary>
        ~TrafodionLookAndFeelChangeHandler()
        {
            Dispose();
        }

        /// <summary>
        /// We have to ensure that when the object is disposed
        /// </summary>
        public virtual void Dispose()
        {
            if (!this._disposed)
            {
                _disposed = true;
                _control = null;
                lookAndFeelManager = null;
                GC.SuppressFinalize(this);
            }
        }


        /// <summary>
        /// Method to handle the change in look and feel 
        /// </summary>
        /// <param name="source"></param>
        /// <param name="args"></param>
        public void OnTrafodionLookAndFeelChanged(Object source, EventArgs args)
        {
            checkIfDisposed();
            setCurrentTheme();
        }

        //Use reflection to set the current theme of the control
        private void setCurrentTheme()
        {

        }

        //Method to throw an exception if an opertion is attempted on the object that is disposed
        private void checkIfDisposed()
        {
            if (this._disposed)
            {
                throw new ObjectDisposedException("TrafodionLookAndFeelChangeHandler for object " + _control + " has already been disposed");
            }
        }

    }
}
