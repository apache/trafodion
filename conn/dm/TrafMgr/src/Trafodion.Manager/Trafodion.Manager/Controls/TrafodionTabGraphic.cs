
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

namespace Trafodion.Manager.Framework.Controls
{
    public partial class TrafodionTabGraphic : UserControl
    {
        private Color ROLL_OVER_COL = Color.FromArgb(241, 241, 241);
        private Color CLICK_COL = Color.FromArgb(230, 230, 230);

        public TrafodionTabGraphic()
        {
            InitializeComponent();
        }

        public void SetMouseClickGraphics()
        {
            this.panel1.BackColor = this.CLICK_COL;
            this.pictureBox8.Image = global::Trafodion.Manager.Properties.Resources.Right;
            this.pictureBox7.Image = global::Trafodion.Manager.Properties.Resources.Left;
            this.pictureBox5.Image = global::Trafodion.Manager.Properties.Resources.TopLeft2;
            this.pictureBox6.Image = global::Trafodion.Manager.Properties.Resources.TopTop2;
            this.pictureBox4.Image = global::Trafodion.Manager.Properties.Resources.TopRight2;
        }

        public void SetMouseOverGraphics()
        {
            this.panel1.BackColor = this.ROLL_OVER_COL;
            this.pictureBox8.Image = global::Trafodion.Manager.Properties.Resources.Right1;
            this.pictureBox7.Image = global::Trafodion.Manager.Properties.Resources.Left4;
            this.pictureBox5.Image = global::Trafodion.Manager.Properties.Resources.TopLeft22;
            this.pictureBox6.Image = global::Trafodion.Manager.Properties.Resources.TopTop22;
            this.pictureBox4.Image = global::Trafodion.Manager.Properties.Resources.TopRight22;
        }

    }
}

