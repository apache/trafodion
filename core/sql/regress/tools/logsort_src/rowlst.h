/*

    ROWLST.C

    This header file defines the row list object.

                                                                          */

// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1993-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@

/*  object definitions - all fields considered private  */

struct row_list
   {
   struct row *first;
   struct row *last;
   } ;

/*  function definitions  */

struct row_list *row_list_create(void);
struct row *row_list_remove_min(struct row_list *r);
void row_list_add(struct row_list *r,struct row *rw);
