/**********************************************************************
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
//
**********************************************************************/
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         SqlciHelp.C
 * RCS:          $Id: SqlciHelp.cpp,v 1.2 1997/04/23 00:30:55  Exp $
 * Description:  
 *               
 *               
 * Created:      2/23/96
 * Modified:     $ $Date: 1997/04/23 00:30:55 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------
// Change history:
// 
// $Log: SqlciHelp.cpp,v $
// Revision 1.2  1997/04/23 00:30:55
// Merge of MDAM/Costing changes into SDK thread
//
// Revision 1.1.1.1.2.1  1997/04/11 23:24:47
// Checking in partially resolved conflicts from merge with MDAM/Costing
// thread. Final fixes, if needed, will follow later.
//
// Revision 1.4.4.1  1997/04/10 18:33:08
// *** empty log message ***
//
// Revision 1.1.1.1  1997/03/28 01:39:44
// These are the source files from SourceSafe.
//
// 
// 6     1/22/97 11:04p 
// Merged UNIX and NT versions.
// 
// 4     1/14/97 4:55a 
// Merged UNIX and  NT versions.
// Revision 1.4  1996/10/11 23:32:30
// Modified the functions for error generation to use ComDiags.
//
// Revision 1.3  1996/05/31 21:16:10
// no change
//
// Revision 1.2  1996/04/05 20:08:03
// Included the standard banner with RCS strings.
// 

#include <iostream>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "SqlciCmd.h"
#include "immudefs.h"
#include "SqlciError.h"
#include "Platform.h"

Help::Help(char * argument_, Lng32 arglen_, help_type type_)
                 : SqlciCmd(SqlciCmd::HELP_TYPE, argument_, arglen_)
{
  type = type_;
};

short Help::process(SqlciEnv * sqlci_env)
{
  // help is not yet supported from MXCI.
  SqlciError (SQLCI_HELP_NOT_FOUND, (ErrorParam *) 0);
  return 0;

#pragma nowarn(269)   // warning elimination 
  char * topic_name = get_argument();
#pragma warn(269)  // warning elimination 
  short  topic_len ;

  short open_err ;
  short read_err ;

  short fnum ;

  char  fname[36] ; // Maximum filename length, including terminating null
  char  pnum[9] ;
  char * alt_msgfile;

  short symbol = 0 ;
  short lang = 0 ;

  enum  { msg_max_len = 200 } ;
  char  help_msg[msg_max_len] ;
  short help_msg_len ;
  short more_lines ;
  char  p1, p2, p3, p4, p5 ;
  short done = 0 ;

  struct IMMUkey_def key ;


  strcpy (pnum, "T9197X00");
  
  alt_msgfile = getenv("SQLMSGFILE");

  if (alt_msgfile)
    strcpy (fname, alt_msgfile);
  else
    strcpy (fname, "$SYSTEM.SYSTEM.SQLMSG");

#pragma nowarn(1506)   // warning elimination 
  topic_len = strlen(topic_name) ;
#pragma warn(1506)  // warning elimination 

  if (topic_len >= len_HelpTopic)
    {
      SqlciError (SQLCI_LONG_HELP_TOPIC, (ErrorParam *) 0);
      return 0 ;
    }

  open_err = IMMU_OPEN_(fname, (short)strlen(fname), &fnum, pnum,
                        &symbol, &lang);

  if (open_err)
    {
      SqlciError (SQLCI_HELP_FOPEN_ERROR,
		  new ErrorParam (open_err),
		  new ErrorParam (&fname[0]) ,
		  (ErrorParam *) 0
		  );
      return 0;
    }

  // Build the key for message file 
  strcpy (key.IMMUkey_product_number, "T9197X00");
  strcpy (key.IMMUkey_record_type, "HL");
  key.IMMUkey_key_field1 = 0 ;
  key.IMMUkey_key_field2 = 1 ; // more 
  
  memset (key.IMMUkey_key_field3, ' ', sizeof(key.IMMUkey_key_field3));

  for (Int32 i=0; i < topic_len; i++)
#pragma nowarn(1506)   // warning elimination 
    topic_name[i] = tolower(topic_name[i]);
#pragma warn(1506)  // warning elimination 
  memcpy (key.IMMUkey_key_field3, topic_name, topic_len) ;

  key.IMMUkey_key_field3[topic_len] = '^' ;

  switch (type)
    {
      case SYNTAX_  : memcpy (&key.IMMUkey_key_field3[topic_len+1], "syntax", 6); 
                      break;
      case EXAMPLE_ : memcpy (&key.IMMUkey_key_field3[topic_len+1], "example", 7);
		      break;
      case DETAIL_  : memcpy (&key.IMMUkey_key_field3[topic_len+1], "detail", 6);
		      break;
    }

  // The key is complete. Read the records now 
  
  done = 0 ;
  while (!done)
    {
      read_err = IMMU_READ_FORMAT_ (fnum, &key, &help_msg[0], msg_max_len,
				    &help_msg_len, symbol, &more_lines,
				    &p1, &p2, &p3, &p4, &p5) ;

      if (read_err)
	{
	  if (read_err == 11) 
	    SqlciError (SQLCI_HELP_NOT_FOUND, (ErrorParam *) 0);
	  else
	    SqlciError (SQLCI_HELP_READ_ERROR,
			new ErrorParam (read_err),
			new ErrorParam (&fname[0]),
			(ErrorParam *) 0
			);
	  done = -1;
	}
      else
	{
	  if (help_msg_len == 0)
	    {
	      cout << "\n" ;
	    } 
	  else
	    {
	      help_msg[help_msg_len] = (char) 0 ;
	      cout << help_msg << "\n" ;
	    }

	  if (!more_lines) done = -1;
	}
    }

  IMMU_CLOSE_ (fnum);

  cout << "Help Command : \n" ;
  cout << "  Topic : " << topic_name << "\n" ; 
  cout << "  Len   : " << strlen(topic_name) << "\n" ;
  cout << "  Type  : " ;
  switch (type)
    {
      case SYNTAX_  : cout << "Syntax\n" ; 
                      break;
      case EXAMPLE_ : cout << "Example\n" ;
		      break;
      case DETAIL_  : cout << "Detail\n" ;
		      break;
      default       : cout << "UNKNOWN type\n" ;
		      break;
    }

  cout << "\n" ;
  
  return 0 ;

};


