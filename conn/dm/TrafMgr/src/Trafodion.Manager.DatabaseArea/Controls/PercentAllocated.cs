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
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    public class PercentAllocatedColumn : DataGridViewColumn
    {
        public PercentAllocatedColumn()
            : base(new PercentAllocatedCell())
        {
        }

        public override DataGridViewCell CellTemplate
        {
            get
            {
                return base.CellTemplate;
            }
            set
            {
                // Ensure that the cell used for the template is a PercentAllocatedCell.
                if (value != null &&
                    !value.GetType().IsAssignableFrom(typeof(PercentAllocatedCell)))
                {
                    throw new InvalidCastException("Must be a PercentAllocatedCell");
                }
                base.CellTemplate = value;
            }
        }
    }

    public class PercentAllocatedCell : DataGridViewTextBoxCell
    {

        public PercentAllocatedCell()
            : base()
        {
            this.ReadOnly = false;
        }

        public override void InitializeEditingControl(int rowIndex, 
            object initialFormattedValue, 
            DataGridViewCellStyle dataGridViewCellStyle)
        {
            // Set the value of the editing control to the current cell value.
            base.InitializeEditingControl(rowIndex, 
                initialFormattedValue,
                dataGridViewCellStyle);

            PercentAllocated ctl = DataGridView.EditingControl as PercentAllocated;
            if(ctl != null)
            {
                ctl.Value = (int)this.Value;
            }

        }

        public override Type EditType
        {
            get
            {
                // Return the type of the editing contol that CalendarCell uses.
                return typeof(PercentAllocated);
            }
        }

        public override Type ValueType
        {
            get
            {
                // Return the type of the value that CalendarCell contains.
                return typeof(PercentAllocated);
            }
        }

        public override object DefaultNewRowValue
        {
            get
            {
                // Use the current date and time as the default value.
                return new PercentAllocated(0);
            }
        }
    }


    public partial class PercentAllocated : UserControl, IDataGridViewEditingControl
    {
        int rowIndex;
        int _thePercentAllocated;
        private bool valueChanged = false;
        DataGridView dataGridView;

        public PercentAllocated():this(0)
        {
            Init(0);
        }

        public PercentAllocated(int aPercentAllocated)
        {
            Init(aPercentAllocated);
        }

        private void Init(int aPercentAllocated)
        {
            _thePercentAllocated = aPercentAllocated;
            InitializeComponent();
            this._percentAllocatedProgress.Maximum = 100;
            this._percentAllocatedProgress.Minimum = 0;
            this._percentAllocatedProgress.Value = aPercentAllocated;
            this._percentAllocatedLbl.Text = aPercentAllocated + "%";
        }


        public int Value
        {
            get { return this._percentAllocatedProgress.Value; }
            set 
            { 
                this._percentAllocatedProgress.Value = value;
                valueChanged = true;
                this.EditingControlDataGridView.NotifyCurrentCellDirty(true);
            //base.OnValueChanged(eventargs);
            }
        }


        // Implements the IDataGridViewEditingControl.EditingControlFormattedValue 
        // property.
        public object EditingControlFormattedValue
        {
            get
            {
                return _thePercentAllocated;
            }
            set
            {
                if (value is int)
                {
                    this.Value = (int)value;
                }                
            }
        }

        // Implements the 
        // IDataGridViewEditingControl.GetEditingControlFormattedValue method.
        public object GetEditingControlFormattedValue(
            DataGridViewDataErrorContexts context)
        {
            return _thePercentAllocated;
        }

        // Implements the 
        // IDataGridViewEditingControl.ApplyCellStyleToEditingControl method.
        public void ApplyCellStyleToEditingControl(
            DataGridViewCellStyle dataGridViewCellStyle)
        {
        }

        // Implements the IDataGridViewEditingControl.EditingControlRowIndex 
        // property.
        public int EditingControlRowIndex
        {
            get
            {
                return rowIndex;
            }
            set
            {
                rowIndex = value;
            }
        }

        // Implements the IDataGridViewEditingControl.EditingControlWantsInputKey 
        // method.
        public bool EditingControlWantsInputKey(
            Keys key, bool dataGridViewWantsInputKey)
        {
            return true;
        }

        // Implements the IDataGridViewEditingControl.PrepareEditingControlForEdit 
        // method.
        public void PrepareEditingControlForEdit(bool selectAll)
        {
            // No preparation needs to be done.
        }

        // Implements the IDataGridViewEditingControl
        // .RepositionEditingControlOnValueChange property.
        public bool RepositionEditingControlOnValueChange
        {
            get
            {
                return false;
            }
        }

        // Implements the IDataGridViewEditingControl
        // .EditingControlDataGridView property.
        public DataGridView EditingControlDataGridView
        {
            get
            {
                return dataGridView;
            }
            set
            {
                dataGridView = value;
            }
        }

        // Implements the IDataGridViewEditingControl
        // .EditingControlValueChanged property.
        public bool EditingControlValueChanged
        {
            get
            {
                return valueChanged;
            }
            set
            {
                valueChanged = value;
            }

        }

        // Implements the IDataGridViewEditingControl
        // .EditingPanelCursor property.
        public Cursor EditingPanelCursor
        {
            get
            {
                return base.Cursor;
            }
        }

        //protected override void OnValueChanged(EventArgs eventargs)
        //{
        //    // Notify the DataGridView that the contents of the cell
        //    // have changed.
        //    valueChanged = true;
        //    this.EditingControlDataGridView.NotifyCurrentCellDirty(true);
        //    base.OnValueChanged(eventargs);
        //}


    }
}
