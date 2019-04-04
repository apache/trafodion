#! /usr/bin/env python

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

import xml.etree.ElementTree as ET
import errno
import fcntl
import glob
import inspect
import optparse
import os
import re
import select
import string
import subprocess
import sys
import urllib2

from distutils import spawn


#----------------------------------------------------------------------------
# Dictionary for the following pre-defined test groups :
#    PART1 : First half of the phoenix test divided by time. Takes
#            approx 30 min.
#    PART2 : Second half of the phoenix test divided by time. Takes
#            approx 30 min.
#    QUICK1: Half of the phoenix tests selected by probability of
#            failure and time (old set). Takes approx. 15 min to run.
#    QUICK2: Half of the phoenix tests selected by probability of
#            failure and time (new set). Takes approx. 15 min to run.
#
# FunkyNamesTest has been removed temporarily
# EsgynDB is not able to support delimited column names correctly. It has always been returning incorrect results.
# We have modified FunkyNamesTest to suit our behavior. With the change to move from Array to HashMap for performance
# reasons in both T4 and T2 driver code. However, the incorrect results differ between T2 and T4 when there are 
# more than one delimited column name matches due to uppercasing the column name always.  This needs some change
# in the descriptor information sent from SQL engine to denote that column names are delimited to preserve case.
# We need to use the case preserving key String for delimited column names and case insensitive key String for 
# regular column names in the HashMap to convert from column names to column index.
#----------------------------------------------------------------------------
TEST_SUBSET = {'PART1': ['AlterTableTest', 'ArithmeticQueryTest', 'BinaryRowKeyTest',
                         'CoalesceFunctionTest', 'CompareDecimalToLongTest', 'CustomEntityDataTest',
                         'DeleteRangeTest', 'DescColumnSortOrderTest', 'DistinctCountTest',
                         'ExtendedQueryExecTest', 'IsNullTest', 'OrderByTest',
                         'QueryExecTest', 'SaltedTableTest', 'SaltedTableUpsertSelectTest',
                         'SaltedTableVarLengthRowKeyTest', 'SkipScanQueryTest',
                         'UpsertSelectAutoCommitTest', 'UpsertSelectTest', 'UpsertValuesTest'],
               'PART2': ['AutoCommitTest', 'CreateTableTest', 'ExecuteStatementsTest',
                         'GroupByCaseTest', 'IndexTest', 'KeyOnlyTest', 'MultiCfQueryExecTest',
                         'ProductMetricsTest', 'QueryExecWithoutSCNTest', 'QueryPlanTest',
                         'ReadIsolationLevelTest', 'ServerExceptionTest', 'StatementHintsTest',
                         'StddevTest', 'ToCharFunctionTest', 'ToNumberFunctionTest', 'TopNTest',
                         'UpsertBigValuesTest', 'VariableLengthPKTest'],
               'QUICK1': ['AlterTableTest', 'ArithmeticQueryTest', 'AutoCommitTest',
                          'BinaryRowKeyTest', 'CoalesceFunctionTest', 'CompareDecimalToLongTest',
                          'CreateTableTest', 'CustomEntityDataTest', 'DeleteRangeTest',
                          'DistinctCountTest', 'ExecuteStatementsTest', 'ExtendedQueryExecTest',
                          'GroupByCaseTest', 'IndexTest', 'IsNullTest',
                          'KeyOnlyTest', 'MultiCfQueryExecTest', 'OrderByTest'],
               'QUICK2': ['AlterTableTest', 'ArithmeticQueryTest', 'AutoCommitTest',
                          'BinaryRowKeyTest', 'CompareDecimalToLongTest', 'CreateTableTest',
                          'DeleteRangeTest', 'DistinctCountTest', 'ExecuteStatementsTest',
                          'GroupByCaseTest', 'IndexTest', 'IsNullTest', 'KeyOnlyTest',
                          'OrderByTest', 'QueryExecWithoutSCNTest', 'SaltedTableUpsertSelectTest',
                          'StatementHintsTest', 'ToCharFunctionTest', 'ToNumberFunctionTest',
                          'TopNTest', 'UpsertSelectAutoCommitTest']
               }


#----------------------------------------------------------------------------
# A function (or a structure in C's sense) that contains all variables
# recording user's command-line parameters.
#----------------------------------------------------------------------------
def ArgList():
    _target = None
    _user = None
    _pw = None
    _role = None
    _dsn = None
    _javahome = None
    _jdbc_classpath = None
    _result_dir = None
    _prop_file = None
    _target_type = None
    _export_str1 = None
    _export_str2 = None
    _export_str3 = None
    _export_str4 = None
    _export_str5 = None
    _jdbc_type = None
    _tests = None
    _hadoop_distro = None
    _maven_local_repo = None
    _no_maven = None


#----------------------------------------------------------------------------
# Tracking all of the global variables in here.
#----------------------------------------------------------------------------
def gvars():
    my_ROOT = None
    my_RUN_CMD = None
    my_EXPORT_CMD = None
    my_TOTAL_TESTS_RUN = None
    my_TOTAL_TESTS_FAILED = None
    my_TOTAL_TESTS_ERRORS = None
    my_TOTAL_TESTS_SKIPPED = None
    my_TOTAL_EXEC_TIME = None
    my_MVN_ERROR_FILE = None


