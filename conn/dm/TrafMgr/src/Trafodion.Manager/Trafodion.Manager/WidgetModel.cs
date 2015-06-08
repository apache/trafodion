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
using System.Drawing;
using System.Windows.Forms;

namespace Trafodion.Manager.Framework
{
    /// <summary>
    /// Holds properties of a widget container
    /// </summary>
	[Serializable]
	public class WidgetModel
	{
		#region Members
		private bool    _autoResize = true;
		private bool    _resizable = true;
		private bool    _allowDelete = true;
		private bool    _allowMaximize = false;
		private bool    _moveable = true;
		private String  _text = null;
		private String  _type = null;
		private Size    _size = new Size();
		private Point   _location = new Point();
        private Object  _layoutConstraint = null;        
        private bool    _layoutChangedByUser = false;
        private String  _controlClass = null;
        private DockStyle _dock;
        private bool _isActive;
 
		#endregion

		#region Properties
        /// <summary>
        /// Type name of the class that is in the container
        /// </summary>
        public String ControlClass
        {
            get { return _controlClass; }
            set { _controlClass = value; }
        }

        /// <summary>
        /// Flag to enable/disable auto resize of the container
        /// </summary>
		public bool WidgetAutoResize
		{
			get { return _autoResize; }
			set { _autoResize = value; }
		}

        /// <summary>
        /// Flag to enable/disable the ability of an user to resize a container
        /// </summary>
		public bool WidgetResizable
		{
			get { return _resizable; }
			set { _resizable = value; }
		}

        /// <summary>
        /// Flag to allow deletion of a widget container
        /// </summary>
		public bool WidgetAllowDelete
		{
			get { return _allowDelete; }
			set { _allowDelete = value; }
		}

        /// <summary>
        /// Flag to enable/disable the maximizing/minimizing of a container
        /// </summary>
		public bool WidgetAllowMaximize
		{
			get { return _allowMaximize; }
			set { _allowMaximize = value; }
		}

        /// <summary>
        /// Flag to enable the moving of a container
        /// </summary>
		public bool WidgetMoveable
		{
			get { return _moveable; }
			set { _moveable = value; }
		}

        /// <summary>
        /// The Title of the text
        /// </summary>
		public String WidgetText
		{
			get { return _text; }
			set { _text = value; }
		}

        /// <summary>
        /// The named type of the container
        /// </summary>
		public String WidgetType {
			get { return _type; }
			set { _type = value; }
		}

        /// <summary>
        /// The size of the widget
        /// </summary>
		public Size WidgetSize
		{
			get { return _size; }
			set { _size = value; }
		}

        /// <summary>
        /// The location of the widget in the canvas
        /// </summary>
		public Point WidgetLocation
		{
			get { return _location; }
			set { _location = value; }
		}

        /// <summary>
        /// Object that shall have the information needed to layout the 
        /// container on the canvas.
        /// </summary>
        public Object LayoutConstraint
        {
            get { return _layoutConstraint; }
            set { _layoutConstraint = value; }
        }

        /// <summary>
        /// Indicates if the layout has been changed by the user
        /// </summary>
        public bool LayoutChangedByUser
        {
            get { return _layoutChangedByUser; }
            set { _layoutChangedByUser = value; }
        }

        /// <summary>
        /// The current dock style of the container
        /// </summary>
        public DockStyle Dock
        {
            get { return _dock; }
            set { _dock = value; }
        }

        /// <summary>
        /// indicates if the widget is currently active
        /// </summary>
        public bool IsActive
        {
            get { return _isActive; }
            set { _isActive = value; }
        }

 		#endregion


		#region Constructors
        public WidgetModel()
        {
        }

        public WidgetModel(WidgetContainer widget)
		{
			this._autoResize = widget.AutoResize;
			this._resizable = widget.Resizable;
			this._allowDelete = widget.AllowDelete;
			this._allowMaximize = widget.AllowMaximize;
			this._moveable = widget.Moveable;
			this._text = widget.Text;
			this._type = widget.WidgetType;
            this._size = widget.ContainerSize;
            this._location = widget.ContainerLocation;
            this._controlClass = widget.View.GetType().FullName;
            this.LayoutChangedByUser = widget.LayoutChangedByUser;
            this.LayoutConstraint = widget.LayoutConstraint;
            this.Dock = widget.Dock;
            this.IsActive = widget.isActive;
		}

		#endregion
	}
}
