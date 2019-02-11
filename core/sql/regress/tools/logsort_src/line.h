/*

    LINE.H

    This header file contains prototypes for functions that classify
    lines in a SQLCI log.
                                                                          */

// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@


int line_isstmt(char *line);
char *line_strip(char *line);
int line_isblank(char *line);
int line_is0rows(char *line);
int line_iserror(char *line);
int line_iswarning(char *line);
int line_isheading(char *line);
int line_isunderline(char *line);
int line_isnnrows(char *line);
int line_isignore(char *line);
int line_isstats(char *line);
int line_isoptstats(char *line);
int line_isgetheadingorfooting(char *line);
int line_issqloperationcomplete(char *line);
