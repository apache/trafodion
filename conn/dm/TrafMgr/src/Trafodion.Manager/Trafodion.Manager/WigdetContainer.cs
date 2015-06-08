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
using System.Collections;
using System.Reflection;
using System.Text;

using System.Windows.Forms;

// using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Runtime.InteropServices;
using Trafodion.Manager.Framework.Controls;


namespace Trafodion.Manager.Framework
{
    /// <summary>
    /// This class acts as a container to hold the control that is placed on the
    /// canvas. It takes care of the floating of the control on the Canvas.
    /// </summary>
    public class WidgetContainer : TrafodionGroupBox
    {
        #region  Members

        #region  Static Members

        /// <summary>
        /// Minimum width of a widget.
        /// </summary>
        public const int MinimumWidth = 50;

        /// <summary>
        /// Minimum height of a widget.
        /// </summary>
        public const int MinimumHeight = 50;

        /// <summary>
        /// Minimum height of a widget.
        /// </summary>
        private static String[] _arrowDirections = new String[16];

        /// <summary>
        /// Static members needed to render the screen buttons correctly
        /// </summary>
        private static System.Drawing.Size buttonSize = new System.Drawing.Size(20, 20); //Size of the buttons
        private static int buttonSpacing = 2; //The spacing between the two buttons

        #endregion  /*  End of region : Static Members.  */

        /// <summary>
        /// States of the widget
        /// </summary>
        public enum WIDGET_STATE { /* MINIMIZED = 85, */ Normal = 101, Maximized = 280 };

        private const int BLINK_INTERVAL = 400;

        #region  Instance Members

        private WidgetCanvas    _containerCanvas = null;
        private String          _type = null;
        private String          _data = null;
        private Control         _view = null;
        private Button _deleteButton = new Button();
        private Button _maximizeButton = new Button();
        private bool            _autoResize = true;
        private bool            _resizable = true;
        private bool            _allowDelete = true;
        private bool            _allowMaximize = true;
        private bool            _moveable = true;
        private bool            _confirmWidgetDelete = false;
        private Point           _grabLocationOffset = new Point(0, 0);
        private bool            _amMoving = false;
        private bool            _amActive = false;
        private Color           _foreColor;
        private bool            _viewLocked = false;
        private DockStyle       _dockStyle = DockStyle.None;
        private DockStyle       _initialDockStyle = DockStyle.None;
        private List<EventHandlerForAttribute> _eventHandlers = new List<EventHandlerForAttribute>();
        private List<EventInfo> _events = new List<EventInfo>();
        private Object          _layoutConstraint = null;
        private bool            _layoutChangedByUser = false;
        private Size            _containerSize;
        private Point           _containerLocation;
        private WidgetModel     _defaultModel;

        private Timer _blinkTimer = null;
        private object _blinkSyncRoot = new object();
        private Color _defaultBackColor = SystemColors.Control;

        #endregion  /*  End of region : Instance Members.  */

        #endregion  /*  End of region : Members.  */

        #region Constructors

        static WidgetContainer()
        {
            _arrowDirections[0] = "NoDirection";
            _arrowDirections[NativeUser32Wrapper.DIR_INSIDE + 1] = "Inside";
            _arrowDirections[NativeUser32Wrapper.DIR_BOTTOM + 1] = "Bottom";
            _arrowDirections[NativeUser32Wrapper.DIR_TOP + 1] = "Top";
            _arrowDirections[NativeUser32Wrapper.DIR_LEFT + 1] = "Left";
            _arrowDirections[NativeUser32Wrapper.DIR_RIGHT + 1] = "Right";
            _arrowDirections[NativeUser32Wrapper.DIR_TOPLEFT + 1] = "TopLeft";
            _arrowDirections[NativeUser32Wrapper.DIR_TOPRIGHT + 1] = "TopRight";
            _arrowDirections[NativeUser32Wrapper.DIR_BOTTOMLEFT + 1] = "BottomLeft";
            _arrowDirections[NativeUser32Wrapper.DIR_BOTTOMRIGHT + 1] = "BottomRight";
        }

        /// <summary>
        /// Default constructor
        /// </summary>
        /// <param name="containerCanvas"> The canvas on which this widget shall be displayed</param>
        /// <param name="view">The Control that shall be placed on this container</param>
        public WidgetContainer(WidgetCanvas containerCanvas, Control view)
            : this(containerCanvas, view, "TrafodionManager Widget", new Point(0, 0), new Size(MinimumWidth, MinimumHeight))
        {
        }

