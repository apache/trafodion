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
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.Framework
{
    public class TrafodionChangeTracker
    {
          public delegate void ChangeDetected(object sender, EventArgs e);
          public event ChangeDetected OnChangeDetected;

          private Control _formTracked;
          private TrafodionControlChangeTrackerCollection _controlsTracked;
         

          // initialize in the constructor by assigning controls to track
          public TrafodionChangeTracker(Control parentControl)
          {
              _formTracked = parentControl;
              _controlsTracked = new TrafodionControlChangeTrackerCollection(parentControl);
              _controlsTracked.OnChangeDetected = OnChangeDetectedImpl;
              EnableChangeEvents = false;
          }

          public void RemoveChangeHandlers()
          {
              if (_formTracked != null)
              {
                  _controlsTracked.RemoveHandlersFromCollection(_formTracked.Controls);
              }
          }

          // property denoting whether the tracked control is clean or dirty;
          // used if the full list of dirty controls isn't necessary
          public bool IsDirty
          {
              get 
              {
                  List<Control> dirtyControls
                      = _controlsTracked.GetListOfDirtyControls();

                  return (dirtyControls.Count > 0);
              }
          }

          public bool EnableChangeEvents
          {
              get { return _controlsTracked.FireChangeEvents; }
              set { _controlsTracked.FireChangeEvents = value; }
          }

          // public method for accessing the list of currently
          // "dirty" controls
          public List<Control> GetListOfDirtyControls()
          {
              return _controlsTracked.GetListOfDirtyControls();
          }

          
          // establish the form as "clean" with whatever current
          // control values exist
          public void MarkAsClean()
          {
              _controlsTracked.MarkAllControlsAsClean();
          }

          private void OnChangeDetectedImpl(object sender, EventArgs e)
          {
              if  (OnChangeDetected != null)
              {
                  OnChangeDetected(sender, e);
              }
          }

    }


    public class TrafodionControlChangeTrackerCollection : List<TrafodionControlChangeTracker>
    {
        private bool _changeDetected = false;
        private bool _fireChangeEvents = true;

        public bool FireChangeEvents
        {
            get { return _fireChangeEvents; }
            set { _fireChangeEvents = value; }
        }
        public TrafodionChangeTracker.ChangeDetected OnChangeDetected;

        // constructors
        public TrafodionControlChangeTrackerCollection() : base() { }
        public TrafodionControlChangeTrackerCollection(Control parentControl)
            : base()
        {
            AddControlsFromContainer(parentControl);
        }


        // utility method to add the controls from a Form to this collection
        public void AddControlsFromContainer(Control parentControl)
        {
            this.Clear();
            AddControlsFromCollection(parentControl.Controls);
        }


        // recursive routine to inspect each control and add to the 
        // collection accordingly
        public void AddControlsFromCollection(Control.ControlCollection coll)
        {
            foreach (Control c in coll)
            {
                //Assign change handler
                AssignHandler(c);

                // if the control is supported for dirty tracking, add it
                if (TrafodionControlChangeTracker.IsControlTypeSupported(c))
                    this.Add(new TrafodionControlChangeTracker(c));

                // recurively apply to inner collections
                if (c.HasChildren)
                    AddControlsFromCollection(c.Controls);
            }
        }

        // loop through all controls and return a list of those that are dirty
        public List<Control> GetListOfDirtyControls()
        {
            List<Control> list = new List<Control>();

            foreach (TrafodionControlChangeTracker c in this)
            {
                if (c.DetermineIfDirty())
                    list.Add(c.Control);
            }

            return list;
        }


        // mark all the tracked controls as clean
        public void MarkAllControlsAsClean()
        {
            _changeDetected = false;
            foreach (TrafodionControlChangeTracker c in this)
                c.EstablishValueAsClean();
        }


        private void doChangeDetected(object sender, EventArgs e)
        {
            _changeDetected = true;
            if (_fireChangeEvents && (OnChangeDetected != null))
            {
                OnChangeDetected(sender, e);
            }            
        }

        // event handlers
        private void TrafodionChangeTracker_InputChanged(object sender, EventArgs e)
        {
            doChangeDetected(sender, e);
        }


        // recursive routine to inspect each control and assign handlers accordingly
        private void AssignHandler(Control aControl)
        {
            if (aControl is TextBox)
                (aControl as TextBox).TextChanged += TrafodionChangeTracker_InputChanged;

            if (aControl is CheckBox)
                (aControl as CheckBox).CheckedChanged += TrafodionChangeTracker_InputChanged;

            if (aControl is RadioButton)
                (aControl as RadioButton).CheckedChanged += TrafodionChangeTracker_InputChanged;

            if (aControl is NumericUpDown)
                (aControl as NumericUpDown).ValueChanged += TrafodionChangeTracker_InputChanged;

            if (aControl is RichTextBox)
                (aControl as RichTextBox).TextChanged += TrafodionChangeTracker_InputChanged;

            if (aControl is ComboBox)
                (aControl as ComboBox).SelectedIndexChanged += TrafodionChangeTracker_InputChanged;

            if (aControl is ComboBox)
                (aControl as ComboBox).TextChanged += TrafodionChangeTracker_InputChanged;

            if (aControl is ListBox)
                (aControl as ListBox).SelectedValueChanged += TrafodionChangeTracker_InputChanged;

            if (aControl is DateTimePicker)
                (aControl as DateTimePicker).ValueChanged += TrafodionChangeTracker_InputChanged;

            if (aControl is TrafodionIGrid)
            {
                (aControl as TrafodionIGrid).SelectionChanged += TrafodionChangeTracker_InputChanged;
                (aControl as TrafodionIGrid).AfterCommitEdit += TrafodionChangeTracker_InputChanged;
            }

            // ... apply for other desired input types similarly ...

        }


        public void RemoveHandlersFromCollection(Control.ControlCollection coll)
        {
            foreach (Control c in coll)
            {
                //Assign change handler
                RemoveHandler(c);

                // recurively apply to inner collections
                if (c.HasChildren)
                    RemoveHandlersFromCollection(c.Controls);
            }
        }

        // recursive routine to inspect each control and assign handlers accordingly
        private void RemoveHandler(Control aControl)
        {
            if (aControl is TextBox)
                (aControl as TextBox).TextChanged -= TrafodionChangeTracker_InputChanged;

            if (aControl is CheckBox)
                (aControl as CheckBox).CheckedChanged -= TrafodionChangeTracker_InputChanged;

            if (aControl is RadioButton)
                (aControl as RadioButton).CheckedChanged -= TrafodionChangeTracker_InputChanged;

            if (aControl is NumericUpDown)
                (aControl as NumericUpDown).ValueChanged -= TrafodionChangeTracker_InputChanged;

            if (aControl is RichTextBox)
                (aControl as RichTextBox).TextChanged -= TrafodionChangeTracker_InputChanged;

            if (aControl is ComboBox)
                (aControl as ComboBox).TextChanged -= TrafodionChangeTracker_InputChanged;

            if (aControl is ComboBox)
                (aControl as ComboBox).SelectedIndexChanged -= TrafodionChangeTracker_InputChanged;

            if (aControl is ListBox)
                (aControl as ListBox).SelectedValueChanged -= TrafodionChangeTracker_InputChanged;

            if (aControl is DateTimePicker)
                (aControl as DateTimePicker).ValueChanged -= TrafodionChangeTracker_InputChanged;

            if (aControl is TrafodionIGrid)
            {
                (aControl as TrafodionIGrid).SelectionChanged -= TrafodionChangeTracker_InputChanged;
                (aControl as TrafodionIGrid).AfterCommitEdit -= TrafodionChangeTracker_InputChanged;

            }

            // ... apply for other desired input types similarly ...

        }

    }

    public class TrafodionControlChangeTracker
    {
        private Control _control;
        private string _cleanValue;

        // read only properties
        public Control Control { get { return _control; } }
        public string CleanValue { get { return _cleanValue; } }              

        // constructor establishes the control and uses its current 
        // value as "clean"
        public TrafodionControlChangeTracker(Control ctl)
        {            
            // if the control type is not one that is supported, 
            // throw an exception
            if (TrafodionControlChangeTracker.IsControlTypeSupported(ctl))
            {
                _control = ctl;
                _cleanValue = GetControlCurrentValue();
            }
        }
        // static class utility method; return whether or not the control type 
        // of the given control is supported by this class;
        // developers may modify this to extend support for other types
        public static bool IsControlTypeSupported(Control ctl)
        {
            // list of types supported
            if (ctl is TextBox) return true;
            if (ctl is RichTextBox) return true;
            if (ctl is CheckBox) return true;
            if (ctl is NumericUpDown) return true;
            if (ctl is RadioButton) return true;
            if (ctl is ComboBox) return true;
            if (ctl is ListBox) return true;
            if (ctl is DateTimePicker) return true;
            if (ctl is TrafodionIGrid) return true;
            // ... add additional types as desired ...

            // not a supported type
            return false;
        }

        // private method to determine the current value (as a string) 
        // of the control;
        // developers may modify this to extend support for other types
        private string GetControlCurrentValue()
        {
            if (_control is TextBox)
                return (_control as TextBox).Text;

            if (_control is RichTextBox)
                return (_control as RichTextBox).Text;

            if (_control is CheckBox)
                return (_control as CheckBox).Checked.ToString();

            if (_control is NumericUpDown)
                return (_control as NumericUpDown).Value.ToString();

            if (_control is RadioButton)
                return (_control as RadioButton).Checked.ToString();

            if (_control is ComboBox)
                return (_control as ComboBox).Text;

            if (_control is ListBox)
            {
                // for a listbox, create a list of the selected indexes
                StringBuilder val = new StringBuilder();
                ListBox lb = (_control as ListBox);
                ListBox.SelectedIndexCollection coll = lb.SelectedIndices;
                for (int i = 0; i < coll.Count; i++)
                    val.AppendFormat("{0};", coll[i]);

                return val.ToString();
            }
            if (_control is DateTimePicker)
                return (_control as DateTimePicker).Value.ToString();


            // ... add additional types as desired ...

            return "";
        }


        // method to establish the the current control value as "clean"
        public void EstablishValueAsClean()
        {
            _cleanValue = GetControlCurrentValue();
        }


        // determine if the current control value is considered "dirty"; 
        // i.e. if the current control value is different than the one
        // remembered as "clean"
        public bool DetermineIfDirty()
        {
            // compare the remembered "clean value" to the current value;
            // if they are the same, the control is still clean;
            // if they are different, the control is considered dirty.
            return (
              string.Compare(
                _cleanValue, GetControlCurrentValue(), false
                ) != 0
            );
        }
    }

}
