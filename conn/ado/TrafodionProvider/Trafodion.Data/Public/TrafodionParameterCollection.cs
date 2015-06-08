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
    using System.Collections;
    using System.Collections.Generic;
    using System.Data;
    using System.Data.Common;

    /// <summary>
    /// Represents a collection of parameters associated with a TrafodionDBCommand and their respective mappings to columns in a DataSet. This class cannot be inherited.
    /// </summary>
    public sealed class TrafodionDBParameterCollection : DbParameterCollection, IDataParameterCollection
    {
        private TrafodionDBCommand _cmd;
        private List<TrafodionDBParameter> _parameters;
        private object _syncObject;

        internal TrafodionDBParameterCollection(TrafodionDBCommand cmd)
        {
            this._parameters = new List<TrafodionDBParameter>();
            this._syncObject = new object();

            this._cmd = cmd;
        }

        /// <summary>
        /// Returns an Integer that contains the number of elements in the TrafodionDBParameterCollection. Read-only.
        /// </summary>
        public override int Count
        {
            get { return this._parameters.Count; }
        }

        /// <summary>
        /// Gets a value that indicates whether the TrafodionDBParameterCollection has a fixed size.
        /// </summary>
        public override bool IsFixedSize
        {
            get { return false; }
        }

        /// <summary>
        /// Gets a value that indicates whether the TrafodionDBParameterCollection is read-only.
        /// </summary>
        public override bool IsReadOnly
        {
            get { return false; }
        }

        /// <summary>
        /// Gets a value that indicates whether the TrafodionDBParameterCollection is synchronized.
        /// </summary>
        public override bool IsSynchronized
        {
            get { return false; }
        }

        /// <summary>
        /// Gets an object that can be used to synchronize access to the TrafodionDBParameterCollection.
        /// </summary>
        public override object SyncRoot
        {
            get { return this._syncObject; }
        }

        internal List<TrafodionDBParameter> Parameters
        {
            get { return this._parameters; }
        }

        /// <summary>
        /// Gets the TrafodionDBParameter with the specified name.
        /// </summary>
        /// <param name="parameterName">The name of the parameter to retrieve. </param>
        /// <returns>The TrafodionDBParameter with the specified name.</returns>
        /// <exception cref="IndexOutOfRangeException">The specified parameterName is not valid.</exception>
        public new TrafodionDBParameter this[string parameterName]
        {
            get
            {
                return this._parameters[this.IndexOf(parameterName)];
            }

            set
            {
                this._parameters[this.IndexOf(parameterName)] = value;
                this._cmd.IsPrepared = false;
            }
        }

        /// <summary>
        /// Gets the TrafodionDBParameter at the specified index.
        /// </summary>
        /// <param name="index">The zero-based index of the parameter to retrieve.</param>
        /// <returns>The TrafodionDBParameter at the specified index.</returns>
        /// <exception cref="IndexOutOfRangeException">The specified index does not exist.</exception>
        public new TrafodionDBParameter this[int index]
        {
            get
            {
                return this._parameters[index];
            }

            set
            {
                this._parameters[index] = value;
                this._cmd.IsPrepared = false;
            }
        }

        /// <summary>
        /// Adds the specified TrafodionDBParameter object to the TrafodionDBParameterCollection.
        /// </summary>
        /// <param name="value">The TrafodionDBParameter to add to the collection. </param>
        /// <returns>The index of the new TrafodionDBParameter object.</returns>
        /// <exception cref="InvalidCastException">The parameter passed was not a TrafodionDBParameter.</exception>
        /// <exception cref="ArgumentNullException">The value parameter is null.</exception>
        public override int Add(object value)
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(this._cmd.Connection, TraceLevel.Public, value);
            }

            if (value == null)
            {
                TrafodionDBException.ThrowException(null, new ArgumentNullException("value"));
            }

            TrafodionDBParameter param = (TrafodionDBParameter)value;

            this._parameters.Add(param);
            this._cmd.IsPrepared = false;

            return this._parameters.IndexOf(param);
        }

        /// <summary>
        /// Adds an Enumerable Collection of TrafodionDBParameter objects to the end of the TrafodionDBParameterCollection.
        /// </summary>
        /// <param name="values">The Enumerable Collection of TrafodionDBParameter objects to add.</param>
        /// <exception cref="InvalidCastException">The parameter passed was not an Enumerable Collection of TrafodionDBParameter objects.</exception>
        /// <exception cref="ArgumentNullException">The values parameter is null.</exception>
        public override void AddRange(Array values)
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(this._cmd.Connection, TraceLevel.Public, values);
            }

            if (values == null)
            {
                TrafodionDBException.ThrowException(null, new ArgumentNullException("values"));
            }

            foreach (TrafodionDBParameter p in (IEnumerable<TrafodionDBParameter>)values)
            {
                this.Add(p);
            }
        }

        /// <summary>
        /// Removes all the TrafodionDBParameter objects from the TrafodionDBParameterCollection.
        /// </summary>
        public override void Clear()
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(this._cmd.Connection, TraceLevel.Public);
            }

            this._parameters.Clear();
            this._cmd.IsPrepared = false;
        }

        /// <summary>
        /// Gets a value indicating whether a TrafodionDBParameter in this TrafodionDBParameterCollection has the specified name.
        /// </summary>
        /// <param name="value">The name of the TrafodionDBParameter. </param>
        /// <returns>true if the TrafodionDBParameterCollections contains the TrafodionDBParameter; otherwise false.</returns>
        public override bool Contains(string value)
        {
            return this.IndexOf(value) != -1;
        }

        /// <summary>
        /// Determines whether the specified TrafodionDBParameter is in this TrafodionDBParameterCollection.
        /// </summary>
        /// <param name="value">The TrafodionDBParameter value.</param>
        /// <returns>true if the TrafodionDBParameterCollections contains the TrafodionDBParameter; otherwise false.</returns>
        /// <exception cref="InvalidCastException">The parameter passed was not a TrafodionDBParameter.</exception>
        public override bool Contains(object value)
        {
            return this._parameters.Contains((TrafodionDBParameter)value);
        }

        /// <summary>
        /// Copies all the elements of the current TrafodionDBParameterCollection to the specified TrafodionDBParameterCollection starting at the specified destination index.
        /// </summary>
        /// <param name="array">The TrafodionDBParameterCollection that is the destination of the elements copied from the current TrafodionDBParameterCollection.</param>
        /// <param name="index">A 32-bit integer that represents the index in the TrafodionDBParameterCollection at which copying starts.</param>
        /// <exception cref="InvalidCastException">The parameter passed was not a TrafodionDBParameterCollection.</exception>
        public override void CopyTo(Array array, int index)
        {
            this._parameters.CopyTo((TrafodionDBParameter[])array, index);
        }

        /// <summary>
        /// Returns an enumerator that iterates through the TrafodionDBParameterCollection.
        /// </summary>
        /// <returns>An IEnumerator for the TrafodionDBParameterCollection.</returns>
        public override IEnumerator GetEnumerator()
        {
            return this._parameters.GetEnumerator();
        }

        /// <summary>
        /// Gets the location of the specified TrafodionDBParameter with the specified name.
        /// </summary>
        /// <param name="parameterName">The case-sensitive name of the TrafodionDBParameter to find.</param>
        /// <returns>The zero-based location of the specified TrafodionDBParameter with the specified case-sensitive name. Returns -1 when the object does not exist in the TrafodionDBParameterCollection.</returns>
        public override int IndexOf(string parameterName)
        {
            for (int i = 0; i < this._parameters.Count; i++)
            {
                if (this._parameters[i].ParameterName.Equals(parameterName))
                {
                    return i;
                }
            }

            return -1;
        }

        /// <summary>
        /// Gets the location of the specified TrafodionDBParameter within the collection.
        /// </summary>
        /// <param name="value">The TrafodionDBParameter to find.</param>
        /// <returns>The zero-based location of the specified TrafodionDBParameter that is a TrafodionDBParameter within the collection. Returns -1 when the object does not exist in the TrafodionDBParameterCollection.</returns>
        /// <exception cref="InvalidCastException">The parameter passed was not a TrafodionDBParameter.</exception>
        public override int IndexOf(object value)
        {
            return this.IndexOf(((TrafodionDBParameter)value).ParameterName);
        }

        /// <summary>
        /// Inserts a TrafodionDBParameter object into the TrafodionDBParameterCollection at the specified index.
        /// </summary>
        /// <param name="index">The zero-based index at which value should be inserted.</param>
        /// <param name="value">A TrafodionDBParameter object to be inserted in the TrafodionDBParameterCollection.</param>
        /// <exception cref="InvalidCastException">The parameter passed was not a TrafodionDBParameter.</exception>
        public override void Insert(int index, object value)
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(this._cmd.Connection, TraceLevel.Public, index, value);
            }

            this._parameters.Insert(index, (TrafodionDBParameter)value);
            this._cmd.IsPrepared = false;
        }

        /// <summary>
        /// Removes the specified TrafodionDBParameter from the collection.
        /// </summary>
        /// <param name="value">A TrafodionDBParameter object to remove from the collection.</param>
        /// <exception cref="InvalidCastException">The parameter passed was not a TrafodionDBParameter.</exception>
        public override void Remove(object value)
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(this._cmd.Connection, TraceLevel.Public, value);
            }

            TrafodionDBParameter param = (TrafodionDBParameter)value;
            if (this._parameters.Remove(param))
            {
                this._cmd.IsPrepared = false;
            }
        }

        /// <summary>
        /// Removes the TrafodionDBParameter from the TrafodionDBParameterCollection at the specified parameter name.
        /// </summary>
        /// <param name="parameterName">The name of the TrafodionDBParameter to remove.</param>
        /// <exception cref="ArgumentOutOfRangeException">The specified index does not exist.</exception>
        public override void RemoveAt(string parameterName)
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(this._cmd.Connection, TraceLevel.Public, parameterName);
            }

            this._parameters.RemoveAt(this.IndexOf(parameterName));
            this._cmd.IsPrepared = false;
        }

        /// <summary>
        /// Removes the TrafodionDBParameter from the TrafodionDBParameterCollection at the specified index.
        /// </summary>
        /// <param name="index">The zero-based index of the TrafodionDBParameter object to remove.</param>
        /// <exception cref="ArgumentOutOfRangeException">The specified index does not exist.</exception>
        public override void RemoveAt(int index)
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(this._cmd.Connection, TraceLevel.Public, index);
            }

            this._parameters.RemoveAt(index);
            this._cmd.IsPrepared = false;
        }

        internal void Prepare(Descriptor[] desc)
        {
            if (TrafodionDBTrace.IsInternalEnabled)
            {
                TrafodionDBTrace.Trace(this._cmd.Connection, TraceLevel.Internal);
            }

            if (this._parameters.Count != desc.Length && this._cmd.isRWRS == false)
            {
                string msg = TrafodionDBResources.FormatMessage(TrafodionDBMessage.ParameterCountMismatch);
                TrafodionDBException.ThrowException(this._cmd.Connection, new InvalidOperationException(msg));
            }
            int idx = 0;
            for (int i = 0; i < desc.Length; i++)
            {
                if (this._cmd.isRWRS && i < 3)
                {
                    idx = 3;
                    continue;
                }
                this._parameters[i - idx].Descriptor = desc[i];
            }
        }

        /// <summary>
        /// Returns DbParameter the object with the specified name.
        /// </summary>
        /// <param name="parameterName">The name of the DbParameter in the collection.</param>
        /// <returns>The DbParameter the object with the specified name.</returns>
        protected override DbParameter GetParameter(string parameterName)
        {
            return this[parameterName];
        }

        /// <summary>
        /// Returns the DbParameter object at the specified index in the collection.
        /// </summary>
        /// <param name="index">The index of the DbParameter in the collection.</param>
        /// <returns>The DbParameter object at the specified index in the collection.</returns>
        protected override DbParameter GetParameter(int index)
        {
            return this[index];
        }

        /// <summary>
        /// Sets the DbParameter object with the specified name to a new value.
        /// </summary>
        /// <param name="parameterName">The name of the DbParameter object in the collection.</param>
        /// <param name="value">The new DbParameter value.</param>
        protected override void SetParameter(string parameterName, DbParameter value)
        {
            this[parameterName] = (TrafodionDBParameter)value;
        }

        /// <summary>
        /// Sets the DbParameter object at the specified index to a new value.
        /// </summary>
        /// <param name="index">The index where the DbParameter object is located.</param>
        /// <param name="value">The new DbParameter value.</param>
        protected override void SetParameter(int index, DbParameter value)
        {
            this[index] = (TrafodionDBParameter)value;
        }
    }
}
