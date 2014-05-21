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

#ifndef __SB_SHMAP_H_
#define __SB_SHMAP_H_

#include <assert.h>    // assert
#include <errno.h>     // errno
#include <exception>   // std::exception
#include <fcntl.h>     // open
#include <limits.h>    // INT_MAX
#include <stdio.h>     // printf
#include <stdlib.h>    // getenv
#include <string.h>    // strcpy
#include <time.h>      // struct timespec
#include <unistd.h>    // getuid

#include <sys/ipc.h>   // IPC_CREAT
#include <sys/mman.h>  // mmap
#include <sys/sem.h>   // semop

// base exception
class SB_ShExcep : public std::exception {
public:
    SB_ShExcep(const char *pp_msg);
    SB_ShExcep(const SB_ShExcep &pr_excep); // copy const
    virtual ~SB_ShExcep() throw ();
    SB_ShExcep &operator =(const SB_ShExcep &pr_excep);
    // called by terminate
    virtual const char *what() const throw ();
protected:
    char *ip_msg;
};

// invalid exception
class SB_ShExcep_Inv : public SB_ShExcep {
public:
    SB_ShExcep_Inv(const char *pp_msg);
    virtual ~SB_ShExcep_Inv() throw ();
};

// sequencing exception
class SB_ShExcep_Seq : public SB_ShExcep {
public:
    SB_ShExcep_Seq(const char *pp_msg);
    virtual ~SB_ShExcep_Seq() throw ();
};



// shared robust semaphore
class SB_ShSem {
public:
    SB_ShSem();
    ~SB_ShSem();

    void create_sem(const char *pp_sem_name);
    void lock();
    int  lock_timed(long pv_ms);
    void unlock();

private:
    int             iv_sem;
    struct timespec iv_ts_start;
    struct timespec iv_ts_stop;
};

enum {      SB_SHMAP_DEFAULT_BUCKETS = 61 };
const float SB_SHMAP_DEFAULT_LF      = 0.75;

// shared map
//
// shared memory layout:
// (space in block is contiguous)
//
//   shmem -> +------------+
//            |   ip_hdr   | // header info
//            .            .
//            .            .
//            +------------+
//            | ip_buckets | // buckets for hashed links
//            .            . // links are chained via index
//            .            .
//            .            .
//            +------------+
//            |  ip_link   | // links allocated here
//            .            .
//            .            .
//            .            .
//            +------------+
//
template <class K, class V>
class SB_ShMap {
public:
    SB_ShMap(const char *pp_map_name,
             int         pv_buckets  = SB_SHMAP_DEFAULT_BUCKETS,
             float       pv_lf       = SB_SHMAP_DEFAULT_LF);
    ~SB_ShMap();

    void      clear();                             // clear all entries
    V        *get(K pv_key);                       // returns pointer to value
    void      get_end()                            // get end
    throw (SB_ShExcep_Seq); 
    V        *get_first()                          // get first
    throw (SB_ShExcep_Seq); 
    V        *get_lock(K    pv_key,                // returns pointer to value
                       bool pv_lock);
    V        *get_next()                           // get next
    throw (SB_ShExcep_Seq); 
    void      lock();                              // lock
    void      print();                             // print map
    void      put(K pv_key, V *pp_value);          // copies V into map
    void      put_lock(K     pv_key,               // copies V into map
                       V    *pp_value,
                       bool  pv_lock);
    V        *remove(K pv_key)                     // returns copy of V
    throw (SB_ShExcep_Inv); 
    V        *remove_lock(K    pv_key,             // returns copy of V
                          bool pv_lock) 
    throw (SB_ShExcep_Inv); 
    int       size();                              // size
    void      unlock();                            // unlock

private:
    enum {
        SB_SHMAP_HDR_VERS =     1,
        SB_SHMAP_MIN_SIZE =    64,
        SB_SHMAP_MAX_SIZE = 65521
    };

    // this goes into shmem
    typedef struct link {
        int iv_inx;           // use indexes, so mmap can move mem around
        int iv_next_inx;      // next index
        int iv_inuse;         // inuse?
        K   iv_key;           // key
        V   iv_value;         // value
    } Link;

    // this goes into shmem
    typedef struct hdr {
        int    iv_hdr_vers;            // hdr vers
        int    iv_hdr_len;             // hdr len
        int    iv_link_len;            // link len
        size_t iv_shmem_len;           // shmem len
        size_t iv_shmem_off;           // shmem offset
        size_t iv_buckets_off;         // buckets offset
        size_t iv_links_off;           // links offset
        int    iv_free_link_inx;       // free-link index
        int    iv_free_link_count;     // free-link count

        int    iv_buckets;             // bucket count
        int    iv_buckets_resize;      // resize
        int    iv_buckets_resize_inx;  //
        int    iv_buckets_threshold;   // threshold
        int    iv_count;               // count
        int    iv_fd;                  // mmap fd
        float  iv_lf;                  // lf
        int    iv_max_map;             // max map count
        char   ia_map_name[100];
    } Hdr;

    // shmem anchors
    typedef struct anchor {
        char  *ip_shmem;             // shared-memory
        Hdr   *ip_hdr;               // hdr
        int   *ip_buckets;           // hash buckets
        Link  *ip_links;             // links
        size_t iv_shmem_len;         // shmem len
    } Anchor;

    // bucket info
    typedef struct bi {
        int     iv_buckets;
        int     iv_buckets_resize;
        int     iv_buckets_resize_inx;
        int     iv_buckets_threshold;
    } BI;