        /// <summary>
        /// Creates a widget container with the control passed
        /// </summary>
        /// <param name="containerCanvas"> The canvas on which this widget shall be displayed</param>
        /// <param name="view">The Control that shall be placed on this container</param>
        public WidgetContainer(WidgetCanvas containerCanvas, Control view, String title)
            : this(containerCanvas, view, title, new Point(0, 0), new Size(MinimumWidth, MinimumHeight))
        {
        }   


        /// <summary>
        /// Creates a widget container with the control passed
        /// </summary>
        /// <param name="containerCanvas"> The canvas on which this widget shall be displayed</param>
        /// <param name="view">The Control that shall be placed on this container</param>
        /// <param name="title">The title of this control</param>
        /// <param name="position"> The position where the widget will be placed on the UI</param>
        /// <param name="bestFit"> The initial size that shall be used</param>
        public WidgetContainer(WidgetCanvas containerCanvas, Control view, String title, Point position, Size bestFit)
        {
            this._containerCanvas = containerCanvas;
            AddControlToContainer(view, title, position, bestFit);
            base.Show();
        }  

        #endregion  /*  End of region : Constructors.  */

        #region Events
        /// <summary>
        /// The delegate of the method that gets called when the widget container is closed
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public delegate void WidgetContainerClosedEventDelegate(object sender, EventArgs e);
        public event WidgetContainerClosedEventDelegate WidgetContainerClosedEvent;

        public delegate void WidgetContainerMovedEventDelegate(object sender, EventArgs e);
        public event WidgetContainerMovedEventDelegate WidgetContainerMovedEvent;

        #endregion  /*  End of region : Events.  */
        
        #region Properties

        /// <summary>
        /// Returns a list of attributes that indicate what events the Control in the container can
        /// handle.
        /// </summary>
        public List<EventHandlerForAttribute> EventHandlers
        {
          get { return _eventHandlers; }
          set { _eventHandlers = value; }
        }

        /// <summary>
        /// Returns the list of events that the Control in this container can fire
        /// </summary>
        public List<EventInfo> EventsItCanFire
        {
            get { return _events; }
            set { _events = value; }
        }

        /// <summary>
        /// If the widget was docked when it was put on the canvas, returns the initial dock state
        /// </summary>
        public DockStyle InitialDockStyle
        {
            get { return _initialDockStyle; }
            set { _initialDockStyle = value; }
        }

        /// <summary>
        /// Saved background color.
        /// </summary>
        public override Color ForeColor
        {
            get { return base.ForeColor; }
            set { base.ForeColor = value; this._foreColor = value; }

        }  /*  End of  ForeColor  Property.  */

        /// <summary>
        /// Saved dock state.
        /// </summary>
        public override DockStyle Dock
        {
            get { return base.Dock; }
            set { base.Dock = value; this._dockStyle = value; }
        } 

        /// <summary>
        /// Knob to indicate whether or not the widget is auto resizable.
        /// </summary>
        public bool AutoResize
        {
            get { return this._autoResize; }
            set 
            { 
                this._autoResize = value; 
                AdjustMaximizeButtonPosition(); 
            }
        }  

        /// <summary>
        ///  Knob to indicate whether or not the widget can be resized.
        /// </summary>
        public bool Resizable
        {
            get { return this._resizable; }
            set { this._resizable = value; }
        }  

        /// <summary>
        /// Knob to allow the widget to be moved.
        /// </summary>
        public bool Moveable
        {
            get { return this._moveable; }
            set { this._moveable = value; }
        }  

        /// <summary>
        /// Knob to allow the widget to be deleted (show/hide the Close Button).
        /// </summary>
        public bool AllowDelete
        {
            get { return this._allowDelete; }
            set
            {
                this._deleteButton.Visible = value;
                this._allowDelete = value;
                AdjustMaximizeButtonPosition();
            }
        } 

        /// <summary>
        /// Knob to allow the widget to be maximized/restored (show/hide the Maximize Button).
        /// </summary>
        public bool AllowMaximize
        {
            get { return this._allowMaximize; }
            set
            {
                this._allowMaximize = value;
                AdjustMaximizeButtonPosition();
            }
        }  

        /// <summary>
        /// Knob to indicate whether or not to confirm widget delete.
        /// </summary>
        public bool DeleteConfirmation
        {
            get { return this._confirmWidgetDelete; }
            set { this._confirmWidgetDelete = value; }
        }  

        /// <summary>
        /// The control that this widget is a container of
        /// </summary>
        public Control View
        {
            get { return this._view; }
            set { this._view = value; }
        }  

        /// <summary>
        /// Is this widget Active
        /// </summary>
        public bool isActive
        {
            get
            {
                if (null == _view)
                {
                    return false;
                }
                return this._amActive;
            }
        }

        /// <summary>
        /// This object contains the container properties as
        /// specified buring design item.
        /// </summary>
        public WidgetModel DefaultModel
        {
            get { return _defaultModel; }
            set { _defaultModel = value; }
        }

