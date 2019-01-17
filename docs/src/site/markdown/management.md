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
This page describes how you manage Trafodion.

# Management Scripts
The following table provides an overview of the different Trafodion management scripts.

Component                             | Start                | Stop                | Status
--------------------------------------|----------------------|---------------------|---------------------
All of Trafodion                      | **```sqstart```**    | **```sqstop```**    | **```sqcheck```**
DCS (Database Connectivity Services)  | **```dcsstart```**   | **```dcsstop```**   | **```dcscheck```**
REST Server                           | **```reststart```**  | **```reststop```**  | —
LOB Server                            | **```lobstart```**   | **```lobstop```**   | —
RMS Server                            | **```rmsstart```**   | **```rmsstop```**   | **```rmscheck```**  

**Example: Start Trafodion**

    cd $TRAF_HOME/sql/scripts
    sqstart
    sqcheck

# Validate Trafodion Installation
You can use **```sqlci```** or **```trafci```** (connects via DCS) to validate your installation.

    get schemas;
    create table table1 (a int);
    invoke table1;
    insert into table1 values (1), (2), (3), (4);
    select * from table1;
    exit;

Assuming no errors, your installation has been successful. You can start working on your modifications.

# Troubleshooting Tips
If you are not able to start up the environment or if there are problems running **```sqlci```** or **```trafci```**, then verify that the all the processes are up and running.

* **```swstatus```** should show at 6 java servers and 2 mysql processes. 
* **```sqcheck```** should indicate all processes are running. 

If processes are not running as expected, then:

* **```sqstop```** to shut down Traodion. If some Trafodion processes do not terminate cleanly, then run **```ckillall```**.
* **```swstopall```** to shut down the Hadoop ecosystem.
* **```swstartall```** to restart the Hadoop ecosystem.
* **```sqstart```** to restart Trafodion.

If problems persist please review logs:

* **```$TRAF_HOME/sql/local_hadoop/\*/log```**: Hadoop, HBase, and Hive logs.
* **```$TRAF_LOG```**: Trafodion logs.