#----------------------------------------------------------------------------
# Make a call to the shell and fetch the results back.
#----------------------------------------------------------------------------
def stdout_write(output):
    sys.stdout.write(output)
    sys.stdout.flush()


def read_async(fd):
    try:
        return fd.read()
    except IOError, e:
        if e.errno != errno.EAGAIN:
            raise e
        else:
            return ''


def shell_call(cmd):
    # subprocess.call(cmd)
    ON_POSIX = 'posix' in sys.builtin_module_names
    hpipe = subprocess.Popen(cmd, stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                             bufsize=1, close_fds=ON_POSIX, shell=True)

    # Change to non-blocking mode, so read returns even if there is no data.
    fcntl.fcntl(hpipe.stdout, fcntl.F_SETFL, os.O_NONBLOCK)
    fcntl.fcntl(hpipe.stderr, fcntl.F_SETFL, os.O_NONBLOCK)

    total_output_stdout = ''
    total_output_stderr = ''
    while True:
        # wait for data to become available
        select.select([hpipe.stdout, hpipe.stderr], [], [])

        # Try reading some data from each
        output_stdout = read_async(hpipe.stdout)
        output_stderr = read_async(hpipe.stderr)

        if output_stdout:
            stdout_write(output_stdout)
            total_output_stdout += output_stdout
        if output_stderr:
            stdout_write(output_stderr)
            total_output_stderr += output_stderr

        rc = hpipe.poll()
        if rc is not None:
            return total_output_stdout + total_output_stderr


#----------------------------------------------------------------------------
# Find the token after s2 in s1
#----------------------------------------------------------------------------
def get_token_after_string(s1, s2, extra_split_chars=None):
    if s2 not in s1:
        return None
    else:
        token = s1[s1.find(s2)+len(s2):].split()[0]
        if extra_split_chars is not None:
            token = token.split(extra_split_chars)[0]
        return token


#----------------------------------------------------------------------------
# Find the sub string after s2 in s1
#----------------------------------------------------------------------------
def get_substr_after_string(s1, s2):
    if s2 not in s1:
        return None
    else:
        return s1[s1.find(s2)+len(s2):]


#----------------------------------------------------------------------------
# Create the result directory if it does not exist
#----------------------------------------------------------------------------
def result_dir_create(result_dir):
    my_result_dir = os.path.abspath(result_dir)
    d = my_result_dir
    folders = []
    while True:
        d, f = os.path.split(d)
        if f:
            folders.insert(0, f)
        else:  # f is empty
            if d:
                folders.insert(0, d)
            break
    path = ''
    for f in folders:
        path = os.path.join(path, f)
        if not os.path.isdir(path):
            os.mkdir(path)
    return my_result_dir


#----------------------------------------------------------------------------
# Get Hadoop component version (hadoop, hbase, hive, zookeeper) based
# on distribution
#----------------------------------------------------------------------------
def get_hadoop_component_ver(distro, component):
    rpm_ver = ''
    if 'HBASE_CNF_DIR' in os.environ:
        local_file_dict = {
            'hadoop_regex': re.compile("^hadoop-common-(\d.*).jar"),
            'hadoop': (os.environ['HBASE_CNF_DIR'] + "/../lib/hadoop-common-*[0-9].jar"),
            'hbase_regex': re.compile("^hbase-common-(\d.*).jar"),
            'hbase': (os.environ['HBASE_CNF_DIR'] + "/../lib/hbase-common-*[0-9].jar"),
            'hive_regex': re.compile("^hive-common-(\d.*).jar"),
            'hive': (os.environ['HIVE_CNF_DIR'] + "/../lib/hive-common-*[0-9].jar"),
            'zookeeper_regex': re.compile("^zookeeper-(\d.*).jar"),
            'zookeeper': (os.environ['HBASE_CNF_DIR'] + "/../lib/zookeeper-*[0-9].jar")
        }

        # find version number of component using rpm command
    if 'HBASE_CNF_DIR' in os.environ and "local_hadoop" in os.environ['HBASE_CNF_DIR']:
        rpm_ver = 'LOCAL'
    elif distro == 'CDH':
        rpm_ver = subprocess.Popen(["rpm", "-q", "--qf", '%{VERSION}', component],
                                   stdout=subprocess.PIPE).communicate()[0]

    # if distro is Cloudera (CDH) then parse string from rpm_ver
    # if distro is HortonWorks (HW) then interrogate commands using script
    # parse those for the actual version of the component

    if rpm_ver == 'LOCAL':
        return(local_file_dict[component + "_regex"].search(
               os.path.basename(glob.glob(local_file_dict[component])[0])).group(1))
    elif distro.startswith("CDH"):
        return(rpm_ver[:rpm_ver.rfind("+")].replace("+", "-"))
    elif distro.startswith("HDP"):
        return(shell_call(gvars.my_ROOT + '/hadoop_ver.sh ' + component))


