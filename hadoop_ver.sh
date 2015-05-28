#!/bin/bash

# report version number of hadoop components

if [[ $1 == "hadoop" ]]
then
  hadoop version | sed -n 's/^.*Hadoop \(.*\)$/\1/p'
elif [[ $1 == "hbase" ]]
then
  hbase version 2>&1 | sed -n 's/^.*HBase \(.*\)$/\1/p'
elif [[ $1 == "hive" ]]
then
  hive --version 2>&1 | sed -n 's/^.*Hive \(.*\)$/\1/p'
elif [[ $1 == "zookeeper" ]]
then
   # zookeeper does not report full version number
   #echo status | nc localhost 2181 | sed -n 's/^.*Zookeeper version: \(.*\)$/\1/p'
   # so go back to rpm package
   rpm -qa | grep -E '^zookeeper' | sed -n 's/^[^-]*-\(.*\).el6.*$/\1/p'
fi
exit 0
