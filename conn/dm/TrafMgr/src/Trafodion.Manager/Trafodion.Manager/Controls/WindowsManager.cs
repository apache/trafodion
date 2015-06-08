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
using System.Windows;
using System.Windows.Forms;
using System.ComponentModel;
using System.Drawing;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.Framework.Controls
{

    /// <summary>
    /// The windows manager logic.  It is all static since there is only one windows manager.
    /// </summary>
    static public class WindowsManager
    {
        

        /// <summary>
        /// Non null if user if Windows menu is showing on a window that can be cloned
        /// </summary>
        static private ICloneToWindow _theClonable = null;

        // The size of the form that is showing the menu
        static Size _theFormSize;

        /// <summary>
        /// A list of all of the click handlers that we have added
        /// </summary>
        static private List<ToolStripItem> ClickHandlerAdded = new List<ToolStripItem>();

        /// <summary>
        /// The One GUi main window
        /// </summary>
        static private TrafodionForm theMainWindow = null;

        /// <summary>
        /// Bring the One GUI main winow to the front
        /// </summary>
        static public void ActivateMainWindow()
        {
            if (MainWindow != null)
            {
                MainWindow.BringToFront();
            }
        }

        /// <summary>
        /// Get or set the window that the windows manager considers to be the One GUI main window
        /// </summary>
        public static TrafodionForm MainWindow
        {
            get { return WindowsManager.theMainWindow; }
            set { WindowsManager.theMainWindow = value; }
        }

        /// <summary>
        /// Given the title, return a reference to the managed window
        /// </summary>
        /// <param name="aWindowTitle"></param>
        /// <returns></returns>
        public static ManagedWindow GetManagedWindow(string aWindowTitle)
        {
            if (Exists(aWindowTitle))
            {
                ManagedWindow aWindow;
                if (theManagedWindowsDictionary.TryGetValue(aWindowTitle, out aWindow))
                {
                    return aWindow;
                }
            }
            return null;
        }

        /// <summary>
        /// Returns a boolean to indicate if a managed window with the given title exists
        /// </summary>
        /// <param name="aWindowTitle"></param>
        /// <returns></returns>
        public static bool Exists(string aWindowTitle)
        {
            return theManagedWindowsDictionary.ContainsKey(aWindowTitle);
        }

        /// <summary>
        /// Brings a managed window to front
        /// </summary>
        /// <param name="aWindowTitle"></param>
        public static void BringToFront(string aWindowTitle)
        {
            if (Exists(aWindowTitle))
            {
                ManagedWindow aWindow;
                if (theManagedWindowsDictionary.TryGetValue(aWindowTitle, out aWindow))
                {
                    aWindow.BringToFront();
                }
            }
        }
        
        /// <summary>
        /// Restore a managed window to its only state
        /// </summary>
        /// <param name="aWindowTitle"></param>
        public static void Restore(string aWindowTitle)
        {
            if (Exists(aWindowTitle))
            {
                ManagedWindow aWindow;
                if (theManagedWindowsDictionary.TryGetValue(aWindowTitle, out aWindow))
                {
                    aWindow.Restore();
                }
            }
        }
        
        /// <summary>
        /// Given a windows menu, make sure that it has the correct list of managed windows
        /// </summary>
        /// <param name="aForm">The form owning the windows menu so that it can be disabled if it shows up in its own menu</param>
        /// <param name="aWindowsMenu">The windows menu to fix</param>
        static public void FixupWindowsMenu(TrafodionForm aForm, ToolStripMenuItem aWindowsMenu)
        {

            // Save the size of this form
            _theFormSize = aForm.Size;

            // Loop over all of the items currently in the menu.  We do this by index because we are going to 
            // be removing items from the menu as we walk it.
            for (int theIndex = 0; theIndex < aWindowsMenu.DropDownItems.Count; )
            {

                // Get the current menu item
                ToolStripItem theToolStripMenuItem = aWindowsMenu.DropDownItems[theIndex];

                // Test to see if it is a managed window
                if (theToolStripMenuItem is ManagedWindowMenuItem)
                {

                    // It is.  Remove it and don't advance the index because we need to
                    // consider the item the moved into its palce on the next iteration
                    aWindowsMenu.DropDownItems.RemoveAt(theIndex);

                }
                else
                {

                    // Thgis was not a managed window item, step past it for the next iteration
                    theIndex++;

                }
            }

            // True if there are any managed windows at this time
            bool managedWindowsExist = (theManagedWindowsDictionary.Count > 0);

            // Get the Clone This Window menu item 
            ToolStripItem theCloneThisWindowToolStripMenuItem = aWindowsMenu.DropDownItems[global::Trafodion.Manager.Properties.Resources.MenuCloneThisWindow];

            // Check to see if this windows menu has one
            if (theCloneThisWindowToolStripMenuItem != null)
            {

                // Enable it if this is a clonable window
                TrafodionForm theTrafodionForm = aForm as TrafodionForm;
                bool enableClone = (theTrafodionForm != null) && theTrafodionForm.CanCloneToWindow;
                theCloneThisWindowToolStripMenuItem.Visible = enableClone;
                _theClonable = enableClone ? theTrafodionForm.GetICloneToWindow() : null;

                // Check to see if we have added a click handler
                if (!ClickHandlerAdded.Contains(theCloneThisWindowToolStripMenuItem))
                {

                    // Add a click handler to the menu
                    theCloneThisWindowToolStripMenuItem.Click += new EventHandler(theCloneThisWindowToolStripMenuItem_Click);

                    // And remember that we did so
                    ClickHandlerAdded.Add(theCloneThisWindowToolStripMenuItem);

                }
            }

            // Get the Close All Windows Except Main menu item 
            ToolStripItem theCloseAllWindowsExceptMainWindowToolStripMenuItem = aWindowsMenu.DropDownItems[global::Trafodion.Manager.Properties.Resources.MenuCloseAllWindowsExceptMainWindow];

            // Check to see if this windows menu has one
            if (theCloseAllWindowsExceptMainWindowToolStripMenuItem != null)
            {

                // Enable it if there are managed windows that it could close
                theCloseAllWindowsExceptMainWindowToolStripMenuItem.Enabled = managedWindowsExist;

                // Check to see if we have added a click handler
                if (!ClickHandlerAdded.Contains(theCloseAllWindowsExceptMainWindowToolStripMenuItem))
                {

                    // Add a click handler to the menu
                    theCloseAllWindowsExceptMainWindowToolStripMenuItem.Click += new EventHandler(theCloseAllWindowsExceptMainWindowToolStripMenuItem_Click);

                    // And remember that we did so
                    ClickHandlerAdded.Add(theCloseAllWindowsExceptMainWindowToolStripMenuItem);

                }
            }

            // Get the Windows Manager menu item 
            ToolStripItem theWindowsManagerToolStripMenuItem = aWindowsMenu.DropDownItems[global::Trafodion.Manager.Properties.Resources.MenuWindowsManager];

            // Check to see if this windows menu has one
            if (theWindowsManagerToolStripMenuItem != null)
            {

                // This entry is always enabled
                theWindowsManagerToolStripMenuItem.Enabled = true;

                // Check to see if we have added a click handler
                if (!ClickHandlerAdded.Contains(theWindowsManagerToolStripMenuItem))
                {

                    // Add a click handler to the menu
                    theWindowsManagerToolStripMenuItem.Click += new EventHandler(theWindowsManagerToolStripMenuItem_Click);

                    // And remember that we did so
                    ClickHandlerAdded.Add(theWindowsManagerToolStripMenuItem);

                }
            }

            // Check to see if there any managed windows
            if (managedWindowsExist)
            {

                // There are.  Get the menu items with the one for the current form disabled if the current
                // form is itself a managed window.
                ManagedWindowMenuItem[] theManagedWindowMenuItems = GetManagedWindowMenuItems(aForm);

                // Add the items for the managed windows
                aWindowsMenu.DropDownItems.AddRange(theManagedWindowMenuItems);

            }

        }

        /// <summary>
        /// Called when the user selects Close All Windows Except Main Window
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        static void theCloseAllWindowsExceptMainWindowToolStripMenuItem_Click(object sender, EventArgs e)
        {
            DoCloseAllWindowsExceptMainWindow();
        }

        /// <summary>
        /// Called when the user selects Close This Window
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        static void theCloneThisWindowToolStripMenuItem_Click(object sender, EventArgs e)
        {
            DoCloneThisWindow();
        }

        private static void DoCloneThisWindow()
        {
            if (_theClonable != null)
            {
                CloneToWindow(_theFormSize, _theClonable);
            }
        }

        /// <summary>
        /// Close all windows except the main window
        /// </summary>
        private static void DoCloseAllWindowsExceptMainWindow()
        {

            // Close all of the managed windows asking the user to confirm intention to do so
            if (CloseAllManagedWindows(true))
            {

                // If the user confirmed, bring the main window to the front
                ActivateMainWindow();

            }

        }

        /// <summary>
        /// Called when the user selects Windows Manager
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        static void theWindowsManagerToolStripMenuItem_Click(object sender, EventArgs e)
        {

            // Create the windows manager form or just bring it to front if it already exists
            DoActivateWindowsManager();

        }

        /// <summary>
        /// The windows manager from
        /// </summary>
        static private WindowsManagerForm theWindowsManagerForm = null;

        /// <summary>
        /// These variables allow us to create a windows manager form in the same place on the screen as a 
        /// previous one in this session.  It is assumed that user put it where they like it.
        /// </summary>
        static bool previousWindowsManager = false;
        static Size thePreviousWindowsManagerSize;

        /// <summary>
        /// Property to get or set the windows manager form reference
        /// </summary>
        public static WindowsManagerForm WindowsManagerForm
        {
            get { return WindowsManager.theWindowsManagerForm; }
            set { WindowsManager.theWindowsManagerForm = value; }
        }

        /// <summary>
        /// Read only property giving the number of managed windows at the moment
        /// </summary>
        static public int ManagedWindowsCount
        {
            get { return theManagedWindowsDictionary.Count; }
        }

        /// <summary>
        /// Activate the windows manager form.  Create it if it doesn't exist; just bring it to the front if it does.
        /// </summary>
        static public void DoActivateWindowsManager()
        {

            // Check to see if there is already a Windows Manager
            if (WindowsManagerForm == null)
            {

                // There is none.  Check to see if there is an event handler for managed windows closing
                if (theFormClosingEventHandler == null)
                {

                    // There is not.  Create one.
                    theFormClosingEventHandler = new FormClosingEventHandler(WindowsManagerForm_FormClosing);
                }

                // Create the windows manager form.
                WindowsManagerForm = new WindowsManagerForm();

                // Add the event handler to it.
                WindowsManagerForm.FormClosing += theFormClosingEventHandler;
                                
                WindowsManagerForm.SizeChanged += new EventHandler
                    (
                        // Save the size for next time
                        (sender, e) => thePreviousWindowsManagerSize = WindowsManagerForm.Size
                    );

            }

            // Check to see if there has previously been a windows manager
            if (previousWindowsManager)
            {
                // There was one.  Put this one at the same size.
                WindowsManagerForm.Size = thePreviousWindowsManagerSize;
            }

            // Make it be normal state in case it was minimized
            WindowsManagerForm.WindowState = FormWindowState.Normal;

            // Make sure it is in front
            WindowsManagerForm.BringToFront();

            Utilities.SetCenterParent(WindowsManagerForm);

            // And show it.  It is NOT modal.
            WindowsManagerForm.Show();

        }

        /// <summary>
        /// Handler for form closing events
        /// </summary>
        static private FormClosingEventHandler theFormClosingEventHandler = null;

        /// <summary>
        /// Called when the windows manager form is closing
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        static void WindowsManagerForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            DoWindowsManagerFormClosing();
        }

        /// <summary>
        /// Handle the windows manager form closing
        /// </summary>
        private static void DoWindowsManagerFormClosing()
        {

            // Remove the event handler
            WindowsManagerForm.FormClosing -= theFormClosingEventHandler;

            // Note that a previous windows manager form ahs existed
            previousWindowsManager = true;

            // Set its state to normal if it isn't so we can get a proper reading on its postion and size
            if (WindowsManagerForm.WindowState != FormWindowState.Normal)
            {
                WindowsManagerForm.WindowState = FormWindowState.Normal;
            }

            // And note that there is no qwindow manager form now
            WindowsManagerForm = null;

        }

        /// <summary>
        /// Clone a clonable window into a managed window
        /// </summary>
        /// <param name="aSize">The size it is to have</param>
        /// <param name="anICloneToWindow">The window to be cloned</param>
        /// <returns></returns>
        static public TrafodionForm CloneToWindow(Size aSize, ICloneToWindow anICloneToWindow)
        {

            // Clone the window and pout the clone into a window
            return PutInWindow(aSize, anICloneToWindow.Clone(), anICloneToWindow.WindowTitle, anICloneToWindow.ConnectionDefn);


        }

        /// <summary>
        /// Put a control into a managed window where the Persistence event will be fired when the window is closed.
        /// </summary>
        /// <param name="aSize">The size it is to have</param>
        /// <param name="aControl">The control to fill the managed window</param>
        /// <param name="aWindowTitle">The title for the managed window</param>
        /// <param name="aNeedsPersistence">Will indicate if the persistence event should be fired</param>
        /// <param name="aConnDef">The ConnectionDefinition for the managed window</param>        
        /// <returns></returns>
        static public TrafodionForm PutInWindow(Size aSize, Control aControl, string aWindowTitle, bool aNeedsPersistence, ConnectionDefinition aConnDef)
        {
            ManagedWindow window = PutInWindow(aSize, aControl, aWindowTitle, aConnDef) as ManagedWindow;
            if (window != null)
            {
                window.NeedsPersistence = aNeedsPersistence;
            }
            return window;
        }

        static public TrafodionForm PutInWindow(Size aSize, Control aControl, string aWindowTitle, bool aNeedsPersistence, bool aIsIndependent, ConnectionDefinition aConnDef)
        {
            ManagedWindow window = PutInWindow(aSize, aControl, aWindowTitle, aConnDef) as ManagedWindow;
            if (window != null)
            {
                window.NeedsPersistence = aNeedsPersistence;
                window.IsIndependent = aIsIndependent;
            }
            return window;
        }

        /// <summary>
        /// Put a control into a managed window
        /// </summary>
        /// <param name="aSize">The size it is to have</param>
        /// <param name="aControl">The control to fill the managed window</param>
        /// <param name="aWindowTitle">The title for the managed window</param>
        /// <param name="aConnDef">The ConnectionDefinition for the managed window</param>
        /// <returns></returns>
        static public TrafodionForm PutInWindow(Size aSize, Control aControl, string aWindowTitle, ConnectionDefinition aConnDef)
        {
            // Create a managed window
            ManagedWindow theManagedWindow = new ManagedWindow();

            // Fill it with the control
            aControl.Dock = DockStyle.Fill;
            aControl.TabIndex = 0;
            theManagedWindow.Controls.Add(aControl);

            // Make the window be the desired size
            theManagedWindow.ClientSize = aSize;

            // ManagedWindow needs to have a ConnectionDefinition to enable its closing
            theManagedWindow.ConnectionDefn = aConnDef;

            string systemIdentifier = (aConnDef != null) ? aConnDef.Name + " : " : "";
            // Add our prefix to the desired title
            string theBaseTitle = TrafodionForm.TitlePrefix + systemIdentifier + aWindowTitle;

            // Use it as the beginning of a search for a title unique among current
            // instances, not over all history.
            string theUniqueTitle = theBaseTitle;

            // We add a number to the title if needed to make it unique
            for (int theIndex = 1; ; theIndex++)
            {
                // test to see if the current title candidate is currently in use
                if (!theManagedWindowsDictionary.ContainsKey(theUniqueTitle))
                {

                    // Not in use, quit the loop.
                    break;
                }

                // Otherwise create a new uniqueness candidate by incorporating the loop index
                // and try again
                theUniqueTitle = theBaseTitle + " (" + theIndex + ")";
            }

            // Use the unique title
            theManagedWindow.Text = theUniqueTitle;

            // Remember that this title is in use
            theManagedWindowsDictionary.Add(theUniqueTitle, theManagedWindow);

            // Add an event handler so that we know when the managed window has closed
            theManagedWindow.FormClosed += new FormClosedEventHandler(theForm_FormClosed);

            // Set popup window's location to be center of parent window
            Utilities.SetCenterParent(theManagedWindow);

            // Show the managed window
            theManagedWindow.Show();

            // So that menu bar doesn't obscure it
            aControl.BringToFront();

            //Add any menus from the control
            IMenuProvider menuProvider = aControl as IMenuProvider;
            if (menuProvider != null)
            {
                MenuManager menuManager = new MenuManager();
                menuManager.PopulateMainMenuBar(menuProvider, theManagedWindow.MainMenuBar, theManagedWindow.TheMainMenu);
            }

            //Perform customization of the Main tool bar
            IMainToolBarConsumer mainToolBarConsumer = aControl as IMainToolBarConsumer;
            if (mainToolBarConsumer != null)
            {
                //First turn on the managed window's tool strip
                theManagedWindow.ShowToolStripButtons = true;

                //Customize Main Tool Bar
                mainToolBarConsumer.CustomizeMainToolBarItems(theManagedWindow.TheMainToolBar);
            }

            // Add any tool buttons from the control
            IToolBarProvider toolBarProvider = aControl as IToolBarProvider;
            if (toolBarProvider != null)
            {
                TrafodionToolBarManager toolBarManager = new TrafodionToolBarManager();
                toolBarManager.PopulateMainToolBar(toolBarProvider, theManagedWindow.TheManagedWindowToolButtons, theManagedWindow.TheMainToolBar);
            }

            aControl.Select();

            // Notify interested parties that a managed window has opened
            FireOpened(theManagedWindow);

            // Pass it back
            return theManagedWindow;

        }

        /// <summary>
        /// Replaces the title of a managed window
        /// </summary>
        /// <param name="oldTitle">old title</param>
        /// <param name="newTitle">new title</param>
        static public void ReplaceManagedWindowTitle(string oldTitle, string newTitle)
        {
            ManagedWindow managedWindow = theManagedWindowsDictionary[oldTitle];
            if (managedWindow != null)
            {
                theManagedWindowsDictionary.Remove(oldTitle);
                managedWindow.Text = newTitle;
                theManagedWindowsDictionary.Add(managedWindow.Text, managedWindow);
            }
        }

        /// <summary>
        /// Close all managed windows unconditionally
        /// </summary>
        /// <returns></returns>
        static public bool CloseAllManagedWindows()
        {
            return CloseAllManagedWindows(false);
        }

        /// <summary>
        /// Close all managed windows if there are any after asking the user to confirm
        /// </summary>
        /// <param name="confirm">True to ask the user to confirm</param>
        /// <returns>False if the user canceled else true</returns>
        static public bool CloseAllManagedWindows(bool confirm)
        {

            // Just exit if there are no managed windows
            if (ManagedWindows.Count == 0)
            {

                // The user did not cancel
                return true;

            }

            // If the caller want confiramtion, ask for it
            if (confirm && (MessageBox.Show(Utilities.GetForegroundControl(), "Do you really want to close all of the windows (except the Main window)?", "Window Manager", MessageBoxButtons.YesNo) != DialogResult.Yes))
            {

                // The user canceled
                return false;

            }


            // Make copy of the dictionary of all managed windows because thereal  dictionary will be 
            // changed by the events that fire as we work
            ManagedWindow[] theManagedWindowsArray = new ManagedWindow[ManagedWindows.Count];
            ManagedWindows.CopyTo(theManagedWindowsArray, 0);

            // Loop over all of the managed windows in the copy of the dictionary
            foreach (ManagedWindow theManagedWindow in theManagedWindowsArray)
            {

                // Close each one which fire an event which will remove it from the real dictionary
                theManagedWindow.Close();

            }

            // The suer did not cancel
            return true;

        }



        /// <summary>
        /// Close all managed windows for the connection if user wants to 
        /// Also see method - CloseAllManagedWindows 
        /// </summary>
        /// <param name="aConnDef">The changed ConnectionDefinition object</param>
        /// <param name="blnCloseAll">Enforce close all windows when Disconnect/Remove a system</param>
        /// <returns></returns>
        static public void CloseAllManagedWindowsPerConnection(ConnectionDefinition aConnDef, bool blnCloseAll)
        {
            // Just exit if there are no managed windows
            if ((ManagedWindows.Count == 0) || (aConnDef == null))
            {
                // we can not really do much if connDef is null, going ahead with execution, will  result in exceptions...
                // no point in showing this internal-logic-error to user
                //MessageBox.Show(Utilities.GetForegroundControl(), "ConnectionDefinition is null, nothing can be done to close Cloned-Windows. ");
                //System.Console.WriteLine("ConnectionDefinition may be null, nothing can be done to close Cloned-Windows.");
                return;
            }
  
            // Apart from the search-criteria as to which window to close, all other logic/code in this method
            // should be consistent with the CloseAllManagedWindows()  method.

            // Make copy of the dictionary of all managed windows because the real  dictionary will be 
            // changed by the events that fire as we work
            ManagedWindow[] theManagedWindowsArray = new ManagedWindow[ManagedWindows.Count];
            ManagedWindows.CopyTo(theManagedWindowsArray, 0);

            // Loop over all of the managed windows in the copy of the dictionary
            foreach (ManagedWindow theManagedWindow in theManagedWindowsArray)
            {
                // Check if this window has same ConnDef as the connection that changed
                // ConnectionString itself is NOT unique
                if (theManagedWindow.ConnectionDefn != null && !theManagedWindow.IsIndependent)
                {
                    string managedWindowConnNameString = theManagedWindow.ConnectionDefn.Name + theManagedWindow.ConnectionDefn.ConnectionString;
                    string connNameString = aConnDef.Name + aConnDef.ConnectionString;
                    if (managedWindowConnNameString.Equals(connNameString))
                    {
                        if (!blnCloseAll &&
                            (theManagedWindow.Text.Contains(": System Monitor") ||
                             theManagedWindow.Text.Contains(": Alerts") ||
                             theManagedWindow.Text.Contains(": Live Event Viewer")))
                        {
                            //Leave these 3 windows open
                        }
                        else
                        {
                            // Close each one which fire an event which will remove it from the real dictionary
                            theManagedWindow.Close();
                        }
                    }
                }
            }
        }



        /// <summary>
        /// Checks if there are any Cloned windows opened for the given ConnectionDef  
        /// Also see method - CloseAllManagedWindows 
        /// </summary>
        /// <param name="aConnDef">The ConnectionDefinition object</param>
        /// <returns>true- when cloned windows exist. false -when no clned windows are open</returns>
        static public bool ClonedWindowsStillOpenForConnection(ConnectionDefinition aConnDef)
        {
            // Just exit if there are no managed windows
            if ((ManagedWindows.Count == 0) || (aConnDef == null))
            {
                // we can not really do much if connDef is null, going ahead with execution, will  result in exceptions...
                // no point in showing this internal-logic-error to user
                //MessageBox.Show(Utilities.GetForegroundControl(), "ConnectionDefinition is null, nothing can be done to close Cloned-Windows. ");
                //System.Console.WriteLine("ConnectionDefinition may be null, nothing can be done to close Cloned-Windows.");
                return false;
            }


            // Make copy of the dictionary of all managed windows because the real  dictionary will be 
            // changed by the events that fire as we work
            ManagedWindow[] theManagedWindowsArray = new ManagedWindow[ManagedWindows.Count];
            ManagedWindows.CopyTo(theManagedWindowsArray, 0);

            bool windowsStillOpenForConnection = false;

            // Loop over all of the managed windows in the copy of the dictionary
            foreach (ManagedWindow theManagedWindow in theManagedWindowsArray)
            {
                if (theManagedWindow.ConnectionDefn != null)
                {
                    // Check if this window has same ConnDef as the connection that changed or to be removed etc.
                    // NOTE: - ConnectionString itself is NOT unique
                    string managedWindowConnNameString = theManagedWindow.ConnectionDefn.Name + theManagedWindow.ConnectionDefn.ConnectionString;
                    string connNameString = aConnDef.Name + aConnDef.ConnectionString;
                    if (managedWindowConnNameString.Equals(connNameString))
                    {
                        windowsStillOpenForConnection = true;
                        break;
                    }
                }
            }
            return windowsStillOpenForConnection;
        }



        /// <summary>
        /// Checks if there are any Cloned windows opened for the given ConnectionDef  LIST  
        /// Also see method - CloseAllManagedWindows 
        /// </summary>
        /// <param name="aConnectionDefinitionList">The ConnectionDefinition list</param>
        /// <returns>true- when cloned windows exist. false -when no cloned windows are open</returns>
        static public bool ClonedWindowsStillOpenForConnectionList(List<ConnectionDefinition> aConnectionDefinitionList)

        {
            // Just exit if there are no managed windows
            if (ManagedWindows.Count == 0)
            {
                return false;
            }

            // leverage ClonedWindowsStillOpenForConnection()
            //
            bool windowsStillOpenForConnection = false;
            foreach (ConnectionDefinition theConnectionDefinition in aConnectionDefinitionList)
            {
                if (ClonedWindowsStillOpenForConnection(theConnectionDefinition))
                {
                    windowsStillOpenForConnection = true;
                    break;
                }
            }
            return windowsStillOpenForConnection;
        }





        /// <summary>
        /// Called when a managed window closes
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        static void theForm_FormClosed(object sender, FormClosedEventArgs e)
        {
            DoManagedWindowClosed(sender);
        }

        /// <summary>
        /// Handle a managed window closed event
        /// </summary>
        /// <param name="sender"></param>
        private static void DoManagedWindowClosed(object sender)
        {
            ManagedWindow theManagedWindow = sender as ManagedWindow;

            // Remove it from the dictionary
            theManagedWindowsDictionary.Remove(theManagedWindow.Text);

            // And notify all other intersted parties
            FireClosed(theManagedWindow);

        }

        /// <summary>
        /// Gets an array of menu items, one for each managed window, with a specified window
        /// grayed out if it is in the list.
        /// </summary>
        /// <param name="aForm">The window to gray out if present</param>
        /// <returns>The array of menu items</returns>
        static public ManagedWindowMenuItem[] GetManagedWindowMenuItems(Form aForm)
        {

            // Get the keys from the managed windows dictionary
            Dictionary<string, ManagedWindow>.KeyCollection theKeys = theManagedWindowsDictionary.Keys;

            // Create an array of menu items big enough to hold entries for all of the managed windows
            ManagedWindowMenuItem[] theMenuItems = new ManagedWindowMenuItem[theKeys.Count];

            // Loop over all of the key
            int theIndex = 0;
            foreach (string theKey in theKeys)
            {

                // Find the managed window for this key
                ManagedWindow theManagedWindow = theManagedWindowsDictionary[theKey];

                // Create a menu item for it
                ManagedWindowMenuItem theMenuItem = new ManagedWindowMenuItem(theManagedWindow);

                // The key is the text for the menu item
                theMenuItem.Text = theKey;

                // Enable to menu item unless it is for the caller-specified one
                theMenuItem.Enabled = (theManagedWindowsDictionary[theKey] != aForm);

                // All a mouse up handler to the menu item
                theMenuItem.MouseUp += new MouseEventHandler(theMenuItem_MouseUp);

                // Put the menu item into the array we're filling
                theMenuItems[theIndex++] = theMenuItem;

            }

            // Pass back the array we built
            return theMenuItems;

        }

        /// <summary>
        /// Called for mouse up in a managed window's menu item
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        static void theMenuItem_MouseUp(object sender, MouseEventArgs e)
        {
            DoMenuItemMouseUp(sender);
        }

        /// <summary>
        /// Handle mouse up in a menu item for a managed window
        /// </summary>
        /// <param name="sender"></param>
        private static void DoMenuItemMouseUp(object sender)
        {

            // Find out whioch menu item
            ManagedWindowMenuItem theManagedWindowMenuItemItem = sender as ManagedWindowMenuItem;

            // Can be null, so test it
            if (theManagedWindowMenuItemItem != null)
            {

                // Get the managed window itself from the menu item
                ManagedWindow theManagedWindow = theManagedWindowMenuItemItem.ManagedWindow;

                // And bring it to the front
                theManagedWindow.BringToFront();

            }
        }

        /// <summary>
        /// Read only property for the dictionary of managed windows
        /// </summary>
        public static Dictionary<string, ManagedWindow>.ValueCollection ManagedWindows
        {
            get
            {
                return theManagedWindowsDictionary.Values;
            }
        }

        /// <summary>
        /// Handlers for managed windows openaning and closing
        /// </summary>
        /// <param name="aManagedWindow"></param>
        public delegate void ManagedWindowOpenedHandler(ManagedWindow aManagedWindow);
        public delegate void ManagedWindowClosedHandler(ManagedWindow aManagedWindow);

        /// <summary>
        /// List of all interested parties
        /// </summary>
        static private EventHandlerList theEventHandlers = new EventHandlerList();

        /// <summary>
        /// Keys for the kinds of entries
        /// </summary>
        private static readonly string theOpenedKey = "ManagedWindowOpened";
        private static readonly string theClosedKey = "ManagedWindowClosed";

        /// <summary>
        /// The list of parties intersted in managed windows opening
        /// </summary>
        static public event ManagedWindowOpenedHandler ManagedWindowOpened
        {
            add { theEventHandlers.AddHandler(theOpenedKey, value); }
            remove { theEventHandlers.RemoveHandler(theOpenedKey, value); }
        }

        /// <summary>
        /// Tell all interested parties that a manged window has opened
        /// </summary>
        /// <param name="aManagedWindow">The managed window</param>
        static private void FireOpened(ManagedWindow aManagedWindow)
        {

            // Get the list of parties interested in opens
            ManagedWindowOpenedHandler theOpenedHandlers = (ManagedWindowOpenedHandler)theEventHandlers[theOpenedKey];

            // Check to see if there are any
            if (theOpenedHandlers != null)
            {

                // There are; multicast to all
                theOpenedHandlers(aManagedWindow);

            }
        }

        /// <summary>
        /// The list of parties intersted in managed windows closing
        /// </summary>
        static public event ManagedWindowClosedHandler ManagedWindowClosed
        {
            add { theEventHandlers.AddHandler(theClosedKey, value); }
            remove { theEventHandlers.RemoveHandler(theClosedKey, value); }
        }

        /// <summary>
        /// Tell all interested parties that a manged window has closed
        /// </summary>
        /// <param name="aManagedWindow">The managed window</param>
        static private void FireClosed(ManagedWindow aManagedWindow)
        {

            // Get the list of parties interested in closes
            ManagedWindowClosedHandler theClosedHandlers = (ManagedWindowClosedHandler)theEventHandlers[theClosedKey];

            // Check to see if there are any
            if (theClosedHandlers != null)
            {

                // There are; multicast to all
                theClosedHandlers(aManagedWindow);

            }
        }

        /// <summary>
        /// The dictionary of all managed windows keyed by their titles which therefore must be unique.
        /// </summary>
        static Dictionary<string, ManagedWindow> theManagedWindowsDictionary = new Dictionary<string, ManagedWindow>();

        /// <summary>
        /// Special class for a manu item for a managed window
        /// </summary>
        public class ManagedWindowMenuItem : ToolStripMenuItem
        {

            // Must refer to a managed window
            public ManagedWindowMenuItem(ManagedWindow aManagedWindow)
            {
                ManagedWindow = aManagedWindow;
            }

            /// <summary>
            /// Property for the managed window
            /// </summary>
            public ManagedWindow ManagedWindow
            {
                get { return theManagedWindow; }
                set { theManagedWindow = value; }
            }

            private ManagedWindow theManagedWindow;

        }

    }

}