    Link *bucket_link(int pv_hash);
    int   calc_buckets(int pv_buckets);
    void  create_sem(const char *pp_map_name);
    void  get_shmem(const char *pp_map_name,
                    bool        pv_resize,
                    BI         *pp_bi);
    int   hash(K pv_key);
    Link *link_inx_to_link(int pv_inx);
    int   pages(size_t pv_size, long pv_page_size);
    char *remmap_long(const char *pp_where,
                      int         pv_fd,
                      char       *pp_shmem_old,
                      size_t      pv_shmem_old_len,
                      size_t      pv_shmem_new_len,
                      size_t     *pp_shmem_new_len);
    void  remmap_short();
    void  resize(int pv_buckets);
    void  sanity_check();
    void  set_buckets(int pv_buckets, BI *pp_bi);
    void  trace_shmem_hdr(const char *pp_where, Hdr *pp_hdr);

    // access shared mem
    Anchor     iv_shmem_anchor;

    // strictly local
    char      *ip_shmem;
    bool       iv_iter_active;
    float      iv_lf;
    int        iv_shmem_fd;
    size_t     iv_shmem_size;
    SB_ShSem   iv_sem;
    bool       iv_sanity;
    bool       iv_trace;

    // iterator
    class Iter {
    public:
        Iter();
        ~Iter();

        bool  more();
        V    *next();

    private:
        friend class SB_ShMap<K, V>;
        void  reinit(SB_ShMap<K, V> *pp_map);

        Link           *ip_link;
        SB_ShMap<K, V> *ip_map;
        int             iv_count;
        int             iv_hash;
        int             iv_inx;
    };
    Iter iv_iter;
};

// primes
static const int ga_shmap_size[] = {
       61, //      1 - 2^^0
       61, //      2 - 2^^1
       61, //      4 - 2^^2
       61, //      8 - 2^^3
       61, //     16 - 2^^4
       61, //     32 - 2^^5
       61, //     64 - 2^^6
      113, //    128 - 2^^7
      251, //    256 - 2^^8
      509, //    512 - 2^^9
     1021, //   1024 - 2^^10
     2039, //   2048 - 2^^11
     4093, //   4096 - 2^^12
     8179, //   8192 - 2^^13
    16381, //  16384 - 2^^14
    32749, //  32768 - 2^^15
    65521, //  65536 - 2^^16
    65521  // 131072 - 2^^17
};

//
//// from w. richard stevens : unix network programming

//// wait for semaphore #0 to equal 0
//// then increment semaphore #0 by 1 to lock it
//// UNDO to release the lock if process exits before explicitly unlocking
static struct sembuf ga_shsem_op_lock[2] = {
    // semnum, semop, semflg
    {  0,      0,     0        },
    {  0,      1,     SEM_UNDO }
};

// decrement sem #0 by 1 (sets it to 0) to unlock it
static struct sembuf ga_shsem_op_unlock[1] = {
    // semnum, semop, semflg
    {  0,      -1,    SEM_UNDO }
};

SB_ShExcep::SB_ShExcep(const char *pp_msg) {
    if (pp_msg != NULL) {
        ip_msg = new char[strlen(pp_msg)+1];
        strcpy(ip_msg, pp_msg);
    } else
        ip_msg = NULL;
}

SB_ShExcep::SB_ShExcep(const SB_ShExcep &pr_excep)
: std::exception(pr_excep) {
    if (pr_excep.ip_msg != NULL) {
        ip_msg = new char[strlen(pr_excep.ip_msg)+1];
        strcpy(ip_msg, pr_excep.ip_msg);
    } else
        ip_msg = NULL;
}

SB_ShExcep &SB_ShExcep::operator =(const SB_ShExcep &pr_excep) {
    if (this != &pr_excep) {
        if (ip_msg != NULL)
            delete [] ip_msg;
        ip_msg = new char[strlen(pr_excep.ip_msg)+1];
        strcpy(ip_msg, pr_excep.ip_msg);
    }
    return *this;
}

SB_ShExcep::~SB_ShExcep() throw () {
    if (ip_msg != NULL)
        delete [] ip_msg;
}

const char *SB_ShExcep::what() const throw () {
    return ip_msg;
}

SB_ShExcep_Inv::SB_ShExcep_Inv(const char *pp_msg)
: SB_ShExcep(pp_msg) {
}

SB_ShExcep_Inv::~SB_ShExcep_Inv() throw () {
}

SB_ShExcep_Seq::SB_ShExcep_Seq(const char *pp_msg)
: SB_ShExcep(pp_msg) {
}

SB_ShExcep_Seq::~SB_ShExcep_Seq() throw () {
}

// constructor
template <class K, class V>
SB_ShMap<K, V>::SB_ShMap(const char *pp_map_name,
                         int         pv_buckets,
                         float       pv_lf)
: iv_iter_active(false), iv_lf(pv_lf), iv_sanity(false), iv_trace(false) {
    BI  lv_bi;
    int lv_buckets;

    // iv_sanity = true; // debug
    // iv_trace = true;  // debug
    iv_sem.create_sem(pp_map_name);

    lock();

    lv_buckets = calc_buckets(pv_buckets);
    set_buckets(lv_buckets, &lv_bi);
    get_shmem(pp_map_name,
              false, // resize
              &lv_bi);

    unlock();
}

