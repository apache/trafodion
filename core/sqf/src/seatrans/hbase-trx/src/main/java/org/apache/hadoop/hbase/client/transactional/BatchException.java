// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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

public enum BatchException {
    // No Exception, region processed
    EXCEPTION_OK,
    // Unable to find region in batch call
    EXCEPTION_REGIONNOTFOUND_ERR,
    // Optimization, no need to process remaining regions, positive case
    EXCEPTION_SKIPREMAININGREGIONS_OK,
    // Problem with first region, no need to process remaining regions, failure case
    EXCEPTION_SKIPREMAININGREGIONS_ERR,
    // Hit an exception, do not retry individual region call, fail
    EXCEPTION_NORETRY_ERR,
    // Hit an exception, do retry individual region call
    EXCEPTION_RETRY_ERR,
    // Unknown transaction exception
    EXCEPTION_UNKNOWNTRX_ERR
}
