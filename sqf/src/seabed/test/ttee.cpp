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
// ttee [ <file> ]
//
// Like tee, but locks file
//

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <unistd.h>

char la_line[BUFSIZ];

static void fail() {
    char errnobuf[100];

    printf("fatal error, errno=%d(%s)\n",
           errno,
           strerror_r(errno, errnobuf, sizeof(errnobuf)));
    exit(1);
}

static void flock(int file_no, struct flock *lock) {
    int cnt;
    int err;

    lock->l_type = F_WRLCK;
    lock->l_whence = SEEK_SET;
    lock->l_start = 0;
    lock->l_len = 1;
    for (cnt = 0; cnt < 1000; cnt++) {
        err = fcntl(file_no, F_SETLK, lock);
        if (err == 0)
            break;
        usleep(100);
    }
    if (err != 0)
        fail();
}

static void funlock(int file_no, struct flock *lock) {
    int err;

    lock->l_type = F_UNLCK;
    err = fcntl(file_no, F_SETLK, lock);
    if (err != 0)
        fail();
}

int main(int argc, char *argv[]) {
    char         *arg;
    FILE         *file;
    char          errnobuf[100];
    int           file_no;
    struct flock  lock;
    bool          lock_file;
    char         *s;

    if (argc > 1) {
        arg = argv[1];
        file = fopen(arg, "a");
        if (file == NULL) {
            printf("could not open '%s', errno=%d(%s)\n",
                   arg,
                   errno,
                   strerror_r(errno, errnobuf, sizeof(errnobuf)));
            return 1;
        }
        file_no = fileno(file);
        lock_file = true;
    } else {
        file = stdout;
        file_no = -1;
        lock_file = false;
    }
    setvbuf(file, NULL, _IOLBF, 0);
    for (;;) {
        s = fgets(la_line, sizeof(la_line), stdin);
        if (s == NULL)
            break;
        if (lock_file)
            flock(file_no, &lock);
        fputs(s, file);
        if (lock_file)
            funlock(file_no, &lock);
        if (file != stdout)
            fputs(s, stdout);
    }

    return 0;
}
