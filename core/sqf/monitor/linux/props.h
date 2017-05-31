///////////////////////////////////////////////////////////////////////////////
//
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
///////////////////////////////////////////////////////////////////////////////

#ifndef _MON_PROPS_H_
#define _MON_PROPS_H_

typedef struct mon_ql_type {
    struct mon_ql_type *ip_next;
    union {
        int                i;
        long               l;
    } iv_id;
} MON_QL_Type;

class MON_Smap_Enum;

class MON_Smap {
public:
    MON_Smap();
    virtual ~MON_Smap();

    friend class MON_Smap_Enum;

    virtual bool           empty();
    virtual const char    *get(const char *pp_key);
    virtual void          *getv(const char *pp_key);
    virtual MON_Smap_Enum *keys();
    virtual void           printself(bool pv_traverse);
    virtual void           put(const char *pp_key, const char *pp_value);
    virtual void           putv(const char *pp_key, void *pp_value);
    virtual void           remove(const char *pp_key, char *pp_value);
    virtual void          *removev(const char *pp_key);
    virtual int            size();

protected:
    int                   hash(const char *pp_key);

    enum { TABLE_SIZE = 64 };
    typedef struct {
        MON_QL_Type iv_link;
        char        *ip_key;
        char        *ip_value;
        void        *ip_vvalue;
        bool         iv_use_vvalue;
    } SML_Type;
    SML_Type *iv_HT[TABLE_SIZE+1]; // extra slot for enum
    int       iv_count;
};

class MON_Smap_Enum {
public:
    MON_Smap_Enum(MON_Smap *pp_map);
    virtual ~MON_Smap_Enum();

    bool  more();
    char *next();

private:
    MON_Smap_Enum() {}
    MON_Smap::SML_Type *ip_item;
    MON_Smap           *ip_map;
    int                 iv_hash;

};

class MON_Props : public MON_Smap {
public:
    MON_Props();
    MON_Props(bool pv_getenv);
    virtual ~MON_Props();

    bool load(const char *pp_file);
    bool store(const char *pp_file);

private:
    bool repl_var(char *pp_value);

    bool iv_getenv;
};

#endif // _MON_PROPS_H_
