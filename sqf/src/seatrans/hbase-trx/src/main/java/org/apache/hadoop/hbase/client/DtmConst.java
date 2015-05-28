// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.
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


package org.apache.hadoop.hbase.client;

import java.nio.charset.Charset;



public class DtmConst {

  public static final Charset META_CHARSET = Charset.forName("UTF-8");

  public static final byte[] TRANSACTION_META_FAMILY = "mt_"
      .getBytes(META_CHARSET);     // it is likely that user table also name CF as mt_, but this is not the case for Trafodion, but for user hbase table, something need to revisit in the future

  public static final int MVCC_MAX_VERSION = 1;
  public static final int SSCC_MAX_VERSION = 3;
  public static final int MVCC_MAX_DATA_VERSION = MVCC_MAX_VERSION * 2;
  public static final int SSCC_MAX_DATA_VERSION = SSCC_MAX_VERSION * 2;

}
