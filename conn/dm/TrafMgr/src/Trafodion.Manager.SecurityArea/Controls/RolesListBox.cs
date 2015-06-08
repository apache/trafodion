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
    public class RolesListBoxItem
    {
        private string _theText;
        private int _theImageIndex;
        // properties 
        public string Text
        {
            get { return _theText; }
            set { _theText = value; }
        }
        public int ImageIndex
        {
            get { return _theImageIndex; }
            set { _theImageIndex = value; }
        }
        //constructor
        public RolesListBoxItem(string text, int index)
        {
            _theText = text;
            _theImageIndex = index;
        }
        public RolesListBoxItem(string text) : this(text, -1) { }
        public RolesListBoxItem() : this("") { }
        public override string ToString()
        {
            return _theText;
        }
    }

    public class RolesListBox: ListBox
	{
		private ImageList _theImageList;
		public ImageList ImageList
		{
			get {return _theImageList;}
			set {_theImageList = value;}
		}
        public RolesListBox()
		{
			// Set owner draw mode
			this.DrawMode = DrawMode.OwnerDrawFixed;
		}
        protected override void OnDrawItem(System.Windows.Forms.DrawItemEventArgs e)
        {
            e.DrawBackground();
            e.DrawFocusRectangle();
            RolesListBoxItem item;
            Rectangle bounds = e.Bounds;
            Size imageSize = (_theImageList != null) ? _theImageList.ImageSize : new Size(0, 0);
            try
            {
                item = (RolesListBoxItem)Items[e.Index];
                if (item.ImageIndex != -1)
                {
                    _theImageList.Draw(e.Graphics, bounds.Left, bounds.Top, item.ImageIndex);
                    e.Graphics.DrawString(item.Text, e.Font, new SolidBrush(e.ForeColor),
                        bounds.Left + imageSize.Width, bounds.Top);
                }
                else
                {
                    e.Graphics.DrawString(item.Text, e.Font, new SolidBrush(e.ForeColor),
                        bounds.Left, bounds.Top);
                }
            }
            catch
            {
                if (e.Index != -1)
                {
                    e.Graphics.DrawString(Items[e.Index].ToString(), e.Font,
                        new SolidBrush(e.ForeColor), bounds.Left, bounds.Top);
                }
                else
                {
                    e.Graphics.DrawString(Text, e.Font, new SolidBrush(e.ForeColor),
                        bounds.Left, bounds.Top);
                }
            }
            base.OnDrawItem(e);
        }
    }
}
