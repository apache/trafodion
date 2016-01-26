<!--
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at
 
      http://www.apache.org/licenses/LICENSE-2.0
 
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the 
  License.
-->
This page describes how to setup passwordless ssh for a Trafodion user.

# Overview
Please use these commands to enable passwordless ssh access for a user ID; for example: **```trafodion```**

You need to define the authorization keys on each node on the instance. To simplify this procedure, this procedure starts from scratch; that is, it removes the **```ssh```** configuration so that new files can be created. The procedure on this page defines what you need to do to set up passwordless ssh manually:

* Run all commands from a single node in the instance. This node is labeled **master node** in the procedure.
* The commands will reach out to all other nodes in the instance. You will have to provide the correct password manually until the procedure has been completed.
* Ensure that you run the validation step. If passwordless ssh isn't working, then you likely missed a step in the procedure; you'll have to start over. 

<table>
  <tr>
     <td><strong>Note</strong> <br /><br />You run these commands for each node in the instance, including the master node; that is, you run these commands as well if you are on a single-node instance.
     </td>
  </tr>
</table>

In this procedure, you perform the following steps: 

* Generate the **```~/.ssh/id_dsa.pub```** file.
* Populate the local **```~/.ssh/authorized_keys```** file. 
* Copy files to all nodes in the instance.
* Add **```localhost```** key to each node's **```~/.ssh/known_hosts```** file.
* Turn on **```NoHostAuthenticationForLocalhost```** on each node.
* Secure the files for passwordless ssh access. 
* Validate passwordless ssh access. 

The result of the above procedure should be that the content of the **```~/.ssh/authorized_keys```** file is the same on all nodes in your instance and that the security is set up the same way. This allows you to use ssh to login in to any node from any node in the instance without having to specify a password. This is key for many scripts and support commands, which assume that passwordless ssh is working properly. For example, most support commands uses the **```pdsh $MY_NODES```** or **```pdsh $MY_HADOOP_NODES```** command for status checks.

# Command Summary
A summary of the commands used in this procedures to help experienced users:

    # Skip this step if logged in as a user that has run EsgynDB sqenv.sh, which defines NODE_LIST.
    # Modify the NODE_LIST content to list the nodes on which you are installing Trafodion.
    export NODE_LIST="node01.host.com node02.host.com node03.host.com node04.host.com node05.host.com"
    
    # Get the user ID we're doing this work for. Makes the rest of the commands cleaner.
    user=$(whoami)

    # Generate ~/.ssh/id_dsa.pub on each node in the instance
    for node in $NODE_LIST; do ssh $user@$node "rm -rf ~/.ssh; mkdir ~/.ssh; cd ~/.ssh; echo -e 'y\n' | ssh-keygen -t rsa -N '' -f $~/.ssh/id_rsa"; done

    # Populate ~/.ssh/authorized_keys file on the master node.
    for node in $NODE_LIST; do ssh $user@$node "cat ~/.ssh/id_dsa.pub" >> ~/.ssh/authorized_keys; done
    
    # Copy files to all nodes in the instance but the master node.
    my_node=$(echo $HOSTNAME | cut -d '.' -f 1)
    for node in $NODE_LIST; do if [ $node <> $my_node ]; then scp ~/.ssh/authorized_keys ~/.ssh/known_hosts $user@$node:~/.ssh/.; fi; done
    
    # Add localhost public key to each node's ~/.ssh/known_hosts file.
    for node in $NODE_LIST; do ssh $user@$node "echo localhost $(cat /etc/ssh/ssh_host_rsa_key.pub) >> ~/.ssh/known_hosts"; done
    
    # Turn on NoHostAuthenticationForLocalhost on each node.
    for node in $NODE_LIST; do ssh $user@$node "echo "NoHostAuthenticationForLocalhost=yes" >> ~/.ssh/config"; done
    
    # Change security of ~/.ssh directory and key files.
    for node in $NODE_LIST; do ssh $user@$node "chmod 755 ~/.ssh; chmod 600 ~/.ssh/authorized_keys; cd ~/.ssh; chmod 700 .."; done

    # Validate that passwordless ssh works. No password prompts should be seen. Try from all or some nodes in the instance.
    for node in $NODE_LIST; do ssh $user@$node "ls -al /etc/passwd"; done