        /// <summary>
        /// Type of this widget.
        /// </summary>
        public String WidgetType
        {
            get { return this._type; }
            set { this._type = value; }
        }

        /// <summary>
        /// The initial layout constraint for container
        /// </summary>
        public Object LayoutConstraint
        {
            get { return _layoutConstraint; }
            set { _layoutConstraint = value; }
        }

        /// <summary>
        /// Will be set to indicate that the user has changed the layout
        /// </summary>
        public bool LayoutChangedByUser
        {
            set { _layoutChangedByUser = value; }
            get { return _layoutChangedByUser; }
        }

        /// <summary>
        /// The current size of the container
        /// </summary>
        public Size ContainerSize
        {
            get 
            {
                if (Dock == DockStyle.None)
                {
                    return Size;
                }
                return _containerSize; 
            }

            set { _containerSize = value; }
        }

        /// <summary>
        /// The location of the container on the canvas
        /// </summary>
        public Point ContainerLocation
        {
            get 
            {
                if (Dock == DockStyle.None)
                {
                    return Location;
                }
                return _containerLocation; 
            }
            set { _containerLocation = value; }
        }

        /// <summary>
        /// The title 
        /// </summary>
        public string Title
        {
            get { return this.Text; }
            set { this.Text = value; }
        }

        #endregion  /*  End of region : Properties.  */
        
        #region Public

        /// <summary>
        /// Return a new WidgetModel for this container.
        /// </summary>
        /// <returns>
        /// The new WidgetModel. 
        /// </returns>
        public WidgetModel GetWidgetModel()
        {
            return new WidgetModel(this);
        }


        /// <summary>
        /// Given a WidgetModel, sets the member variables appropriately
        /// </summary>
        /// <param name="model"></param>
        public void LoadModelInformation(WidgetModel model)
        {
            this._autoResize = model.WidgetAutoResize;
            this._resizable = model.WidgetResizable;
            this._allowDelete = model.WidgetAllowDelete;
            this._allowMaximize = model.WidgetAllowMaximize;
            this._moveable = model.WidgetMoveable;
            this.Size = model.WidgetSize;
            this.Location = model.WidgetLocation;
            this.Text = model.WidgetText;
            this.WidgetType = model.WidgetType;
            this.LayoutConstraint = model.LayoutConstraint;
            this.LayoutChangedByUser = model.LayoutChangedByUser;
            
            //TODO: temporary hack to set the dockstyle. needs to be fixed
            if (DockStyle.Fill == model.Dock)
            {
                DockContainer();
                this._dockStyle = DockStyle.None;
            }

        }


        /// <summary>
        /// Resizes the container with the width factor and height factor provided
        /// </summary>
        /// <param name="wFactor"></param>
        /// <param name="hFactor"></param>
        public virtual void ResizeWidget(double wFactor, double hFactor)
        {
            if (this.AutoResize)
            {
                ResizeGroupBox(wFactor,	hFactor);
            }
        }


        /// <summary>
        /// Activate the container.
        /// </summary>
        public void ActivateWidget()
        {
            if (! this.isActive)
            {
                this._view.Focus();

                base.Refresh();

            }

            //base.ForeColor = System.Drawing.SystemColors.WindowText;
            base.BringToFront();
            _amActive = true;
        }


        /// <summary>
        /// Deactivate the container.
        /// </summary>
        public void DeactivateWidget()
        {
            //The following line can be used to highlight only the widget that is 
            //active. If this line is un-commented, the inactive widgets will show
            //in gray.
            //base.ForeColor = this._foreColor;
            base.Refresh();
            _amActive = false;
        }


        /// <summary>
        /// Delete the container if it is allowed and not locked.  Also, a
        /// confirmation message will be posted for the user to confirm it
        /// before the container is removed.
        /// </summary>
        public virtual void DeleteWidget()
        {
            if ((null == this._view) || this._viewLocked || !this.AllowDelete)
                return;

            if (this.DeleteConfirmation)
            {
                DialogResult yesNo = MessageBox.Show(Utilities.GetForegroundControl(), "\nAre you sure want to delete the '" + this._type + "' tool?\n",
                                                     "Delete Tool '" + this._type + "' Confirmation",
                                                     MessageBoxButtons.YesNo, MessageBoxIcon.Exclamation);

                if (DialogResult.No == yesNo)
                    return;
            }

            // Now, make sure the control is also deleted.
            _view.Dispose();

            try
            {
                this.Parent.Controls.Remove(this);
                RemoveClickEventListener(View);
                if (WidgetContainerClosedEvent != null)
                {
                    WidgetContainerClosedEvent(this, new WidgetContainerEventArgs());
                }
            }
            catch (Exception e)
            {
            }
        }

