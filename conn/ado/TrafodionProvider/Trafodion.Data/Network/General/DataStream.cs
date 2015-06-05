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
using System.Diagnostics;

namespace Trafodion.Data
{
    internal enum ByteOrder
    {
        LittleEndian = 1,
        BigEndian = 2
    }

    internal enum BufferType
    {
        Main = 0,
        Alt = 1,
        Fetch = 2
    }

    //.NET does not support BigEndian very well, we need to do lots of wrapping
    internal class DataStream
    {
        private const int _minBufferSize = 4096; // 4k

        private ByteOrder _bo;

        // use two swappable buffers as we may need to compress/decompress often
        private byte[][] _buffers;
        private BufferType _currentBuffer;

        private byte[] _buf;
        private int _pos;

        public DataStream(byte[] buf, ByteOrder bo)
        {
            this.ByteOrder = bo; //assume NSK

            this._buffers = new byte[3][];
            this._buffers[0] = buf;
            this._currentBuffer = BufferType.Main;

            this._buf = this._buffers[(int)BufferType.Main];
            this._pos = 0;
        }

        public DataStream(ByteOrder bo)
            :this(new byte[DataStream._minBufferSize], bo)
        {

        }

        public int Length
        {
            get
            {
                return _buf.Length;
            }
        }

        public int Position
        {
            get
            {
                return _pos;
            }
            set
            {
                if (value > Length)
                {
                    throw new Exception("out of bounds");
                }

                _pos = value;
            }
        }

        public ByteOrder ByteOrder
        {
            get
            {
                return _bo;
            }
            set
            {
                _bo = value;
            }
        }

        public byte[] Buffer
        {
            get
            {
                return this._buf;
            }
        }

        public BufferType CurrentBuffer
        {
            get
            {
                return this._currentBuffer;
            }
            set
            {
                this._currentBuffer = value;

                int offset = (int)value;
                if (this._buffers[offset] == null)
                {
                    this._buffers[offset] = new byte[DataStream._minBufferSize];
                }
                this._buf = this._buffers[offset];
            }
        }

        public byte[] GetBuffer(BufferType type)
        {
            return this._buffers[(int)type];
        }

        public void SetBuffer(BufferType type, byte[] buf)
        {
            this._buffers[(int)type] = buf;
            if (type == this.CurrentBuffer)
            {
                this._buf = buf;
            }
        }

        public void Compress(int algorithm, int sourceLen, BufferType dest)
        {
            byte [] sourceData;
            byte [] destData;

            // save source information
            sourceData = this._buf;

            // switch buffers and setup dest information
            this.CurrentBuffer = dest;
            Resize(sourceLen);
            destData = this._buf;

            switch(algorithm)
            {
                case 1:
                    this.Position = LZF.NET.CLZF.lzf_compress(sourceData, sourceLen, destData, sourceLen);
                    break;
                default:
                    break;
            }
        }

        public void Decompress(int algorithm, int sourceLen, int destLen, BufferType dest)
        {
            byte[] sourceData;
            byte[] destData;

            //Console.WriteLine("Compressed: {0} Decompressed: {1} Ratio: {2}", sourceLen, destLen, (double)sourceLen / (double)destLen);

            //Stopwatch sw = new Stopwatch();
            //sw.Start();
            // save source information
            sourceData = this._buf;

            // switch buffers and setup dest information
            this.CurrentBuffer = dest;
            Resize(destLen);
            destData = this._buf;

            switch (algorithm)
            {
                case 1:
                    this.Position = LZF.NET.CLZF.lzf_decompress(sourceData, sourceLen, destData, destLen);
                    break;
                default:
                    break;
            }

            //sw.Stop();
            //Console.WriteLine(sw.ElapsedMilliseconds);
        }

        //resize will clear data
        public void Resize(int size)
        {
            if (size > this.Length)
            {
                size += DataStream._minBufferSize;
                this._buffers[(int) this._currentBuffer] = new byte[size];
                this._buf = this._buffers[(int)this._currentBuffer];
            }
        }

        public void Reset()
        {
            this.Position = Header.Size;
        }

        //generic writes

        public void WriteBytes(byte[] b)
        {
            System.Buffer.BlockCopy(b, 0, _buf, _pos, b.Length);

            _pos += b.Length;
        }