# Detailed Commands

## Define ```NODE_LIST```
The steps below assumes that the **```NODE_LIST```** environmental variable contains the node names onto which Trafodion should be installed. You populate this variable using one of the following methods:

1. Invoke the Trafodion **```sqenv.sh```** script, which is present after Trafodion has been installed.
2. Create **```NODE_LIST```** manually; for example:

    export NODE_LIST="node01.host.com node02.host.com node03.host.com node04.host.com node05.host.com"

## Generate the ```~/.ssh/id_dsa.pub file``` 
In this step, you create the authorization key for each node in the instance. This key will be located in the **```~/.ssh/id_dsa.pub```** file. Use the following commands for each node of the instance: 

    $ user=$(whoami)
    $ for node in $NODE_LIST; do ssh $user@$node "rm -rf ~/.ssh; mkdir ~/.ssh; cd ~/.ssh; echo -e 'y\n' | ssh-keygen -t rsa -N '' -f $~/.ssh/id_rsa"; done

## Populate the local ```~/.ssh/authorized_keys file```
In this step, you populate the **```~/.ssh/authorized_keys```** file on master node by copying the content from each node's **```~/.ssh/id_dsa.pub```** file. 

Use the following command string to copy the content of **```~/.ssh/id_dsa.pub```** file for each node into the **```~/.ssh/authorized_keys```** on the master node:

    $ for node in $NODE_LIST; do ssh $user@$node "cat ~/.ssh/id_dsa.pub" >> ~/.ssh/authorized_keys; done

## Copy Files to All Nodes in the Instance
In this step, you copy the **```~/.ssh/authorized_keys```** and **```~/.ssh/known_host```** files to all other nodes in the instance. Use the following commands to copy the files to all nodes in the instance '''except''' the master node: 

    $ my_node=$(echo $HOSTNAME | cut -d '.' -f 1) 
    $ for node in $NODE_LIST; do if [ $node <> $my_node ]; then scp ~/.ssh/authorized_keys ~/.ssh/known_hosts $user@$node:~/.ssh/.; fi; done

## Add ```localhost``` key to each node's ```~/.ssh/known_hosts file```
In this step, you add the public key for **```localhost```** to the **```~/.ssh/known_hosts```** file

    $ for node in $NODE_LIST; do ssh $user@$node "echo localhost $(cat /etc/ssh/ssh_host_rsa_key.pub) >> ~/.ssh/known_hosts"; done

## Turn on ```NoHostAuthenticationForLocalhost```
In this step, you turn the **```NoHostAuthenticationForLocalhost```** on in each node in the instance.

    $ for node in $NODE_LIST; do ssh $user@$node "echo "NoHostAuthenticationForLocalhost=yes" >> ~/.ssh/config"; done

## Secure the Files for Passwordless ```ssh``` Access
In this step, you change the security of the **```~/.ssh/authorized_keys```** file and the **```~/.ssh```** directory so that passwordless ssh will be allowed. Use the following commands for each node of the instance: 

    $ for node in $NODE_LIST; do ssh $user@$node "chmod 755 ~/.ssh; chmod 600 ~/.ssh/authorized_keys; cd ~/.ssh; chmod 700 .."; done

## Validate Passwordless ssh Access 
Test by starting a remote shell on the different nodes in the instance — no password should be necessary.  Be sure to check **```ssh```** access to the node itself as well. Use the following command string to check that you can reach each node in the instance without having to provide a password: 

    $ for node in $NODE_LIST; do ssh $user@$node "ls -al /etc/passwd"; done

Try the validation command string from all or some nodes in the instance.

# Additional Information
Passwordless ssh isn't considered secure in some installations. We don't have a solution for this situation yet.