#----------------------------------------------------------------------------
# Generate pom.xml
#----------------------------------------------------------------------------
def generate_pom_xml(targettype, jdbc_groupid, jdbc_artid, jdbc_path, hadoop_distro):
    # dictionary for Hadoop Distribution dependent dependencies
    # we'll only access one distro, so no need to create full dict
    if hadoop_distro == 'CDH':
        hadoop_dict = {
            'CDH': {'MY_HADOOP_DISTRO': 'cloudera',
                    'MY_HADOOP_VERSION': get_hadoop_component_ver(hadoop_distro, "hadoop"),
                    'MY_MVN_URL': 'http://repository.cloudera.com/artifactory/cloudera-repos',
                    'MY_HBASE_VERSION': get_hadoop_component_ver(hadoop_distro, "hbase"),
                    'MY_HIVE_VERSION': get_hadoop_component_ver(hadoop_distro, "hive"),
                    'MY_ZOOKEEPER_VERSION': get_hadoop_component_ver(hadoop_distro, "zookeeper"),
                    # cdh sub-string added at 1.1
                    'TRAF_HBASE_TRX_REGEX': re.compile("^hbase-trx-(cdh[\d_]*-)?[\d\.]{3,}jar"),
                    'MVN_DEPS': [('org.apache.hbase', 'hbase-client', '${hbase_version}', 'EDEP'),
                                 ('org.apache.hbase', 'hbase-common', '${hbase_version}', 'EDEP'),
                                 ('org.apache.hbase', 'hbase-server', '${hbase_version}', 'EDEP'),
                                 ('org.apache.hbase', 'hbase-protocol', '${hbase_version}', 'EDEP'),
                                 ('org.apache.hadoop', 'hadoop-auth', '${hadoop_version}', 'IDEP'),
                                 ('org.apache.hadoop', 'hadoop-common', '${hadoop_version}',
                                  'IDEP'),
                                 ('org.apache.hadoop', 'hadoop-hdfs', '${hadoop_version}', 'IDEP'),
                                 ('org.apache.hadoop', 'hadoop-mapreduce-client-core',
                                  '${hadoop_version}', 'IDEP'),
                                 ('org.apache.hive', 'hive-common', '${hive_version}', 'IDEP'),
                                 ('org.apache.hive', 'hive-exec', '${hive_version}', 'IDEP'),
                                 ('org.apache.hive', 'hive-jdbc', '${hive_version}', 'EDEP'),
                                 ('org.apache.zookeeper', 'zookeeper', '${zookeeper_version}',
                                  'EDEP')
                                 ]
                    },
        }
    elif hadoop_distro == 'HDP':
        hadoop_dict = {
            'HDP': {'MY_HADOOP_DISTRO': 'HDPReleases',
                    'MY_HADOOP_VERSION': get_hadoop_component_ver(hadoop_distro, "hadoop"),
                    'MY_MVN_URL': 'http://repo.hortonworks.com/content/groups/public/',
                    'MY_HBASE_VERSION': get_hadoop_component_ver(hadoop_distro, "hbase"),
                    'MY_HIVE_VERSION': get_hadoop_component_ver(hadoop_distro, "hive"),
                    'MY_ZOOKEEPER_VERSION': get_hadoop_component_ver(hadoop_distro, "zookeeper"),
                    'TRAF_HBASE_TRX_REGEX': re.compile("^hbase-trx-hdp[\d_]*-[\d\.]{3,}jar"),
                    'MVN_DEPS': [('org.apache.hbase', 'hbase-client', '${hbase_version}', 'EDEP'),
                                 ('org.apache.hbase', 'hbase-common', '${hbase_version}', 'EDEP'),
                                 ('org.apache.hbase', 'hbase-server', '${hbase_version}', 'EDEP'),
                                 ('org.apache.hbase', 'hbase-protocol', '${hbase_version}', 'EDEP'),
                                 ('org.apache.hadoop', 'hadoop-auth', '${hadoop_version}', 'IDEP'),
                                 ('org.apache.hadoop', 'hadoop-common', '${hadoop_version}',
                                  'EDEP'),
                                 ('org.apache.hadoop', 'hadoop-hdfs', '${hadoop_version}', 'EDEP'),
                                 ('org.apache.hadoop', 'hadoop-mapreduce-client-core',
                                  '${hadoop_version}', 'EDEP'),
                                 ('org.apache.hadoop', 'hadoop-yarn-common', '${hadoop_version}',
                                  'EDEP'),
                                 ('org.apache.hive', 'hive-common', '${hive_version}', 'IDEP'),
                                 ('org.apache.hive', 'hive-exec', '${hive_version}', 'IDEP'),
                                 ('org.apache.hive', 'hive-jdbc', '${hive_version}', 'EDEP'),
                                 ('org.apache.zookeeper', 'zookeeper', '${zookeeper_version}',
                                  'EDEP')
                                 ]
                    }
        }

    # read template file into multiline string
    with open(os.path.join(gvars.my_ROOT, 'pom.xml.template'), 'r') as fd1:
        template_text = fd1.read()

    # substitute template variables with real info
    with open(os.path.join(gvars.my_ROOT, 'pom.xml'), 'w') as fd2:
        # remove T2 only specifications when using T4 driver
        if jdbc_artid == 't4driver':
            remove_t2 = re.compile('<!-- START_FOR_T2_ONLY -->.*?<!-- END_FOR_T2_ONLY -->',
                                   re.MULTILINE | re.DOTALL)
            template_text = remove_t2.sub("", template_text)
        elif jdbc_artid == 't2driver':
            dep_string = ""
            # generate Hadoop Distribution dependent dependency multiline string
            for groupid, artid, version, exclusion in hadoop_dict[hadoop_distro]['MVN_DEPS']:
                dep_string = dep_string + "    <dependency>\n"
                dep_string = dep_string + "        <groupId>" + groupid + "</groupId>\n"
                dep_string = dep_string + "        <artifactId>" + artid + "</artifactId>\n"
                dep_string = dep_string + "        <version>" + version + "</version>\n"
                dep_string = dep_string + "        <scope>test</scope>\n"

                # check to see if we need to include/exclude transitive dependencies
                if exclusion != 'IDEP':
                    dep_string = dep_string + "        <exclusions>\n"
                    dep_string = dep_string + "          <exclusion>\n"
                    dep_string = dep_string + "            <groupId>*</groupId>\n"
                    dep_string = dep_string + "            <artifactId>*</artifactId>\n"
                    dep_string = dep_string + "          </exclusion>\n"
                    dep_string = dep_string + "        </exclusions>\n"

                dep_string = dep_string + "    </dependency>\n"

            template_text = re.sub('<!-- START_DISTRO_DEP -->', dep_string, template_text)

            # look for Trafodion Hbase TRX file and Trafodion Hbase Access file
            # assume in $TRAF_HOME/export/lib
            traf_lib_file_list = os.listdir(os.environ['TRAF_HOME'] + '/export/lib')

            # assume regular expression used for traf_hbase_trx_file and traf_hbase_access_file
            # is precise enough to ever return only 1 value
            traf_hbase_trx_file = [m.group(0) for l in traf_lib_file_list for m in
                                   [hadoop_dict[hadoop_distro]['TRAF_HBASE_TRX_REGEX'].search(l)]
                                   if m][0]
            template_text = re.sub('TRAF_HBASE_TRX_FILE', traf_hbase_trx_file, template_text)

            # fix up T2 Hadoop properties
            for hprop in ['MY_HADOOP_DISTRO', 'MY_HADOOP_VERSION', 'MY_MVN_URL', 'MY_HBASE_VERSION',
                          'MY_HIVE_VERSION', 'MY_ZOOKEEPER_VERSION']:
                template_text = re.sub(hprop, hadoop_dict[hadoop_distro][hprop], template_text)

        # fix up properties common to T2 and T4
        template_text = re.sub('MYJDBC_GROUP_ID', jdbc_groupid, template_text)
        template_text = re.sub('MYJDBC_ART_ID', jdbc_artid, template_text)
        template_text = re.sub('MYJDBC_PATH', jdbc_path, template_text)

        fd2.write(template_text)