        public void StartBlink()
        {
            lock (_blinkSyncRoot)
            {
                if (_blinkTimer == null)
                {
                    _blinkTimer = new Timer();
                    _blinkTimer.Interval = BLINK_INTERVAL;
                    _blinkTimer.Tick += new EventHandler(BlinkBackColor);
                    _blinkTimer.Tag = false;
                }

                if (!(bool)_blinkTimer.Tag)
                {
                    _blinkTimer.Start();
                    _blinkTimer.Tag = true;
                }
            }
        }

        public void StopBlink()
        {
            if (_blinkTimer != null && (bool)_blinkTimer.Tag)
            {
                _blinkTimer.Stop();
                _blinkTimer.Tag = false;
                this.BackColor = _defaultBackColor;
            }
        }

        void BlinkBackColor(object sender, EventArgs e)
        {
            this.BackColor = this.BackColor == _defaultBackColor ? Color.Red : _defaultBackColor;
        }

        #endregion  /*  End of region : Public.  */
        
        #region Event Methods


        protected override void OnResize(EventArgs eArgs)
        {
            base.OnResize(eArgs);

            if (false == this._resizable)
                return;

            ResizeGroupBox(1, 1);
        }  /*  End of  OnResize  eventHandler.  */


        private void WidgetContainer_Click(Object sender, EventArgs eArgs)
        {
            if (!this.isActive)
            {
                ActivateMe();
            }
        }


        private void RefreshWorkspaceView()
        {
            try
            {
                //this._view.TabControl.Refresh();
                //TODO: fire event to refresh work space
            }
            catch (Exception e)
            {
            }

        }

        private void DeleteWidgetEventHandler(Object sender, EventArgs eArgs)
        {
            ActivateMe();
            DeleteWidget();
            RefreshWorkspaceView();
        }

        private void MaximizeWidgetEventHandler(Object sender, EventArgs eArgs)
        {
            if (DockStyle.None == base.Dock)
            {
                //save the size of the pre-dock widget so that it can be restored after persistance
                ContainerSize = Size;
                ContainerLocation = Location;
            }

            DockContainer();
        }

        private void DockContainer()
        {
            DockStyle style = DockStyle.Fill;
            Image backgndImg = global::Trafodion.Manager.Properties.Resources.nofullscreen;

            if (DockStyle.Fill == base.Dock)
            {
                style = this._dockStyle;
                backgndImg = global::Trafodion.Manager.Properties.Resources.fullscreen;
            }

            base.Dock = style;
            this._maximizeButton.BackgroundImage = backgndImg;
            ActivateMe();

            RefreshWorkspaceView();
        }

        #region Mouse Events

        protected override void OnMouseUp(MouseEventArgs e)
        {
            base.OnMouseUp(e);
            ActivateMe();
            //Notify listeners that the container has moved
            if ((_amMoving) && (WidgetContainerMovedEvent != null))
            {
                WidgetContainerMovedEvent(this, null);                
            }
            this._amMoving = false;

            ResizeGroupBox(1, 1);
        }

        protected override void OnMouseMove(MouseEventArgs meArgs)
        {
            if ((false == this.Resizable) && (false == this.Moveable))
                return;
            
            base.OnMouseMove(meArgs);

            if (this.Moveable && (MouseButtons.Left == meArgs.Button) &&
                this._amMoving)
            {
                //We set the dockstyle to None otherwise the layout manager will not let the control
                // to move
                _layoutChangedByUser = true;
                this.Dock = DockStyle.None;
                this.Cursor = Cursors.Default;
                Point myPos = base.Parent.PointToClient(Cursor.Position);
                int posX = Math.Max(0 - this.Width , myPos.X - this._grabLocationOffset.X);
                int posY = Math.Max(0 - this.Height, myPos.Y - this._grabLocationOffset.Y);
                //Console.WriteLine("({0}, {1})", posX, posY);
                this.Location = new Point(posX, posY);
                base.Refresh();
                return;
            }

            if (false == this.Resizable)
                return;

            
            int dir = GetResizeDirection(meArgs.Location);
            switch (dir)
            {
                case NativeUser32Wrapper.DIR_TOP:
                    if (this.Moveable)
                        this.Cursor = Cursors.SizeAll;
                    else
                        this.Cursor = Cursors.SizeNS;

                    break;

                case NativeUser32Wrapper.DIR_BOTTOM:
                    this.Cursor = Cursors.SizeNS;
                    break;

                case NativeUser32Wrapper.DIR_LEFT:
                case NativeUser32Wrapper.DIR_RIGHT:
                    this.Cursor = Cursors.SizeWE;
                    break;

                case NativeUser32Wrapper.DIR_TOPLEFT:
                case NativeUser32Wrapper.DIR_BOTTOMRIGHT:
                    this.Cursor = Cursors.SizeNWSE;
                    break;

                case NativeUser32Wrapper.DIR_TOPRIGHT:
                case NativeUser32Wrapper.DIR_BOTTOMLEFT:
                    this.Cursor = Cursors.SizeNESW;
                    break;

                default: this.Cursor = Cursors.Default;
                    break;
            }


        }  /*  End of  OnMouseMove  eventHandler.  */



