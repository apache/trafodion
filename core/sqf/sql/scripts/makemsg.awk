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

# This version handles message texts that can cross multiple text lines.
# A backslash character at the end of the line indicates the 
# the continuation to the following line.

# Variable usage description: 
#                 msg_id = the id of the message. 
#                 mode = the mode of operation. Valid mode = 1 or 2.
#                        Mode 1 indicates normal mode while mode 2 indicates
#                        we are in the text continuation mode.
#
BEGIN   { prev_set = -1; msg_id = ""; mode = 1; }

NF == 0 { next }

{

  # The string contains the last character in the input line.
  # Can be used to determine if a line contains a backslash character 
  # at the end.
  last_char_str = substr($0, length($0), 1)

  if ( mode == 1 ) {
    current_set = int($1/10) * 10
    if (current_set != prev_set)
    {
      prev_set = current_set
      print "$set "current_set"\n"
    }
    sub("^[       ][      ]*", "",  $0)   # ' 99 ...'  => '99 ...'
    sub("[        ][      ]*", " ", $0)   # '100  ..'  => '100 ..'

    #remember the message id in case this message has multiple text lines.
    msg_id = $1;

    if ( last_char_str == "\\" ) {
      #switch mode. Insert \n so gencat can break the text line at the end.
      # Also retain the backslash character
      mode = 2;
      print "E_" substr($0, 1, length($0)-1) "\\n\\"
    } else 
      print "E_"$0

  } else { 

    # Processing the follow-on text lines (mode 2). There should be at
    # least one such line.

    if ( last_char_str != "\\" ) {
      # We are at the last follow-on line. Need to switch back to mode 1
      mode =1 
      print $0
    } else
      print substr($0, 1, length($0)-1) "\\n\\"
  }

  # Regardless the mode, if the text line does not have a backslash at the
  # end, we are safe to print out other parts of the message.
  if ( last_char_str != "\\" ) {
      print "C_" msg_id " Cause for Error num : " msg_id
      print "F_" msg_id " Effect of Error num : " msg_id
      print "R_" msg_id " Recovery for Error num : " msg_id "\n"
  }
}
