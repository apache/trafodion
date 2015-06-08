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
using System.Drawing;
using System.Windows.Forms;

namespace Trafodion.Manager.Framework.Controls
{
  /// <summary>
  /// Wrapper class extending the HPRichTextBox class. It has hooks to change the look and feel
  /// when the look and feel of the framework is changed.
  /// </summary>
    [ToolboxBitmapAttribute(typeof(RichTextBox))]
    [DefaultPropertyAttribute("Text")]
    public class TrafodionRichTextBox : RichTextBox  
    {
        private TrafodionLookAndFeelChangeHandler lookAndFeelChangeHandler = null;
        private System.Windows.Forms.ToolStripMenuItem contextSelectAll = new System.Windows.Forms.ToolStripMenuItem();
        private System.Windows.Forms.ToolStripMenuItem contextCut = new System.Windows.Forms.ToolStripMenuItem();
        private System.Windows.Forms.ToolStripMenuItem contextCopy = new System.Windows.Forms.ToolStripMenuItem();
        private System.Windows.Forms.ToolStripMenuItem contextPaste = new System.Windows.Forms.ToolStripMenuItem();
        private System.Windows.Forms.ToolStripMenuItem contextUndo = new System.Windows.Forms.ToolStripMenuItem();
        private System.Windows.Forms.ToolStripMenuItem contextRedo = new System.Windows.Forms.ToolStripMenuItem();
        private System.Windows.Forms.ToolStripMenuItem contextWordWrap = new System.Windows.Forms.ToolStripMenuItem();
        static private Font regularFont = new Font("Tahoma", 8.25F, FontStyle.Regular);
        static private Size regularSize = new System.Drawing.Size(117, 22);

