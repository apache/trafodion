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
  /// Wrapper class extending the HPSplitter class. It has hooks to change the look and feel
  /// when the look and feel of the framework is changed.
  /// </summary>
  [ToolboxBitmapAttribute(typeof(Splitter))]
  public  class TrafodionSplitter : Splitter  
   {
      private TrafodionLookAndFeelChangeHandler lookAndFeelChangeHandler = null;

      /// <summary>
      /// Constructor
      /// </summary>
      public TrafodionSplitter()
          : base()
      {
          //Changes the theme when the theme is changed for the framework and
          //also sets the default theme
          lookAndFeelChangeHandler = new TrafodionLookAndFeelChangeHandler(this);
      }

   }
}