        public void WriteChar(char c)
        {
            _buf[_pos++] = (byte)c;
        }

        public void WriteByte(byte b)
        {
            _buf[_pos++] = b;
        }

        public void WriteInt16(short s)
        {
            if (_bo == ByteOrder.LittleEndian) {
			    _buf[_pos + 1] = (byte) ((s >> 8) & 0xff);
			    _buf[_pos] = (byte) ((s) & 0xff);
		    } else {
                _buf[_pos] = (byte)((s >> 8) & 0xff);
                _buf[_pos + 1] = (byte)((s) & 0xff);
		    }

		    _pos += 2;
        }

        public void WriteUInt16(ushort s)
        {
            if (_bo == ByteOrder.LittleEndian)
            {
                _buf[_pos + 1] = (byte)((s >> 8) & 0xff);
                _buf[_pos] = (byte)((s) & 0xff);
            }
            else
            {
                _buf[_pos] = (byte)((s >> 8) & 0xff);
                _buf[_pos + 1] = (byte)((s) & 0xff);
            }

            _pos += 2;
        }

        public void WriteInt32(int i)
        {
            if (_bo == ByteOrder.LittleEndian) {
			    _buf[_pos + 3] = (byte) ((i >> 24) & 0xff);
			    _buf[_pos + 2] = (byte) ((i >> 16) & 0xff);
			    _buf[_pos + 1] = (byte) ((i >> 8) & 0xff);
			    _buf[_pos] = (byte) ((i) & 0xff);
		    } else {
			    _buf[_pos] = (byte) ((i >> 24) & 0xff);
			    _buf[_pos + 1] = (byte) ((i >> 16) & 0xff);
			    _buf[_pos + 2] = (byte) ((i >> 8) & 0xff);
			    _buf[_pos + 3] = (byte) ((i) & 0xff);
		    }

		    _pos += 4;
        }

        public void WriteUInt32(uint i)
        {
            if (_bo == ByteOrder.LittleEndian)
            {
                _buf[_pos + 3] = (byte)((i >> 24) & 0xff);
                _buf[_pos + 2] = (byte)((i >> 16) & 0xff);
                _buf[_pos + 1] = (byte)((i >> 8) & 0xff);
                _buf[_pos] = (byte)((i) & 0xff);
            }
            else
            {
                _buf[_pos] = (byte)((i >> 24) & 0xff);
                _buf[_pos + 1] = (byte)((i >> 16) & 0xff);
                _buf[_pos + 2] = (byte)((i >> 8) & 0xff);
                _buf[_pos + 3] = (byte)((i) & 0xff);
            }

            _pos += 4;
        }

        public void WriteInt64(long l)
        {
            if (_bo == ByteOrder.LittleEndian)
            {
                _buf[_pos + 7] = (byte)((l >> 56) & 0xff);
                _buf[_pos + 6] = (byte)((l >> 48) & 0xff);
                _buf[_pos + 5] = (byte)((l >> 40) & 0xff);
                _buf[_pos + 4] = (byte)((l >> 32) & 0xff);
                _buf[_pos + 3] = (byte)((l >> 24) & 0xff);
                _buf[_pos + 2] = (byte)((l >> 16) & 0xff);
                _buf[_pos + 1] = (byte)((l >> 8) & 0xff);
                _buf[_pos] = (byte)((l) & 0xff);
            }
            else
            {
                _buf[_pos] = (byte)((l >> 56) & 0xff);
                _buf[_pos + 1] = (byte)((l >> 48) & 0xff);
                _buf[_pos + 2] = (byte)((l >> 40) & 0xff);
                _buf[_pos + 3] = (byte)((l >> 32) & 0xff);
                _buf[_pos + 4] = (byte)((l >> 24) & 0xff);
                _buf[_pos + 5] = (byte)((l >> 16) & 0xff);
                _buf[_pos + 6] = (byte)((l >> 8) & 0xff);
                _buf[_pos + 7] = (byte)((l) & 0xff);
            }

            _pos += 8;
        }

        public void WriteString(byte[] b)
        {
            if (b.Length > 0)
            {
                WriteInt32(b.Length + 1);
                WriteBytes(b);
                WriteByte((byte)0);
            }
            else
            {
                WriteInt32(0);
            }
        }