#----------------------------------------------------------------------------
# Generate propfile
#----------------------------------------------------------------------------
def generate_t4_propfile(propfile, target, user, pw, role, dsn, targettype):
    with open(propfile, 'w') as fd:
        fd.write('url=jdbc:t4jdbc://' + target + '/\n')
        fd.write('user=' + user + '\n')
        fd.write('roleName=' + role + '\n')
        fd.write('password=' + pw + '\n')
        fd.write('catalog=trafodion\n')
        fd.write('schema=phoenixT4\n')
        fd.write('serverDataSource=' + dsn + '\n')
        fd.write('targettype=' + targettype + '\n')
        fd.write('sessionName=phoenix\n')
        fd.write('applicationName=phoenix\n')


def generate_t2_propfile(propfile, targettype):
    with open(propfile, 'w') as fd:
        fd.write('url=jdbc:t2jdbc:\n')
        fd.write('user=DONTCARE\n')
        fd.write('roleName=DONTCARE\n')
        fd.write('password=DONTCARE\n')
        fd.write('catalog=trafodion\n')
        fd.write('schema=phoenixT2\n')
        fd.write('serverDataSource=DONTCARE\n')
        fd.write('targettype=' + targettype + '\n')
        fd.write('sessionName=phoenix\n')
        fd.write('applicationName=phoenix\n')


#----------------------------------------------------------------------------
# Write to the stdout and/or the log files
#----------------------------------------------------------------------------
def logfile_write(file, output):
    if ArgList._result_dir is not None:
        fd = open(os.path.join(ArgList._result_dir, file), 'w')
        fd.write(output)
        fd.close()


