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

using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Controls.Tree;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea.Controls
{

    /// <summary>
    /// A hyperlink to a sql object in a data grid view.
    /// </summary>
    public class DatabaseAreaObjectsDataGridViewLink : DataGridViewTextBoxCell
    {
        /// <summary>
        /// Default constructor
        /// </summary>
        public DatabaseAreaObjectsDataGridViewLink()
        {
        }

        public DatabaseAreaObjectsDataGridViewLink(DatabaseTreeView aDatabaseTreeView, TrafodionObject aTrafodionObject)
            : base()
        {
            TheDatabaseTreeView = aDatabaseTreeView;
            TheTrafodionObject = aTrafodionObject;
        }

        override public string ToString()
        {
            if (TheTrafodionObject == null)
            {
                return base.ToString();
            }
            return TheTrafodionObject.ExternalName;
        }

        protected override void Paint(
            Graphics aGraphics,
            Rectangle aClipBounds,
            Rectangle aCellBounds,
            int aRowIndex,
            DataGridViewElementStates aCellState,
            object aValue,
            object aFormattedValue,
            string anErrorText,
            DataGridViewCellStyle aCellStyle,
            DataGridViewAdvancedBorderStyle anAdvancedBorderStyle,
            DataGridViewPaintParts aPaintParts)
        {

            // Check to see if this is a type we know how to deal with
            if (!(aValue is DatabaseAreaObjectsDataGridViewLink))
            {

                // It's not ... just do standard paint as defined by base class
                base.Paint(aGraphics, aClipBounds, aCellBounds, aRowIndex, aCellState,
                    aValue, aFormattedValue, anErrorText, aCellStyle,
                    anAdvancedBorderStyle, aPaintParts);

                // And exit
                return;

            }

            DataGridViewCellStyle theCellStyle = new DataGridViewCellStyle(aCellStyle);
            theCellStyle.Font = new Font(aCellStyle.Font, aCellStyle.Font.Style | FontStyle.Underline);

            // Retrieve the client location of the mouse pointer.
            Point theCursorPosition = DataGridView.PointToClient(Cursor.Position);
            if (aCellBounds.Contains(theCursorPosition))
            {
                theCellStyle.ForeColor = Color.Blue;

                base.Paint(aGraphics, aClipBounds, aCellBounds, aRowIndex, aCellState,
                    aValue, aFormattedValue, anErrorText, theCellStyle,
                    anAdvancedBorderStyle, aPaintParts);

            }
            else
            {

                base.Paint(aGraphics, aClipBounds, aCellBounds, aRowIndex, aCellState,
                    aValue, aFormattedValue, anErrorText, theCellStyle,
                    anAdvancedBorderStyle, aPaintParts);

            }

        }

        // Force the cell to repaint itself when the mouse pointer enters it.
        protected override void OnMouseEnter(int rowIndex)
        {
            DataGridView.InvalidateCell(this);
        }

        // Force the cell to repaint itself when the mouse pointer leaves it.
        protected override void OnMouseLeave(int rowIndex)
        {
            DataGridView.InvalidateCell(this);
        }

        protected override void OnClick(DataGridViewCellEventArgs e)
        {
            DatabaseAreaOptions databaseOptions = DatabaseAreaOptions.GetOptions();
            // Can get here with no rows selected if the click is unselecting a row
            if (DataGridView.SelectedRows.Count == 0)
            {
                return;
            }

            DataGridViewRow theDataGridViewRow = DataGridView.SelectedRows[0];
            DataGridViewCell dataGridCell = DataGridView.Rows[e.RowIndex].Cells[e.ColumnIndex];
            DatabaseAreaObjectsDataGridViewLink theLink = dataGridCell.Value as DatabaseAreaObjectsDataGridViewLink;

            if (theLink != null)
            {
                DatabaseTreeView theDatabaseTreeView = theLink.TheDatabaseTreeView;
                TrafodionObject theTrafodionObject = theLink.TheTrafodionObject;
                if (theTrafodionObject != null)
                {
                    bool openAsLink = true;
                    if (theDatabaseTreeView != null && openAsLink)
                    {
                        theDatabaseTreeView.SelectTrafodionObject(theTrafodionObject);
                    }
                    else
                    {
                        string theVisibleAnsiName = theTrafodionObject.VisibleAnsiName;
                        string objectType = "";
                        Control theControl = null;

                        if (theTrafodionObject is TrafodionTable)
                        {
                            theControl = new TableTabControl(null, theTrafodionObject as TrafodionTable);
                            objectType = Properties.Resources.Table;
                        }
                        else if (theTrafodionObject is TrafodionMaterializedView)
                        {
                            theControl = new MaterializedViewTabControl(null, theTrafodionObject as TrafodionMaterializedView);
                            objectType = Properties.Resources.MaterializedView;
                        }
                        else if (theTrafodionObject is TrafodionMaterializedViewGroup)
                        {
                            theControl = new MaterializedViewGroupTabControl(null, theTrafodionObject as TrafodionMaterializedViewGroup);
                            objectType = Properties.Resources.MaterializedViewGroup;
                        }
                        else if (theTrafodionObject is TrafodionView)
                        {
                            theControl = new ViewTabControl(null, theTrafodionObject as TrafodionView);
                            objectType = Properties.Resources.View;
                        }
                        else if (theTrafodionObject is TrafodionProcedure)
                        {
                            theControl = new ProcedureTabControl(null, theTrafodionObject as TrafodionProcedure);
                            objectType = Properties.Resources.Procedure;
                        }
                        else if (theTrafodionObject is TrafodionLibrary)
                        {
                            theControl = new LibraryTabControl(null, theTrafodionObject as TrafodionLibrary);
                            objectType = Properties.Resources.Library;
                        }
                        else if (theTrafodionObject is TrafodionSynonym)
                        {
                            theControl = new SynonymTabControl(null, theTrafodionObject as TrafodionSynonym);
                            objectType = Properties.Resources.Synonym;
                        }
                        else if (theTrafodionObject is TrafodionIndex)
                        {
                            theControl = new IndexTabControl(null, theTrafodionObject as TrafodionIndex);
                            objectType = Properties.Resources.Index;
                        }
                        else if (theTrafodionObject is TrafodionTrigger)
                        {
                            theControl = new TriggerTabControl(null, theTrafodionObject as TrafodionTrigger);
                            objectType = Properties.Resources.Trigger;
                        }
                        else if (theTrafodionObject is TrafodionCatalog)
                        {
                            theControl = new CatalogTabControl(null, theTrafodionObject as TrafodionCatalog);
                            objectType = Properties.Resources.Catalog;
                        }
                        else if (theTrafodionObject is TrafodionSchema)
                        {
                            theControl = new SchemaTabControl(null, theTrafodionObject as TrafodionSchema);
                            objectType = Properties.Resources.Schema;
                        }
                        else if (theTrafodionObject is TrafodionUDFunction)
                        {
                            //theControl = new UDFTabControl(null, theTrafodionObject as TrafodionUDF);
                            //objectType = Properties.Resources.UDF;
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

                            WindowsManager.PutInWindow(new Size(640, 480), popControl, theVisibleAnsiName, theTrafodionObject.ConnectionDefinition);
                        }

                    }

                }
            }

        }

        private DatabaseTreeView theDatabaseTreeView = null;

        public DatabaseTreeView TheDatabaseTreeView
        {
            get { return theDatabaseTreeView; }
            set { theDatabaseTreeView = value; }
        }

        private TrafodionObject theTrafodionObject = null;

        public TrafodionObject TheTrafodionObject
        {
            get { return theTrafodionObject; }
            set { theTrafodionObject = value; }
        }
    }

}