// destructor
template <class K, class V>
SB_ShMap<K, V>::~SB_ShMap() {
    const char *WHERE = "SB_ShMap::~SB_ShMap";

    if (iv_sanity)
        sanity_check();

    if (iv_trace) {
        if (iv_shmem_anchor.ip_hdr != NULL)
            trace_shmem_hdr(WHERE, iv_shmem_anchor.ip_hdr);
    }
}

// get bucket link
template <class K, class V>
typename SB_ShMap<K, V>::Link *SB_ShMap<K, V>::bucket_link(int pv_hash) {
    Link *lp_link;
    int   lv_inx;

    lv_inx = iv_shmem_anchor.ip_buckets[pv_hash];
    lp_link = link_inx_to_link(lv_inx);
    return lp_link;
}

// calculate bucket counts
template <class K, class V>
int SB_ShMap<K, V>::calc_buckets(int pv_buckets) {
    int lv_b2;
    int lv_b2_inx;
    int lv_buckets;

    lv_buckets = pv_buckets;
    if (lv_buckets < SB_SHMAP_MIN_SIZE)
        lv_buckets = SB_SHMAP_MIN_SIZE;
    else if (lv_buckets > SB_SHMAP_MAX_SIZE)
        lv_buckets = SB_SHMAP_MAX_SIZE;
    lv_b2_inx = 0;
    lv_b2 = 1;
    while (lv_b2 < lv_buckets) {
        lv_b2_inx++;
        lv_b2 *= 2;
    }
    lv_buckets = ga_shmap_size[lv_b2_inx];
    return lv_buckets;
}

// get link from index
template <class K, class V>
typename SB_ShMap<K, V>::Link *SB_ShMap<K, V>::link_inx_to_link(int pv_inx) {
    Link *lp_link;

    if (pv_inx >= 0)
        lp_link = &iv_shmem_anchor.ip_links[pv_inx];
    else
        lp_link = NULL;
    return lp_link;
}

// clear map
template <class K, class V>
void SB_ShMap<K, V>::clear() {
    Link *lp_link;
    int   lv_buckets;
    int   lv_hash;

    lock();

    if (iv_shmem_anchor.ip_hdr->iv_shmem_len > iv_shmem_size)
        remmap_short();

    lv_buckets = iv_shmem_anchor.ip_hdr->iv_buckets;
    for (lv_hash = 0; lv_hash < lv_buckets; lv_hash++) {
        for (;;) {
            lp_link = bucket_link(lv_hash);
            if (lp_link == NULL)
                break;
            iv_shmem_anchor.ip_buckets[lv_hash] = lp_link->iv_next_inx;
            lp_link->iv_next_inx = iv_shmem_anchor.ip_hdr->iv_free_link_inx;
            iv_shmem_anchor.ip_hdr->iv_free_link_inx = lp_link->iv_inx;
            iv_shmem_anchor.ip_hdr->iv_free_link_count++;
            lp_link->iv_inuse = 0;
            iv_shmem_anchor.ip_hdr->iv_count--;
        }
    }

    unlock();
}

// using key, get value 
template <class K, class V>
V *SB_ShMap<K, V>::get(K pv_key) {
    return get_lock(pv_key, true);
}

// get end - ends iterator
template <class K, class V>
void SB_ShMap<K, V>::get_end() throw (SB_ShExcep_Seq) {
    if (!iv_iter_active)
        throw SB_ShExcep_Seq("iterator not active");
    iv_iter_active = false;
    unlock();
}

// get first - starts iterator
template <class K, class V>
V *SB_ShMap<K, V>::get_first() throw (SB_ShExcep_Seq) {
    if (iv_iter_active)
        throw SB_ShExcep_Seq("iterator already active");
    lock();
    iv_iter_active = true;
    iv_iter.reinit(this);
    return iv_iter.next();
}

// using key, get value 
template <class K, class V>
V *SB_ShMap<K, V>::get_lock(K pv_key, bool pv_lock) {
    Link *lp_link;
    Link *lp_prev;
    V    *lp_ret;
    int   lv_hash;

    if (pv_lock)
        lock();

    lv_hash = hash(pv_key);
    lp_prev = NULL;
    lp_link = bucket_link(lv_hash);
    while (lp_link != NULL) {
        if (lp_link->iv_key == pv_key) {
            lp_ret = &lp_link->iv_value;
            if (pv_lock)
                unlock();
            return lp_ret;
        }
        lp_prev = lp_link;
        lp_link = link_inx_to_link(lp_link->iv_next_inx);
    }

    if (pv_lock)
        unlock();
    return NULL;
}

// get next - continues iterator
template <class K, class V>
V *SB_ShMap<K, V>::get_next() throw (SB_ShExcep_Seq) {
    if (!iv_iter_active)
        throw SB_ShExcep_Seq("iterator not active");
    return iv_iter.next();
}

