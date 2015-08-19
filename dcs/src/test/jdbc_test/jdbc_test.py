#! /usr/bin/env python

# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.
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

#----------------------------------------------------------------------------
# A function (or a structure in C's sense) that contains all variables
# recording user's command-line parameters.
#----------------------------------------------------------------------------
def ArgList():
    _appid = None
    _target = None
    _user = None
    _pw = None
    _role = None
    _dsn = None
    _schema = None
    _javahome = None
    _jdbc_classpath = None
    _jdbc_version = None
    _prop_file = None
    _export_str1 = None
    _export_str2 = None
    _export_str3 = None
    _export_str4 = None
    _export_str5 = None
    _jdbc_type = None
    _tests = None

#----------------------------------------------------------------------------
# Tracking all of the global variables in here.
#----------------------------------------------------------------------------
def gvars():
    my_ROOT = None
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
# Generate pom.xml
#----------------------------------------------------------------------------
def generate_pom_xml(appid, jdbccp, jdbc_version):
    fd1= open(os.path.join(gvars.my_ROOT, 'pom.xml.template'), 'r')
    fd2 = open(os.path.join(gvars.my_ROOT, 'pom.xml'), 'w')
    t, jdbc_artifact_id = jdbc_version.rsplit('.', 1)

    for line in fd1:
        line = string.replace(line, 'MY_APP_ID', appid)
        line = string.replace(line, 'MY_JDBC_VERSION', jdbc_version)
        line = string.replace(line, 'MY_JDBC_ARTIFACT_ID', jdbc_artifact_id.lower())
        line = string.replace(line, 'MY_JDBC_SYS_PATH', jdbccp)
        fd2.write(line)

    fd1.close()
    fd2.close()

#----------------------------------------------------------------------------
# Generate propfile
#----------------------------------------------------------------------------
def generate_t4_propfile(propfile, target, user, pw, role, dsn, schema, appname, jdbc_version):
    fd = open(propfile, 'w')
    fd.write('url=jdbc:t4jdbc://' + target + '/\n')
    fd.write('user=' + user + '\n')
    fd.write('role=' + role + '\n')
    fd.write('password=' + pw + '\n')
    fd.write('catalog=TRAFODION\n')
    fd.write('schema=' + schema + '\n')
    fd.write('serverDataSource=' + dsn + '\n')
    fd.write('trafjdbc_version=' + jdbc_version + '\n')
    fd.write('sessionName=' + appname + '\n')
    fd.write('applicationName=' + appname + '\n')
    fd.write('batchBinding=500\n')

    fd.close()

