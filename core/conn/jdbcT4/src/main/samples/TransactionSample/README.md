Compiling the Java files
========================
On Windows Platform:
%JAVA_HOME%\bin\javac -classpath ..\..\lib\jdbcT4.jar *.java ..\common\*.java

On Linux:
$JAVA_HOME/bin/javac -classpath ../../lib/jdbcT4.jar *.java ../common/*.java

Note: Make sure there are no compilation errors displayed on
      the screen.


Executing TransactionSample
=========================
On Windows Platform:
%JAVA_HOME%\bin\java -classpath ..\..\lib\jdbcT4.jar;..;. -Dt4jdbc.properties=..\t4jdbc.properties TransactionSample

On Linux:
$JAVA_HOME/bin/java -classpath ../../lib/jdbcT4.jar:..:. -Dt4jdbc.properties=../t4jdbc.properties TransactionSample


Output of the execution would look like:
========================================

<DATE, TIME> common.sampleUtils getPropertiesConnection
INFO: DriverManager.getConnection(url, props) passed

Printing ResultSetMetaData ...
No. of Columns 3
Column 1 Data Type: INTEGER Name: C1
Column 2 Data Type: CHAR Name: C2
Column 3 Data Type: INTEGER Name: C3

Fetching rows...

Printing Row 1 using getString(), getObject()
Column 1 - 1,1
Column 2 - row1                ,row1                
Column 3 - 11,11

Printing Row 2 using getString(), getObject()
Column 1 - 2,2
Column 2 - row2                ,row2                
Column 3 - 22,22

End of Data
Rolling back data here....
*** ERROR[15001] A syntax error occurred at or before: 
insert in TransactionSample values(4,'row2',44);
        ^ (9 characters from start of SQL statement) [2017-11-22 07:47:01]
SQLState   42000
Error Code -15001
*** ERROR[8822] The statement was not prepared. [2017-11-22 07:47:01]
SQLState   X08MU
Error Code -8822