// get shared-mem
template <class K, class V>
void SB_ShMap<K, V>::get_shmem(const char *pp_map_name,
                               bool        pv_resize,
                               BI         *pp_bi) {
    const char  *WHERE = "SB_ShMap::get_shmem";
    char         la_buf[1024];
    int         *lp_buckets;
    Link        *lp_free_links;
    Hdr         *lp_hdr;
    Link        *lp_link;
    char        *lp_p;
    char        *lp_page;
    char        *lp_shmem;
    int          lv_err;
    int          lv_exists;
    int          lv_fd;
    int          lv_flags;
    int          lv_free_count;
    int          lv_hash;
    int          lv_inx;
    int          lv_links;
    int          lv_max_map;
    size_t       lv_shmem_size;
    int          lv_page;
    int          lv_pages;
    long         lv_page_size;

    if (pv_resize)
        unlink(pp_map_name); // ignore error

    lv_flags = O_RDWR;
    lv_fd = open(pp_map_name, lv_flags);
    if (lv_fd == -1) {
        lv_exists = false;
        if (iv_trace)
            printf("%s: open failed(O_RDWR), file=%s, errno=%d(%s)\n",
                   WHERE, pp_map_name, errno, strerror(errno));
        if (errno == ENOENT) {
            lv_flags |= O_CREAT;
            lv_fd = open(pp_map_name, lv_flags, 0600);
            if (lv_fd == -1) {
                if (iv_trace)
                    printf("%s: open failed(O_RDWR|O_CREAT), file=%s, errno=%d(%s)\n",
                           WHERE, pp_map_name, errno, strerror(errno));
                sprintf(la_buf, "could not open '%s'", pp_map_name);
                perror(la_buf);
                abort();
            }
        } else {
            sprintf(la_buf, "could not open '%s'", pp_map_name);
            perror(la_buf);
            abort();
        }
    } else {
        lv_exists = true;
    }

    lv_page_size = sysconf(_SC_PAGESIZE);
    if (lv_page_size == -1) {
        perror("sysconf(_SC_PAGESIZE)");
        abort();
    }

    lv_max_map = pp_bi->iv_buckets_resize;
    lv_links = lv_max_map + pp_bi->iv_buckets;
    lv_pages = pages(sizeof(Hdr), lv_page_size) +
               pages(pp_bi->iv_buckets * sizeof(Link *), lv_page_size) +
               pages(lv_max_map * sizeof(Link), lv_page_size);
    lv_shmem_size = lv_pages * lv_page_size;

    if (lv_exists) {
    } else {
        lp_page = static_cast<char *>(malloc(lv_page_size));
        memset(lp_page, 0, lv_page_size);

        for (lv_page = 0; lv_page < lv_pages; lv_page++) {
            lv_err = static_cast<int>(write(lv_fd, lp_page, lv_page_size));
            if (lv_err == -1) {
                sprintf(la_buf, "could not write '%s'",
                        iv_shmem_anchor.ip_hdr->ia_map_name);
                perror(la_buf);
                abort();
            }
        }

        free(lp_page);
    }

    lp_shmem = remmap_long(WHERE,
                           lv_fd,
                           NULL,      // old
                           0,         // old
                           lv_shmem_size,
                           NULL);

    lp_p = lp_shmem;
    lp_hdr = reinterpret_cast<Hdr *>(lp_p);

    if (lv_exists) {
        if (iv_trace) {
            printf("%s: open exists, file=%s\n", WHERE, pp_map_name);
            trace_shmem_hdr(WHERE, lp_hdr);
        }
        if (lp_hdr->iv_hdr_vers != SB_SHMAP_HDR_VERS) {
            printf("hdr version mismatch\n");
            abort();
        }
        if (lp_hdr->iv_hdr_len != sizeof(Hdr)) {
            printf("hdr size mismatch\n");
            abort();
        }
        if (lp_hdr->iv_link_len != sizeof(Link)) {
            printf("link size mismatch\n");
            abort();
        }
        if (lp_hdr->iv_shmem_len > lv_shmem_size) {
            lp_shmem = remmap_long(WHERE,
                                   lv_fd,
                                   lp_shmem,
                                   lv_shmem_size,
                                   lp_hdr->iv_shmem_len,
                                   &lv_shmem_size);
            lp_hdr = reinterpret_cast<Hdr *>(lp_shmem);
        }
        lp_buckets =
          reinterpret_cast<int *>(&lp_shmem[lp_hdr->iv_buckets_off]);
        lp_free_links =
          reinterpret_cast<Link *>(&lp_shmem[lp_hdr->iv_links_off]);

        lv_free_count = 0;
        lv_inx = lp_hdr->iv_free_link_inx;
        while (lv_inx >= 0) {
            lp_link = &lp_free_links[lv_inx];
            lv_free_count++;
            assert(lv_free_count <= lp_hdr->iv_max_map);
            assert(lp_link->iv_inuse == 0);
            lv_inx = lp_link->iv_next_inx;
        }
        assert(lv_inx == -1);
    } else {
        lv_pages = pages(sizeof(Hdr), lv_page_size);
        lp_p += lv_pages * lv_page_size;
        lp_buckets = reinterpret_cast<int *>(lp_p);
        lv_pages = pages(sizeof(Link *) * pp_bi->iv_buckets, lv_page_size);
        lp_p += lv_pages * lv_page_size;
        lp_free_links = reinterpret_cast<Link *>(lp_p);

        memset(lp_hdr, 0, sizeof(*lp_hdr));
        lp_hdr->iv_hdr_vers = SB_SHMAP_HDR_VERS;
        lp_hdr->iv_hdr_len = sizeof(Hdr);
        lp_hdr->iv_link_len = sizeof(Link);
        lp_hdr->iv_shmem_off = 0;
        lp_hdr->iv_shmem_len = lv_shmem_size;
        lp_hdr->iv_buckets_off =
          reinterpret_cast<char *>(lp_buckets) - lp_shmem;
        lp_hdr->iv_links_off =
          reinterpret_cast<char *>(lp_free_links) - lp_shmem;

        lp_hdr->iv_buckets = pp_bi->iv_buckets;
        lp_hdr->iv_buckets_resize = pp_bi->iv_buckets_resize;
        lp_hdr->iv_buckets_resize_inx = pp_bi->iv_buckets_resize_inx;
        lp_hdr->iv_buckets_threshold = pp_bi->iv_buckets_threshold;
        lp_hdr->iv_max_map = lv_max_map;
        lp_hdr->iv_fd = lv_fd;
        lp_hdr->iv_lf = iv_lf;
        strcpy(lp_hdr->ia_map_name, pp_map_name);

        // clear buckets
        for (lv_hash = 0; lv_hash < pp_bi->iv_buckets; lv_hash++)
            lp_buckets[lv_hash] = -1;

        // set free links
        for (lv_hash = 0; lv_hash < lv_max_map; lv_hash++) {
            lp_free_links[lv_hash].iv_inx = lv_hash;
            lp_free_links[lv_hash].iv_next_inx = lv_hash + 1;
            lp_free_links[lv_hash].iv_inuse = 0;
        }
        lp_free_links[lv_max_map - 1].iv_next_inx = -1;
        lp_hdr->iv_free_link_inx = 0;
        lp_hdr->iv_free_link_count = lv_max_map;
    }

    memset(&iv_shmem_anchor, 0, sizeof(iv_shmem_anchor));
    iv_shmem_anchor.ip_hdr = lp_hdr;
    iv_shmem_anchor.ip_buckets = lp_buckets;
    iv_shmem_anchor.ip_shmem = lp_shmem;
    iv_shmem_anchor.ip_links = lp_free_links;
    iv_shmem_anchor.iv_shmem_len = lv_shmem_size;

    ip_shmem = lp_shmem;
    iv_shmem_fd = lv_fd;
    iv_shmem_size = lv_shmem_size;

    if (iv_sanity)
        sanity_check();
}

