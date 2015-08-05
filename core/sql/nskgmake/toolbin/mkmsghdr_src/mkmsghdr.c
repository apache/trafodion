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
**********************************************************************/
/*
 *   - This program generates sqlmxmsg_msg.h from the SqlciErrors.txt file.
 *     The older method required the MC.EXE Windows executable, which is
 *     not available on non-Microsoft platforms.
 */

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>

#ifdef _MSC_VER
#  define SECURITY_WIN32 1
#  include <windows.h>
#  include <Security.h>
#else
#  include <unistd.h>
#  include <pwd.h>
#endif

void
usage(void)
{
  fprintf(stderr, "Usage: mkmsghdr -i input_filename -o output_filename\n");
  exit(1);
}

void
process_msgs(char* inbuf, FILE* outfile)
{
  char *p = inbuf;
  char *msg_txt;
  int msg_id;
  int at_end_of_file = 0;
  int reached_end_of_msg;
  int year, month, day, hour, minute;
  char userName[256];
  size_t userNameLen = sizeof(userName);

  const char *filehdr =
      "//\n"
      "//  Values are 32 bit values layed out as follows:\n"
      "//\n"
      "//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1\n"
      "//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0\n"
      "//  +---+-+-+-----------------------+-------------------------------+\n"
      "//  |Sev|C|R|     Facility          |               Code            |\n"
      "//  +---+-+-+-----------------------+-------------------------------+\n"
      "//\n"
      "//  where\n"
      "//\n"
      "//      Sev - is the severity code\n"
      "//\n"
      "//          00 - Success\n"
      "//          01 - Informational\n"
      "//          10 - Warning\n"
      "//          11 - Error\n"
      "//\n"
      "//      C - is the Customer code flag\n"
      "//\n"
      "//      R - is a reserved bit\n"
      "//\n"
      "//      Facility - is the facility code\n"
      "//\n"
      "//      Code - is the facility's status code\n"
      "//\n"
      "//\n"
      "// Define the facility codes\n"
      "//\n\n\n"
      "//\n"
      "// Define the severity codes\n"
      "//\n\n\n";

  const char *msg_fmt1 =
      "//\n"
      "// MessageId: MSG_%d\n"
      "//\n"
      "// MessageText:\n"
      "//\n";

  const char *msg_fmt2 =
      "//\n"
      "#define MSG_%-5d                        0x%08XL\n\n";

  struct tm *tm_p;
  time_t curtime;

  time(&curtime);
  tm_p = localtime(&curtime);
  year = tm_p->tm_year + 1900;
  month = tm_p->tm_mon;
  day = tm_p->tm_mday;
  hour = tm_p->tm_hour;
  minute = tm_p->tm_min;


#ifdef _MSC_VER
  GetUserNameEx(NameSamCompatible, userName, &userNameLen);
#else
  struct passwd *my_pwd;
  my_pwd = getpwuid(getuid());
  if (my_pwd) 
    strncpy(userName, my_pwd->pw_name, userNameLen);
  else
    strcpy(userName, "UnknownUser");
#endif

  /* Write the file header to the output file */
  fprintf(outfile, "%s", filehdr);

  /*
   * Now write MSG_10 to the header.  This contains the time the message file 
   * header was generated and who generated it.
   */
  fprintf(outfile, msg_fmt1, 10);
  fprintf(outfile, "//  00000 99999 UUUUUUUU UUUUU UUUUUUU Message file version: "
                   "{%d-%02d-%02d %02d:%02d %s}.\n",
                   year, month, day, hour, minute, userName);
  fprintf(outfile, msg_fmt2, 10, 10);

  /* Now process the intput file that has been read into a buffer */
  while (*p != '0')
  {
    if (!isdigit(*p))
      break;

    msg_id = atoi(p);

    while (*p != ' ' && *p != '\0')
      p++;

    /* Write the first part of the message header to the output file */
    fprintf(outfile, msg_fmt1, msg_id);

    if (*p != ' ')
    {
      fprintf(stderr, "mkmsghdr: Error in input file for msg %d (1)\n", msg_id);
      break;
    }
    while (*p == ' ')
      p++;

    /* The "p" pointer is now pointing to the first part of the message text.
     * The message text can consist of multiple lines so we need to handle
     * a line at a time in a loop.
     */
    reached_end_of_msg = 0;

    while (!reached_end_of_msg)
    {
      msg_txt = p;
      while (*p != '\n' && *p != '\r' && *p != '\0')
        p++;

      if (p[-1] == '\\')
        p[-1] = '\0';
      else
        reached_end_of_msg = 1;

      if (*p == '\0')
        at_end_of_file = 1;

      *p = '\0';

      fprintf(outfile, "//  %s\n", msg_txt);

      /* Skip any extra new-lines or carriage returns */
      if (!at_end_of_file)
      {
        p++;
        while (*p == '\n' || *p == '\r')
          p++;
      }
    }
    fprintf(outfile, msg_fmt2, msg_id, msg_id);

    /* Skip until a digit is seen */
    while (*p != '\0' && !isdigit(*p))
      p++;
  }
}

int
main(int argc, char **argv)
{
  int i;
  int num_read;
  int num_left;
  char *input_filename = NULL;
  char *output_filename = NULL;
  struct stat stbuf;
  char *buf;
  char *p;
  FILE *in_file;
  FILE *out_file;

  for (i = 1; i < argc; i++)
  {
    if (argv[i][0] != '-')
      usage();

    if (argv[i][1] == 'i')
    {
      if (argv[i][2] != '\0')
        input_filename = &argv[i][2];
      else
        input_filename = argv[++i];
    }
    else if (argv[i][1] == 'o')
    {
      if (argv[i][2] != '\0')
        output_filename = &argv[i][2];
      else
        output_filename = argv[++i];
    }
  }

  if (input_filename == NULL || output_filename == NULL)
    usage();

  if (stat(input_filename, &stbuf) != 0) 
  {
    fprintf(stderr, "mkmsghdr: Unable to stat %s: %s\n",
            input_filename, strerror(errno));
    exit(1);
  }

  if ((buf = (char*)malloc(stbuf.st_size + 1)) == NULL)
  {
    fprintf(stderr, "mkmsghdr: Unable to allocate %d bytes\n",
            stbuf.st_size);
    exit(1);
  }

  if ((in_file = fopen(input_filename, "r")) == NULL)
  {
    fprintf(stderr, "mkmsghdr: Unable to open %s for reading: %s\n",
            input_filename, strerror(errno));
    exit(1);
  }

  p = buf;
  num_left = stbuf.st_size;

  while (num_left > 0)
  {
    num_read = fread(buf, 1, num_left, in_file);
    if (num_read == 0)
      break;
    p += num_read;
    num_left -= num_read;
  }
  fclose(in_file);
  buf[stbuf.st_size] = '\0';

  if ((out_file = fopen(output_filename, "wb")) == NULL)
  {
    fprintf(stderr, "mkmsghdr: Unable to open %s for writing: %s\n",
            output_filename, strerror(errno));
    exit(1);
  }

  process_msgs(buf, out_file);

  return 0;
}
