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
using System.ComponentModel;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.OverviewArea.Controls.Tree;
using Trafodion.Manager.OverviewArea.Models;

namespace Trafodion.Manager.OverviewArea.Controls
{
    /// <summary>
    /// Shows the system-wide message for a given system.
    /// </summary>
    public partial class SystemMessageControl : UserControl, ICloneToWindow
    {
        #region Fields

        private SystemMessage _systemMessage;
        private SystemMessage.ChangeHandler _systemMessageChangeHandler;
        private bool _localUpdateFlag = false;
        private bool _getUpdate = true;
        private OverviewTreeView _overviewTreeView;
        private ConnectionDefinition _connectionDefinition;

        #endregion

        #region Properties

        public OverviewTreeView OverviewTreeView
        {
            get { return _overviewTreeView; }
            set { _overviewTreeView = value; }
        }
        /// <summary>
        /// The SystemMessage associated with this control
        /// </summary>
        public SystemMessage SystemMessage
        {
            get { return _systemMessage; }
            set
            {
                if (_systemMessage != null)
                {
                    // Remove from the old SystemMessage object.
                    _systemMessage.Change -= _systemMessageChangeHandler;
                }

                if (value == null)
                {
                    // The control shuts itself off when a SystemMessage is not set.
                    _headerPanel.Visible = false;
                    _messageRichTextBox.ReadOnly = true;
                    _buttonPanel.Visible = false;
                    return;
                }

                // Resurect the control when the SystemMessage is set.
                _headerPanel.Visible = true;
                _buttonPanel.Visible = true;

                _systemMessage = value;

                // Only can edit and apply changes if the SystemMessage allows it.
                SetEditable(_systemMessage.IsEditable);

                // Listen to SystemMessage changes.
                _systemMessage.Change += _systemMessageChangeHandler;

                // Do we request to update from the server or use the local copy.
                if (_getUpdate)
                {
                    GetSystemMessageUpdate();
                }
                else
                {
                    OnSystemMessageChange(_systemMessage, _systemMessage.SynchronizationState, _systemMessage.SynchronizationState);
                }
            }
        }

        /// <summary>
        /// The name of the system which this control is connected. If it is not
        /// associated with any system, the text will be "[Unknown System]".
        /// </summary>
        public string SystemName
        {
            get
            {
                if (SystemMessage == null)
                {
                    return Properties.Resources.UnknownSystemIndicator;
                }
                else
                {
                    return SystemMessage.ConnectionDef.Server;
                }
            }
        }

        /// <summary>
        /// Read only property that supplies a suitable base title for the managed window.
        /// </summary>
        public string WindowTitle
        {
            get
            {
                return string.Format(Properties.Resources.SystemMessageTitle, new object[] { SystemName });
            }
        }