// calculate hash based on key
template <class K, class V>
int SB_ShMap<K, V>::hash(K pv_key) {
    int lv_buckets;
    int lv_hash;

    lv_buckets = iv_shmem_anchor.ip_hdr->iv_buckets;
    lv_hash = ((unsigned int) (pv_key)) % lv_buckets;
    return lv_hash;
}

// lock map
template <class K, class V>
void SB_ShMap<K, V>::lock() {
    iv_sem.lock();
}

// calculate pages
template <class K, class V>
int SB_ShMap<K, V>::pages(size_t pv_size, long pv_page_size) {
    int lv_pages = static_cast<int>((pv_size + (pv_page_size - 1)) / pv_page_size);
    return lv_pages;
}

// print map
template <class K, class V>
void SB_ShMap<K, V>::print() {
    Link *lp_link;
    int   lv_buckets;
    int   lv_hash;

    printf("map=%p, count=%d, buckets=%d\n",
           (void *) this,
           iv_shmem_anchor.ip_hdr->iv_count,
           iv_shmem_anchor.ip_hdr->iv_buckets);
    lv_buckets = iv_shmem_anchor.ip_hdr->iv_buckets;
    for (lv_hash = 0; lv_hash < lv_buckets; lv_hash++) {
        lp_link = bucket_link(lv_hash);
        while (lp_link != NULL) {
            printf("  bucket[%d]. link=%p, inx=%d, key=%ld, val=%p\n",
                   lv_hash,
                   (void *) lp_link,
                   lp_link->iv_next_inx,
                   lp_link->iv_key,
                   (void *) &lp_link->iv_value);
            lp_link = link_inx_to_link(lp_link->iv_next_inx);
        }
    }
}

// put key/value into map
template <class K, class V>
void SB_ShMap<K, V>::put(K pv_key, V *pp_value) {
    put_lock(pv_key, pp_value, true);
}

// put key/value into map
template <class K, class V>
void SB_ShMap<K, V>::put_lock(K pv_key, V *pp_value, bool pv_lock) {
    Link *lp_link;
    int   lv_hash;
    int   lv_inx;

    if (pv_lock)
        lock();

    if (iv_shmem_anchor.ip_hdr->iv_shmem_len > iv_shmem_size)
        remmap_short();

    lv_inx = iv_shmem_anchor.ip_hdr->iv_free_link_inx;
    lp_link = link_inx_to_link(lv_inx);
    if (lp_link == NULL) {
        printf("map is full\n"); // should not happen
        abort();
    }
    lp_link->iv_key = pv_key;
    memcpy(&lp_link->iv_value, pp_value, sizeof(V));
    iv_shmem_anchor.ip_hdr->iv_free_link_inx = lp_link->iv_next_inx;
    lv_hash = hash(pv_key);
    lp_link->iv_next_inx = iv_shmem_anchor.ip_buckets[lv_hash];
    iv_shmem_anchor.ip_buckets[lv_hash] = lv_inx;
    iv_shmem_anchor.ip_hdr->iv_free_link_count--;
    lp_link->iv_inuse = 1;
    iv_shmem_anchor.ip_hdr->iv_count++;
    if (iv_shmem_anchor.ip_hdr->iv_count >
        iv_shmem_anchor.ip_hdr->iv_buckets_threshold)
        resize(iv_shmem_anchor.ip_hdr->iv_buckets_resize);

    if (iv_sanity)
        sanity_check();
    if (pv_lock)
        unlock();
}

