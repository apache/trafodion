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
# monitor functions

define monitor
	help monitor
end

document monitor
	<cmd>          <args>       <description>
	pprocs         none         display all processes
        lnprocs        <nid>        display proceses for a given logical node
	findprocbypid  <nid> <pid>  find process given by nid pid
	findprocbyname "name"       find process given by name
        noticecount    <process>    display count of CNotice objects for a process
        tailnoticecount <process>   display count of CNotice objectss for a process

        noticeaudit    <process>    checks notices on process' death notification list
        noticebw       <process>    Displays CNotice objects (backward traversal)
        findnoticetarget <process> <nid> <pid> find specific CNotice object
        listprocopens  <process>   lists the processes opened by the given process.
        openbw         <process>   Given a COpen object, follows previous links
        findprocbyopenhead <nid> <open>  Looks for process with given OpenHead
        liobufnum  <address>        display buffer number for local io buffer address
        liobufaddr <number>         display address for local io buffer number
	scan_liobuf    none         display information about each in-use local io buffer
	mon_stats      none         display local i/o statistics
end

#
# The "pprocs" command displays all proceses.  It uses the linked
# list of CProcess objects in the CProcessContainer for each physical node.
#
define pprocs
   printf "NumberPNodes %d\n", Nodes.NumberPNodes
   printf "NumberLNodes %d\n", Nodes.NumberLNodes
   set $n = 0
   while $n < Nodes.NumberPNodes
      printf "Node[%d]=%p\n", $n, Nodes.Node[$n]
      set $numprocs = Nodes.Node[$n]->numProcs_
      printf "NumProcs=%d\n",$numprocs

      set $proc = Nodes.Node[$n]->CProcessContainer::head_

      set $n++

      set $p = 0
      while $p < $numprocs
         printf "CProcess[%d] for %s (%d, %d) = %p\n", $p, $proc->Name, $proc->Nid, $proc->Pid, $proc
         set $proc = $proc->next_
         set $p++
      end
      if $proc != 0
         printf "Final process next pointer non-zero.\n"
      end
   end
end

document pprocs
   Displays the address of each monitor process object (CProcess) in all nodes.

   Syntax:  pprocs
end

#
# The "lnprocs" command displays all proceses for a specific logical node.
# It uses the linked list of CProcess objects.
define lnprocs
   if $argc != 1
      help npprocs
   else
      set $lnode = Nodes->LNode[$arg0]
      set $numprocs = $lnode->numProcs_
      printf "Logical node[%d]=%p, %d processes\n", $arg0, $lnode, $numprocs
      set $proc = $lnode->head_

      set $p = 0
      while $p < $numprocs
         printf "CProcess[%d] for %s (%d, %d) = %p\n", $p, $proc->Name, $proc->Nid, $proc->Pid, $proc
         set $proc = $proc->nextL_
         set $p++
      end
      if $proc != 0
         printf "Final process next pointer non-zero.\n"
      end
   end
end

document lnprocs
   Displays the address of each monitor process object (CProcess) for a given logical node.

   Syntax:  lnprocs <nid>
end


#
# The "findprocbypid" command locates the CProcess object for a given
# process which identified by its <nid> and <pid>.   It uses the linked
# list of CProcess objects in the CProcessContainer for each node.
#
define findprocbypid
   if $argc != 2
      help findprocbypid 
   else
      if $arg0 < 0 || $arg0 >= Nodes->NumberLNodes
         printf "no such node: %d\n", $arg0
      else
         set $pnode = Nodes->LNode[$arg0].lnodes_->node_
         set $proc = $pnode->CProcessContainer::head_
         set $found = 0
         while $proc != 0
            if $proc->Pid == $arg1
               printf "CProcess object for %s (%d, %d) = %p\n", $proc->Name, $arg0, $arg1, $proc
               set $proc = 0
               set $found = 1
               loop_break
            else
               set $proc = $proc->next_
            end
         end
         if $found == 0
            printf "no such process %d %d\n", $arg0, $arg1
         end
      end
   end
end


document findprocbypid
   Given a node-id and process-id attempts to find associated process object

   Syntax: findprocbypid <node-id> <process-id>
end 

#
# The "findprocbyname" command locates the CProcess object for a given
# process which is identified by its name.   It uses the linked
# list of CProcess objects in the CProcessContainer for each node.
#
define findprocbyname
   if $argc != 1
      help findprocbyname
   else
      printf "Searching for process %s\n", $arg0

      set $n = 0
      set $proc = 0
      set $found = 0
      while $n < Nodes.NumberPNodes

         set $numprocs = Nodes.Node[$n]->numProcs_
         printf "Node[%d]=%p, NumProcs=%d\n", $n, Nodes.Node[$n], $numprocs

         set $proc = Nodes.Node[$n]->CProcessContainer::head_

         set $n++

         set $p = 0
         while $p < $numprocs
            if strcmp($proc->Name, $arg0) == 0
               printf "CProcess object for %s (%d, %d) = %p\n", $proc->Name, $proc->Nid, $proc->Pid, $proc
               set $found = 1
               loop_break
            end

            set $proc = $proc->next_
            set $p++
         end
         if $found == 0
            printf "no such process %s\n", $arg0
         end
      end
   end
