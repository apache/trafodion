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
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// 
    /// </summary>
    /// <typeparam name="PARENT"></typeparam>
    /// <typeparam name="CHILD"></typeparam>
    abstract public class TrafodionObjectsComboBox<PARENT, CHILD> : TrafodionComboBox
        where PARENT : TrafodionObject
        where CHILD : TrafodionObject
    {
        private PARENT _theParentTrafodionObject = null;
        private bool _isLoading = false;

        /// <summary>
        /// constructor
        /// </summary>
        public TrafodionObjectsComboBox()
        {
            DropDownStyle = ComboBoxStyle.DropDownList;
        }

        /// <summary>
        /// Boolean value indicating if the combo box is being loaded with sql objects
        /// </summary>
        public bool IsLoading
        {
            get { return _isLoading; }
        }

        /// <summary>
        /// 
        /// </summary>
        abstract protected List<CHILD> ChildTrafodionObjects
        {
            get;
        }

        /// <summary>
        /// 
        /// </summary>
        public void RefreshContents()
        {
            if (_theParentTrafodionObject != null)
            {
                _theParentTrafodionObject.Refresh();
            }
            Load();
        }

        protected PARENT TheParentTrafodionObject
        {
            get { return _theParentTrafodionObject; }
            set
            {
                PARENT oldParent = _theParentTrafodionObject;
                _theParentTrafodionObject = value;
                if (oldParent == null || oldParent != _theParentTrafodionObject)
                {
                    Load();
                }
            }
        }

        private void Load()
        {
            string theSelectedName = null;

            if ((SelectedItem != null) && (SelectedItem is TrafodionObjectWrapper))
            {
                theSelectedName = ((TrafodionObjectWrapper)SelectedItem).ExternalName;
            }

            _isLoading = true;

            Items.Clear();

            try
            {
                if (_theParentTrafodionObject != null)
                {
                    foreach (CHILD theTrafodionObject in ChildTrafodionObjects)
                    {
                        Items.Add(new TrafodionObjectWrapper(theTrafodionObject));
                    }
                }
                SelectExternalName(theSelectedName);
            }
            finally
            {
                _isLoading = false;
            }

        }

        public void SelectExternalName(string anExternalName)
        {
            if ((anExternalName != null) && (anExternalName.Length > 0))
            {
                string theCanonicalName = TrafodionName.CanonicalForm(anExternalName);

                foreach (object theObject in Items)
                {
                    if ((theObject is TrafodionObjectWrapper) && TrafodionName.CanonicalForm(((TrafodionObjectWrapper)theObject).ExternalName).Equals(theCanonicalName))
                    {
                        SelectedItem = theObject;
                        break;
                    }
                }
            }
            if ((Items.Count > 0) && (SelectedIndex < 0))
            {
                SelectedIndex = 0;
            }
        }

        public CHILD SelectedTrafodionObject
        {
            get
            {
                if ((SelectedIndex < 0) || !(SelectedItem is TrafodionObjectWrapper))
                {
                    return null;
                }
                return ((TrafodionObjectWrapper)SelectedItem).TheTrafodionObject as CHILD;
            }
            set
            {
                this.SelectedItem = null;
                if (value != null)
                {
                    foreach (object theObject in Items)
                    {
                        TrafodionObjectWrapper theTrafodionObjectWrapper = theObject as TrafodionObjectWrapper;
                        if (theTrafodionObjectWrapper != null)
                        {
                            if (value.ExternalName.Equals(theTrafodionObjectWrapper.TheTrafodionObject.ExternalName))
                            {
                                SelectedItem = theTrafodionObjectWrapper;
                            }
                        }
                    }
                }
            }
        }

        private class TrafodionObjectWrapper : IComparable
        {
            private TrafodionObject _theTrafodionObject;

            internal TrafodionObject TheTrafodionObject
            {
                get { return _theTrafodionObject; }
            }

            internal TrafodionObjectWrapper(TrafodionObject aTrafodionObject)
            {
                _theTrafodionObject = aTrafodionObject;
            }

            public override string ToString()
            {
                return ExternalName;
            }

            public string ExternalName
            {
                get { return _theTrafodionObject.ExternalName; }
            }

            #region IComparable Members

            public int CompareTo(object anObject)
            {
                return anObject.ToString().CompareTo(ToString());
            }

            #endregion
        }

    }

}
