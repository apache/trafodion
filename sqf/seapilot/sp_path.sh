# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2012-2014 Hewlett-Packard Development Company, L.P.
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

#Simple file to set up path for Seapilot, separate from other defines

export SP_EXPORT_BIN="${MY_SQROOT}/seapilot/export/bin${SQ_MBTYPE}"
export SP_WORKFLOW_BIN="${MY_SQROOT}/seapilot/scripts/workflows/platform"

export PATH=$SP_EXPORT_BIN:$SP_WORKFLOW_BIN:$MY_SQROOT/seapilot/scripts:/usr/bin:/usr/sbin:/sbin:$PATH