#----------------------------------------------------------------------------
# Parse the argument list for main
#----------------------------------------------------------------------------
def prog_parse_args():
    rec = inspect.stack()[1]  # 0 reprents this line,
                              # 1 reprents line at caller,
                              # 2 reprents line at caller's caller,
                              # ... and so on ...
    frame = rec[0]
    info = inspect.getframeinfo(frame)
    gvars.my_ROOT = os.path.dirname(os.path.abspath(info.filename))
    DEFAULT_RESULTS_DIR = os.path.join(gvars.my_ROOT, 'results')
    DEFAULT_JDBC_CLASSPATH = '${project.basedir}/lib/jdbcT4-${TRAFODION_VER}.jar'
    DEFAULT_PROP_FILE = os.path.join(gvars.my_ROOT, 'jdbcprop')

    # alas, the more powerful argparse module only exists in >= 2.7 and >= 3.2,
    # use optparse instead.
    option_list = [
        # No need to add '-h' or '-help', optparse automatically adds one.

        # we do not use short options, hence the first ''.
        # required args
        optparse.make_option('', '--target', action='store', type='string',
                             dest='target',
                             help="target '<IP>:<port>', required argument with no default. " +
                                  "This is still needed for jdbc loader even when using sqlci."),
        optparse.make_option('', '--user', action='store', type='string',
                             dest='user',
                             help='user id for the target, required argument with no default. ' +
                                  'It can be any non-empty string if using sqlci.'),
        optparse.make_option('', '--pw', action='store', type='string',
                             dest='pw',
                             help='password for the target, required argument with no default. ' +
                                  'It can be any non-empty string if using sqlci.'),

        # optional args
        optparse.make_option('', '--role', action='store', type='string',
                             dest='role', default='',
                             help="role for the target, defaulted to ''"),
        optparse.make_option('', '--dsn', action='store', type='string',
                             dest='dsn', default='TDM_Default_DataSource',
                             help="data source for the target, defaulted to " +
                                  "'TDM_Default_DataSource'"),
        optparse.make_option('', '--javahome', action='store', type='string',
                             dest='javahome', default='/usr',
                             help="java program (version 1.7 required) location, " +
                                  "defaulted to '/usr'"),
        optparse.make_option('', '--jdbccp', action='store', type='string',
                             dest='jdbccp', default=DEFAULT_JDBC_CLASSPATH,
                             help="jdbc classpath, defaulted to " +
                                  "'${project.basedir}/lib/jdbcT4-${TRAFODION_VER}.jar', \t\t "
                                  "<test_root> is where this program is"),
        optparse.make_option('', '--resultdir', action='store', type='string',
                             dest='resultdir', default=DEFAULT_RESULTS_DIR,
                             help='results directory, ONLY used with option --nomvntest, ' +
                                  'defaults to \'<test root>/results\', <test root> is where ' +
                                  'this program is located'),
        optparse.make_option('', '--propfile', action='store', type='string',
                             dest='propfile', default=DEFAULT_PROP_FILE,
                             help="property file, defaulted to automatically generated " +
                                  "'<test root>/jdbcprop', <test root> is where this program is"),
        optparse.make_option('', '--targettype', action='store', type='string',
                             dest='targettype', default='TR',
                             help='target type, TR for TRAFODION, ' +
                                  'defaulted to TR'),
        optparse.make_option('', '--jdbctype', action='store', type='string',
                             dest='jdbctype', default='T4',
                             help='jdbctype, defaulted to T4'),
        optparse.make_option('', '--hadoop', action='store', type='string',
                             dest='hadoop', default='CDH',
                             help='Hadoop Distro, possible values are CDH (Cloudera), ' +
                                  'HDP (Hortonworks). Defaulted to CDH.'),
        optparse.make_option('', '--mvnlocalrepo', action='store', type='string',
                             dest='mvnlocalrepo', default='$HOME/.m2/repository',
                             help='Location of Maven local repository, defaulted to ' +
                                  '$HOME/.m2/repository'),
        optparse.make_option('', '--nomvntest', action='store_true', dest='nomaven', default=False,
                             help='turn off usage of Maven in test phase. Maven will still be ' +
                                  'used in the compile phase. This will change the tests to '
                                  'run via the system Java environment. Default is False '),
        optparse.make_option('', '--export1', action='store', type='string',
                             dest='exportstr1', default='NONE',
                             help='any export string, defaulted to NONE'),
        optparse.make_option('', '--export2', action='store', type='string',
                             dest='exportstr2', default='NONE',
                             help='any export string, defaulted to NONE'),
        optparse.make_option('', '--export3', action='store', type='string',
                             dest='exportstr3', default='NONE',
                             help='any export string, defaulted to NONE'),
        optparse.make_option('', '--export4', action='store', type='string',
                             dest='exportstr4', default='NONE',
                             help='any export string, defaulted to NONE'),
        optparse.make_option('', '--export5', action='store', type='string',
                             dest='exportstr5', default='NONE',
                             help='any export string, defaulted to NONE'),
        optparse.make_option('', '--tests', action='store', type='string',
                             dest='tests', default='.*',
                             help="specify a subset of tests to run. This can either be a string " +
                                  "with each test sperated by ',' without space characters OR " +
                                  "one of the following pre-defined subsets: PART1, PART2, " +
                                  "QUICK1, or QUICK2. If this option is omitted, the default is " +
                                  "ALL tests. \t\t\t\t For example: \t\t\t\t\t\t\t\t\t " +
                                  "--tests=AlterTableTest \t\t\t\t " +
                                  "--tests=AlterTableTest,ArithmeticQueryTest \t\t" +
                                  "--tests=QUICK1 \t\t\t\t\t\t\t\t")
    ]

    usage = 'usage: %prog [-h|--help|<options>]'
    parser = optparse.OptionParser(usage=usage, option_list=option_list)
    # OptionParser gets the options out, whatever is not preceeded by
    # an option is considered args.
    (options, args) = parser.parse_args()

    # we are not expecting any args right now.  In the future, if we do,
    # make a list of the known args and check against it.
    if args:
        parser.error('Invalid argment(s) found: ' + str(args))

    if options.targettype != 'SQ' and options.targettype != 'TR':
        parser.error('Invalid --targettype.  Only SQ aor TR is supported: ' +
                     options.targettype)

    # take first three characters to be backwards compatible with CDH51, etc
    distro = options.hadoop[0:3]
    if distro != 'CDH' and distro != 'HDP':
        parser.error('Invalid --hadoop.  Only CDH (Cloudera) or HDP ' +
                     '(Hortonworks) + is supported: ' + options.hadoop)

    if options.jdbctype != 'T2' and options.jdbctype != 'T4':
        parser.error('Invalid --jdbctype.  Only T2 or T4 is supported: ' +
                     options.jdbctype)

    # check for the required args for certain conditions
    if options.jdbctype == 'T4':
        not_found = []
        T4_required_args = ['target', 'user', 'pw']
        for r in T4_required_args:
            if options.__dict__[r] is None:
                not_found.append('--' + r)
        if not_found:
            parser.error('Required option(s) not found: ' + str(not_found))

        myjdbc_groupid = 'org.trafodion.jdbc.t4.T4Driver'
        myjdbc_artid = 't4driver'
    elif options.jdbctype == 'T2':
        # check for Trafodion ENV variables to be set
        req_envs_error_string = ""
        for req_env in ['SQ_MBTYPE', 'TRAF_HOME', 'MPI_TMPDIR', 'LD_PRELOAD', 'LD_LIBRARY_PATH',
                        'PATH', 'LANG', 'HADOOP_CNF_DIR', 'HBASE_CNF_DIR', 'HIVE_CNF_DIR',
                        'HBASE_TRXDIR']:
            if req_env not in os.environ:
                req_envs_error_string = (req_envs_error_string + 'Required environment variable ' +
                                         req_env + ' for T2 test has NOT been set!\n')

        if req_envs_error_string:
            parser.error(req_envs_error_string)

        myjdbc_groupid = 'org.apache.trafodion.jdbc.t2.T2Driver'
        myjdbc_artid = 't2driver'

    # Automatically generate the prop file if the user did not specify one
    if options.propfile == DEFAULT_PROP_FILE:
        if options.jdbctype == 'T2':
            generate_t2_propfile(options.propfile, options.targettype)
        elif options.jdbctype == 'T4':
            generate_t4_propfile(options.propfile, options.target, options.user, options.pw,
                                 options.role, options.dsn, options.targettype)

    ArgList._target = options.target
    ArgList._user = options.user
    ArgList._pw = options.pw
    ArgList._role = options.role
    ArgList._dsn = options.dsn
    ArgList._javahome = options.javahome
    if options.jdbccp != DEFAULT_JDBC_CLASSPATH:
        options.jdbccp = os.path.abspath(options.jdbccp)
    ArgList._jdbc_classpath = options.jdbccp
    ArgList._result_dir = os.path.abspath(options.resultdir)
    ArgList._prop_file = os.path.abspath(options.propfile)
    ArgList._target_type = options.targettype
    ArgList._jdbc_type = options.jdbctype
    ArgList._hadoop_distro = distro
    ArgList._maven_local_repo = options.mvnlocalrepo
    ArgList._no_maven = options.nomaven

    # Turn off usage of maven for T2 tests
    # Remove once TRAFODION-1929 is fixed
    if options.jdbctype == 'T2':
        ArgList._no_maven = True

    ArgList._export_str1 = options.exportstr1
    ArgList._export_str2 = options.exportstr2
    ArgList._export_str3 = options.exportstr3
    ArgList._export_str4 = options.exportstr4
    ArgList._export_str5 = options.exportstr5

    # check if tests parameter was passed in a pre-defined test group
    if options.tests.upper() in ['PART1', 'PART2', 'QUICK1', 'QUICK2']:
        # tests parameter was passed a pre-defined test group so
        # generate list of tests seperated by a comma
        ArgList._tests = ','.join(map(str, TEST_SUBSET[options.tests.upper()]))
    else:
        ArgList._tests = options.tests

    # Generate the pom.xml file from the template according to target type
    generate_pom_xml(ArgList._target_type, myjdbc_groupid, myjdbc_artid,
                     ArgList._jdbc_classpath, ArgList._hadoop_distro)

    print 'target:                ', ArgList._target
    print 'user:                  ', ArgList._user
    print 'pw:                    ', ArgList._pw
    print 'role:                  ', ArgList._role
    print 'dsn:                   ', ArgList._dsn
    print 'java home:             ', ArgList._javahome
    print 'jdbc classpath:        ', ArgList._jdbc_classpath
    print 'prop file:             ', ArgList._prop_file
    print 'target type:           ', ArgList._target_type
    print 'jdbc type:             ', ArgList._jdbc_type
    print 'hadoop distro:         ', ArgList._hadoop_distro
    print 'maven local repo:      ', ArgList._maven_local_repo
    print 'export string 1:       ', ArgList._export_str1
    print 'export string 2:       ', ArgList._export_str2
    print 'export string 3:       ', ArgList._export_str3
    print 'export string 4:       ', ArgList._export_str4
    print 'export string 5:       ', ArgList._export_str5
    if ArgList._no_maven:
        print 'test tool:              System Java'
        print 'results directory:     ', ArgList._result_dir
    else:
        print 'test tool:              Maven (mvn)'
    if options.tests != '.*':
        print 'tests to be run:       ', ArgList._tests
    else:
        print 'tests to be run:        ALL tests'
    print

    sys.stdout.flush()

    gvars.my_EXPORT_CMD = 'export PATH=' + ArgList._javahome + '/bin:$PATH'

    if ArgList._export_str1 != 'NONE':
        if gvars.my_EXPORT_CMD != '':
            gvars.my_EXPORT_CMD += ';'
        gvars.my_EXPORT_CMD += 'export ' + ArgList._export_str1
    if ArgList._export_str2 != 'NONE':
        if gvars.my_EXPORT_CMD != '':
            gvars.my_EXPORT_CMD += ';'
        gvars.my_EXPORT_CMD += 'export ' + ArgList._export_str2
    if ArgList._export_str3 != 'NONE':
        if gvars.my_EXPORT_CMD != '':
            gvars.my_EXPORT_CMD += ';'
        gvars.my_EXPORT_CMD += 'export ' + ArgList._export_str3
    if ArgList._export_str4 != 'NONE':
        if gvars.my_EXPORT_CMD != '':
            gvars.my_EXPORT_CMD += ';'
        gvars.my_EXPORT_CMD += 'export ' + ArgList._export_str4
    if ArgList._export_str5 != 'NONE':
        if gvars.my_EXPORT_CMD != '':
            gvars.my_EXPORT_CMD += ';'
        gvars.my_EXPORT_CMD += 'export ' + ArgList._export_str5


