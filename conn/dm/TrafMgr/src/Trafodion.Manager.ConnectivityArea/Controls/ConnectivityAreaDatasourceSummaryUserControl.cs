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
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.ConnectivityArea.Model;
using System.Data;
using TenTec.Windows.iGridLib;
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.ConnectivityArea.Controls
{
    /// <summary>
    /// 
    /// </summary>
     public partial class ConnectivityAreaDatasourceSummaryUserControl : UserControl, ICloneToWindow
    {
        #region Fields

        NDCSSystem _NdcsSystem;
        private Trafodion.Manager.Framework.Navigation.NavigationTreeView appropriateTreeView = null;
        ConnectivityObjectsIGrid<NDCSSystem> ConnectivityiGrid = null;
        TrafodionIGridHyperlinkCellManager _hyperLinkCellManager = new TrafodionIGridHyperlinkCellManager();
        iGCellClickEventHandler _cellClickHandler = null;
         
        DataTable dataTable = null;
        // default is no column has underline link.
        int HyperlinkColIndex = -1;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Gets the underlying system
        /// </summary>
        public NDCSSystem NdcsSystem
        {
            get { return _NdcsSystem; }
            set { _NdcsSystem = value; }
        }

        #endregion Properties

        #region ICloneToWindow Members

        /// <summary>
        /// Creates a new instance
        /// </summary>
        /// <returns></returns>
        public Control Clone()
        {
            return new ConnectivityAreaDatasourceSummaryUserControl(NdcsSystem, this.appropriateTreeView, NdcsSystem.ConnectionDefinition.Name);

        }

        /// <summary>
        /// Get the window title of the cloned window
        /// </summary>
        public string WindowTitle
        {
            get { return Properties.Resources.ActiveSystemSummary + " - " + NdcsSystem.ConnectionDefinition.Name; }
        }

        /// <summary>
        /// Stores Connection Definition for this object
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return NdcsSystem.ConnectionDefinition; }
        }

        #endregion

         /// <summary>
        /// 
        /// </summary>
        /// <param name="aNdcsSystem"></param>
        /// <param name="treeView"></param>
        public ConnectivityAreaDatasourceSummaryUserControl(NDCSSystem aNdcsSystem, Trafodion.Manager.Framework.Navigation.NavigationTreeView treeView)
        {
            InitializeComponent();
            NdcsSystem = aNdcsSystem;
            this.appropriateTreeView = treeView;
            SetInitialValues();
        }

        /// <summary>
        /// Used for clone window only!  this constructor will populate Grid right away!!
        /// </summary>
        /// <param name="aNdcsSystem"></param>
        /// <param name="treeView"></param>
        /// <param name="ConnectionDefinitionName"></param>
        public ConnectivityAreaDatasourceSummaryUserControl(NDCSSystem aNdcsSystem, Trafodion.Manager.Framework.Navigation.NavigationTreeView treeView, string ConnectionDefinitionName)
        {
            InitializeComponent();
            NdcsSystem = aNdcsSystem;
            this.appropriateTreeView = treeView;
            string strTemp = ConnectionDefinitionName;
            SetInitialValues();
            Populate();
        }

         /// <summary>
         /// Call to this control to refresh the contain ...
         /// </summary>
         public override void Refresh()
        {
            if (this.Parent != null)
            {
                this.Parent.Refresh();
            }

             // clear up the iGrid...
            ConnectivityiGrid = new ConnectivityObjectsIGrid<NDCSSystem>();

            Populate();
        }

        internal void Populate()
        {

            Cursor.Current = Cursors.WaitCursor;
            try
            {
                if (dataTable != null)
                {
                    dataTable.Clear();
                }
                BuildIGrid(NdcsSystem);
            }
            finally
            {
                Cursor.Current = Cursors.Default;
            }

        }

        
         /// <summary>
        /// 
        /// </summary>
        /// <param name="aNdcsSystem"></param>
         public void BuildIGrid(NDCSSystem aNdcsSystem)
        {

             dataTable = new DataTable();
             dataTable.TableName = "DataSource Summary";

            dataTable.Columns.Add(Properties.Resources.DSName, typeof(System.String));
            if (aNdcsSystem.UserHasInfoPrivilege)
            {
                dataTable.Columns.Add(Properties.Resources.MaxServerCount, typeof(int));
                dataTable.Columns.Add(Properties.Resources.InitServerCount, typeof(int));
                dataTable.Columns.Add(Properties.Resources.AvailServerCount, typeof(int));
            }

            try
            {
                // there are some cases which NDCSSystem failed to get NDCS DataSources from server.
                foreach (NDCSDataSource datasource in aNdcsSystem.NDCSDataSources)
                {
                    if (aNdcsSystem.UserHasInfoPrivilege)
                    {
                        dataTable.Rows.Add(new object[] {
                            datasource.Name, datasource.MaxServerCount, 
                            datasource.InitialServerCount, datasource.AvailableServerCount
                            });
                    }
                    else
                    {
                        dataTable.Rows.Add(new object[] {
                            datasource.Name
                            });
                    }
                }
            }
            catch (System.Data.Odbc.OdbcException oe)
            {
                // Got an ODBC erorr. Show it.
                MessageBox.Show(Utilities.GetForegroundControl(), oe.Message, Properties.Resources.ODBCError, MessageBoxButtons.OK);
            }
            catch (Exception ex)
            {
                // Got some other exception.  Show it.
                MessageBox.Show(Utilities.GetForegroundControl(), ex.Message, Properties.Resources.SystemError, MessageBoxButtons.OK);
            }
            finally
            {
                ConnectivityiGrid.BeginUpdate();

                ConnectivityiGrid.FillWithData(dataTable);
                ConnectivityiGrid.ResizeGridColumns(dataTable);
                ConnectivityiGrid.Dock = DockStyle.Fill;
                this.TrafodionPanel1.Controls.Clear();
                TrafodionPanel1.Controls.Add(ConnectivityiGrid);

                for (int i = 0; i < ConnectivityiGrid.Rows.Count; i++)
                {
                    ConnectivityiGrid.Rows[i].Tag = Trafodion.Manager.Properties.Resources.MySystemsText + "\\" +
                        NdcsSystem.Name + "\\" + Properties.Resources.DataSources + "\\" +
                        ConnectivityiGrid.Rows[i].Cells[Properties.Resources.DSName].Value.ToString();
                }

                string _timestamp = Utilities.CurrentFormattedDateTime;

                string _strformat = Properties.Resources.DatasourceCount;

                ConnectivityiGrid.AddCountControlToParent(_strformat, DockStyle.Top);
                Control buttonControl = ConnectivityiGrid.GetButtonControl();
                buttonControl.Dock = DockStyle.Fill;
                theBottomPanel.Controls.Add(buttonControl);

                ConnectivityiGrid.EndUpdate();
                if (_NdcsSystem.UserHasInfoPrivilege)
                {
                    _hyperLinkCellManager.Attach(ConnectivityiGrid, HyperlinkColIndex);
                    // It's better to call this method after you attached iGHyperlinkCellManager to the grid
                    // because the manager can change the formatting of the links dynamically
                }
                ConnectivityiGrid.ResizeGridColumns(dataTable, 7, 30);
            }

        }

        /// <summary>
        /// Stores the initial settings
        /// </summary>
        private void SetInitialValues()
        {
            // set initial values of the iGrid controls here
            ConnectivityiGrid = new ConnectivityObjectsIGrid<NDCSSystem>();

            HyperlinkColIndex = 0;
            // handle hyper link in iGrid cell
            _cellClickHandler = new iGCellClickEventHandler(MethodsGrid_CellClick);
            ConnectivityiGrid.CellClick += _cellClickHandler;

        }

         
         private void MethodsGrid_CellClick(object sender, iGCellClickEventArgs e)
         {
             if (_NdcsSystem.UserHasInfoPrivilege)
             {
                 // Handle hyperline here
                 if (e.ColIndex == HyperlinkColIndex)
                 {
                     try
                     {
                         if (this.appropriateTreeView != null)
                         {
                             TreeNode clickedNode = appropriateTreeView.FindByFullPath(this.ConnectivityiGrid.Rows[e.RowIndex].Tag.ToString());
                             appropriateTreeView.Select(clickedNode);
                         }
                     }
                     catch (Exception ex) { }

                 }
             }
        }


    }
}
