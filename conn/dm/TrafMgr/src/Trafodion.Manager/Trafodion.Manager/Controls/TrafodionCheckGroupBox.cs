// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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
using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
using System.Drawing;

namespace Trafodion.Manager.Framework.Controls
{
    public class TrafodionCheckGroupBox :GroupBox
    {
        public event EventHandler CheckBoxCheckedChanged;
        private CheckBox withEventsField_m_CheckBox;
        private bool m_EnableDisableInnerControls=false;
	  private CheckBox m_CheckBox {
		get { return withEventsField_m_CheckBox; }
		set {
			if (withEventsField_m_CheckBox != null) {
				withEventsField_m_CheckBox.CheckedChanged -= m_CheckBox_CheckedChanged;
				withEventsField_m_CheckBox.Layout -= m_CheckBox_Layout;
			}
			withEventsField_m_CheckBox = value;
			if (withEventsField_m_CheckBox != null) {
				withEventsField_m_CheckBox.CheckedChanged += m_CheckBox_CheckedChanged;
				withEventsField_m_CheckBox.Layout += m_CheckBox_Layout;
			}
		}

	}
	// Add the CheckBox to the control.
      public TrafodionCheckGroupBox()
          : base()
	{

		m_CheckBox = new CheckBox();
		m_CheckBox.Location = new Point(8, 0);
		this.Text = "CheckGroup";

		this.Controls.Add(m_CheckBox);
	}

	// Keep the CheckBox text synced with our text.
	public override string Text {
		get { return base.Text; }
		set {
			base.Text = value;
			m_CheckBox.Text = value;

			Graphics gr = this.CreateGraphics();
			SizeF s = gr.MeasureString(base.Text, this.Font);
            int iWidthOffset = 20;
            if (this.Font.Bold)
            {
                iWidthOffset = 40;                
            }
            m_CheckBox.Size = new Size((int)s.Width + iWidthOffset, (int)s.Height + 5);
		}
	}

	// Delegate to CheckBox.Checked.
	public bool Checked {
		get { return m_CheckBox.Checked; }
		set { m_CheckBox.Checked = value; }
	}

    public bool EnableDisableInnerControls
    {
        get { return m_EnableDisableInnerControls; }
        set
        {
            m_EnableDisableInnerControls = value;
        }
    }

	// Enable/disable contained controls.
	private void EnableDisableControls()
	{
        if (!EnableDisableInnerControls) return;
		foreach (Control ctl in this.Controls) {
			if ((!object.ReferenceEquals(ctl, m_CheckBox))) {
				try {
					ctl.Enabled = m_CheckBox.Checked;
				} catch (Exception ex) {
				}
			}
		}
	}

	// Enable/disable contained controls.
	private void m_CheckBox_CheckedChanged(object sender, System.EventArgs e)
	{
		EnableDisableControls();
        if (CheckBoxCheckedChanged != null)
        {
            CheckBoxCheckedChanged(sender, e);
        }
	}

	// Enable/disable contained controls.
	// We do this here to set editability
	// when the control is first loaded.
	private void m_CheckBox_Layout(object sender, System.Windows.Forms.LayoutEventArgs e)
	{
		EnableDisableControls();
	}

    }
}
