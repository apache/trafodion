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
using System.Data.Odbc;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.Framework.Navigation
{
    public partial class TrafodionRightPaneControl : UserControl, ICloneToWindow
    {
        #region Fields

        private NavigationUserControl _navigationUserControl;
        private NavigationTreeView.SelectedHandler _navigationTreeControlSelectedHandler = null;
        private TreeViewCancelEventHandler _navigationTreeControlBeforeSelectedHandler = null; //To capture 'Before Select'
        private NavigationTreeNameFilter.ChangedHandler _filterChangedHandler = null;
        private NavigationTreeView.ExceptionOccurredHandler _navigationTreeControlExceptionOccurredHandler = null;
        private string _associatedAreaName;
        protected NavigationTreeNode _previouslySelectedNode;
        private bool selectedNodeChanged = false;
        private Control _rightPaneUserControl = null;

        #endregion Fields

        # region Properties

        /// <summary>
        /// 
        /// </summary>
        public NavigationUserControl TheNavigationUserControl
        {
            get { return _navigationUserControl; }
            set
            {
                _navigationUserControl = value;

                if (_navigationUserControl!= null && _navigationUserControl.TheNavigationTreeView != null)
                {
                    if (_navigationTreeControlSelectedHandler == null)
                    {
                        _navigationTreeControlSelectedHandler = new NavigationTreeView.SelectedHandler(NavigationTreeControlSelected);
                    }
                    if (_navigationTreeControlExceptionOccurredHandler == null)
                    {
                        _navigationTreeControlExceptionOccurredHandler = new NavigationTreeView.ExceptionOccurredHandler(NavigationTreeControlExceptionOccurred);
                    }
                    _navigationUserControl.TheNavigationTreeView.ExceptionOccurred += _navigationTreeControlExceptionOccurredHandler;

                    //Event handler added to capture the 'Before Select' event. 
                    //This allows us to cancel if changes haven't been saved
                    if (_navigationTreeControlBeforeSelectedHandler == null)
                    {
                        _navigationTreeControlBeforeSelectedHandler = new TreeViewCancelEventHandler(NavigationTreeControlBeforeSelect);
                    }
                    
                    _navigationUserControl.TheNavigationTreeView.Selected += _navigationTreeControlSelectedHandler;
                    _navigationUserControl.TheNavigationTreeView.BeforeSelect += _navigationTreeControlBeforeSelectedHandler;
                    if (_filterChangedHandler == null)
                    {
                        _filterChangedHandler = new NavigationTreeNameFilter.ChangedHandler(FilterChanged);
                        NavigationTreeNameFilter.Changed += _filterChangedHandler;
                    }
                }
            }
        }

        public Control RightPaneUserControl
        {
            get { return _rightPaneUserControl; }
            set { _rightPaneUserControl = value; }
        }

        #endregion Properties

        public TrafodionRightPaneControl()
        {
            InitializeComponent();
            TheNavigationUserControl = null;
        }
        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aNavigationUserControl"></param>
        public TrafodionRightPaneControl(NavigationUserControl aNavigationUserControl)
        {
            InitializeComponent();
            TheNavigationUserControl = aNavigationUserControl;
        }

        void MyDispose()
        {
            if (_navigationUserControl != null && _navigationUserControl.TheNavigationTreeView != null)
            {
                _navigationUserControl.TheNavigationTreeView.Selected -= _navigationTreeControlSelectedHandler;
                _navigationUserControl.TheNavigationTreeView.BeforeSelect -= _navigationTreeControlBeforeSelectedHandler;
            }
            if (_filterChangedHandler != null)
            {
                NavigationTreeNameFilter.Changed -= _filterChangedHandler;
            }
        }
        /// <summary>
        /// Handles the event triggered before a 'Select' is performed on the Connectivity Tree
        /// </summary>
        void NavigationTreeControlBeforeSelect(object sender, TreeViewCancelEventArgs e)
        {
            try
            {
                if (_navigationUserControl.TheNavigationTreeView.EditInProgress)
                {
                    e.Cancel = true;
                    return;
                }
                if(_associatedAreaName != null)
                {
                    if (_associatedAreaName.Equals(TrafodionContext.Instance.TheTrafodionMain.ActiveAreaName))
                    {
                        // check if user is about to change selected node
                        if (_previouslySelectedNode != null)
                        {
                            if (_previouslySelectedNode.FullPath != e.Node.FullPath)
                            {
                                // keep tracking if the RightPane control has to refresh/reload
                                selectedNodeChanged = true;
                                if (_rightPaneUserControl != null && _rightPaneUserControl is IPendingChange)
                                {
                                    PendingChangeObject pco = ((IPendingChange)_rightPaneUserControl).GetPendingChangeObject();
                                    if (pco.IsPending)
                                    {
                                        if (MessageBox.Show(Utilities.GetForegroundControl(), pco.Message, "Unsaved Changes",
                                            MessageBoxButtons.YesNo, MessageBoxIcon.Warning) == DialogResult.Yes)
                                        {
                                            e.Cancel = false;
                                        }
                                        else
                                        {
                                            e.Cancel = true;
                                        }
                                    }
                                }

                            }
                            else
                            {
                                // keep tracking if the RightPane control has to refresh/reload
                                selectedNodeChanged = false;
                            }
                        }
                    }
                    //else
                    //{
                    //    if (_previouslySelectedNode != null && _previouslySelectedNode != e.Node)
                    //    {
                    //        e.Cancel = true;
                    //    }
                    //}
                }

            }
            catch (Exception ex)
            {
            }
        }

        private void NavigationTreeControlExceptionOccurred(Exception anException)
        {
            NavigationTreeControlExceptionOccurredThreadProc(anException);
        }

        private void NavigationTreeControlExceptionOccurredThreadProc(object anObject)
        {
        }
        private void FilterChanged(NavigationTreeNameFilter aNameFilter)
        {
        }

        /// <summary>
        /// Handles clicks to the  Connectivity Tree
        /// </summary>
        /// <param name="aNavigationTreeNode"></param>
        private void NavigationTreeControlSelected(NavigationTreeNode aNavigationTreeNode)
        {
            if (Trafodion.Manager.Framework.Utilities.InUnselectedTabPage(this))
            {
                return;
            }
            
            _previouslySelectedNode = aNavigationTreeNode;
            _associatedAreaName = TrafodionContext.Instance.TheTrafodionMain.ActiveAreaName;

            HandleNavigationTreeNodeSelect(aNavigationTreeNode);
            SetRightPaneHeader(aNavigationTreeNode);
        }

        public virtual void HandleNavigationTreeNodeSelect(NavigationTreeNode aNavigationTreeNode)
        {

        }

        public virtual void SetRightPaneHeader(NavigationTreeNode aNavigationTreeNode)
        {
            SetLabels(aNavigationTreeNode.LongerDescription);
        }

        public virtual TrafodionRightPaneControl DoClone()
        {
            return new TrafodionRightPaneControl();
        }


        protected void AddControl(string text)
        {
            theContentPanel.Controls.Clear();
            TrafodionLabel aLabel = new TrafodionLabel();
            aLabel.Text = text;
            aLabel.Dock = DockStyle.Fill;
            aLabel.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            theContentPanel.Controls.Add(aLabel);
        }

        protected void AddControl(Control aControl)
        {
            RightPaneUserControl = aControl;
            theContentPanel.Controls.Clear();
            aControl.Dock = DockStyle.Fill;
            theContentPanel.Controls.Add(aControl);
        }

        protected void AddControl(Exception anException)
        {
            TrafodionTextBox theMessageTextBox = new TrafodionTextBox();
            theMessageTextBox.ReadOnly = true;
            theMessageTextBox.WordWrap = true;
            theMessageTextBox.Multiline = true;

            if (anException is OdbcException)
            {
                theMessageTextBox.Text = ConnectionDefinition.FixOdbcExceptionMessage((OdbcException)anException);
            }
            else
            {
                theMessageTextBox.Text = anException.Message;
            }
            theMessageTextBox.Text = string.Format("{0}{1}{2}", theMessageTextBox.Text , 
                                                        Environment.NewLine + Environment.NewLine + 
                                                        "A detailed log of this exception has been written to ", Logger.ErrorLog);

            Logger.OutputErrorLog(anException.StackTrace);

            theContentPanel.Controls.Clear();
            theMessageTextBox.Dock = DockStyle.Fill;
            theContentPanel.Controls.Add(theMessageTextBox);
        }

        private delegate void SetLabelsDelegate(string aTopPanelUpperText);
        protected void SetLabels(string aTopPanelUpperText)
        {
            if (InvokeRequired)
            {
                Invoke(new SetLabelsDelegate(SetLabels), new object[] { aTopPanelUpperText });
            }
            else
            {
                this._rightPaneToolTip.SetToolTip(this.topPanel_richTextBox, aTopPanelUpperText);
                this.topPanel_richTextBox.Controls.Clear();
                this.topPanel_richTextBox.Text = aTopPanelUpperText;
            }
        }
        #region IClone Methods

        public Control Clone()
        {
            TrafodionRightPaneControl theClonedTrafodionRightPaneControl = DoClone();

            theClonedTrafodionRightPaneControl.SetLabels(this.topPanel_richTextBox.Text);

            theClonedTrafodionRightPaneControl.Size = Size;
            if (_previouslySelectedNode != null)
            {
                theClonedTrafodionRightPaneControl.HandleNavigationTreeNodeSelect(_previouslySelectedNode);
            }

            return theClonedTrafodionRightPaneControl;
        }

        public string WindowTitle
        {
            get { return "" + " | " + this.topPanel_richTextBox.Text; }
        }

        public ConnectionDefinition ConnectionDefn
        {
            get
            {
                if (_previouslySelectedNode != null)
                {
                    return ((NavigationTreeNode)_previouslySelectedNode).TheConnectionDefinition;
                }
                return null;
            }
        }
        #endregion IClone Methods


    }
}
