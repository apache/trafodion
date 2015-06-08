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
using System.Reflection;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace Trafodion.Manager.Framework.Controls
{
  /// <summary>
  /// Wrapper class extending the SplitContainer class. It has hooks to change the look and feel
  /// when the look and feel of the framework is changed.
  /// </summary>
    [ToolboxBitmapAttribute(typeof(SplitContainer))]
    public class TrafodionSplitContainer : SplitContainer  
   {
      private TrafodionLookAndFeelChangeHandler lookAndFeelChangeHandler = null;

      // Fields
      private Rectangle leftbutton;
      private static int mywidth = 9;
      private Rectangle rightbutton;
      private int splitterposition = -1;
 
       /// <summary>
      /// Constructor
      /// </summary>
      public TrafodionSplitContainer()
          : base()
      {
          base.SetStyle(ControlStyles.DoubleBuffer | ControlStyles.AllPaintingInWmPaint | ControlStyles.UserPaint, true);
          this.SplitterWidth = mywidth;
          base.Click += new EventHandler(this.closeside);
          base.MouseUp += new MouseEventHandler(this.redrawleft);
          base.ResizeRedraw = true;
          base.BorderStyle = BorderStyle.None;

          //Changes the theme when the theme is changed for the framework and
          //also sets the default theme
          lookAndFeelChangeHandler = new TrafodionLookAndFeelChangeHandler(this);
      }

      private void closeside(object sender, EventArgs e)
      {
          Point pt = base.PointToClient(Cursor.Position);
          if (this.leftbutton.Contains(pt))
          {
              this.splitterposition = base.Panel1MinSize;
              base.SplitterDistance = this.splitterposition;
          }
          else if (this.rightbutton.Contains(pt))
          {
              int num = (base.Orientation == Orientation.Vertical) ? base.Width : (base.Height - 0x19);
              this.splitterposition = num - base.Panel2MinSize;
              base.SplitterDistance = this.splitterposition;
          }
      }

      [DllImport("user32.dll")]
      private static extern int GetSystemMetrics(int nIndex);
      protected override void OnPaint(PaintEventArgs e)
      {
          int num;
          int num2;
          int num6;
          int num7;
          int num10;
          Control control;
          PropertyInfo property;
          Graphics g = e.Graphics;
          bool flag = base.Orientation == Orientation.Vertical;
          if (this.splitterposition > -1)
          {
              base.SplitterDistance = this.splitterposition;
              this.splitterposition = -1;
          }
          if (!(flag || (this.SplitterWidth == mywidth)))
          {
              this.SplitterWidth = mywidth;
          }
          Rectangle splitterRectangle = base.SplitterRectangle;
          SolidBrush brush = new SolidBrush(base.Parent.BackColor);
          g.FillRectangle(brush, base.SplitterRectangle);
          brush.Dispose();
          Pen pen = new Pen(TrafodionGraphicsUtility.neutralMediumLight);
          Rectangle rect = splitterRectangle;
          if (flag)
          {
              rect.X++;
              rect.Width -= 3;
              rect.Height--;
          }
          else
          {
              rect.Y++;
              rect.Width--;
              rect.Height -= 3;
          }
          g.DrawRectangle(pen, rect);
          if (flag)
          {
              num = 0x12;
              num2 = mywidth - 3;
              this.leftbutton = new Rectangle(new Point(rect.X, 0), new Size(num2, num));
              this.rightbutton = new Rectangle(new Point(rect.X, num), new Size(num2, num));
              g.DrawLine(pen, new Point(this.leftbutton.Left, this.leftbutton.Bottom), new Point(this.leftbutton.Right, this.leftbutton.Bottom));
              g.DrawLine(pen, new Point(this.rightbutton.Left, this.rightbutton.Bottom), new Point(this.rightbutton.Right, this.rightbutton.Bottom));
          }
          else
          {
              num = mywidth - 3;
              num2 = 0x12;
              this.leftbutton = new Rectangle(new Point(0, rect.Y), new Size(num2, num));
              this.rightbutton = new Rectangle(new Point(num2, rect.Y), new Size(num2, num));
              g.DrawLine(pen, new Point(this.leftbutton.Right, this.leftbutton.Top), new Point(this.leftbutton.Right, this.leftbutton.Bottom));
              g.DrawLine(pen, new Point(this.rightbutton.Right, this.rightbutton.Top), new Point(this.rightbutton.Right, this.rightbutton.Bottom));
          }
          pen.Dispose();
          int width = 5;
          int height = 3;
          if (flag)
          {
              TrafodionGraphicsUtility.DrawArrow(g, TrafodionGraphicsUtility.neutralMediumDark, TrafodionGraphicsUtility.Direction.WEST, this.leftbutton.Left + 5, this.leftbutton.Top + 6, width, height);
              TrafodionGraphicsUtility.DrawArrow(g, TrafodionGraphicsUtility.neutralMediumDark, TrafodionGraphicsUtility.Direction.EAST, this.rightbutton.Left + 2, this.rightbutton.Top + 6, width, height);
          }
          else
          {
              TrafodionGraphicsUtility.DrawArrow(g, TrafodionGraphicsUtility.neutralMediumDark, TrafodionGraphicsUtility.Direction.NORTH, this.leftbutton.Left + 6, this.leftbutton.Top + 5, width, height);
              TrafodionGraphicsUtility.DrawArrow(g, TrafodionGraphicsUtility.neutralMediumDark, TrafodionGraphicsUtility.Direction.SOUTH, this.rightbutton.Left + 7, this.rightbutton.Top + 2, width, height);
          }
          Pen pen2 = new Pen(TrafodionGraphicsUtility.neutralMediumDark);
          int size = 0x18;
          if (flag)
          {
              num6 = (int)TrafodionGraphicsUtility.CenterOffset(rect.Height, size);
              num7 = (rect.X + (rect.Width / 2)) - 1;
              int x = num7 + 2;
              if (num6 > this.rightbutton.Bottom)
              {
                  g.DrawLine(pen2, new Point(num7, num6), new Point(num7, num6 + size));
                  g.DrawLine(pen2, new Point(x, num6), new Point(x, num6 + size));
              }
          }
          else
          {
              num6 = (rect.Y + (rect.Height / 2)) - 1;
              num7 = (int)TrafodionGraphicsUtility.CenterOffset(rect.Width, size);
              int y = num6 + 2;
              if (num7 > this.rightbutton.Right)
              {
                  g.DrawLine(pen2, new Point(num7, num6), new Point(num7 + size, num6));
                  g.DrawLine(pen2, new Point(num7, y), new Point(num7 + size, y));
              }
          }
          pen2.Dispose();
          for (num10 = 0; num10 < base.Panel1.Controls.Count; num10++)
          {
              control = base.Panel1.Controls[num10];
              try
              {
                  property = control.GetType().GetProperty("BorderStyle");
                  if (!((property == null) || property.GetValue(control, null).ToString().Equals("None")))
                  {
                      property.SetValue(control, BorderStyle.None, null);
                  }
              }
              catch
              {
              }
          }
          for (num10 = 0; num10 < base.Panel2.Controls.Count; num10++)
          {
              control = base.Panel2.Controls[num10];
              try
              {
                  property = control.GetType().GetProperty("BorderStyle");
                  if (!((property == null) || property.GetValue(control, null).ToString().Equals("None")))
                  {
                      property.SetValue(control, BorderStyle.None, null);
                  }
              }
              catch
              {
              }
          }
      }

      private void redrawleft(object sender, MouseEventArgs e)
      {
          base.Panel1.Invalidate();
      }

      [Description("The splitter width"), Category("Appearance")]
      public int SplitterWidth
      {
          get
          {
              return mywidth;
          }
          set
          {
              base.SplitterWidth = mywidth;
          }
      }

      protected override void OnSizeChanged(EventArgs e)
      {
          try
          {
              if (this.IsHandleCreated)
              {
                  this.BeginInvoke((MethodInvoker)delegate
                  {
                      base.OnSizeChanged(e);
                  });
              }
          }
          catch (Exception ex)
          {
          }

      }
   }
}
