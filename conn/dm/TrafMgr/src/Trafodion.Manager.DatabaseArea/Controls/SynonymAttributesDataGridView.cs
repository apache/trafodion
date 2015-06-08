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
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea.Controls
{
     
    /// <summary>
    /// Used to load Synonym attributes data into a data grid
    /// </summary>
    public class SynonymAttributesDataGridView : TrafodionSchemaObjectAttributesDataGridView
    {
        /// <summary>
        /// Holds catalog attributes 
        /// </summary>
        /// <param name="aDatabaseObjectsControl A Database object control or null"></param>
        /// <param name="aTrafodionCatalog A Trafodion catalog object"></param>
        public SynonymAttributesDataGridView(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionSynonym aTrafodionSynonym)
            : base(aDatabaseObjectsControl, aTrafodionSynonym)
        {
           
            
        }

        /// <summary>
        /// Gets or Set SQLMx catalog object value
        /// </summary>
        public TrafodionSynonym TheTrafodionSynonym
        {
            get { return TrafodionObject as TrafodionSynonym; }
            set { TrafodionObject = value; }
        }

        /// <summary>
        /// This LoadRows is not being used right now.  If more Synonym attributes ever become
        /// available then add the attribute rows here.
        /// </summary>
        override protected void LoadRows()
        {
      
        }

        #region ICloneToWindow

        /// <summary>
        /// Get a clone suitable for embedding in a managed window.
        /// </summary>
        /// <returns>The control</returns>
        override public Control Clone()
        {
            return new SynonymAttributesDataGridView(null, TheTrafodionSynonym);
        }

        #endregion

    }
}
