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
using System.Linq;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using System.Collections.Generic;
using System.Xml.Linq;

namespace Trafodion.Manager
{
    /// <summary>
    /// A singleton that shall be used to manage the common set elements across 
    /// different areas
    /// </summary>
    public class TrafodionContext
    {
        public static string LookAndFeelManagerProperty = "LookAndFeelManager";
        public static string CurrentConnectionProperty  = "CurrentConnection";        
        public static readonly string CommunityEdition = "Community"; 
        private static TrafodionContext _instance = new TrafodionContext();

        #region Member varaibles

        /// <summary>
        /// The current look and feel manager being used
        /// </summary>
        private TrafodionLookAndFeelManager _lookAndFeelManager;

        /// <summary>
        /// Reference to the current connection definition
        /// </summary>
        private ConnectionDefinition _currentConnection;

        /// <summary>
        /// These hash tables contain the severitis and components for the connected systems
        /// The key is the connection definition name and the value is List. 
        /// </summary>
        private Hashtable _severitiesForSystems = new Hashtable();
        private Hashtable _componentsForSystems = new Hashtable();

        private Hashtable _permanentSeveritiesForSystems = new Hashtable();
        private Hashtable _permanentComponentsForSystems = new Hashtable();

        /// <summary>
        /// Reference to TrafodionMain. Very useful if needed access from any control
        /// </summary>
        private ITrafodionMain _theTrafodionMain;
        private List<String> _startupSQLStatements = new List<string>();
        private string _startupMode = "Community";

        #endregion

        #region Properties
        public string StartupMode
        {
            get { return _startupMode; }
            set { _startupMode = value; }
        }
        public List<String> StartupSQLStatements
        {
            get { return _startupSQLStatements; }
            set { _startupSQLStatements = value; }
        }
        public TrafodionLookAndFeelManager LookAndFeelManager
        {
            get { return _lookAndFeelManager; }
        }
        public bool isCommunityEdition
        {
            get { return TrafodionContext.Instance.StartupMode != null && TrafodionContext.Instance.StartupMode.Equals(TrafodionContext.CommunityEdition); }
        }

        public ConnectionDefinition CurrentConnectionDefinition
        {
            get { return _currentConnection; }
            set
            {
                if (value != null)
                {
                    if ((_currentConnection == null) || (!value.Equals(_currentConnection)))
                    {
                        Object oldValue = _currentConnection;
                        _currentConnection = value;
                        firePropertyChangedEvent(CurrentConnectionProperty, oldValue, _currentConnection);
                    }
                }
            }
        }

        /// <summary>
        ///  Enable/Disable connect/disconnect toolbar based on the current connection definition's state
        /// </summary>
        /// <param name="aSender"></param>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="aReason"></param>
        void ConnectionDefinition_Changed(object aSender, ConnectionDefinition aConnectionDefinition, ConnectionDefinition.Reason aReason)
        {
            if (aReason == ConnectionDefinition.Reason.Tested || aReason == ConnectionDefinition.Reason.Disconnected)
            {
                EnableDisableConnectionToolBars();
            }
        }

        public ITrafodionMain TheTrafodionMain
        {
            get { return _theTrafodionMain; }
            set { _theTrafodionMain = value; }
        }
        #endregion

        #region Events
        /// <summary>
        /// The delegate of the method that gets called when the properties in the context are changed
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public delegate void ContextPropertyChangedEventDelegate(object sender, EventArgs e);
        public event ContextPropertyChangedEventDelegate ContextPropertyChangedEvent;

        #endregion  /*  End of region : Events.  */

        #region Constructor
        private TrafodionContext()
        {
            _lookAndFeelManager = new TrafodionLookAndFeelManager();
            try
            {
                var document = XElement.Load("TrafodionInit.xml");
                var sqlStatements = from elem in document.Elements("SQLStatement")
                                    select elem.Value;
                _startupSQLStatements = new List<string>(sqlStatements);
                var startupMode = document.Element("mode").Value;
                _startupMode = startupMode;
            }
            catch (Exception ex)
            {
            }
            ConnectionDefinition.Changed += ConnectionDefinition_Changed;
        }

        ~TrafodionContext()
        {
            ConnectionDefinition.Changed -= ConnectionDefinition_Changed;
        }
        #endregion

        #region Public methods

        /// <summary>
        /// Retrns the reference of the TrafodionContext
        /// </summary>
        /// <returns></returns>
        public static TrafodionContext Instance
        {
            get { return _instance; }
        }

        /// <summary>
        /// Method invoked to select the current connection
        /// </summary>
        /// <param name="aConnectionSelector"></param>
        public void OnConnectionDefinitionSelection(IConnectionDefinitionSelector aConnectionSelector)
        {
            if (aConnectionSelector != null)
            {
                CurrentConnectionDefinition = aConnectionSelector.CurrentConnectionDefinition;
            }
        }

        /// <summary>
        /// Given a connection definition name returns the severities for that system
        /// </summary>
        /// <param name="connectionDefinitionName"></param>
        /// <returns></returns>
        public ArrayList GetSeverities(string connectionDefinitionName)
        {
            return _severitiesForSystems[connectionDefinitionName] as ArrayList;
        }