        public void WriteStringWithCharset(byte [] b, int charset)
        {
            if (b.Length > 0)
            {
                WriteString(b);
                WriteInt32(charset);
            }
            else
            {
                WriteInt32(0);
            }
        }

        //usersid
        public void WriteUserSid(byte[] b)
        {
            WriteInt32(b.Length);
            WriteBytes(b);
            //no null term
        }

        // BEGIN READS

        public byte[] ReadBytes()
        {
            return this.ReadBytes(this.ReadInt32());
        }

        public byte[] ReadBytes(int count)
        {
            byte[] ret = new byte[count];
            System.Buffer.BlockCopy(_buf, _pos, ret, 0, count);
            _pos += count;

            return ret;
        }

        public char ReadChar()
        {
            return (char)_buf[_pos++];
        }

        public byte ReadByte()
        {
            return _buf[_pos++];
        }

        public short ReadInt16()
        {
            short ret;

            if (_bo == ByteOrder.LittleEndian)
            {
                ret = (short) (((_buf[_pos]) & 0x00ff) | ((_buf[_pos + 1] << 8) & 0xff00));
            }
            else
            {
                ret = (short) (((_buf[_pos + 1]) & 0x00ff) | ((_buf[_pos] << 8) & 0xff00));
            }

            _pos += 2;

            return ret;
        }

        public ushort ReadUInt16()
        {
            ushort ret;

            if (_bo == ByteOrder.LittleEndian)
            {
                ret = (ushort) (((_buf[_pos]) & 0x00ff) | ((_buf[_pos + 1] << 8) & 0xff00));
            }
            else
            {
                ret = (ushort) (((_buf[_pos + 1]) & 0x00ff) | ((_buf[_pos] << 8) & 0xff00));
            }

            _pos += 2;

            return ret;
        }

        public int ReadInt32()
        {
	        int ret;

	        if (_bo == ByteOrder.LittleEndian) {
		        ret = (int) (((_buf[_pos]) & 0x000000ffL) | ((_buf[_pos + 1] << 8) & 0x0000ff00L)
				        | ((_buf[_pos + 2] << 16) & 0x00ff0000L) | ((_buf[_pos + 3] << 24) & 0xff000000L));
	        } else {
		        ret = (int)(((_buf[_pos + 3]) & 0x000000ffL) | ((_buf[_pos + 2] << 8) & 0x0000ff00L)
				        | ((_buf[_pos + 1] << 16) & 0x00ff0000L) | ((_buf[_pos] << 24) & 0xff000000L));
	        }

	        _pos += 4;

	        return ret;
        }

        public uint ReadUInt32()
        {
            uint ret;

            if (_bo == ByteOrder.LittleEndian)
            {
                ret = (uint)(((_buf[_pos]) & 0x000000ffL) | ((_buf[_pos + 1] << 8) & 0x0000ff00L)
                        | ((_buf[_pos + 2] << 16) & 0x00ff0000L) | ((_buf[_pos + 3] << 24) & 0xff000000L));
            }
            else
            {
                ret = (uint)(((_buf[_pos + 3]) & 0x000000ffL) | ((_buf[_pos + 2] << 8) & 0x0000ff00L)
                        | ((_buf[_pos + 1] << 16) & 0x00ff0000L) | ((_buf[_pos] << 24) & 0xff000000L));
            }

            _pos += 4;

            return ret;
        }

        public long ReadInt64()
        {
            long ret = 0;

            //doing bitwise operations with max value longs is messy, this is easier
            if (_bo == ByteOrder.LittleEndian)
            {
                for (int shift = 0; shift < 64; shift += 8)
                {
                    ret |= ((long)(_buf[_pos++] & 0xff)) << shift;
                }
            }
            else
            {
                for (int shift = 56; shift >= 0; shift -= 8)
                {
                    ret |= ((long)(_buf[_pos++] & 0xff)) << shift;
                }
            }

            return ret;
        }

        public byte [] ReadString()
        {
            int len = ReadInt32();
            byte[] buf;

            if (len > 0)
            {
                len--;
                buf = ReadBytes(len);
                ReadByte(); //null
            }
            else
            {
                buf = new byte[0];
            }

            return buf;
        }

        public byte[] ReadStringBytes()
        {
            int len = ReadInt32();
            byte [] ret = ReadBytes(len);
            ReadByte();

            return ret;
        }
    }
}
