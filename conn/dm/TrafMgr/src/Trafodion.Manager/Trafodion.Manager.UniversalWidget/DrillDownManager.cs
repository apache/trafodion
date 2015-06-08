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
using Trafodion.Manager.UniversalWidget.Controls;


namespace Trafodion.Manager.UniversalWidget
{
    /// <summary>
    /// class to co-ordinate a drill down
    /// </summary>
    public class DrillDownManager
    {

        public delegate void DrillDownRequested(object sender, DrillDownEventArgs e);
        public event DrillDownRequested OnDrillDownRequested;

        public delegate void ShowNewWidget(UniversalWidgetConfig config, WidgetLinkerObject widgetLinker);
        private ShowNewWidget _theShowNewWidgetImpl;

        public delegate void PopulateCalledWidget(UniversalWidgetConfig config, WidgetLinkerObject widgetLinker);
        private PopulateCalledWidget _thePopulateCalledWidgetImpl;

        private UniversalWidgetConfig _theConfig;

        public UniversalWidgetConfig Config
        {
            get { return _theConfig; }
            set { _theConfig = value; }
        }
        private IDataDisplayControl _theDataDisplayControl;

        public IDataDisplayControl DataDisplayControl
        {
            get { return _theDataDisplayControl; }
            set { _theDataDisplayControl = value; }
        }

        public DrillDownManager()
        {
            this.OnDrillDownRequested += new DrillDownRequested(DrillDownManager_OnDrillDownRequested);
        }

        public DrillDownManager(UniversalWidgetConfig aConfig, IDataDisplayControl aDataDisplayControl) : this()
        {
            _theConfig = aConfig;
            _theDataDisplayControl = aDataDisplayControl;
        }

         /// <summary>
        /// Called by the display control to notify the listeners that a drill down has been requested
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public void FireDrillDownRequested(object sender, DrillDownEventArgs e)
        {
            if (OnDrillDownRequested != null)
            {
                OnDrillDownRequested(sender, e);
            }
        }

        /// <summary>
        /// Removes all event handlers and delegates
        /// </summary>
        public void Reset()
        {
            OnDrillDownRequested = null;
            ShowNewWidgetImpl = null;
        }


        /// <summary>
        /// The delegate to display the new widget
        /// </summary>
        public ShowNewWidget ShowNewWidgetImpl
        {
            get { return _theShowNewWidgetImpl; }
            set { _theShowNewWidgetImpl = value; }
        }

        /// <summary>
        /// the delegate to identify the report to execute next and populate it with the
        /// parameters
        /// </summary>
        public PopulateCalledWidget PopulateCalledWidgetImpl
        {
            get { return _thePopulateCalledWidgetImpl; }
            set { _thePopulateCalledWidgetImpl = value; }
        }


        /// <summary>
        /// 
        /// This is where the next widget to display determined. If there is only one associated
        /// widget, then that is selected otherwise the user is prompted with a dialog to select 
        /// the widget to display.
        /// 
        /// This is a helper method.
        /// </summary>
        /// <param name="widgetLinker"></param>
        public virtual void PopulateTheCalledWidget(WidgetLinkerObject widgetLinker)
        {
            UniversalWidgetConfig callerConfig = WidgetRegistry.GetInstance().GetUniversalWidgetConfig(widgetLinker.CallingWidget);
            if ((callerConfig != null) && (_thePopulateCalledWidgetImpl != null))
            {
                _thePopulateCalledWidgetImpl(callerConfig, widgetLinker);
                //List<AssociatedWidgetConfig> associatedWidgets = callerConfig.AssociatedWidgets;
                //if ((associatedWidgets != null) && (associatedWidgets.Count > 0))
                //{
                //    UniversalWidgetConfig config = null;
                //    WidgetSelectorDialog dialog = new WidgetSelectorDialog(_theConfig, widgetLinker);
                //    dialog.ShowDialog();
                //    string selectedWidget = dialog.SelectedWidget;
                //    if (selectedWidget != null)
                //    {
                //        config = WidgetRegistry.GetInstance().GetUniversalWidgetConfig(selectedWidget);
                //        if (config != null)
                //        {
                //            widgetLinker.CalledWidget = config.Name;

                //            //get the updated parameters from user input
                //            List<ReportParameter> updatedParams = dialog.GetUpdatedParameters();

                //            //Update the widget linker RowHashtbale with the user provided values
                //            widgetLinker.UpdateFromUserInput(updatedParams);
                //        }
                //    }
                //    else
                //    {
                //        widgetLinker.CalledWidget = null;
                //    }
                //}
            }
        }

        /// <summary>
        /// This is a helper method to diplay the drilled down widget.
        /// </summary>
        /// <param name="widgetLinker"></param>
        public void DisplayWidget(WidgetLinkerObject widgetLinker)
        {
            //UniversalWidgetConfig callerConfig = WidgetRegistry.GetInstance().GetUniversalWidgetConfig(widgetLinker.CallingWidget);
            string selectedWidget = widgetLinker.CalledWidget;
            if (selectedWidget != null)
            {
                UniversalWidgetConfig calledConfig = WidgetRegistry.GetInstance().GetUniversalWidgetConfig(selectedWidget);
                if (calledConfig != null)
                {
                    calledConfig.DataProviderConfig.ConnectionDefinition = _theDataDisplayControl.DataProvider.DataProviderConfig.ConnectionDefinition;//callerConfig.DataProviderConfig.ConnectionDefinition;
                    if (_theShowNewWidgetImpl != null)
                    {
                        _theShowNewWidgetImpl(calledConfig, widgetLinker);
                    }
                }
            }
        }


        /// <summary>
        /// This event will get fired when the user trues to drill down. e
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void DrillDownManager_OnDrillDownRequested(object sender, DrillDownEventArgs e)
        {
            //Find the widget that needs to be called 
            WidgetLinkerObject widgetLinker = e.WidgetLinkerObject;
            PopulateTheCalledWidget(widgetLinker);

            //Display the widget
            DisplayWidget(widgetLinker);

        }


    }

    /// <summary>
    /// This class has the fields needed to support the dataprovider events
    /// </summary>
    public class DrillDownEventArgs : EventArgs
    {
        Dictionary<string, object> _theDrillDownParameters;
        WidgetLinkerObject _theWidgetLinkerObject;

        public WidgetLinkerObject WidgetLinkerObject
        {
            get { return _theWidgetLinkerObject; }
            set { _theWidgetLinkerObject = value; }
        }

        public Dictionary<string, object> DrillDownParameters
        {
            get { return _theDrillDownParameters; }
        }

        /// <summary>
        /// Add a parameter to the event arg
        /// </summary>
        /// <param name="aKey"></param>
        /// <param name="aValue"></param>
        public void AddEventProperty(string aKey, object aValue)
        {
            if (_theDrillDownParameters == null)
            {
                _theDrillDownParameters = new Dictionary<string, object>();
            }
            if (_theDrillDownParameters.ContainsKey(aKey))
            {
                _theDrillDownParameters.Remove(aKey);
            }
            _theDrillDownParameters.Add(aKey, aValue);
        }

        /// <summary>
        /// Returns the value of  a viven event property
        /// </summary>
        /// <param name="aKey"></param>
        /// <returns></returns>
        public object GetEventProperty(string aKey)
        {
            if (_theDrillDownParameters != null)
            {
                return _theDrillDownParameters[aKey];
            }
            return null;
        }
    }
}