#----------------------------------------------------------------------------
# Generate summary from log file
#----------------------------------------------------------------------------
def generate_summary(output):
    parse_maven = False
    parse_junit = False
    parse_mvnerror = False

    lines = output.split('\n')

    for ln in lines:
        # for Maven results
        if ln.startswith('Results :'):
            parse_maven = True
            parse_junit = False
            continue

        if ln.startswith('# An error report file with more information is saved as:'):
            parse_mvnerror = True
            continue

        # for non-Maven JUnit results
        if ln.startswith('Running test without Maven :'):
            parse_maven = False
            parse_junit = True

        # for Maven results
        if parse_maven:
            if ln.startswith('Tests run: '):
                gvars.my_TOTAL_TESTS_RUN = int(get_token_after_string(ln, 'Tests run:', ','))
                gvars.my_TOTAL_TESTS_FAILED = int(get_token_after_string(ln, 'Failures:', ','))
                gvars.my_TOTAL_TESTS_ERRORS = int(get_token_after_string(ln, 'Errors:', ','))
                gvars.my_TOTAL_TESTS_SKIPPED = int(get_token_after_string(ln, 'Skipped:', ','))

            if ln.startswith('[INFO] Total time: '):
                gvars.my_TOTAL_EXEC_TIME = get_token_after_string(ln, '[INFO] Total time: ')

        if parse_mvnerror:
            gvars.my_MVN_ERROR_FILE = str(get_token_after_string(ln, '# ')).strip()
            parse_mvnerror = False

        # for non-Maven JUnit results
        if parse_junit:
            if ln.startswith('OK ('):
                gvars.my_TOTAL_TESTS_RUN += int(get_token_after_string(ln, 'OK (', ')'))
            elif 'Tests run:' in ln and 'Failures:' in ln:
                gvars.my_TOTAL_TESTS_RUN += int(get_token_after_string(ln, 'Tests run:', ','))
                gvars.my_TOTAL_TESTS_FAILED += int(get_token_after_string(ln, 'Failures:'))
            elif ln.startswith('Time:'):
                gvars.my_TOTAL_EXEC_TIME += float(get_token_after_string(ln,
                                                  'Time:').replace(',', ''))