        /// <summary>
        /// Stores Connection Definition for this object
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return _connectionDefinition; }
            set 
            {
                _connectionDefinition = value;
                if (_systemMessage != null)
                {
                    _systemMessage.ConnectionDef = value;
                }
            }
        }

        #endregion

        /// <summary>
        /// Creates a new SystemMessageControl.
        /// </summary>
        public SystemMessageControl()
        {
            InitializeComponent();

            // Default to a shutoff state until the SystemMessage is set.
            _messageRichTextBox.ReadOnly = true;
            _headerPanel.Visible = false;
            _buttonPanel.Visible = false;

            _systemMessageChangeHandler = new SystemMessage.ChangeHandler(OnSystemMessageChange);
        }

        /// <summary>
        /// Creates a new SystemMessageControl using another one. It will copy all of the
        /// information from the given control and add itself as a listener to changes made
        /// to the given control.
        /// </summary>
        /// <param name="systemMessageControl">The control that will be copied and listened to.</param>
        public SystemMessageControl(SystemMessageControl systemMessageControl) : this()
        {
            // Do not trigger another request to update the SystemMessage. Keep using the local copy.
            _getUpdate = false;

            // Simply copy the SystemMessage. The rest will sync up nicely.
            SystemMessage = systemMessageControl.SystemMessage;

            _getUpdate = true;
        }

        /// <summary>
        /// Get a clone suitable for embedding in a managed window.
        /// </summary>
        /// <returns>The control</returns>
        public Control Clone()
        {
            SystemMessageControl systemMessageControl = new SystemMessageControl(this);
            return systemMessageControl;
        }

        /// <summary>
        /// Handles clicks to the Overview Tree
        /// </summary>
        /// <param name="aNavigationTreeNode"></param>
        public void OverviewTreeControlSelected(NavigationTreeNode aNavigationTreeNode)
        {
        }

        private void GetSystemMessageUpdate()
        {
            try
            {
                // Trigger an update.
                _systemMessage.Refresh();
            }
            catch (SystemMessageException e)
            {
                _lastUpdatedLabel.Text = Properties.Resources.UnknownIndicator;
                MessageBox.Show(Utilities.GetForegroundControl(), e.Message, Properties.Resources.Error, MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        /// <summary>
        /// Handler of changes made to other SystemMessage objects.
        /// </summary>
        /// <param name="systemMessage">The SystemMessage object that changed.</param>
        /// <param name="oldState">The SystemMessage's state before it changed.</param>
        /// <param name="newState">The SystemMessage's state after it changed.</param>
        private void OnSystemMessageChange(SystemMessage systemMessage, SystemMessage.State oldState, SystemMessage.State newState)
        {
            switch(newState)
            {
                case SystemMessage.State.Updating:
                    {
                        Enabled = false;
                        _lastUpdatedLabel.Text = Properties.Resources.ContactingServer;
                        break;
                    }
                case SystemMessage.State.Updated:
                case SystemMessage.State.ReadError:
                    {
                        Enabled = true;

                        // Update the updated label text.
                        if (systemMessage.HasUnsavedChanges)
                        {
                            _lastUpdatedLabel.Text = Properties.Resources.UnsavedChanges;
                        }
                        else
                        {
                            if (newState == SystemMessage.State.ReadError)
                            {
                                MessageBox.Show(Utilities.GetForegroundControl(), systemMessage.LastError.Message, Properties.Resources.Error, MessageBoxButtons.OK);
                            }
                            _lastUpdatedLabel.Text = systemMessage.LastUpdateText;
                        }

                        break;
                    }
                case SystemMessage.State.SaveError:
                    {
                        Enabled = true;
                        if (newState == SystemMessage.State.SaveError)
                        {
                            MessageBox.Show(Utilities.GetForegroundControl(), systemMessage.LastError.Message, Properties.Resources.Error, MessageBoxButtons.OK);
                        }

                        // Update the updated label text.
                        if (systemMessage.HasUnsavedChanges)
                        {
                            _lastUpdatedLabel.Text = Properties.Resources.UnsavedChanges;
                        }
                        else
                        {
                            _lastUpdatedLabel.Text = systemMessage.LastUpdateText;
                        }

                        break;
                    }
                default:
                    // No other cases to handle.
                    break;
            }

            _applyButton.Enabled = systemMessage.HasUnsavedChanges;

            // Set the flag so that the text handler will not notify the SystemMesage about its own update.
            _localUpdateFlag = true;

            // Update the text area.
            try
            {
                // The SystemMessage fires a change event when the connection definition changes.
                // Update the edit state just in case.
                SetEditable(systemMessage.IsEditable);
                _messageRichTextBox.Rtf = systemMessage.Message;
                //_messageRichTextBox.Text = systemMessage.Message;
            }
            catch (ArgumentException)
            {
                // The text doesn't appear to be in RTF format, try basic text.
                _messageRichTextBox.Text = systemMessage.Message;
            }
            _messageRichTextBox.SelectionStart = _messageRichTextBox.Text.Length;
            _localUpdateFlag = false;
        }

        /// <summary>
        /// Handler of clicks made to the apply button. Sends the text from
        /// the message area to the server.
        /// </summary>
        /// <param name="sender">The source of the event.</param>
        /// <param name="e">Arguments that pertain to the event.</param>
        private void OnApplyButtonClick(object sender, EventArgs e)
        {
            // Update the server.
            _systemMessage.Save();
        }

        /// <summary>
        /// Handler of clicks made to the refresh button. It will cause the
        /// message area to be updated with text from the server.
        /// </summary>
        /// <param name="sender">The source of the event.</param>
        /// <param name="e">Arguments that pertain to the event.</param>
        private void OnRefreshButtonClick(object sender, EventArgs e)
        {
            if (_systemMessage.HasUnsavedChanges &&
                MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.LoseUnsavedChangesConfirmation,
                    Properties.Resources.UnsavedChanges, MessageBoxButtons.YesNo, MessageBoxIcon.Warning, MessageBoxDefaultButton.Button2) != DialogResult.Yes)
            {
                return;
            }
            
            //ByPass text change handler when clearing the message text
            _localUpdateFlag = true;
            this._messageRichTextBox.Clear();
            _systemMessage.Reset();
            _localUpdateFlag = false;

            GetSystemMessageUpdate();
        }

        /// <summary>
        /// Handler of changes made to the message area.
        /// </summary>
        /// <param name="sender">The source of the event.</param>
        /// <param name="e">Arguments that pertain to the event.</param>
        private void OnMessageTextChanged(object sender, EventArgs e)
        {
            if (!_localUpdateFlag)
            {
                SystemMessage.Message = _messageRichTextBox.Text;
            }
        }

        /// <summary>
        /// Sets the controls appearance if the user can edit the system message.
        /// </summary>
        /// <param name="isEditable">true if the system message is editable, otherwise false.</param>
        private void SetEditable(bool isEditable)
        {
            // Bug in the text box-- when switching readonly, the
            // background doesn't change color.
            if (_messageRichTextBox.ReadOnly == isEditable)
            {
                if (isEditable)
                    _messageRichTextBox.BackColor = System.Drawing.Color.White;
                else
                    _messageRichTextBox.BackColor = System.Drawing.SystemColors.Control;
            }

            _messageRichTextBox.ReadOnly = !isEditable;
            _applyButton.Visible = isEditable;
        }

        private void MessageContextMenuStrip_Opening(object sender, CancelEventArgs e)
        {
            if (_messageRichTextBox.ReadOnly)
            {
                e.Cancel = true;
                return;
            }

            bool textSelected = _messageRichTextBox.SelectionLength > 0;
            _cutToolStripMenuItem.Enabled = textSelected;
            _copyToolStripMenuItem.Enabled = textSelected;
            _pasteToolStripMenuItem.Enabled = _messageRichTextBox.CanPaste(DataFormats.GetFormat(DataFormats.Text));
            _undoToolStripMenuItem.Enabled = _messageRichTextBox.CanUndo;
            _redoToolStripMenuItem.Enabled = _messageRichTextBox.CanRedo;
        }

        private void CopyToolStripMenuItem_Click(object sender, EventArgs e)
        {
            _messageRichTextBox.Copy();
        }

        private void PasteToolStripMenuItem_Click(object sender, EventArgs e)
        {
            _messageRichTextBox.Paste();
        }

        private void CutToolStripMenuItem_Click(object sender, EventArgs e)
        {
            _messageRichTextBox.Cut();
        }

        private void UndoToolStripMenuItem_Click(object sender, EventArgs e)
        {
            _messageRichTextBox.Undo();
        }

        private void RedoToolStripMenuItem_Click(object sender, EventArgs e)
        {
            _messageRichTextBox.Redo();
        }
    }
}
