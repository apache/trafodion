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
    using System.Data;
    using System.Data.Common;

    /// <summary>
    /// The callback for Info and Warning events for a TrafodionDBConnection.
    /// </summary>
    /// <param name="sender">The TrafodionDBConnection sending the event.</param>
    /// <param name="e">The event arguments.</param>
    public delegate void TrafodionDBInfoMessageEventHandler(object sender, TrafodionDBInfoMessageEventArgs e);

    /// <summary>
    /// Represents the method that will handle the event of a TrafodionDBDataAdapter.
    /// </summary>
    /// <param name="sender">The object sending the event.</param>
    /// <param name="e">The event arguments.</param>
    public delegate void TrafodionDBRowUpdatingEventHandler(object sender, TrafodionDBRowUpdatingEventArgs e);

    /// <summary>
    /// Represents the method that will handle the event of a TrafodionDBDataAdapter.
    /// </summary>
    /// <param name="sender">The object sending the event.</param>
    /// <param name="e">The event arguments.</param>
    public delegate void TrafodionDBRowUpdatedEventHandler(object sender, TrafodionDBRowUpdatedEventArgs e);

    /// <summary>
    /// The event arguments for Info and Warning events for a TrafodionDBConnection.
    /// </summary>
    public sealed class TrafodionDBInfoMessageEventArgs : EventArgs
    {
        internal TrafodionDBInfoMessageEventArgs(TrafodionDBErrorCollection errors)
        {
            this.Errors = errors;
            this.ErrorCode = errors[0].ErrorCode;
            this.Message = errors[0].Message;
            this.State = errors[0].State;
            this.RowId = errors[0].RowId;
        }

        /// <summary>
        /// Gets the message text of the error.
        /// </summary>
        public string Message { get; private set; }

        /// <summary>
        /// Gets the error code of the error.
        /// </summary>
        public int ErrorCode { get; private set; }

        /// <summary>
        /// Gets the SqlState of the error.
        /// </summary>
        public string State { get; private set; }

        /// <summary>
        /// Gets the RowId of the error.
        /// </summary>
        public int RowId { get; private set; }

        /// <summary>
        /// Gets a collection of all the errors that occured.
        /// </summary>
        public TrafodionDBErrorCollection Errors { get; private set; }
    }

    /// <summary>
    /// Provides data for the RowUpdating event.
    /// </summary>
    public sealed class TrafodionDBRowUpdatingEventArgs : RowUpdatingEventArgs
    {
        /// <summary>
        /// Initializes a new instance of the TrafodionDBRowUpdatingEventArgs class.
        /// </summary>
        /// <param name="row">The DataRow sent through an Update.</param>
        /// <param name="command">The IDbCommand executed when Update is called.</param>
        /// <param name="statementType">One of the StatementType values that specifies the type of query executed.</param>
        /// <param name="tableMapping">The DataTableMapping sent through an Update.</param>
        public TrafodionDBRowUpdatingEventArgs(DataRow row, IDbCommand command, System.Data.StatementType statementType, DataTableMapping tableMapping)
            : base(row, command, statementType, tableMapping)
        {
        }

        /// <summary>
        /// Gets or sets the TrafodionDBCommand executed when Update is called.
        /// </summary>
        public new TrafodionDBCommand Command
        {
            get { return (TrafodionDBCommand)base.Command; }
            set { base.Command = value; }
        }
    }

    /// <summary>
    /// Provides data for the RowUpdated event.
    /// </summary>
    public sealed class TrafodionDBRowUpdatedEventArgs : RowUpdatedEventArgs
    {
        /// <summary>
        /// Initializes a new instance of the TrafodionDBRowUpdatedEventArgs class.
        /// </summary>
        /// <param name="row">The DataRow sent through an Update.</param>
        /// <param name="command">The IDbCommand executed when Update is called.</param>
        /// <param name="statementType">One of the StatementType values that specifies the type of query executed.</param>
        /// <param name="tableMapping">The DataTableMapping sent through an Update.</param>
        public TrafodionDBRowUpdatedEventArgs(DataRow row, IDbCommand command, System.Data.StatementType statementType, DataTableMapping tableMapping)
            : base(row, command, statementType, tableMapping)
        {
        }

        /// <summary>
        /// Gets the TrafodionDBCommand executed when Update is called.
        /// </summary>
        public new TrafodionDBCommand Command
        {
            get { return (TrafodionDBCommand)base.Command; }
        }
    }
}
