#! /usr/bin/env python

# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2013 Hewlett-Packard Development Company, L.P.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
# @@@ END COPYRIGHT @@@

import sys
import subprocess
import fcntl
import select
import errno
import os
import string
import inspect
import optparse

all_tests = [
    'AlterTableTest',
    'ArithmeticQueryTest',
    'AutoCommitTest',
    'BinaryRowKeyTest',
    'CoalesceFunctionTest',
    'CompareDecimalToLongTest',
    'CreateTableTest',
    'CustomEntityDataTest',
    'DeleteRangeTest',
    'DescColumnSortOrderTest',
    'DistinctCountTest',
    'ExecuteStatementsTest',
    'ExtendedQueryExecTest',
    'FunkyNamesTest',
    'GroupByCaseTest',
    'IndexTest',
    'IsNullTest',
    'KeyOnlyTest',
    'MultiCfQueryExecTest',
    'OrderByTest',
    'ProductMetricsTest',
    'QueryExecTest',
    'QueryExecWithoutSCNTest',
    'QueryPlanTest',
    'ReadIsolationLevelTest',
    'SaltedTableTest',
    'SaltedTableUpsertSelectTest',
    'SaltedTableVarLengthRowKeyTest',
    'ServerExceptionTest',
    'SkipScanQueryTest',
    'StatementHintsTest',
    'StddevTest',
    'ToCharFunctionTest',
    'ToNumberFunctionTest',
    'TopNTest',
    'UpsertBigValuesTest',
    'UpsertSelectAutoCommitTest',
    'UpsertSelectTest',
    'UpsertValuesTest',
    'VariableLengthPKTest'
    ] 

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
    _java = None
    _jdbc_classpath = None
    _result_dir = None
    _prop_file = None
    _target_type = None
    _tests = None
    _export_str1 = None
    _export_str2 = None
    _export_str3 = None
    _export_str4 = None
    _export_str5 = None
    _jdbc_type = None

#----------------------------------------------------------------------------
# Tracking all of the global variables in here.
#----------------------------------------------------------------------------
def gvars():
    my_ROOT = None
    my_BUILD_CMD = None
    my_RUN_CMD = None
    my_EXPORT_CMD = None

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
        if rc != None:
            return total_output_stdout + total_output_stderr

#----------------------------------------------------------------------------
# Find the token after s2 in s1
#----------------------------------------------------------------------------
def get_token_after_string(s1, s2, extra_split_chars=None):
    if s2 not in s1:
        return None
    else:
        token = s1[s1.find(s2)+len(s2):].split()[0]
        if extra_split_chars != None:
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
    folders=[]
    while True:
        d, f = os.path.split(d)
        if f:
            folders.insert(0, f)
        else: # f is empty
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
# Generate propfile
#----------------------------------------------------------------------------
def generate_t4_propfile(propfile, target, user, pw, role, dsn, targettype):
    fd = open(propfile, 'w')
    fd.write('url=jdbc:hpt4jdbc://' + target + '/\n')
    fd.write('user=' + user + '\n')
    fd.write('roleName=' + role + '\n')
    fd.write('password=' + pw + '\n')
    fd.write('catalog=seabase\n')
    fd.write('schema=phoenix\n')
    fd.write('serverDataSource=' + dsn + '\n')
    fd.write('targettype=' + targettype + '\n')
    fd.write('sessionName=phoenix\n')
    fd.write('applicationName=phoenix\n')

    fd.close()

def generate_t2_propfile(propfile, targettype):
    fd = open(propfile, 'w')
    fd.write('url=jdbc:sqlmx:\n')
    fd.write('user=DONTCARE\n')
    fd.write('roleName=DONTCARE\n')
    fd.write('password=DONTCARE\n')
    fd.write('catalog=seabase\n')
    fd.write('schema=phoenix\n')
    fd.write('serverDataSource=DONTCARE\n')
    fd.write('targettype=' + targettype + '\n')
    fd.write('sessionName=phoenix\n')
    fd.write('applicationName=phoenix\n')

    fd.close()

#----------------------------------------------------------------------------
# Write to the stdout and/or the log files
#----------------------------------------------------------------------------
def logfile_write(file, output):
    if ArgList._result_dir != None:
        fd = open(os.path.join(ArgList._result_dir, file), 'w')
        fd.write(output)
        fd.close() 