        protected override void OnMouseDown(MouseEventArgs meArgs)
        {
            if ((false == this.Resizable) && (false == this.Moveable))
            {
                ActivateMe();
                return;
            }

            base.OnMouseDown(meArgs);
            ActivateMe();

            int dir = GetResizeDirection(meArgs.Location);

            //if (NCCTraceManager.IsTracingEnabled)
            //    NCCTraceManager.OutputToLog("Direction = " + dir + " [" + _arrowDirections[dir + 1] + "]. ");

            if (this.Moveable && (MouseButtons.Left == meArgs.Button) &&
                ((NativeUser32Wrapper.DIR_TOP == dir) ||
                 (NativeUser32Wrapper.DIR_INSIDE == dir)))
            {
                this.Cursor = Cursors.SizeAll;
                this._amMoving = true;
                this._grabLocationOffset = meArgs.Location;
                base.Refresh();
                return;
            }

            if (false == this.Resizable)
                return;


            IntPtr hwnd = this.Handle;


            if (-1 != dir)
            {
                NativeUser32Wrapper.ReleaseCapture(hwnd);
                NativeUser32Wrapper.SendMessage(hwnd, NativeUser32Wrapper.WinUser_WM_SYSCOMMAND,
                                                NativeUser32Wrapper.WinUser_SC_SIZE + dir, 0);
                _layoutChangedByUser = true;
            }


        }  /*  End of  OnMouseDown  eventHandler.  */



        protected override void OnMouseLeave(EventArgs eArgs)
        {
            base.OnMouseLeave(eArgs);
            this.Cursor = Cursors.Default;

        }  /*  End of  OnMouseLeave  eventHandler.  */


        #endregion  /*  End of region : Mouse Events.  */

        #endregion  /*  End of region : Event Methods.  */

        #region Private Methods

        /// <summary>
        /// Helper method to activate this container
        /// </summary>
        private void ActivateMe()
        {
            if (_containerCanvas != null)
            {
                _containerCanvas.ActivateWidget(this);
            }
            else
            {
                ActivateWidget();
            }
        }

        /// <summary>
        /// This method will return all the events that have been defined in the Object that is being passed
        /// </summary>
        /// <param name="obj"></param>
        /// <returns></returns>
        private List<EventInfo> FindEvents(Object obj)
        {
            List<EventInfo> ret = new List<EventInfo>();
            Type type = obj.GetType();
            System.Reflection.BindingFlags myBindingFlags = System.Reflection.BindingFlags.Instance | System.Reflection.BindingFlags.Public ;//| System.Reflection.BindingFlags.DeclaredOnly;
            System.Reflection.EventInfo[] events = type.GetEvents(myBindingFlags);
            //Console.WriteLine("Got " + events.Length + " events");
            for (int i = 0; i < events.Length; i++)
            {
                EventInfo eventItem = events[i];
                ret.Add(eventItem);
            }
            return ret;
        }

        /// <summary>
        /// Returns the size that should be used to display the control
        /// </summary>
        /// <param name="sz"></param>
        /// <returns></returns>
        private Size ValidateSize(Size sz)
        {
            //return new Size(Math.Min(sz.Width, this.MinimumSize.Width),
            //                 Math.Min(sz.Height, this.MinimumSize.Height));

            return new Size(Math.Max(sz.Width, this.MinimumSize.Width) + 20,
                             Math.Max(sz.Height, this.MinimumSize.Height) + 28);

        }  

        /// <summary>
        /// Given a position and size, returns the valid position where the
        /// control should be splayed
        /// </summary>
        /// <param name="pos"></param>
        /// <param name="bestFit"></param>
        /// <returns></returns>
        private Point ValidatePosition(Point pos, Size bestFit)
        {
            Size sz = ValidateSize(bestFit);

            int xCoordinate = 0;
            int yCoordinate = 0;
            if (null != base.Parent)
            {
                if (pos.X + sz.Height > base.Parent.Size.Height)
                    xCoordinate = Math.Max(0, Math.Min(pos.X, base.Parent.Size.Height));

                if (pos.Y + sz.Width > base.Parent.Size.Width)
                    yCoordinate = Math.Max(0, Math.Min(pos.Y, base.Parent.Size.Width));

                return new Point(xCoordinate, yCoordinate);
            }
            return pos;
        }


