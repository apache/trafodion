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
    /// Represents a header type.  The only one actually required on the client is WriteRequestFirst.
    /// </summary>
    internal enum HeaderType: int
    {
        WriteRequestFirst = 1,
	    WriteRequestNext = 2,
	    ReadResponseFirst = 3,
	    ReadResponseNext = 4
    }

    /// <summary>
    /// Specifies if bytes need to be swapped.
    /// </summary>
    internal enum SwapType: byte {
        Yes = (byte)'Y',
        No = (byte)'N'
    }

    internal enum HeaderVersion : int
    {
        Old = 100,
        ClientBigEndian = 101,
        ClientLittleEndian = 102,
        ServerBigEndian = 201,
        ServerLittleEndian = 202
    }

    /// <summary>
    /// This is the header structure which all network messages contain.
    /// </summary>
    internal class Header : INetworkReply, INetworkMessage
    {
        public const int Size = 40;

        public short operationId;
        public int dialogueId;
        public int totalLength;
        public int compressLength;
        public char compressInd;
        public byte compressType;
        public HeaderType headerType;
        public int signature;
        public HeaderVersion version;
        public char platform;
        public char transport;
        public SwapType swap;
        public short error;
        public short errorDetail;

        //must set operationId, dialogueID, totalLength externally
        public Header()
        {
            compressLength = 0;
            compressInd = 'N';
            compressType = 0;
            signature = 12345;
            version = HeaderVersion.ClientBigEndian;
            platform = 'P';
            transport = 'T';
            swap = SwapType.No;
            error = 0;
            errorDetail = 0;
        }

        public void WriteToDataStream(DataStream ds)
        {
            ds.WriteInt16(operationId);
            ds.WriteInt16((short)0); // + 2 filler
            ds.WriteInt32(dialogueId);
            ds.WriteInt32(totalLength);
            ds.WriteInt32(compressLength);
            ds.WriteChar(compressInd);

            ds.WriteByte(compressType);
            ds.WriteInt16((short)0); // + 2 filler
            ds.WriteInt32((int)headerType);
            ds.WriteInt32(signature);
            ds.WriteInt32((int)version);
            ds.WriteChar(platform);
            ds.WriteChar(transport);
            ds.WriteChar((char)swap);
            ds.WriteByte((byte)0); // + 2 filler
            ds.WriteInt16(error);
            ds.WriteInt16(errorDetail);
        }

        public int PrepareMessageParams(TrafodionDBEncoder enc)
        {
            return 40;
        }

        public void ReadFromDataStream(DataStream ds, TrafodionDBEncoder enc)
        {
            operationId = ds.ReadInt16();
            ds.ReadInt16(); // + 2 filler
            dialogueId = ds.ReadInt32();
            totalLength = ds.ReadInt32();
            compressLength = ds.ReadInt32();
            compressInd = ds.ReadChar();

            compressType = ds.ReadByte();
            ds.ReadInt16(); // + 2 filler
            headerType = (HeaderType) ds.ReadInt32();
            signature = ds.ReadInt32();
            version = (HeaderVersion)ds.ReadInt32();
            platform = ds.ReadChar();
            transport = ds.ReadChar();
            swap = (SwapType) ds.ReadChar();
            ds.ReadByte(); // + 2 filler
            error = ds.ReadInt16();
            errorDetail = ds.ReadInt16();
        }
    }
}
