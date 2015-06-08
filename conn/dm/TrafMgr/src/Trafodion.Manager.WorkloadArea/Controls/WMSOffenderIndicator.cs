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
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class WMSOffenderIndicator : TrafodionForm
    {
        public WMSOffenderIndicator()
        {
            InitializeComponent();

            timer1.Tick += new EventHandler(timer1_Tick);
            this.Disposed += new EventHandler(WMSOffenderIndicator_Disposed);
            
            progressBar1.Value = 0;
            progressBar1.Maximum = 30;
            progressBar1.Minimum = 0;
            progressBar1.Step = 1;
            startTimer();

        }

        void WMSOffenderIndicator_Disposed(object sender, EventArgs e)
        {
            stopTimer();
        }

        void timer1_Tick(object sender, EventArgs e)
        {
            progressBar1.PerformStep();
            if (progressBar1.Value == 30)
                progressBar1.Value = 0;
        }

        private void startTimer()
        {
            timer1.Interval = 100;
            if (timer1.Enabled)
            {
                timer1.Stop();
            }
            timer1.Start();
        }

        private void stopTimer()
        {
            if (timer1.Enabled)
            {
                timer1.Stop();
            }
        }

    }
}