#----------------------------------------------------------------------------
# main
#----------------------------------------------------------------------------
sumfile_output = ''
gvars.my_TOTAL_TESTS_RUN = 0
gvars.my_TOTAL_TESTS_FAILED = 0
gvars.my_TOTAL_TESTS_ERRORS = 0
gvars.my_TOTAL_TESTS_SKIPPED = 0
gvars.my_MVN_ERROR_FILE = None

prog_parse_args()

os.chdir(gvars.my_ROOT)

# check to make sure executables mvn and javac are in the PATH
if spawn.find_executable("mvn") is None:
    print "ERROR: Could not find the Maven executable \'mvn\' in the PATH! \n"
    sys.exit(2)

if spawn.find_executable("javac") is None:
    print "ERROR: Could not find the Java Compiler executable \'javac\' in the PATH! \n"
    sys.exit(2)

# clean the target
output = shell_call(gvars.my_EXPORT_CMD + ';mvn clean')
stdout_write(output + '\n')

# find out location of Maven Downloads
maven_repo = shell_call('mvn help:evaluate -Dmaven.repo.local=' + ArgList._maven_local_repo +
                        ' -Dexpression=settings.localRepository | grep -B 2 "BUILD SUCCESS" ' +
                        '| head -1').strip(' \t\n\t')
print "\nMaven Repo Location is : " + maven_repo + ".\n"

