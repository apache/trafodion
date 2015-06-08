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
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.Main
{
    /// <summary>
    /// The User control that the TrafodionManager framework uses to display options
    /// </summary>
    public partial class OptionsPanel : UserControl
    {
        #region Members
        Dictionary<string, IOptionsProvider> _options;
        IOptionControl  _currentOptionControl = null;
        IOptionObject   _currentOptionObject = null;
        String          _currentProviderName = null;
        String          _currentOptionName = null;
        #endregion

        #region Properties
        public Trafodion.Manager.Framework.Controls.TrafodionButton TheOkButton
        {
            get { return theOkButton; }
        }

        public Trafodion.Manager.Framework.Controls.TrafodionButton TheCancelButton
        {
            get { return theCancelButton; }
        }

        public Trafodion.Manager.Framework.Controls.TrafodionButton TheApplyButton
        {
            get { return theApplyButton; }
        }
        #endregion

        #region Constructor
        public OptionsPanel()
        {
            InitializeComponent();
            this.theOptionsTree.ImageList = null;
        }
        #endregion
        
        #region Public methods
        /// <summary>
        /// Displays the options based on option providers
        /// </summary>
        /// <param name="options"></param>
        public void showOptions(Dictionary<string, IOptionsProvider> options)
        {
            this._options = options;
            this.populateTree();
            this.theOptionsTree.ExpandAll();
        }

        /// <summary>
        /// Persists the options that the user has updated
        /// </summary>
        public bool PersistOptions()
        {
            object optionsObject = null;

            //Get the object to be persisted
            try
            {
                if (_currentOptionControl != null)
                {
                    optionsObject = _currentOptionControl.OnOptionsChanged();

                }
                else if (this._currentOptionObject != null)
                {
                    optionsObject = _currentOptionObject;
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), ex.Message, global::Trafodion.Manager.Properties.Resources.Error, MessageBoxButtons.OK);
                return false;
            }

            //Persist the object
            if (optionsObject != null)
            {
                try
                {
                    OptionStore.SaveOptionValues(_currentProviderName, _currentOptionName, optionsObject);
                }
                catch (Exception ex)
                {
                    MessageBox.Show(Utilities.GetForegroundControl(), ex.Message, global::Trafodion.Manager.Properties.Resources.Error, MessageBoxButtons.OK);
                    return false;
                }
            }

            //Raise event only when things really get changed.
            if (_currentOptionObject != null)
            {
                _currentOptionObject.OnOptionsChanged();
            }
            return true;
        }
        #endregion

        #region Private Methods
        private void populateTree()
        {
            // Enumerate and display all key and value pairs.
            foreach (KeyValuePair<string, IOptionsProvider> kvp in _options)
            {                
                OptionProviderNode node = new OptionProviderNode(kvp.Key, kvp.Value);
                if (node.ShouldBeDisplayed)
                {
                    this.theOptionsTree.Nodes.Add(node);
                    node.populateChildren();
                }
            }
        }

        private void theOptionsTree_AfterSelect(object sender, TreeViewEventArgs e)
        {
            showOptionsPanel();
        }

        private void showOptionsPanel()
        {
            OptionNode selectedNode = theOptionsTree.SelectedNode as OptionNode;
            this._currentOptionName = selectedNode.OptionsTitle;
            this._currentOptionControl = selectedNode.OptionsPanel as IOptionControl;
            PropertyGrid optionsView = selectedNode.OptionsPanel as PropertyGrid;
            this._currentOptionObject = (optionsView != null) ? optionsView.SelectedObject as IOptionObject: null;
            this._currentProviderName = selectedNode.ProviderName;

            showOptionControl(selectedNode.OptionsTitle, selectedNode.OptionsPanel);
        }
        
        private void showOptionControl(String title, Control control)
        {
            TrafodionGroupBox groupBox = new TrafodionGroupBox();
            groupBox.Text = title;
            groupBox.Controls.Add(control);

            groupBox.Dock = DockStyle.Fill;
            groupBox.Padding = new Padding(10);
            control.Dock = DockStyle.Fill;

            this.theOptionsPanel.Controls.Clear();
            this.theOptionsPanel.Controls.Add(groupBox);
            LoadOptions();

        }

        private void LoadOptions()
        {
            object optionsObject = OptionStore.GetOptionValues(_currentProviderName, _currentOptionName);
        
            if (_currentOptionControl != null)
            {
                ((IOptionControl)_currentOptionControl).LoadedFromPersistence(optionsObject);
            }
            else if (this._currentOptionObject != null)
            {
                if (optionsObject != null)
                {
                    _currentOptionObject.LoadedFromPersistence(optionsObject);
                }
            }
        }
        #endregion
    }

    /// <summary>
    /// This class shall be the node to represent the the Option provider on the tree.
    /// </summary>
    public class OptionProviderNode : TreeNode, OptionNode
    {
        #region Member
        int _controlCount = 0;
        int _objectCount = 0;
        IOptionsProvider _optionsProvider = null;
        Control _optionPanel = null;
        string _optionTitle = null;
        #endregion

        #region Constructor
        public OptionProviderNode(String text, IOptionsProvider provider)
        {
            _optionsProvider = provider;
            this.Text = text;
            if (this._optionsProvider != null)
            {
                _controlCount = (_optionsProvider.OptionControls != null) ? _optionsProvider.OptionControls.Count : 0;
                _objectCount = (_optionsProvider.OptionObjects != null) ? _optionsProvider.OptionObjects.Count : 0;
            }
            this.Tag = getOptionProviderNodeTag();
        }
        #endregion

        #region Public
        /// <summary>
        /// Indicates if there is a need to display this provider on the tree
        /// </summary>
        /// <returns></returns>
        public bool ShouldBeDisplayed
        {
            get
            {
                return hasOptionControls() || hasOptionObjects();
            }
        }

        /// <summary>
        /// Populates the tree with it's children
        /// </summary>
        public void populateChildren()
        {
            if (ShouldBeDisplayed)
            {
                if (!hasOnlyOneOption())
                {
                    if (hasOptionControls())
                    {
                        foreach (IOptionControl optionControl in _optionsProvider.OptionControls)
                        {
                            this.Nodes.Add(new OptionControlNode(optionControl));
                        }
                    }

                    if (hasOptionObjects())
                    {
                        foreach (KeyValuePair<string, IOptionObject> optionNameValuePair in _optionsProvider.OptionObjects)
                        {
                            this.Nodes.Add(new OptionsObjectNode(optionNameValuePair.Key, optionNameValuePair.Value));
                        }
                    }
                }


                if (!this.hasOnlyOneOption())
                {
                    OptionNode optionNode = this.FirstNode as OptionNode;
                    _optionPanel = (optionNode != null) ? optionNode.OptionsPanel : null;
                    _optionTitle = (optionNode != null) ? optionNode.OptionsTitle : null;
                }
                else
                {
                    if (hasOptionControls())
                    {
                        IOptionControl optionControl = this.Tag as IOptionControl;
                        _optionPanel = this.Tag as UserControl;
                        _optionTitle = optionControl.OptionTitle;
                    }
                    else if (hasOptionObjects())
                    {
                        DictionaryEntry de = (DictionaryEntry)this.Tag;
                        PropertyGrid optionsView = new PropertyGrid();
                        optionsView.SelectedObject = (de.Value != null) ? ((IOptionObject)de.Value).Clone() : de.Value;
                        _optionPanel = optionsView;
                        _optionTitle = (String)de.Key;
                    }
                }

            }
        }
        #endregion

        #region OptionNode
        /// <summary>
        /// Returns the Control that shall be displayed on the right
        /// </summary>
        public Control OptionsPanel
        {
            get
            {
                return _optionPanel;
            }
        }

        /// <summary>
        /// Returns the title of the option
        /// </summary>
        public string OptionsTitle
        {
            get
            {
                return _optionTitle;
            }
        }

        /// <summary>
        /// The name of the option provider
        /// </summary>
        public string ProviderName
        {
            get
            {
                return this.Text;
            }
        }
        #endregion

        #region Private
        private bool hasOnlyOneOption()
        {
            return ((_controlCount + _objectCount) == 1);
        }

        private bool hasOptionControls()
        {
            return _controlCount > 0;
        }

        private bool hasOptionObjects()
        {
            return _objectCount > 0;
        }

        private object getOptionProviderNodeTag()
        {
            if ((_optionsProvider != null) && (hasOptionControls() || hasOptionObjects()))
            {
                if (hasOnlyOneOption())
                {
                    if (hasOptionControls())
                    {
                        return _optionsProvider.OptionControls[0];
                    }
                    else
                    {
                        foreach (KeyValuePair<string, IOptionObject> optionNameValuePair in _optionsProvider.OptionObjects)
                        {
                            return new DictionaryEntry(optionNameValuePair.Key, optionNameValuePair.Value); //optionNameValuePair.Value;
                        }
                    }
                }
                else
                {
                    return _optionsProvider;
                }
            }
            return null;
        }
        #endregion
    }

    /// <summary>
    /// This class shall be used to display a user control that implements the IOptionControl interface
    /// </summary>
    public class OptionControlNode : TreeNode, OptionNode
    {
        #region Member
        IOptionControl _optionControl;
        #endregion

        #region Constructor
        public OptionControlNode(IOptionControl control)
        {
            if (control != null)
            {
                this._optionControl = control;
                this.Text = control.OptionTitle;
                this.Tag = control;
            }
        }
        #endregion

        #region OptionNode
        /// <summary>
        /// Returns the Control that shall be displayed on the right
        /// </summary>
        public Control OptionsPanel
        {
            get
            {
                return _optionControl as UserControl;
            }
        }

        /// <summary>
        /// Returns the title of the option
        /// </summary>
        public string OptionsTitle
        {
            get
            {
                return _optionControl.OptionTitle;
            }
        }

        /// <summary>
        /// The name of the option provider
        /// </summary>
        public string ProviderName
        {
            get
            {
                return ((OptionNode)this.Parent).ProviderName;
            }
        }
        #endregion
    }

    /// <summary>
    /// This class shall be used to display a user control that implements the IOptionObject interface
    /// </summary>
    public class OptionsObjectNode : TreeNode, OptionNode
    {
        #region Member
        String _title;
        Control _optionPanel = null;
        #endregion

        #region Constructor
        public OptionsObjectNode(String aTitle, IOptionObject anOptionObject)
        {
            _title = aTitle;
            this.Text = aTitle;
            this.Tag = anOptionObject;

            _optionPanel = new PropertyGrid();
            ((PropertyGrid)_optionPanel).SelectedObject = (anOptionObject != null) ? anOptionObject.Clone() : anOptionObject;

        }
        #endregion

        #region OptionNode
        /// <summary>
        /// Returns the Control that shall be displayed on the right
        /// </summary>
        public Control OptionsPanel
        {
            get
            {
                return _optionPanel;
            }
        }

        /// <summary>
        /// Returns the title of the option
        /// </summary>
        public string OptionsTitle
        {
            get
            {
                return _title;
            }
        }

        /// <summary>
        /// The name of the option provider
        /// </summary>
        public string ProviderName
        {
            get
            {
                return ((OptionNode)this.Parent).ProviderName;
            }
        }
        #endregion
    }

    /// <summary>
    /// Interface to be implemented by all TreeNode objects that shall be used to 
    /// display options
    /// </summary>
    public interface OptionNode
    {
        /// <summary>
        /// Returns the Control that shall be displayed on the right
        /// </summary>
        Control OptionsPanel { get; }
        
        /// <summary>
        /// Returns the title of the option
        /// </summary>
        string OptionsTitle { get; }
        
        /// <summary>
        /// The name of the option provider
        /// </summary>
        string ProviderName { get;}
    }
}