        /// <summary>
        /// This method adds the control to the container and sets it up appropriately so that
        /// it can participate in the framework.
        /// </summary>
        /// <param name="view"></param>
        /// <param name="title"></param>
        /// <param name="pos"></param>
        /// <param name="bestFit"></param>
        private void AddControlToContainer(Control view, String title, Point pos, Size bestFit)
        {
            //set the groupbox style
            base.SetStyle(ControlStyles.AllPaintingInWmPaint |
                          ControlStyles.OptimizedDoubleBuffer |
                          ControlStyles.DoubleBuffer |
                          ControlStyles.UserPaint, true);
            base.DoubleBuffered = true;
            base.MinimumSize = new Size(MinimumWidth, MinimumHeight);
            /*base.Font = new System.Drawing.Font("Trebuchet MS", 
                                                8.25F, 
                                                System.Drawing.FontStyle.Bold,
                                                System.Drawing.GraphicsUnit.Point, 
                                                0);*/
            base.FlatStyle = FlatStyle.Popup;
            base.TabStop = false;
            InitialDockStyle = view.Dock;
            
            //set the properties of the container
            //this.ForeColor = Color.Gray;
            this.BackColor = Color.FromKnownColor(KnownColor.Snow);
            this._view = view;
            this.Padding = new Padding(6, 8, 6, 6); //This padding is used to determine where the re-size cursor will be displayed
            this.Location = ValidatePosition(pos, bestFit);
            this.Text = title;
            this.WidgetType = title;
            this.Size = ValidateSize(view.Size);//bestFit);
            this._grabLocationOffset = base.Location;

            //set the properties of the delete button
            int xCoordinate = base.Width - buttonSize.Width;
            int yCoordinate = Padding.Top - 8;

            this._deleteButton.Anchor = AnchorStyles.Right | AnchorStyles.Top;
            this._deleteButton.Location = new System.Drawing.Point(xCoordinate, yCoordinate);
            this._deleteButton.Size = buttonSize;
            this._deleteButton.TabStop = true;
            this._deleteButton.TabIndex = 1;
            this._deleteButton.Text = "x";
            this._deleteButton.TextAlign = ContentAlignment.MiddleCenter;
            this._deleteButton.BackColor = Color.Red;
            this._deleteButton.ForeColor = Color.White;
            this._deleteButton.Padding = new Padding(0);
            this._deleteButton.Font = new System.Drawing.Font("Lucida Console", 8.25F, System.Drawing.FontStyle.Bold);
            this._deleteButton.Visible = this.AllowDelete;

            //set the properties of the maximize button
            this._maximizeButton.BackgroundImage = Properties.Resources.fullscreen;
            this._maximizeButton.BackgroundImageLayout = ImageLayout.Center;
            this._maximizeButton.Anchor = AnchorStyles.Right | AnchorStyles.Top;
            this._maximizeButton.Size = buttonSize;
            this._maximizeButton.TabStop = true;
            this._maximizeButton.TabIndex = 2;
            this._maximizeButton.Text = "";
            this._maximizeButton.TextAlign = ContentAlignment.MiddleCenter;
            this._maximizeButton.BackColor = Color.FromKnownColor(KnownColor.ActiveCaption);
            this._maximizeButton.Padding = new Padding(0);
            this._maximizeButton.Visible = this.AllowMaximize;

            //Place the control in the container
            this.Focus();
            this.Controls.Add(view);
            view.Dock = System.Windows.Forms.DockStyle.Fill;
            view.Location = new System.Drawing.Point(0, 0);

            //Add the buttons in the panel
            this.Controls.Add(this._deleteButton);
            this.Controls.Add(this._maximizeButton);
            
            //Place the maximize button correctly in the UI
            AdjustMaximizeButtonPosition();

            //Associate the event handlers
            this._deleteButton.Click += new EventHandler(DeleteWidgetEventHandler);
            this._maximizeButton.Click += new EventHandler(MaximizeWidgetEventHandler);
            this.DoubleClick += new EventHandler(WidgetContainer_DoubleClick);
            this.Click += new EventHandler(WidgetContainer_Click);
            view.Enter += new EventHandler(ViewGotFocus);
            view.Leave += new EventHandler(ViewLostFocus);
            this.AttachClickEventListener(view);

            //Obtain the list of events the control can fire and the list of event handlers
            //that it has
            this.EventHandlers = GetEventHandlerForAttributes(this.View);
            this.EventsItCanFire = FindEvents(this.View);
        }

