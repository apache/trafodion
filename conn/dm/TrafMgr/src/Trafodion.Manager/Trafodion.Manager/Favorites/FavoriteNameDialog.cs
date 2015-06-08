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

namespace Trafodion.Manager.Framework.Favorites
{

    /// <summary>
    /// A dialog that allows the user to name a favorite or favorites folder.  This
    /// class merely adds the fucntionality of determining that the OK button should be
    /// enabled when the input field is non-empty.
    /// </summary>
    public class FavoriteNameDialog : Trafodion.Manager.Framework.Controls.GetStringDialog
    {

        /// <summary>
        ///  Constructor
        /// </summary>
        /// <param name="aTitle">A titel for the dialog</param>
        /// <param name="aPrompt">A prompt string</param>
        public FavoriteNameDialog(string aTitle, string aPrompt)
            : base(aTitle, aPrompt)
        {
        }

        /// <summary>
        /// Default constructor.
        /// </summary>
        public FavoriteNameDialog()
        {
        }

        /// <summary>
        /// Called by the base class to determine if the OK button should be anabled
        /// </summary>
        /// <returns></returns>
        override protected bool EnableOKButton()
        {

            // OK button is enabled whenever there is some input
            return (TheString.Length > 0);

        }
    }
}
