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
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Drawing;
using System.Windows.Forms;

namespace Trafodion.Manager.Framework
{
    /// <summary>
    /// This class shall be used to store the properties of a canvas
    /// so that it can be restored.
    /// </summary>
	[Serializable]
    class WidgetCanvasModel
    {
        #region Members
        private bool _locked = false;
        private string _viewName = null;
        Hashtable _widgetModels;
        #endregion

        /// <summary>
        /// The models of the WidgetContainers that are in the canvas
        /// </summary>
        public Hashtable WidgetModels
        {
            get { return _widgetModels; }
            set { _widgetModels = value; }
        }

        /// <summary>
        /// Property to store the locked property of the canvas
        /// </summary>
        public bool Locked
        {
            get { return _locked; }
            set { _locked = value; }
        }

        /// <summary>
        /// Name of a canvas. Needed to differentiate multiple instance of
        /// the same canvas.
        /// </summary>
        public string ViewName
        {
            get { return _viewName; }
            set { _viewName = value; }
        }

        /// <summary>
        /// Constructor that takes the canvas and populates the model from it.
        /// </summary>
        /// <param name="aCanvas"></param>
        public WidgetCanvasModel(WidgetCanvas aCanvas)
		{
            this._locked = aCanvas.Locked;
            this._viewName = aCanvas.ViewName;
            aCanvas.RebuildWidgetsModel();
            this._widgetModels = aCanvas.WidgetsModel;
		}

    }
}
