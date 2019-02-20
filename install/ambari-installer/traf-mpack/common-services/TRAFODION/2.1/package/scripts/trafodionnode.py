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
import sys, os, pwd, signal, time
from resource_management import *

class Node(Script):
  def install(self, env):
  
    # Install packages listed in metainfo.xml
    self.install_packages(env)
  
    import params
    Directory(params.traf_conf_dir, 
              mode=0755, 
              owner = params.traf_user, 
              group = params.traf_group, 
              create_parents = True)
    cmd = "source /etc/trafodion/trafodion_config 2>/dev/null; cp -rf $TRAF_HOME/conf/* %s/"  % params.traf_conf_dir
    Execute(cmd,user=params.traf_user)

    # cluster file will be over-written by trafodionmaster install
    # until then, make file that shell can source without error
    traf_conf_path = os.path.join(params.traf_conf_dir, "traf-cluster-env.sh")
    File(traf_conf_path,
         owner = params.traf_user, 
         group = params.traf_group, 
         content="# place-holder\n",
         mode=0644)

    self.configure(env)

  def configure(self, env):
    import params

    ##################
    # trafodion cluster-wide ssh config
    trafhome = os.path.expanduser("~" + params.traf_user)
    Directory(os.path.join(trafhome,".ssh"), 
              mode=0700,
              owner = params.traf_user, 
              group = params.traf_group)
    # private key generated on ambari server
    File(os.path.join(trafhome,".ssh/id_rsa"),
         owner = params.traf_user, 
         group = params.traf_group, 
         content=params.traf_priv_key,
         mode=0600)

    # log and tmp dirs
    Directory(params.traf_logdir, 
              mode=0755, 
              owner = params.traf_user, 
              group = params.traf_group, 
              create_parents = True)
    Directory(params.traf_vardir, 
              mode=0755, 
              owner = params.traf_user, 
              group = params.traf_group, 
              create_parents = True)

    # generate public key from the private one
    cmd = "ssh-keygen -y -f " + trafhome + "/.ssh/id_rsa > " + trafhome + "/.ssh/id_rsa.pub"
    Execute(cmd,user=params.traf_user)
    cmd = "cat " + trafhome + "/.ssh/id_rsa.pub >> " + trafhome + "/.ssh/authorized_keys"
    Execute(cmd,user=params.traf_user)
    cmd = "chmod 0600 " + trafhome + "/.ssh/authorized_keys"
    Execute(cmd,user=params.traf_user)
    sshopt = format('Host *\n' + '    StrictHostKeyChecking=no\n')
    File(os.path.join(trafhome,".ssh/config"),
         owner = params.traf_user, 
         group = params.traf_group, 
         content=sshopt,
         mode=0600)

    ##################
    # create env files
    env.set_params(params)
    traf_conf_path = os.path.join(params.traf_conf_dir, "trafodion-env.sh")
    File(traf_conf_path,
         owner = params.traf_user, 
         group = params.traf_group, 
         content=InlineTemplate(params.traf_env_template,trim_blocks=False),
         mode=0644)
    # initialize & verify env
    cmd = "source ~/.bashrc"
    Execute(cmd,user=params.traf_user)

    ##################
    # Link TRX files into HBase lib dir
    hlib = "/usr/hdp/current/hbase-regionserver/lib/"
    trx = "$TRAF_HOME/export/lib/hbase-trx-hdp2_3-${TRAFODION_VER}.jar"
    util = "$TRAF_HOME/export/lib/trafodion-utility-${TRAFODION_VER}.jar"

    # run as root, but expand variables using trafodion env
    # must be after trafodion user already initializes bashrc
    cmd = "source ~" + params.traf_user + "/.bashrc ; ln -f -s " + trx + " " + hlib
    Execute(cmd)
    cmd = "source ~" + params.traf_user + "/.bashrc ; ln -f -s " + util + " " + hlib
    Execute(cmd)

    ##################
    # Make sure our JVM files are owned by us
    if os.path.exists("/tmp/hsperfdata_%s" % params.traf_user):
      cmd = "chown -R %s:%s /tmp/hsperfdata_%s" % (params.traf_user, params.traf_group, params.traf_user)
      Execute(cmd)


    ##################
    # LDAP config
    # In future, should move to traf_conf_dir
    if params.traf_ldap_enabled == 'YES':
      File(os.path.join(trafhome,".traf_authentication_config"),
           owner = params.traf_user, 
           group = params.traf_group, 
           content = InlineTemplate(params.traf_ldap_template),
           mode=0750)
      cmd = "source ~/.bashrc ; mv -f ~/.traf_authentication_config $TRAF_CONF/"
      Execute(cmd,user=params.traf_user)
      cmd = "source ~/.bashrc ; ldapconfigcheck -file $TRAF_CONF/.traf_authentication_config"
      Execute(cmd,user=params.traf_user)
      cmd = 'source ~/.bashrc ; ldapcheck --verbose --username=%s' % params.traf_db_admin
      Execute(cmd,user=params.traf_user)

    ##################
    # All Trafodion Nodes need DCS config files
    # In future, should move DCS conf to traf_conf_dir
    File(os.path.join(trafhome,"dcs-env.sh"),
         owner = params.traf_user, 
         group = params.traf_group, 
         content = InlineTemplate(params.dcs_env_template),
         mode=0644)
    File(os.path.join(trafhome,"log4j.properties"),
         owner = params.traf_user, 
         group = params.traf_group, 
         content = InlineTemplate(params.dcs_log4j_template),
         mode=0644)

    serverlist = '\n'.join(params.dcs_mast_node_list) + '\n'
    File(os.path.join(trafhome,"masters"),
         owner = params.traf_user, 
         group = params.traf_group, 
         content = serverlist,
         mode=0644)

    serverlist = ''
    node_cnt = len(params.traf_node_list)
    per_node = int(params.dcs_servers) // node_cnt
    extra = int(params.dcs_servers) % node_cnt
    for nnum, node in enumerate(params.traf_node_list, start=0):
      if nnum < extra:
         serverlist += '%s %s\n' % (node, per_node + 1)
      else:
         serverlist += '%s %s\n' % (node, per_node)
    File(os.path.join(trafhome,"servers"),
         owner = params.traf_user, 
         group = params.traf_group, 
         content = serverlist,
         mode=0644)
    XmlConfig("dcs-site.xml",
              conf_dir=trafhome,
              configurations=params.config['configurations']['dcs-site'],
              owner=params.traf_user,
              mode=0644)
    # install DCS conf files
    cmd = "source ~/.bashrc ; mv -f ~/dcs-env.sh ~/log4j.properties ~/dcs-site.xml ~/masters ~/servers $TRAF_CONF/dcs/"
    Execute(cmd,user=params.traf_user)

    XmlConfig("rest-site.xml",
              conf_dir=trafhome,
              configurations=params.config['configurations']['rest-site'],
              owner=params.traf_user,
              mode=0644)
    # install REST conf files
    cmd = "source ~/.bashrc ; mv -f ~/rest-site.xml $TRAF_CONF/rest/"
    Execute(cmd,user=params.traf_user)



    ##################
    # create trafodion scratch dirs
    for sdir in params.traf_scratch.split(','):
      Directory(sdir,
                mode=0777,
                owner = params.traf_user,
                group = params.traf_group,
                create_parents = True)
 

  # Master component does real stop/start for cluster, 
  #   but for ambari restart, provide expected status
  def stop(self, env):
    import status_params
    Execute('touch ~/ambari_node_stop',user=status_params.traf_user)
    return True

  def start(self, env):
    import status_params
    Execute('rm -f ~/ambari_node_stop',user=status_params.traf_user)
    self.configure(env)
    return True

  def status(self, env):
    import status_params
    try:
      Execute('ls ~/ambari_node_stop && exit 1 || exit 0',user=status_params.traf_user)
    except:
      raise ComponentIsNotRunning()
    try:
      Execute('source ~/.bashrc ; sqshell -c node info | grep $(hostname) | grep -q Up',user=status_params.traf_user)
    except:
      raise ComponentIsNotRunning()

if __name__ == "__main__":
  Node().execute()
