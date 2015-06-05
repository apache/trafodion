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
    /// Represents a SQL transaction to be made in the database. This class cannot be inherited.
    /// </summary>
    public sealed class TrafodionDBTransaction : DbTransaction, IDbTransaction
    {
        private TrafodionDBConnection _conn;
        private IsolationLevel _isoLevel;

        internal TrafodionDBTransaction(TrafodionDBConnection conn, IsolationLevel isolation)
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(null, TraceLevel.Public, isolation);
            }

            this._conn = conn;
            this._isoLevel = isolation;

            // an unspecified IsolationLevel means we should use the current settings
            if (isolation != IsolationLevel.Unspecified)
            {
                this.SetIsolationLevel(isolation);
            }

            this.SetAutoCommit(false);
        }

        /// <summary>
        /// Gets the TrafodionDBConnection object associated with the transaction, or null if the transaction is no longer valid.
        /// </summary>
        public new TrafodionDBConnection Connection
        {
            get
            {
                return this._conn;
            }
        }

        /// <summary>
        /// Specifies the IsolationLevel for this transaction.
        /// </summary>
        public override IsolationLevel IsolationLevel
        {
            get
            {
                return this._isoLevel;
            }
        }

        /// <summary>
        /// Specifies the DbConnection object associated with the transaction.
        /// </summary>
        protected override DbConnection DbConnection
        {
            get
            {
                return this._conn;
            }
        }

        /// <summary>
        /// Commits the database transaction.
        /// </summary>
        /// <exception cref="InvalidOperationException">
        /// The transaction has already been committed or rolled back.
        /// -or-
        /// The connection is broken.
        /// </exception>
        /// <exception cref="TrafodionDBException">An error occurred while trying to commit the transaction.</exception>
        public override void Commit()
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(this._conn, TraceLevel.Public);
            }

            if (this._conn == null)
            {
                string msg = TrafodionDBResources.FormatMessage(TrafodionDBMessage.InvalidTransactionState);
                TrafodionDBException.ThrowException(this._conn, new InvalidOperationException(msg));
            }

            if (this._conn.State != ConnectionState.Open)
            {
                string msg = TrafodionDBResources.FormatMessage(TrafodionDBMessage.InvalidConnectionState, this._conn.State);
                TrafodionDBException.ThrowException(this._conn, new InvalidOperationException(msg));
            }

            this.EndTransaction(EndTransactionOption.Commit);
            this.SetAutoCommit(true);

            this._conn.Transaction = null;
            this._conn = null;
        }

        /// <summary>
        /// Rolls back a transaction from a pending state.
        /// </summary>
        /// /// <exception cref="InvalidOperationException">
        /// The transaction has already been committed or rolled back.
        /// -or-
        /// The connection is broken.
        /// </exception>
        /// <exception cref="TrafodionDBException">An error occurred while trying to rollback the transaction.</exception>
        public override void Rollback()
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(this._conn, TraceLevel.Public);
            }

            if (this._conn == null)
            {
                string msg = TrafodionDBResources.FormatMessage(TrafodionDBMessage.InvalidTransactionState);
                TrafodionDBException.ThrowException(this._conn, new InvalidOperationException(msg));
            }

            if (this._conn.State != ConnectionState.Open)
            {
                string msg = TrafodionDBResources.FormatMessage(TrafodionDBMessage.InvalidConnectionState, this._conn.State);
                TrafodionDBException.ThrowException(this._conn, new InvalidOperationException(msg));
            }

            this.EndTransaction(EndTransactionOption.Rollback);
            this.SetAutoCommit(true);

            this._conn.Transaction = null;
            this._conn = null;
        }

        private void EndTransaction(EndTransactionOption opt)
        {
            if (TrafodionDBTrace.IsInternalEnabled)
            {
                TrafodionDBTrace.Trace(this._conn, TraceLevel.Internal, opt);
            }

            EndTransactionMessage message;
            EndTransactionReply reply;

            message = new EndTransactionMessage()
            {
                Option = opt
            };

            reply = this._conn.Network.EndTransaction(message);

            if (reply.error == EndTransactionError.SqlError)
            {
                throw new TrafodionDBException(reply.errorDesc);
            }
            else if (reply.error != EndTransactionError.Success)
            {
                string msg = TrafodionDBResources.FormatMessage(
                    TrafodionDBMessage.InternalError,
                    "EndTransaction",
                    reply.error.ToString() + " " + reply.errorDetail + " " + reply.errorText);

                TrafodionDBException.ThrowException(this._conn, new InternalFailureException(msg, null));
            }
        }

        private void SetAutoCommit(bool enabled)
        {
            if (TrafodionDBTrace.IsInternalEnabled)
            {
                TrafodionDBTrace.Trace(this._conn, TraceLevel.Internal, enabled);
            }

            SetConnectionOptMessage message;
            SetConnectionOptReply reply;

            message = new SetConnectionOptMessage()
            {
                Option = ConnectionOption.AutoCommit,
                IntValue = enabled ? 1 : 0,
                StringValue = string.Empty
            };

            reply = this._conn.Network.SetConnectionOpt(message);

            if (reply.error == SetConnectionOptError.SqlError)
            {
                throw new TrafodionDBException(reply.errorDesc);
            }
            else if (reply.error != SetConnectionOptError.Success)
            {
                string msg = TrafodionDBResources.FormatMessage(
                    TrafodionDBMessage.InternalError,
                    "SetAutoCommit",
                    reply.error.ToString() + " " + reply.errorDetail + " " + reply.errorText);

                TrafodionDBException.ThrowException(this._conn, new InternalFailureException(msg, null));
            }
        }

        private void SetIsolationLevel(IsolationLevel il)
        {
            if (TrafodionDBTrace.IsInternalEnabled)
            {
                TrafodionDBTrace.Trace(this._conn, TraceLevel.Internal, il);
            }

            SetConnectionOptMessage message;
            SetConnectionOptReply reply;

            TransactionIsolation neoIl = TransactionIsolation.ReadCommmited;

            switch (il)
            {
                case IsolationLevel.ReadCommitted:
                    neoIl = TransactionIsolation.ReadCommmited;
                    break;
                case IsolationLevel.ReadUncommitted:
                    neoIl = TransactionIsolation.ReadUncommitted;
                    break;
                case IsolationLevel.Serializable:
                    neoIl = TransactionIsolation.Serializable;
                    break;
                case IsolationLevel.RepeatableRead:
                    neoIl = TransactionIsolation.RepeatableRead;
                    break;
                default:
                    string msg = TrafodionDBResources.FormatMessage(TrafodionDBMessage.UnsupportedIsolationLevel, il.ToString());
                    TrafodionDBException.ThrowException(this._conn, new ArgumentException(msg));
                    break;
            }

            message = new SetConnectionOptMessage()
            {
                Option = ConnectionOption.TransactionIsolation,
                IntValue = (int)neoIl,
                StringValue = string.Empty
            };

            reply = this._conn.Network.SetConnectionOpt(message);

            if (reply.error == SetConnectionOptError.SqlError)
            {
                throw new TrafodionDBException(reply.errorDesc);
            }
            else if (reply.error != SetConnectionOptError.Success)
            {
                string msg = TrafodionDBResources.FormatMessage(
                   TrafodionDBMessage.InternalError,
                   "SetIsolationLevel",
                   reply.error.ToString() + " " + reply.errorDetail + " " + reply.errorText);

                TrafodionDBException.ThrowException(this._conn, new InternalFailureException(msg, null));
            }
        }
    }
}
