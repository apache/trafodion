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
using System.Collections.Generic;
using System.Text;
using Trafodion.Manager.Framework.Controls;
using System.Windows.Forms;

namespace Trafodion.Manager.Framework.Controls
{
    public class TrafodionVerticalProgressBar : TrafodionProgressBar
    {
        /*
         * Progress bar constants from commctrl.h header file. Interface for common windows controls.
         */
        private const int PBS_SMOOTH = 0x01;
        private const int PBS_VERTICAL = 0x04;
        private const int PBS_MARQUEE = 0x08;
        private const int PBS_SMOOTHREVERSE = 0x10;

        public TrafodionVerticalProgressBar()
        {
            base.Minimum = 0;
            base.Maximum = 100;
            base.Step = 5;
            base.Value = 0;
        }

        protected override CreateParams CreateParams
        {
            get
            {
                CreateParams sbParams = base.CreateParams;
                sbParams.Style = sbParams.Style | PBS_VERTICAL; // | PBS_SMOOTH;
                return sbParams;
            }

        }
    }
}
