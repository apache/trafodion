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
using Trafodion.Manager.DatabaseArea.Model;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
namespace Trafodion.Manager.DatabaseArea.Controls
{
    public class PartitionsDataGridView : DatabaseAreaObjectsDataGridView
    {
        private PartitionedSchemaObject thePartitionedSchemaObject;
        private long theTotalRowCount = 0;
        private long theTotalMaxSize = 0;
        private long theTotalCurrentEOF = 0;
        private long theTotalCompressionEOF = 0;
        private int theCompressionType = -1;

        public PartitionedSchemaObject ThePartitionedSchemaObject
        {
            get { return thePartitionedSchemaObject; }
            set { thePartitionedSchemaObject = value; }
        }

        public PartitionsDataGridView(PartitionedSchemaObject aPartitionedSchemaObject)
        : base(null)
        {
            this.AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.None;
            this.ScrollBars = ScrollBars.Both;
            ThePartitionedSchemaObject = aPartitionedSchemaObject;
            PercentAllocatedColumn tempPercentAllocatedColumn = new PercentAllocatedColumn();
            tempPercentAllocatedColumn.CellTemplate = new PercentAllocatedCell();
            Columns.Add("thePartitionNumber", "Partition Number");
            //Columns.Add("theSegmentColumn", "Segment");
            //Columns.Add("theWindowTitleColumn", "Name");
            Columns.Add("theFilenameColumn", "File");
            Columns.Add("theRowCount", "Row Count");
            Columns.Add("thePercentAllocated", "Percent Allocated");
            Columns.Add("theCurrentEOF", "Current Size");
            Columns.Add("theCompressionEOF", "Compressed Size");
            Columns.Add("theMaxSize", "Max Size");
            Columns.Add("theCompressionType", "Compression Type");
            Columns.Add("theCompressionRatio", "Compression Ratio");
            Columns.Add("theInsertedRowCount", "Rows Inserted Since Last Update Stats");
            Columns.Add("theDeletedRowCount", "Rows Deleted Since Last Update Stats");
            Columns.Add("theUpdatedRowCount", "Rows Updated Since Last Update Stats");
            Columns.Add("thePrimaryExtents", "Primary Extents");
            Columns.Add("theSecondaryExtents", "Secondary Extents");
            Columns.Add("theMaximumExtents", "Max Extents");
            Columns.Add("theAllocatedExtents", "Allocated Extents");
            if (ThePartitionedSchemaObject.ConnectionDefinition != null && ThePartitionedSchemaObject.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ120)
            {
                Columns.Add("theInternalOverhead", "Overhead");
                Columns.Add("theAccessCount", "Access Count");
            }

            //Columns["theWindowTitleColumn"].Width = 185;
            Columns["theFilenameColumn"].Width = 165;			
            Columns["theRowCount"].DefaultCellStyle.Format = "N0";
            Columns["theInsertedRowCount"].DefaultCellStyle.Format = "N0";
            Columns["theDeletedRowCount"].DefaultCellStyle.Format = "N0";
            Columns["theUpdatedRowCount"].DefaultCellStyle.Format = "N0";
            Columns["thePrimaryExtents"].DefaultCellStyle.Format = "N0";
            Columns["theSecondaryExtents"].DefaultCellStyle.Format = "N0";
            Columns["theAllocatedExtents"].DefaultCellStyle.Format = "N0";
            Columns["theMaximumExtents"].DefaultCellStyle.Format = "N0";
            Columns["theCompressionType"].DefaultCellStyle.Format = "N0";
            Columns["theCompressionEOF"].DefaultCellStyle.Format = "N0";
            Columns["theCompressionRatio"].DefaultCellStyle.Format = "N2";
            Columns["theMaxSize"].ValueType = typeof(DecimalSizeObject);
            Columns["theCurrentEOF"].ValueType = typeof(DecimalSizeObject);
            Columns["theCompressionEOF"].ValueType = typeof(DecimalSizeObject);
            Columns["thePercentAllocated"].ValueType = typeof(PercentObject);

            if (ThePartitionedSchemaObject.ConnectionDefinition != null && ThePartitionedSchemaObject.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ120)
            {
                Columns["theInternalOverhead"].DefaultCellStyle.Format = "N0";
                Columns["theAccessCount"].DefaultCellStyle.Format = "N0";
            }
            theCompressionType = -1;
            foreach (Partition thePartition in ThePartitionedSchemaObject.Partitions)
            {
                theCompressionType = thePartition.TheCompressionType;

                if (ThePartitionedSchemaObject.ConnectionDefinition != null && ThePartitionedSchemaObject.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ120)
                {
                    
                    Rows.Add(new object[] {
                    thePartition.ThePartitionNum,
                    //thePartition.TheName, 
                    thePartition.ThePartitionFileName,
                    thePartition.TheRowCount,
                    new PercentObject(thePartition.ThePercentAllocated),
                    new DecimalSizeObject(thePartition.TheCurrentEOF),
                    new DecimalSizeObject(thePartition.TheCompressedEOF),
                    new DecimalSizeObject(thePartition.TheMaxSize),
                    thePartition.FormattedCompressionType,
                    thePartition.TheCompressionRatio,
                    thePartition.TheInsertedRowCount,
                    thePartition.TheDeletedRowCount,
                    thePartition.TheUpdatedRowCount,
                    thePartition.ThePrimaryExtentSize,
                    thePartition.TheSecondaryExtentSize,
                    thePartition.TheMaxExtents,
                    thePartition.TheAllocatedExtents,
                    Utilities.FormatSize(thePartition.TheInternalOverhead),
                    thePartition.TheAccessCount
                });
                }
                else
                {
                    Rows.Add(new object[] {
                    thePartition.ThePartitionNum,
                    //thePartition.TheName, 
                    thePartition.ThePartitionFileName,
                    thePartition.TheRowCount,
                    new PercentObject(thePartition.ThePercentAllocated),
                    new DecimalSizeObject(thePartition.TheCurrentEOF),
                    new DecimalSizeObject(thePartition.TheCompressedEOF),
                    new DecimalSizeObject(thePartition.TheMaxSize),
                    thePartition.FormattedCompressionType,
                    thePartition.TheCompressionRatio,
                    thePartition.TheInsertedRowCount,
                    thePartition.TheDeletedRowCount,
                    thePartition.TheUpdatedRowCount,
                    thePartition.ThePrimaryExtentSize,
                    thePartition.TheSecondaryExtentSize,
                    thePartition.TheMaxExtents,
                    thePartition.TheAllocatedExtents
                });
                }

                theTotalRowCount += thePartition.TheRowCount;
                theTotalMaxSize += thePartition.TheMaxSize;
                theTotalCurrentEOF += thePartition.TheCurrentEOF;
                theTotalCompressionEOF += thePartition.TheCompressedEOF;
                theCompressionType = thePartition.TheCompressionType;
            }

            if (theCompressionType < 1)
            {
                if (Columns.Contains("theCompressionEOF"))
                {
                    Columns.Remove("theCompressionEOF");
                }
                if (Columns.Contains("theCompressionRatio"))
                {
                    Columns.Remove("theCompressionRatio");
                }
   
                Columns["theCurrentEOF"].HeaderText = "Current Size";
            }

            //Resize each column
            //foreach (DataGridViewColumn col in this.Columns)
            //{
            //    col.AutoSizeMode = DataGridViewAutoSizeColumnMode.DisplayedCells;
            //}

        }
            
    }
}
