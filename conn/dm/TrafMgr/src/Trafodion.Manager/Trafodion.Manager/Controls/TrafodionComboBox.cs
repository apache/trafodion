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
using System.ComponentModel;
using System.Windows.Forms;
using System;

namespace Trafodion.Manager.Framework.Controls
{
  /// <summary>
  /// Wrapper class extending the HPComboBox class. It has hooks to change the look and feel
  /// when the look and feel of the framework is changed.
  /// </summary>
    [ToolboxBitmapAttribute(typeof(ComboBox))]
    public class TrafodionComboBox : ComboBox  
   {
      private TrafodionLookAndFeelChangeHandler lookAndFeelChangeHandler = null;
      public event CancelEventHandler SelectedIndexChanging;

      [Browsable(false)]
      public int LastAcceptedSelectedIndex { get; private set; }

      /// <summary>
      /// Constructor
      /// </summary>
      public TrafodionComboBox()
          : base()
      {
          //Changes the theme when the theme is changed for the framework and
          //also sets the default theme
          lookAndFeelChangeHandler = new TrafodionLookAndFeelChangeHandler(this);
          FlatStyle = FlatStyle.System;
          LastAcceptedSelectedIndex = -1;
      }

      protected void OnSelectedIndexChanging(CancelEventArgs e)
      {
          var selectedIndexChanging = SelectedIndexChanging;
          if (selectedIndexChanging != null)
              selectedIndexChanging(this, e);
      }


      protected override void OnSelectedIndexChanged(EventArgs e)
      {
          if (LastAcceptedSelectedIndex != SelectedIndex)
          {
              var cancelEventArgs = new CancelEventArgs();
              OnSelectedIndexChanging(cancelEventArgs);

              if (!cancelEventArgs.Cancel)
              {
                  LastAcceptedSelectedIndex = SelectedIndex;
                  base.OnSelectedIndexChanged(e);
              }
              else
                  SelectedIndex = LastAcceptedSelectedIndex;
          }
      }
   }
}
