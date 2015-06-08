/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2010-2015 Hewlett-Packard Development Company, L.P.
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

namespace Trafodion.Data.ETL
{
    internal class MxHash
    {
        private static readonly uint[] hashVaues = { 0x905ebe29, 0x95ff0b84, 0xe5357ed6, 0x2cffae90, 0x8350b3f1, 0x1748a7eb, 0x2a0695db, 0x1e7ca00c, 0x60f80c24, 0x9a41fe1c, 0xa985a647,
			    0x0ed7e512, 0xcd34ef43, 0xe06325a6, 0xecbf735a, 0x76540d38, 0x35cba55d, 0xff539efc, 0x64545d45, 0xd7112c0d, 0x17e09e1c, 0x02359d32, 0x45976350, 0xd630a578, 0x34cd0c12,
			    0x754546f6, 0x1bf4f249, 0xbc65c34f, 0x5c932f44, 0x6cb0d8d0, 0xfd0e7030, 0x2b160e3b, 0x101daff6, 0x25bbcb9d, 0xe7eca21f, 0x6d3b24ca, 0xaef7e6b9, 0xd212f049, 0x2de2817e,
			    0x2792bcd5, 0x67f794b2, 0xaec6f7cc, 0x79a3e367, 0xd5a85114, 0xa98ecc2d, 0xf373e266, 0x58ae2757, 0xd8faa0ff, 0x45e7eb61, 0xbd72ba1e, 0xc28f6b16, 0x804bc2e6, 0xfed74984,
			    0x881cd177, 0xa02647e8, 0xd799d053, 0xbe143d12, 0x49177474, 0xbbc0c5f4, 0x99f7fe9f, 0x24fc1559, 0xce0925cf, 0x1dded5f4, 0x1d1a2cd3, 0xafe3ef48, 0x6fd5d075, 0x4a63bc1d,
			    0x93aa36c0, 0x2942d778, 0xb26a2444, 0x5616cc50, 0x7565c161, 0xa006197b, 0xee700b07, 0x4a236a82, 0x693db870, 0x9a919e64, 0x995b05b1, 0xd4659569, 0x90e45846, 0xbca11996,
			    0x3e345cd9, 0xb29a9967, 0x7e9e66f7, 0x9ce136d0, 0xcde74e76, 0xde56e4bb, 0xba4dc6ae, 0xf9d40779, 0x4e5c0bdb, 0xde14f9e5, 0x278f8745, 0x13ce0128, 0x8bb308f5, 0x4c41a359,
			    0x273d1927, 0x50338e76, 0xdfceb7c2, 0xf1b86f68, 0xc8b12d6a, 0xf4cb0e08, 0xa74b4b14, 0x81571c6a, 0xebc4a928, 0x1d6d5fd6, 0x7f4bbc87, 0x61ba542f, 0x9b06d11d, 0xb53ae1c1,
			    0xdcc2a6c0, 0x7f04f8a8, 0x8da9d186, 0xa168e054, 0x21ed0ce7, 0x9ca9e9d1, 0x0e01fb38, 0xd8b6b1d9, 0xb8d10266, 0x203a9de1, 0x37ba3ffe, 0x9fefb09f, 0x5e4cb3e2, 0xcecd03b4,
			    0xcc270838, 0xa1619089, 0x22995679, 0x6dcd6b78, 0x8c50f9b1, 0x1c354ada, 0x48a0f13e, 0xca7b4696, 0x5c1fe8bf, 0xdd0f433f, 0x8aa411f1, 0x149b2ee3, 0x181d16a1, 0x3b84b01d,
			    0xee745103, 0x0f230907, 0x663d1014, 0xd614181b, 0xb1b88cc9, 0x015f672c, 0x660ea636, 0x4107c7f3, 0x6f0d8afe, 0xf0aeffeb, 0x93b25fa0, 0x620c9075, 0x155a4d7e, 0x10fdbd73,
			    0xb162eabe, 0xaf9605db, 0xba35d441, 0xde327cfa, 0x15a6fd70, 0x0f2f4b54, 0xfb1b4995, 0xec092e68, 0x37ebade6, 0x850f63ca, 0xe72a879f, 0xc823f741, 0xc6f114b8, 0x74e461f6,
			    0x1d01ad14, 0xfe1ed7d3, 0x306b9444, 0x9ebd40a6, 0x3275b333, 0xa8540ca1, 0xeb8d394c, 0xa2aef54c, 0xf12d0705, 0x8974e70e, 0x59ae82cf, 0x32469aca, 0x973325d8, 0x27ba604d,
			    0x9aeb7827, 0xaf0af97c, 0x9783e6f8, 0xe0725a87, 0x2f02d864, 0x717a0587, 0x0c90d7b0, 0x6828b84e, 0xba08ebe7, 0x65cf8360, 0x63132f80, 0xbb8d4a41, 0xbd5b8b41, 0x459f019f,
			    0x5e68369f, 0xe855f000, 0xa79a634c, 0x172c7704, 0x07337ab3, 0xb2926453, 0x11084c8a, 0x328689ca, 0xa7e3efcf, 0x8b9a5695, 0x76b65bbe, 0x87bb5a2a, 0x5f73e6ad, 0xcf59b265,
			    0x4fe46ec9, 0x52561232, 0x70db002c, 0xc21d1b8f, 0xd7ceb1c6, 0xff4a97c8, 0xdd21c90b, 0x48c14c38, 0x64262c68, 0x74c5d3f9, 0x66bf60e7, 0xce804348, 0x98585792, 0x7619fc86,
			    0x91de3f72, 0x57f5191c, 0x576d9737, 0x5f4535b0, 0xb9ee8ef5, 0x2e9eff6c, 0xc7c9f874, 0xe6ac0843, 0xd93b8c08, 0x2f34a779, 0x407799eb, 0x2b9904e0, 0x14bb018f, 0x1fcf367b,
			    0x7975c362, 0xba31448f, 0xa59286f7, 0x1255244a, 0xd685169b, 0xc791ec84, 0x3b5461b1, 0x4822924a, 0x26d86175, 0x596e6b2f, 0x6a157bef, 0x8bc98a9b, 0xa8220343, 0x91eaad8a,
			    0x42b89a9e, 0x7c9b5f81, 0xb5f9ec6c, 0xd999ef9e, 0xa547f6a3, 0xc391f010, 0xe9d8bb43 };

        private const int hashBits = 32;
        private const uint nullValue = 0x27BC582D;

        public static uint Hash(byte[][] vals, uint numParts)
        {
            int i = vals.Length;
		    uint hashVal = HashValue(vals[0]);

            for (i--; i >= 0; i--)
            {
                hashVal = ((hashVal << 1) | (hashVal >> 31)) ^ HashValue(vals[i]);
            }

		    return (uint) ((hashVal * numParts) >> hashBits);
	    }

	    private static uint HashValue(byte[] data) {
		    uint hashValue;
            short v;

            if (data != null)
            {
                hashValue = 0;

                for (int i = 0; i < data.Length; i++)
                {
                    v = data[i];
                    if (v < 0)
                    {
                        v = (short)(256 + v);
                    }
                    hashValue = ((hashValue << 1) | (hashValue >> 31)) ^ hashVaues[v];
                }
            }
            else
            {
                hashValue = nullValue;
            }

		    return hashValue;
	    }
    }
}