        /// <summary>
        /// Reset the widget canvas layout when the user clicks on the group box title.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void WidgetContainer_DoubleClick(object sender, EventArgs e)
        {
            //Reset allowed only when the canvas is unlocked
            if (_containerCanvas != null && !_containerCanvas.Locked)
            {
                _containerCanvas.ResetWidgetLayout();
            }
        }   

        /// <summary>
        /// Recursively go over all the controls and attach the click listener to
        /// the WidgetContainer_Click handler
        /// </summary>
        /// <param name="control"></param>
        private void AttachClickEventListener(Control control)
        {
            foreach (Control c in control.Controls)
            {
                c.Click += WidgetContainer_Click;
                AttachClickEventListener(c);
            }
        }//AttachClickEventListener


        //Recursively go over all the controls and remove the click listener to
        //the WidgetContainer_Click handler
        private void RemoveClickEventListener(Control control)
        {
            foreach (Control c in control.Controls)
            {
                c.Click -= WidgetContainer_Click;
            }
        }//RemoveClickEventListener

        //Set the position of the maximize button correctly on the UI
        private void AdjustMaximizeButtonPosition()
        {
            this._maximizeButton.Enabled = this.AllowMaximize;
            this._maximizeButton.Visible = this.AllowMaximize;

            if (false == this.AllowMaximize)
                return;

            int offset = this._maximizeButton.Size.Width;
            if (this.AllowDelete)
            {
                offset += this._deleteButton.Size.Width;
                offset += buttonSpacing;
            }

            Point loc = new System.Drawing.Point(base.Width - offset, Padding.Top - 8);
            this._maximizeButton.Location = loc;
        }


        /**
         *   Resize direction -- gets the point location within this rectangle.
         *
         *  		|--|------------------|--|
         *  		|  |                  |  |
         *  		|--|------------------|--|
         *  		|  |                  |  |
         *  		|  |                  |  |
         *  		|  |                  |  |
         *  		|--|------------------|--|
         *  		|  |                  |  |
         *  		|--|------------------|--|
         *
         */
        private int GetResizeDirection(Point position)
        {
            Rectangle borderRect;

            int leftPad = base.Padding.Left;
            int rightPad = base.Padding.Right;
            int topPad = base.Padding.Top + base.Font.Height;
            int bottomPad = base.Padding.Bottom;

            /**
             *  Check the right border first.
             */
            borderRect = new Rectangle(base.Width - rightPad, topPad,
                                      rightPad, base.Height - topPad - bottomPad);
            if (borderRect.Contains(position))
                return NativeUser32Wrapper.DIR_RIGHT;


            /**
             *  Check the bottom border next.
             */
            borderRect = new Rectangle(leftPad, base.Height - bottomPad,
                                       base.Width - leftPad - rightPad, bottomPad);
            if (borderRect.Contains(position))
                return NativeUser32Wrapper.DIR_BOTTOM;


            /**
             *  Check the bottom right corner next.
             */
            borderRect = new Rectangle(base.Width - rightPad, base.Height - bottomPad,
                                       rightPad, bottomPad);
            if (borderRect.Contains(position))
                return NativeUser32Wrapper.DIR_BOTTOMRIGHT;


            /**
             *  Check the top right border next.
             */
            borderRect = new Rectangle(base.Width - rightPad, 0, rightPad, topPad);
            if (borderRect.Contains(position))
                return NativeUser32Wrapper.DIR_TOPRIGHT;


            /**
             *  Check the top border next.
             */
            borderRect = new Rectangle(leftPad, 0, base.Width - leftPad - rightPad, topPad);
            if (borderRect.Contains(position))
                return NativeUser32Wrapper.DIR_TOP;


            /**
             *  Check the left border next.
             */
            borderRect = new Rectangle(0, topPad, leftPad, base.Height - topPad - bottomPad);
            if (borderRect.Contains(position))
                return NativeUser32Wrapper.DIR_LEFT;


            /**
             *  Check the top left border next.
             */
            borderRect = new Rectangle(0, 0, leftPad, topPad);
            if (borderRect.Contains(position))
                return NativeUser32Wrapper.DIR_TOPLEFT;


            /**
             *  Check the bottom left border next.
             */
            borderRect = new Rectangle(0, base.Height - bottomPad, leftPad, bottomPad);
            if (borderRect.Contains(position))
                return NativeUser32Wrapper.DIR_BOTTOMLEFT;

            /**
             *  Check if its inside the widget.
             */
            borderRect = new Rectangle(leftPad, topPad, base.Width - leftPad - rightPad,
                                       base.Height - bottomPad - topPad);
            if (borderRect.Contains(position))
                return NativeUser32Wrapper.DIR_INSIDE;

            /**
             *  Hmm, shouldn't happen -- not in the window, just return -1 for now.
             */
            return -1;

        }  /*  End of  GetResizeDirection  method.  */