// remmap-long version
template <class K, class V>
char *SB_ShMap<K, V>::remmap_long(const char *pp_where,
                                  int         pv_fd,
                                  char       *pp_shmem_old,
                                  size_t      pv_shmem_old_len,
                                  size_t      pv_shmem_new_len,
                                  size_t     *pp_shmem_new_len) {
    char *lp_shmem;
    int   lv_err;

    if (pp_shmem_old != NULL) {
        if (iv_trace)
            printf("%s: munmap=%p, munmap-len=%ld\n",
                   pp_where, pp_shmem_old, pv_shmem_old_len);
        lv_err = munmap(pp_shmem_old, pv_shmem_old_len);
        if (lv_err == -1) {
            perror("could not munmap");
            abort();
        }
    }

    lp_shmem = static_cast<char *>(mmap(NULL,
                                        pv_shmem_new_len,
                                        PROT_READ | PROT_WRITE,
                                        MAP_SHARED,
                                        pv_fd,  // fd                   
                                        0));    // offset                   
    if (iv_trace)
        printf("%s: shmem=%p, shmem-len=%ld\n",
               pp_where, lp_shmem, pv_shmem_new_len);
    if (lp_shmem == MAP_FAILED) {
        perror("mmap");
        abort();
    }

    if (pp_shmem_new_len != NULL)
        *pp_shmem_new_len = pv_shmem_new_len;

    return lp_shmem;
}

// remmap-short version
template <class K, class V>
void SB_ShMap<K, V>::remmap_short() {
    const char *WHERE = "SB_ShMap::remmap_short";
    char       *lp_shmem;

    lp_shmem = remmap_long(WHERE,
                           iv_shmem_fd,
                           ip_shmem,
                           iv_shmem_size,
                           iv_shmem_anchor.ip_hdr->iv_shmem_len,
                           &iv_shmem_size);
}

// remove key/value from map
template <class K, class V>
V *SB_ShMap<K, V>::remove(K pv_key) throw (SB_ShExcep_Inv) {
    return remove_lock(pv_key, true);
}

// remove key/value from map
template <class K, class V>
V *SB_ShMap<K, V>::remove_lock(K pv_key, bool pv_lock) throw (SB_ShExcep_Inv) {
    Link *lp_link;
    Link *lp_prev;
    V    *lp_ret;
    int   lv_hash;

    if (iv_iter_active)
        throw SB_ShExcep_Seq("remove not allowed during iterator use");

    if (pv_lock)
        lock();

    if (iv_shmem_anchor.ip_hdr->iv_shmem_len > iv_shmem_size)
        remmap_short();

    lv_hash = hash(pv_key);
    lp_prev = NULL;
    lp_link = bucket_link(lv_hash);
    while (lp_link != NULL) {
        if (lp_link->iv_key == pv_key) {
            if (lp_prev == NULL)
                iv_shmem_anchor.ip_buckets[lv_hash] = lp_link->iv_next_inx;
            else
                lp_prev->iv_next_inx = lp_link->iv_next_inx;
            lp_ret = static_cast<V *>(malloc(sizeof(V)));
            memcpy(lp_ret, &lp_link->iv_value, sizeof(V));
            lp_link->iv_next_inx = iv_shmem_anchor.ip_hdr->iv_free_link_inx;
            iv_shmem_anchor.ip_hdr->iv_free_link_inx = lp_link->iv_inx;
            iv_shmem_anchor.ip_hdr->iv_free_link_count++;
            lp_link->iv_inuse = 0;
            iv_shmem_anchor.ip_hdr->iv_count--;
            if (iv_sanity)
                sanity_check();
            if (pv_lock)
                unlock();
            return lp_ret;
        }
        lp_prev = lp_link;
        lp_link = link_inx_to_link(lp_link->iv_next_inx);
    }

    if (iv_sanity)
        sanity_check();
    if (pv_lock)
        unlock();
    return NULL;
}

// resize map
template <class K, class V>
void SB_ShMap<K, V>::resize(int pv_buckets) {
    int   *lp_buckets_old;
    Link  *lp_links_old;
    Link  *lp_link;
    char  *lp_shmem_old;
    BI     lv_bi;
    int    lv_buckets_new;
    int    lv_buckets_old;
    int    lv_err;
    int    lv_fd_old;
    int    lv_hash;
    Link   lv_list;
    int    lv_map_old;
    size_t lv_shmem_old_len;

    lv_buckets_old = iv_shmem_anchor.ip_hdr->iv_buckets;
    lv_buckets_new = calc_buckets(pv_buckets);
    if (lv_buckets_new <= lv_buckets_old)
        return; // forget it

    if (iv_sanity)
        sanity_check();

    set_buckets(lv_buckets_new, &lv_bi);
    lv_map_old = iv_shmem_anchor.ip_hdr->iv_max_map;
    lp_shmem_old = iv_shmem_anchor.ip_shmem;
    lp_buckets_old = iv_shmem_anchor.ip_buckets;
    lv_shmem_old_len = iv_shmem_anchor.iv_shmem_len;
    lv_fd_old = iv_shmem_anchor.ip_hdr->iv_fd;

    // take old links and put on list
    lv_list.iv_next_inx = -1;
    for (lv_hash = 0; lv_hash < lv_buckets_old; lv_hash++) {
        for (;;) {
            lp_link = bucket_link(lv_hash);
            if (lp_link == NULL)
                break;
            iv_shmem_anchor.ip_buckets[lv_hash] = lp_link->iv_next_inx;
            lp_link->iv_next_inx = lv_list.iv_next_inx;
            lv_list.iv_next_inx = lp_link->iv_inx;
        }
    }

    // save old links so put below works
    lp_links_old = iv_shmem_anchor.ip_links;

    // get new shared-mem
    get_shmem(iv_shmem_anchor.ip_hdr->ia_map_name,
              true, // resize
              &lv_bi);

    // put old entries into new map
    for (;;) {
        if (lv_list.iv_next_inx < 0)
            break;
 
        lp_link = &lp_links_old[lv_list.iv_next_inx];
        lv_list.iv_next_inx = lp_link->iv_next_inx;
        put_lock(lp_link->iv_key, &lp_link->iv_value, false);
    }

    // deleted old map
    lv_err = munmap(lp_shmem_old, lv_shmem_old_len);
    if (lv_err == -1) {
        perror("could not munmap");
        abort();
    }
    lv_err = close(lv_fd_old);
    if (lv_err == -1) {
        perror("could not close old mmap fd");
        abort();
    }

    if (iv_sanity)
        sanity_check();
}

