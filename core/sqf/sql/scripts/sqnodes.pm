#
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
#

use strict;
use Exporter ();

# Uncomment if using the RecDescent Parser
# The Parse::RecDescent code is open source code licensed under GNU
# At this time Apache does not support GNU licenses so the code that
# uses RecDescent is commented out.  If a decision is made to use this
# code, then the licensing issues needs to be resolved.
# use Parse::RecDescent;

use sqconfigdb;

package sqnodes;

# set debugFlag to 1 to print detailed debugging info
my $debugFlag = 0;
# set g_sqParseDebugFlag to 1 to print detailed debugging info during parsing
$::g_sqParseDebugFlag = 0;


# variables used in grammar "actions" must be global so they are also
# visible to this module.
$::g_sqStmtType;
$::g_sqNodeId;
$::g_sqNodeName;
@::g_sqCores;
@::g_sqRoles;
$::g_sqProcessors;
@::g_sqSpareList;
@::g_sqExclCoreList;

$::g_sqEncName;
@::g_sqNodeList;

my $errors = 0;
my $stmt;
my $nodeName;
my @nodeData;
my %nodeNameToIds;
my %spares;
my %excluded;
my $lastSqlNode = -1;
my %EnclosureToNodeName;

my $grammar = <<'EOGRAMMAR';

statement: (stmt_type2 ';')(s) stmt_type2
              {$::g_sqStmtType = 2; }
         | (stmt_type3 ';')(?) stmt_type3
              {$::g_sqStmtType = 3; }
         | (stmt_type1 ';')(s) stmt_type1
              {$::g_sqStmtType = 1; }
         | stmt_type4
              {$::g_sqStmtType = 4; }
         | <error: Unknown statement $text. >

stmt_type1: attr_node_id
          | attr_node_name
          | attr_cores
          | attr_processors
          | attr_roles

stmt_type2: attr_node_name
          | attr_cores
          | attr_spares

stmt_type3: attr_node_name
          | attr_excl_cores

stmt_type4: enc_name '=' enc_node(s)
    {$::g_sqEncName = $item[1]; }

enc_nodes: enc_node enc_nodes | enc_node
enc_node: node_name ','
{ push(@::g_sqNodeList, $item[1]); }
        | node_name
{ push(@::g_sqNodeList, $item[1]); }

attr_node_id: 'node-id' '=' /\d+/
    {$::g_sqNodeId = $item[3]; }

attr_node_name: 'node-name' '=' node_name
   {$::g_sqNodeName = $item[3]; }

node_name: /\w[a-z0-9.\-]*\w+/
enc_name: /\w+\-*\w+/

attr_cores: 'cores' '=' core_list
   { # Place each core into the "cores" array
     my $cr = $item[3];
     @::g_sqCores = @$cr;
     if ($::g_sqParseDebugFlag)
     {
        print "cores=@::g_sqCores\n";
     };
     1;
   }