#----------------------------------------------------------------------------
# Parse the argument list for main
#----------------------------------------------------------------------------
def prog_parse_args():
    rec = inspect.stack()[1] # 0 reprents this line,
                             # 1 reprents line at caller,
                             # 2 reprents line at caller's caller,
                             # ... and so on ...
    frame = rec[0]
    info = inspect.getframeinfo(frame)
    gvars.my_ROOT = os.path.dirname(os.path.abspath(info.filename))

    DEFAULT_RESULTS_DIR = os.path.join(gvars.my_ROOT, 'results')
    DEFAULT_JDBC_CLASSPATH = os.path.join(gvars.my_ROOT, 'test/java/lib/hp/hpt4jdbc.jar')
    DEFAULT_PROP_FILE = os.path.join(gvars.my_ROOT, 'jdbcprop')
    
    # alas, the more powerful argparse module only exists in >= 2.7 and >= 3.2,
    # use optparse instead.
    option_list = [
        # No need to add '-h' or '-help', optparse automatically adds one.

        # we do not use short options, hence the first ''.
        # required args
        optparse.make_option('', '--target', action='store', type='string',
          dest='target',
          help='target \'<IP>:<port>\', required argument with no default. This is still needed for jdbc loader even when using sqlci.'),
        optparse.make_option('', '--user', action='store', type='string',
          dest='user',
          help='user id for the target, required argument with no default. It can be any non-empty string if using sqlci.'),
        optparse.make_option('', '--pw', action='store', type='string',
          dest='pw',
          help='password for the target, required argument with no default. It can be any non-empty string if using sqlci.'),

        # optional args
        optparse.make_option('', '--role', action='store', type='string',
          dest='role', default='',
          help='role for the target, defaulted to \'\''),
        optparse.make_option('', '--dsn', action='store', type='string',
          dest='dsn', default='TDM_Default_DataSource',
          help='data source for the target, defaulted to \'TDM_Default_DataSource\''),
        optparse.make_option('', '--java', action='store', type='string',
          dest='java', default='/usr',
          help='java program (version 1.7 required) location, defaulted to \'/usr\''),
        optparse.make_option('', '--jdbccp', action='store', type='string',
          dest='jdbccp', default=DEFAULT_JDBC_CLASSPATH,
          help='jdbc classpath, defaulted to \'<test root>/test/java/lib/hp/hpt4jdbc.jar\', <test_root> is where this program is.'),
        optparse.make_option('', '--resultdir', action='store', type='string',
          dest='resultdir', default=DEFAULT_RESULTS_DIR,
          help='results directory, defaulted to \'<test root>/results\', <test root> is where this program is.'),
        optparse.make_option('', '--propfile', action='store', type='string',
          dest='propfile', default=DEFAULT_PROP_FILE,
          help='results directory, defaulted to automatically generated \'<test root>/t4properties\', <test root> is where this program is.'),
        optparse.make_option('', '--targettype', action='store', type='string',
          dest='targettype', default='TR',
          help='target type, SQ for Seaquest, TR for TRAFODION, defaulted to TR'),
        optparse.make_option('', '--jdbctype', action='store', type='string',
          dest='jdbctype', default='T4',
          help='jdbctype, defaulted to T4'
),
        optparse.make_option('', '--export1', action='store', type='string',
          dest='exportstr1', default='NONE',
          help='any export string, defaulted to NONE'
),
        optparse.make_option('', '--export2', action='store', type='string',
          dest='exportstr2', default='NONE',
          help='any export string, defaulted to NONE'
),
        optparse.make_option('', '--export3', action='store', type='string',
          dest='exportstr3', default='NONE',
          help='any export string, defaulted to NONE'
),
        optparse.make_option('', '--export4', action='store', type='string',
          dest='exportstr4', default='NONE',
          help='any export string, defaulted to NONE'
),
        optparse.make_option('', '--export5', action='store', type='string',
          dest='exportstr5', default='NONE',
          help='any export string, defaulted to NONE'
),
        optparse.make_option('', '--tests', action='store', type='string',
          dest='tests', default='ALL',
          help='specify a subset of tests to run, for example: --tests=AlterTableTest or --tests=AlterTableTest,ArithmeticQueryTest.  Multiple tests can be specified as a string with each test sperated by \',\' without space characters.  When this option is omitted, the default is ALL tests.')
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
                     options.targettype);
    
    if options.jdbctype != 'T2' and options.jdbctype != 'T4':
        parser.error('Invalid --jdbctype.  Only T2 or T4 is supported: ' +
                     options.jdbctype);

    # check for the required args for certain conditions
    if options.jdbctype == 'T4':
        not_found = []
        T4_required_args = ['target', 'user', 'pw']
        for r in T4_required_args:
            if options.__dict__[r] == None:
                not_found.append('--' + r)
        if not_found:
            parser.error('Required option(s) not found: ' + str(not_found))

    # Create the result directory if needed
    result_dir_create(options.resultdir)

    # Automatically generate the prop file if the user did not specify one
    if options.propfile == DEFAULT_PROP_FILE:
        if options.jdbctype == 'T2':
            generate_t2_propfile(options.propfile, options.targettype)
        elif options.jdbctype == 'T4':
            generate_t4_propfile(options.propfile, options.target, options.user, options.pw, options.role, options.dsn, options.targettype)

    ArgList._target = options.target
    ArgList._user = options.user
    ArgList._pw = options.pw
    ArgList._role = options.role
    ArgList._dsn = options.dsn
    ArgList._java = options.java
    ArgList._jdbc_classpath = os.path.abspath(options.jdbccp)
    ArgList._result_dir = os.path.abspath(options.resultdir)
    ArgList._prop_file = os.path.abspath(options.propfile)
    ArgList._target_type = options.targettype
    ArgList._tests = options.tests.split(',')
    ArgList._jdbc_type = options.jdbctype
    ArgList._export_str1 = options.exportstr1
    ArgList._export_str2 = options.exportstr2
    ArgList._export_str3 = options.exportstr3
    ArgList._export_str4 = options.exportstr4
    ArgList._export_str5 = options.exportstr5

    print 'target:                ', ArgList._target
    print 'user:                  ', ArgList._user
    print 'pw:                    ', ArgList._pw
    print 'role:                  ', ArgList._role
    print 'dsn:                   ', ArgList._dsn
    print 'java program location: ', ArgList._java
    print 'jdbc classpath:        ', ArgList._jdbc_classpath
    print 'results directory:     ', ArgList._result_dir
    print 'prop file:             ', ArgList._prop_file
    print 'target type:           ', ArgList._target_type
    print 'jdbc type:             ', ArgList._jdbc_type
    print 'export string 1:       ', ArgList._export_str1
    print 'export string 2:       ', ArgList._export_str2
    print 'export string 3:       ', ArgList._export_str3
    print 'export string 4:       ', ArgList._export_str4
    print 'export string 5:       ', ArgList._export_str5
    print 'test(s):               ', ArgList._tests
   
    sys.stdout.flush()

    gvars.my_EXPORT_CMD = ''
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

    # set up the class path here
    myclasspath = '.:' + ArgList._jdbc_classpath + ':' + gvars.my_ROOT + '/test/java/lib/junit/junit-4.11.jar:' + gvars.my_ROOT + '/test/java/lib/junit/hamcrest-all-1.3.jar:' + gvars.my_ROOT + '/test/java/lib/thirdparty/hbase-0.94.5.jar:' + gvars.my_ROOT + '/test/java/lib/thirdparty/google-collections-1.0-rc2.jar:' + gvars.my_ROOT + '/test/java/lib/thirdparty/commons-logging-1.1.3.jar:' + gvars.my_ROOT + '/test/java/lib/thirdparty/commons-lang-2.6.jar:' + gvars.my_ROOT + '/test/java/lib/thirdparty/guava-14.0.1.jar'

    gvars.my_BUILD_CMD = os.path.join(ArgList._java, 'bin/javac') + ' -classpath ' + myclasspath + ' ' + gvars.my_ROOT + '/test/java/com/hp/phoenix/end2end/*.java'

    gvars.my_RUN_CMD = os.path.join(ArgList._java, 'bin/java') + ' -cp ' + myclasspath + ' -Dhpjdbc.properties=' + ArgList._prop_file + ' org.junit.runner.JUnitCore test.java.com.hp.phoenix.end2end.'


