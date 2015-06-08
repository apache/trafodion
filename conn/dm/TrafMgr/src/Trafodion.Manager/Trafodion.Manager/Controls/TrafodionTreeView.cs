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

using System.Drawing;
using System.Windows.Forms;

namespace Trafodion.Manager.Framework.Controls
{
    /// <summary>
    /// Wrapper class extending the TreeView class. It has hooks to change the look and feel
    /// when the look and feel of the framework is changed.
    /// </summary>
    [ToolboxBitmapAttribute(typeof(TreeView))]
    public class TrafodionTreeView : TreeView
    {
        public const string SERVER_ICON = "SERVER_ICON";
        public const string LF_CONNECTED_SERVER_ICON = "LF_CONNECTED_SERVER_ICON";
        public const string CONNECTED_SERVER_ICON = "CONNECTED_SERVER_ICON";
        public const string FOLDER_CLOSED_ICON = "FOLDER_CLOSED_ICON";
        public const string FOLDER_OPEN_ICON = "FOLDER_OPEN_ICON";
        public const string BLANK_DOCUMENT_ICON = "BLANK_DOCUMENT_ICON";

        protected ImageList theImageList;
        private System.ComponentModel.IContainer components;
        private TrafodionLookAndFeelChangeHandler lookAndFeelChangeHandler = null;

        /// <summary>
        /// Constructor
        /// </summary>
        public TrafodionTreeView()
            : base()
        {
            InitializeComponent();
            //Changes the theme when the theme is changed for the framework and
            //also sets the default theme
            lookAndFeelChangeHandler = new TrafodionLookAndFeelChangeHandler(this);
            this.Font = new Font("Tahoma", 8.25F, FontStyle.Regular);

            theImageList.Images.Add(SERVER_ICON, Properties.Resources.ServerIcon);
            theImageList.Images.Add(LF_CONNECTED_SERVER_ICON, Properties.Resources.ServerLFConnectedIcon);
            theImageList.Images.Add(CONNECTED_SERVER_ICON, Properties.Resources.ServerConnectedIcon);
            theImageList.Images.Add(FOLDER_CLOSED_ICON, Properties.Resources.FolderClosedIcon);
            theImageList.Images.Add( FOLDER_OPEN_ICON, Properties.Resources.FolderOpenIcon);
            theImageList.Images.Add( BLANK_DOCUMENT_ICON, Properties.Resources.BlankDocumentIcon);
        
            this.ImageKey = BLANK_DOCUMENT_ICON;
            this.SelectedImageKey = BLANK_DOCUMENT_ICON;
        }

        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.theImageList = new System.Windows.Forms.ImageList(this.components);
            this.SuspendLayout();
            // 
            // theImageList
            // 
            this.theImageList.ColorDepth = System.Windows.Forms.ColorDepth.Depth8Bit;
            this.theImageList.ImageSize = new System.Drawing.Size(16, 16);
            this.theImageList.TransparentColor = System.Drawing.Color.Transparent;
            // 
            // TrafodionTreeView
            // 
            this.ImageIndex = 0;
            this.ImageList = this.theImageList;
            this.LineColor = System.Drawing.Color.Black;
            this.SelectedImageIndex = 0;
            this.ResumeLayout(false);

        }

        public TreeNode FindChildNode(TreeNode aNode, string text)
        {
            if (aNode != null)
            {
                foreach (TreeNode node in aNode.Nodes)
                {
                    if (node.Text.Equals(text))
                        return node;
                }
            }
            return null;
        }
    }
}
