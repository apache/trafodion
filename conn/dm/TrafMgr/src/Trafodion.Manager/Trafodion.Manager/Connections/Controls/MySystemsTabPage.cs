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

using System.Windows.Forms;

namespace Trafodion.Manager.Framework.Connections.Controls
{

    /// <summary>
    /// A tab page that has a user control of connection definitions.  This class can be used directly to show all of the
    /// connection definitions or subclassed to show a subset.
    /// </summary>
    public class MySystemsTabPage : Trafodion.Manager.Framework.Controls.DelayedPopulateTabPage
    {
        private ConnectionDefinition.ChangedHandler _theChangedHandler;
        private MySystemsUserControl _theMySystemsUserControl;

        /// <summary>
        /// Default Constructor
        /// </summary>
        public MySystemsTabPage()
            : base("My Systems")
        {

            // Save the user control
            _theMySystemsUserControl = new MySystemsUserControl();

        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aName">The name that goes on the tab</param>
        /// <param name="aMySystemsUserControl">The user control to show</param>
        public MySystemsTabPage(string aName, MySystemsUserControl aMySystemsUserControl)
            : base(aName)
        {

            // Save the user control
            _theMySystemsUserControl = aMySystemsUserControl;

            // Track changes in the connections
            _theChangedHandler = new ConnectionDefinition.ChangedHandler(ConnectionDefinitionChanged);
            ConnectionDefinition.Changed += _theChangedHandler;

        }

        ~MySystemsTabPage()
        {
            if (_theChangedHandler != null)
            {
                ConnectionDefinition.Changed -= _theChangedHandler;
            }
        }

        /// <summary>
        /// Report whether or not a given connection definition belongs in this tab page's user control.  Default
        /// behavior is to include all.
        /// </summary>
        /// <param name="aConnectionDefinition">The connection definition</param>
        /// <returns>True to include this connection definition in user control</returns>
        public virtual bool BelongsInThisTable(ConnectionDefinition aConnectionDefinition)
        {
            return true;
        }

        /// <summary>
        /// Report whether or not there are any systems to show.  Default
        /// behavior is to show all.
        /// </summary>
        /// <param name="aConnectionDefinition">The connection definition</param>
        /// <returns>True to include this connection definition in user control</returns>
        public virtual bool SystemsToShow()
        {
            return true;
        }

        /// <summary>
        /// Base class calls this to populate the user control
        /// </summary>
        override protected void Populate()
        {

            // No controls
            Controls.Clear();

            Control theControl;

            if (SystemsToShow())
            {

                // Populate the user control
                _theMySystemsUserControl.Populate();

                // And show it
                theControl = _theMySystemsUserControl;

            }
            else
            {

                // No systems ... show user how to add new one
                theControl = new NoSystemsUserControl();

            }

            // Fill us with the chosen control
            theControl.Dock = DockStyle.Fill;
            Controls.Add(theControl);


        }

        void ConnectionDefinitionChanged(object aSender, ConnectionDefinition aConnectionDefinition, ConnectionDefinition.Reason aReason)
        {
            Populate();
        }

    }

    /// <summary>
    /// A tab page showing the systems whose connection definitions have been tested and passed.
    /// </summary>
    public class MyActiveSystemsTabPage : MySystemsTabPage
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MyActiveSystemsTabPage()
            : base("My Active Systems", new MyActiveSystemsUserControl())
        {
        }
        
        /// <summary>
        /// Report whether or not a given connection definition belongs in this tab page's user control
        /// </summary>
        /// <param name="aConnectionDefinition">The connection definition</param>
        /// <returns>True to include this connection definition in user control</returns>
        public override bool BelongsInThisTable(ConnectionDefinition aConnectionDefinition)
        {

            // We show connection definitions that have been tested successfully
            return (aConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded);

        }

        public override bool SystemsToShow()
        {
            return (ConnectionDefinition.ActiveConnectionDefinitions.Count > 0);
        }

    }

    /// <summary>
    /// A tab page showing the systems whose connection definitions have not been tested and passed.
    /// </summary>
    public class MyOtherSystemsTabPage : MySystemsTabPage
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MyOtherSystemsTabPage()
            : base("My Other Systems", new MyOtherSystemsUserControl())
        {
        }

        /// <summary>
        /// Report whether or not a given connection definition belongs in this tab page's user control
        /// </summary>
        /// <param name="aConnectionDefinition">The connection definition</param>
        /// <returns>True to include this connection definition in user control</returns>
        public override bool BelongsInThisTable(ConnectionDefinition aConnectionDefinition)
        {

            // We show connection definitions that have  not been tested successfully
            return (aConnectionDefinition.TheState != ConnectionDefinition.State.TestSucceeded);

        }

        public override bool SystemsToShow()
        {
            return (ConnectionDefinition.OtherConnectionDefinitions.Count > 0);
        }

    }

}
