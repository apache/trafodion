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
#ifndef PARSERMSG_H
#define PARSERMSG_H

/* -*-C++-*-
 *****************************************************************************
 * File:         ParserMsg.h
 * RCS:          $Id: ParserMsg.h,v 1.2 1998/08/10 15:36:19  Exp $
 *****************************************************************************
 */


#include "NAWinNT.h"

#include "charinfo.h"

class ComDiagsArea;

// Single-byte version 
void StoreSyntaxError(const char* input_str, Int32 input_pos,
		      ComDiagsArea& diags, Int32 dgStrNum = 0, 
                      /*  input_str_cs: the charset of input_str */
                      CharInfo::CharSet input_str_cs = CharInfo::ISO88591,
                      /*  terminal_cs : the charset of terminal */
                      CharInfo::CharSet terminal_cs = CharInfo::ISO88591
                     );

// Unicode version 
void StoreSyntaxError(const NAWchar* input_str, Int32 input_pos,
		      ComDiagsArea& diags, Int32 dgStrNum = 0,
                      /*  terminal_cs : the charset of terminal */
                      CharInfo::CharSet terminal_cs = CharInfo::ISO88591);

#endif /* PARSERMSG_H */
