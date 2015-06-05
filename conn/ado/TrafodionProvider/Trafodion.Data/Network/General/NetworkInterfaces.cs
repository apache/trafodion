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

using System;

namespace Trafodion.Data
{
    /// <summary>
    /// Common interface for all structures marshalled to the server.
    /// </summary>
    internal interface INetworkMessage
    {
        /// <summary>
        /// Writes structured data into the <code>DataStream</code>
        /// </summary>
        /// <param name="ds"></param>
        void WriteToDataStream(DataStream ds);

        /// <summary>
        /// converts string params to bytes and calculates the total size of the message
        /// </summary>
        /// <returns>the total size for the message</returns>
        int PrepareMessageParams(TrafodionDBEncoder enc);
    }

    /// <summary>
    /// Common interface for all structures marshalled from the server.
    /// </summary>
    internal interface INetworkReply
    {
        /// <summary>
        /// Reads structured data out of the DataStream
        /// </summary>
        /// <param name="ds">The DataStream object.</param>
        /// <param name="enc">The TrafodionDBEncoder object.</param>
        void ReadFromDataStream(DataStream ds, TrafodionDBEncoder enc);
    }
}
