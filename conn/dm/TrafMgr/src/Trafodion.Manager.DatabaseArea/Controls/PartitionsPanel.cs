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
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework;
using TenTec.Windows.iGridLib;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// A panel that shows the partition info for any partitoned object.
    /// </summary>
    public class PartitionsPanel : TrafodionObjectPanel
    {
        private const string DATABASE_PARTITION_GRID_CONFIG_ID = "GridConfig_DatabasePartition";
        private const string HEADER_FORMAT = "There are {0} partitions. Total row count: {1}  Current size: {2}  Maximum size: {3}  ";
        private const string HEADER_FORMAT_COMPRESSED = "There are {0} partitions. Total row count: {1}  Current size: {2}  Compressed size: {3}  Maximum size: {4}  ";

        private PartitionedSchemaObject partitionedSchemaObject;
        TrafodionProgressUserControl progressControl;

        private TrafodionIGrid grid = null;
        private long theTotalRowCount = 0;
        private long theTotalMaxSize = 0;
        private long theTotalCurrentEOF = 0;
        private long theTotalCompressionEOF = 0;
        private int theCompressionType = -1;

        /// <summary>
        /// Create a panel that shows the partition info for any partitoned object.
        /// </summary>
        public PartitionsPanel()
            :base("Partitions", null)
        {

        }
        public PartitionsPanel(PartitionedSchemaObject partitionedSchemaObject)
            : base("Partitions", partitionedSchemaObject)
        {
            PartitionedSchemaObject = partitionedSchemaObject;
        }


        /// <summary>
        /// Read/write property whose value is the partitioned object.
        /// </summary>
        public PartitionedSchemaObject PartitionedSchemaObject
        {
            get { return this.partitionedSchemaObject; }
            set
            {
                if (value != null)
                {
                    this.partitionedSchemaObject = value;

                    Controls.Clear();

                    TrafodionProgressArgs args = new TrafodionProgressArgs("Fetching partition details", this.partitionedSchemaObject, "FetchPartitions", new object[0]);

                    //Clear the config privileges lookup control and event handler
                    this.progressControl = new TrafodionProgressUserControl(args);
                    this.progressControl.Dock = DockStyle.Fill;
                    this.progressControl.ProgressCompletedEvent += FetchCompleted;
                    Controls.Add(this.progressControl);
                }
            }
        }

        #region ICloneToWindow

        /// <summary>
        /// Makes a clone of this panel suitable for inclusion in some container.
        /// </summary>
        /// <returns>The clone control</returns>
        override public Control Clone()
        {
            return new PartitionsPanel(PartitionedSchemaObject);
        }

        /// <summary>
        /// A string suitable for a window title.
        /// </summary>
        /// <returns>The string</returns>
        public override string WindowTitle
        {
            get { return PartitionedSchemaObject.VisibleAnsiName + " partitions"; }
        }

        #endregion

        private void FetchCompleted(object sender, TrafodionProgressCompletedArgs e)
        {
            if (this.progressControl.Error != null)
            {
                Controls.Clear();
                MessageBox.Show(Utilities.GetForegroundControl(), this.progressControl.Error.Message, "Error obtaining partition details",
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            else
            {
                Controls.Clear();
                CreateGrid();
            }

        }

        private void InitializeComponent()
        {
            this.SuspendLayout();
            this.ResumeLayout(false);
        }
        
        private void CreateGrid()
        {
            InitializeGrid();
            this.grid.FillWithDataConfig(FillGrid);
            AttachGrid();
            AddBottomButtonsControl();
        }
        
        private void AddBottomButtonsControl()
        {
            TrafodionPanel panel = new TrafodionPanel();

            panel.Height = 40;

            TrafodionIGridButtonsUserControl bottomButtonsControl = new TrafodionIGridButtonsUserControl(this.grid);
            bottomButtonsControl.Dock = DockStyle.Bottom;
            panel.Controls.Add(bottomButtonsControl);

            panel.Dock = DockStyle.Bottom;
            Controls.Add(panel);
        }

        private void InitializeGrid()
        {
            this.grid = new TrafodionIGrid();
            this.grid.CreateGridConfig(DATABASE_PARTITION_GRID_CONFIG_ID);
            this.grid.Dock = DockStyle.Fill;
            this.grid.AutoResizeCols = false;
            this.grid.RowMode = true;
            this.grid.SelectionMode = iGSelectionMode.MultiExtended;
        }

        private void FillGrid()
        {
            this.grid.Cols.Add("thePartitionNumber", "Partition Number");
            this.grid.Cols.Add("theFilenameColumn", "File");
            this.grid.Cols.Add("theRowCount", "Row Count");
            this.grid.Cols.Add("thePercentAllocated", "Percent Allocated");
            this.grid.Cols.Add("theCurrentEOF", "Current Size");
            this.grid.Cols.Add("theCompressionEOF", "Compressed Size");
            this.grid.Cols.Add("theMaxSize", "Max Size");
            this.grid.Cols.Add("theCompressionType", "Compression Type");
            this.grid.Cols.Add("theCompressionRatio", "Compression Ratio");
            this.grid.Cols.Add("theInsertedRowCount", "Rows Inserted Since Last Update Stats");
            this.grid.Cols.Add("theDeletedRowCount", "Rows Deleted Since Last Update Stats");
            this.grid.Cols.Add("theUpdatedRowCount", "Rows Updated Since Last Update Stats");
            this.grid.Cols.Add("thePrimaryExtents", "Primary Extents");
            this.grid.Cols.Add("theSecondaryExtents", "Secondary Extents");
            this.grid.Cols.Add("theMaximumExtents", "Max Extents");
            this.grid.Cols.Add("theAllocatedExtents", "Allocated Extents");
            if (this.PartitionedSchemaObject.ConnectionDefinition != null && this.PartitionedSchemaObject.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ120)
            {
                this.grid.Cols.Add("theInternalOverhead", "Overhead");
                this.grid.Cols.Add("theAccessCount", "Access Count");
            }

            this.grid.Cols["theRowCount"].CellStyle.FormatString = "{0:N0}";
            this.grid.Cols["theInsertedRowCount"].CellStyle.FormatString = "{0:N0}";
            this.grid.Cols["theDeletedRowCount"].CellStyle.FormatString = "{0:N0}";
            this.grid.Cols["theUpdatedRowCount"].CellStyle.FormatString = "{0:N0}";
            this.grid.Cols["thePrimaryExtents"].CellStyle.FormatString = "{0:N0}";
            this.grid.Cols["theSecondaryExtents"].CellStyle.FormatString = "{0:N0}";
            this.grid.Cols["theAllocatedExtents"].CellStyle.FormatString = "{0:N0}";
            this.grid.Cols["theMaximumExtents"].CellStyle.FormatString = "{0:N0}";
            this.grid.Cols["theCompressionType"].CellStyle.FormatString = "{0:N0}";
            this.grid.Cols["theCompressionEOF"].CellStyle.FormatString = "{0:N0}";
            this.grid.Cols["theCompressionRatio"].CellStyle.FormatString = "{0:N2}";
            this.grid.Cols["theMaxSize"].CellStyle.ValueType = typeof(DecimalSizeObject);
            this.grid.Cols["theCurrentEOF"].CellStyle.ValueType = typeof(DecimalSizeObject);
            this.grid.Cols["theCompressionEOF"].CellStyle.ValueType = typeof(DecimalSizeObject);
            this.grid.Cols["thePercentAllocated"].CellStyle.ValueType = typeof(PercentObject);
            
            if (this.PartitionedSchemaObject.ConnectionDefinition != null && this.PartitionedSchemaObject.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ120)
            {
                this.grid.Cols["theInternalOverhead"].CellStyle.FormatString = "{0:N0}";
                this.grid.Cols["theAccessCount"].CellStyle.FormatString = "{0:N0}";
            }
            theCompressionType = -1;

            foreach (Partition partition in this.partitionedSchemaObject.Partitions)
            {
                theCompressionType = partition.TheCompressionType;

                if (this.PartitionedSchemaObject.ConnectionDefinition != null && this.PartitionedSchemaObject.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ120)
                {
                    this.grid.AddRow(new object[] {
                    partition.ThePartitionNum,
                    partition.ThePartitionFileName,
                    partition.TheRowCount,
                    new PercentObject(partition.ThePercentAllocated),
                    new DecimalSizeObject(partition.TheCurrentEOF),
                    theCompressionType>0?string.Format("{0:N0}",new DecimalSizeObject(partition.TheCompressedEOF)):"N/A",
                    new DecimalSizeObject(partition.TheMaxSize),
                    partition.FormattedCompressionType,
                    theCompressionType>0?string.Format("{0:N2}",partition.TheCompressionRatio):"N/A",
                    partition.TheInsertedRowCount,
                    partition.TheDeletedRowCount,
                    partition.TheUpdatedRowCount,
                    partition.ThePrimaryExtentSize,
                    partition.TheSecondaryExtentSize,
                    partition.TheMaxExtents,
                    partition.TheAllocatedExtents,
                    Utilities.FormatSize(partition.TheInternalOverhead),
                    partition.TheAccessCount
                });
                }
                else
                {
                    this.grid.AddRow(new object[] {
                    partition.ThePartitionNum,
                    partition.ThePartitionFileName,
                    partition.TheRowCount,
                    new PercentObject(partition.ThePercentAllocated),
                    new DecimalSizeObject(partition.TheCurrentEOF),
                    theCompressionType>0?string.Format("{0:N0}",new DecimalSizeObject(partition.TheCompressedEOF)):"N/A",
                    new DecimalSizeObject(partition.TheMaxSize),
                    partition.FormattedCompressionType,
                    theCompressionType>0?string.Format("{0:N2}",partition.TheCompressionRatio):"N/A",
                    partition.TheInsertedRowCount,
                    partition.TheDeletedRowCount,
                    partition.TheUpdatedRowCount,
                    partition.ThePrimaryExtentSize,
                    partition.TheSecondaryExtentSize,
                    partition.TheMaxExtents,
                    partition.TheAllocatedExtents
                });
                }

                theTotalRowCount += partition.TheRowCount;
                theTotalMaxSize += partition.TheMaxSize;
                theTotalCurrentEOF += partition.TheCurrentEOF;
                theTotalCompressionEOF += partition.TheCompressedEOF;
                theCompressionType = partition.TheCompressionType;
            }

        }

        private void AttachGrid()
        {
            this.Controls.Add(this.grid);
            string headerText = string.Empty;
            if (theCompressionType > 0)
            {
                headerText = string.Format(HEADER_FORMAT_COMPRESSED,
                                new object[]{"{0}", 
                                theTotalRowCount.ToString("N0"),   
                                DisplayFormatter.FormatSize(theTotalCurrentEOF),
                                DisplayFormatter.FormatSize(theTotalCompressionEOF),
                                DisplayFormatter.FormatSize(theTotalMaxSize)});
            }
            else
            {
                headerText = string.Format(HEADER_FORMAT,
                           new object[]{"{0}", 
                                theTotalRowCount.ToString("N0"),                                         
                                DisplayFormatter.FormatSize(theTotalCurrentEOF),
                                DisplayFormatter.FormatSize(theTotalMaxSize)});
            } 


            this.grid.AddCountControlToParent(headerText, DockStyle.Top);
        }

        private void AddGridColumn()
        {
        }
    }

}