def generate_t2_propfile(propfile, user, pw, role, dsn, schema, appname, jdbc_version):
    fd = open(propfile, 'w')
    fd.write('url=jdbc:sql:\n')
    fd.write('user=' + user + '\n')
    fd.write('role=' + role + '\n')
    fd.write('password=' + pw + '\n')
    fd.write('catalog=TRAFODION\n')
    fd.write('schema=' + schema + '\n')
    fd.write('serverDataSource=' + dsn + '\n')
    fd.write('trafjdbc_version=' + jdbc_version + '\n')
    fd.write('sessionName=' + appname + '\n')
    fd.write('applicationName=' + appname + '\n')
    fd.write('batchBinding=500\n')

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

    DEFAULT_JDBC_CLASSPATH = '${project.basedir}/lib/jdbcT4.jar'
    DEFAULT_PROP_FILE = os.path.join(gvars.my_ROOT, 'jdbcprop')
    
    # alas, the more powerful argparse module only exists in >= 2.7 and >= 3.2,
    # use optparse instead.
    option_list = [
        # No need to add '-h' or '-help', optparse automatically adds one.

        # we do not use short options, hence the first ''.
        # required args
        optparse.make_option('', '--appid', action='store', type='string',
          dest='appid',
          help='Test application id. For instance, phoenix, jdbc_test, etc. Required argument with no default.'),
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
        optparse.make_option('', '--schema', action='store', type='string',
          dest='schema', default='AUTO',
          help='schema name for the target, defaulted to \'AUTO\'. This means this is automatically generated.'),
        optparse.make_option('', '--javahome', action='store', type='string',
          dest='javahome', default='/usr',
          help='java program (version 1.7 required) location, defaulted to \'/usr\''),
        optparse.make_option('', '--jdbccp', action='store', type='string',
          dest='jdbccp', default=DEFAULT_JDBC_CLASSPATH,
          help='jdbc classpath, defaulted to \'${project.basedir}/lib/jdbcT4.jar\', <test_root> is where this program is.'),
        optparse.make_option('', '--propfile', action='store', type='string',
          dest='propfile', default=DEFAULT_PROP_FILE,
          help='property file, defaulted to automatically generated \'<test root>/jdbcprop\', <test root> is where this program is.'),
        optparse.make_option('', '--jdbctype', action='store', type='string',
          dest='jdbctype', default='T4',
          help='jdbctype, defaulted to T4'),
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

    if options.jdbctype != 'T2' and options.jdbctype != 'T4':
        parser.error('Invalid --jdbctype.  Only T2 or T4 is supported: ' +
                     options.jdbctype);

    # set certain dynamic parameters for JDBC and check for the required args
    if options.jdbctype == 'T4':
        ArgList._jdbc_version = 'org.trafodion.jdbc.t4.T4Driver'
	if options.schema == 'AUTO':
            ArgList._schema = 'T4QA'
	else:
	    ArgList._schema = options.schema

        not_found = []
        T4_required_args = ['target', 'user', 'pw']
        for r in T4_required_args:
            if options.__dict__[r] == None:
                not_found.append('--' + r)
        if not_found:
            parser.error('Required option(s) not found: ' + str(not_found))
    elif options.jdbctype == 'T2':
        ArgList._jdbc_version = 'org.trafodion.sql.T2Driver'
	if options.schema == 'AUTO':
            ArgList._schema = 'T2QA'
	else:
	    ArgList._schema = options.schema


    ArgList._appid = options.appid
    ArgList._target = options.target
    ArgList._user = options.user
    ArgList._pw = options.pw
    ArgList._role = options.role
    ArgList._dsn = options.dsn
    ArgList._javahome = options.javahome
    if options.jdbccp != DEFAULT_JDBC_CLASSPATH:
        options.jdbccp = os.path.abspath(options.jdbccp)
    ArgList._jdbc_classpath = options.jdbccp
    ArgList._prop_file = os.path.abspath(options.propfile)
    ArgList._jdbc_type = options.jdbctype
    ArgList._export_str1 = options.exportstr1
    ArgList._export_str2 = options.exportstr2
    ArgList._export_str3 = options.exportstr3
    ArgList._export_str4 = options.exportstr4
    ArgList._export_str5 = options.exportstr5
    ArgList._tests = options.tests

    # Automatically generate the prop file if the user did not specify one
    if options.propfile == DEFAULT_PROP_FILE:
        if options.jdbctype == 'T2':
            generate_t2_propfile(ArgList._prop_file, ArgList._user, ArgList._pw, ArgList._role, ArgList._dsn, ArgList._schema, ArgList._appid, ArgList._jdbc_version)
        elif options.jdbctype == 'T4':
            generate_t4_propfile(ArgList._prop_file, ArgList._target, ArgList._user, ArgList._pw, ArgList._role, ArgList._dsn, ArgList._schema, ArgList._appid, ArgList._jdbc_version)

    # Generate the pom.xml file from the template according to target type
    generate_pom_xml(ArgList._appid, ArgList._jdbc_classpath, ArgList._jdbc_version)

    print 'appid:                 ', ArgList._appid
    print 'target:                ', ArgList._target
    print 'user:                  ', ArgList._user
    print 'pw:                    ', ArgList._pw
    print 'role:                  ', ArgList._role
    print 'dsn:                   ', ArgList._dsn
    print 'schema:                ', ArgList._schema
    print 'java home:             ', ArgList._javahome
    print 'jdbc classpath:        ', ArgList._jdbc_classpath
    print 'prop file:             ', ArgList._prop_file
    print 'jdbc type:             ', ArgList._jdbc_type
    print 'jdbc version:          ', ArgList._jdbc_version
    print 'export string 1:       ', ArgList._export_str1
    print 'export string 2:       ', ArgList._export_str2
    print 'export string 3:       ', ArgList._export_str3
    print 'export string 4:       ', ArgList._export_str4
    print 'export string 5:       ', ArgList._export_str5
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
# main
#----------------------------------------------------------------------------
prog_parse_args()

# clean the target
output = shell_call('unset JAVA_HOME;' + gvars.my_EXPORT_CMD + '; mvn clean')
stdout_write(output + '\n')

# do the whole build, including running tests
output = shell_call('unset JAVA_HOME;' + gvars.my_EXPORT_CMD + '; mvn test -Dtest=' + ArgList._tests)

lines = output.split('\n')
to_parse = False
num_tests_run=0
num_tests_failed=0
num_tests_errors=0
num_tests_skipped=0
for ln in lines:
    if ln.startswith('Results '):
        to_parse = True
        continue
 
    if to_parse:
        if ln.startswith('Tests run: '):
            num_tests_run = int(get_token_after_string(ln, 'Tests run:', ','))
            num_tests_failed = int(get_token_after_string(ln, 'Failures:', ','))
            num_tests_errors = int(get_token_after_string(ln, 'Errors:', ','))
            num_tests_skipped = int(get_token_after_string(ln, 'Skipped:', ','))

sumfile_output = '\n---------- SUMARY ----------\n'
sumfile_output += ('Total number of tests run:    ' + str(num_tests_run) + '\n')
sumfile_output += ('Total number of failures: ' + str(num_tests_failed) + '\n')
sumfile_output += ('Total number of errors: ' + str(num_tests_errors) + '\n')
sumfile_output += ('Total number of tests skipped: ' + str(num_tests_skipped) + '\n')
stdout_write(sumfile_output)