# run tests with or without Maven depending on program arguments
if not ArgList._no_maven:
    # use Maven for testing
    gvars.my_TOTAL_EXEC_TIME = ''

    # have Maven generate CLASSPATH it will use for testing
    # additionally the following env variables need to be appended to the CLASSPATH since they
    # are added to the CLASSPATH by the Surefire Plugin after Maven generates the CLASSPATH
    #  HADOOP_CNF_DIR, HBASE_CNF_DIR, HIVE_CNF_DIR, HBASE_TRXDIR
    subs_string = ':${HADOOP_CNF_DIR}:${HBASE_CNF_DIR}:${HIVE_CNF_DIR}:${HBASE_TRXDIR}\\n'
    output = shell_call('rm phoenix_classpath.log 2>/dev/null; ' +
                        'mvn dependency:build-classpath -Dmaven.repo.local=' +
                        ArgList._maven_local_repo + ' -Dmdep.outputFile=phoenix_classpath.log; ' +
                        'sed -i -e "s|$|' + subs_string + '|g" phoenix_classpath.log; ' +
                        'echo "Phoenix Test Classpath :"; cat phoenix_classpath.log')

    # download dependencies, compile test, and run test with Maven
    output = shell_call(gvars.my_EXPORT_CMD + '; free -m; mvn test -Dmaven.repo.local=' +
                        ArgList._maven_local_repo + ' -Dtest=' + ArgList._tests)

    # generate summary
    generate_summary(output)

else:
    # do NOT use Maven for testing
    gvars.my_TOTAL_EXEC_TIME = 0

    # check to make sure CLASSPATH is set
    if 'CLASSPATH' not in os.environ:
        print ("Required environment variable, CLASSPATH, for running Phoenix Test without Maven" +
               "has not been set!")
        sys.exit(2)

    # create results directory if needed
    result_dir_create(ArgList._result_dir)

    # set up CLASSPATH by adding the following :
    #   Maven generated class directory target/test-classes
    #   JUnit 4.11 jar, Hamcrest Core 1.3 jar, Google Collections 1.0 RC2 jar
    if ArgList._jdbc_type == 'T2':
        #myclasspath = '.:$CLASSPATH'
        myclasspath = ('.:' + gvars.my_ROOT + '/target/test-classes:$CLASSPATH:' +
                       ArgList._jdbc_classpath + ':' + maven_repo +
                       '/junit/junit/4.11/junit-4.11.jar:' + maven_repo +
                       '/org/hamcrest/hamcrest-core/1.3/hamcrest-core-1.3.jar:' + maven_repo +
                       '/com/google/collections/google-collections/1.0-rc2/' +
                       'google-collections-1.0-rc2.jar')
        myoptions = ''
    else:
        myclasspath = '.:'
        myoptions = ''

    print "Phoenix Test Classpath : " + myclasspath + ".\n"
    shell_call(gvars.my_EXPORT_CMD + '; echo "' + myclasspath + '" > phoenix_classpath.log')

    # use Maven to compile the tests and download dependencies but do NOT use Maven for testing
    output = shell_call(gvars.my_EXPORT_CMD + '; mvn test-compile')

    # generate command to run tests without Maven
    gvars.my_RUN_CMD = (os.path.join(ArgList._javahome, 'bin/java') + ' -cp ' + myclasspath +
                        ' ' + myoptions + ' -Duser.timezone=GMT -Dtrafjdbc.properties=' +
                        ArgList._prop_file + ' org.junit.runner.JUnitCore ' +
                        'test.java.org.trafodion.phoenix.end2end.')

    # Generate list of tests if needed
    if ArgList._tests is None:
        testlist = TEST_SUBSET['PART1'] + TEST_SUBSET['PART2']
    else:
        testlist = ArgList._tests.split(",")

    for my_test in testlist:
        echo_start = 'echo "Running test without Maven : ' + my_test + ' ...";'
        if gvars.my_EXPORT_CMD != '':
            cmd = echo_start + gvars.my_EXPORT_CMD + '; ' + gvars.my_RUN_CMD + my_test
        else:
            cmd = echo_start + gvars.my_RUN_CMD + my_test

        print "\nCommand to run : " + cmd + "\n"

        output = shell_call(cmd)
        logfile_write(my_test + '.log', output)

        # generate summary
        generate_summary(output)

    # format gvars.my_TOTAL_EXEC_TIME
    m, s = divmod(gvars.my_TOTAL_EXEC_TIME, 60)
    h, m = divmod(m, 60)
    gvars.my_TOTAL_EXEC_TIME = "%d:%02d:%02ds" % (h, m, s)

# print summary
sumfile_output = '\n---------- SUMARY ----------\n'
sumfile_output += ('Total number of tests run     : ' + str(gvars.my_TOTAL_TESTS_RUN) + '\n')
sumfile_output += ('Total number of failures      : ' + str(gvars.my_TOTAL_TESTS_FAILED) + '\n')
sumfile_output += ('Total number of errors        : ' + str(gvars.my_TOTAL_TESTS_ERRORS) + '\n')
sumfile_output += ('Total number of tests skipped : ' + str(gvars.my_TOTAL_TESTS_SKIPPED) + '\n')
sumfile_output += ('Total execution time          : ' + str(gvars.my_TOTAL_EXEC_TIME) + '\n\n')
if gvars.my_MVN_ERROR_FILE is not None and os.path.isfile(gvars.my_MVN_ERROR_FILE):
    sumfile_output += ('Maven error file              : ' + str(gvars.my_MVN_ERROR_FILE) + '\n\n')
    shell_call('cp ' + gvars.my_MVN_ERROR_FILE + ' ' + gvars.my_ROOT)
stdout_write(sumfile_output)