        //Event handler for the control contained in this container
        private void ViewGotFocus(object sender, EventArgs e)
        {
            if (!isActive)
            {
                // If any of our controls got focus, the container needs to be
                // activated too.  But, do it only if we're not active.
                _amActive = true;
                ActivateMe();
            }
        }

        //Event handler for the control contained in this container
        private void ViewLostFocus(object sender, EventArgs e)
        {
            if (isActive)
            {
                _amActive = false;
            }
        }

        //The goal of this method is to harvest all the EventHandlerForAttributes specified in this
        //class
        private List<EventHandlerForAttribute> GetEventHandlerForAttributes(Object obj)
        {
            List<EventHandlerForAttribute> attributeList = new List<EventHandlerForAttribute>();
            Type type = obj.GetType();
            MethodInfo[] myArrayMethodInfo = type.GetMethods(BindingFlags.Public | BindingFlags.Instance | BindingFlags.DeclaredOnly);
            for (int i = 0; i < myArrayMethodInfo.Length; i++)
            {
                Object[] EventHandlers = Attribute.GetCustomAttributes(myArrayMethodInfo[i]);
                if (EventHandlers != null)
                {
                    //Console.WriteLine("Array length is " + EventHandlers.Length);
                    for (int j = 0; j < EventHandlers.Length; j++)
                    {
                        if (EventHandlers[j].GetType() ==  typeof(EventHandlerForAttribute))
                        {
                            EventHandlerForAttribute attr = (EventHandlerForAttribute)EventHandlers[j];
                            attr.Method = myArrayMethodInfo[i];                            
                            attributeList.Add(attr);
                            //Console.WriteLine( EventHandlers[j].GetType().Name + " -- " + EventHandlers[j]);
                        }
                    }
                }
            }
            return attributeList;
        }


        private void ResizeGroupBox(double wFactor, double hFactor)
        {
            if (false == this.Resizable)
                return;

            int xPos = this.Location.X;
            int yPos = this.Location.Y;

            int newWidth = (int)(this.Width * wFactor);
            this.Width = Math.Max(newWidth, this.MinimumSize.Width);


            int newHeight = (int)(this.Height * hFactor);
            this.Height = Math.Max(newHeight, this.MinimumSize.Height);

            if (null != this.Parent)
            {
                if (this.Location.X + this.Width > this.Parent.Width)
                    xPos = this.Parent.Width - this.Width - 3;

                xPos = Math.Max(0, xPos);

                if (this.Location.Y + this.Height > this.Parent.Height)
                    yPos = this.Parent.Height - this.Height - 3;

                yPos = Math.Max(0, yPos);
                this.Location = new Point(xPos, yPos);
            }

        }

        #endregion  /*  End of region : Private Methods.  */
        
        #region Inner Classes

        private class NativeUser32Wrapper
        {

            #region user32.dll Imports

            /**
			 *  Send a specific message to a window or windows.
			 */
            [DllImport("user32.dll", EntryPoint = "SendMessage")]
            public static extern int SendMessage(IntPtr hWnd, int msg, int wParam, int lParam);


            /**
             *  Release the mouse capture from a window and resume normal mouse input processing.
             */
            [DllImport("user32.dll", EntryPoint = "ReleaseCapture")]
            public static extern int ReleaseCapture(IntPtr hwnd);

            #endregion  /*  End of region : user32.dll Imports.  */


            #region Defines_in_WinUser.h

            /**
			 *  Codes for WM_SYSCOMMAND, SC_SIZE, DIR_{LEFT,TOP,BOTTOM,RIGHT} flag values.
			 */

            public const int WinUser_WM_SYSCOMMAND = 0x0112;
            public const int WinUser_SC_SIZE = 0xF000;

            public const int DIR_INSIDE = 0x0000;
            public const int DIR_LEFT = 0x0001;
            public const int DIR_RIGHT = 0x0002;
            public const int DIR_TOP = 0x0003;
            public const int DIR_BOTTOM = 0x0006;

            public const int DIR_TOPLEFT = (DIR_TOP + DIR_LEFT);
            public const int DIR_TOPRIGHT = (DIR_TOP + DIR_RIGHT);
            public const int DIR_BOTTOMLEFT = (DIR_BOTTOM + DIR_LEFT);
            public const int DIR_BOTTOMRIGHT = (DIR_BOTTOM + DIR_RIGHT);

            #endregion  /*  End of region : Defines_in_WinUser.h.  */



        }  /*  End of  inner class  NativeUser32Wrapper.  */


        #endregion  /*  End of region : Inner Classes.  */

    }  /*  End of  class  WidgetContainer.  */
}