        public void SetSeverities(string connectionDefinitionName, ArrayList severities)
        {
            if (_severitiesForSystems.ContainsKey(connectionDefinitionName))
            {
                _severitiesForSystems.Remove(connectionDefinitionName);
            }
            _severitiesForSystems.Add(connectionDefinitionName, severities);
        }

        /// <summary>
        /// given a connection definition name returns whether the severities for that system
        /// is made permanent.
        /// </summary>
        /// <param name="connectionDefinitionName"></param>
        /// <returns></returns>
        public bool IsSeverityListPermanent(string connectionDefinitionName)
        {
            if (_permanentSeveritiesForSystems.ContainsKey(connectionDefinitionName))
            {
                return (bool)_permanentSeveritiesForSystems[connectionDefinitionName]; 
            }

            return false;
        }

        /// <summary>
        /// To set the severity list permanent for the given connection definition name
        /// </summary>
        /// <param name="connectionDefinitionName"></param>
        public void SetServerityListPermanent(string connectionDefinitionName)
        {
            if (_permanentSeveritiesForSystems.ContainsKey(connectionDefinitionName))
            {
                _permanentSeveritiesForSystems.Remove(connectionDefinitionName);
            }
            _permanentSeveritiesForSystems.Add(connectionDefinitionName, true);
        }

        /// <summary>
        /// Given a connection definition name returns the components for that system
        /// </summary>
        /// <param name="connectionDefinitionName"></param>
        /// <returns></returns>
        public ArrayList GetComponents(string connectionDefinitionName)
        {
            return _componentsForSystems[connectionDefinitionName] as ArrayList;
        }

        public void SetComponents(string connectionDefinitionName, ArrayList components)
        {
            if (_componentsForSystems.ContainsKey(connectionDefinitionName))
            {
                _componentsForSystems.Remove(connectionDefinitionName);
            }
            _componentsForSystems.Add(connectionDefinitionName, components);
        }

        /// <summary>
        /// given a connection definition name returns whether the components for that system
        /// is made permanent.
        /// </summary>
        /// <param name="connectionDefinitionName"></param>
        /// <returns></returns>
        public bool IsComponentListPermanent(string connectionDefinitionName)
        {
            if (_permanentComponentsForSystems.ContainsKey(connectionDefinitionName))
            {
                return (bool)_permanentComponentsForSystems[connectionDefinitionName];
            }

            return false;
        }

        /// <summary>
        /// To set the component list permanent for the given connection definition name
        /// </summary>
        /// <param name="connectionDefinitionName"></param>
        public void SetComponentListPermanent(string connectionDefinitionName)
        {
            if (_permanentComponentsForSystems.ContainsKey(connectionDefinitionName))
            {
                _permanentComponentsForSystems.Remove(connectionDefinitionName);
            }
            _permanentComponentsForSystems.Add(connectionDefinitionName, true);
        }

        #endregion

        #region Private methods
        /// <summary>
        /// Method to notify all event listeners that the current prioperty has changed
        /// </summary>
        /// <param name="aPropertyName"></param>
        /// <param name="oldValue"></param>
        /// <param name="newValue"></param>
        private void firePropertyChangedEvent(string aPropertyName, Object oldValue, Object newValue)
        {
            if (ContextPropertyChangedEvent != null)
            {                
                ContextPropertyChangedEvent(this, new TrafodionContextChangedEventArgs(aPropertyName, oldValue, newValue));
            }
        }

        /// <summary>
        /// Enable/Disable connect/disconnect toolbar based on the current connection definition's state
        /// </summary>
        private void EnableDisableConnectionToolBars()
        {
            MainToolBar mainToolBar = TheTrafodionMain.TheMainToolBar;
            if (CurrentConnectionDefinition != null)
            {
                if (CurrentConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
                {
                    mainToolBar.TheConnectToolStripItem.Enabled = false;
                    mainToolBar.TheDisconnectToolStripItem.Enabled = true;
                }
                else
                {
                    mainToolBar.TheConnectToolStripItem.Enabled = true;
                    mainToolBar.TheDisconnectToolStripItem.Enabled = false;
                }
            }
            else
            {
                TheTrafodionMain.TheMainToolBar.TheConnectToolStripItem.Enabled = false;
                TheTrafodionMain.TheMainToolBar.TheDisconnectToolStripItem.Enabled = false;
            }
        }
        #endregion
    }

    #region Inner calss TrafodionContextChangedEventArgs

    /// <summary>
    /// Custom EventArgs class to carry the information when the context propert changes
    /// </summary>
    public class TrafodionContextChangedEventArgs : EventArgs
    {
        public TrafodionContextChangedEventArgs(string aPropertyName, object aOldValue, object aNewValue)
        {
            this._propertyName = aPropertyName;
            this._oldValue = aOldValue;
            this._newValue = aNewValue;
        }

        private string _propertyName;
        private object _oldValue;
        private object _newValue;


        public string PropertyName
        {
            get { return _propertyName; }
            set { _propertyName = value; }
        }

        public object OldValue
        {
            get { return _oldValue; }
            set { _oldValue = value; }
        }

        public object NewValue
        {
            get { return _newValue; }
            set { _newValue = value; }
        }
    }
    #endregion

}
