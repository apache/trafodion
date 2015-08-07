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

#ifdef USE_SEAPILOT
#include "amqpmon.h"
#include "wrapper/amqpwrapper.h"

bool amqp_initialized = false;
#endif

#include "msgdef.h"

void AMQP_INITIALIZE()
{
#ifdef USE_SEAPILOT

    if (!amqp_initialized)
        createAMQPConnection();
#endif
}
void STARTUP ()
{
#ifdef USE_SEAPILOT
    if (!amqp_initialized)
        AMQP_INITIALIZE();

    monitor::instance_up mon_instance;
    initAMQPInfoHeader(mon_instance.mutable_header(), 1);
    AMQPRoutingKey routingKey(SP_HEALTH_STATE, SP_MONITORPACKAGE, SP_INSTANCE, 
                              SP_PUBLIC, SP_GPBPROTOCOL, "instance_up");
    sendAMQPMessage(false, mon_instance.SerializeAsString(), 
                    SP_CONTENT_TYPE_APP, routingKey);
#endif
}

void SHUTDOWN (void *msg)
{
#ifdef USE_SEAPILOT
    if (!amqp_initialized)
        AMQP_INITIALIZE();

    struct message_def *local_msg = (struct message_def *)msg;
    monitor::instance_down mon_instance;

    mon_instance.set_nid(local_msg->u.request.u.shutdown.nid);
    mon_instance.set_pid(local_msg->u.request.u.shutdown.pid);
    mon_instance.set_level(local_msg->u.request.u.shutdown.level);

    initAMQPInfoHeader(mon_instance.mutable_header(), 1);
    AMQPRoutingKey routingKey(SP_HEALTH_STATE, SP_MONITORPACKAGE, SP_INSTANCE, 
                              SP_PUBLIC, SP_GPBPROTOCOL, "mon_instance");
    sendAMQPMessage(false, mon_instance.SerializeAsString(), 
                    SP_CONTENT_TYPE_APP, routingKey);
#endif
}

void NODEDOWN (void *msg)
{
#ifdef USE_SEAPILOT
    if (!amqp_initialized)
        AMQP_INITIALIZE();

    struct message_def *local_msg = (struct message_def *)msg;
    monitor::node_down node_down;

    node_down.set_nid(local_msg->u.request.u.down.nid);
    node_down.set_node_name(local_msg->u.request.u.down.node_name);

    initAMQPInfoHeader(node_down.mutable_header(), 1);
    AMQPRoutingKey routingKey(SP_HEALTH_STATE, SP_MONITORPACKAGE, SP_INSTANCE, 
                              SP_PUBLIC, SP_GPBPROTOCOL, "node_down");
    sendAMQPMessage(false, node_down.SerializeAsString(), 
                    SP_CONTENT_TYPE_APP, routingKey);
#endif
}

void NODEUP (void *msg)
{
#ifdef USE_SEAPILOT
    if (!amqp_initialized)
        AMQP_INITIALIZE();

    struct message_def *local_msg = (struct message_def *)msg;
    monitor::node_up node_up;

    node_up.set_nid(local_msg->u.request.u.up.nid);
    node_up.set_node_name(local_msg->u.request.u.up.node_name);

    initAMQPInfoHeader(node_up.mutable_header(), 1);
    AMQPRoutingKey routingKey(SP_HEALTH_STATE, SP_MONITORPACKAGE, SP_INSTANCE, 
                              SP_PUBLIC, SP_GPBPROTOCOL, "node_up");
    sendAMQPMessage(false, node_up.SerializeAsString(), 
                    SP_CONTENT_TYPE_APP, routingKey);
#endif
}
void PROCESSOPEN (void *msg)
{
#ifdef USE_SEAPILOT
    if (!amqp_initialized)
        AMQP_INITIALIZE();

    struct message_def *local_msg = (struct message_def *)msg;
    monitor::process_open process_open;

    process_open.set_nid(local_msg->u.request.u.open.nid);
    process_open.set_pid(local_msg->u.request.u.open.pid);
    process_open.set_process_name(local_msg->u.request.u.open.process_name);
    process_open.set_death_notification(local_msg->u.request.u.open.death_notification);

    initAMQPInfoHeader(process_open.mutable_header(), 1);
    AMQPRoutingKey routingKey(SP_HEALTH_STATE, SP_MONITORPACKAGE, SP_INSTANCE, 
                              SP_PUBLIC, SP_GPBPROTOCOL, "process_open");
    sendAMQPMessage(false, process_open.SerializeAsString(), 
                    SP_CONTENT_TYPE_APP, routingKey);
#endif
}

