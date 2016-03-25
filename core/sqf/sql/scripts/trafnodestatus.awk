#
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
#
# awk script used by the trafnodestatus shell-script
# to check the trafodion-status of the nodes 
# of a trafodion cluster.
#
BEGIN {
    lv_header_line_number = 0;
    lv_lines_since_header = 0;
    lv_first = 1;
    lv_got_data = 0;
} 

{
    if (lv_header_line_number > 0) {
	lv_lines_since_header = FNR - lv_header_line_number;
	if (lv_lines_since_header >= 3) {
	    if ( ((lv_lines_since_header - 1) % 2) == 0) {
		lv_node_status = toupper($3);
		if (lv_node_status ~ "UP") {
		    lv_node_name = $8;
		}
		else {
		    lv_node_name = $4;
		}
		if (jason_output == 1) {
		    if (lv_first == 1) { 
			lv_first = 0;
			printf("[");
			lv_got_data = 1;
		    }
		    else {
			printf(",");
		    }
		    printf("{\"NODE\":\"%s\",\"STATUS\":\"%s\"}",
			   lv_node_name,
			   lv_node_status);
		}
		else {
		    printf ("%s [ %s ]\n",
			    lv_node_name,
			    lv_node_status);
		}
	    }
	}
    }
}

/MemFree SwapFree/ {lv_header_line_number = FNR;} 

END {
    if ((jason_output == 1) && 
	(lv_got_data == 1)) {
	printf("]\n");
    }
}
