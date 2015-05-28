
To build:
>cd <your path to DCS>
>mvn clean site package
[INFO] Scanning for projects...
[INFO]
[INFO] ------------------------------------------------------------------------
[INFO] Building dcs 1.0.0
[INFO] ------------------------------------------------------------------------
[INFO]
[INFO] --- maven-clean-plugin:2.4.1:clean (default-clean) @ dcs ---
[INFO]
[INFO] --- maven-resources-plugin:2.5:resources (default-resources) @ dcs ---
[debug] execute contextualize
[INFO] Using 'UTF-8' encoding to copy filtered resources.
[INFO] Copying 1 resource
[INFO]
[INFO] --- maven-compiler-plugin:2.3.2:compile (default-compile) @ dcs ---
[INFO] Compiling 50 source files to /target/classes
[INFO]
[INFO] --- maven-resources-plugin:2.5:testResources (default-testResources) @ dcs ---
[debug] execute contextualize
[INFO] Using 'UTF-8' encoding to copy filtered resources.
[INFO] Copying 1 resource
[INFO]
[INFO] --- maven-compiler-plugin:2.3.2:testCompile (default-testCompile) @ dcs ---
[INFO] Compiling 1 source file to /target/test-classes
[INFO]
[INFO] --- maven-surefire-plugin:2.10:test (default-test) @ dcs ---
[INFO] Surefire report directory: /target/surefire-reports

-------------------------------------------------------
 T E S T S
-------------------------------------------------------
Running org.trafodion.dcs.AppTest
Tests run: 1, Failures: 0, Errors: 0, Skipped: 0, Time elapsed: 0.007 sec

Results :

Tests run: 1, Failures: 0, Errors: 0, Skipped: 0

[INFO]
[INFO] --- maven-jar-plugin:2.3.2:jar (default-jar) @ dcs ---
[INFO] Building jar: /target/dcs-1.0.0.jar
[INFO]
[INFO] --- maven-assembly-plugin:2.2-beta-5:single (tarball) @ dcs ---
[INFO] Reading assembly descriptor: src/assembly/all.xml
[INFO] Building tar : /target/dcs-1.0.0.tar.gz
[INFO]
[INFO] --- maven-resources-plugin:2.5:resources (default-resources) @ dcs ---
[debug] execute contextualize
[INFO] Using 'UTF-8' encoding to copy filtered resources.
[INFO] Copying 1 resource
[INFO]
[INFO] --- maven-compiler-plugin:2.3.2:compile (default-compile) @ dcs ---
[INFO] Nothing to compile - all classes are up to date
[INFO]
[INFO] --- maven-resources-plugin:2.5:testResources (default-testResources) @ dcs ---
[debug] execute contextualize
[INFO] Using 'UTF-8' encoding to copy filtered resources.
[INFO] Copying 1 resource
[INFO]
[INFO] --- maven-compiler-plugin:2.3.2:testCompile (default-testCompile) @ dcs ---
[INFO] Nothing to compile - all classes are up to date
[INFO]
[INFO] --- maven-surefire-plugin:2.10:test (default-test) @ dcs ---
[INFO] Skipping execution of surefire because it has already been run for this configuration
[INFO]
[INFO] --- maven-jar-plugin:2.3.2:jar (default-jar) @ dcs ---
[INFO]
[INFO] --- maven-assembly-plugin:2.2-beta-5:single (tarball) @ dcs ---
[INFO] Reading assembly descriptor: src/assembly/all.xml
[INFO] Building tar : /target/dcs-1.0.0.tar.gz
[WARNING] Artifact org.trafodion.dcs:dcs:tar.gz:1.0.0 already attached to project, ignoring duplicate
[INFO]
[INFO] --- maven-install-plugin:2.3.1:install (default-install) @ dcs ---
[INFO] Installing dcs-1.0.0.jar to /opt/home/maven-repository/org.trafodion.dcs/dcs/1.0.0/dcs-1.0.0.jar
[INFO] Installing pom.xml to /opt/home/maven-repository/org.trafodion.dcs/dcs/1.0.0/dcs-1.0.0.pom
[INFO] Installing target/dcs-1.0.0.tar.gz to /opt/home/maven-repository/org.trafodion.dcs/dcs/1.0.0/dcs-1.0.0.tar.gz
[INFO] ------------------------------------------------------------------------
[INFO] BUILD SUCCESS
[INFO] ------------------------------------------------------------------------
[INFO] Total time: 11.460s
[INFO] Finished at: Tue Jul 23 20:21:44 UTC 2013
[INFO] Final Memory: 20M/1006M
[INFO] ------------------------------------------------------------------------

To start DCS :
>cd <your path to DCS>
>bin/start-dcs.sh 

To stop DCS:
>cd <your path to DCS>
>bin/stop-dcs.sh 