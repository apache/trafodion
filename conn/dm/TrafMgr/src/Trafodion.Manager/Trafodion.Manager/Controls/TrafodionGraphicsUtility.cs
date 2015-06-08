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

namespace Trafodion.Manager.Framework.Controls
{
  /// <summary>
  /// Wrapper class extending the GraphicsUtility class. It has hooks to change the look and feel
  /// when the look and feel of the framework is changed.
  /// </summary>
  public  class TrafodionGraphicsUtility 
   {
      private TrafodionLookAndFeelChangeHandler lookAndFeelChangeHandler = null;

      public static readonly Color neutralMediumLight = Color.FromArgb(0xff, 0xcc, 0xcc, 0xcc);
      public static readonly Color neutralMediumDark = Color.FromArgb(0xff, 0x66, 0x66, 0x66);
      public enum Direction
      {
          NORTH,
          EAST,
          SOUTH,
          WEST
      }
 
      /// <summary>
      /// Constructor
      /// </summary>
      public TrafodionGraphicsUtility()
      {
          //Changes the theme when the theme is changed for the framework and
          //also sets the default theme
          lookAndFeelChangeHandler = new TrafodionLookAndFeelChangeHandler(this);
      }

      public static double CenterOffset(int space, int size)
      {
          return (double)((space / 2) - (size / 2));
      }

      public static Point CenterOffsetPoint(Size space, Size size)
      {
          int x = (int)CenterOffset(space.Width, size.Width);
          return new Point(x, (int)CenterOffset(space.Height, size.Height));
      }

      public static void DrawArrow(Graphics g, Color color, Direction dir, int x, int y)
      {
          DrawArrow(g, color, dir, x, y, 7, 4);
      }
      public static void DrawArrow(Graphics g, Color color, Direction dir, int x, int y, int width, int height)
      {
          Point[] points = new Point[3];
          points[0] = new Point(x, y);
          switch (dir)
          {
              case Direction.NORTH:
                  points[1] = new Point((x + width) + 2, y);
                  points[2] = new Point((x + (width / 2)) + 1, (y - height) - 1);
                  break;

              case Direction.EAST:
                  points[1] = new Point(x, (y + width) + 1);
                  points[2] = new Point(x + height, (y + (width / 2)) + 1);
                  break;

              case Direction.SOUTH:
                  points[1] = new Point(x + width, y);
                  points[2] = new Point(x + (width / 2), y + height);
                  break;

              case Direction.WEST:
                  points[1] = new Point(x, (y + width) + 1);
                  points[2] = new Point(x - height, (y + (width / 2)) + 1);
                  break;
          }
          SolidBrush brush = new SolidBrush(color);
          g.FillPolygon(brush, points);
          brush.Dispose();
      }
   }
}
