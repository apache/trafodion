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

using System.Collections;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Favorites;

namespace Trafodion.Manager.Framework
{

    /// <summary>
    /// This is the interface via which the main window can make functionality avaiable
    /// to the framework and other components.  This interface is implemented only by the main window.
    /// </summary>
    public interface ITrafodionMain
    {
        /// <summary>
        /// This is a method that lets any component decide if a control is currently 
        /// available in the main window.  This can be used by an component to adjust its 
        /// behavior.
        /// </summary>
        /// <param name="aControl">The control in question</param>
        /// <returns>True if the control's parent chain leads up to area
        /// controls in the navigation pane or the right pane.</returns>
        bool IsControlInActiveTrafodionArea(Control aControl);

        /// <summary>
        /// The framework wants to show a favorites folder in the right pane.
        /// </summary>
        /// <param name="aFavoritesFolder">The favorites folder to show</param>
        void ShowFavoritesFolder(FavoritesFolder aFavoritesFolder);

        /// <summary>
        /// The framework wants to make sure that the currently active area is showing in the right pane.
        /// </summary>
        void HideFavoritesFolder();

        /// <summary>
        /// The current connection definition used by the active area
        /// </summary>
        ConnectionDefinition CurrentConnectionDefinition { get; set; }
       
        /// <summary>
        /// Given the menu provider, it will populate the menu bar with the appropriate menu items
        /// </summary>
        /// <param name="menuProvider"></param>
        void PopulateMenubar(IMenuProvider menuProvider);

        /// <summary>
        /// Populates the toolbars provided the toolbar provider into the main toolbar
        /// </summary>
        /// <param name="toolBarProvider"></param>
        void PopulateToolBar(IToolBarProvider toolBarProvider);

        /// <summary>
        /// Resets the main toolbar to the default items provided by the framework
        /// </summary>
        void ResetToolBar();

        /// <summary>
        /// To Launch a event viewer
        /// </summary>
        /// <param name="theConnectionDefinition"></param>
        void LaunchEventViewer(ConnectionDefinition theConnectionDefinition, bool live);

        /// <summary>
        /// To Launch a audit log viewer
        /// </summary>
        /// <param name="theConnectionDefinition"></param>
        void LaunchAuditLogViewer(ConnectionDefinition theConnectionDefinition, bool live);

        /// <summary>
        /// To Launch audit log configuration
        /// </summary>
        /// <param name="theConnectionDefinition"></param>
        void LaunchAuditLogConfig(ConnectionDefinition theConnectionDefinition);

        /// <summary>
        /// To Launch the SQL whiteboard
        /// </summary>
        void LaunchSQLWhiteboard();

        /// <summary>
        /// To Launch the SQL whiteboard and load the given list of statements
        /// </summary>
        void LaunchSQLWhiteboard(ArrayList aListOfStatements, string from);

        /// <summary>
        /// Makes the main toolbar accessible to widgets
        /// </summary>
        MainToolBar TheMainToolBar { get; }

        Trafodion.Manager.Framework.Controls.TrafodionForm HelpContainerForm { get; }

        string ActiveAreaName { get; }
    }
}
