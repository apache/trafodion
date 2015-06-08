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
using Trafodion.Manager.Framework.Controls;
using System.Windows.Forms;

namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    public class QueryResultTabPage : TrafodionTabPage
    {
        #region Fields

        private Control _theQueryResultControl;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: TheQueryResultControl - the result control to be placed in the Tab page.
        /// </summary>
        public Control TheQueryResultControl
        {
            get { return _theQueryResultControl; }
            set
            {
                _theQueryResultControl = value;

                Controls.Clear();

                if (_theQueryResultControl != null)
                {
                    _theQueryResultControl.Dock = DockStyle.Fill;
                    Controls.Add(_theQueryResultControl);
                }
            }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// Default constructor
        /// </summary>
        public QueryResultTabPage()
            : base("Execution Results")
        {
        }

        /// <summary>
        /// Constructor with given title name
        /// </summary>
        /// <param name="aTitle"></param>
        public QueryResultTabPage(string aTitle)
            : base(aTitle)
        {
        }

        #endregion Constructors

        #region Private methods

        /// <summary>
        /// Override the dispose method to prevent result control being disposed.
        /// The result control is part of the report definition and should be disposed
        /// when the report definition is disposed.
        /// </summary>
        /// <param name="disposing"></param>
        protected override void Dispose(bool disposing)
        {
            // This would cause only the container to be disposed.  The result control will still be
            // kept in the report definition. 
            Controls.Clear(); 
                              
            base.Dispose(disposing);
        }

        #endregion Private methods

    }
}
