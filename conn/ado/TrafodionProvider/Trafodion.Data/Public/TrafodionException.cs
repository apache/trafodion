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
    using System.Data.Common;

    /// <summary>
    /// The exception that is thrown when TrafodionDB returns a warning or error. This class cannot be inherited.
    /// </summary>
    [Serializable]
    public sealed class TrafodionDBException : DbException
    {
        private TrafodionDBErrorCollection _errors;
        private TrafodionDBErrorCollection _warnings;

        //overwrite the base classes since we cant use thier constructor for everything
        private string _message;
        private int _errorCode;

        internal static void ThrowException(TrafodionDBConnection conn, Exception e)
        {
            if (TrafodionDBTrace.IsErrorEnabled)
            {
                TrafodionDBTrace.Trace(conn, TraceLevel.Error, e);
            }

            throw e;
        }

        internal TrafodionDBException(TrafodionDBMessage msg, params object[] objs)
        {
            this._errors = new TrafodionDBErrorCollection();
            this._warnings = new TrafodionDBErrorCollection();

            TrafodionDBError error = new TrafodionDBError()
            {
                ErrorCode = 0,
                Message = (objs != null)?TrafodionDBResources.FormatMessage(msg, objs):TrafodionDBResources.GetMessage(msg),
                RowId = -1,
                State = "00000"
            };

            if (error.ErrorCode > 0)
            {
                this.Warnings.Add(error);
            }
            else
            {
                this.Errors.Add(error);
            }

            SetMembers();


            if (TrafodionDBTrace.IsErrorEnabled)
            {
                TrafodionDBTrace.Trace(null, TraceLevel.Error, this);
            }
        }

        internal TrafodionDBException(ErrorDesc[] list)
        {
            TrafodionDBError error;

            this._errors = new TrafodionDBErrorCollection();
            this._warnings = new TrafodionDBErrorCollection();

            foreach (ErrorDesc d in list)
            {
                error = new TrafodionDBError()
                {
                    ErrorCode = d.sqlCode,
                    Message = d.errorText,
                    RowId = d.rowId,
                    State = d.sqlState
                };

                if (error.ErrorCode > 0)
                {
                    this._warnings.Add(error);
                }
                else
                {
                    this._errors.Add(error);
                }
            }

            this.SetMembers();

            if (TrafodionDBTrace.IsErrorEnabled)
            {
                TrafodionDBTrace.Trace(null, TraceLevel.Error, this);
            }
        }

        internal TrafodionDBException(SqlWarningOrError[] list)
        {
            TrafodionDBError error;

            this._errors = new TrafodionDBErrorCollection();
            this._warnings = new TrafodionDBErrorCollection();

            foreach (SqlWarningOrError e in list)
            {
                error = new TrafodionDBError()
                {
                    ErrorCode = e.sqlCode,
                    Message = e.text,
                    RowId = e.rowId,
                    State = e.sqlState
                };

                if (error.ErrorCode > 0)
                {
                    this._warnings.Add(error);
                }
                else
                {
                    this._errors.Add(error);
                }
            }

            this.SetMembers();

            if (TrafodionDBTrace.IsErrorEnabled)
            {
                TrafodionDBTrace.Trace(null, TraceLevel.Error, this);
            }
        }

        /// <summary>
        /// Gets a collection of one or more TrafodionDBError objects that give detailed information about exceptions generated.
        /// </summary>
        public TrafodionDBErrorCollection Errors
        {
            get { return this._errors; }
        }

        /// <summary>
        /// Gets a collection of one or more TrafodionDBError objects that give detailed information about warnings generated.
        /// </summary>
        public TrafodionDBErrorCollection Warnings
        {
            get { return this._warnings; }
        }

        //these should be the same as defined in HPError and InfoMessageEventArgs
        public int RowId { get; private set; }
        public override string Message { get { return _message; } }
        public override int ErrorCode { get { return _errorCode; } }
        public string State { get; private set; }

        /// <summary>
        /// Creates and returns a string representation of the current exception.
        /// </summary>
        /// <returns>A string representation of the current exception.</returns>
        public override string ToString()
        {
            return this._message;
        }

        private void SetMembers()
        {
            //we might get initialized with only warnings
            if (this.Errors.Count > 0)
            {
                this._message = this._errors[0].Message;
                this._errorCode = this._errors[0].ErrorCode;
                this.State = this._errors[0].State;
                this.RowId = this._errors[0].RowId;
            }
            else if(this.Warnings.Count > 0)
            {
                this._message = this._warnings[0].Message;
                this._errorCode = this._warnings[0].ErrorCode;
                this.State = this._warnings[0].State;
                this.RowId = this._warnings[0].RowId;
            }
        }
    }

    /// <summary>
    /// The exception that is thrown when an internal error occurs.
    /// </summary>
    [Serializable]
    public sealed class InternalFailureException : Exception
    {
        internal InternalFailureException(string msg, Exception innerException)
            : base(msg, innerException)
        {

        }
    }

    /// <summary>
    /// The exception that is thrown when a communcations failure occurs
    /// </summary>
    [Serializable]
    public sealed class CommunicationsFailureException : Exception
    {
        internal CommunicationsFailureException(Exception innerException)
            : base("Communications Failure: " + innerException.Message, innerException)
        {

        }

        internal CommunicationsFailureException(string msg)
            : base("Communications Failure: " + msg)
        {

        }
    }
}
