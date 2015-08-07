#!/bin/sh
#######################################################################
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
#######################################################################
#
# This is a shell script to generate a SET PATTERN statement that can
# later be obeyed in MXCI. The value of the MXCI pattern will be a
# single-quoted string. It turns out to be hard to create quoted
# pattern values in MXCI if you want to include environment variable
# values in the pattern value. This script makes that task easy.
#
# Example: Suppose you want an MXCI pattern in your regression
# test that represents the single-quoted value of environment variable
# $X. Let's say environment variable $X has the value /usr/joe and you
# want MXCI pattern $$XQ$$ to have the value '/usr/joe' so that you can
# use $$XQ$$ in SQL statements where a quoted literal is expected. You
# can accomplish this with something like the following in MXCI:
#
#   sh make-quoted-pattern.ksh XQ $$X$$ > patterns.obey;
#   obey patterns.obey;
#   show pattern;
#
#   The SHOW PATTERN output should include the following line:
#
#     PATTERN $$XQ$$ '/usr/joe'
#
#   Note that in the make-quoted-pattern arguments we could have used
#   a reference to the shell value $X rather than than the mxci pattern
#   $$X$$. In other words, except for subtleties related to embedded
#   quotes or spaces, the following two commands are probably
#   equivalent:
#
#     sh make-quoted-pattern.ksh XQ $$X$$
#     sh make-quoted-pattern.ksh XQ $X
#
# Usage: make-quoted-pattern <pattern name> <pattern value>...
#
#   All arguments after <pattern name> are concatenated to form
#   the value that gets written into the SET PATTERN statement.
#

NAME="$1"
shift

VALUE="$*"

echo set pattern "\$\$${NAME}\$\$" "'''$VALUE''';"

