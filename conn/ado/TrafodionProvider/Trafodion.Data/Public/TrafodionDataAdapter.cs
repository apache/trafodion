/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2009-2015 Hewlett-Packard Development Company, L.P.
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
********************************************************************/

namespace Trafodion.Data
{
    using System;
    using System.Collections.Generic;
    using System.Data;
    using System.Data.Common;

    /// <summary>
    /// Represents a set of data commands and a database connection that are used to fill the DataSet and update a TrafodionDB database. This class cannot be inherited.
    /// </summary>
    public sealed class TrafodionDBDataAdapter : DbDataAdapter, IDbDataAdapter
    {
        private int _updateBatchSize;
        private List<TrafodionDBCommand> _commandBatch;
        private List<Object[]> _batchedParams;

        /// <summary>
        /// Initializes a new instance of the TrafodionDBDataAdapter class.
        /// </summary>
        public TrafodionDBDataAdapter()
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(null, TraceLevel.Public);
            }
            this._batchedParams = new List<object[]>();
            this._updateBatchSize = 1;
        }

        /// <summary>
        /// Initializes a new instance of the TrafodionDBDataAdapter class with the specified TrafodionDBCommand as the SelectCommand property.
        /// </summary>
        /// <param name="selectCommand">The TrafodionDBCommand to set as the SelectCommand property.</param>
        public TrafodionDBDataAdapter(TrafodionDBCommand selectCommand)
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(null, TraceLevel.Public, selectCommand);
            }

            this._batchedParams = new List<object[]>();
            this._updateBatchSize = 1;
            this.SelectCommand = selectCommand;
        }

        /*
        public TrafodionDBDataAdapter(string selectCommandText, string selectConnectionString)
        {

        }

        public TrafodionDBDataAdapter(string selectCommandText, TrafodionDBConnection selectConnection)
        {

        }
        */

        /// <summary>
        /// Occurs during Update before a command is executed against the data source. The attempt to update is made, so the event fires.
        /// </summary>
        public event TrafodionDBRowUpdatingEventHandler RowUpdating;

        /// <summary>
        /// Occurs during Update after a command is executed against the data source. The attempt to update is made, so the event fires.
        /// </summary>
        public event TrafodionDBRowUpdatedEventHandler RowUpdated;

        //these provide strong typing
        protected override RowUpdatedEventArgs CreateRowUpdatedEvent(DataRow dataRow, IDbCommand command, System.Data.StatementType statementType, DataTableMapping tableMapping)
        {
            return new TrafodionDBRowUpdatedEventArgs(dataRow, command, statementType, tableMapping);
        }
        protected override RowUpdatingEventArgs CreateRowUpdatingEvent(DataRow dataRow, IDbCommand command, System.Data.StatementType statementType, DataTableMapping tableMapping)
        {
            return new TrafodionDBRowUpdatingEventArgs(dataRow, command, statementType, tableMapping);
        }

        /// <summary>
        /// Overridden. Raises the RowUpdating event.
        /// </summary>
        /// <param name="value">A TrafodionDBRowUpdatingEventArgs that contains the event data.</param>
        protected override void OnRowUpdating(RowUpdatingEventArgs value)
        {
            if (RowUpdating != null)
            {
                RowUpdating(this, (value as TrafodionDBRowUpdatingEventArgs));
            }
            if (value.StatementType == System.Data.StatementType.Insert)
            {
                this._batchedParams.Add(value.Row.ItemArray);
            }
        }

        /// <summary>
		/// Overridden. Raises the RowUpdated event.
		/// </summary>
		/// <param name="value">A TrafodionDBUpdatedEventArgs that contains the event data. </param>
        override protected void OnRowUpdated(RowUpdatedEventArgs value)
        {
            if (RowUpdated != null)
            {
                RowUpdated(this, (value as TrafodionDBRowUpdatedEventArgs));
            }
            this._batchedParams.Clear();
        }

        public new TrafodionDBCommand SelectCommand { get; set; }
        public new TrafodionDBCommand InsertCommand { get; set; }
        public new TrafodionDBCommand DeleteCommand { get; set; }
        public new TrafodionDBCommand UpdateCommand { get; set; }

        /// <summary>
        /// Gets the parameters set by the user when executing an SQL SELECT statement.
        /// </summary>
        public new TrafodionDBParameter[] GetFillParameters()
        {
            TrafodionDBParameter[] parameters = new TrafodionDBParameter[SelectCommand.Parameters.Count];
            for( int index = 0; index < SelectCommand.Parameters.Count; index++ )
            {
                parameters[index] = SelectCommand.Parameters[index];
            }

            return parameters;
        }

        /// <summary>
        /// Returns a TrafodionDBParameter from one of the commands in the current batch.
        /// </summary>
        public new TrafodionDBParameter GetBatchedParameter(int commandIdentifier, int parameterIndex)
        {
            return (TrafodionDBParameter)_commandBatch[commandIdentifier].Parameters[parameterIndex];
        }

        public override int UpdateBatchSize
        {
            get
            {
                return this._updateBatchSize;
            }
            set
            {
                this._updateBatchSize = value;
            }
        }

        protected override void InitializeBatching()
        {
            if (TrafodionDBTrace.IsInternalEnabled)
            {
                TrafodionDBTrace.Trace(null, TraceLevel.Internal);
            }

            _commandBatch = new List<TrafodionDBCommand>();
        }

        protected int AddToBatch(TrafodionDBCommand command)
        {
            if (TrafodionDBTrace.IsInternalEnabled)
            {
                TrafodionDBTrace.Trace(null, TraceLevel.Internal);
            }

            _commandBatch.Add((TrafodionDBCommand)((ICloneable)command).Clone());

            return _commandBatch.Count - 1;
        }

        protected override int AddToBatch(IDbCommand command)
        {
            return this.AddToBatch((TrafodionDBCommand)command);
        }

        protected override int ExecuteBatch()
        {
            if (TrafodionDBTrace.IsInternalEnabled)
            {
                TrafodionDBTrace.Trace(null, TraceLevel.Internal);
            }

            int recordsAffected = 0;
            int index = 0;
            if (this.InsertCommand != null)
            {
                TrafodionDBCommand command = this._commandBatch[0];
                for (int i = 0; i < this._batchedParams.Count; i++)
                {
                    command.AddBatch(this._batchedParams[i]);
                }

                recordsAffected = command.ExecuteNonQuery();
            }
            else if(this.SelectCommand != null)
            {
                while (index < _commandBatch.Count)
                {
                    TrafodionDBCommand command = _commandBatch[index++];
                    recordsAffected += command.ExecuteNonQuery();
                }
            }

            return recordsAffected;
        }

        protected override void ClearBatch()
        {
            if (TrafodionDBTrace.IsInternalEnabled)
            {
                TrafodionDBTrace.Trace(null, TraceLevel.Internal);
            }

            _commandBatch.Clear();
        }

        protected override void TerminateBatching()
        {
            if (TrafodionDBTrace.IsInternalEnabled)
            {
                TrafodionDBTrace.Trace(null, TraceLevel.Internal);
            }

            this.ClearBatch();
            this._commandBatch = null;
        }

        protected override int Update(DataRow[] dataRows, DataTableMapping tableMapping)
        {
            if (this.UpdateCommand != null)
            {
                this.UpdateCommand.Prepare();
            }
            if (this.InsertCommand != null)
            {
                this.InsertCommand.Prepare();
            }
            if (this.DeleteCommand != null)
            {
                this.DeleteCommand.Prepare();
            }

            return base.Update(dataRows, tableMapping);
        }

        public new void Dispose()
        {
            base.Dispose();
        }

        IDbCommand IDbDataAdapter.SelectCommand
        {
            get { return this.SelectCommand; }
            set { this.SelectCommand = (TrafodionDBCommand)value; }
        }

        IDbCommand IDbDataAdapter.InsertCommand
        {
            get { return this.InsertCommand; }
            set { this.InsertCommand = (TrafodionDBCommand)value; }
        }

        IDbCommand IDbDataAdapter.DeleteCommand
        {
            get { return this.DeleteCommand; }
            set { this.DeleteCommand = (TrafodionDBCommand)value; }
        }

        IDbCommand IDbDataAdapter.UpdateCommand
        {
            get { return this.UpdateCommand; }
            set { this.UpdateCommand = (TrafodionDBCommand)value; }
        }

        IDataParameter[] IDataAdapter.GetFillParameters()
        {
            return this.GetFillParameters();
        }
    }
}
