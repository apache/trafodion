///////////////////////////////////////////////////////////////////////////////
//
// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "seabed/trace.h"
#include "montrace.h"
#include "monlogging.h"
#include "cmsh.h"

typedef list<string>    StringList_t;

// Class inplementation

CCmsh::CCmsh( const char *command )
      :CSystem( command )
      ,nodeStateList_( )
{
    const char method_name[] = "CCmsh::CCmsh";
    TRACE_ENTRY;

    TRACE_EXIT;
}

CCmsh::~CCmsh( void )
{
    const char method_name[] = "CCmsh::~CCmsh";
    TRACE_ENTRY;

    TRACE_EXIT;
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: CCmsh::PopulateClusterState
//
// Description: Executes the command string passed in the constructor and
//              populates the internal node state list with the state of each node
//              in the cluster. Clients can then inquire about state of each node.
//              
// Return:
//        0 - success
//       -1 - failure
//
///////////////////////////////////////////////////////////////////////////////
int CCmsh::PopulateClusterState( void )
{
    const char method_name[] = "CCmsh::PopulateClusterState";
    TRACE_ENTRY;

    int rc;

    // The caller should save and close stdin before calling this proc
    // and restore it when done. This is to prevent ssh from consuming
    // caller's stdin contents when executing the command. 
    rc = ExecuteCommand( nodeStateList_ );
    if ( rc == -1 )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s] Error: While executing '%s' command\n", method_name, command_.data());
        mon_log_write(MON_CMSH_GET_CLUSTER_STATE_1, SQ_LOG_ERR, la_buf);
    }

    TRACE_EXIT;
    return( rc );
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: CCmsh::PopulateNodeState
//
// Description: Executes the command string passed in the constructor and
//              populates the internal node state list with the state of each node
//              in the cluster. Clients can then inquire about state of each node.
//              
// Return:
//        0 - success
//       -1 - failure
//
///////////////////////////////////////////////////////////////////////////////
int CCmsh::PopulateNodeState( const char *nodeName )
{
    const char method_name[] = "CCmsh::PopulateNodeState";
    TRACE_ENTRY;

    int rc;

    // The caller should save and close stdin before calling this proc
    // and restore it when done. This is to prevent ssh from consuming
    // caller's stdin contents when executing the command. 
    string commandArgs;
    {
        commandArgs = "-n ";
        commandArgs += nodeName;
    }
    rc = ExecuteCommand( commandArgs.c_str(), nodeStateList_ );
    if ( rc == -1 )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s] Error: While executing '%s' command\n", method_name, command_.data());
        mon_log_write(MON_CMSH_GET_CLUSTER_STATE_1, SQ_LOG_ERR, la_buf);
    }

    TRACE_EXIT;
    return( rc );
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: CCmsh::GetClusterState
//
// Description: Updates the state of the nodes in the physicalNodeMap passed in
//              as a parameter. Caller should ensure that the node names are already
//              present in the physicalNodeMap. 
//
// Return:
//        0 - success
//       -1 - failure
//
///////////////////////////////////////////////////////////////////////////////
int CCmsh::GetClusterState( PhysicalNodeNameMap_t &physicalNodeMap )
{
    const char method_name[] = "CCmsh::GetClusterState";
    TRACE_ENTRY;

    int rc;

    rc = PopulateClusterState();

    if ( rc != -1 )
    {
        // Parse each line extracting name and state
        string nodeName;
        NodeState_t nodeState;
        CPhysicalNode  *physicalNode;
        PhysicalNodeNameMap_t::iterator it;
        
        StringList_t::iterator    alit;
        for ( alit = nodeStateList_.begin(); alit != nodeStateList_.end() ; alit++ )
        {
            ParseNodeStatus( *alit, nodeName, nodeState );
            // Get corresponding physical node by name
            it = physicalNodeMap.find( nodeName.c_str() );
            if (it != physicalNodeMap.end())
            {
               // TEST_POINT and Exclude List : to force state down on node name 
                const char *downNodeName = getenv( TP001_NODE_DOWN );
                const char *downNodeList = getenv( TRAF_EXCLUDE_LIST );
                string downNodeString = " ";
                if (downNodeList)
                {
                    downNodeString += downNodeList;
                    downNodeString += " ";
                }
                string downNodeToFind = " ";
                downNodeToFind += nodeName.c_str();
                downNodeToFind += " ";
                if (((downNodeList != NULL) && 
                      strstr(downNodeString.c_str(),downNodeToFind.c_str())) ||
                    ((downNodeName != NULL) && 
                     !strcmp(downNodeName, nodeName.c_str())))
                {
                    nodeState = StateDown;
                }
          
                // Set physical node state
                physicalNode = it->second;
                physicalNode->SetState( nodeState );
            }
        }
    }  

    TRACE_EXIT;
    return( rc );
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: CCmsh::GetNodeState
//
// Description: Updates the state of the nodeName in the physicalNode passed in
//              as a parameter. Caller should ensure that the node names are already
//              present in the physicalNodeMap. 
//
// Return:
//        0 - success
//       -1 - failure
//
///////////////////////////////////////////////////////////////////////////////
int CCmsh::GetNodeState( char *name ,CPhysicalNode  *physicalNode )
{
    const char method_name[] = "CCmsh::GetNodeState";
    TRACE_ENTRY;

    int rc;

    rc = PopulateNodeState( name );

    if ( rc != -1 )
    {
        // Parse each line extracting name and state
        string nodeName;
        NodeState_t nodeState;
        PhysicalNodeNameMap_t::iterator it;
        
        StringList_t::iterator    alit;
        for ( alit = nodeStateList_.begin(); alit != nodeStateList_.end() ; alit++ )
        {
            ParseNodeStatus( *alit, nodeName, nodeState );

            // TEST_POINT and Exclude List : to force state down on node name 
            const char *downNodeName = getenv( TP001_NODE_DOWN );
            const char *downNodeList = getenv( TRAF_EXCLUDE_LIST );
            string downNodeString = " ";
            if (downNodeList)
            {
                downNodeString += downNodeList;
                downNodeString += " ";
            }
            string downNodeToFind = " ";
            downNodeToFind += nodeName.c_str();
            downNodeToFind += " ";
            if (((downNodeList != NULL) && 
                  strstr(downNodeString.c_str(),downNodeToFind.c_str())) ||
                ((downNodeName != NULL) && 
                 !strcmp(downNodeName, nodeName.c_str())))
            {
                nodeState = StateDown;
            }

            if (!strcmp(name, nodeName.c_str()))
            {
                // Set physical node state
                physicalNode->SetState( nodeState );
            }
        }
    }  

    TRACE_EXIT;
    return( rc );
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: CCmsh::ParseNodeStatus
//
// Description: Parses the nodeStatus string passed and 
//              returns the node name and its state.
//              
// Return:      Node name and state as parameters. 
//
///////////////////////////////////////////////////////////////////////////////
void CCmsh::ParseNodeStatus( string &nodeStatus, string &nodeName, NodeState_t &state )
{
    const char method_name[] = "CCmsh::ParseNodeStatus";
    TRACE_ENTRY;

    char  token[TOKEN_SIZE_MAX];
    char  workString[TOKEN_SIZE_MAX];
    char *ptr1 = workString;

    strcpy( workString, nodeStatus.c_str() );
    char *end = FindEndOfToken( ptr1, TOKEN_SIZE_MAX );
    char *ptr2 = strchr( end, '[' );
    if ( ptr2 )
    {
        ++ptr2;
        ptr2 = RemoveWhiteSpace ( ptr2 );
        state = !strncmp ( ptr2, "UP", 2) ? StateUp : StateDown ;
    }
    else
    {
        state = StateDown;
    }
    
    if ( *end )
    {
        *end = '\0';
        strcpy (token, ptr1);
    }
    else
    {
        strcpy (token, end);
    }
    
    nodeName.assign( token ) ;
    TRACE_EXIT;
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: CCmsh::GetNodeState
//
// Description: Returns the state of a node.
//              
// Return:      Node state
//
///////////////////////////////////////////////////////////////////////////////
NodeState_t CCmsh::GetNodeState( char nodeNameIn[] )
{
    const char method_name[] = "CCmsh::GetNodeState";
    TRACE_ENTRY;

    string nodeName;
    NodeState_t nodeState;
    StringList_t::iterator alit;

    for ( alit = nodeStateList_.begin(); alit != nodeStateList_.end() ; alit++ )
    {
        ParseNodeStatus( *alit, nodeName, nodeState );
        
        if (!nodeName.compare(nodeNameIn))
        {
            TRACE_EXIT;
            return nodeState;
        }
    }

    TRACE_EXIT;
    return StateUnknown; 
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: CCmsh::IsInitialized
//
// Description: Returns whether the node state list has been populated or not
//              
// Return:      bool value
//
///////////////////////////////////////////////////////////////////////////////
bool CCmsh::IsInitialized( void )
{
    const char method_name[] = "CCmsh::IsInitialized";
    TRACE_ENTRY;

    bool retval = false; 
    
    if ( nodeStateList_.size() > 0 )
    {
        retval = true;
    }
    
    TRACE_EXIT;
    return retval; 
}
