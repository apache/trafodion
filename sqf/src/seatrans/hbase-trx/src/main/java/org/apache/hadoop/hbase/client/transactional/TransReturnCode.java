// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
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

package org.apache.hadoop.hbase.client.transactional;

// Return values
public enum TransReturnCode {
    RET_OK(0),
		RET_NOTX(1),
		RET_READONLY(2),
		RET_PARAMERR(3),
		RET_EXCEPTION(4),
		RET_HASCONFLICT(5),
		RET_IOEXCEPTION(6),
		RET_NOCOMMITEX(7),
		RET_LAST(7);
    private Integer value;
    
    private TransReturnCode(int value) { this.value = value; }
    private TransReturnCode(short value) { this.value = new Integer(value); }
    public short getShort() { return value.shortValue(); }
    public int getValue() { return value; }
    public String toString() {
      return super.toString();
    }
}
