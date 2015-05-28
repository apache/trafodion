//------------------------------------------------------------------
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2006-2014 Hewlett-Packard Development Company, L.P.
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

//
// Implement eye catcher
//

#ifndef __SB_ECID_H_
#define __SB_ECID_H_

#include <endian.h>
#include <string.h>

typedef long long SB_Ecid_Data_Type;

#if __BYTE_ORDER == __LITTLE_ENDIAN
// little endian
#define sb_ecid(c1,c2,c3,c4,c5,c6,c7,c8) \
  ((static_cast<SB_Ecid_Data_Type>(c8) << 56) | \
   (static_cast<SB_Ecid_Data_Type>(c7) << 48) | \
   (static_cast<SB_Ecid_Data_Type>(c6) << 40) | \
   (static_cast<SB_Ecid_Data_Type>(c5) << 32) | \
   (static_cast<SB_Ecid_Data_Type>(c4) << 24) | \
   (static_cast<SB_Ecid_Data_Type>(c3) << 16) | \
   (static_cast<SB_Ecid_Data_Type>(c2) <<  8) | \
   (static_cast<SB_Ecid_Data_Type>(c1)))
#else
// big-endian
#define sb_ecid(c1,c2,c3,c4,c5,c6,c7,c8) \
  ((static_cast<SB_Ecid_Data_Type>(c1) << 56) | \
   (static_cast<SB_Ecid_Data_Type>(c2) << 48) | \
   (static_cast<SB_Ecid_Data_Type>(c3) << 40) | \
   (static_cast<SB_Ecid_Data_Type>(c4) << 32) | \
   (static_cast<SB_Ecid_Data_Type>(c5) << 24) | \
   (static_cast<SB_Ecid_Data_Type>(c6) << 16) | \
   (static_cast<SB_Ecid_Data_Type>(c7) <<  8) | \
   (static_cast<SB_Ecid_Data_Type>(c8)))
#endif

const SB_Ecid_Data_Type SB_ECID_LRU_T            = sb_ecid('l','r','u','-','t',' ',' ',' ');
const SB_Ecid_Data_Type SB_ECID_MAP_I            = sb_ecid('m','a','p','-','i',' ',' ',' ');
const SB_Ecid_Data_Type SB_ECID_MAP_L            = sb_ecid('m','a','p','-','l',' ',' ',' ');
const SB_Ecid_Data_Type SB_ECID_MAP_LL           = sb_ecid('m','a','p','-','l','l',' ',' ');
const SB_Ecid_Data_Type SB_ECID_MAP_MD           = sb_ecid('m','a','p','-','m','d',' ',' ');
const SB_Ecid_Data_Type SB_ECID_MAP_NPV          = sb_ecid('m','a','p','-','n','p','v',' ');
const SB_Ecid_Data_Type SB_ECID_MAP_S            = sb_ecid('m','a','p','-','s',' ',' ',' ');
const SB_Ecid_Data_Type SB_ECID_MAP_T            = sb_ecid('m','a','p','-','t',' ',' ',' ');
const SB_Ecid_Data_Type SB_ECID_MAP_TIME         = sb_ecid('m','a','p','-','t','i','m','e');
const SB_Ecid_Data_Type SB_ECID_QUEUE            = sb_ecid('q','u','e',' ',' ',' ',' ',' ');
const SB_Ecid_Data_Type SB_ECID_QUEUE_D          = sb_ecid('q','u','e','-','d',' ',' ',' ');
const SB_Ecid_Data_Type SB_ECID_QUEUE_RECV       = sb_ecid('q','u','e','-','r','c','v',' ');
const SB_Ecid_Data_Type SB_ECID_SLOTMGR          = sb_ecid('s','l','o','t','m','g','r',' ');
const SB_Ecid_Data_Type SB_ECID_STREAM_MPI       = sb_ecid('s','t','m','-','m','p','i',' ');
const SB_Ecid_Data_Type SB_ECID_STREAM_SM        = sb_ecid('s','t','m','-','s','m',' ',' ');
const SB_Ecid_Data_Type SB_ECID_STREAM_SOCK      = sb_ecid('s','t','m','-','s','o','c','k');
const SB_Ecid_Data_Type SB_ECID_STREAM_TRANS     = sb_ecid('s','t','m','-','t','r','n',' ');
const SB_Ecid_Data_Type SB_ECID_TABLE_ENTRY_MGR  = sb_ecid('t','b','l','-','m','g','r',' ');

#undef sb_ecid

//
// the generated code for either case seems to be the same,
// but using ia_eye simplifies display
//

// uncomment to use direct union access
//#define SB_USE_ECID_UNION


class SB_Ecid_Type {
public:
    // constructor
    SB_Ecid_Type(SB_Ecid_Data_Type pv_eye) {
#ifdef SB_USE_ECID_UNION
        iv_eye.iv_int = pv_eye;
#else
        memcpy(ia_eye, &pv_eye, sizeof(SB_Ecid_Data_Type));
#endif
    }

    // destructor
    ~SB_Ecid_Type() {
#ifdef SB_USE_ECID_UNION
        iv_eye.ia_char[sizeof(SB_Ecid_Data_Type) - 1] = static_cast<char>(0xdf);
#else
        ia_eye[sizeof(SB_Ecid_Data_Type) - 1] = static_cast<char>(0xdf);
#endif
    }

    // get ecid
    SB_Ecid_Data_Type get_ecid() {
#ifdef SB_USE_ECID_UNION
        return iv_eye.iv_int;
#else
        SB_Ecid_Union_Type *lp_eye;

        lp_eye = reinterpret_cast<SB_Ecid_Union_Type *>(ia_eye);
        return lp_eye->iv_int;
#endif
    }

private:
    typedef union SB_Ecid_Union_Type {
        char              ia_char[sizeof(SB_Ecid_Data_Type)];
        SB_Ecid_Data_Type iv_int;
    } SB_Ecid_Union_Type;

#ifdef SB_USE_ECID_UNION
    SB_Ecid_Union_Type iv_eye;
#else
    char ia_eye[sizeof(SB_Ecid_Data_Type)];
#endif

};

#endif // !__SB_ECID_H_
