#ifndef FORMATTER_H
#define FORMATTER_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         Formatter.h
 * RCS:          $Id: Formatter.h,v 1.2 1997/04/23 00:30:10  Exp $
 * Description:  
 *               
 *               
 * Created:      4/15/95
 * Modified:     $ $Date: 1997/04/23 00:30:10 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
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
 *
 *
 *****************************************************************************
 */

#include "BaseTypes.h"
#include "SqlciEnv.h"


class Formatter
{
public:

  enum BufferIt		{ BLANK_SEP_WIDTH = 2 };
  enum ShowNonprinting	{ HEX_EXPANSION_ON = 9, HEX_BUFSIZ_MULTIPLIER = 3 };

  static Int32 buffer_it(SqlciEnv * sqlci_env, char *data,
  		       Int32 datatype,
		       Lng32 length, Lng32 precision, Lng32 scale,
		       char *ind_data,
                         // display length is printed len in single-wide chars  
                       Int32 display_length,
                         // display buffer len may be longer for UTF-8
                       Int32 display_buf_length,
		       Int32 null_flag,
		       char *buf, Lng32 *curpos,
		       NABoolean separatorNeeded      = FALSE,
		       NABoolean checkShowNonPrinting = FALSE);

  static Lng32 display_length(
                       Lng32 datatype,
                       Lng32 length,
                       Lng32 precision,
                       Lng32 scale,
                       Lng32 charsetEnum,
                       Lng32 heading_len,
                       SqlciEnv *sqlci_env,
                       Lng32 *output_buflen);

  static char	getShowNonprintingReplacementChar(NABoolean reeval = FALSE);

  static size_t	showNonprinting(char *s, size_t z, NABoolean varchar);

  #define	showNonprintingCHAR(f)					\
		showNonprinting(f, sizeof(f),  FALSE)

  #define	showNonprintingVARCHAR(v,z)				\
		showNonprinting(v, size_t(z)+1, TRUE)

  #define	showNonprintingCSTRING(c)				\
		showNonprinting(c, strlen(c)+1, TRUE)


  static NABoolean replace8bit_;

private:
};

#endif

