Compiling the Java files
========================
On Windows Platform:
%JAVA_HOME%\bin\javac -classpath ..\..\lib\jdbcT4.jar *.java ..\common\*.java

On Linux:
$JAVA_HOME/bin/javac -classpath ../../lib/jdbcT4.jar *.java ../common/*.java

Note: Make sure there are no compilation errors displayed on
      the screen.


Executing BatchSample
=========================
On Windows Platform:
%JAVA_HOME%\bin\java -classpath ..\..\lib\jdbcT4.jar;..;. -Dt4jdbc.properties=..\t4jdbc.properties BatchSample

On Linux:
$JAVA_HOME/bin/java -classpath ../../lib/jdbcT4.jar:..:. -Dt4jdbc.properties=../t4jdbc.properties BatchSample


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
Column 1 - 0,0
Column 2 - BatchS1             ,BatchS1             
Column 3 - 0,0

Printing Row 2 using getString(), getObject()
Column 1 - 1,1
Column 2 - BatchS1             ,BatchS1             
Column 3 - 1,1

Printing Row 3 using getString(), getObject()
Column 1 - 2,2
Column 2 - BatchS1             ,BatchS1             
Column 3 - 2,2

Printing Row 4 using getString(), getObject()
Column 1 - 3,3
Column 2 - BatchS1             ,BatchS1             
Column 3 - 3,3

Printing Row 5 using getString(), getObject()
Column 1 - 4,4
Column 2 - BatchS1             ,BatchS1             
Column 3 - 4,4

Printing Row 6 using getString(), getObject()
Column 1 - 5,5
Column 2 - BatchS1             ,BatchS1             
Column 3 - 5,5

Printing Row 7 using getString(), getObject()
Column 1 - 6,6
Column 2 - BatchS1             ,BatchS1             
Column 3 - 6,6

Printing Row 8 using getString(), getObject()
Column 1 - 7,7
Column 2 - BatchS1             ,BatchS1             
Column 3 - 7,7

Printing Row 9 using getString(), getObject()
Column 1 - 8,8
Column 2 - BatchS1             ,BatchS1             
Column 3 - 8,8

Printing Row 10 using getString(), getObject()
Column 1 - 9,9
Column 2 - BatchS1             ,BatchS1             
Column 3 - 9,9

End of Data

Printing ResultSetMetaData ...
No. of Columns 3
Column 1 Data Type: INTEGER Name: C1
Column 2 Data Type: CHAR Name: C2
Column 3 Data Type: INTEGER Name: C3

Fetching rows...

Printing Row 1 using getString(), getObject()
Column 1 - 0,0
Column 2 - BatchPS0            ,BatchPS0            
Column 3 - 0,0

Printing Row 2 using getString(), getObject()
Column 1 - 1,1
Column 2 - BatchPS1            ,BatchPS1            
Column 3 - 11,11

Printing Row 3 using getString(), getObject()
Column 1 - 2,2
Column 2 - BatchPS2            ,BatchPS2            
Column 3 - 22,22

Printing Row 4 using getString(), getObject()
Column 1 - 3,3
Column 2 - BatchPS3            ,BatchPS3            
Column 3 - 33,33

Printing Row 5 using getString(), getObject()
Column 1 - 4,4
Column 2 - BatchPS4            ,BatchPS4            
Column 3 - 44,44

Printing Row 6 using getString(), getObject()
Column 1 - 5,5
Column 2 - BatchPS5            ,BatchPS5            
Column 3 - 55,55

Printing Row 7 using getString(), getObject()
Column 1 - 6,6
Column 2 - BatchPS6            ,BatchPS6            
Column 3 - 66,66

Printing Row 8 using getString(), getObject()
Column 1 - 7,7
Column 2 - BatchPS7            ,BatchPS7            
Column 3 - 77,77

Printing Row 9 using getString(), getObject()
Column 1 - 8,8
Column 2 - BatchPS8            ,BatchPS8            
Column 3 - 88,88

Printing Row 10 using getString(), getObject()
Column 1 - 9,9
Column 2 - BatchPS9            ,BatchPS9            
Column 3 - 99,99

End of Data