      /// <summary>
      /// Constructor
      /// </summary>
      public TrafodionRichTextBox() : base()
      {
          //Changes the theme when the theme is changed for the framework and
          //also sets the default theme
          lookAndFeelChangeHandler = new TrafodionLookAndFeelChangeHandler(this);
          //Font = new Font("Tahoma", 8.25F, FontStyle.Regular);
          Font = regularFont;
          ContextMenuStrip = new TrafodionContextMenuStrip();
          ContextMenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.contextUndo,
            this.contextRedo,
            new System.Windows.Forms.ToolStripSeparator(),
            this.contextCut,
            this.contextCopy,
            this.contextPaste,
            new System.Windows.Forms.ToolStripSeparator(),
            this.contextSelectAll,
            new System.Windows.Forms.ToolStripSeparator(),
            this.contextWordWrap
          });
          ContextMenuStrip.Name = "contextMenuStrip1";
          ContextMenuStrip.Opening += new System.ComponentModel.CancelEventHandler(this.contextMenuStrip_Opening);
          // 
          // contextSelectAll
          // 
          this.contextSelectAll.Name = "contextSelectAll";
          this.contextSelectAll.Size = regularSize; // new System.Drawing.Size(117, 22);
          this.contextSelectAll.Text = global::Trafodion.Manager.Properties.Resources.MenuSelectAll;
          this.contextSelectAll.Visible = false;
          this.contextSelectAll.Click += new System.EventHandler(this.contextSelectAll_Click);
          // 
          // contextCut
          // 
          this.contextCut.Name = "contextCut";
          this.contextCut.Size = regularSize; // new System.Drawing.Size(117, 22);
          this.contextCut.Text = global::Trafodion.Manager.Properties.Resources.MenuCut;
          this.contextCut.Visible = false;
          this.contextCut.Click += new System.EventHandler(this.contextCut_Click);
          // 
          // contextCopy
          // 
          this.contextCopy.Name = "contextCopy";
          this.contextCopy.Size = regularSize; // new System.Drawing.Size(117, 22);
          this.contextCopy.Text = global::Trafodion.Manager.Properties.Resources.MenuCopy;
          this.contextCopy.Visible = false;
          this.contextCopy.Click += new System.EventHandler(this.contextCopy_Click);
          // 
          // contextPaste
          // 
          this.contextPaste.Name = "contextPaste";
          this.contextPaste.Size = regularSize; // new System.Drawing.Size(117, 22);
          this.contextPaste.Text = global::Trafodion.Manager.Properties.Resources.MenuPaste;
          this.contextPaste.Visible = false;
          this.contextPaste.Click += new System.EventHandler(this.contextPaste_Click);
          // 
          // contextUndo
          // 
          this.contextUndo.Name = "contextUndo";
          this.contextUndo.Size = regularSize; // new System.Drawing.Size(117, 22);
          this.contextUndo.Text = global::Trafodion.Manager.Properties.Resources.MenuUndo;
          this.contextUndo.Visible = false;
          this.contextUndo.Click += new System.EventHandler(this.contextUndo_Click);
          // 
          // contextRedo
          // 
          this.contextRedo.Name = "contextUndo";
          this.contextRedo.Size = regularSize; // new System.Drawing.Size(117, 22);
          this.contextRedo.Text = global::Trafodion.Manager.Properties.Resources.MenuRedo;
          this.contextRedo.Visible = false;
          this.contextRedo.Click += new System.EventHandler(this.contextRedo_Click);
          // 
          // wrapOnOff
          // 
          this.contextWordWrap.Name = "contextWordWrap";
          this.contextWordWrap.Size = regularSize; // new System.Drawing.Size(117, 22);
          this.contextWordWrap.Text = global::Trafodion.Manager.Properties.Resources.MenuWordWrap;
          this.contextWordWrap.CheckOnClick = true;
          this.contextWordWrap.Checked = false;
          this.contextWordWrap.Visible = true;
          this.contextWordWrap.Click += new System.EventHandler(this.wrapOnOff_Click);
      }

      private void contextMenuStrip_Opening(object sender, CancelEventArgs e)
      {
          this.Focus();

          contextSelectAll.Visible = true;
          contextCopy.Visible = true;
          contextCut.Visible = true;
          contextPaste.Visible = true;
          contextCut.Visible = true;
          contextUndo.Visible = true;
          contextRedo.Visible = true;
          //contextSeparator1.Visible = true;

          if (this.Text.Length > 0)
          {
              contextSelectAll.Enabled = true;
              contextCopy.Enabled = true;

              if (this.SelectionLength > 0)
              {
                  contextCut.Enabled = true;
                  contextCopy.Enabled = true;
              }
              else
              {
                  contextCut.Enabled = false;
                  contextCopy.Enabled = false;
              }
          }
          else
          {
              contextSelectAll.Enabled = false;
              contextCopy.Enabled = false;
              contextCut.Enabled = false;
          }

          if (this.CanUndo)
              contextUndo.Enabled = true;
          else
              contextUndo.Enabled = false; 
          
          if (this.CanRedo)
              contextRedo.Enabled = true;
          else
              contextRedo.Enabled = false;

          if (System.Windows.Forms.Clipboard.ContainsText())
              contextPaste.Enabled = true;
          else
              contextPaste.Enabled = false;
      }

      private void contextSelectAll_Click(object sender, EventArgs e)
      {
          this.SelectAll();
      }


      private void contextCopy_Click(object sender, EventArgs e)
      {
            this.Copy();
      }

      private void contextPaste_Click(object sender, EventArgs e)
      {
          DataFormats.Format textFormat = DataFormats.GetFormat(DataFormats.Text);
          if (this.CanPaste(textFormat))
          {
              this.Paste(textFormat);
          }
      }


      protected override void OnKeyDown(KeyEventArgs e)
      {
          if (e.Control && e.KeyCode == Keys.V || e.Shift && e.KeyCode == Keys.I)
          {
              if (Clipboard.ContainsImage())
              {
                  e.Handled = true;
                  return;
              }
          }
          base.OnKeyDown(e);
      }

      protected override void OnPreviewKeyDown(PreviewKeyDownEventArgs e)
      {
          if (e.Control && e.KeyCode == Keys.V || e.Shift && e.KeyCode == Keys.I)
          {
              if (Clipboard.ContainsImage())
              {
                  e.IsInputKey = true;
                  return;
              }
          }
          base.OnPreviewKeyDown(e);
      }


      private void contextUndo_Click(object sender, EventArgs e)
      {
          this.Undo();
      }

      private void contextRedo_Click(object sender, EventArgs e)
      {
          this.Redo();
      }

      private void contextCut_Click(object sender, EventArgs e)
      {
          this.Cut();
      }

      private void wrapOnOff_Click(object sender, EventArgs e)
      {
          this.WordWrap = contextWordWrap.Checked;
      }

   }
}
