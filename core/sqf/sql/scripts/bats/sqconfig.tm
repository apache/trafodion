# @@@ START COPYRIGHT @@@
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
# @@@ END COPYRIGHT @@@

begin node
_virtualnodes 2
end node

# a -1 means all the nodes for that role
begin role_node_map
storage -1
end role_node_map

# The following section must be specified when doing fault tolerant
# testing across multiple racks.  If it isn't specified, some backup
# ASE/TSE processes will be created on a different rack from the
# primary, which doesn't work because storage isn't visible across
# racks.  The list of hostnames can either be all hostnames for the
# nodes in each enclosure, or only the list of storage nodes in use
# for this configuration.

#begin enclosure
#enc-1=n003,n004,n005,n006,n007,n008,n009,n010,n011,n012,n013,n014
#enc-2=n015,n016,n017,n018,n019,n020,n021,n022,n023,n024,n025,n026
#end enclosure

storage_loc $TRAF_HOME/sql/database

# Please note that if you have mirroring turned ON, you must specify
# the mirror storage locations.  You can either specify a global
# mirror location by uncommenting and changing the line below, or
# you can configure mirror locations on a per volume basis.

#storage_mir_loc $TRAF_HOME/sql/databasem

# Extents are for data audit volumes only, not $TLOG
# Minimum is 1550
begin at_extent_size
1550
end at_extent_size

# Files per volume are for data audit volumes only, not $TLOG
# Minimum is 10
begin at_files_per_volume
10
end at_files_per_volume

# Percentage of audit allowed before TX is DOOMED
# Maximum is 40
begin tx_capacity_threshold
40
end tx_capacity_threshold

# Number of unuique Backout processes per TSE
# Maximum is 3
begin backouts_per_tse
1
end backouts_per_tse

begin tmase
$TLOG
end tmase

begin ase
$AUDIT1
end ase

# volumeName [node id] [storage location] [mir storage location]
begin tse
$SYSTEM
$DATA
$DATA1
$DATA2
$DATA3
$DATA4
end tse

# NDCS association server
# port [port range] [edgenode 1] [edgenode 2]
# If the block is present with no entries only the single default port (18650)
# is configured.
# begin ndcs
# end ndcs
