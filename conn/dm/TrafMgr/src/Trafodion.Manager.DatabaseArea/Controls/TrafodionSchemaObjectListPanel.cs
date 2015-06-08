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

using System.Collections.Generic;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.Framework.Connections;
using TenTec.Windows.iGridLib;
using System.Drawing;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// A panel to display a list of schema objects
    /// </summary>
    /// <typeparam name="T"></typeparam>
    public class TrafodionSchemaObjectListPanel<T> : TrafodionPanel, ICloneToWindow where T : TrafodionSchemaObject
    {
        private DatabaseObjectsControl _databaseObjectsControl;
        private List<T> _sqlMxObjects;
        private TrafodionObject _parentTrafodionObject;
        private string _title;
        private string _headerText;
        private NavigationTreeNameFilter _nameFilter;
                
        protected TrafodionIGrid grid = null;

        /// <summary>
        /// A reference to the database navigation tree
        /// </summary>
        public DatabaseObjectsControl TheDatabaseObjectsControl
        {
            get { return _databaseObjectsControl; }
        }


        /// <summary>
        /// The list of sql schema objects to display
        /// </summary>
        public List<T> TheTrafodionObjects
        {
            get { return _sqlMxObjects; }
        }


        /// <summary>
        /// A reference to the parent schema
        /// </summary>
        public TrafodionObject TheParentTrafodionObject
        {
            get { return _parentTrafodionObject; }
        }
 

        /// <summary>
        /// The title of the panel
        /// </summary>
        public string TheTitle
        {
            get { return _title; }
        }

        /// <summary>
        /// The header text for the datagridview
        /// </summary>
        public string TheHeaderText
        {
            get { return _headerText; }
        }

        /// <summary>
        /// Filter
        /// </summary>
        private NavigationTreeNameFilter NameFilter
        {
            get { return _nameFilter; }
        }

  
        /// <summary>
        /// Constructs a generic panel to display a list of TrafodionSchemaObjects
        /// </summary>
        /// <param name="aDatabaseObjectsControl">The control that has reference to the DatabaseTreeView</param>
        /// <param name="aHeaderText">The text to display as the header for the object list</param>
        /// <param name="parentTrafodionObject">The parent sql object in whose context, we are displaying this list</param>
        /// <param name="sqlMxObjects">The list of TrafodionSchemaObjects that need to be displayed</param>
        /// <param name="aTitle"></param>
        public TrafodionSchemaObjectListPanel(DatabaseObjectsControl databaseObjectsControl, string headerText, TrafodionObject parentTrafodionObject,
                    List<T> sqlMxObjects, string title)
        {
            _databaseObjectsControl = databaseObjectsControl;
            _headerText = headerText;
            _parentTrafodionObject = parentTrafodionObject;
            _sqlMxObjects = sqlMxObjects;
            _title = title;
            _nameFilter = databaseObjectsControl != null ? databaseObjectsControl.TheNameFilter : null;

            CreateGrid();
        }
        /// <summary>
        /// Creates the grid that will list the sql schema object list
        /// </summary>
        protected virtual void CreateGrid()
        {
            InitializeGrid();
            FillGrid();
            AttachGrid();
            AddBottomButtonsControl();
        }

        protected virtual void AddAdditionalButton(TrafodionIGridButtonsUserControl buttonsControl)
        {
        }

        protected virtual void AddGridColumn()
        {
            this.grid.Cols.Add("Name", Properties.Resources.Name);
            this.grid.Cols.Add("Owner", Properties.Resources.Owner);
            this.grid.Cols.Add("MetadataUID", Properties.Resources.MetadataUID);
            this.grid.Cols.Add("CreationTime", Properties.Resources.CreationTime);
            this.grid.Cols.Add("RedefinitionTime", Properties.Resources.RedefinitionTime);
        }

        protected virtual object[] ExtractValues(TrafodionSchemaObject sqlMxSchemaObject)
        {
            return new object[] 
                {                            
                    sqlMxSchemaObject.ExternalName, 
                    sqlMxSchemaObject.Owner,
                    sqlMxSchemaObject.UID, 
                    sqlMxSchemaObject.FormattedCreateTime(),
                    sqlMxSchemaObject.FormattedRedefTime()
                };
        }

        private void AddBottomButtonsControl()
        {
            TrafodionPanel panel = new TrafodionPanel();

            panel.Height = 40;

            TrafodionIGridButtonsUserControl bottomButtonsControl = new TrafodionIGridButtonsUserControl(this.grid);
            bottomButtonsControl.Dock = DockStyle.Bottom;
            panel.Controls.Add(bottomButtonsControl);

            AddAdditionalButton(bottomButtonsControl);

            panel.Dock = DockStyle.Bottom;
            Controls.Add(panel);
        }

        private void InitializeGrid()
        {
            this.grid = new TrafodionIGrid();
            AddGridColumn();

            this.grid.Dock = DockStyle.Fill;
            this.grid.AutoResizeCols = true;
            this.grid.RowMode = false;
            this.grid.RowSelectionInCellMode = iGRowSelectionInCellModeTypes.None;
            this.grid.SelectionMode = iGSelectionMode.MultiExtended;

            this.grid.DoubleClickHandler = Navigate;
            this.grid.CellClick += new iGCellClickEventHandler(grid_CellClick);
        }

        private void FillGrid()
        {
            this.grid.BeginUpdate();
            foreach (TrafodionSchemaObject sqlMxSchemaObject in TheTrafodionObjects)
            {
                if (NameFilter == null || NameFilter.Matches(sqlMxSchemaObject.VisibleAnsiName))
                {
                    object[] values = ExtractValues(sqlMxSchemaObject);
                    iGRow row = this.grid.AddRow(values);
                    row.Cells[0].AuxValue = sqlMxSchemaObject;
                }
            }
            this.grid.EndUpdate();
        }

        private void AttachGrid()
        {
            this.Controls.Add(this.grid);
            this.grid.AddCountControlToParent(_headerText, DockStyle.Top);
            TrafodionIGridHyperlinkCellManager hyperLinkCellManager = new TrafodionIGridHyperlinkCellManager();
            hyperLinkCellManager.Attach(this.grid, 0);
        }

        private void Navigate(int gridRowIndex)
        {
            if (gridRowIndex < 0) return;
            TrafodionObject sqlMxObject = (TrafodionObject)this.grid.Rows[gridRowIndex].Cells[0].AuxValue;
            if (sqlMxObject != null)
            {
                if (TheDatabaseObjectsControl != null && TheDatabaseObjectsControl.TheDatabaseTreeView != null)
                {
                    TheDatabaseObjectsControl.TheDatabaseTreeView.SelectTrafodionObject(sqlMxObject);
                }
                else
                {
                    string theVisibleAnsiName = sqlMxObject.VisibleAnsiName;
                    string objectType = "";
                    Control theControl = null;

                    if (sqlMxObject is TrafodionTable)
                    {
                        theControl = new TableTabControl(null, sqlMxObject as TrafodionTable);
                        objectType = Properties.Resources.Table;
                    }
                    else if (sqlMxObject is TrafodionMaterializedView)
                    {
                        theControl = new MaterializedViewTabControl(null, sqlMxObject as TrafodionMaterializedView);
                        objectType = Properties.Resources.MaterializedView;
                    }
                    else if (sqlMxObject is TrafodionMaterializedViewGroup)
                    {
                        theControl = new MaterializedViewGroupTabControl(null, sqlMxObject as TrafodionMaterializedViewGroup);
                        objectType = Properties.Resources.MaterializedViewGroup;
                    }
                    else if (sqlMxObject is TrafodionView)
                    {
                        theControl = new ViewTabControl(null, sqlMxObject as TrafodionView);
                        objectType = Properties.Resources.View;
                    }
                    else if (sqlMxObject is TrafodionProcedure)
                    {
                        theControl = new ProcedureTabControl(null, sqlMxObject as TrafodionProcedure);
                        objectType = Properties.Resources.Procedure;
                    }
                    else if (sqlMxObject is TrafodionLibrary)
                    {
                        theControl = new LibraryTabControl(null, sqlMxObject as TrafodionLibrary);
                        objectType = Properties.Resources.Library;
                    }
                    else if (sqlMxObject is TrafodionSynonym)
                    {
                        theControl = new SynonymTabControl(null, sqlMxObject as TrafodionSynonym);
                        objectType = Properties.Resources.Synonym;
                    }
                    else if (sqlMxObject is TrafodionIndex)
                    {
                        theControl = new IndexTabControl(null, sqlMxObject as TrafodionIndex);
                        objectType = Properties.Resources.Index;
                    }
                    else if (sqlMxObject is TrafodionTrigger)
                    {
                        theControl = new TriggerTabControl(null, sqlMxObject as TrafodionTrigger);
                        objectType = Properties.Resources.Trigger;
                    }
                    else if (sqlMxObject is TrafodionCatalog)
                    {
                        theControl = new CatalogTabControl(null, sqlMxObject as TrafodionCatalog);
                        objectType = Properties.Resources.Catalog;
                    }
                    else if (sqlMxObject is TrafodionSchema)
                    {
                        theControl = new SchemaTabControl(null, sqlMxObject as TrafodionSchema);
                        objectType = Properties.Resources.Schema;
                    }
                    else if (sqlMxObject is TrafodionUDFunction)
                    {
                    }
                    if (theControl != null)
                    {
                        Control popControl = new Control();
                        TrafodionTextBox objectNameBox = new TrafodionTextBox();
                        objectNameBox.Text = objectType + " " + theVisibleAnsiName;
                        objectNameBox.Font = new Font("Tahoma", 8.25F, FontStyle.Bold);
                        objectNameBox.ReadOnly = true;
                        objectNameBox.Dock = DockStyle.Top;
                        theControl.Dock = DockStyle.Fill;

                        popControl.Controls.Add(theControl);
                        popControl.Controls.Add(objectNameBox);

                        WindowsManager.PutInWindow(new Size(640, 480), popControl, theVisibleAnsiName, sqlMxObject.ConnectionDefinition);
                    }

                }

            }
        }
        
        private void grid_CellClick(object sender, iGCellClickEventArgs e)
        {
            if (e.ColIndex == 0)
            {
                Navigate(e.RowIndex);
            }
        }

        #region ICloneToWindow

        /// <summary>
        /// Makes a clone of this panel suitable for inclusion in some container.
        /// </summary>
        /// <returns>The clone control</returns>
        virtual public Control Clone()
        {
            TrafodionSchemaObjectListPanel<T> sqlMxObjectListPanel = new TrafodionSchemaObjectListPanel<T>(null, TheHeaderText, 
                                                                        TheParentTrafodionObject, TheTrafodionObjects, TheTitle);
            return sqlMxObjectListPanel;
        }

        /// <summary>
        /// A string suitable for a window title.
        /// </summary>
        /// <returns>A string</returns>
        virtual public string WindowTitle
        {
            get { return TheParentTrafodionObject.VisibleAnsiName + " " + TheTitle; }
        }


        /// <summary>
        /// Stores Connection Definition Property for this object
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return TheParentTrafodionObject.ConnectionDefinition; }
        }

        #endregion

    }

}
