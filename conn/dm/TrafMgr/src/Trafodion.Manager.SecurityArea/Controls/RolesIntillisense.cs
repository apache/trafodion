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
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Trafodion.Manager.SecurityArea.Controls
{
    class RolesIntillisense
    {
        private RolesListBox _theAutoCompleteListBox;

        public RolesListBox TheAutoCompleteListBox
        {
            get { return _theAutoCompleteListBox; }
            set { _theAutoCompleteListBox = value; }
        }
        private System.Windows.Forms.RichTextBox _theRichTextBox;

        private string typed = "";
        private bool wordMatched = false;

        public RolesIntillisense()
        {
            initializeComponents();

        }

        public RolesIntillisense(RichTextBox aRichTextBox) : this()
        {
            TheRichTextBox = aRichTextBox;
        }


        public System.Windows.Forms.RichTextBox TheRichTextBox
        {
            get { return _theRichTextBox; }
            set 
            {
                if (_theRichTextBox != null)
                {
                    _theRichTextBox.KeyDown -= _theRichTextBox_KeyDown;
                    _theRichTextBox.MouseDown -= _theRichTextBox_MouseDown;
                }
                _theRichTextBox = value;
                _theRichTextBox.KeyDown += _theRichTextBox_KeyDown;
                _theRichTextBox.MouseDown += _theRichTextBox_MouseDown;

            }
        }


        private void initializeComponents()
        {
            this._theAutoCompleteListBox = new RolesListBox();
            this._theAutoCompleteListBox.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this._theAutoCompleteListBox.DrawMode = System.Windows.Forms.DrawMode.OwnerDrawFixed;
            this._theAutoCompleteListBox.Location = new System.Drawing.Point(136, 288);
            this._theAutoCompleteListBox.Name = "_theAutoCompleteListBox";
            this._theAutoCompleteListBox.Size = new System.Drawing.Size(208, 54);
            this._theAutoCompleteListBox.TabIndex = 3;
            this._theAutoCompleteListBox.Visible = false;

            _theAutoCompleteListBox.KeyDown += _theAutoCompleteListBox_KeyDown;
            _theAutoCompleteListBox.DoubleClick += _theAutoCompleteListBox_DoubleClick;
            _theAutoCompleteListBox.SelectedIndexChanged += _theAutoCompleteListBox_SelectedIndexChanged;

        }
        #region Component events

        private void _theRichTextBox_KeyDown(object sender, System.Windows.Forms.KeyEventArgs e)
        {
            // Keep track of the current character, used
            // for tracking whether to hide the list of members,
            // when the delete button is pressed
            int i = this._theRichTextBox.SelectionStart;
            string currentChar = "";

            if (i > 0)
            {
                currentChar = this._theRichTextBox.Text.Substring(i - 1, 1);
            }

            if (e.KeyData == Keys.OemPeriod)
            {
                // The amazing dot key

                if (!this._theAutoCompleteListBox.Visible)
                {
                    // Display the member listview if there are
                    // items in it
                    if (populateListBox())
                    {
                        //this._theAutoCompleteListBox.SelectedIndex = 0;

                        // Find the position of the caret
                        Point point = this._theRichTextBox.GetPositionFromCharIndex(_theRichTextBox.SelectionStart);
                        point.Y += (int)Math.Ceiling(this._theRichTextBox.Font.GetHeight()) + 2;
                        point.X += 2; // for Courier, may need a better method

                        this._theAutoCompleteListBox.Location = point;
                        this._theAutoCompleteListBox.BringToFront();
                        this._theAutoCompleteListBox.Show();
                    }
                }
                else
                {
                    this._theAutoCompleteListBox.Hide();
                    typed = "";
                }

            }
            else if (e.KeyCode == Keys.Back)
            {
                // Delete key - hides the member list if the character
                // being deleted is a dot

                //this.textBoxTooltip.Hide();
                if (typed.Length > 0)
                {
                    typed = typed.Substring(0, typed.Length - 1);
                }
                if (currentChar == ".")
                {
                    this._theAutoCompleteListBox.Hide();
                }

            }
            else if (e.KeyCode == Keys.Up)
            {
                // The up key moves up our member list, if
                // the list is visible

                //this.textBoxTooltip.Hide();

                if (this._theAutoCompleteListBox.Visible)
                {
                    this.wordMatched = true;
                    if (this._theAutoCompleteListBox.SelectedIndex > 0)
                        this._theAutoCompleteListBox.SelectedIndex--;

                    e.Handled = true;
                }
            }
            else if (e.KeyCode == Keys.Down)
            {
                // The up key moves down our member list, if
                // the list is visible

                //this.textBoxTooltip.Hide();

                if (this._theAutoCompleteListBox.Visible)
                {
                    this.wordMatched = true;
                    if (this._theAutoCompleteListBox.SelectedIndex < this._theAutoCompleteListBox.Items.Count - 1)
                        this._theAutoCompleteListBox.SelectedIndex++;

                    e.Handled = true;
                }
            }
            else if (e.KeyCode == Keys.D9)
            {
                // Trap the open bracket key, displaying a cheap and
                // cheerful tooltip if the word just typed is in our tree
                // (the parameters are stored in the tag property of the node)

                string word = this.getLastWord();
                //this.foundNode = false;
                //this.nameSpaceNode = null;

                //this.currentPath = "";
                //searchTree(this.treeViewItems.Nodes, word, true);

                //if (this.nameSpaceNode != null)
                //{
                //    if (this.nameSpaceNode.Tag is string)
                //    {
                //        this.textBoxTooltip.Text = (string)this.nameSpaceNode.Tag;

                //        Point point = this._theRichTextBox.GetPositionFromCharIndex(_theRichTextBox.SelectionStart);
                //        point.Y += (int)Math.Ceiling(this._theRichTextBox.Font.GetHeight()) + 2;
                //        point.X -= 10;
                //        this.textBoxTooltip.Location = point;
                //        this.textBoxTooltip.Width = this.textBoxTooltip.Text.Length * 6;

                //        this.textBoxTooltip.Size = new Size(this.textBoxTooltip.Text.Length * 6, this.textBoxTooltip.Height);

                //        // Resize tooltip for long parameters
                //        // (doesn't wrap text nicely)
                //        if (this.textBoxTooltip.Width > 300)
                //        {
                //            this.textBoxTooltip.Width = 300;
                //            int height = 0;
                //            height = this.textBoxTooltip.Text.Length / 50;
                //            this.textBoxTooltip.Height = height * 15;
                //        }
                //        this.textBoxTooltip.Show();
                //    }
                //}
            }
            else if (e.KeyCode == Keys.D8)
            {
                // Close bracket key, hide the tooltip textbox

                //this.textBoxTooltip.Hide();
            }
            else if (e.KeyValue < 48 || (e.KeyValue >= 58 && e.KeyValue <= 64) || (e.KeyValue >= 91 && e.KeyValue <= 96) || e.KeyValue > 122)
            {
                // Check for any non alphanumerical key, hiding
                // member list box if it's visible.

                if (this._theAutoCompleteListBox.Visible)
                {
                    // Check for keys for autofilling (return,tab,space)
                    // and autocomplete the richtextbox when they're pressed.
                    if (e.KeyCode == Keys.Return || e.KeyCode == Keys.Tab || e.KeyCode == Keys.Space)
                    {
                        //this.textBoxTooltip.Hide();

                        // Autocomplete
                        this.selectItem();

                        this.typed = "";
                        this.wordMatched = false;
                        e.Handled = true;
                    }

                    // Hide the member list view
                    this._theAutoCompleteListBox.Hide();
                }
            }
            else
            {
                // Letter or number typed, search for it in the listview
                if (this._theAutoCompleteListBox.Visible)
                {
                    char val = (char)e.KeyValue;
                    this.typed += val;

                    this.wordMatched = false;

                    // Loop through all the items in the listview, looking
                    // for one that starts with the letters typed
                    for (i = 0; i < this._theAutoCompleteListBox.Items.Count; i++)
                    {
                        if (this._theAutoCompleteListBox.Items[i].ToString().ToLower().StartsWith(this.typed.ToLower()))
                        {
                            this.wordMatched = true;
                            this._theAutoCompleteListBox.SelectedIndex = i;
                            break;
                        }
                    }
                }
                else
                {
                    this.typed = "";
                }
            }
        }

        private void _theRichTextBox_MouseDown(object sender, System.Windows.Forms.MouseEventArgs e)
        {
            // Hide the listview and the tooltip
            //this.textBoxTooltip.Hide();
            this._theAutoCompleteListBox.Hide();
        }


        private void _theAutoCompleteListBox_KeyDown(object sender, System.Windows.Forms.KeyEventArgs e)
        {
            // Ignore any keys being pressed on the listview
            this._theRichTextBox.Focus();
        }

        private void _theAutoCompleteListBox_DoubleClick(object sender, System.EventArgs e)
        {
            // Item double clicked, select it
            if (this._theAutoCompleteListBox.SelectedItems.Count == 1)
            {
                this.wordMatched = true;
                this.selectItem();
                this._theAutoCompleteListBox.Hide();
                this._theRichTextBox.Focus();
                this.wordMatched = false;
            }
        }

        private void _theAutoCompleteListBox_SelectedIndexChanged(object sender, System.EventArgs e)
        {
            // Make sure when an item is selected, control is returned back to the richtext
            this._theRichTextBox.Focus();
        }


        #endregion

        /// <summary>
        /// Called when a "." is pressed - the previous word is found,
        /// and if matched in the treeview, the members listbox is
        /// populated with items from the tree, which are first sorted.
        /// </summary>
        /// <returns>Whether an items are found for the word</returns>
        private bool populateListBox()
        {
            bool result = false;
            string word = this.getLastWord();

            //System.Diagnostics.Debug.WriteLine(" - Path: " +word);

            if (word != "")
            {
                result = true;
                this._theAutoCompleteListBox.Items.Add(new RolesListBoxItem("Role-1"));
                this._theAutoCompleteListBox.Items.Add(new RolesListBoxItem("Role-2"));
                this._theAutoCompleteListBox.Items.Add(new RolesListBoxItem("Role-3"));
                //    findNodeResult = null;
            //    findNode(word, this.treeViewItems.Nodes);

            //    if (this.findNodeResult != null)
            //    {
            //        this._theAutoCompleteListBox.Items.Clear();

            //        if (this.findNodeResult.Nodes.Count > 0)
            //        {
            //            result = true;

            //            // Sort alphabetically (this could be replaced with
            //            // a sortable treeview)
            //            MemberItem[] items = new MemberItem[this.findNodeResult.Nodes.Count];
            //            for (int n = 0; n < this.findNodeResult.Nodes.Count; n++)
            //            {
            //                MemberItem memberItem = new MemberItem();
            //                memberItem.DisplayText = this.findNodeResult.Nodes[n].Text;
            //                memberItem.Tag = this.findNodeResult.Nodes[n].Tag;

            //                if (this.findNodeResult.Nodes[n].Tag != null)
            //                {
            //                    System.Diagnostics.Debug.WriteLine(this.findNodeResult.Nodes[n].Tag.GetType().ToString());
            //                }

            //                items[n] = memberItem;
            //            }
            //            Array.Sort(items);

            //            for (int n = 0; n < items.Length; n++)
            //            {
            //                int imageindex = 0;

            //                if (items[n].Tag != null)
            //                {
            //                    // Default to method (contains text for parameters)
            //                    imageindex = 2;
            //                    if (items[n].Tag is MemberTypes)
            //                    {
            //                        MemberTypes memberType = (MemberTypes)items[n].Tag;

            //                        switch (memberType)
            //                        {
            //                            case MemberTypes.Custom:
            //                                imageindex = 1;
            //                                break;
            //                            case MemberTypes.Property:
            //                                imageindex = 3;
            //                                break;
            //                            case MemberTypes.Event:
            //                                imageindex = 4;
            //                                break;
            //                        }
            //                    }
            //                }

            //                this._theAutoCompleteListBox.Items.Add(new GListBoxItem(items[n].DisplayText, imageindex));
            //            }
            //        }
            //    }
            }

            return result;
        }

        /// <summary>
        /// Autofills the selected item in the member listbox, by
        /// taking everything before and after the "." in the richtextbox,
        /// and appending the word in the middle.
        /// </summary>
        private void selectItem()
        {
            if (this.wordMatched)
            {
                int selstart = this._theRichTextBox.SelectionStart;
                int prefixend = this._theRichTextBox.SelectionStart - typed.Length;
                int suffixstart = this._theRichTextBox.SelectionStart + typed.Length;

                if (suffixstart >= this._theRichTextBox.Text.Length)
                {
                    suffixstart = this._theRichTextBox.Text.Length;
                }

                string prefix = this._theRichTextBox.Text.Substring(0, prefixend);
                string fill = this._theAutoCompleteListBox.SelectedItem.ToString();
                string suffix = this._theRichTextBox.Text.Substring(suffixstart, this._theRichTextBox.Text.Length - suffixstart);

                this._theRichTextBox.Text = prefix + fill + suffix;
                this._theRichTextBox.SelectionStart = prefix.Length + fill.Length;
            }
        }

        /// <summary>
        /// Searches backwards from the current caret position, until
        /// a space or newline is found.
        /// </summary>
        /// <returns>The previous word from the carret position</returns>
        private string getLastWord()
        {
            string word = "";

            int pos = this._theRichTextBox.SelectionStart;
            if (pos > 1)
            {

                string tmp = "";
                char f = new char();
                while (f != ' ' && f != 10 && pos > 0)
                {
                    pos--;
                    tmp = this._theRichTextBox.Text.Substring(pos, 1);
                    f = (char)tmp[0];
                    word += f;
                }

                char[] ca = word.ToCharArray();
                Array.Reverse(ca);
                word = new String(ca);

            }
            return word.Trim();

        }
    }
}