void PROCESSCLOSE (void *msg)
{
#ifdef USE_SEAPILOT
    if (!amqp_initialized)
        AMQP_INITIALIZE();

    struct message_def *local_msg = (struct message_def *)msg;
    monitor::process_close process_close;

    process_close.set_nid(local_msg->u.request.u.close.nid);
    process_close.set_pid(local_msg->u.request.u.close.pid);
    process_close.set_process_name(local_msg->u.request.u.close.process_name);
    process_close.set_aborted(local_msg->u.request.u.close.aborted);
    process_close.set_mon(local_msg->u.request.u.close.mon);

    initAMQPInfoHeader(process_close.mutable_header(), 1);
    AMQPRoutingKey routingKey(SP_HEALTH_STATE, SP_MONITORPACKAGE, SP_INSTANCE, 
                              SP_PUBLIC, SP_GPBPROTOCOL, "process_close");
    sendAMQPMessage(false, process_close.SerializeAsString(), 
                    SP_CONTENT_TYPE_APP, routingKey);
#endif
}

void PROCESSSTART (void *msg)
{
#ifdef USE_SEAPILOT
    if (!amqp_initialized)
        AMQP_INITIALIZE();

    struct message_def *local_msg = (struct message_def *)msg;
    monitor::process_start process_start;

    process_start.set_nid(local_msg->u.request.u.startup.nid);
    process_start.set_nid(local_msg->u.request.u.startup.pid);
    process_start.set_os_pid(local_msg->u.request.u.startup.os_pid);
    process_start.set_system_messages(local_msg->u.request.u.startup.system_messages);
    process_start.set_port_name(local_msg->u.request.u.startup.port_name);
    process_start.set_paired(local_msg->u.request.u.startup.paired);
    process_start.set_program(local_msg->u.request.u.startup.program);
    process_start.set_process_name(local_msg->u.request.u.startup.process_name);

    initAMQPInfoHeader(process_start.mutable_header(), 1);
    AMQPRoutingKey routingKey(SP_HEALTH_STATE, SP_MONITORPACKAGE, SP_INSTANCE, 
                              SP_PUBLIC, SP_GPBPROTOCOL, "process_start");
    sendAMQPMessage(false, process_start.SerializeAsString(), 
                    SP_CONTENT_TYPE_APP, routingKey);
#endif
}

void PROCESSKILL (void *msg)
{
#ifdef USE_SEAPILOT
    if (!amqp_initialized)
        AMQP_INITIALIZE();

    struct message_def *local_msg = (struct message_def *)msg;
    monitor::process_kill process_kill;

    process_kill.set_nid(local_msg->u.request.u.kill.nid);
    process_kill.set_pid(local_msg->u.request.u.kill.pid);
    process_kill.set_target_nid(local_msg->u.request.u.kill.target_nid);
    process_kill.set_target_pid(local_msg->u.request.u.kill.target_pid);
    process_kill.set_process_name(local_msg->u.request.u.kill.process_name);

    initAMQPInfoHeader(process_kill.mutable_header(), 1);
    AMQPRoutingKey routingKey(SP_HEALTH_STATE, SP_MONITORPACKAGE, SP_INSTANCE, 
                              SP_PUBLIC, SP_GPBPROTOCOL, "process_kill");
    sendAMQPMessage(false, process_kill.SerializeAsString(), 
                    SP_CONTENT_TYPE_APP, routingKey);
#endif
}

void PROCESSEXIT (void *msg)
{
#ifdef USE_SEAPILOT
    if (!amqp_initialized)
        AMQP_INITIALIZE();

    struct message_def *local_msg = (struct message_def *)msg;
    monitor::process_exit process_exit;

    process_exit.set_nid(local_msg->u.request.u.exit.nid);
    process_exit.set_pid(local_msg->u.request.u.exit.pid);

    initAMQPInfoHeader(process_exit.mutable_header(), 1);
    AMQPRoutingKey routingKey(SP_HEALTH_STATE, SP_MONITORPACKAGE, SP_INSTANCE, 
                              SP_PUBLIC, SP_GPBPROTOCOL, "process_exit");
    sendAMQPMessage(false, process_exit.SerializeAsString(), 
                    SP_CONTENT_TYPE_APP, routingKey);
#endif
}