end

document findprocbyname
Given a process name attempts to find associated process object

Syntax: findprocbyname name

Example:
	findprocbyname "$SYSTEM"
end 


#
# The "noticecount" command displays the number of CNotice objects on the
# "process death notice" list for a given process.
#
define noticecount
   if $argc != 1
      help noticecount
   else
      set $process = $arg0
      if $process->eyecatcher_ != 0x434f5250
         printf "Argument 1 (%p) does not specify a valid CProcess object (%d)\n", $arg0, $process->eyecatcher_
      else
         printf "For process %p: Nid=%d Pid=%d NoticeHead=%p NoticeTail=%p\n", $process, $process->Nid, $process->Pid, $process->NoticeHead, $process->NoticeTail
         set $notice = $process->NoticeHead
         set $ncount = 0
         while $notice != 0 && $ncount < 50000
            set $ncount++
            set $notice = $notice->Next
         end
         if $ncount < 50000
            printf "For process %p: there are %d notices\n", $process, $ncount
         else
            printf "For process %p: possible notice list problem, stopped after 5000 notices", $process
         end
      end
   end
end

document noticecount
Displays the count of notices on the process' death notification list

Syntax: noticecount processObj

Examples:
	noticecount process
        noticecount (CProcess*)0xe04b2ae8
end


#
# The "tailnoticecount" command displays the number of CNotice objects on the
# "process death notice" list for a given process.  It starts with the 
# final notice and proceeds backwards up the notice list.
#
define tailnoticecount
   if $argc != 1
      help noticecount
   else
      set $process = $arg0
      if $process->eyecatcher_ != 0x434f5250
         printf "Argument 1 (%p) does not specify a valid CProcess object (%d)\n", $arg0, $process->eyecatcher_
      else
         printf "For process %p: Nid=%d Pid=%d NoticeHead=%p NoticeTail=%p\n", $process, $process->Nid, $process->Pid, $process->NoticeHead, $process->NoticeTail
         set $notice = $process->NoticeTail
         set $ncount = 0
         while $notice != 0 && $ncount < 5000
            set $ncount++
            set $notice = $notice->Prev
         end
         if $ncount < 5000
            printf "For process %p: there are %d notices\n", $process, $ncount
         else
            printf "For process %p: possible notice list problem, stopped after 5000 notices", $process
         end
      end
   end
end

document tailnoticecount
Displays the count of notices on the process' death notification list 
(starting from the tail of the list)

Syntax: tail noticecount processObj

Examples:
	tailnoticecount process
        tailnoticecount (CProcess*)0xe04b2ae8
end

#
# The "noticeaudit" command displays the number of CNotice objects on the
# "process death notice" list for a given process.
#
define noticeaudit
   if $argc != 1
      help noticeaudit
   else
      set $process = $arg0
      if $process->eyecatcher_ != 0x434f5250
         printf "Argument 1 (%p) does not specify a valid CProcess object (%d)\n", $arg0, $process->eyecatcher_
      else
         printf "For process %p: Nid=%d Pid=%d NoticeHead=%p NoticeTail=%p\n", $process, $process->Nid, $process->Pid, $process->NoticeHead, $process->NoticeTail
         set $notice = $process->NoticeHead
         set $ncount = 0
         set $procExists = 0
         set $procNotExists = 0
         while $notice != 0 && $ncount < 50000

            set $pnode = Nodes->LNode[$notice->Nid].lnodes_->node_
            set $proc = $pnode->CProcessContainer::head_
            set $found = 0
            while $proc != 0
               if $proc->Pid == $notice->Pid
                  set $procExists++
                  set $proc = 0
                  set $found = 1
                  loop_break
               else
                  set $proc = $proc->next_
               end
            end
            if $found == 0
               set $procNotExists++
            end

            set $ncount++

#            printf "Notice #%d (%x), found=%d, Nid=%d, Pid=%d\n", $ncount, $notice, $found, $notice->Nid, $notice->Pid

            set $notice = $notice->Next
         end
         if $ncount < 50000
            printf "For process %p: there are %d notices, non-existent processes=%d, active processes=%d\n", $process, $ncount, $procNotExists, $procExists
         else
            printf "For process %p: possible notice list problem, stopped after 50000 notices", $process
         end
      end
   end