#----------------------------------------------------------------------------
# main
#----------------------------------------------------------------------------
prog_parse_args()

# build the class files
output = shell_call('rm ' + gvars.my_ROOT + '/test/java/com/hp/phoenix/end2end/*.class')
output = shell_call(gvars.my_BUILD_CMD)

total_num_test_groups = 0
total_num_tests_run = 0
total_num_tests_failed = 0
total_exec_time = 0
num_tests_run = 0
num_tests_failed = 0
exec_time = 0
status = ''

if ArgList._tests==['ALL']:
    testlist = all_tests
else:
    testlist = ArgList._tests

sumfile_output = ''

for test in testlist:
    if gvars.my_EXPORT_CMD != '':
        cmd = gvars.my_EXPORT_CMD + ';' + gvars.my_RUN_CMD + test
    else:
        cmd = gvars.my_RUN_CMD + test
    output = shell_call(cmd)
    logfile_write(test + '.log', output)

    lines = output.split('\n')
    num_tests_failed = 0
    for ln in lines:
        if ln.startswith('OK ('):
            status = 'OK' 
            num_tests_run = int(get_token_after_string(ln, 'OK (', ')'))
        elif 'Tests run:' in ln and 'Failures:' in ln:
            num_tests_run = int(get_token_after_string(ln, 'Tests run:', ','))
            num_tests_failed = int(get_token_after_string(ln, 'Failures:')) 
            if num_tests_failed == 1:
                status = str(num_tests_failed) + ' test failed'
            else:
                status = str(num_tests_failed) + ' tests failed'
        elif ln.startswith('Time:'):
            exec_time = float(get_token_after_string(ln, 'Time:').replace(',',''))

    total_num_test_groups += 1
    total_num_tests_run += num_tests_run
    total_num_tests_failed += num_tests_failed  
    total_exec_time += exec_time 
    sumfile_output += (string.ljust(test, 35) + ': ' + status + ' (Tests: ' + string.rjust(str(num_tests_run), 3) + ', Time:' + string.rjust(str(exec_time), 8) + ')\n')

sumfile_output += '\n---------- SUMARY ----------\n'
sumfile_output += ('Total number of test suites:  ' + str(total_num_test_groups) + '\n')
sumfile_output += ('Total number of tests run:    ' + str(total_num_tests_run) + '\n')
sumfile_output += ('Total number of tests failed: ' + str(total_num_tests_failed) + '\n')
sumfile_output += ('Total execution time:         ' + str(total_exec_time) + '\n')
stdout_write(sumfile_output)
logfile_write('phoenix.summary', sumfile_output)