core_list: (/\d+/ '-' {$item[1]})(?) /\d+/
   { # Place each core into the "coreList" array
     my $cl = $item[1];
     my @coreList = @$cl;
     $coreList[$#coreList+1] = $item[2];
     if ($::g_sqParseDebugFlag)
     {
        my $numCores = @coreList + 0;
        print "numCores=$numCores coreList=@coreList\n";
     }
     $return=\@coreList;
   }

attr_processors: 'processors' '=' /\d+/
   {  $::g_sqProcessors = $item[3]; }

attr_roles: 'roles' '=' (attr_role ',' {$item[1]} )(s?) attr_role
   { # Place each role into the "roles" array
     my $rl = $item[3];
     @::g_sqRoles = @$rl;
     $::g_sqRoles[$#::g_sqRoles+1] = $item[4];
     if ($::g_sqParseDebugFlag)
     {
        my $numRoles = @::g_sqRoles + 0;
        print "numRoles=$numRoles roles=@::g_sqRoles\n";
     }
     1;
   }

attr_role: 'aggregation'
         | 'storage'
         | 'operation'
         | 'maintenance'
         | 'loader'
         | 'connection'
         | <error>

attr_spares: 'spares' '=' spare_list

spare_list: (/\d+/ ',' {$item[1]} )(s?) /\d+/
   { # Place each role into the "spareList" array
     my $nl = $item[1];
     @::g_sqSpareList = @$nl;
     $::g_sqSpareList[$#::g_sqSpareList+1] = $item[2];
     if ($::g_sqParseDebugFlag)
     {
        my $numSpareList = @::g_sqSpareList + 0;
        print "numSpareList=$numSpareList  spareList=@::g_sqSpareList\n";
     }
     1;
   }

attr_excl_cores: 'excluded-cores' '=' core_list
    { # Place each excluded core into the "exclCoreList" array
      my $xc = $item[3];
      @::g_sqExclCoreList = @$xc;
      if ($::g_sqParseDebugFlag)
      {
         print "excluding cores @::g_sqExclCoreList\n";
      }
      1;
    }

EOGRAMMAR

sub resetVars
{ $::g_sqStmtType = 0;
  $::g_sqNodeName = "";
  $::g_sqNodeId = -1;
  @::g_sqCores = ();
  @::g_sqRoles = ();
  $::g_sqProcessors = -1;
  @::g_sqSpareList = ();
  @::g_sqExclCoreList = ();
  $::g_sqEncName = "";
  @::g_sqNodeList = ();
}

# Display node configuration statement if not already displayed.
sub displayStmt
{
    $errors++;
    if ($_[0] == 1)
    {
        print "For \"$stmt\":\n";
        # Set flag that statement has been displayed
        $_[0] = 0;
    }
}

sub verifyCoreList
{
    my $numArgs = @_ + 0;

    if ($numArgs == 2)
    {   # Only one core number specified, make sure it is in valid range
        if ($_[1] > 255)
        {
            displayStmt($_[0]);
            print "   Error: core number must be number must be in the range 0 .. 15.\n";
        }
    }
    else
    {   # A range of core numbers was specified
        if ($_[1] > 14)
        {
            displayStmt($_[0]);
            print "   Error: first core number must be number must be in the range 0 .. 14.\n";
        }
        if ($_[2] == 0 || $_[2] > 255)
        {
            displayStmt($_[0]);
            print "   Error: second core number must be number must be in the range 1 .. 15.\n";
        }
        if ($_[1] >= $_[2])
        {
            displayStmt($_[0]);
            print "   Error: second core number must be number must be in greater than first core number.\n";
        }
    }
}

sub verifyParse
{
    my $stmtType = $::g_sqStmtType;
    my $nodeId = $::g_sqNodeId;
    my $nodeName = $::g_sqNodeName;
    my @cores = @::g_sqCores;
    my @roles = @::g_sqRoles;
    my $processors = $::g_sqProcessors;
    my @spareList = @::g_sqSpareList;
    my @exclCoreList = @::g_sqExclCoreList;
    my $encname = $::g_sqEncName;
    my @nodeList = @::g_sqNodeList;

    my $stmtOk = 1;

    if ($stmtType == 1)
    {
        if ($debugFlag)
        {
            print "verifyParse: stmtType=$stmtType, nodeName=$nodeName, nodeId=$nodeId, cores=@cores, processors=$processors, roles=@roles\n";
        }

        if ($processors == -1)
        {
            displayStmt($stmtOk);
            print "   Error: processors not specified.\n";
        }
        elsif ($processors == 0)
        {
            displayStmt($stmtOk);
            print "   Error: Zero is not a valid value for processors.\n";
            $processors = -1;
        }
        if ($nodeId == -1)
        {
            displayStmt($stmtOk);
            print "   Error: node-id not specified\n";
        }
        elsif ($nodeId > 1535)
        {
            displayStmt($stmtOk);
            print "   Error: node-id must be in the range 0..1535.\n";
        }
        if (@cores == 0)
        {
            displayStmt($stmtOk);
            print "   Error: core-list not specified\n";
        }
        else
        {
            verifyCoreList($stmtOk, @cores);

            if ($processors != -1)
            {   # Make sure number of items in core list is evenly divisible
                # by the number of processors.
                my $numCores = (@cores == 1) ? 1 :(($cores[1]+1) - $cores[0]);
                my $result = $numCores % $processors;
                if ($result != 0)
                {
                    displayStmt($stmtOk);
                    print "   Error: Number of core-list elements is not evenly divisible by the number of processors.\n";
                }
            }
        }
        if (@roles == 0)
        {
            displayStmt($stmtOk);
            print "   Error: roles not specified\n";
        }
        if ($stmtOk == 1)
        {
            if (!exists $nodeData[$nodeId])
            {
                # Store this node configuration
                $nodeData[$nodeId] = [ $nodeName, $processors,
                                       [@cores], [@roles], -1];

                if (exists $nodeNameToIds{$nodeName})
                {
                    # add the $nodeId to the list
                    my $nnRef = $nodeNameToIds{$nodeName};
                    push @$nnRef, $nodeId;
 
                }
                else
                {   # Create a new mapping from $nodeName to $nodeId
                    $nodeNameToIds{$nodeName} = [ $nodeId ];
                }
            }
            else
            {
                displayStmt($stmtOk);
                print "   Error: there is already a configuration line for node-id $nodeId.\n";
            }
        }
    }

    elsif ($stmtType == 2)
    {
        if ($debugFlag)
        {
            print "verifyParse: stmtType=$stmtType, nodeName=$nodeName, cores=@cores, spares=@spareList\n";
        }
        if ($nodeName eq "")
        {
            displayStmt($stmtOk);
            print "   Error: node-name not specified\n";
        }
        if (@spareList == 0)
        {
            displayStmt($stmtOk);
            print "   Error: spares not specified\n";
        }
        if (@cores == 0)
        {
            displayStmt($stmtOk);
            print "   Error: core-list not specified\n";
        }
        else
        {
            verifyCoreList($stmtOk, @cores);
        }
        if ($stmtOk == 1)
        {
            $spares{$nodeName} = [ [@spareList], [@cores] ];
        }
    }

    elsif ($stmtType == 3)
    {
        if ($debugFlag)
        {
            print "verifyParse: stmtType=$stmtType, nodeName=$nodeName, excluded-cores=@exclCoreList\n";
        }
        if ($nodeName eq "")
        {
            displayStmt($stmtOk);
            print "   Error: node-name not specified\n";
        }
        if (@exclCoreList == 0)
        {
            displayStmt($stmtOk);
            print "   Error: excluded-cores not specified\n";
        }
        verifyCoreList($stmtOk, @exclCoreList);
        if ($stmtOk == 1)
        {
            $excluded{$nodeName} = [ @exclCoreList ];
        }
    }

    elsif ($stmtType = 4)
    {
        if ($debugFlag)
        {
            print "verifyParse: stmtType=$stmtType, rack-name=$encname, node-list=@nodeList\n";
        }
        if ($encname eq "")
        {
            displayStmt($stmtOk);
            print "   Error: Enclosure name not specified\n";
        }

        if (scalar(@nodeList) == 0)
        {
            displayStmt($stmtOk);
            print "   Error: Enclosure node list not specified\n";
        }

        $EnclosureToNodeName{$encname} = \@nodeList;
    }

    else
    {
        displayStmt($stmtOk);
        print "   Error: not a valid node configuration statement.\n";
    }

};

# Uncomment if using the RecDescent Parser
#my $parse = new Parse::RecDescent ($grammar) || die "Bad grammar! No biscuit!\n";

sub parseStmt
{

# Set RD_TRACE to get detail trace of parsing steps.
#    $::RD_TRACE = 1;

    chomp($_);
    $stmt = $_;
    resetVars();
    my @KeyValues=split(';',$stmt);

    # Assuming the statement type to be 1, i.e. containing
    # node_id, node_name, cores, processors, roles
    $::g_sqStmtType = 1;

    for (my $i = 0; $i <= $#KeyValues; $i++) {

        my $KeyValue=$KeyValues[$i];
        if ($::g_sqParseDebugFlag) {
            print "KeyValues[$i]=", $KeyValue, "\n";
        }
        my @KeyValuePair = split('=', $KeyValue);
        my $Key = $KeyValuePair[0];
        my $Value = $KeyValuePair[1];
        if ($::g_sqParseDebugFlag) {
            print "Key: ", $Key, " Value: ", $Value, "\n";
        }
        
        if ($Key eq 'node-id') {
            $::g_sqNodeId = $Value;
        }
        elsif ($Key eq 'node-name') {
            $::g_sqNodeName = $Value;
        }
        elsif ($Key eq 'cores') {
            @::g_sqCores = split('-', $Value);
            if ($::g_sqParseDebugFlag) {
                print "cores=@::g_sqCores\n";
            };
        }
        elsif ($Key eq 'processors') {
            $::g_sqProcessors = $Value;
        }
        elsif ($Key eq 'roles') {
            @::g_sqRoles = split(',', $Value);
            if ($::g_sqParseDebugFlag) {
                my $numRoles = @::g_sqRoles + 0;
                print "numRoles=$numRoles roles=@::g_sqRoles\n";
            }
        }
    }

    sqnodes::verifyParse();
    
    if ($errors != 0) { # Had errors
        return 1;
    }
}

sub parseStmtRecursiveDescent
{
# Not using the RecDescent Parser, so return an error 
    return 1;
# Set RD_TRACE to get detail trace of parsing steps.
#    $::RD_TRACE = 1;

    $stmt = $_;
    resetVars();
# Uncomment if using the RecDescent Parser
#    $parse->statement($stmt);
    sqnodes::verifyParse();
    
    if ($errors != 0) { # Had errors
        return 1;
    }
}


sub validateConfig
{

    if ($errors != 0)
    { # Had errors during parseStmt so do not proceed
        return 1;
    }

    if ($debugFlag)
    {
        my $nodeDataSize = @nodeData + 0;
        print "Logical node data ($nodeDataSize nodes):\n";
    }

    my $foundNonSqlNode = 0;
    my $row;
    my $raRef;
    my $clRef;
    my $firstCore;
    my $lastCore;
    my $numCores;
    my $coreRatio;
    my $coreRatio2;
    my $numProcessors;
    my $slRef;
    my $numSpareIds;
    my @roles;

    for my $i ( 0 .. $#nodeData )
    {
        if (exists $nodeData[$i])
        {
            $row = $nodeData[$i];

            # Get reference to roles array
            $raRef = $row->[3];
            @roles = @$raRef;

            if ($debugFlag)
            {
                my $clRef = $row->[2];
                print "   node-id $i: node-name=$row->[0] processors=$row->[1] cores=@$clRef roles=@$raRef\n";
            }

            # Examine roles array looking for storage role
            my $sqlNode = 0;
            for my $j ( 0 .. $#roles )
            {
                if ($roles[$j] eq "storage")
                {   # This node-id has the storage role
                    $sqlNode = 1;
                    last;
                }
            }
            if ($sqlNode == 1)
            {
                if ($foundNonSqlNode == 0)
                {   # Remember highest numbered node-id with storage role
                    $lastSqlNode = $i;
                }
                else
                {
                    my $num = $lastSqlNode + 1;
                    print "Error: node-id $i has storage node role but node-id $num is not a storage node.  Storage nodes must be contiguous starting at node-id 0.\n";
                    $errors++;
                }
            }
            else
            {   # Remember that a non-storage node has been encountered.
                $foundNonSqlNode = 1;
            }
        }
        else
        {
            print "Error: There is no node configuration for node-id $i\n";
            $errors++;
        }
    }

    #
    # Verify the core to processor ratio:
    #    Rule: All logical nodes in the physical node must have the same
    #    ratio of cores in all processors.
    #
    # Verify spares list for a physical node:
    #    Rule: The list of logical nodes in a spares list must contain all or
    #    none of the logical nodes in a physical node.
    #
    foreach my $key (keys %nodeNameToIds)
    {
        my $nnRef = $nodeNameToIds{$key};
        my $itemCount = @$nnRef + 0;

        if ($debugFlag)
        {
            print "Physical node $key contains logical nodes: @$nnRef\n";
        }

        if ($itemCount > 1)
        {
            # Compute the ratio of cores to processors for the first
            # node-id associated with this node name.
            $row = $nodeData[$nnRef->[0]];
            $numProcessors = $row->[1];
            $clRef = $row->[2];
            $firstCore = $clRef->[0];
            if (@$clRef == 1)
            {
                $lastCore = $firstCore;
            }
            else
            {
                $lastCore = $clRef->[1];
            }
            # Compute the ratio of cores to processors
            $numCores = ($lastCore + 1) - $firstCore;
            $coreRatio = $numCores / $numProcessors;

            # Compute the ratio of cores to processors for each addtional
            # node-id associated with this node name and verify that they
            # are all the same.
            for (my $i = 1; $i < $itemCount; $i++)
            {
                $row = $nodeData[$nnRef->[$i]];
                $numProcessors = $row->[1];
                $clRef = $row->[2];
                $firstCore = $clRef->[0];
                if (@$clRef == 1)
                {
                    $lastCore = $firstCore;
                }
                else
                {
                    $lastCore = $clRef->[1];
                }
                $numCores = ($lastCore + 1) - $firstCore;
                $coreRatio2 = $numCores / $numProcessors;
                if ($coreRatio != $coreRatio2)
                {
                    print "Error: for node-name $key, node-id $nnRef->[0] and node-id $nnRef->[$i] do not have the same core to processor ratio.\n";
                    $errors++;
                }
            }

            # For each row in the spares array:
            #   If one of the node-ids is present, they all must be present
            foreach my $spareKey (keys %spares)
            {
                $row = $spares{$spareKey};
                $slRef = $row->[0];

                $numSpareIds = @$slRef + 0;
                my $numMatches = 0;
                for (my $j = 0; $j < $itemCount; $j++)
                {
                    for (my $i = 0; $i < $numSpareIds; $i++)
                    {
                        if ($slRef->[$i] == $nnRef->[$j])
                        {
                            $numMatches++;
                        }
                    }
                }
                if ($numMatches != 0 && $numMatches != $itemCount)
                {
                    print "Error: for node-name $key, some but not all of its node-ids are listed in node-name $spareKey spares list.\n";
                    $errors++;
                }

            }
        }
    }

    # Verify that all node-ids in the spares list are valid.
    foreach my $spareKey (keys %spares)
    {
        $row = $spares{$spareKey};
        $slRef = $row->[0];
        $numSpareIds = @$slRef + 0;
        for (my $i = 0; $i < $numSpareIds; $i++)
        {
            if (!exists $nodeData[$slRef->[$i]])
            {
                print "Error: for spare node $spareKey, there is no node-id $slRef->[$i].\n";
                $errors++;
            }
        }
    }


    if ($debugFlag)
    {
        print "Spares:\n";
        foreach my $key (keys %spares)
        {
            $row = $spares{$key};
            $slRef = $row->[0];
            $clRef = $row->[1];
            print "   $key => cores=@$clRef, spares=@$slRef\n";
        }

        print "Excluded cores:\n";
        foreach my $key (keys %excluded)
        {
            $row = $excluded{$key};
            print "   $key => @$row\n";
        }
    }

    return $errors;
}

sub numNodes
{
    return @nodeData + 0;
}

sub finalSqlNode
{
    return $lastSqlNode;
}

# Given a logical node-id, return the logical node-id of the next
# node that is in a different physical node.
sub nextPhysicalStorageNode
{
   my $node_id = shift;

   my $row = $nodeData[$node_id];
   # Physical node id associated with the given node-id
   my $pnode_id = $row->[4];
   my $name = $row->[0];

   my @encnodelist;      # List of nodes in the same enclosure
   my $encnodeidx;       # Index into the list of nodes for the physical node

    # Given a physical node associated with the node id, get list of all
    # other physical nodes in the same enclosure
    if (scalar(keys(%EnclosureToNodeName)) > 0)
    {
        my $matchfound = 0;

        for my $enc (keys(%EnclosureToNodeName))
        {
            @encnodelist = ();

            my @lnodes = @{$EnclosureToNodeName{$enc}};
            my $lidx = -1;

            for my $nodename (@lnodes)
            {
                my $isStorageNode = 0;
                my $nnRef = $nodeNameToIds{$nodename};
                $row = $nodeData[$nnRef->[0]];

                $lidx++;

                for my $role (@{$row->[3]})
                {
                    $isStorageNode = 1 if (lc($role) eq "storage");
                }

                # Throw out physical nodes not being used, and also non-storage nodes
                if (defined($nodeNameToIds{$nodename}) && $isStorageNode)
                {
                    push(@encnodelist, $nodename);

                    if ($name eq $nodename)
                    {
                        $encnodeidx = $lidx;
                        $matchfound = 1;
                    }
                }
            }

            last if ($matchfound);
        }
    }

    # Do it as we were before, if this hash hasn't been filled
    if (scalar(keys(%EnclosureToNodeName)) == 0)
    {
        my $examined = 0;
        while ($examined < @nodeData)
        {
            ++$examined;
            ++$node_id;
            if ($node_id > $lastSqlNode)
            {
                $node_id = 0;
            }
            $row = $nodeData[$node_id];
            if ($row->[4] != $pnode_id)
            {
                # Found a different physical node
        #           print "nextPhysNode: for node-id @_[0] in physical node $pnode_id, found node-id $node_id in physical node $row->[4]\n";
                return $node_id;
            }
        }

        # There is only one physical node
        return -50;
    }

    # Preferred way, especially on real cluster
    else
    {
        my ($nnRef, $enc_first_nid, $enc_last_nid);

        $nnRef = $nodeNameToIds{$encnodelist[0]}; # Get the first physical node name in this enclosure
        $enc_first_nid = $nnRef->[0]; # Get the first logical node id on the first physical node

        $nnRef = $nodeNameToIds{$encnodelist[-1]}; # Get the last physical node name in this enclosure
        $enc_last_nid = $nnRef->[-1]; # Get the last logical node id on the last physical node

        my $examined = 0;

        # Only loop over the nodes in this enclosure
        while ($examined < scalar(@encnodelist))
        {
            ++$examined;
            ++$node_id;

            $node_id = $enc_first_nid
                if (($node_id > $enc_last_nid) || ($node_id > $lastSqlNode)); # Wrap around
            $row = $nodeData[$node_id];

            return $node_id if ($row->[4] != $pnode_id);
        }
    }
}

# Given a node-id return the node-name
sub getNodeName
{
   my $node_id = @_[0];
   my $row = $nodeData[$node_id];
#   print "getNodeName: for node_id=$node_id, node-name=$row->[0]\n";
   return $row->[0];
}

# Given a node-id return the physical-node-id
sub getNodePNid
{
   my $node_id = @_[0];
   my $row = $nodeData[$node_id];
#   print "getNodePNid: for node_id=$node_id, physical_node_id=$row->[4]\n";
   return $row->[4];
}

# Given a node-name return a node-id on that node
sub nodeNameToId
{
   my $node_name = @_[0];
   my $nnRef = $nodeNameToIds{$node_name};
   return $nnRef->[0];
}

sub getConnNode
{
    my $ordinalReq = @_[0];

    my $row;
    my $raRef;
    my @roles;
    my $countConn = 0;
    my $firstConn = -1;

    for my $i ( 0 .. $#nodeData )
    {
        $row = $nodeData[$i];

        # Get reference to roles array
        $raRef = $row->[3];
        @roles = @$raRef;

        for my $j ( 0 .. $#roles )
        {
            if ($roles[$j] eq "connection")
            {
                ++$countConn;
                $firstConn = ($firstConn == -1) ? $i : $firstConn;
                if ($countConn == $ordinalReq)
                {   # Found requested connection node
                    return $i;
                }
            }
        }
    }
    if ($firstConn != -1)
    {   # Requested connection node does not exist, return first one.
        return $firstConn;
    }
    else
    {
        print "NDCS No edge nodes defined!\n";
        print "Exiting..\n";
        exit 5;
    }
}

sub getNumberOfConnNodes
{
    my $ordinalReq = @_[0];

    my $row;
    my $raRef;
    my @roles;
    my $countConn = 0;

    for my $i ( 0 .. $#nodeData )
    {
        $row = $nodeData[$i];

        # Get reference to roles array
        $raRef = $row->[3];
        @roles = @$raRef;

        for my $j ( 0 .. $#roles )
        {
            if ($roles[$j] eq "connection")
            {
                ++$countConn;
            }
        }
    }

    return $countConn;
}

sub getConnNodesList
{
    my $ordinalReq = @_[0];

    my $row;
    my $raRef;
    my @roles;
    my @nodelist;
    my $nodelistindex = 0;

    for my $i ( 0 .. $#nodeData )
    {
        $row = $nodeData[$i];

        # Get reference to roles array
        $raRef = $row->[3];
        @roles = @$raRef;

        for my $j ( 0 .. $#roles )
        {
            if ($roles[$j] eq "connection")
            {
                $nodelist[$nodelistindex] = $i;
                ++$nodelistindex;
            }
        }
    }

    return \@nodelist;
}

sub getStorageNodeNames
{
    my $ordinalReq = @_[0];

    my $row;
    my @storNodes;
    my $raRef;
    my $nameRef;
    my @roles;
    my $countConn = 0;

    for my $i ( 0 .. $#nodeData )
    {
        $row = $nodeData[$i];
        $nameRef = $row->[0];

        # Get reference to roles array
        $raRef = $row->[3];
        @roles = @$raRef;

        for my $j ( 0 .. $#roles )
        {
            push(@storNodes, $nameRef) if ($roles[$j] eq "storage")
        }
    }

    return @storNodes;
}

sub buildRoleSet
{
    my $raRef = @_[0];
    my @roles = @$raRef;
    my $roleSet = 0;

    for my $j ( 0 .. $#roles )
    {
        if ($roles[$j] eq "aggregation")
        {
            $roleSet = $roleSet | 0x0002;
        }
        elsif ($roles[$j] eq "storage")
        {
            $roleSet = $roleSet | 0x0004;
        }
        elsif ($roles[$j] eq "connection")
        {
            $roleSet = $roleSet | 0x0001;
        }
        elsif ($roles[$j] eq "operation")
        {
            $roleSet = $roleSet | 0x0008;
        }
        elsif ($roles[$j] eq "maintenance")
        {
            $roleSet = $roleSet | 0x0010;
        }
        elsif ($roles[$j] eq "loader")
        {
            $roleSet = $roleSet | 0x0020;
        }
    }

    return $roleSet;    
}

# Rules that cannot be validated:
#
# The number of cores must be less than or equal to (<=) the number of cores
#  in the physical node <node-name>.
# The total number of cores <core-list> in a physical node <node-name> must be
#  specified in a node section <config>. All cores must be accounted for in the node section

#
# Generate cluster configuration database
#
sub genConfigDb
{
    my $physicalNid = -1;
    my $prevNodeName = "";
    my $row;
    my $nodeName;
    my $clRef;
    my $firstCore;
    my $lastCore;
    my $firstExcl = -1;
    my $lastExcl = -1;
    my $numProcessors;

    # Generate entries for each logical node
    for my $nodeId ( 0 .. $#nodeData )
    {
        $row = $nodeData[$nodeId];

        $nodeName = $row->[0];
        if ($nodeName ne $prevNodeName)
        {
            if (exists $excluded{$nodeName})
            {
                my $xcRef = $excluded{$nodeName};
                $firstExcl = $xcRef->[0];
                $lastExcl = $firstExcl;
                if (@$xcRef > 1)
                {
                    $lastExcl = $xcRef->[1];
                }
            }
            else
            {
                $firstExcl = -1;
                $lastExcl = -1;
            }

            $physicalNid++;
            $prevNodeName = $nodeName;

            # Add a row for this physical node in configuration database
            if ($debugFlag)
            {
                print "Add to pnode table: physicalNid=$physicalNid, node-name=$nodeName, excluded cores=$firstExcl $lastExcl\n";
            }
            sqconfigdb::addDbPNode( $physicalNid, $nodeName, $firstExcl, $lastExcl );

        }
        
        $numProcessors = $row->[1];

        # Get upper and lower bound for cores in this node.
        $clRef = $row->[2];
        $firstCore = $clRef->[0];
        if (@$clRef == 1)
        {
            $lastCore = $firstCore;
        }
        else
        {
            $lastCore = $clRef->[1];
        }

        # Compute the ratio of cores to processors
        my $numCores = ($lastCore + 1) - $firstCore;
        my $coreRatio = $numCores / $numProcessors;
        if ($debugFlag)
        {
            print "    node=$nodeName,nodeId=$nodeId,firstCore=$firstCore, lastCore=$lastCore,coreRatio=$coreRatio\n";
        }

        # List of roles
        my $raRef = $row->[3];

        # Save the physical node id assigned to this logical node.
        $row->[4] = $physicalNid;

        my $procNum = 0;
        for my $core ($firstCore .. $lastCore)
        {
            if ((($core+1) % $coreRatio) == 0)
            {
                $procNum++;
            }
        }

        # Add a row for this logical node in configuration database
        my $roleSet = buildRoleSet( $raRef );
        if ($debugFlag)
        {
            print "      Add to lnode table: physicalNid=$physicalNid, nodeId=$nodeId, processors=$numProcessors, cores=$firstCore $lastCore, roles=$roleSet\n";
        }
        sqconfigdb::addDbLNode( $nodeId, $physicalNid, $numProcessors, $roleSet, $firstCore, $lastCore );
    }

    my $slRef;
    my $sparesData;
    my $numSpareIds;

    # Generate entries for each spare node
    foreach my $spareNodeName (sort keys %spares)
    {
        $sparesData = $spares{$spareNodeName};
        $slRef = $sparesData->[0];
        $numSpareIds = @$slRef + 0;

        # Construct the list of physical node ids that correspond 
        # to the logical node ids in the spare list.
        my @spareSet = ();
        for (my $nodeId = 0; $nodeId < $numSpareIds; $nodeId++)
        {
            # Get the physical node id associated with the logical node id
            $row = $nodeData[$slRef->[$nodeId]];
            my $targetPnid = $row->[4];

            # Add the physical node id to the spare set (unless it is
            # already there).
            my $inSpareSet = 0;
            for (my $i=0; $i<@spareSet; $i++)
            {
                if ($spareSet[$i] == $targetPnid)
                {
                    $inSpareSet = 1;
                    last;
                }
            }
            if (!$inSpareSet)
            {
                push @spareSet, $targetPnid;
            }
        }

        if (exists $excluded{$spareNodeName})
        {
            my $xcRef = $excluded{$spareNodeName};
            $firstExcl = $xcRef->[0];
            $lastExcl = $firstExcl;
            if (@$xcRef > 1)
            {
                $lastExcl = $xcRef->[1];
            }
        }
        else
        {
            $firstExcl = -1;
            $lastExcl = -1;
        }

        # Spare node gets the next physical node id
        $physicalNid++;

        # Add a row for this physical node in configuration database
        if ($debugFlag)
        {
            print "Add spare to pnode table: physicalNid=$physicalNid, spare-node-name=$spareNodeName, excluded cores=$firstExcl $lastExcl\n";
        }
        sqconfigdb::addDbPNode( $physicalNid, $spareNodeName, $firstExcl, $lastExcl );

        # Get upper and lower bound for cores in this node.
        $clRef = $sparesData->[1];
        $firstCore = $clRef->[0];
        if (@$clRef == 1)
        {
            $lastCore = $firstCore;
        }
        else
        {
            $lastCore = $clRef->[1];
        }

        for (my $i=0; $i<@spareSet; $i++)
        {
            if ($debugFlag)
            {
                print "   Add to snode table: spare physicalNid=$physicalNid, spare node-name=$spareNodeName, cores=$firstCore $lastCore, physicalNid=$spareSet[$i]\n";
            }
            sqconfigdb::addDbSpare( $physicalNid
                        , $spareNodeName
                        , $firstCore
                        , $lastCore
                        , $spareSet[$i] );
        }
    }
}

#
# Generate virtual cluster configuration database
#
sub genVirtualConfigDb
{
    my $node_name = @_[0];
    my $node_count = @_[1];

    my $nodeIndex = 0;
    my $firstCore = 0;
    my $lastCore = 0;
    my $firstExcl = -1;
    my $lastExcl = -1;
    my $numProcessors = 0;
    my $roleSet = 0;

    $roleSet = $roleSet | 0x0002; # aggregation
    $roleSet = $roleSet | 0x0004; # storage
    $roleSet = $roleSet | 0x0001; # connection


    # Generate entries for each logical node
    for ($nodeIndex = 0; $nodeIndex < $node_count; $nodeIndex++) {
        # Add a row for this physical node in configuration database
        if ($debugFlag)
        {
            print "Add to pnode table: physicalNid=$nodeIndex, node-name=$node_name, excluded cores=$firstExcl $lastExcl\n";
        }
        sqconfigdb::addDbPNode( $nodeIndex, $node_name, $firstExcl, $lastExcl );

        # Add a row for this logical node in configuration database
        if ($debugFlag)
        {
            print "      Add to lnode table: physicalNid=$nodeIndex, nodeId=$nodeIndex, processors=$numProcessors, cores=$firstCore $lastCore, roles=$roleSet\n";
        }
        sqconfigdb::addDbLNode( $nodeIndex, $nodeIndex, $numProcessors, $roleSet, $firstCore, $lastCore );
    }
}