// sanity check map
template <class K, class V>
void SB_ShMap<K, V>::sanity_check() {
    Hdr  *lp_hdr;
    Link *lp_link;
    Link *lp_free_links;
    int   lv_buckets;
    int   lv_count;
    int   lv_free_count;
    int   lv_hash;
    int   lv_inx;

    assert(iv_shmem_anchor.ip_shmem != NULL);
    assert(iv_shmem_anchor.ip_hdr != NULL);
    assert(iv_shmem_anchor.ip_buckets != NULL);
    assert(iv_shmem_anchor.ip_links != NULL);
    assert(iv_shmem_anchor.iv_shmem_len != 0);
    lp_hdr = iv_shmem_anchor.ip_hdr;
    lp_free_links = iv_shmem_anchor.ip_links;
    assert(lp_hdr->iv_hdr_vers == SB_SHMAP_HDR_VERS);
    assert(lp_hdr->iv_hdr_len == sizeof(Hdr));
    assert(lp_hdr->iv_link_len == sizeof(Link));
    assert(lp_hdr->iv_lf >= 0);
    assert(lp_hdr->iv_lf <= 1);
    assert(lp_hdr->iv_count >= 0);
    assert(lp_hdr->iv_count <= lp_hdr->iv_max_map);

    lv_buckets = iv_shmem_anchor.ip_hdr->iv_buckets;
    lv_count = 0;
    for (lv_hash = 0; lv_hash < lv_buckets; lv_hash++) {
        lp_link = bucket_link(lv_hash);
        while (lp_link != NULL) {
            assert(lp_link->iv_inuse == 1);
            assert(lp_link->iv_inx >= 0);
            assert(lp_link->iv_inx < lp_hdr->iv_max_map);
            lv_count++;
            assert(lv_count <= lp_hdr->iv_max_map);
            lp_link = link_inx_to_link(lp_link->iv_next_inx);
        }
    }
    assert(lv_count == lp_hdr->iv_count);

    lv_free_count = 0;
    lv_inx = lp_hdr->iv_free_link_inx;
    while (lv_inx >= 0) {
        lv_free_count++;
        assert(lv_inx < lp_hdr->iv_max_map);
        assert(lv_free_count <= lp_hdr->iv_max_map);
        lp_link = &lp_free_links[lv_inx];
        assert(lp_link->iv_inuse == 0);
        assert(lp_link->iv_inx >= 0);
        assert(lp_link->iv_inx < lp_hdr->iv_max_map);
        lv_inx = lp_link->iv_next_inx;
    }
    assert(lv_inx == -1);
    assert(lv_free_count == lp_hdr->iv_free_link_count);
    assert((lv_free_count + lp_hdr->iv_count) == lp_hdr->iv_max_map);
}

// set buckets
template <class K, class V>
void SB_ShMap<K, V>::set_buckets(int pv_buckets, BI *pp_bi) {
    int lv_b2;
    int lv_b2_inx;

    lv_b2_inx = 0;
    lv_b2 = 1;
    while (lv_b2 < pv_buckets) {
        lv_b2_inx++;
        lv_b2 *= 2;
    }
    pp_bi->iv_buckets = pv_buckets;
    pp_bi->iv_buckets_resize_inx = lv_b2_inx + 1;
    pp_bi->iv_buckets_resize = ga_shmap_size[pp_bi->iv_buckets_resize_inx];
    if ((pp_bi->iv_buckets_resize == pv_buckets) &&
        (pp_bi->iv_buckets_resize >= SB_SHMAP_MAX_SIZE))
        pp_bi->iv_buckets_threshold = INT_MAX; // stop growing
    else
        pp_bi->iv_buckets_threshold =
          static_cast<int>(iv_lf * static_cast<float>(pv_buckets));
}

// get size of map
template <class K, class V>
int SB_ShMap<K, V>::size() {
   return iv_shmem_anchor.ip_hdr->iv_count;
}

