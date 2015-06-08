//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2007-2015 Hewlett-Packard Development Company, L.P.
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
//

using System;
using System.Collections.Generic;
using System.Text;
using System.Globalization;

namespace Trafodion.Manager.Framework
{
    public class DisplayFormatter
    {
        public static String FormatPercent(double aPercent)
        {
            return aPercent.ToString("F0", CultureInfo.InvariantCulture) + " %";
        }


        public static String FormatSize(long aSize)
        {    	
	        // Just make zero be 0 without units
	        if (aSize == 0)
	        {
	            return "0";
	        }
        	
	        // Make one byte be singluar
	        if (aSize == 1)
	        {
	            return "1 Byte";
	        }
        	
	        double theSize = aSize;                
            const long kilobyte = 1024;                
	        double[] theSizes =
	        { 
                kilobyte * kilobyte * kilobyte * kilobyte,
                kilobyte * kilobyte * kilobyte,
                kilobyte * kilobyte,
                kilobyte,
                0 
            };

            String[] theSizeUnits = 
            { 
                "TB",
                "GB", 
                "MB", 
                "KB", 
                "Bytes" 
            };
        	
            for (int theIndex = 0; ; theIndex++)
            {
                double theLimit = theSizes[theIndex];
                if (theSize >= theLimit)
                {
                    if (theLimit > 0)
                    {
                        theSize /= theLimit;
                    }
                    return String.Format("{0:#,#.## " + theSizeUnits[theIndex] + "}", theSize); 
                }
            }
        }
    }
}