end

document noticeaudit
Checks the notices on the process' death notification list.  For each notice verify that the target process still exists.

Syntax: noticeaudit processObj

Examples:
	noticeaudit process
        noticeaudit (CProcess*)0xe04b2ae8
end

define noticebw
   if $argc != 1
      help noticebw
   else
      set $notice = $arg0
      if $notice->eyecatcher_ != 0x4543544e
         printf "Argument 1 (%p) does not specify a valid CNotice object (%d)\n", $arg0, $notice->eyecatcher_
      else
         set $ncount = 0
         while $notice != 0 && $ncount < 50000
            printf "CNotice %p: nid=%d pid=%d process=%d\n", $notice, $notice->Nid, $notice->Pid, $notice->Process
            set $ncount++
            set $notice = $notice->Prev
         end         
      end
   end
end

document noticebw

Displays CNotice object proceeding backward through the list. 

Syntax: noticebw notice


end


#
# The "findnoticetarget" command scans the "process death notice" list for
# a given process.  It attempts to locate a notice for a specific <nid>
# and <pid>.
#
define findnoticetarget
   if $argc != 3
      help findnoticetarget
   else
      set $process = $arg0
      if $process->eyecatcher_ != 0x434f5250
         printf "Argument 1 (%p) does not specify a valid CProcess object (%d)\n", $arg0, $process->eyecatcher_
      else
         printf "For process %p: Nid=%d Pid=%d NoticeHead=%p NoticeTail=%p\n", $process, $process->Nid, $process->Pid, $process->NoticeHead, $process->NoticeTail
         set $notice = $process->NoticeHead
         set $ncount = 0
         while $notice != 0
            set $ncount++
            if $notice->Nid == $arg1 && $notice->Pid == $arg2
               loop_break
            end
         set $notice = $notice->Next
         end
         if $notice != 0
             printf "For process %p: notice %p has target nid=%d pid=%d, position %d\n", $process, $notice, $notice->Nid, $notice->Pid, $ncount
         else
             printf "For process %p: there is no notice with target nid=%d pid=%d\n", $process, $arg1, $arg2
         end
      end
   end
end

document findnoticetarget
Locates a death notice entry on a process' death notification list

Syntax: findnoticetarget processObj

Examples:
	findnoticetarget process 18 9968
        findnoticetarget (CProcess*)0xe04b2ae8 18 9968
end

#
# The "listprocopens" command lists the processes opened by the given
# process.
#
define listprocopens
   if $argc != 1
      help listprocopens
   else
      set $proc = (CProcess * ) ($arg0)
      if $proc->eyecatcher_ != 0x434f5250
         printf "Argument 1 (%p) does not specify a valid CProcess object (%d)\n", $proc, $proc->eyecatcher_
      else
         printf "Opens for %s (%d, %d)\n", $proc->Name, $proc->Nid, $proc->Pid
         set $open = $proc->OpenHead
         while $open != 0
            printf "Opened %s (%d, %d)\n", $open->Name, $open->Nid, $open->Pid
            set $open = $open->Next
         end
      end
   end
end

document listprocopens
Displays the processes opened by the specified process.

Syntax: listprocopens <process addr>

Example:
	listprocopens 0xdfd65778
end


define openbw
   if $argc != 1
      help openbw
   else
      set $open = $arg0
      if $open->eyecatcher_ != 0x4e45504f
         printf "Argument 1 (%p) does not specify a valid COpen object (%d)\n", $arg0, $open->eyecatcher_
      else
         set $ocount = 0
         while $open != 0 && $ocount < 50000
            printf "COpen %p: nid=%d pid=%d process=%s\n", $open, $open->Nid, $open->Pid, $open->Name
            set $ocount++
            set $open = $open->Prev
         end         
      end
   end
end

document openbw
Given a COpen object, follows previous links

Syntax: openbw openObj

Examples:
	openbw process
        openbw (COpen*)0xe04b2ae8
end

define findprocbyopenhead
   if $argc != 2
      help findprocbyopenhead
   else
      if $arg0 < 0 || $arg0 >= Nodes->NumberLNodes
         printf "no such node: %d\n", $arg0
      else
         set $pnode = Nodes->LNode[$arg0].lnodes_->node_
         set $proc = $pnode->CProcessContainer::head_
         set $found = 0
         while $proc != 0
            if $proc->OpenHead == $arg1
               printf "CProcess object for %s (%d, %d) = %p has OpenHead=%p\n", $proc->Name, $arg0, $proc->Pid, $proc, $proc->OpenHead
               set $proc = 0
               set $found = 1
               loop_break
            else
               set $proc = $proc->next_
            end
         end
         if $found == 0
            printf "no process in node %d with OpenHead %p\n", $arg0, $arg1
         end
      end
   end
