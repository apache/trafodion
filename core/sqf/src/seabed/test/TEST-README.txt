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

t2	- FS	- simple FS app 	- similar to t1
t3	- MS	- simple MS app 	- similar to t1
t4	- MS	- simple MS app 	- check > 32K data size
t5	- MS	- simple MS app 	- ctrl reply is larger than ctrl max
t6	- MS	- simple MS app 	- test permutations of sizes/results
t9	- SB	- thread/SB test
t14	- MS	- nowait MSG_LINK_ with multiple outstanding requests
t15	- MS	- multithreaded server test
t16	- MS	- multithreaded server test
t17	- MS	- cli->svc->srv
t18	- MS	- 1 client, 2 servers
t19	- MS	- simple MS app		- similar to t3, with MEM_LEAK define
t20	- MS	- simple MS app		- read monitor messages
t21	- MS	- simple MS app		- read monitor messages
t22	- MS	- test msg_mon_get_process_info
t23	- MS	- 1 client, 1 DTM, 1 TSE- test multiple opens
t24	- MS	- test msg_mon_register_death_notification and shutdown notication
t25	- MS	- process-pair testing
t27	- MS	- simple MS app		- used for bm
t30	- MS 	- process-pair testing	- similar to t25
t31	- FS	- simple FS app 	- similar to t2
t32	- FS	- simple FS app 	- similar to t2
t35	- MS	- transaction stuff
t36	- MS	- msg_mon_get_my_process_name, msg_mon_get_process_...
t37	- FS	- simple FS app		- outstanding requests
t38	- FS	- simple FS app		- reply with error
t39	- FS	- simple FS app		- both open $receive
t40	- FS	- simple FS app		- FILE_GETRECEIVEINFO test
t41	- MS	- simple MS app		- cl opens srv, srv opens cli
t42	- MS	- simple MS app		- test msg_mon_get_process_info_type
t43	- MS	- simple MS app		- test LDONE queueing
t44	- MS	- simple MS app		- test LDONE queueing threaded
t45	- FS	- simple FS app		- test trans enlist/delist
t48	- FS	- simple FS app		- test AWAITIOX with timeout > 0
t49	- FS	- simple FS app		- test 2 WR to 2 processes
t50	- FS	- simple FS app 	- similar to t2, double-open
t51	- MS	- simple MS app 	- test tmsync
t52	- SB	- simple MS app		- test node-info
t53	- FS	- simple FS app		- test trans no cb for tx=0
t54	- FS	- simple FS app		- test trans for double WR
t55	- FS	- simple FS app		- test trans for double WR (ART)
t56	- FS	- simple FS app		- used for bm
t57	- MS	- simple MS app		- test LDONE queueing threaded (2)
t58	- MS	- simple MS app		- test LDONE queueing threaded (3)
t61	- MS	- simple MS app 	- similar to t3 (BIGGER I/O)
t62	- FS	- simple FS app 	- similar to t2 (BIGGER I/O)
t63	- MS	- simple MS app		- test LDONE queueing (with srv death)
t64	- MS	- simple MS app		- similar to t43 (two servers)
t65	- FS	- simple FS app		- simple CC testing
t66	- MS	- simple MS app		- uses no copy interface
t67	- FS	- simple FS app		- uses no copy interface
t68	- MS	- simple MS app		- test LISTEN filtering
t69	- MS	- simple MS app		- similar to t3 using attach
t70	- SB	- simple MS app		- start process	- infile/outfile
t71	- SB	- simple MS app		- registry test
t72	- FS	- simple FS app		- similar to t2 using attach
t73	- MS	- multithreaded client/server test
t74	- MS	- simple MS app		- similar to t3 (open/close)
t75	- MS	- multithreaded client/server test
t76	- FS	- recreate 511 error - open/open/close/open/close
t77	- FS	- simple FS app 	- similar to t2 (check srv sender map)
t78	- FS	- simple FS app 	- similar to t2 (check receive info)
t79	- FS	- simple FS app 	- similar to t2 (check receive info)
t80	- FS	- simple FS app 	- similar to t2 (check sys msgs)
t83	- FS	- simple FS app 	- 3-tier AWAITIOX(-1) test
t84	- FS	- simple FS app 	- test no sys msgs
t85	- MS	- simple MS app 	- test send-event
t86	- MS	- simple MS app 	- test open-info
t88	- MS	- simple MS app 	- test MSG_HOLD
t91	- MS	- simple MS app		- threaded waiting test
t92	- MS	- simple MS app		- threaded waiting test
t93	- MS	- simple MS app		- 1 client to many servers
t94	- FS	- simple FS app		- test open problem
t95	- FS	- simple FS app		- test trans no cb for tx=0 (supp)
t96	- FS	- simple FS app		- test slot dealloc problem
t98	- MS	- simple MS app 	- start-and-exit
t100	- MS	- simple MS app 	- client fail/start/open (fails if no ADOPT_PID)
t101	- FS	- simple FS app 	- BWRITEREADX2 and WRITEREADX2
t102	- FS	- simple FS app 	- test shutdown system message
t103	- FS	- simple FS app 	- test change system message
t104	- MS	- simple MS app 	- mount device
t105	- MS	- simple MS app 	- test tmsync take 2
t115	- MS	- simple MS app		- longevity
t118	- MS	- simple MS app		- used for monitor-services bm
t124	- FS	- simple FS app 	- M-client to N-server FS test
t126	- FS	- simple FS app		- test trans cb errors
t127	- MS	- simple MS app		- open test
t128	- FS	- simple FS app		- 16 outstanding READUPDATES
t129	- MS	- simple MS app		- longevity - similar to t115
t130	- SB	- simple SB app		- map test
t131	- MS	- simple MS app		- ABANDON test
t132	- MS	- simple MS app		- opens cross, accept problem
t133	- MS	- simple MS app		- start-process nw
t135	- MS	- simple MS app		- ABANDON test
t137	- MS	- simple MS app		- XPROCESSOR_GETINFOLIST_
t138	- MS	- simple MS app		- check CV.wait
t139	- SB	- simple SB app		- check itoa
t141	- MS	- simple MS app		- LINK/REPLY after broken conn
t142	- MS	- simple MS app		- start-process nw-callback
t145	- MS	- simple MS app		- test shutdown API
t149	- MS	- simple MS app		- multi-threaded LINK w/ errors
t151	- MS	- simple MS app		- shutdown with accept
t152	- MS	- simple MS app		- MS open-nowait
t153	- FS	- simple FS app		- AWAITIOX -2
t155	- SB	- simple MS app		- send-to-self MS
t156	- SB	- simple FS app		- send-to-self FS
t157	- SB	- simple SB app		- send-to-self simple
t160	- MS	- simple MS app		- multiple $RECEIVE readers
t161	- MS	- simple MS app		- timer
t162	- FS	- simple FS app		- timer
t163	- MS	- simple MS app		- priority-q
t164	- FS	- simple FS app		- FS open-nowait
t165	- FS	- simple FS app 	- FS opens over set of servers
t167	- MS	- simple MS app 	- XWAIT with timeouts
t168	- FS	- simple FS app 	- test LDONE problem
t169	- MS	- simple MS app 	- multiple $RECEIVE readers bm
t170	- MS	- simple MS app 	- test X/BLISTEN_TEST_IREQM
t172	- MS	- simple MS app 	- exec timing bm
t176	- FS	- simple FS app 	- M-client to N-server FS test
t180	- MS	- simple MS app 	- M-client to N-server MS test
t182	- FS	- simple FS app 	- BFILE_COMPLETE_
t183	- MS	- simple MS app 	- MSG_LINK_ with errors
t184	- FS	- simple FS app 	- epoll/PWU
t185	- FS	- simple FS app 	- similar to t2 with dup open
t186	- FS	- simple FS app 	- WRITEX/READX
t187	- MS	- simple MS app 	- msg_mon_register_death_notification2
t188	- MS	- simple MS app 	- XPROCESS_AWAKE_
t190	- FS	- simple FS app 	- mon open/close msg test
t192	- FS	- simple FS app 	- XFILENAME_TO_PROCESSHANDLE_
t194	- FS	- simple FS app 	- BCANCELREQ/XCANCELREQ
t195	- FS	- simple FS app 	- like t37, but with start-process
t198	- FS	- simple FS app 	- cross-close
t197	- FS	- simple FS app 	- check recv-depth
t201	- MS	- simple MS app 	- resume/suspend
t202	- MS	- simple MS app 	- launch 3rd party binary
t205	- FS	- simple FS app		- used for monitor-services bm (FS)
t206	- FS	- simple FS app		- mult-out i/os [optional abort]
t207	- FS	- simple FS app 	- more BCANCELREQ/XCANCELREQ
t208	- FS	- simple FS app 	- similar to t84 with userid
t209	- MS	- simple MS app 	- dump
t210	- MS	- simple MS app 	- mount device take 2
t212	- MS	- simple MS app 	- server dies - BREAK hang problem
t213	- FS	- simple FS app 	- multi-threaded FS client
t214	- FS	- simple FS app 	- multi-client/multi-server FS
t215	- FS	- simple FS app 	- many clients to server
t216	- MS	- simple MS app 	- many clients to server (w/ aband)
t217	- FS	- simple FS app 	- many clients to server (w/ cancel)
t219	- MS	- simple MS app 	- XMSG_LINK2_/XMSG_BREAK2_
t220	- FS	- simple FS app 	- BCANCEL/BCANCELREQ for $RECEIVE
t221	- MS	- simple MS app 	- ABANDON after PATHDOWN
t222	- MS	- simple MS app 	- start process
t226	- FS	- simple FS app 	- close while outstanding i/os
t227	- FS	- simple FS app 	- close while outstanding i/os [2]
t228	- MS	- simple FS app 	- ABANDON with multi server threads
t231	- FS	- simple FS app 	- client wr server that reincarnates
t233	- MS	- simple MS app 	- M-client to N-server MS (max out)
t234	- MS	- simple MS app 	- M-client to N-server MS (open/close)
t235	- MS	- simple MS app 	- many opens to a server
t238	- MS	- alternate logger test
t240	- MS	- simple MS app 	- test gdb/sb.gdb
t241	- MS	- simple MS app 	- msg_enable_open_cleanup
t242	- FS	- simple FS app 	- file_enable_open_cleanup
t245	- MS	- simple MS app 	- Error on many outstanding-pair
t246	- MS	- simple MS app 	- Check notices (up/down)
t249	- MS	- simple MS app		- Check text functions
t250	- MS	- simple MS app 	- similar to t3
t253	- MS	- simple MS app 	- multiple threads opening same server
t261	- MS	- simple MS app		- msg_mon_get_zone_info and detail
t262	- MS	- simple MS app		- object-timers
t263	- MS	- simple MS app		- idle-conn
t266	- MS	- simple MS app		- TLS
t277	- FS	- simple FS app		- thread-specific FS
t279	- MS	- simple MS app		- verifier
t280	- FS	- simple FS app		- verifier
t281	- MS	- simple MS app		- stream-count-problem
t282	- MS	- simple MS app		- client-death stress
t285	- FS	- simple FS app		- like t45 but with tmlib2
t286	- MS	- simple MS app		- stress monitor
