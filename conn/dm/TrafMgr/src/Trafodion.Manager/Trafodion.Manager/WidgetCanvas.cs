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


using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.Reflection;
using System.Windows.Forms.Layout;
using System.IO;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.Framework
{
    /// <summary>
    /// This class shall be used as a work space to display User Controls. It will 
    /// manage how widgets float in the panel
    /// </summary>
    public partial class WidgetCanvas : UserControl
    {
        #region  Members
        /// <summary>
        /// Variables used to indicate if a widget is being added or being removed from a 
        /// container.
        /// </summary>
        public enum Operations { Add = 1, Delete };

        /// <summary>
        /// The layout manager that will be used when the workspace is first loaded.
        /// The user can then drag the controls to their appropriate position according
        /// to their need.
        /// </summary>
        private ILayoutManager _layoutManager;
        
        /// <summary>
        /// The list of widgets that have been added to this panel
        /// </summary>
        [NonSerialized]
        private Hashtable _widgets = new Hashtable();

		/// <summary>
        /// The types of widgets and linkages to the actual widgets added to this view.
		/// </summary>
		[NonSerialized]
		private Hashtable _widgetTypes = new Hashtable();
        
        /// <summary>
        /// Contains WidgetModel objects for the widgets that have been placed in the 
        /// panel
        /// </summary>
        private Hashtable _widgetsModel = new Hashtable();

        /// <summary>
        /// Flag indicating whether or not this Workspace View is locked.
        /// </summary>
        private bool _locked = false;

        /// <summary>
        /// In case of multiple work spaces, determines the order in which they
        /// are loaded.
        /// </summary>
        private int _viewNum = 0;

        /// <summary>
        /// The name given to the workspace. This is very important because this will also be used as a
        /// key for persistance.
        /// </summary>
        private string _viewName = null;

        /// <summary>
        /// Textual description of the workspace
        /// </summary>
        private string _viewText = null;

        /// <summary>
        /// Flag indicating whether or not this Workspace can be deleted.
        /// </summary>
        private bool _allowDeletion = true;


        /// <summary>
        /// Current active widget in this view. The one with the focus on it.
        /// </summary>
        private WidgetContainer _activeWidget = null;

        /// <summary>
        /// The key used by the persistance sub-system to persist the state of 
        /// the canvas.
        /// </summary>
        private string _thePersistenceKey = null;

        //Menu items that will pertain to all canvas
        /// <summary>
        /// The menu item that shall be used to reset the layout of the canvas
        /// to the default layout that was specified at design time.
        /// </summary>
        private TrafodionToolStripMenuItem theResetLayoutMenuItem = null;

        /// <summary>
        /// The menu item that shall be used to Lock/Unlock a canvas and all the 
        /// containers in it.
        /// </summary>
        private TrafodionToolStripMenuItem theLockStripMenuItem = null; 

        /// <summary>
        /// Saves the default color so that it can be toggled between the locked and default mode
        /// </summary>
        private Color _theDefaultColor;

        /// <summary>
        /// Keeps tracking the background color when it is locked.
        /// </summary>
        private Color _theLockBackColor;

        /// <summary>
        /// Keeps tracking the background color when it is unlocked.
        /// </summary>
        private Color _theUnlockBackColor;

        #endregion  /*  End of region : Members.  */

        #region Constructors
        /// <summary>
        /// The default constructor
        /// </summary>
        public WidgetCanvas()
        {
            InitializeComponent();
            //InitializeCanvas(null);

            this._theDefaultColor = BackColor;
            //this._theUnlockBackColor = System.Drawing.Color.PowderBlue;
            this._theUnlockBackColor = System.Drawing.Color.Azure;
            this._theLockBackColor = BackColor;
            //create the menu items

            theLockStripMenuItem = new TrafodionToolStripMenuItem();
            this.theLockStripMenuItem.Text = (this.Locked) ? global::Trafodion.Manager.Properties.Resources.MenuUnlock : global::Trafodion.Manager.Properties.Resources.MenuLock;
            this.theLockStripMenuItem.Click += onLockUnlock;

            theResetLayoutMenuItem = new TrafodionToolStripMenuItem();
            theResetLayoutMenuItem.Text = "Reset Layout";
            theResetLayoutMenuItem.Click += onLayoutReset;

        }

        /// <summary>
        /// Constructs the canvas with the canvas model
        /// </summary>
        /// <param name="aCanvasModel"></param>
        public WidgetCanvas(CanvasModel aCanvasModel)
        {
            InitializeComponent();
            //This call has been commented. InitializeCanvas is now a public method and
            //should be called after the canvas layout has been done.
            //InitializeCanvas(aCanvasModel);
        }
        #endregion  /*  End of region : Constructors.  */

        #region  Properties

        public ILayoutManager LayoutManager
        {
            get { return _layoutManager; }
            set { _layoutManager = value; }
        }

        public bool Locked
        {
            get { return this._locked; }
            set { this._locked = value; LockWidgets(value); }
        }
        public bool AllowDelete
        {
            get { return this._allowDeletion; }
            set { this._allowDeletion = value; }
        }  
        public int ViewNum
        {
            get { return this._viewNum; }
            set { this._viewNum = value; }
        }
        public string ViewName
        {
            get { return this._viewName; }
            set { this._viewName = value; }
        }
        public string ViewText
        {
            get { return this._viewText; }
            set { this._viewText = value; }
        }  
        public WidgetContainer ActiveWidget
        {
            get { return this._activeWidget; }
            set { ActivateWidget(value); }

        }  
        public Hashtable Widgets
        {
            get { return this._widgets; }
        } 
        public Hashtable WidgetsModel
        {
            get { return this._widgetsModel; }
            set { this._widgetsModel = value; }
        }

        public string ThePersistenceKey
        {
            get { return _thePersistenceKey; }
            set { _thePersistenceKey = value; }
        }

        public TrafodionToolStripMenuItem ResetLayoutMenuItem
        {
            get 
            {
                this.theResetLayoutMenuItem.Enabled = !this.Locked;
                return theResetLayoutMenuItem; 
            }
        }

        public TrafodionToolStripMenuItem LockMenuItem
        {
            get 
            {
                this.theLockStripMenuItem.Text = (this.Locked) ? global::Trafodion.Manager.Properties.Resources.MenuUnlock : global::Trafodion.Manager.Properties.Resources.MenuLock;
                return theLockStripMenuItem; 
            }
        }

        public Color LockBackColor
        {
            get { return _theLockBackColor; }
            set { _theLockBackColor = value; }
        }

        public Color UnlockBackColor
        {
            get { return _theUnlockBackColor; }
            set { _theUnlockBackColor = value; }
        }

        #endregion  /*  End of region  Properties.  */

        #region IOptionsProvider
        ///TODO: We have to re-visit this code. It has a problem because we might have to 
        ///create a hierarchy for each widget inside a canvas. The current code will work
        ///but will not be very user friendly 
        /// <summary>
        /// Property that the framework reads to get the options control
        /// It aggregates all of the option controls from the widgets and returns it
        /// </summary>
        public List<IOptionControl> OptionControls
        {
            get
            {
                
                List<IOptionControl> list = new List<IOptionControl>();
                foreach (DictionaryEntry de in _widgets)
                {
                    WidgetContainer widgetContainer = (WidgetContainer)de.Value;
                    IOptionsProvider optionProvider = widgetContainer.View as IOptionsProvider;
                   
                    if (optionProvider != null)
                    {
                        if (!hasOnlyOneOption(optionProvider))
                        {
                            throw new Exception(widgetContainer.Name + " has more than one option. Hence cannot be displayed");
                        }
                        list.AddRange(optionProvider.OptionControls);
                    }
                } 
                return list;
            }
        }
        /// <summary>
        /// Aggregates all of the OptionObjects from the widgets and returns it
        /// </summary>
        public Dictionary<String, IOptionObject> OptionObjects
        {            
            get
            {
                Dictionary<String, IOptionObject> optionObjects = new Dictionary<String, IOptionObject>();
                foreach (DictionaryEntry de in _widgets)
                {
                    WidgetContainer widgetContainer = (WidgetContainer)de.Value;
                    IOptionsProvider optionProvider = widgetContainer.View as IOptionsProvider;
                    if (optionProvider != null)
                    {
                        if (!hasOnlyOneOption(optionProvider))
                        {
                            throw new Exception(widgetContainer.Name + " has more than one option. Hence cannot be displayed");
                        }
                        foreach (KeyValuePair<String, IOptionObject> optionObjectsDE in optionProvider.OptionObjects)
                        {
                            String key = optionObjectsDE.Key;
                            if (! optionObjects.ContainsKey(key))
                            {
                                optionObjects.Add(key, optionObjectsDE.Value);
                            }
                        }
                    }
                }
                return optionObjects;
            }
        }
        #endregion

        #region  Public Methods

        /// <summary>
        /// Overloaded method to initialize the canvas if the model is not available
        /// </summary>
        /// <param name="aCanvasModel"></param>
        public void InitializeCanvas()
        {
            InitializeCanvas(null);
        }
        /// <summary>
        /// This method initializes the canvas from the canvas model
        /// </summary>
        /// <param name="aCanvasModel"></param>
        public void InitializeCanvas(CanvasModel aCanvasModel)
        {
            if (aCanvasModel != null)
            {
                Locked = aCanvasModel.Locked;
                ViewName = aCanvasModel.ViewName;
                ViewText = aCanvasModel.ViewText;
                AllowDelete = aCanvasModel.AllowDelete;
            }
            //Add the persistance handler
            Persistence.PersistenceHandlers += new Persistence.PersistenceHandler(TrafodionMain_Persistence);

            //Load the persistance values as saved currently
            LoadFromPersistance();

        }

        /// <summary>
        /// Returns the next widget from the canvas. 
        /// </summary>
        /// <param name="aCurrentWidget"></param>
        /// <returns></returns>
		public WidgetContainer FindNextWidget(WidgetContainer aCurrentWidget)
		{
			if (null == aCurrentWidget)
			{
                if (_widgets.Count > 0)
                {
                    return (WidgetContainer)_widgets[0];
                }

				return null;
			}

			bool found = false;
			foreach (DictionaryEntry de in _widgets)
			{
				WidgetContainer widgetContainer = (WidgetContainer) de.Value;
                if (found)
                {
                    return widgetContainer;
                }

                if (widgetContainer == aCurrentWidget)
                {
                    found = true;
                }
			}

			return null;

		}  /*  End of  findNextWidget.  */


        /// <summary>
        /// Given a widget in a canvas, returns the previous widget
        /// </summary>
        /// <param name="aCurrentWidget"></param>
        /// <returns></returns>
		public WidgetContainer FindPreviousWidget(WidgetContainer aCurrentWidget)
		{
			if (null == aCurrentWidget)
			{
                if (_widgets.Count > 0)
                {
                    return (WidgetContainer)_widgets[0];
                }
				return null;
			}

			WidgetContainer prevWidget = null;
			foreach (DictionaryEntry de in _widgets)
			{
				WidgetContainer widgetContainer = (WidgetContainer)de.Value;
				if (widgetContainer == aCurrentWidget)
				{
					return prevWidget; //If currentWidget is the first widget null will be returned
				}

			    prevWidget = widgetContainer;
			}

			return prevWidget;

		}  /*  End of  findPreviousWidget.  */


        /// <summary>
        /// This method shall be used to remove all elements from the canvas 
        /// </summary>
        /// <returns></returns>
        public bool DeleteCanvas()
        {
            if (!AllowDelete)
            {
                return false;
            }

            if (Locked)
            {
                throw new Exception("Cannot delete as the workspace is currently locked. ");
            }

            foreach (DictionaryEntry de in _widgets)
            {
                WidgetContainer widgetContainer = (WidgetContainer)de.Value;

                //Use the delete method in the widget container to release
                //all resources before the widget is removed from the container.
                widgetContainer.DeleteWidget();
                RemoveWidget(widgetContainer);
            }

            //Clear the local cache
            _widgets.Clear();
            _widgetTypes.Clear();
            _widgetsModel.Clear(); 

            return true;
        }  /*  End of  delete  method.  */


        /// <summary>
        /// This method shall be used to ResizeWidget the widget containers in a canvas using the
        /// ResizeWidget factors provided
        /// </summary>
        /// <param name="aWidthFactor"></param>
        /// <param name="aHeightFactor"></param>
        public void ResizeWidgets(double aWidthFactor, double aHeightFactor)
        {
            foreach (DictionaryEntry de in _widgets)
            {
                WidgetContainer widgetContainer = (WidgetContainer)de.Value;
                widgetContainer.ResizeWidget(aWidthFactor, aHeightFactor);
            }

        }  /*  End of  ResizeWidget  method.  */


        /// <summary>
        /// Given a widget name, returns if the widget is present in the canvas
        /// </summary>
        /// <param name="aName"></param>
        /// <returns></returns>
        public  bool  ContainsWidget(String aName) 
        {
            return _widgets.ContainsKey(aName);

        }  /*  End of  ContainsWidget  method.  */

        /// <summary>
        /// Given a widget name, returns all widgets of that type
        /// </summary>
        /// <param name="aName"></param>
        /// <returns></returns>
        public List<WidgetContainer> FindWidgetsOfType(String aName)
        {
            return (List<WidgetContainer>)_widgetTypes[aName];

        }  /*  End of  FindWidgetsOfType  method.  */

        
        /// <summary>
        /// 
        /// </summary>
        /// <param name="aWidgetContainer"> The container that would be added to the canvas</param>
        /// <param name="aLayoutConstraint">The constraint needed by the layout manager</param>
        /// <param name="aIndex"> The index where the container will be inserted in the canvas.
        ///                       Currently it's not being used</param>
        public void AddWidget(WidgetContainer aWidgetContainer, Object aLayoutConstraint, int aIndex)
        {
            if (null == aWidgetContainer)
            {
                return;
            }

            //Save a copy of the default properties as defined in code or configuration
            aWidgetContainer.DefaultModel = new WidgetModel(aWidgetContainer);
            aWidgetContainer.DefaultModel.LayoutConstraint = aLayoutConstraint;


            lock (_widgetTypes)
            {
                String widgetType = (String)aWidgetContainer.WidgetType;
                List<WidgetContainer> widgetList = null;
                if (_widgetTypes.ContainsKey(widgetType))
                {
                    widgetList = (List<WidgetContainer>)_widgetTypes[widgetType];
                }
                else
                {
                    widgetList = new List<WidgetContainer>();
                    _widgetTypes.Add(widgetType, widgetList);
                }
                widgetList.Add(aWidgetContainer);

                //If the name of the container is not set, generate one
                if ((null == aWidgetContainer.Name) || (0 >= aWidgetContainer.Name.Trim().Length))
                {
                    // Generate a unique name for this widget.
                    String suffix = (1 + _widgets.Count) + DateTime.Now.ToBinary().ToString();
                    aWidgetContainer.Name = Name + "_Widget_" + suffix;
                }

                String widgetHandle = aWidgetContainer.Name;
                if (_widgets.ContainsKey(widgetHandle))
                {
                    WidgetContainer oldwidget = (WidgetContainer)_widgets[widgetHandle];
                    RemoveWidget(oldwidget);
                }

                WidgetModel wm = aWidgetContainer.GetWidgetModel();

                if (!_widgetsModel.Contains(widgetHandle))
                {
                    _widgetsModel.Add(widgetHandle, wm);
                }

                ActiveWidget = aWidgetContainer;
                _widgets.Add(widgetHandle, aWidgetContainer);
            }
            
           
            //Add the widget onto the canvas
            AddContainerToCanvas(aWidgetContainer, aLayoutConstraint, aIndex);

            //Associate the evnt handlers of the widget to all the other controls
            //in the canvas
            AssociateEventsAndDelegates(aWidgetContainer);

            //Associate the close event handler of the container to the canvas
            aWidgetContainer.WidgetContainerClosedEvent += OnWidgetContainerClosedEvent;

            aWidgetContainer.WidgetContainerMovedEvent += OnWidgetContainerMovedEvent;

            //Activate the container
            ActivateWidget(aWidgetContainer);
            //aWidgetContainer.activate();
        }


        /// <summary>
        /// Adds a widget container to the canvas.
        /// </summary>
        /// <param name="aWidgetContainer"></param>
        public void AddWidget(WidgetContainer aWidgetContainer) 
        {
            this.AddWidget(aWidgetContainer, null, -1);

		}  


        /// <summary>
        /// This method prints all the containers in the canvas and the eventlisteners
        /// that are associated with them.
        /// 
        /// This is a function that is purely used for testing.
        /// </summary>
        public void printStats()
        {
            foreach (DictionaryEntry element in Widgets)
            {
                WidgetContainer container = (WidgetContainer)element.Value;
                Console.WriteLine(container.View);
            }
        }

        /// <summary>
        /// Removes the active method from the Canvas.
        /// </summary>
        /// <returns></returns>
		public  bool  DeleteActiveWidget() {
			if (RemoveWidget(ActiveWidget) ) 
            {
				return  true;
			}

			return  false;

		}  /*  End of  DeleteActiveWidget  method.  */

        /// <summary>
        /// Handles the event that gets fired when the user closes a widget container in the
        /// canvas.
        /// </summary>
        /// <param name="source"></param>
        /// <param name="args"></param>
        public void OnWidgetContainerClosedEvent(Object source, EventArgs args)
        {
            RemoveWidget((WidgetContainer)source);
        }


        /// <summary>
        ///  Handles the WidgetContainerMovedEvent from the container. This method can be used to 
        ///  implement docking after a container is moved
        ///  
        ///  Note: We don't want the docking feature for now. We will  leave it for a future release
        /// </summary>
        /// <param name="source"></param>
        /// <param name="args"></param>
        public void OnWidgetContainerMovedEvent(Object source, EventArgs args)
        {
            //WidgetContainer container = source as WidgetContainer;
            //if (container != null)
            //{
            //    Rectangle containerBound = container.Bounds;
            //    Point topLeft       = this.Location;
            //    Point topRight      = new Point(this.Location.X + this.Width, this.Location.Y);
            //    Point bottomLeft    = new Point(this.Location.X , this.Location.Y + this.Height);
            //    Point bottomRight = new Point(this.Location.X + this.Width, this.Location.Y + this.Height);

            //    //deal with the sides

            //    //top
            //    DockStyle dockStyle = DockStyle.None;
            //    if (containerBound.Location.Y <= topLeft.Y)
            //    {
            //        dockStyle = DockStyle.Top;
            //    }
            //    //bottom
            //    else if (containerBound.Location.Y + containerBound.Height >= bottomLeft.Y )
            //    {
            //        dockStyle = DockStyle.Bottom;
            //    }
            //    //left
            //    else if (containerBound.Location.X <= topLeft.X)
            //    {
            //        dockStyle = DockStyle.Left;
            //    }
            //    //right
            //    else if (containerBound.Location.X + containerBound.Width >= topRight.X)
            //    {
            //        dockStyle = DockStyle.Right;
            //    }
            //    container.Dock =  dockStyle;
            //}
        }

        /// <summary>
        /// Activates the widget container specified 
        /// </summary>
        /// <param name="aWidgetContainer"></param>
        public void ActivateWidget(WidgetContainer aWidgetContainer)
        {
            if (aWidgetContainer != null)
            {
                if ((null != _activeWidget) && (aWidgetContainer != _activeWidget))
                {
                    _activeWidget.DeactivateWidget();
                }
                _activeWidget = aWidgetContainer;
                _activeWidget.ActivateWidget();
            }
        }


        /// <summary>
        /// Persists the state of the Canvas
        /// </summary>
        /// <param name="aDictionary"></param>
        /// <param name="aPersistenceOperation"></param>
        void TrafodionMain_Persistence(Dictionary<string, object> aDictionary, Persistence.PersistenceOperation aPersistenceOperation)
        {
            if ((ThePersistenceKey == null) || (ThePersistenceKey.Trim().Length == 0))
            {
                return;
            }

            switch (aPersistenceOperation)
            {
                case Persistence.PersistenceOperation.Load:
                    {
                        LoadFromPersistance();
                        break;
                    }
                case Persistence.PersistenceOperation.Save:
                    {
                        //RebuildWidgetsModel();
                        if ((ThePersistenceKey != null) && (ThePersistenceKey.Trim().Length > 0))
                        {
                            aDictionary[ThePersistenceKey] = new WidgetCanvasModel(this);
                        }
                        break;
                    }
            }
        }

        /// <summary>
        /// Explicit call to persist Widget Canvas model
        /// </summary>
        public void SaveToPersistence()
        {
            if ((ThePersistenceKey != null) && (ThePersistenceKey.Trim().Length > 0))
            {
                Persistence.Put(ThePersistenceKey, new WidgetCanvasModel(this));
            }
        }

        /// <summary>
        /// Method to apply the default layouts of all the widget containers in the canvas
        /// </summary>
        public void ResetWidgetLayout()
        {
            foreach (DictionaryEntry de in _widgets)
            {
                WidgetContainer widgetContainer = (WidgetContainer)de.Value;
                widgetContainer.Dock = DockStyle.None;
                if (widgetContainer.DefaultModel != null)
                {
                    widgetContainer.LoadModelInformation(widgetContainer.DefaultModel);
                    //If we are dealing with the active widget, we need to call activate on that
                    if (widgetContainer.DefaultModel.IsActive)
                    {
                        this.ActivateWidget(widgetContainer);
                    }
                }
            }

            RedrawWidgetsOnCanvas();
        }

        /// <summary>
        /// The _widgetsModel gets re-created
        /// </summary>
        public void RebuildWidgetsModel()
        {
            _widgetsModel.Clear();
            foreach (DictionaryEntry de in _widgets)
            {
                WidgetContainer widgetContainer = (WidgetContainer)de.Value;
                String widgetHandle = widgetContainer.Name;
                WidgetModel wm = widgetContainer.GetWidgetModel();

                if (!_widgetsModel.Contains(widgetHandle))
                {
                    _widgetsModel.Add(widgetHandle, wm);
                }
            }
        }

		#endregion  /*  End of region  Public Methods.  */
        
        #region  Private Methods
        private static bool hasOnlyOneOption(IOptionsProvider optionsProvider)
        {
            //int optionsCount = (hasOptionObjects(optionsProvider) ? optionsProvider.OptionObjects.Count : 0)
            //    + (hasOptionControls(optionsProvider) ? optionsProvider.OptionControls.Count : 0);
            //if (optionsCount == 1)
            //{
            //    return true;
            //}
            return false;
        }

        /// <summary>
        /// Method to load the saved layout from the persistenace framework and apply it to 
        /// the canvas and all its containers.
        /// </summary>
        private void LoadFromPersistance()
        {
            if (ThePersistenceKey != null)
            {
                WidgetCanvasModel canvasModel = null;
                try
                {
                    canvasModel =  Persistence.Get(ThePersistenceKey) as WidgetCanvasModel;
                }
                catch(PersistenceNotLoadedException ex)
                {
                    //can't do anything about that
                }

                if (canvasModel != null)
                {
                    Hashtable models = canvasModel.WidgetModels;
                    this.Locked = canvasModel.Locked;
                    this.ViewName = canvasModel.ViewName;

                    if (models != null)
                    {
                        foreach (DictionaryEntry de in _widgets)
                        {                            
                            WidgetContainer widgetContainer = (WidgetContainer)de.Value;
                            String widgetHandle = widgetContainer.Name;
                            WidgetModel wm = models[widgetHandle] as WidgetModel;

                            if (wm != null)
                            {
                                widgetContainer.LoadModelInformation(wm);

                                //If we are dealing with the active widget, we need to call activate on that
                                if (wm.IsActive)
                                {
                                    this.ActivateWidget(widgetContainer);
                                }
                            }
                        }

                        LayoutWidgetsOnCanvas();
                    }
                }
            }
        }


        /// <summary>
        /// The goal of this method is to associate the event originators and the delegates
        /// for the components in the container.
        /// </summary>
        /// <param name="newWidgetContainer"></param>
        private void AssociateEventsAndDelegates(WidgetContainer newWidgetContainer)
        {
            AddDeleteEventsAndDelegates(newWidgetContainer, Operations.Add);
        }//AssociateEventsAndDelegates

        /// <summary>
        /// When a control is removed from the canvas, all the event listeners to that
        /// that control are removed and that control stops listening to all other events
        /// </summary>
        /// <param name="removedWidgetContainer"></param>
        private void RemoveDelegatesFromEvents(WidgetContainer removedWidgetContainer)
        {
            AddDeleteEventsAndDelegates(removedWidgetContainer, Operations.Delete);
        }//RemoveDelegatesFromEvents

        /// <summary>
        /// The goal of this method is to associate the event originators and the delegates
        /// for the components in the container.
        /// </summary>
        /// <param name="newWidgetContainer"> The Widget container that is being added to the canvas</param>
        /// <param name="operation">Indicates if the container is being added or being removed from the canvas</param>
        private void AddDeleteEventsAndDelegates(WidgetContainer newWidgetContainer, Operations operation)
        {
            //For all the events that the new control can fire, we have to find out the other
            //controls in the canvas that can handle those events. Then add those controls as
            //event handlers of this new control that is being added
            if ((Widgets != null) && (Widgets.Count > 0))
            {
                List<EventInfo> eventsFired = newWidgetContainer.EventsItCanFire;

                foreach (DictionaryEntry element in Widgets)
                {
                    WidgetContainer container = (WidgetContainer)element.Value;
                    List<EventHandlerForAttribute> handlers = container.EventHandlers;
                    if (handlers != null)
                    {
                        foreach (EventHandlerForAttribute handler in handlers)
                        {
                            foreach (EventInfo evt in eventsFired)
                            {
                                if (handler.EventName.Equals(evt.Name) && handler.ClassName.Equals(newWidgetContainer.View.GetType().FullName))
                                {
                                    //Console.WriteLine("Mapping event " + evt.Name + " of class " + newWidgetContainer.View.Name + " to handler method " + handler.method.Name + " of class " + container.View.GetType().FullName);
                                    try
                                    {
                                        Delegate tempDel = Delegate.CreateDelegate(evt.EventHandlerType, container.View, handler.Method.Name);
                                        if (operation == Operations.Add)
                                        {
                                            evt.AddEventHandler(newWidgetContainer.View, tempDel);
                                        }
                                        else if (operation == Operations.Delete)
                                        {
                                            evt.RemoveEventHandler(newWidgetContainer.View, tempDel);
                                        }
                                    }
                                    catch (Exception ex)
                                    {
                                        //Error encountered while doing the mapping. Ignoring the mapping
                                        //TODO: Fire a log event
                                    }
                                }
                            }
                        }
                    }
                }
            }


            //Next check all the events that the new control can handle and see if there are 
            //any existing controls in the canvas that generate them. If we find any, associate
            //them together.

            if ((Widgets != null) && (Widgets.Count > 0))
            {
                List<EventHandlerForAttribute> eventsHandeled = newWidgetContainer.EventHandlers;

                foreach (DictionaryEntry element in Widgets)
                {
                    WidgetContainer container = (WidgetContainer)element.Value;
                    List<EventInfo> eventsFired = container.EventsItCanFire;
                    if (eventsFired != null)
                    {
                        foreach (EventHandlerForAttribute handler in eventsHandeled)
                        {
                            foreach (EventInfo evt in eventsFired)
                            {
                                if (handler.EventName.Equals(evt.Name) && handler.ClassName.Equals(container.View.GetType().FullName))
                                {
                                    //Console.WriteLine("Mapping event " + evt.Name + " of class " + newWidgetContainer.View.Name + " to handler method " + handler.method.Name + " of class " + container.View.GetType().FullName);
                                    try
                                    {
                                        Delegate tempDel = Delegate.CreateDelegate(evt.EventHandlerType, newWidgetContainer.View, handler.Method.Name);
                                        if (operation == Operations.Add)
                                        {
                                            evt.AddEventHandler(container.View, tempDel);

                                        }
                                        else if (operation == Operations.Delete)
                                        {
                                            evt.RemoveEventHandler(container.View, tempDel);
                                        }
                                    }
                                    catch (Exception ex)
                                    {
                                        //invalid declaration so ignore the mapping
                                        //TODO: Fire a log event
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }//AddDeleteEventsAndDelegates

        /// <summary>
        /// Romoves a widget container from the canvas
        /// </summary>
        /// <param name="aWidgetContainer"></param>
        /// <returns> true if the widget got removed</returns>
        private bool RemoveWidget(WidgetContainer aWidgetContainer)
        {
            if (Locked || (null == aWidgetContainer))
            {
                return false;
            }

            //Remove all listeners
            RemoveDelegatesFromEvents(aWidgetContainer);

            //Remove all internal references
            _widgets.Remove(aWidgetContainer.Name);
            _widgetsModel.Remove(aWidgetContainer.Name);
            List<WidgetContainer> widgetList = (List<WidgetContainer>)_widgetTypes[(String)aWidgetContainer.WidgetType];
            if (null != widgetList)
            {
                widgetList.Remove(aWidgetContainer);
            }

            //Make the previous widget the active widget
            if (aWidgetContainer == ActiveWidget)
            {
                WidgetContainer prevWidget = FindPreviousWidget(aWidgetContainer);
                if (null != prevWidget)
                {
                    ActivateWidget(prevWidget);
                    //prevWidget.activate();
                }
                ActiveWidget = prevWidget;
            }

            return true;
        }  /*  End of  RemoveWidget  method.  */


        /// <summary>
        /// Helper method to add a Widget container to the canvas
        /// </summary>
        /// <param name="aWidgetContainer"></param>
        /// <param name="aLayoutConstraint"></param>
        /// <param name="aIndex"></param>
        private void AddContainerToCanvas(WidgetContainer aWidgetContainer, Object aLayoutConstraint, int aIndex)
        {
            //Set the layout constraint to the container
            aWidgetContainer.LayoutConstraint = aLayoutConstraint;
            LayoutWidgetsOnCanvas();
        }


        /// <summary>
        /// Given the size of a control, returns the best place to place the control on the
        /// canvas.
        /// </summary>
        /// <param name="sz"></param>
        /// <returns></returns>
        private Point FindBestPosition(Size sz)
        {
            if (1 > _widgets.Count)
            {
                return new Point(10, 10);
            }

            Rectangle[] rects = new Rectangle[_widgets.Count];
            int idx = 0;

            foreach (DictionaryEntry de in _widgets)
            {
                WidgetContainer widgetContainer = (WidgetContainer)de.Value;
                rects[idx++] = new Rectangle(widgetContainer.Location.X - Parent.Location.X,
                                             widgetContainer.Location.Y - Parent.Location.Y,
                                             widgetContainer.Width, widgetContainer.Height);
            }

            int yPos = 10;
            while (yPos + sz.Height < Parent.Height)
            {
                int xPos = 10;
                while (xPos + sz.Width < Parent.Width)
                {
                    Point startPt = new Point(xPos, yPos);
                    Point endPt = new Point(xPos + sz.Width, yPos + sz.Height);
                    bool hasPoint = false;
                    foreach (Rectangle r in rects)
                    {
                        if (r.Contains(startPt) || r.Contains(endPt))
                        {
                            hasPoint = true; break;
                        }
                    }

                    if (!hasPoint)
                        return startPt;

                    xPos += sz.Width + 20;
                }

                yPos += sz.Height + 20;
            }


            return new Point(0, 0);
        }

        /// <summary>
        /// Based on the flag, locks the widget inside the container
        /// </summary>
        /// <param name="lockFlag"></param>
        private  void  LockWidgets(bool lockFlag) 
        {
            Image backgndImg = null;
            Color backColor = _theDefaultColor;
            
            foreach (DictionaryEntry de in _widgets) 
            {
                WidgetContainer widgetContainer = (WidgetContainer)de.Value;
                widgetContainer.Resizable = !lockFlag;
				widgetContainer.Moveable = !lockFlag;
            }

            //Change the background image of the canvas depending on whether the 
            //canvas is locked or not
            if (!lockFlag)
            {
                //backgndImg = global::Trafodion.Manager.Properties.Resources.indesign;
                backColor = UnlockBackColor;
            }

            BackgroundImage = backgndImg;
            BackColor = backColor;


		}

        /// <summary>
        /// When the canvas size changes, the widgets are set in the UI by the layout manager
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void WidgetCanvas_SizeChanged(object sender, EventArgs e)
        {
            RedrawWidgetsOnCanvas();
        }


        private void LayoutWidgetsOnCanvas()
        {
            this.SuspendLayout();
            //Remove all controls from the UI
            Controls.Clear();
            //Repaint them back on the UI
            foreach (DictionaryEntry element in Widgets)
            {
                WidgetContainer container = (WidgetContainer)element.Value;
                if (_layoutManager == null)
                {
                    //just add the control
                    Controls.Add(container);
                }
                else
                {
                    if (container.LayoutChangedByUser)
                    {
                        Controls.Add(container);
                    }
                    else
                    {
                        _layoutManager.addComponent(this, container, container.LayoutConstraint, -1);
                    }
                }

                //if the widget is the active widget, activate it
                if (container.isActive)
                {
                    ActivateWidget(container);
                }
            }

            //Bring the active widget to the front
            if (_activeWidget != null)
            {
                _activeWidget.BringToFront();
            }
            this.ResumeLayout(false);

        }

        private void RedrawWidgetsOnCanvas()
        {
            foreach (DictionaryEntry element in Widgets)
            {
                WidgetContainer container = (WidgetContainer)element.Value;
                if (_layoutManager == null)
                {
                    //the widgets don't need not move. So leave them alone
                }
                else
                {
                    if (container.LayoutChangedByUser)
                    {
                        //the the user has chosen it's position, so leave it alone
                    }
                    else
                    {
                        _layoutManager.LayoutComponent(this, container, container.LayoutConstraint, -1);
                    }
                }

                //if the widget is the active widget, activate it
                if (container.isActive)
                {
                    ActivateWidget(container);
                }
            }
        }

        /// <summary>
        /// Invoked when the Reset Layout menu item is clicked
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void onLayoutReset(object sender, EventArgs e)
        {
            this.ResetWidgetLayout(); ;
        }

        /// <summary>
        /// Invoked when the Lock/Unlock menu items are clicked
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void onLockUnlock(object sender, EventArgs e)
        {
            TrafodionToolStripMenuItem menu = sender as TrafodionToolStripMenuItem;
            if (menu != null)
            {
                if (menu.Text.Equals(global::Trafodion.Manager.Properties.Resources.MenuLock))
                {
                    menu.Text = global::Trafodion.Manager.Properties.Resources.MenuUnlock;
                    this.Locked = true;                    
                }
                else if (menu.Text.Equals(global::Trafodion.Manager.Properties.Resources.MenuUnlock))
                {
                    menu.Text = global::Trafodion.Manager.Properties.Resources.MenuLock;
                    this.Locked = false;
                }
                theResetLayoutMenuItem.Enabled = ! this.Locked;
            }
        }

        #endregion  /*  End of region  Private Methods.  */      

    }
}

