# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2011-2014 Hewlett-Packard Development Company, L.P.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
# @@@ END COPYRIGHT @@@

Abbreviation	- description
------------	  -----------
ab		- abandon
acc		- accept
ack		- acknowledgement
actr		- API counter
amp		- audit management process
api		- application programmatic interface
as		- assert state
ase		- audit storage engine
be		- big endian
bfd		- binary file descriptor
blks		- blocks
bmsg		- big message
btime		- boot time
c2s		- client-to-server
cap		- capacity
cb		- callback
cbt		- callback target
cc		- condition code
chk		- check
clio		- client local i/o
cmsgid		- client message identifier
comp		- completion
compid		- component identifier
con		- connect
ct		- control type
ctr		- counter
ctx		- context
cv		- (a) condition variable
		  (b) component version
cwd		- current working directory
d_queue		- doubly linked
da		- default argument
ddd		- data display debugger
dir		- directory
dp2		- disk process-2
dql		- doubly-linked-queue link
dtm		- distributed transaction manager
dtor		- destructor
ecm		- error-check mutex
eh		- event handler
fb		- fully buffered
fc		- file completion
fd		- file descriptor
fe		- file error
fin		- finish
fnum		- file number
fs		- file system
fsdone		- file-system-done
ft		- file type
gmgr		- global manager
hdr		- header
ht		- hash table
iloc		- inline open/close
imap		- int map (map with an int-key)
inv		- invalid
ireq		- incoming request
irq		- interrupt request
itoa		- int to ascii
lcan		- link ancel
ldone		- link done
le		- little endian
lf	  	- load factor
lfnt		- lock free node type
ll		- long long
llmap		- long long map (map with a long long-key)
llml		- map link (long long)
lmap		- long map (map with a long-key)
lml		- map link (long)
locio		- local i/o
lreq		- requesting a link
lu		- look-up
m2s		- master-to-slave
maj		- major
md		- message descriptor
mgr		- manager
min		- minor
mh		- message header
mi		- meminfo
ml		- map link (int)
mon		- monitor
mpi		- message passing interface
mpip		- mpi profiler
ms		- (a) message system
		  (b) millisecond
msgid		- message identifier
msl		- mutex/spin lock
mt		- multi threaded
neg		- negotiation
nid		- node identifier
nps		- nid/pid-stream (pair)
nsem		- named semaphore
od		- open descriptor
oid		- open identifier
op		- operation
opd		- operation detail
p-id		- process identifier (nid/pid) used in traces
pb		- piggyback
pctl		- process control
pe		- print entry
pf		- printf
pflx		- printf (%lx) cast macro
pfp		- printf (%p) cast macro
phandle		- process handle
pi		- process info
pid		- process identifier
pmh		- protocol message header
pname		- process name
pnid		- physical node identifier
proc		- process
prof		- profiler
props		- properties
pstate		- process state
pt		- pointer type
ptr		- pointer
pwu		- private wakeup
qid		- queue identifier
ql		- queue link
rc		- read count
rcv		- receive
rd		- read descriptor
ref		- reference
reg		- (a) monitor registry
		  (b) register
rep		- reply
req		- request
reqid		- request identifier
ri		- receiveinfo
rm		- reqid-msgid (pair)
rnd		- random
rpc		- remote procedure call
rs		- resume / suspend
rsp		- response
rtn		- return
ru		- readupdate
s2c		- server-to-client
s2m		- slave-to-master
sb		- seabed
sem		- semaphore
seq		- sequence
sig		- (a) signal
		  (b) signaling
		  (c) signature
sl		- spin lock
sm		- state machine
smap		- string map (map with a string-key)
smsgid		- server message identifier
sock		- socket
sol		- solicited
sq		- seaquest
sre		- server request element
ss		- send state
ssid		- subsystem identifier
st		- send type
stfs		- scratch and temporary file system
su		- startup
tc		- tag-callback (pair)
tic		- NSK-ism - 10 milliseconds
tid		- thread identifier
tl		- thread local
tle		- timer list element
tls		- thread local storage
tm		- transaction manager (aka dtm)
tmlib		- transaction manager library
tmsync		- transaction manager synchronization
to		- timeout
tov		- timeout value
tpop		- timer pop
tq		- timer queue
tss		- thread specific storage
transid		- transaction identifier
ts		- thread safe
tse		- table storage engine (aka dp2)
tv		- time value
uid		- user identifier
ull		- unsigned long long
unsol		- un-solicited
upd		- update
us		- microsecond
usem		- unnamed semaphore
utrace		- micro trace
val		- value
vm		- virtual memory
vma		- virtual memory address
vnid		- virtual node identifier
vnode		- virtual node
w		- write
wr		- writeread
xdr		- external data representation (rpc)
zid		- zoneid

Term		- description
slot manager	- Manages a slot-table (see slot table)
slot table	- An array of slots.
		  When a slot-table is created, all slots are available.
		  A slot can be allocated or deallocated.
		  Several allocation-types are available.
		  Used extensively for MPI-based data structures and
		  index-mapped data structures such as fd's and md's.
table		- Array of table entries (see table entry)
		  managed by table manager
table entry	- Generic table entry managed through table manager
table manager	- Generic table manager - manages a table (see table)
