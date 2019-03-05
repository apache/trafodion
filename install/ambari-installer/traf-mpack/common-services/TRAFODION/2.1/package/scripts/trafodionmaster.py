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
import sys, os
from resource_management import *
from tempfile import TemporaryFile

class Master(Script):
  def install(self, env):
  
    # Install packages listed in metainfo.xml
    self.install_packages(env)
    self.configure(env)

  def configure(self, env):
    import params

    # generate sqconfig file
    cmd = "lscpu|grep -E '(^CPU\(s\)|^Socket\(s\))'|awk '{print $2}'"
    ofile = TemporaryFile()
    Execute(cmd,stdout=ofile)
    ofile.seek(0) # read from beginning
    core, processor = ofile.read().split('\n')[:2]
    ofile.close()

    core = int(core)-1 if int(core) <= 256 else 255

    lines = ['begin node\n']
    loc_node_list = []
    for node_id, node in enumerate(params.traf_node_list):
        # find the local hostname for each node
        cmd = "ssh -q %s hostname" % node
        ofile = TemporaryFile()
        Execute(cmd,user=params.traf_user,stdout=ofile)
        ofile.seek(0) # read from beginning
        localhn = ofile.readline().rstrip()
        ofile.close()
        cmd = "ssh %s 'echo success'" % localhn
        Execute(cmd,user=params.traf_user) # verify we can use this hostname to communicate
 
        line = 'node-id=%s;node-name=%s;cores=0-%d;processors=%s;roles=connection,aggregation,storage\n' \
                 % (node_id, localhn, core, processor)
        lines.append(line)
        loc_node_list.append(localhn)

    lines.append('end node\n')
    lines.append('\n')
    lines.append('begin overflow\n')
    for scratch_loc in params.traf_scratch.split(','):
        line = 'hdd %s\n' % scratch_loc
        lines.append(line)
    lines.append('end overflow\n')

    # write sqconfig in trafodion home dir
    trafhome = os.path.expanduser("~" + params.traf_user)
    File(os.path.join(trafhome,"sqconfig"),
         owner = params.traf_user,
         group = params.traf_user,
         content=''.join(lines),
         mode=0644)

    # install sqconfig
    Execute('source ~/.bashrc ; mv -f ~/sqconfig $TRAF_CONF/',user=params.traf_user)

    # write cluster-env in trafodion home dir
    traf_nodes = ' '.join(loc_node_list)
    traf_w_nodes = '-w ' + ' -w '.join(loc_node_list)
    traf_node_count = len(loc_node_list)
    if traf_node_count != len(params.traf_node_list):
      print "Error cannot determine local hostname for all Trafodion nodes"
      exit(1)

    cl_env_temp = os.path.join(trafhome,"traf-cluster-env.sh")
    File(cl_env_temp,
         owner = params.traf_user,
         group = params.traf_user,
         content=InlineTemplate(params.traf_clust_template,
                                traf_nodes=traf_nodes,
                                traf_w_nodes=traf_w_nodes,
                                traf_node_count=traf_node_count,
                                cluster_name=params.cluster_name),
         mode=0644)

    # install cluster-env on all nodes
    for node in params.traf_node_list:
        cmd = "scp %s %s:%s/" % (cl_env_temp, node, params.traf_conf_dir)
        Execute(cmd,user=params.traf_user)
    cmd = "rm -f %s" % (cl_env_temp)
    Execute(cmd,user=params.traf_user)

    # Execute SQ gen
    Execute('source ~/.bashrc ; rm -f $TRAF_VAR/sqconfig.db; sqgen',user=params.traf_user)


  #To stop the service, use the linux service stop command and pipe output to log file
  def stop(self, env):
    import params
    Execute('source ~/.bashrc ; sqstop',user=params.traf_user)

  #To start the service, use the linux service start command and pipe output to log file      
  def start(self, env):
    import params
    self.configure(env)

    # Check HDFS set up
    # Must be in start section, since we need HDFS running
    params.HdfsDirectory("/hbase/archive",
                         action="create_on_execute",
                         owner=params.hbase_user,
                         group=params.hbase_user,
                        )
    params.HdfsDirectory(params.hbase_staging,
                         action="create_on_execute",
                         owner=params.hbase_user,
                         group=params.hbase_user,
                        )
    params.HdfsDirectory("/user/trafodion",
                         action="create_on_execute",
                         owner=params.traf_user,
                         group=params.traf_group,
                         mode=0755,
                        )
    params.HdfsDirectory("/user/trafodion/trafodion_backups",
                         action="create_on_execute",
                         owner=params.traf_user,
                         group=params.traf_group,
                        )
    params.HdfsDirectory("/user/trafodion/bulkload",
                         action="create_on_execute",
                         owner=params.traf_user,
                         group=params.user_group,
                         mode=0750,
                        )
    params.HdfsDirectory("/user/trafodion/lobs",
                         action="create_on_execute",
                         owner=params.traf_user,
                         group=params.traf_group,
                        )
    params.HdfsDirectory(None, action="execute")

    try:
      cmd = "hdfs dfs -setfacl -R -m user:%s:rwx,default:user:%s:rwx,mask::rwx /hbase/archive" % \
               (params.traf_user, params.traf_user)
      Execute(cmd,user=params.hdfs_user)
    except:
      print "Error: HDFS ACLs must be enabled for config of hdfs:/hbase/archive"
      print "       Re-start HDFS, HBase, and other affected components before starting Trafodion"
      raise Fail("Need HDFS component re-start")

    # Start trafodion
    Execute('source ~/.bashrc ; sqstart',user=params.traf_user,logoutput=True)
	
  def status(self, env):
    import status_params
    try:
       Execute('source ~/.bashrc ; sqshell -c node info | grep $(hostname) | grep -q Up',user=status_params.traf_user)
    except:
       raise ComponentIsNotRunning()
 
  def initialize(self, env):
    import params
    cmd = "source ~/.bashrc ; echo 'initialize Trafodion;' | sqlci"
    ofile = TemporaryFile()
    Execute(cmd,user=params.traf_user,stdout=ofile,stderr=ofile,logoutput=True)
    ofile.seek(0) # read from beginning
    output = ofile.read()
    ofile.close()

    if (output.find('1395') >= 0 or output.find('1392') >= 0):
      print output + '\n'
      print "Re-trying initialize as upgrade\n"
      cmd = "source ~/.bashrc ; echo 'initialize Trafodion, upgrade;' | sqlci"
      Execute(cmd,user=params.traf_user,logoutput=True)


if __name__ == "__main__":
  Master().execute()
