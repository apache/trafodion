Build instructions for Wms

----------------------------------------------------------------------------------
Requirements:

* Unix System
* JDK 1.6+
* Maven 3.0 or later
* Findbugs 1.3.9 (if running findbugs)
* GNU autotools chain 2.59 or newer, Zookeeper 3.4.5 and Thrift 0.9.0 (if compiling native code)
* Internet connection for first build (to fetch all Maven and Wms dependencies)

----------------------------------------------------------------------------------
Maven build goals:

 * Clean                     : mvn clean
 * Compile                   : mvn compile [-Pnative]
 * Run tests                 : mvn test [-Pnative]
 * Create JAR                : mvn package
 * Install JAR in M2 cache   : mvn install
 * Deploy JAR to Maven repo  : mvn deploy
 * Build javadocs            : mvn javadoc:javadoc
 * Build distribution        : mvn package [-Pdist][-Pdocs][-Psrc][-Pnative][-Dtar]
 * Change Wms version        : mvn versions:set -DnewVersion=NEWVERSION

 Build options:

  * Use -Pnative to compile/bundle native code
  * Use -Pdocs to generate & bundle the documentation in the distribution (using -Pdist)
  * Use -Psrc to create a project source TAR.GZ
  * Use -Dtar to create a TAR with the distribution (using -Pdist)

 Tests options:

  * Use -DskipTests to skip tests when running the following Maven goals:
    'package',  'install', 'deploy' or 'verify'
  * -Dtest=<TESTCLASSNAME>,<TESTCLASSNAME#METHODNAME>,....
  * -Dtest.exclude=<TESTCLASSNAME>
  * -Dtest.exclude.pattern=**/<TESTCLASSNAME1>.java,**/<TESTCLASSNAME2>.java

----------------------------------------------------------------------------------
C client library: 

If you're building the client library from a source checkout you need to
follow the steps outlined below. 

1) unzip/untar the source tarball and cd to the wms-x.x.x/src/main/c directory
2) do a "autoreconf -if" to bootstrap autoconf, automake and libtool. 
   Please make sure you have autoconf
   version 2.59 or greater installed. If cppunit is installed in a non-standard
   directory, you need to specify where to find cppunit.m4. For example, if
   cppunit is installed under /usr/local, run:
   
       ACLOCAL="aclocal -I /usr/local/share/aclocal" autoreconf -if

3) do a "./configure [OPTIONS]" to generate the makefile. See INSTALL
   for general information about running configure. Additionally, the
   configure supports the following options:
   --enable-debug     enables optimization and enables debug info compiler
                      options, disabled by default
   --disable-static   do not build static libraries, enabled by default
   --disable-shared   do not build shared libraries, enabled by default
   --without-cppunit  do not build the test library, enabled by default.
   
   If Thrift or Zookeeper is installed in a non-standard
   directory, you need to specify where to find them e.g.,
   
   ../configure CPPFLAGS="-I <path to zookeeper>/include -I <path to thrift>/include" 
      LDFLAGS="-L<path to zookeeper>/lib -L<path to thrift>/lib"

4) do a "make" or "make install" to build the libraries and install them. 
   Alternatively, you can also build and run a unit test suite (and
   you probably should).  Please make sure you have cppunit-1.10.x or
   higher installed before you execute step 4.  Once ./configure has
   finished, do a "make run-check". It will build the libraries, build
   the tests and run them.

----------------------------------------------------------------------------------
Building distributions:

Create binary distribution without native code and without documentation:

  $ mvn package -Pdist -DskipTests -Dtar

Create binary distribution with native code and with documentation:

  $ mvn package -Pdist,native,docs -DskipTests -Dtar

Create source distribution:

  $ mvn package -Psrc -DskipTests

Create source and binary distributions with native code and documentation:

  $ mvn package -Pdist,native,docs,src -DskipTests -Dtar

Create a local staging version of the website (in /tmp/hadoop-site)

  $ mvn clean site; mvn site:stage -DstagingDirectory=/tmp/hadoop-site

----------------------------------------------------------------------------------

Handling out of memory errors in builds

----------------------------------------------------------------------------------

If the build process fails with an out of memory error, you should be able to fix
it by increasing the memory used by maven -which can be done via the environment
variable MAVEN_OPTS.

Here is an example setting to allocate between 256 and 512 MB of heap space to
Maven

export MAVEN_OPTS="-Xms256m -Xmx512m"

----------------------------------------------------------------------------------
