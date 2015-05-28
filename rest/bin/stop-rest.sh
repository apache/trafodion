#!/usr/bin/env bash
#/**
# *(C) Copyright 2015 Hewlett-Packard Development Company, L.P.
# *
# * Licensed under the Apache License, Version 2.0 (the "License");
# * you may not use this file except in compliance with the License.
# * You may obtain a copy of the License at
# *
# *     http://www.apache.org/licenses/LICENSE-2.0
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# */
# Stop rest daemons.

bin=`dirname "${BASH_SOURCE-$0}"`
bin=`cd "$bin">/dev/null; pwd`

. "$bin"/rest-config.sh

# start rest daemons
errCode=$?
if [ $errCode -ne 0 ]
then
  exit $errCode
fi

"$bin"/rest-daemon.sh --config "${REST_CONF_DIR}" stop rest