end

document findprocbyopenhead

Searches all processes on a given node for a CProcess object with the given COpen object as its OpenHead

Syntax: findprocbyopenhead <nid> <open>

Examples:
	findprocbyopenhead 3 0xe04b2ae8
end

# Given a local i/o buffer address compute the buffer number
define liobufnum
   if $argc != 1
      help liobufnum
   else
      if ($arg0 < SQ_theLocalIOToClient->clientBuffers) || ($arg0 >= (SQ_theLocalIOToClient->clientBuffers + SQ_theLocalIOToClient->sharedMemHdrSize_) + SQ_theLocalIOToClient->sharedBuffersMax *  SQ_theLocalIOToClient->sharedBufferSize_) 
         printf "Address %p is not in the local i/o buffer pool range (%p-%p)\n", $arg0, (SQ_theLocalIOToClient->clientBuffers+SQ_theLocalIOToClient->sharedMemHdrSize_), ((SQ_theLocalIOToClient->clientBuffers + SQ_theLocalIOToClient->sharedMemHdrSize_) + SQ_theLocalIOToClient->sharedBuffersMax *  SQ_theLocalIOToClient->sharedBufferSize_ - 1)
      else 
         set $t1 = (($arg0 - (int)SQ_theLocalIOToClient->clientBuffers) - SQ_theLocalIOToClient->sharedMemHdrSize_)/SQ_theLocalIOToClient->sharedBufferSize_
         printf "Address %p is local i/o buffer #%d\n", $arg0, $t1
         if $arg0 != (($t1 * SQ_theLocalIOToClient->sharedBufferSize_) + SQ_theLocalIOToClient->clientBuffers + SQ_theLocalIOToClient->sharedMemHdrSize_)
            printf "Warning: address %p is not the beginning of local i/o buffer #%d\n", $arg0, $t1
         end
      end
   end
end

document liobufnum
Given a local i/o buffer address compute the buffer number

Syntax: liobufnum address

Example:
	liobufnum 0xe3846f88
end

# Given a local io buffer number compute its address
define liobufaddr
   if $argc != 1
      help liobufaddr
   else
      if ($arg0 < 0) || ($arg0 >= SQ_theLocalIOToClient->sharedBuffersMax)
         printf "Buffer number %d is out of range (0-%d)\n", $arg0, (SQ_theLocalIOToClient->sharedBuffersMax-1)
      else
         printf "Local i/o buffer #%d is at address %p.\n", $arg0, (($arg0 * SQ_theLocalIOToClient->sharedBufferSize_) + SQ_theLocalIOToClient->clientBuffers + SQ_theLocalIOToClient->sharedMemHdrSize_)
      end
   end
end

document liobufaddr
Given a local i/o buffer number compute the buffer address

Syntax: liobufaddr number

Example:
	liobufaddr 137
end


#
# The "scan_liobuf" command displays information about each in-use local-io
# buffer.
#
define scan_liobuf
  set $inx = -1
  set $maxbufs = SQ_theLocalIOToClient->sharedBuffersMax
  set $clientbufs = SQ_theLocalIOToClient->clientBuffers+SQ_theLocalIOToClient->sharedMemHdrSize_
  while (++$inx < $maxbufs)
    set $liop = $clientbufs + $inx * sizeof(SharedMsgDef)
    if (((SharedMsgDef *)$liop)->trailer.bufInUse != 0)
      printf "lio buf #%d, owner=%d, timestamp=(%d,%d)\n", $inx, ((SharedMsgDef *)$liop)->trailer.OSPid, ((SharedMsgDef *)$liop)->trailer.timestamp.tv_sec, ((SharedMsgDef *)$liop)->trailer.timestamp.tv_nsec
# if notice print type
    end
  end
  printf "\n"
end

document scan_liobuf
   Displays information about each in-use local io buffer
end



#
# The "mon_stats" command displays some monitor local-io statistics.
#
define mon_stats
   printf "acquiredBufferCount= %ld\n", SQ_theLocalIOToClient->acquiredBufferCount
   printf "acquiredBufferCountMax = %ld\n", SQ_theLocalIOToClient->acquiredBufferCountMax
   printf "availableBufferCountMin = %ld\n", SQ_theLocalIOToClient->availableBufferCountMin
   printf "missedBufferCount = %ld\n", SQ_theLocalIOToClient->missedBufferCount
   printf "sharedBuffersMax = %ld\n", SQ_theLocalIOToClient->sharedBuffersMax
end

document mon_stats
   Displays monitor local io buffer statistics

   Syntax:  mon_stats

end

