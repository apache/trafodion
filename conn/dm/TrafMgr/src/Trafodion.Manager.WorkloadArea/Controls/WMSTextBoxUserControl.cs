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
using System.Data;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Queries.Controls;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using Trafodion.Manager.WorkloadArea.Model;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    /// <summary>
    /// The user control for a universal widget containing a simple TextBox
    /// </summary>
    public partial class WMSTextBoxUserControl : UserControl
    {
        #region Fields

        private string m_title = null;
        private ConnectionDefinition m_connectionDefinition = null;

        private WMSOffenderDatabaseDataProvider m_dbDataProvider = null;
        private delegate void HandleEvents(Object obj, DataProviderEventArgs e);
        private GenericUniversalWidget m_widget = null;
        private WMSTextBoxDisplayControl m_textBoxDisplayControl = null;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: WMSOffenderDatabaseDataProvider - the data provider
        /// </summary>
        public WMSOffenderDatabaseDataProvider WMSOffenderDatabaseDataProvider
        {
            get { return m_dbDataProvider; }
        }

        /// <summary>
        /// Property: IDTitle - the title of the ID, this could be Process Name, Query ID, etc.
        /// </summary>
        public string IDTitle
        {
            get { return _theIDTitle.Text; }
            set { _theIDTitle.Text = value; }
        }

        /// <summary>
        /// Property: ID - the identifier of the object showing
        /// </summary>
        public string ID
        {
            get { return _theIDTextBox.Text; }
            set { _theIDTextBox.Text = value; }
        }

        /// <summary>
        /// Property: Title - the title
        /// </summary>
        public string Title
        {
            get { return m_title; }
            set { m_title = value; }
        }

        /// <summary>
        /// Property: DisplayText - the content display in the TextBox
        /// </summary>
        public string DisplayText
        {
            get
            {
                if (m_textBoxDisplayControl != null)
                {
                    return m_textBoxDisplayControl.Contents;
                }
                else
                {
                    return "";
                }
            }
            set
            {
                if (m_textBoxDisplayControl != null)
                {
                    m_textBoxDisplayControl.Contents = value; 
                }
            }
        }

        /// <summary>
        /// Property: HelpTopics - setting the help topic
        /// </summary>
        public string HelpTopics
        {
            set
            {
                if (m_widget != null && m_widget.UniversalWidgetConfiguration != null)
                {
                    m_widget.UniversalWidgetConfiguration.HelpTopic = value;
                }
            }
        }

        #endregion Properties

        #region Constructor

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aTitle"></param>
        /// <param name="aConnectionDefinition"></param>
        public WMSTextBoxUserControl(string aTitle, ConnectionDefinition aConnectionDefinition)
        {
            InitializeComponent();
            m_title = aTitle;
            m_connectionDefinition = aConnectionDefinition;
            ShowWidgets();
        }

        #endregion Constructor

        #region Public methods

        /// <summary>
        /// To initialize the TextBox
        /// </summary>
        /// <returns></returns>
        public virtual string InitialText()
        {
            return "";
        }

        /// <summary>
        /// Load new data to the TextBox
        /// </summary>
        /// <param name="aDataTable"></param>
        /// <returns></returns>
        public virtual string LoadNewData(DataTable aDataTable)
        {
            return "";
        }

        /// <summary>
        /// To start the data provider
        /// </summary>
        public virtual void Start()
        {
            // Start data provider
            m_dbDataProvider.Start();
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// Create and show all widgets
        /// </summary>
        private void ShowWidgets()
        {
            DatabaseDataProviderConfig dbConfig = (DatabaseDataProviderConfig)WidgetRegistry.GetDefaultDBConfig().DataProviderConfig;
            dbConfig.TimerPaused = true;
            dbConfig.RefreshRate = 0;
            dbConfig.SQLText = "";
            dbConfig.ConnectionDefinition = m_connectionDefinition;

            m_dbDataProvider = new WMSOffenderDatabaseDataProvider(dbConfig);

            UniversalWidgetConfig widgetConfig = new UniversalWidgetConfig();
            widgetConfig.DataProvider = m_dbDataProvider;
            widgetConfig.ShowProviderToolBarButton = false;
            widgetConfig.ShowTimerSetupButton = false;
            widgetConfig.ShowHelpButton = true;
            widgetConfig.ShowExportButtons = false;
            widgetConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation;

            m_widget = new GenericUniversalWidget();
            m_widget.DataProvider = m_dbDataProvider;
            m_widget.UniversalWidgetConfiguration = widgetConfig;

            m_textBoxDisplayControl = new WMSTextBoxDisplayControl(this);
            m_widget.DataDisplayControl = m_textBoxDisplayControl;
            m_widget.DataProvider = m_dbDataProvider; // needs to do it again for output display to be correct.
            m_widget.Dock = DockStyle.Fill;

            // place the widget in the host container
            _thePanel.Controls.Clear();
            _thePanel.Controls.Add(m_widget);

            // Registers for events
            m_dbDataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;
        }

        /// <summary>
        /// My disposing
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void MyDispose(bool disposing)
        {
            if (disposing)
            {
                if (m_dbDataProvider != null)
                {
                    m_dbDataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
                    m_dbDataProvider.Stop();
                    m_dbDataProvider = null;
                }
            }
        }

        /// <summary>
        /// New data arrived event invoker
        /// </summary>
        /// <param name="obj"></param>
        /// <param name="e"></param>
        private void InvokeHandleNewDataArrived(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new HandleEvents(m_dbDataProvider_OnNewDataArrived), new object[] { obj, (DataProviderEventArgs)e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.WMS,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        /// <summary>
        /// New data arrived event handler
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void m_dbDataProvider_OnNewDataArrived(object sender, DataProviderEventArgs e)
        {
            m_textBoxDisplayControl.LoadNewData(m_dbDataProvider.DatabaseDataTable);
        }

        #endregion Private methods
    }

    #region Class WMSTextBoxDisplayControl

    /// <summary>
    /// The display control class
    /// </summary>
    public class WMSTextBoxDisplayControl : GenericDataDisplayControl
    {

        #region Fields

        private SqlStatementTextBox m_textBox = null;
        private WMSTextBoxUserControl m_wmsTextBoxUserControl = null;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: Text - the displayed text
        /// </summary>
        public string Contents
        {
            get { return m_textBox.Text; }
            set { m_textBox.Text = value; }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// The constructor
        /// </summary>
        /// <param name="aWMSTextBoxUserControl"></param>
        public WMSTextBoxDisplayControl(WMSTextBoxUserControl aWMSTextBoxUserControl)
        {
            m_wmsTextBoxUserControl = aWMSTextBoxUserControl;

            // Sets up the TextBox
            m_textBox = new SqlStatementTextBox();
            m_textBox.WordWrap = true;
            m_textBox.ReadOnly = true;
            m_textBox.Text = "";
            m_textBox.Dock = DockStyle.Fill;

            // Put it into the base container
            this.TheControl = m_textBox;
        }

        #endregion Constructors

        #region Public methods

        /// <summary>
        /// Loading the new data to the TextBox
        /// </summary>
        /// <param name="aDataTable"></param>
        public void LoadNewData(DataTable aDataTable)
        {
            m_textBox.Text = m_wmsTextBoxUserControl.LoadNewData(aDataTable);
        }

        #endregion Public methods
    }

    #endregion  Class WMSTextBoxDisplayControl

}