// trace shmem header
template <class K, class V>
void SB_ShMap<K, V>::trace_shmem_hdr(const char *pp_where, Hdr *pp_hdr) {
    printf("%s: hdr.hdr-vers=%d, .hdr-len=%d, .link-len=%d, .shmem-len=%ld, .shmem-off=%ld, .buckets-off=%ld, .links-off=%ld, .free-inx=%d\n",
           pp_where,
           pp_hdr->iv_hdr_vers,
           pp_hdr->iv_hdr_len,
           pp_hdr->iv_link_len,
           pp_hdr->iv_shmem_len,
           pp_hdr->iv_shmem_off,
           pp_hdr->iv_buckets_off,
           pp_hdr->iv_links_off,
           pp_hdr->iv_free_link_inx);
    printf("%s: hdr.buckets=%d, .-resize=%d, .-resize-inx=%d, .-threshold=%d\n",
           pp_where,
           pp_hdr->iv_buckets,
           pp_hdr->iv_buckets_resize,
           pp_hdr->iv_buckets_resize_inx,
           pp_hdr->iv_buckets_threshold);
    printf("%s: hdr.count=%d, free-count=%d, .fd=%d, .lf=%f, .max-map=%d\n",
           pp_where,
           pp_hdr->iv_count,
           pp_hdr->iv_free_link_count,
           pp_hdr->iv_fd,
           pp_hdr->iv_lf,
           pp_hdr->iv_max_map);
}

// unlock map
template <class K, class V>
void SB_ShMap<K, V>::unlock() {
    iv_sem.unlock();
}

// iterator constructor
template <class K, class V>
SB_ShMap<K, V>::Iter::Iter() {
}

// iterator destructor
template <class K, class V>
SB_ShMap<K, V>::Iter::~Iter() {
}

// iterator more?
template <class K, class V>
bool SB_ShMap<K, V>::Iter::more() {
    return (iv_inx < iv_count);
}

// iterator next
template <class K, class V>
V *SB_ShMap<K, V>::Iter::next() {
    SB_ShMap<K, V>::Link *lp_link;
    V                    *lp_ret;
    int                   lv_buckets;
    int                   lv_hash;
    int                   lv_limit;

    lp_link = ip_link;
    lv_hash = iv_hash;
    if (iv_inx >= iv_count)
        return NULL;
    lv_buckets = ip_map->iv_shmem_anchor.ip_hdr->iv_buckets;
    lv_limit = lv_buckets - 1;
    for (; lv_hash < lv_buckets; lv_hash++) {
        if (lp_link != NULL) {
            iv_inx++;
            break;
        }
        if (lv_hash >= lv_limit)
            lp_link = NULL;
        else
            lp_link = ip_map->bucket_link(lv_hash+1);
    }
    ip_link = lp_link;
    iv_hash = lv_hash;
    if (lp_link == NULL)
        lp_ret = NULL;
    else {
        ip_link = ip_map->link_inx_to_link(lp_link->iv_next_inx);
        lp_ret = &lp_link->iv_value;
    }
    return lp_ret;
}

// reinit iterator
template <class K, class V>
void SB_ShMap<K, V>::Iter::reinit(SB_ShMap<K, V> *pp_map) {
    ip_link = pp_map->bucket_link(0);
    ip_map = pp_map;
    iv_hash = 0;
    iv_inx = 0;
    iv_count = pp_map->size();
}

// sem constuctor
SB_ShSem::SB_ShSem() : iv_sem(-1) {
}

// sem destuctor
SB_ShSem::~SB_ShSem() {
}

// create sem
void SB_ShSem::create_sem(const char *pp_sem_name) {
    char   la_buf[1024];
    char   la_key[PATH_MAX];
    FILE  *lp_f;
    char  *lp_p;
    key_t  lv_sem_key;

    lp_p = getenv("MY_SQROOT");
    if (lp_p == NULL) {
        printf("MY_SQROOT not set\n");
        abort();
    }
    sprintf(la_key, "%s/sem.shmap.%s", lp_p, pp_sem_name);

    lp_f = fopen(la_key, "w");
    if (lp_f == NULL) {
        sprintf(la_buf, "fopen(%s)", la_key);
        perror(la_buf);
        abort();
    }
    fclose(lp_f);

    lv_sem_key = ftok(la_key, getuid());
    if (lv_sem_key == -1) {
        sprintf(la_buf, "ftok(%s)", la_key);
        perror(la_buf);
        abort();
    }
    iv_sem = semget(lv_sem_key, 1, 0600 | IPC_CREAT);
}

// lock sem
void SB_ShSem::lock() {
    int lv_err;

    clock_gettime(CLOCK_REALTIME, &iv_ts_start);
    lv_err = semop(iv_sem, ga_shsem_op_lock, 2);
    if (lv_err == -1) {
        perror("semop-lock");
        abort();
    }
    clock_gettime(CLOCK_REALTIME, &iv_ts_stop);
}

// lock sem (timed)
int SB_ShSem::lock_timed(long pv_ms) {
    int             lv_err;
    int             lv_ret;
    struct timespec lv_time;

    if (pv_ms > 0) {
        if (pv_ms >= 1000)
            lv_time.tv_sec = pv_ms / 1000;
        else
            lv_time.tv_sec = 0;
        lv_time.tv_nsec = (pv_ms - lv_time.tv_sec * 1000) * 1000000;
        clock_gettime(CLOCK_REALTIME, &iv_ts_start);
        lv_err = semtimedop(iv_sem, ga_shsem_op_lock, 2, &lv_time);
        if (lv_err == -1) {
            if (errno == EAGAIN)
                lv_ret = errno;
            else {
                perror("semop-lock-timed");
                abort();
            }
        }
        clock_gettime(CLOCK_REALTIME, &iv_ts_stop);
    } else {
        lock();
        lv_ret = 0;
    }
    return lv_ret;
}

// unlock sem
void SB_ShSem::unlock() {
    int lv_err;

    lv_err = semop(iv_sem, ga_shsem_op_unlock, 1);
    if (lv_err == -1) {
        perror("semop-unlock");
        abort();
    }
}

#endif // !__SB_SHMAP_H_
