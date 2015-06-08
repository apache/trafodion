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
    /// Option flags which indicate server settings or additional information contained in the message
    /// </summary>
    [Flags]
    internal enum OutContextOptions1 : uint
    {
	    IgnoreCancel = 1073741824, // (2^30)
	    ExtraOptions = 2147483648, // (2^31)
	    DownloadCertificate = 536870912 //(2^29)
    }

    /// <summary>
    /// A set of connection properties returned by the server after login.
    /// </summary>
    internal class OutConnectionContext: INetworkReply
    {
        public Version [] serverVersion;
        public short nodeId;
        public uint processId;

        public string computerName;
        public string catalog;
        public string schema;

        public OutContextOptions1 optionFlags1;
        public OutContextOptions1 optionFlags2;

        public String roleName;
        public Boolean ignoreCancel;

        public byte[] certificate;

        public void ReadFromDataStream(DataStream ds, TrafodionDBEncoder enc)
        {
            serverVersion = Version.ReadListFromDataStream(ds, enc);
            nodeId = ds.ReadInt16();
            processId = ds.ReadUInt32();

            computerName = enc.GetString(ds.ReadString(), enc.Transport);
            catalog = enc.GetString(ds.ReadString(), enc.Transport);
            schema = enc.GetString(ds.ReadString(), enc.Transport);

            optionFlags1 = (OutContextOptions1) ds.ReadUInt32();
            optionFlags2 = (OutContextOptions1) ds.ReadUInt32();

            ignoreCancel = (optionFlags1 & OutContextOptions1.IgnoreCancel) > 0;
            if ((optionFlags1 & OutContextOptions1.DownloadCertificate) > 0)
            {
                int len = ds.ReadInt32();
                certificate = ds.ReadBytes(len);
            }
            else if ((optionFlags1 & OutContextOptions1.ExtraOptions) > 0)
            {
                DecodeExtraOptions(enc.GetString(ds.ReadString(), enc.Transport));
            }
        }

        /// <summary>
        /// Helper method to extract extra options which are in string form.
        /// </summary>
        /// <param name="options"></param>
        private void DecodeExtraOptions(String options)
        {
            String[] opts = options.Split(new char[] { ';' });
            String token;
            String value;
            int index;

            for (int i = 0; i < opts.Length; i++)
            {
                opts[i] = opts[i].Trim();

                if (opts[i].Length > 0)
                {
                    index = opts[i].IndexOf('=');
                    token = opts[i].Substring(0, index).ToUpper();
                    value = opts[i].Substring(index + 1);

                    if (token.Equals("RN"))
                    {
                        this.roleName = value;
                    }
                }
            }
        }
    }
}
