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
##############################################################################
#
# An AWK script to find ports in the output of command lsof -i. 
#
##############################################################################
#
#
#
# For the following ouput, 
# 
# COMMAND     PID    USER   FD   TYPE    DEVICE SIZE/OFF NODE NAME
# java        372 zellerh  325u  IPv6 539757798      0t0  TCP localhost:41761->localhost:eforward (ESTABLISHED)
# java        372 zellerh  330u  IPv6 539758057      0t0  TCP *:4200 (LISTEN)
# master     2354    root   12u  IPv4     38168      0t0  TCP localhost:smtp (LISTEN)
# master     2354    root   13u  IPv6     38170      0t0  TCP localhost:smtp (LISTEN)
# dhclient   4620    root    6u  IPv4   2628139      0t0  UDP *:bootpc
# sshd      10514    root    3r  IPv4 574815707      0t0  TCP adev04.esgyn.com:ssh->cpe-24-27-32-126.austin.res.rr.com:59216 (ESTABLISHED)
# sshd      10535     qfc    3u  IPv4 574815707      0t0  TCP adev04.esgyn.com:ssh->cpe-24-27-32-126.austin.res.rr.com:59216 (ESTABLISHED)
# sshd      10714    root    3r  IPv4 574816896      0t0  TCP adev04.esgyn.com:ssh->cpe-24-27-32-126.austin.res.rr.com:59217 (ESTABLISHED)
# sshd      10735     qfc    3u  IPv4 574816896      0t0  TCP adev04.esgyn.com:ssh->cpe-24-27-32-126.austin.res.rr.com:59217 (ESTABLISHED)
# sshd      13507    root    3r  IPv4 569638562      0t0  TCP adev04.esgyn.com:ssh->76-244-44-66.lightspeed.sntcca.sbcglobal.net:55554 (ESTABLISHED)
# sshd      13540 hegdean    3u  IPv4 569638562      0t0  TCP adev04.esgyn.com:ssh->76-244-44-66.lightspeed.sntcca.sbcglobal.net:55554 (ESTABLISHED)
# python2.6 14346 hegdean    4u  IPv4 552343030      0t0  TCP adev04.esgyn.com:46083 (LISTEN)
# Xvnc-core 15343     qfc    0u  IPv6 449977085      0t0  TCP *:6049 (LISTEN)
# Xvnc-core 15343     qfc    1u  IPv4 449977086      0t0  TCP *:6049 (LISTEN)
#
#
# the script will identify and print out following port numbers.
#
#41761
#4200 
#59216 
#59216 
#59217 
#59217 
#55554 
#55554 
#46083 
#6049 
#6049 



# find port in string of form <do-not-care>:[0-9]{4,5}<do-not-care>
function reportPort(val)
{
   parts = split(val, iparray ,":"); 

   if ( parts > 1 ) {
#      printf "%s\n", iparray[2]
      # get the port number after ':'
      ippos = match(iparray[2], "[0-9][0-9][0-9][0-9]*");
      nonip_pos = match(iparray[2], "[^0-9]");

      if ( ippos > 0 ) {
        if ( nonip_pos > 0 )
          ip=substr(iparray[2], ippos, nonip_pos - ippos + 1);
        else
          ip=substr(iparray[2], ippos);

        printf "%s\n", ip;
     }
   }
}

{
   x=split($0,a,"TCP"); 

   # Identify <string-may-contain-ip> in string of form 
   # <do-not-care>TCP<string-may-cpontain-ip>
   if ( x > 1 ) {

      #<string-may-cpontain-ip> may be one of the following two forms:
      # <ip-string> -> <ip-string>, or
      # <ip-string>
      #
      # <ip-string> is something like <do-not-care>:<port><do-not-care> 
      # <port> is [0-9]{4,}
      #
      # check if exists ->
      y = split(a[2],b,"->"); 

      if (y>1) {
        reportPort(b[1]);
        reportPort(b[2]);
      } else
        reportPort(a[2]);
   }
}
