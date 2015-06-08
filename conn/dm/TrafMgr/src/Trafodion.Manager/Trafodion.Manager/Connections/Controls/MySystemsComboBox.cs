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
using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.Framework.Connections.Controls
{
    /// <summary>
    /// 
    /// </summary>
    public class MySystemsComboBox : TrafodionComboBox
    {
        private bool _isLoading = false;
        ImageList _imageList;

        /// <summary>
        /// 
        /// </summary>
        public MySystemsComboBox()
        {
            DropDownStyle = ComboBoxStyle.DropDownList;
            Sorted = true;
            ConnectionDefinition.Changed += ConnectionDefinitionChanged;
            _imageList = new ImageList();
            _imageList.Images.Add(TrafodionTreeView.CONNECTED_SERVER_ICON, Properties.Resources.ServerConnectedIcon);
            _imageList.Images.Add(TrafodionTreeView.SERVER_ICON, Properties.Resources.ServerIcon);
            DrawMode = DrawMode.OwnerDrawFixed;

            LoadSystems();

        }
        ~MySystemsComboBox()
        {
            ConnectionDefinition.Changed -= ConnectionDefinitionChanged;
        }

        protected override void OnDrawItem(DrawItemEventArgs ea)
        {
            ea.DrawBackground();
            ea.DrawFocusRectangle();

            Size imageSize = _imageList.ImageSize;
            Rectangle bounds = ea.Bounds;

            try
            {
                ConnectionDefinitionWrapper item = (ConnectionDefinitionWrapper)Items[ea.Index];

                if (item.TheConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
                {
                    _imageList.Draw(ea.Graphics, bounds.Left, bounds.Top, _imageList.Images.IndexOfKey(TrafodionTreeView.CONNECTED_SERVER_ICON));
                    ea.Graphics.DrawString(item.ConnectionName, ea.Font, new SolidBrush(ea.ForeColor), bounds.Left + imageSize.Width, bounds.Top);
                }
                else if (item.TheConnectionDefinition.TheState == ConnectionDefinition.State.LiveFeedTestSucceeded)
                {
                    _imageList.Draw(ea.Graphics, bounds.Left, bounds.Top, _imageList.Images.IndexOfKey(TrafodionTreeView.LF_CONNECTED_SERVER_ICON));
                    ea.Graphics.DrawString(item.ConnectionName, ea.Font, new SolidBrush(ea.ForeColor), bounds.Left + imageSize.Width, bounds.Top);
                }
                else
                {
                    _imageList.Draw(ea.Graphics, bounds.Left, bounds.Top, _imageList.Images.IndexOfKey(TrafodionTreeView.SERVER_ICON));
                    ea.Graphics.DrawString(item.ConnectionName, ea.Font, new SolidBrush(ea.ForeColor), bounds.Left + imageSize.Width, bounds.Top);
                }
            }
            catch
            {
                if (ea.Index != -1)
                {
                    ea.Graphics.DrawString(Items[ea.Index].ToString(), ea.Font, new SolidBrush(ea.ForeColor), bounds.Left, bounds.Top);
                }
                else
                {
                    ea.Graphics.DrawString(Text, ea.Font, new SolidBrush(ea.ForeColor), bounds.Left, bounds.Top);
                }
            }

            base.OnDrawItem(ea);
        }



        /// <summary>
        /// 
        /// </summary>
        public bool IsLoading
        {
            get { return _isLoading; }
            set { _isLoading = value; }
        }

        public string SelectedCatalogName
        {
            get
            {
                if ((SelectedIndex < 0) || !(SelectedItem is ConnectionDefinitionWrapper))
                {
                    return null;
                }
                return ((ConnectionDefinitionWrapper)SelectedItem).CatalogName;
            }
            set
            {
                if (SelectedIndex >= 0 && SelectedItem is ConnectionDefinitionWrapper && value != null)
                {
                    ((ConnectionDefinitionWrapper)SelectedItem).CatalogName = value;
                }
            }
        }

        public string SelectedSchemaName
        {
            get
            {
                if ((SelectedIndex < 0) || !(SelectedItem is ConnectionDefinitionWrapper))
                {
                    return null;
                }
                return ((ConnectionDefinitionWrapper)SelectedItem).SchemaName;
            }
            set
            {
                if (SelectedIndex >= 0 && SelectedItem is ConnectionDefinitionWrapper && value != null)
                {
                    ((ConnectionDefinitionWrapper)SelectedItem).SchemaName = value;
                }
            }
        }

        public ConnectionDefinition SelectedConnectionDefinition
        {
            get
            {
                if ((SelectedIndex < 0) || !(SelectedItem is ConnectionDefinitionWrapper))
                {
                    return null;
                }
                return ((ConnectionDefinitionWrapper)SelectedItem).TheConnectionDefinition;
            }
            set
            {
                if (value != null)
                {
                    foreach (object theObject in Items)
                    {
                        ConnectionDefinitionWrapper theConnectionDefinitionWrapper = theObject as ConnectionDefinitionWrapper;
                        if (theConnectionDefinitionWrapper != null)
                        {
                            if (value.Name.Equals(theConnectionDefinitionWrapper.TheConnectionDefinition.Name))
                            {
                                SelectedItem = theConnectionDefinitionWrapper;
                            }
                        }
                    }
                }
            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        /// <returns></returns>
        protected virtual bool BelongsInThisComboBox(ConnectionDefinition aConnectionDefinition)
        {
            return true;
        }

        void ConnectionDefinitionChanged(object aSender, ConnectionDefinition aConnectionDefinition, ConnectionDefinition.Reason aReason)
        {
            if (InvokeRequired)
            {
            }
            LoadSystems();
        }

        private delegate void LoadSystemsDelegate();
        private void LoadSystems()
        {
            if (InvokeRequired)
            {
                Invoke(new LoadSystemsDelegate(LoadSystems), new object[0]);
            }
            else
            {
                try
                {
                    IsLoading = true;

                    string theSelectedConnectionDefinitionName = null;

                    if ((SelectedItem != null) && (SelectedItem is ConnectionDefinitionWrapper))
                    {
                        theSelectedConnectionDefinitionName = ((ConnectionDefinitionWrapper)SelectedItem).ConnectionName;
                    }

                    Items.Clear();

                    foreach (ConnectionDefinition theConnectionDefinition in ConnectionDefinition.ConnectionDefinitions)
                    {
                        if (BelongsInThisComboBox(theConnectionDefinition))
                        {
                            Items.Add(new ConnectionDefinitionWrapper(theConnectionDefinition));
                        }
                    }

                    if (theSelectedConnectionDefinitionName != null)
                    {

                        foreach (object theObject in Items)
                        {
                            if ((theObject is ConnectionDefinitionWrapper) && ((ConnectionDefinitionWrapper)theObject).ConnectionName.Equals(theSelectedConnectionDefinitionName))
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
                finally
                {
                    IsLoading = false;
                }

            }
        }

        private class ConnectionDefinitionWrapper : IComparable
        {
            private ConnectionDefinition _theConnectionDefinition;
            private string _catalogName;
            private string _schemaName;

            public string CatalogName
            {
                get { return _catalogName; }
                set { _catalogName = value; }
            }
            public string SchemaName
            {
                get { return _schemaName; }
                set { _schemaName = value; }
            }

            internal ConnectionDefinition TheConnectionDefinition
            {
                get { return _theConnectionDefinition; }
            }

            internal ConnectionDefinitionWrapper(ConnectionDefinition aConnectionDefinition)
            {
                _theConnectionDefinition = aConnectionDefinition;
                _catalogName = _theConnectionDefinition.DefaultCatalog;
                if (string.IsNullOrEmpty(_catalogName))
                {
                    _catalogName = "TRAFODION";
                }
                _schemaName = _theConnectionDefinition.DefaultSchema;
                ConnectionDefinition.Changed += ConnectionDefinition_Changed;
            }

            ~ConnectionDefinitionWrapper()
            {
                ConnectionDefinition.Changed -= ConnectionDefinition_Changed;
            }


            void ConnectionDefinition_Changed(object aSender, ConnectionDefinition aConnectionDefinition, ConnectionDefinition.Reason aReason)
            {
                if (aConnectionDefinition.Name.Equals(_theConnectionDefinition.Name) && aReason == ConnectionDefinition.Reason.Tested)
                {
                    _schemaName = aConnectionDefinition.DefaultSchema;
                }
            }

            public override string ToString()
            {
                return ConnectionName;
            }

            public string ConnectionName
            {
                get { return _theConnectionDefinition.Name; }
            }

            #region IComparable Members

            public int CompareTo(object anObject)
            {
                return anObject.ToString().CompareTo(ToString());
            }

            #endregion
        }

    }

    /// <summary>
    /// 
    /// </summary>
    public class MyActiveSystemsComboBox : MySystemsComboBox
    {
        /// <summary>
        /// 
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        /// <returns></returns>
        protected override bool BelongsInThisComboBox(ConnectionDefinition aConnectionDefinition)
        {
            return (aConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded);
        }
    }

    /// <summary>
    /// 
    /// </summary>
    public class MyOtherSystemsComboBox : MyActiveSystemsComboBox
    {
        /// <summary>
        /// 
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        /// <returns></returns>
        protected override bool BelongsInThisComboBox(ConnectionDefinition aConnectionDefinition)
        {
            return !base.BelongsInThisComboBox(aConnectionDefinition);
        }
    }

}
