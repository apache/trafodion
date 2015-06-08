//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
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
using System.Windows.Forms;
using Trafodion.Manager.LiveFeedFramework.Models;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;

namespace Trafodion.Manager.LiveFeedFramework.Controls
{
    /// <summary>
    /// Live Feed Universal Widget
    /// </summary>
    public class LiveFeedUniversalWidget : GenericUniversalWidget
    {
        #region Fields

        private ToolStripButton _thePlayButton = null;

        #endregion Fields

        #region Constructor

        /// <summary>
        /// The constructor
        /// </summary>
        /// <param name="aConfig"></param>
        public LiveFeedUniversalWidget(UniversalWidgetConfig aConfig)
         : this()
        {
            aConfig.ShowRefreshTimerButton = false;
            base.UniversalWidgetConfiguration = aConfig;
        }

        /// <summary>
        /// Default constructor
        /// </summary>
        public LiveFeedUniversalWidget()
        {
            RearrangeToolStripButtons();
        }

        #endregion Constructor 

        #region Private methods

        /// <summary>
        /// To override the new data arrived event
        /// </summary>
        /// <param name="obj"></param>
        /// <param name="e"></param>
        protected override void HandleNewDataArrived(Object obj, EventArgs e)
        {
            base.HandleNewDataArrived(obj, e);
            base.SetStopQueryButton(true);
        }

        /// <summary>
        /// To override handle fetch cancelled event
        /// </summary>
        /// <param name="obj"></param>
        /// <param name="e"></param>
        protected override void HandleFetchCancelled(Object obj, EventArgs e)
        {
            base.HandleFetchCancelled(obj, e);
            base.SetStopQueryButton(false);
            //_thePlayButton.Enabled = true;
            //TheStopQueryButton.Enabled = false;
        }

        /// <summary>
        /// To override init dataprovider event
        /// </summary>
        /// <param name="obj"></param>
        /// <param name="e"></param>
        protected override void HandleInitDataproviderForFetch(Object obj, EventArgs e)
        {
            base.HandleInitDataproviderForFetch(obj, e);
            base.SetStopQueryButton(true);
            //_thePlayButton.Enabled = false;
            //TheStopQueryButton.Enabled = true;
        }

        /// <summary>
        /// To re-arrange toolstrip buttons
        /// </summary>
        private void RearrangeToolStripButtons()
        {
            this.TheRefreshButton.Image = (System.Drawing.Image)global::Trafodion.Manager.Properties.Resources.PlayIcon;
            this.TheRefreshButton.ToolTipText = "Play";
            this.TheStopQueryButton.Image = (System.Drawing.Image)global::Trafodion.Manager.Properties.Resources.PauseIcon;
            this.TheStopQueryButton.ToolTipText = "Pause";
        }

        #endregion Private Methods
    }
}
