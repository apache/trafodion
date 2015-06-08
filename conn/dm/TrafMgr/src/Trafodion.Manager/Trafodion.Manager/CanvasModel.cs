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

namespace Trafodion.Manager
{
    public class CanvasModel
    {
        /// <summary>
        /// The name given to the workspace
        /// </summary>
        private string _viewName = null;

        /// <summary>
        /// Textual description of the workspace
        /// </summary>
        private string _viewText = null;

        /// <summary>
        /// Flag indicating whether or not this Workspace can be deleted.
        /// </summary>
        private bool _allowDeletion = true;

        /// <summary>
        /// Flag indicating whether or not this Workspace View is locked.
        /// </summary>
        private bool _locked = false;

        #region  Properties
        public string ViewName
        {
            get { return _viewName; }
            set { _viewName = value; }
        }

        public bool AllowDelete
        {
            get { return this._allowDeletion; }
            set { this._allowDeletion = value; }
        }
        public string ViewText
        {
            get { return this._viewText; }
            set { this._viewText = value; }
        }

        public bool Locked
        {
            get { return this._locked; }
            set { this._locked = value;}
        }

        #endregion  /*  End of region  Properties.  */



    }
}
