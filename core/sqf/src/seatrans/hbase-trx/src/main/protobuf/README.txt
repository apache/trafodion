This directory contains the protobuf definition files used by the Trafodion 
transaction manager.  The compiled java classes will be generated into: 
src/main/java/org/apache/hadoop/hbase/coprocessor/transactional/generated/
The compilation of the protobuf files is a separate step as they are not changed often. 

Version 2.5 of the Protocol Buffer Compilation tool is required. 

To compile, run the following maven command in the hbase-trx base directory:

mvn compile -Dcompile-protobuf

To define the path to the protoc binary use:

mvn compile -Dcompile-protobuf -Dprotoc.path=/opt/local/bin/protoc
