// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1997-2015 Hewlett-Packard Development Company, L.P.
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
// PREPROC: start of file: file guard
#ifndef _tandem_feerrors_h_
#define _tandem_feerrors_h_
//*********************************************
// DERROR
//*********************************************

#define fe_base 0
enum {

  FEOK                         = fe_base + 0,     // No error; operation successful
  FEEOF                        = fe_base + 1,     // Read operation reached end-of-file
  FEINVALOP                    = fe_base + 2,     // Operation not allowed on this type of file
  FEPARTFAIL                   = fe_base + 3,     // Failure to open or purge a partition
  FEKEYFAIL                    = fe_base + 4,     // failure to open alternate key file
  FESEQFAIL                    = fe_base + 5,     // Failure to provide sequential buffering
  FESYSMESS                    = fe_base + 6,     // System message received
  FENOSYSMESS                  = fe_base + 7,     // Unable to receive system messages
  FEREADLOCKED                 = fe_base + 9,     // A read through a locked record was successful
  FEBADERR                     = fe_base + 10,    // first of the errors that set CCL
  FEDUP                        = fe_base + 10,    // File or record already exists
  FENOTFOUND                   = fe_base + 11,    // File not in directory or record not in file
  FEINUSE                      = fe_base + 12,    // File is in use
  FEBADNAME                    = fe_base + 13,    // Illegal filename specification
  FENOSUCHDEV                  = fe_base + 14,    // Device does not exist
  FEWRONGDEV                   = fe_base + 15,    // Volume specification to rename does not agree with name of volume on which file resides
  FENOTOPEN                    = fe_base + 16,    // No file with that file number has been opened
  FENOPRIMARY                  = fe_base + 17,    // A paired-open was specified and the file is not opened by the primary process, the parameters supplied do not match the parameters supplied when the file was opened by the primary, or the primary process is not alive
  FENOSUCHSYS                  = fe_base + 18,    // System specified does not exist in network
  FENOMOREDEVS                 = fe_base + 19,    // No more space for devices in logical device table
  FENETVIOL                    = fe_base + 20,    // attempt to open network file with long crtpid
  FEBADCOUNT                   = fe_base + 21,    // Illegal count specified or attempt to transfer too much or too little data
  FEBOUNDSERR                  = fe_base + 22,    // Parameter or buffer address out of bounds
  FEBADADDR                    = fe_base + 23,    // Disk address out of bounds
  FENOTPRIV                    = fe_base + 24,    // Privileged mode required for this operation
  FEWAITFILE                   = fe_base + 25,    // AWAITIO[X] or CANCEL attempted on a waited file
  FENONEOUT                    = fe_base + 26,    // AWAITIO[X] or CANCEL or CONTROL 22 attempted on a file with no outstanding I/O requests
  FEIOPEND                     = fe_base + 27,    // Waited operation attempted when there were outstanding nowait requests
  FETOOMANY                    = fe_base + 28,    // Number of outstanding nowait operations would exceed that specified, an attempt was made to open a disk file or $RECEIVE with the maximum number of concurrent operations more than 1, sync depth exceeds number the opener can handle, or trying to run more than 254 processes from the same object file
  FEMISSPARM                   = fe_base + 29,    // Parameter missing, or mutually exclusive parameters supplied
  FENOLCB                      = fe_base + 30,    // Unable to obtain memory for a message block
  FENOBUFSPACE                 = fe_base + 31,    // Unable to obtain NonStop Services Distribution Service buffer space
  FENOPOOLSPACE                = fe_base + 32,    // Unable to obtain storage pool space
  FENOIOBUFSPACE               = fe_base + 33,    // I/O process is unable to obtain buffer space
  FENOCBSPACE                  = fe_base + 34,    // Unable to obtain NonStop Services Distribution Service control block
  FENOIOCBSPACE                = fe_base + 35,    // Unable to obtain I/O process control block, or the transaction or open lock unit limit has been reached
  FENOPHYSMEM                  = fe_base + 36,    // Unable to lock physical memory; not enough memory available
  FENOIOPHYSMEM                = fe_base + 37,    // I/O process is unable to lock physical memory
  FEWRONGTNS                   = fe_base + 38,    // unable to run on this system (TNS or TNS/II only)
  FEOLDSYNC                    = fe_base + 39,    // The server process received a request with a sync ID older than the set of saved replies
  FETIMEDOUT                   = fe_base + 40,    // Operation timed out before completion
  FEBADSYNC                    = fe_base + 41,    // A checksum error occurred on a file synchronization block
  FEUNALLOC                    = fe_base + 42,    // Attempt to read from unallocated extent
  FENODISCSPACE                = fe_base + 43,    // Unable to obtain disk space for file extent
  FEDIRFULL                    = fe_base + 44,    // Disk directory or destination control table is full
  FEFILEFULL                   = fe_base + 45,    // File is full, or two entries for a process were already in the process-pair directory
  FEINVKEY                     = fe_base + 46,    // Invalid key specified
  FEKEYSBAD                    = fe_base + 47,    // The system is unable to complete an update to a table and its corresponding index; the table and index might be inconsistent
  FESECVIOL                    = fe_base + 48,    // Security violation
  FEACCVIOL                    = fe_base + 49,    // Access violation
  FEDIRERR                     = fe_base + 50,    // Directory error
  FEDIRBAD                     = fe_base + 51,    // Directory is marked bad
  FEDFSERR                     = fe_base + 52,    // error in disc free space table
  FEFSERR                      = fe_base + 53,    // NonStop Services Distribution Service internal error
  FEDFSIO                      = fe_base + 54,    // I/O error in disk free space table or in NonStop Services Data Access Manager undo area
  FEDIRIO                      = fe_base + 55,    // I/O error in disk directory; the file is no longer accessible
  FEVLABIO                     = fe_base + 56,    // I/O error on disk volume label; the volume is no longer accessible
  FEDFSFULL                    = fe_base + 57,    // disc free space table is full
  FEDFSBAD                     = fe_base + 58,    // disc free space table is messed up
  FEFILEBAD                    = fe_base + 59,    // Disk file is bad
  FEWRONGID                    = fe_base + 60,    // The file resides on a removed volume, the device is down or not open, or a server has failed and been replaced by a different process with the same name since it was opened
  FEOPENSTOP                   = fe_base + 61,    // No more file opens permitted on this volume or device
  FEUNEXMOUNT                  = fe_base + 62,    // volume was mounted but no mount command was given
  FEMOUNTEXP                   = fe_base + 63,    // waiting for mount interrupt
  FEMOUNTINPROG                = fe_base + 64,    // mount is in progress on this device
  FESPECONLY                   = fe_base + 65,    // Only special requests permitted
  FEDEVDOWN                    = fe_base + 66,    // Device is down
  FECONTINUE                   = fe_base + 70,    // Continue file operation
  FEDUPREC                     = fe_base + 71,    // Duplicate record encountered
  FEBADPART                    = fe_base + 72,    // Attempted access to a nonexistent partition or a partition to which access is invalid
  FELOCKED                     = fe_base + 73,    // File or record is locked
  FEBADREPLY                   = fe_base + 74,    // Illegal reply number or no space for reply
  FENOTRANSID                  = fe_base + 75,    // Requesting process has no current process transaction identifier
  FEENDEDTRANSID               = fe_base + 76,    // Transaction is in the process of ending
  FEINVTRANSID                 = fe_base + 78,    // Transaction identifier is invalid or obsolete
  FENOTRANSLOCK                = fe_base + 79,    // Attempt by transaction to update or delete a record that it has not previously locked
  FEAUDITINVALOP               = fe_base + 80,    // Invalid operation on audited file or nonaudited disk volume
  FETRANSNOWAITOUT             = fe_base + 81,    // Operation invalid for transaction that still has nowait I/O outstanding on a disk or process file
  FETMFNOTRUNNING              = fe_base + 82,    // All or part of the NonStop Services Transaction Manager is not running
  FETOOMANYTRANSBEGINS         = fe_base + 83,    // Attempt to begin more concurrent transactions than can be handled
  FETMFNOTCONFIGURED           = fe_base + 84,    // NonStop Services Transaction Manager is not configured
  FEDEVICEDOWNFORTMF           = fe_base + 85,    // Device has not been started for the NonStop Services Transaction Manager
  FEBEGINTRDISABLED            = fe_base + 86,    // BEGINTRANSACTION has been disabled
  FEEXPECTEDREAD               = fe_base + 87,    // waiting on a read request and did not get it
  FEINVALIDSEQUENCE            = fe_base + 88,    // a control read is pending new read invalid
  FENODEVBUFSPACE              = fe_base + 89,    // remote device has no buffer available
  FETRANSABRTOWNERDIED         = fe_base + 90,    // Transaction was aborted by system because its parent process died, a server using the transaction failed, or a message to a server using the transaction was cancelled
  FETRANSABRTBADDBID           = fe_base + 91,    // The NonStop Services Transaction Manager failed during commitment of the transaction; the transaction may or may not have been committed
  FETRANSABRTNETDOWN           = fe_base + 92,    // Distributed transaction aborted by system because the path to a remote system that was part of the transaction was down
  FETRANSABRTAUDOVFL           = fe_base + 93,    // Transaction was aborted because it spanned too many audit trail files
  FETRANSABRTOPRCMD            = fe_base + 94,    // Transaction aborted by operator command
  FETRANSABRTDISCTKOVR         = fe_base + 95,    // Transaction aborted because of takeover by NonStop Services Data Access Manager backup process
  FETRANSABRTTIMEOUT           = fe_base + 96,    // Transaction aborted because it exceeded the autoabort timeout duration
  FEABORTEDTRANSID             = fe_base + 97,    // Transaction aborted by call to ABORTTRANSACTION
  FENOMORETCBS                 = fe_base + 98,    // Allocation of a Transaction Control Block failed because the local table is full, or the table on a remote system is full
  FETOOCHEAP                   = fe_base + 99,    // failure to purchase Enscribe
  FENOTRDY                     = fe_base + 100,   // Device not ready or controller not operational
  FENOWRTRING                  = fe_base + 101,   // no write ring in mag tape
  FEPAPEROUT                   = fe_base + 102,   // paper out bail open or end of ribbon
  FEPONNOTRDY                  = fe_base + 103,   // Disk not ready due to power failure
  FENORESPONSE                 = fe_base + 104,   // device not responding
  FEVFUERR                     = fe_base + 105,   // printer VFU error
  FEPRIORWRITEFAIL             = fe_base + 106,   // A buffered write has failed
  FEBREAKONLY                  = fe_base + 110,   // only break mode requests accepted
  FEBREAK                      = fe_base + 111,   // break occurred on this request
  FEOPWRITEABORT               = fe_base + 112,   // read or writeread preempted by operator message
  FEOPTOOMANY                  = fe_base + 112,   // too many user console messages to operator console
  FEBADDEFUSE                  = fe_base + 113,   // invalid DEFINE passed to the tape process
  FEL3T20TO                    = fe_base + 114,   // X25 restart timeout or retry exceeded
  FEL3T22TO                    = fe_base + 115,   // X25 reset timeout or retry exceeded
  FEL3T23TO                    = fe_base + 116,   // X25 clear timeout or retry exceeded
  FEERRTOOBIG                  = fe_base + 119,   // error code > 255 but placed into 8-bit container
  FEDATAPARITY                 = fe_base + 120,   // Data parity error
  FEOVERRUN                    = fe_base + 121,   // data overrun
  FEDATALOSS                   = fe_base + 122,   // Request aborted due to possible data loss caused by takeover by NonStop Services Data Access Manager backup process
  FESUBBUSY                    = fe_base + 123,   // sub-device busy
  FELINERESET                  = fe_base + 124,   // a line reset is in progress
  FEFIRSTDISC                  = fe_base + 130,   // 1st disc error
  FEADDRCHK                    = fe_base + 130,   // illegal address to disc drive
  FEWRTCHECK                   = fe_base + 131,   // write check error from disc
  FESEEKUNC                    = fe_base + 132,   // seek incomplete from disc
  FEACCNOTRDY                  = fe_base + 133,   // access not ready on disc
  FEADDRCOMP                   = fe_base + 134,   // address compare error on disc
  FEWRTPROTECT                 = fe_base + 135,   // attempt to write to protected disc
  FEUNITOWN                    = fe_base + 136,   // unit ownership error ( dual port discs)
  FEBUFPARITY                  = fe_base + 137,   // disc controller buffer parity error
  FEINTERRUPTOVERRUN           = fe_base + 138,   // long error name
  FECTLRERR                    = fe_base + 139,   // controller error
  FELASTDISC                   = fe_base + 139,   // last disc error
  FEMODEMERR                   = fe_base + 140,   // modem disconnected or screwed up
  FEDISCONNECT                 = fe_base + 140,   // link disconnected
  FEMOTIONCHK                  = fe_base + 145,   // card reader motion check error
  FEREADCHK                    = fe_base + 146,   // card reader read check error
  FEINVALHOLRTH                = fe_base + 147,   // card reader invalid Hollerith code read
  FEWORMSEQ                    = fe_base + 148,   // read unwritten sectors or write already written sectors on optical disk.
  FEEOT                        = fe_base + 150,   // end of tape
  FERUNAWAY                    = fe_base + 151,   // runaway tape
  FEOPABORT                    = fe_base + 152,   // unusual end (unit went offline)
  FETAPEPON                    = fe_base + 153,   // tape drive power on
  FEBOT                        = fe_base + 154,   // beginning of tape on BSR or BSF
  FE9TRKTAPEONLY               = fe_base + 155,   // only nine-track magnetic tape permitted
  FEIOPROTOCOLERR              = fe_base + 156,   // I/O protocol violation detected
  FEIOPROCERR                  = fe_base + 157,   // I/O process internal error
  FEBADTHLFUNC                 = fe_base + 158,   // unknown function for THL
  FEWRONGDEVMODE               = fe_base + 159,   // device mode wrong for request
      //  Data communications error numbers:
      //         160 through 189

  FEINVALIDSTATE               = fe_base + 160,   // invalid state/ protocol error
  FEBADSTATE                   = fe_base + 161,   // impossible state
  FERETRIESEXHAUSTED           = fe_base + 162,   // retry count exhausted
  FEOPERATIONTIMEDOUT          = FERETRIESEXHAUSTED,    //operation timed out
  FERECEIVEDEOT                = fe_base + 163,   // eot was received
  FERECEIVEDDISC               = fe_base + 164,   // disconnect received
  FERECEIVEDRVI                = fe_base + 165,   // rvi was received
  FERECEIVEDENQ                = fe_base + 166,   // enq was received
  FERECEIVEDEOTBID             = fe_base + 167,   // eot received on line bid
  FERECEIVEDNAKBID             = fe_base + 168,   // nak received on line bid
  FERECEIVEDWACKBID            = fe_base + 169,   // wack received on line bid
  FENOSEQUENCEID               = fe_base + 170,   // no id sequence received
  FENOTREPLYING                = fe_base + 171,   // no reply received
  FEIMPROPERREPLY              = fe_base + 172,   // reply not proper for protocol
  FEMAXNAKS                    = fe_base + 173,   // too many naks from a 6520/6524 terminal
  FERECEIVEDWACK               = fe_base + 174,   // wack or busy was received
  FEWRONGACK                   = fe_base + 175,   // ack out of sequence
  FEPOLLEND                    = fe_base + 176,   // poll sequence ended
  FETEXTOVERRUN                = fe_base + 177,   // text received exceeds buffer size
  FEADDRERROR                  = fe_base + 178,   // illegal operation for address list or address missing
  FEBADFORMAT                  = fe_base + 179,   // application buffer is incorrect
  FERECEIVEDSTATUS             = fe_base + 180,   // received status
  FERECEIVEDDUPSTATUS          = fe_base + 181,   // received duplicate status
  FENODATA                     = fe_base + 187,   // no data available
  FEDATALOST                   = fe_base + 188,   // data and/or context lost
  FEDATACOMING                 = fe_base + 189,   // further reply is forthcoming
  FEDEVERR                     = fe_base + 190,   // undefined device error
  FEDEVPON                     = fe_base + 191,   // device power on
  FEDEVXERCISED                = fe_base + 192,   // device is being exercised
  FENOMICROCODE                = fe_base + 193,   // invalid or missing microcode file
  FEOPRREJECT                  = fe_base + 194,   // operator rejects the open request
  FENOZSVR                     = fe_base + 195,   // $zsvr went lunch
  FEBADTAPELABEL               = fe_base + 196,   // bad label field on a labeled tape
  FESQLERR                     = fe_base + 197,   // An SQL error occurred
  FEMISSDEF                    = fe_base + 198,   // missing DEFINE
  FEOBIPROTECTED               = fe_base + 199,   // disc file is OBI protected
  FECATERR                     = fe_base + 200,   // first of the path switch errors
  FEOWNERSHIP                  = fe_base + 200,   // device is owned by the other port
  FEPATHDOWN                   = fe_base + 201,   // The current path to the device is down, an attempt was made to write to a nonexistent process, the message-system request was incorrectly formatted, or an error was found in the message system interface
  FEOWNABORT                   = fe_base + 210,   // ownership changed during operation
  FECPUFAIL                    = fe_base + 211,   // cpu failed during operation
//  (Error 212 no longer used)
      

  FECHPARITY                   = fe_base + 213,   // channel data parity error
//  1) CCG on EIO and device status bit <3> = 1 (channel parity
//     error detected by controller)
//  2) CCG on EIO, device status bits <0:3> = 0, and channel status
//     = %000400 (channel parity error on RDST)
//  3) CCG on IIO and interrupt status bit <3> = 1 (channel parity
//     error detected by controller)
//  4) CCG on IIO, interrupt status bit <2> = 1, and IOC transfer
//     status = %13 (channel parity error iobus data to channel)
//  5) CCG on IIO, interrupt status bits <0:3> = 0, and channel
//     status = %000200 or %000400 (channel parity error on RIC or
//     RIST)
      

  FECHTIMEOUT                  = fe_base + 214,   // channel timeout
//  1) CCL on EIO and channel status contains iobus control field
//     where SVI, STI, and PADI are all 0
//  2) CCG on IIO, interrupt status bit <2> = 1, and IOC transfer
//     status = %25 (channel timeout during data transfer)
      

  FEMEMABS                     = fe_base + 215,   // I/O attempted to absent memory page
//  CCG on IIO, interrupt status bit <2> = 1, and IOC transfer
//  status = %30 or %31 (page absent during data transfer or page
//  absent on IOC1, IOC2, or IOC3)
      

  FEMEMBRKPT                   = fe_base + 216,   // memory breakpoint encountered during this I/O
//  CCG on IIO, interrupt status bit <2> = 1, and IOC transfer
//  status = %20, %21, %22, %23, or %24 (memory address breakpoint
//  hit on IOC0, IOC1, IOC2, while reading data, or while writing
//  data)
      

  FEMEMPARITY                  = fe_base + 217,   // memory parity error during this I/O
//  CCG on IIO, interrupt status bit <2> = 1, and IOC transfer
//  status = %11, %12, or %17 (uncorrectable memory error on IOC1,
//  IOC2, or on data word during transfer)
      

  FEINTTIMEOUT                 = fe_base + 218,   // interrupt timeout
//  An interrupt did not occur when expected
      

  FEILLRECON                   = fe_base + 219,   // illegal device reconnect
//  1) CCG on IIO, interrupt status bit <2> = 1, and IOC transfer
//     status = %02, %03, %06, or %07 (zero byte count on write,
//     zero byte count on read, controller command not read or
//     write, or reconnect when status already %01)
//  2) CCG on IIO, interrupt status bits <2> = 1, and IOC transfer
//     status = %00 or %01 (abort with no cause)
//  3) CCG on IIO, interrupt status bits <2> = 1, and IOC transfer
//     status = %36 or %37 (Continuous reconnect detected by
//                          hardware, TXP only)
      

  FEPROTECT                    = fe_base + 220,   // protect violation
//  CCG on IIO, interrupt status bits <2> = 1, and IOC transfer
//  status = %14 (protect bit, IOC0.<0>, on for read)
      

  FEHANDSHAKE                  = fe_base + 221,   // controller handshake violation
//  1) CCL on EIO and channel status contains iobus control field
//     where the combination of SVI, STI, and PADI indicate the
//     handshake which was detected as an error
//  2) CCG on IIO, interrupt status bits <2> = 1, and IOC transfer
//     status = %10, %15, %16, or %26 (channel sent EOT and device
//     did not return STI, channel sent PADO and device did not
//     return PADI, device returned both SVI and STI, or device
//     returned PADI without STI)
      

  FEBADEIOSTAT                 = fe_base + 222,   // bad channel status from EIO instruction
//  1) Device status bit <0> = 1 after issuing take ownership
//     command and retrying twenty times (constant ownership error)
//  2) Device status bit <1> = 1 after retrying twenty times
//     (constant pending interrupt)
//  3) Device status bit <2> = 1 after retrying twenty times
//     (constant busy)
      

  FEBADIIOSTAT                 = fe_base + 223,   // bad channel status from IIO instruction
//  1) CCG, interrupt status bits <2> = 1, and IOC transfer status =
//     %04, %05, %27, %32-37 (undefined status)
//  2) Interrupt status bit <1> = 1 (interrupt bit set following the
//     IIO instruction)
      

  FECTLERR                     = fe_base + 224,   // controller error
  FEUNITPOLLERR                = fe_base + 225,   // no unit or multiple units at same unit #
  FECPUPON                     = fe_base + 230,   // cpu power on during operation
  FEIOPON                      = fe_base + 231,   // controller power on during operation
  FESMONERR                    = fe_base + 232,   // access denied error in communication with SMON
  FESCERROR                    = fe_base + 233,   // SERVERCLASS_SEND_ error
  FENETERR                     = fe_base + 240,   // first of the network errors
  FENETERRLOW                  = fe_base + 240,   // first of the network errors
  FELHRETRY                    = fe_base + 240,   // line handler error didn't get started
  FENETRETRY                   = fe_base + 241,   // network error didn't get started
  FEBYPASSFAIL                 = fe_base + 246,   // FOX direct route failed aborted req
  FELHFAIL                     = fe_base + 248,   // line handler error aborted
  FENETFAIL                    = fe_base + 249,   // network error aborted
  FENETDOWN                    = fe_base + 250,   // all paths to the system are down
  FENETPROTOCOLERROR           = fe_base + 251,   // network protocol error
  FENOEXPANDCLASS              = fe_base + 252,   // required expand class not available
  FELHTOOMANYUPS               = fe_base + 255,   // linehandler belched too often
  FENETERRHIGH                 = fe_base + 255,   // last of the network errors
  FENEWBUFFER                  = fe_base + 305,   // FENEWBUFFER                  
  FEFILEINCONSISTENT           = fe_base + 306,   // crashopen,corrupt or broken flag is set 
      //  Error numbers 257-259 are reserved for use by SIO
      

  FEFIRSTUSER                  = fe_base + 300,   // errors 300 to 511 are reserved for
  FELASTUSER                   = fe_base + 511,   // use by the user.
     // Special DP2 errors


  FEWRONGIDALT                 = fe_base + 599,   // THIS ERROR CODE IS GIVEN BY THE DISC PROCESS TO AN OPEN WHEN THE DISC PROCESS'S COUNT OF FILES OPEN FOR A PROCESS IS LOWER THAN THE FILE SYSTEM'S COUNT AND THE FILE SYSTEM IS A06 OR LATER. THE FILE SYSTEM THEN MARKS ALL ACBS FOR THIS DEVICE AS ACBWRONGID AND RECOMPUTES ITS COUNT AND RETRIES THE OPEN. THIS REQUIRES A COMPATIBLE VERSION OF THE FILE SYSTEM AND DISC PROCESS AND IS VALID FOR DP1 AND DP2.
      //  Error numbers 512 - 540 are reserved for use by SIO
      

  FEINCOMPATIBLEVERSION        = fe_base + 541,   // Data structure version is incompatible with the requested operation
  FENOBLOCKMODE                = fe_base + 549,   // terminal cannot be put into blockmode now
  FEILLPOS                     = fe_base + 550,   // File operation attempted at illegal position
  FEDUPALTKEY                  = fe_base + 551,   // Duplicate exists for insertion-ordered alternate key
  FEFATPIN                     = fe_base + 560,   // The calling or subject process has a pin > 255.
  FEBADITEM                    = fe_base + 561,   // The item code in a list is not recognized
  FEBUFTOOSMALL                = fe_base + 563,   // An output buffer was too small
  FEBADOP                      = fe_base + 564,   // Operation not supported for this file type
  FEILLEGALREQUEST             = fe_base + 565,   // A malformed request was denied
  FEILLEGALREPLY               = fe_base + 566,   // Reply is malformed or not appropriate
  FETOSWRONGFORDEFINE          = fe_base + 567,   // DEFINE used is not supported by this version of the system
  FESEQORDER                   = fe_base + 570,   // the fs-iop sequence numbers indicate that the message was received out of order.
  FESEQDUP                     = fe_base + 571,   // the fs-iop sequence numbers indicate that the message received was a duplicate.
  FESEQRESET                   = fe_base + 572,   // the fs-iop sequence numbers have been reset so the received sequence number cannot be accepted.
  FELDEVPHANDLE                = fe_base + 573,   // Requested process handle cannot be returned
  FEINVALIDKEEPMSG             = fe_base + 586,   // No Dialog Begin msg queued or of wrong kind
  FETOOMANYKEEPS               = fe_base + 587,   // Process already has kept transaction
  FENOKEPTTRANS                = fe_base + 588,   // There is no kept transaction
  FEOUTSTANDINGKEEP            = fe_base + 589,   // Operation cannot be done because of kept trans.
  FEBADPARMVALUE               = fe_base + 590,   // Parameter value is invalid or inconsistent with another
  FEREQABANDONED               = fe_base + 593,   // Request was cancelled
  FEMISSITEM                   = fe_base + 597,   // Required item is missing from an item list
  FESHORTSTACK                 = fe_base + 632,   // Not enough stack space to perform the operation
  FE_MEASURE_RUNNING           = fe_base + 633,   // Measure is active which prevents the operation
  FEFATLDEV                    = fe_base + 634,   // The specified ldev is greater than 16 bits
  FEUNITLOCK                   = fe_base + 635,   // odp was unable to lock a volume in a drive because one of the available drives was already locked.
  FEUSEOLDWAY                  = fe_base + 637,   // used by DP2 in SETCLOSE
  FEQUEUEDSTOPMODE1            = fe_base + 638,   // Process cannot be stopped until process returns to stopmode 1
  FEQUEUEDSTOPMODE0            = fe_base + 639,   // Process cannot be stopped until process goes to stopmode 0
  FE_DBG_NOWREQUIRED           = fe_base + 640,   // Use DEBUGNOW (instead of DEBUG) for priv process
  FENOSWITCH                   = fe_base + 899,   // Attempt to switch cpus with PUP PRIMARY command failed. Used by DP2.
      //  Error numbers 700 - 750 are reserved for use by TMF3/DP2
      

  FEWRONGRECOVSEQNUM           = fe_base + 700,   // Generated by the DP if the message sequence number does not match expected sequence number
  FEDEVICEDOWNFORTRANSACTIONS  = fe_base + 701,   // Generated by the DP when the device is down but is supposed to be up
  FEUNSUPPORTEDOP              = fe_base + 702,   // Generated by the DP when it receives a physical REDO req. and the request specifies that audit be generated
  FEAUDITTOONEW                = fe_base + 703,   // Generated by the DP during recovery if the CRVSN in the audit trail is is more recent than the FLAB CRVSN
  FEPVSNMISMATCH               = fe_base + 704,   // Generated by the DP when the PVSN in the audit trail record does not match the VSN of the data block on disk
  FENOTANADP                   = fe_base + 705,   // Generated by the DP when it receives an ADP request and it is not an ADP volume
  FEINVALFORADP                = fe_base + 706,   // Generated when the request to the Auditing Disk Process is invalid.
  FEINVALIDDPNAMETIMESTAMP     = fe_base + 707,   // Generated by the DP when the DpName Time Stamp sent by the recovery process does not match the current DpName Time Stamp
  FEFILEUNDONEEDED             = fe_base + 708,   // Returned by the DP when a file was encountered in the Redo/Undo request that had its UndoNeeded flag set and the request specified that the UndoNeeded flag must not be set
  FEFILEREDONEEDED             = fe_base + 709,   // Returned by the DP when a file was encountered in the Redo/Undo request that had its RedoNeeded flag set and the request specified that the RedoNeeded flag must not be set
  FENULLOP                     = fe_base + 710,   // No Further work needs to be done to process the current portion of the current request
  FEINVALAUDITREC              = fe_base + 711,   // Returned by the DP when it finds an audit record whose length word does not match

//----------------------------------------------------------------------------
//
//                                    OPENTMF ERRORS
//
//----------------------------------------------------------------------------
  FERMALREADYREGISTERED        = fe_base + 712,      // The resource manager is already registered
   FERMOUTSTANDINGTRANS        = fe_base + 713,      // An attempt was made to remove a recoverable resource manager which still has unresolved transactions
   FEINVALIDPROTOCOL           = fe_base + 714,      // Invalid or unsupported protocol option
   FEINVALIDTXHANDLE           = fe_base + 715,      // Invalid Transaction handle specified
   FETXSUSPENDREJECTED         = fe_base + 716,      // The suspend operation is rejected because process is not the beginner of the transaction nor did it resume the transaction
   FETXNOTSUSPENDED            = fe_base + 717,      // The transaction has not been suspended
   FEINVALIDSIGNAL             = fe_base + 718,      // Invalid signal supplied
   FEDATASIZEEXCEEDED          = fe_base + 719,      // The branch data exceeded the maximum allowable limit
   FERMWRONGSESSION            = fe_base + 720,      // There is a mismatch in the session between the resource manager and CLOSE/RECOVER messages
   FEBEGINTXNOTCOMPLETED       = fe_base + 721,      // Begin transaction is not yet complete
   FENOMORERMCBS               = fe_base + 722,      // There are no more empty resource manager control blocks
   FENOMOREBCBS                = fe_base + 723,      // There are no more empty branch control blocks
   FENOTNOWAITTFILE            = fe_base + 724,      // The TFILE is not opened in nowait mode
   FEIMPORTINVALOP             = fe_base + 725,      // Invalid operation on an imported transaction ( for example, calling ENDTRANSACTION )
   FEINVALIDTUBADDR            = fe_base + 726,      // Invalid TUB address specified for the branch
   FETOOMANYRECRMS             = fe_base + 727,      // The total allowable recoverable resource managers for a system has been exceeded
   FESETTXHANDLEINVALOP        = fe_base + 728,      // The volatile resource manager has already prepared the branch. If the volatile resource manager has to do more work on the transaction, it must export a transaction branch
   FEBRANCHISPREPARED          = fe_base + 729,      // After a transaction branch is prepared, its branch data will not be allowed to be updated
   FEJOINSOUTSTANDING          = fe_base + 730,      // There are outstanding JOINs for the process. This error is given during ENDTRANSACTION processing
   FEALREADYJOINED             = fe_base + 731,      // The transaction is already joined by the process
   FEALREADYRESUMED            = fe_base + 732,      // The transaction is already resumed by the process
   FEBRANCHISFAILED            = fe_base + 733,      // The transaction branch has already failed. This error is given if branch data is updated while transaction is being aborted

//----------------------------------------------------------------------------
//                Trafodion transaction errors
//----------------------------------------------------------------------------
   FETRANSEXCEPTION     = fe_base + 734,	// The transaction operation failed with exception.
   FETRANSIOEXCEPTION	= fe_base + 735,	// The transaction operation failed with I/O exception.
   FEHASCONFLICT        = fe_base + 736,	// MVCC Conflict detected in transaction during prepare.
   FETRANSERRUNKNOWN    = fe_base + 737,	// An unknown transaction error occurred.

//----------------------------------------------------------------------------
//
//                TRANSACTION INTERNET PROTOCOL (TIP) ERRORS
//
//----------------------------------------------------------------------------
   FETRANSNOTPUSHED           = fe_base + 751,      // Transaction not pushed
   FETRANSNOTPULLED           = fe_base + 752,      // Transaction not pulled
   FETRANSALREADYPULLED       = fe_base + 753,      // Transaction was already pulled
   FESERVICEDISABLED          = fe_base + 754,      // Service disabled
   FERETRY                    = fe_base + 755,      // Retry the operation

      //  Error numbers 900 - 950 are reserved for use by PATHSEND
      

  FESCFIRSTERROR               = fe_base + 900,   // First available PATHSEND Error.
  FESCINVALIDSERVERCLASSNAME   = fe_base + 900,   // Invalid server class name
  FESCINVALIDPATHMONNAME       = fe_base + 901,   // Invalid PATHMON process name
  FESCINVALIDMONITORNAME       = fe_base + 901,   // FESCINVALIDMONITORNAME      
  FESCPATHMONCONNECT           = fe_base + 902,   // Error with PATHMON connection (such as open or I/O error)
  FESCMONITORCONNECT           = fe_base + 902,   // FESCMONITORCONNECT          
  FESCPATHMONMESSAGE           = fe_base + 903,   // Unknown message received from monitor
  FESCMONITORMESSAGE           = fe_base + 903,   // FESCMONITORMESSAGE          
  FESCSERVERLINKCONNECT        = fe_base + 904,   // Error with server link connection (such as open or I/O error)
  FESCNOSERVERAVAILABLE        = fe_base + 905,   // No server available
  FESCNOSENDEVERCALLED         = fe_base + 906,   // User called SC_SEND_INFO_ before ever calling SC_SEND_
  FESCINVALIDSEGMENTID         = fe_base + 907,   // The caller is using an extended segment id that is out of range.
  FESCNOSEGMENTINUSE           = fe_base + 908,   // The caller supplied a reference parameter which is an extended address, but doesn't have an extended segment in use.
  FESCINVALIDFLAGSVALUE        = fe_base + 909,   // Caller set bits in the flags parameter that are reserved and must be 0
  FESCMISSINGPARAMETER         = fe_base + 910,   // Required parameter was not supplied
  FESCINVALIDBUFFERLENGTH      = fe_base + 911,   // A buffer length parameter is invald
  FESCPARAMETERBOUNDSERROR     = fe_base + 912,   // A reference parameter is out of bounds
  FESCSERVERCLASSFROZEN        = fe_base + 913,   // The server class is frozen
  FESCSERVERCLASSSUSPENDED     = fe_base + 913,   // FESCSERVERCLASSSUSPENDED    
  FESCUNKNOWNSERVERCLASS       = fe_base + 914,   // Server monitor does not recognize server class name
  FESCPATHMONSHUTDOWN          = fe_base + 915,   // Send denied because PATHMON is shutting down
  FESCMONITORSHUTDOWN          = fe_base + 915,   // FESCMONITORSHUTDOWN         
  FESCSERVERCREATIONFAILURE    = fe_base + 916,   // Send denied because of a server creation failure
  FESCSERVERCLASSTMFVIOLATION  = fe_base + 917,   // Transaction mode of the NonStop Services Transaction Manager does not match that of the server class
  FESCOPERATIONABORTED         = fe_base + 918,   // Send operation aborted.  See accompanying error for more information
  FESCINVALIDTIMEOUTVALUE      = fe_base + 919,   // Caller supplied an invalid timeout value
  FESCPFSUSEERROR              = fe_base + 920,   // Caller's process file segment could not be accessed
  FESCTOOMANYPATHMONS          = fe_base + 921,   // Maximum number of PATHMON processes allowed has been exceeded
  FESCTOOMANYSERVERCLASSES     = fe_base + 922,   // Maximum number of server classes allowed has been exceeded
  FESCTOOMANYSERVERLINKS       = fe_base + 923,   // Maximum number of server links allowed has been exceeded
  FESCTOOMANYSENDREQUESTS      = fe_base + 924,   // Maximum number of send requests allowed has been exceeded
  FESCTOOMANYREQUESTERS        = fe_base + 925,   // Maximum number of requesters allowed has been exceeded
  FESCDIALOGINVALID            = fe_base + 926,   // Dialog ID is not valid
  FESCTOOMANYDIALOGS           = fe_base + 927,   // Requester has too many dialogs
  FESCOUTSTANDINGSEND          = fe_base + 928,   // Requester already has a send outstanding on the currently open dialog
  FESCDIALOGABORTED            = fe_base + 929,   // Dialog aborted due to server termination or loss of communication between sends
  FESCCHANGEDTRANSID           = fe_base + 930,   // Requester tried to send using a transaction other than the one active when the dialog was started
  FESCDIALOGENDED              = fe_base + 931,   // Server ended the dialog
  FESCDIALOGNOTDSERVER         = fe_base + 932,   // Dialog server is not a DSERVER
  FESCDIALOGOUTSTANDING        = fe_base + 933,   // Server has not ended the dialog
  FESCTRANSACTIONABORTED       = fe_base + 934,   // Dialog's transaction was aborted
  FESCROUTERRESOURCE           = fe_base + 940,   // Insufficient router resources
  FESCDISTRESOURCE             = fe_base + 941,   // Insufficient distributor resources
  FESCSERVERCLASSSTOPPED       = fe_base + 942,   // Server class is stopped
  FESCFUNCTIONNOTSUPPORTED     = fe_base + 943,   // Function not supported (likely indicates a software version problem)
  FESCROUTERMESSAGE            = fe_base + 944,   // Distributor received unknown message from router
  FESCDISTRIBUTORCONNECT       = fe_base + 945,   // Error with distributor connection (such as open or I/O error)
  FESCTOOMANYTESTPOINTUSERS    = fe_base + 946,   // Maximum number of test point users allowed has been exceeded
  FESCLINKMONCONNECT           = fe_base + 947,   // Error with link monitor connection (such as open or I/O error)
  FESCTIMESTAMPINVALID         = fe_base + 948,   // Pathsend internal error
  FESCRETRYOPEN                = fe_base + 949,   // Pathsend internal error
  FESCLASTERROR                = fe_base + 950,   // Last available PATHSEND error.
      //***************************************************************************

  FE_EXCEPTION_RESPONSE_MODE   = fe_base + 951,   // Error occurred in exception response
  FENTEVENT                    = fe_base + 953,   // Windows NT event detected
  FENTIOCOMPLETION             = fe_base + 954,   // Windows NT I/O completion routine executed
  FENTERRORLOGGED              = fe_base + 955,   // Windows NT error logged by NonStop Software component
  FENTERROR                    = fe_base + 956,   // Windows NT error returned by TDM_NTAWAITIOX_
      // Error numbers 1000-1019 are reserved for use by the Tapeprocess
      

  FEASSIGNEDELSEWHERE          = fe_base + 1000,  // The device has been secured by/for another host.
  FEXSUMREADOK                 = fe_base + 1001,  // TapeProcess is in "checksum" mode and the read finished OK.

  FENOSELFREFERENCE            = fe_base + 1191,  // Lock escalated, cannot detect FELOCKSELF
  FELOCKSELF                   = fe_base + 1192,  // Requestor already locked row, self-ref insert.
  
      // Error numbers 3501 - 3550 are explained in much greater detail in the
      // Control Information Formatting Standard.
      

  FETOOMUCHCONTROLINFO         = fe_base + 3501,  // Indicates that control information exceeded maximum for the specified dialect.
  FEREQUESTALLOCATIONFAILURE   = fe_base + 3502,  // Indicates that storage was not available at the server to process the request because of the demands of other activities at the time the request was received.
  FEDIALECTUNSUPPORTED         = fe_base + 3503,  // Indicates that the addressee does not support the dialect of a received request.
  FEREQUESTUNSUPPORTED         = fe_base + 3504,  // Indicates that the addressee (server) supports the dialect but not the request type of the request.
  FESERVERVERSIONTOOLOW        = fe_base + 3505,  // Indicates that the server determined that the version recorded in MINIMUM_INTERPRETATION_VERSION was higher than its own native version.
      // The FE error number range of 4000-4999 are reserved for OSS
      // errors and include the "C" Language errors.
      // These error should never be overloaded with non-OSS meaning.

  FE_EPERM                     = fe_base + 4001,  // Not owner permission denied
  FE_ENOENT                    = fe_base + 4002,  // No such file or directory
  FE_ESRCH                     = fe_base + 4003,  // No such process or table entry
  FE_EINTR                     = fe_base + 4004,  // Interrupted system call
  FE_EIO                       = fe_base + 4005,  // I/O error
  FE_ENXIO                     = fe_base + 4006,  // No such device or address
  FE_E2BIG                     = fe_base + 4007,  // Argument list too long
  FE_ENOEXEC                   = fe_base + 4008,  // Exec format error
  FE_EBADF                     = fe_base + 4009,  // Bad file descriptor
  FE_ECHILD                    = fe_base + 4010,  // No children
  FE_EAGAIN                    = fe_base + 4011,  // No more processes
  FE_ENOMEM                    = fe_base + 4012,  // Insufficient user memory
  FE_EACCES                    = fe_base + 4013,  // Permission denied
  FE_EFAULT                    = fe_base + 4014,  // Bad address
  FE_EBUSY                     = fe_base + 4016,  // Mount device busy
  FE_EEXIST                    = fe_base + 4017,  // File already exists
  FE_EXDEV                     = fe_base + 4018,  // Cross-device link
  FE_ENODEV                    = fe_base + 4019,  // No such device
  FE_ENOTDIR                   = fe_base + 4020,  // Not a directory
  FE_EISDIR                    = fe_base + 4021,  // Is a directory
  FE_EINVAL                    = fe_base + 4022,  // Invalid function argument
  FE_ENFILE                    = fe_base + 4023,  // File table overflow
  FE_EMFILE                    = fe_base + 4024,  // Maximum number of files already open
  FE_ENOTTY                    = fe_base + 4025,  // Not a typewriter
  FE_ETXTBSY                   = fe_base + 4026,  // Object (text) file busy
  FE_EFBIG                     = fe_base + 4027,  // File too large
  FE_ENOSPC                    = fe_base + 4028,  // No space left on device
  FE_ESPIPE                    = fe_base + 4029,  // Illegal seek
  FE_EROFS                     = fe_base + 4030,  // Read only file system
  FE_EMLINK                    = fe_base + 4031,  // Too many links
  FE_EPIPE                     = fe_base + 4032,  // Broken pipe or no reader on socket
  FE_EDOM                      = fe_base + 4033,  // Argument out of range
  FE_ERANGE                    = fe_base + 4034,  // Value out of range
  FE_ENOMSG                    = fe_base + 4035,  // No message of desired type
  FE_EIDRM                     = fe_base + 4036,  // Identifier Removed
  FE_EDEADLK                   = fe_base + 4045,  // Deadlock condition
  FE_ENOLCK                    = fe_base + 4046,  // No record locks available
  FE_ENODATA                   = fe_base + 4061,  // No data sent or received
  FE_ENOSYS                    = fe_base + 4099,  // Function not implemented
  FE_EWOULDBLOCK               = fe_base + 4101,  // Operation would block
  FE_EINPROGRESS               = fe_base + 4102,  // Operation now in progress
  FE_EALREADY                  = fe_base + 4103,  // Operation already in progress
  FE_ENOTSOCK                  = fe_base + 4104,  // Socket operation on non-socket
  FE_EDESTADDRREQ              = fe_base + 4105,  // Destination address required
  FE_EMSGSIZE                  = fe_base + 4106,  // Message too long
  FE_EPROTOTYPE                = fe_base + 4107,  // Protocol wrong type for socket
  FE_ENOPROTOOPT               = fe_base + 4108,  // Protocol not available
  FE_EPROTONOSUPPORT           = fe_base + 4109,  // Protocol not supported
  FE_ESOCKTNOSUPPORT           = fe_base + 4110,  // Socket type not supported
  FE_EOPNOTSUPP                = fe_base + 4111,  // Operation not supported on socket
  FE_EPFNOSUPPORT              = fe_base + 4112,  // Protocol family not supported
  FE_EAFNOSUPPORT              = fe_base + 4113,  // Address family not supported
  FE_EADDRINUSE                = fe_base + 4114,  // Address already in use
  FE_EADDRNOTAVAIL             = fe_base + 4115,  // Can't assign requested address
  FE_ENETDOWN                  = fe_base + 4116,  // Network is down
  FE_ENETUNREACH               = fe_base + 4117,  // Network is unreachable
  FE_ENETRESET                 = fe_base + 4118,  // Network dropped connection on reset
  FE_ECONNABORTED              = fe_base + 4119,  // Software caused connection abort
  FE_ECONNRESET                = fe_base + 4120,  // Connection reset by remote host
  FE_ENOBUFS                   = fe_base + 4121,  // No buffer space available
  FE_EISCONN                   = fe_base + 4122,  // Socket is already connected
  FE_ENOTCONN                  = fe_base + 4123,  // Socket is not connected
  FE_ESHUTDOWN                 = fe_base + 4124,  // Can't send after socket shutdown
  FE_ETIMEDOUT                 = fe_base + 4126,  // Connection timed out
  FE_ECONNREFUSED              = fe_base + 4127,  // Connection refused
  FE_EHOSTDOWN                 = fe_base + 4128,  // Host is down
  FE_EHOSTUNREACH              = fe_base + 4129,  // No route to host
  FE_ENAMETOOLONG              = fe_base + 4131,  // File name too long
  FE_ENOTEMPTY                 = fe_base + 4132,  // Directory not empty
  FE_EBADDATA                  = fe_base + 4180,  // Invalid data in buffer
  FE_ENOREPLY                  = fe_base + 4181,  // No reply in buffer
  FE_EPARTIAL                  = fe_base + 4182,  // Partial buffer received
  FE_ESPIERR                   = fe_base + 4183,  // Interface error from SPI
  FE_EVERSION                  = fe_base + 4184,  // Version mismatch
  FE_EXDRDECODE                = fe_base + 4185,  // XDR encoding error
  FE_EXDRENCODE                = fe_base + 4186,  // XDR decoding error
  FE_EHAVEOOB                  = fe_base + 4195,  // Out-of-band data available
  FE_EBADSYS                   = fe_base + 4196,  // Invalid socket call
  FE_EBADFILE                  = fe_base + 4197,  // File type not supported
  FE_EBADCF                    = fe_base + 4198,  // C file (code 180) not odd-unstructured
  FE_ENOIMEM                   = fe_base + 4199,  // Insufficient internal memory
  FE_ELOOP                     = fe_base + 4200,  // Too Many Symbolic links during path name resolution
  FE_EFSBAD                    = fe_base + 4201,  // Fileset Catalog Internal Consistancy error
  FE_ENOROOT                   = fe_base + 4202,  // Root Fileset is Not Mounted
  FE_EOSSNOTRUNNING            = fe_base + 4203,  // OSS Not running
  FE_EILSEQ                    = fe_base + 4204,  // Illegal byte sequence (from XPG4)
  FE_ENOCRE                    = fe_base + 4205,  // Process is not CRE complient but requests a service thats depends on CRE.
  FE_ENOTOSS                   = fe_base + 4206,  // Non-OSS Process has requested a service available only to OSS processes
  FE_ENOCPU                    = fe_base + 4207,  // Cpu unavailable
  FE_EUNKNOWN                  = fe_base + 4208,  // something impossible happened
  FE_EISGUARDIAN               = fe_base + 4209,  // OSS operation attempted on Guardian file descriptor
  FE_EGUARDIANOPEN             = FEINUSE,         //OSS unlink open against open Guardian file
  FE_EWRONGID                  = FEWRONGID,       //Stale openid
  FE_ENONSTOP                  = fe_base + 4210,  // NONSTOP C language problem
  FE_ECWDTOOLONG               = fe_base + 4211,  // either cwd or cwd/file name is longer PATHMAX
  FE_EDEFINEERR                = fe_base + 4212,  // A Guardian define error was encountered
  FE_EHLDSEM                   = fe_base + 4213,  // An OSS process is trying to do a cross cpu exec while having an active semundo
  FE_EGUARDIANLOCKED           = FELOCKED,        ///G-file is locked by Guardian API
  FE_EBADMSG                   = fe_base + 4214,  // An invalid message tag was encountered
  FE_EBIGDIR                   = fe_base + 4215,  // Positioning of an OSS directory failed because there were more than 65535 names in the directory beginning with the same first two characters
  FE_ENOTSUP                   = fe_base + 4216,  // Operation not supported on referenced object (but supported on some object).
  FE_ETANOTRUNNING             = fe_base + 4217,  // Socket Transport Agent not running
  FE_EMSGQNOTRUNNING           = fe_base + 4218,  // Message queue server not running
// The following errors are internal errors generated by the OSS File system
// and its various subsystems, including the OSS Monitor and Name Server.
// The Errors for the OSS File system are in the range of
// 4500 - 4999.
//  Considerations to keep in mind before adding new error codes:
//  - Just take the next available number to allocate new errors. We decided
//    having holes in the numbers is not a very good idea.
//  - If the error is completely internal to the subsystem use the module
//    prefix to make up the error name.
//    e.g FE_PT_TABLE_FULL - is generated by the PT module to say that the
//                           table is full.
//  - If you are adding an error which might be useful to other subsystems
//    or is self explainatory it's OK to skip the module name.
//  - If the error name is not self describing then use the prefix OSS.
//    e.g FE_OSS_INTERNAL - this just says that an unknown OSS File system
//                           error found.

  FE_PT_TABLE_FULL             = fe_base + 4500,  // Pib Table Full
  FE_PT_INVALID_INDEX          = fe_base + 4501,  // Invalid Pib index
  FE_STALE_FIFOID              = fe_base + 4502,  // Fifo id is out of date in name server
  FE_FIFO_NOT_CREATED          = fe_base + 4503,  // No such fifo
  FE_OPEN_FAILED               = fe_base + 4504,  // FE_OPEN_FAILED              
  FE_RETRY_OPEN                = fe_base + 4505,  // FE_RETRY_OPEN               
  FE_CLIENT_NOT_FOUND          = fe_base + 4506,  // FE_CLIENT_NOT_FOUND         
  FE_OUT_OF_SEQUENCE           = fe_base + 4507,  // FE_OUT_OF_SEQUENCE          
  FE_PP_PAGES_LOCKED           = fe_base + 4508,  // FE_PP_PAGES_LOCKED          
  FE_PP_NO_MORE_PAGES          = fe_base + 4509,  // FE_PP_NO_MORE_PAGES         
  FE_REQ_RECALLED              = fe_base + 4510,  // FE_REQ_RECALLED             
  FE_NS_REDIRECT               = fe_base + 4511,  // FE_NS_REDIRECT              
  FE_NS_ROOTREDIRECT           = fe_base + 4512,  // FE_NS_ROOTREDIRECT          
  FE_FI_INTERNAL_RETRY         = fe_base + 4513,  // FE_FI_INTERNAL_RETRY        
  FE_PO_CLIENT_BLOCKED         = fe_base + 4514,  // FE_PO_CLIENT_BLOCKED        
                                   /*4515*/                        // unused

  FE_FHANDLESTALE              = fe_base + 4516,  // File Sys Req 04/28/95
  FE_NS_REMOTEREDIRECT         = fe_base + 4517,  // File system must redrive request to remote system
  FE_NS_EXPANDREDIRECT         = fe_base + 4518,  // File system must redrive request to different Name Server
// The following errors are used by OSS TTY devices (processes)
//  to inform the requesting file system to generate a signal.

  FESIGTTIN                    = fe_base + 4519,  // requestor should generate a SigTtin 4520 unused (was fesigstop)
  FESIGTERMINATE               = fe_base + 4521,  // requestor should generate a SigTerm
  FESIGTTOU                    = fe_base + 4522,  // requestor should generate a SigTtou
  FESIGINT                     = fe_base + 4523,  // requestor should generate a SigInt
  FESIGHUP                     = fe_base + 4524,  // requestor should generate a SigHup
  FESIGQUIT                    = fe_base + 4525,  // requestor should generate a SigQuit
  FE_CONF_NOLIMIT              = fe_base + 4526,  // Returned on pathconf/fpathconf calls if the variable has no limit for the given file type
  FE_NEED_SEPARATE_OPEN        = fe_base + 4527,  // Returned on openInherit if a Layer3 can't inherit the open by sharing the pob; and needs a separate pob to inherit the open.
  FESIGTSTP                    = fe_base + 4528,  // requestor should generate a SigTstp
      // DP2 OSS errors 4600-4619

  FE_POSITION_LOST             = fe_base + 4600,  // The position of a OSS open is lost. This is fatal, the opener should close this open.
  FE_LOCKS_LOST                = fe_base + 4601,  // Some locks might be lost on the OSS open. The FS might be able to tell DP2 that no locks were lost (because there were never locks). Otherwise this is fatal and the opener should close the file.
  FE_TRY_AGAIN                 = fe_base + 4602,  // The FS should retry the entire request, starting from acquiring the vnode-sem. This might avoid a deadlock.
  FEEDITFILEOPEN               = fe_base + 4603,  // An open() of a code 101 file was attempted.
  FE_FATAL_PROP                = fe_base + 4604,  // Fatal attempt at migrating an open across Cpus
  FE_WRONG_CRVSN               = fe_base + 4605,  // Wrong crvsn for the open file.
  FE_NFS_SYMLINK               = fe_base + 4606,  // Mount call found a symbolic link
  FE_OSS_INTERNAL              = fe_base + 4900,  // OSS internal error
  FE_PX_INTERNAL               = FE_OSS_INTERNAL, //to be deleted
      // New errors go here--
      

  FE_HIGHEST_POSSIBLE          = 32767,    //

//*********************************************
// DFS2FE
//*********************************************


  FEFS2FIRSTERR                = fe_base + 1024,  // This is the first error number reserved for use by FS2.
  FENOSUBSET                   = fe_base + 1024,  // Specified SQL subset is not defined to the system
  FECONSTRAINTVIOL             = fe_base + 1025,  // Supplied row or update value violates one of the constraints for the table
  FEVIEWVIOLATION              = fe_base + 1026,  // The selection expression on an SQL view has been violated
  FELABELBAD                   = fe_base + 1027,  // NonStop Services Data Access Manager encountered a bad SQL label or tree of labels
  FELABELWRONGTYPE             = fe_base + 1028,  // NonStop Services Data Access Manager accessed a label of an unexpected type during an open or during an SQL label operation
  FEOPENSHAREFAIL              = fe_base + 1029,  // A request to share an existing open failed due to no matching open found.
  FEBADLOCKLEN                 = fe_base + 1030,  // Invalid lock key length was specified for an SQL table
  FEBADFIELD                   = fe_base + 1031,  // A supplied numeric value exceeds the declared precision of the column, some of the supplied values for DECIMAL or VARCHAR columns are invalid, or the supplied row is too long.  Also, the NonStop Services Data Access Manager might have encountered a bad column in a stored row or a value in an update on a row that would change the length of a VARCHAR column in an entry-sequenced table
  FEBADRECDESC                 = fe_base + 1032,  // SQL row description is inconsistent
  FEBADKEYDESC                 = fe_base + 1033,  // SQL key column description is inconsistent, or the specified key is too long
  FEWRONGCATALOG               = fe_base + 1034,  // SQL internal error: The requested operation has failed because of an inconsistency in specifying the SQL catalog
  FEBADFLAGS                   = fe_base + 1035,  // SQL internal error
  FEBADFIELDLIST               = fe_base + 1036,  // SQL internal error
  FEBADEXPR                    = fe_base + 1037,  // SQL internal error
  FEBADLOCKMODE                = fe_base + 1038,  // SQL internal error
  FENOUPDATEINTENT             = fe_base + 1039,  // An SQL UPDATE statement was attempted, but update intent was not specified when the cursor was declared
  FENOCURRENT                  = fe_base + 1040,  // There is no current row.  The cursor position is either before the first row of the set, after the last row, or between two rows
  FENOTRANDOMSUBSET            = fe_base + 1041,  // SQL internal error
  FENODEFAULT                  = fe_base + 1042,  // Operation required a default value for a column that was defined as NO DEFAULT
  FENOTUNIQUE                  = fe_base + 1043,  // SQL internal error
  FESUBSETEXISTS               = fe_base + 1044,  // Operation is not allowed while an SQL cursor is open
  FEWRONGFILETYPE              = fe_base + 1045,  // SQL internal error
  FEBADOPEN                    = fe_base + 1046,  // SQL internal error
  FEINDEXINVALID               = fe_base + 1047,  // SQL index being used is marked invalid because the catalog manager has not successfully loaded it
  FE_BAD_DATALIST              = fe_base + 1048,  // SQL internal error
  FE_BAD_SEL                   = fe_base + 1049,  // SQL internal error
  FECONTINUE_AT                = fe_base + 1050,  // SQL internal error
  FECONT_BAD_REC               = fe_base + 1051,  // SQL internal error
  FECONT_PARITY                = fe_base + 1052,  // A row was encountered that resides in a block having a data parity error.  The row does not satisfy the WHERE clause
  FE_ECC_PARITY                = fe_base + 1053,  // ECC error or checksum error occurred indicating that it is impossible to process the accessed block.  No data is returned
  FE_CRASHLABEL_EXEC           = fe_base + 1054,  // Unable to access a nonaudited table or protection view that has been altered by an uncommitted DDL statement
  FE_CRASHLABEL_UTIL           = fe_base + 1055,  // Unable to perform a utility operation while an uncommitted DDL operation from another transaction exists
  FE_OPEN_RECOVERY             = fe_base + 1056,  // Unable to access a table that is being recovered by the NonStop Services Transaction Manager
  FE_LABEL_LOCKED              = fe_base + 1057,  // Unable to access a table that is being altered by another user
  FE_CLEARONPURGE              = fe_base + 1058,  // Unable to access a table that is being dropped by another user
  FE_FILE_MISSING              = fe_base + 1059,  // Unable to access a protection view whose underlying table does not exist or is inconsistent
  FE_BAD_SBB                   = fe_base + 1060,  // SQL internal error
  FEWRONGCBID                  = fe_base + 1061,  // Cursor is no longer defined in the NonStop Services Data Access Manager
  FEWRONGSCBID                 = FEWRONGCBID,     //SCB^ID no longer identifies a
  FEWRONGICBID                 = FEWRONGCBID,     //ICB^ID no longer identifies a
  FE_SCB_RESYNC                = fe_base + 1062,  // SQL internal error
  FEMISSINGLOCK                = fe_base + 1063,  // SQL internal error
  FEINVDROP                    = fe_base + 1064,  // SQL internal error
  FE_LOCK_PROTOCOL             = fe_base + 1065,  // An SQL internal error has occurred, or an attempt was made to execute either an UPDATE WHERE CURRENT OF or DELETE WHERE CURRENT OF statement by using a cursor declared with the BROWSE ACCESS option
  FEFSINTERNALERROR_1          = fe_base + 1066,  // Internal error:  Occurred in OPEN
  FEFSINTERNALERROR_2          = fe_base + 1067,  // Internal error:  Occurred in the NonStop Services Distribution Service or NonStop Services Data Access Manager OPEN protocol
  FEBADCATNAME                 = fe_base + 1068,  // SQL internal error
  FEPARMSINCONSISTENT          = fe_base + 1069,  // SQL internal error
  FENOTSQLLICENSED             = fe_base + 1070,  // Program file is not licensed
  FELENGTHMISMATCH             = fe_base + 1071,  // SQL internal error
  FEINVALIDVALUE               = fe_base + 1072,  // SQL internal error
  FE_BAD_IN_DATALIST           = fe_base + 1073,  // SQL internal error
  FEFS2INTERNALERROR           = fe_base + 1074,  // Internal error:  NonStop Services Distribution Service procedure
  FE_CURRENCY_UNKNOWN          = fe_base + 1075,  // A FETCH was attempted following a FETCH that failed; this left the cursor in an undefined position
  FETIMESTAMP_MISMATCH         = fe_base + 1076,  // SQL internal error
  FETIMESTAMP_FAILURE          = fe_base + 1077,  // Redefinition timestamp for a partition does not match other partitions; this is a serious consistency failure
  FEBADRECORD                  = fe_base + 1078,  // NonStop Services Data Access Manager encountered an invalid row
  FEBADKEYCOMP                 = fe_base + 1079,  // Requested key compression option is inconsistent with the data type, offset in row, or descending flag of some of the key columns
  FETRANSIDMISMATCH            = fe_base + 1080,  // Transaction ID does not match current transaction ID for an audited table with a lock protocol or for a nonaudited temporary table
  FETRANSIDNOTALLOWED          = fe_base + 1081,  // SQL internal error
  FEBUFFERTOOSMALL             = fe_base + 1082,  // SQL internal error
  FEBADSTRUCTURE               = fe_base + 1083,  // An internal input SQL structure has an invalid format, as indicated by an incorrect EYE^CATCHER data item value.  The program might have corrupted the SQL region or SQL executor segment, or an SQL internal error might have occurred
  FENOINSERT                   = fe_base + 1084,  // Unable to insert into an SQL view that does not allow insertions
  FEWRONGVERSION               = fe_base + 1085,  // Attempted to access an SQL object that is incompatible with the version of the accessing NonStop SQL/MX software, or an SQL internal error occurred Unable to unlock an SQL table that has locks through either STABLE ACCESS or REPEATABLE ACCESS
  FELOCKEXIST                  = fe_base + 1086,  // Attempt to unlock a table for which subsets with testlock or keeplock exist.
  FEDP2INTERNALERROR           = fe_base + 1087,  // SQL internal error
  FENOTCREATED                 = fe_base + 1088,  // SQL internal error
  FEKEYMISMATCH                = fe_base + 1089,  // SQL internal error
  FEMSGOVERFLOW                = fe_base + 1090,  // Requested SQL operation cannot be completed because of current limitations on message sizes
  FEEXPIRETIME                 = fe_base + 1091,  // Table or related index cannot be dropped until NOPURGEUNTIL date
  FELABTOOLONG                 = fe_base + 1092,  // Operation cannot be performed because the resulting disk directory entry would be too long.  Too many columns, partitions, indexes, protection views, or constraints have been defined
  FERELRECTOOLONG              = fe_base + 1093,  // Operation cannot be performed because the resulting row would exceed the RECLENGTH value defined for the relative table
  FEBADNUMFIELDS               = fe_base + 1094,  // Limit on number of columns that can make up a key has been exceeded for indexes or key-sequenced tables
  FEBADBLOCKSIZE               = fe_base + 1095,  // Invalid value has been supplied for the BLOCKSIZE attribute
  FERECLENTOOLONG              = fe_base + 1096,  // BLOCKSIZE value for this table is too small for the row length of the table
  FEOFFLINE                    = fe_base + 1097,  // Unable to access an object that is offline or has an inconsistent definition
  FEFILETOOLARGE               = fe_base + 1098,  // Supplied MAXEXTENTS value is too large, or the total size of the partitions would exceed the largest allowable table size
  FEPARTSDONTMATCH             = fe_base + 1099,  // Definitions of the table or index partitions are inconsistent, or the index label and corresponding underlying table values are inconsistent
  FETRUNC                      = fe_base + 1100,  // Operand was truncated during assignment of character data
  FETRUNCNOTALLOWED            = fe_base + 1101,  // Truncation was needed, but prohibited during assignment of character data
  FEOVFL                       = fe_base + 1102,  // Overflow occurred during expression evaluation
  FEUNDERFLOW                  = fe_base + 1103,  // Underflow occurred during expression evaluation
  FEBADDECDATA                 = fe_base + 1104,  // SQL column of type DECIMAL contained values that are not digits
  FESIGNERROR                  = fe_base + 1105,  // Unable to assign a negative value to a column defined as unsigned
  FENEGATIVEUNSIGNED           = fe_base + 1106,  // Unsigned numeric has a negative value
  FEDIVBYZERO                  = fe_base + 1107,  // Division by zero occurred during expression evaluation
  FEUNIMPLEMENTEDDATATYPE      = fe_base + 1108,  // Data type of column not supported by this release
  FEILLEGALDATATYPE            = fe_base + 1109,  // Invalid SQL data type was encountered
  FETYPEINCOMPATIBILITY        = fe_base + 1110,  // Operation between incompatible SQL types was requested
  FEILLEGALDIVIDE              = fe_base + 1111,  // SQL internal error
  FEUNSIGNEDARITH              = fe_base + 1112,  // Arithmetic operation was requested in an unexpected unsigned data type
  FEILLEGALOPERATOR            = fe_base + 1113,  // Invalid operator value during expression evaluation
  FEILLEGALPATTERN             = fe_base + 1114,  // Invalid LIKE pattern during expression evaluation
  FESYSKEYTOOLARGE             = fe_base + 1115,  // Specified SYSKEY value exceeds current defined size of the relative SQL table
  FEFS2TRANSABORT              = fe_base + 1116,  // NonStop Services Distribution Service aborted the transaction because an SQL statement could not be completed
  FEINDEXNOTFOUND              = fe_base + 1117,  // Unable to use an undefined index
  FEIXVIEWNOTFOUND             = fe_base + 1118,  // Specified view is not defined
  FEPARTNOTFOUND               = fe_base + 1119,  // Specified partition is not defined
  FEFIELDSNOTININDEX           = fe_base + 1120,  // SQL internal error
  FEINVALIDPROTECTION          = fe_base + 1121,  // Remote user specified local-only authority in the SECURE attribute for the object being created or altered, or the SECURE attribute did not grant read authority to all users being granted write authority
  FEDUPKEYSPEC                 = fe_base + 1122,  // Supplied KEYTAG value is already defined for this table
  FEBADTREE_RECOVERY           = fe_base + 1123,  // SQL internal error
  FELOCKED_AT                  = fe_base + 1124,  // SQL internal error
  FELABELCHANGED               = fe_base + 1125,  // SQL internal error
  FEBAD_IXVIEWARRAY            = fe_base + 1126,  // SQL internal error
  FECATVIOL                    = fe_base + 1127,  // Unable to update an SQL catalog table from a process that is not licensed
  FETRUNCSCALE                 = fe_base + 1128,  // Operand was scale truncated during expression evaluation
  FETRUNCSCALENOTALLOWED       = fe_base + 1129,  // Scale truncation was needed but prohibited during expression evaluation
  FENOSQL                      = fe_base + 1130,  // Unable to use NonStop SQL/MX on a system where the product is not installed
  FECANCELLED                  = fe_base + 1131,  // The current request to the disk process was found on the lock queue and was cancelled.
  FESQLNOTCOMPLETE             = fe_base + 1132,  // This error is returned for nowait SQL requests that have not completed. This error is not returned to the SQL user.
  FESCBOVERFLOW                = fe_base + 1133,  // Selection expression or update expression is too complex to be represented in NonStop SQL/MX internal data structures
  FE_ENSCRIBECOMPLETE          = fe_base + 1134,  // This error is returned by DM^WAIT to indicate that an ENSCRIBE nowait request has completed. This error is not returned to the SQL user.
  FENEEDNEWOPEN                = fe_base + 1135,  // This error indicates that the SQL Executor should obtain a new open. This error is not returned to the SQL user.
  FESBBCONFLICT                = fe_base + 1136,  // This operation might conflict with a concurrent operation that is running under the same transaction ID and is performing sequential inserts using virtual sequential block buffering
  FEPARTDUPKEY                 = fe_base + 1137,  // Supplied key for new partition is already defined at existing partitions
  FEPARTEXISTS                 = fe_base + 1138,  // SQL internal error
  FEINVALIDDATE                = fe_base + 1139,  // Result of this expression is an invalid date
  FENULLTONONULL               = fe_base + 1140,  // Null value is being assigned to a field that cannot contain null values
  FEBADDATEINTSYNTAX           = fe_base + 1141,  // Incorrect syntax for date-time or INTERVAL value
  FEBADBUFFERADDR              = fe_base + 1142,  // Either the program needs to be recompiled using the latest SQL compiler or an SQL internal error has occurred
  FEVSBBFLUSHFAIL              = fe_base + 1143,  // SQL request failed due to an error occurring while server process was flushing a VSBB insert/update buffer
  FEVSBBWRITEEXISTED           = fe_base + 1144,  // SQL internal error
  FESKIPERROR                  = fe_base + 1145,  // SQL internal error
  FEUNKNOWNOPENREPLY           = fe_base + 1146,  // NonStop Services Distribution Service received an unexpected reply to an SQL open request
  FECOLLATIONNOTFOUND          = fe_base + 1147,  // Specified collation does not exist in file label
  FECOLLATIONEXISTS            = fe_base + 1148,  // Specified collation being added was found in label
  FECPRLWRONGVERSION           = fe_base + 1149,  // Version of character processing rules object is no longer supported
  FECPRLWONTFIT                = fe_base + 1150,  // Encoded string does not fit in supplied buffer
  FECPRLUNKNOWN                = fe_base + 1151,  // Encountered an unknown error while encoding a column defined explicitly with collation
  FECOLLATIONTOOMANY           = fe_base + 1152,  // Attempted to create table or index that references more collation objects than can be handled
  FEBADCOLLARRAY               = fe_base + 1153,  // SQL internal error
  FECATNAMEDOESNTMATCH         = fe_base + 1154,  // SQL internal error
  FEPROTECTWRONGVERSION        = fe_base + 1155,  // SQL internal error
  FEBADPROTECTIONINFO          = fe_base + 1156,  // SQL internal error
  FEUNKNOWNLABELREPLY          = fe_base + 1157,  // SQL internal error
  FEOVFL_AT                    = fe_base + 1158,  // SQL internal error
  FEBADPARTKEY                 = fe_base + 1159,  // Attempted to add new partition before primary partition
  FEBADCOLLENTRY               = fe_base + 1160,  // SQL internal error
  FEUNKNOWNCHARSET             = fe_base + 1161,  // SQL internal error
  FEUNKNOWNMBCSERROR           = fe_base + 1162,  // SQL internal error
  FERESERVEDNAME               = fe_base + 1163,  // Illegal operation attempted on a file having a system-reserved filename
  FECHARSETINCOMPATIBILITY     = fe_base + 1164,  // Operation between incompatible character sets was detected
  FENOAGGREXPR                 = fe_base + 1165,  // No aggregate expression was supplied with an aggregate request
  FEBADAGGRCOMPEXPR            = fe_base + 1166,  // Bad aggregate composite expression
  FEWRONGLOCALVERSION          = fe_base + 1167,  // Object SQL version is higher than SQL version of system where request is issued
  FEWRONGREMOTEVERSION         = fe_base + 1168,  // Object SQL version is higher than SQL version of system where object resides
  FEINVALIDVERSION             = fe_base + 1169,  // Invalid SQL version encountered
  FEBADEXPRVERSION             = fe_base + 1170,  // HLSQLV of SQL expression is not compatible with the system
  FEMSGCOUNTOVERFLOW           = fe_base + 1171,  // Number of message bytes exchanged between NonStop Services Distribution Service and NonStop Services Data Access Manager has exceeded the highest limit (2147479551)
  FEFS2WRONGREQID              = fe_base + 1172,  // Reserved for FS2 internal use in proc kernel^timed^sendbuf. Supposed not to be used or seen by any other procedure.
             // * * * DON'T FORGET TO CHANGE AFS2FE TO MATCH THIS FILE * * *

  FERCLABELINCONSISTENT        = fe_base + 1173,  // The SLAB RC does not match the table or index labels.
  FERFORKLOCKED                = fe_base + 1174,  // A DDL operation is updating the Table
  FEWRONGOP                    = fe_base + 1177,  // Invalid SQL operation type for NonStop Services Data Access Manager
  FERESENDDATA                 = fe_base + 1178,  // NonStop Services Distribution Service needs to resend session data to NonStop Services Data Access Manager
  FEMOREDATA                   = fe_base + 1179,  // NonStop Services Distribution Service needs to send more data to NonStop Services Data Access Manager
  FESEQUENCE                   = fe_base + 1180,  // Input key to NonStop Services Data Access Manager out of sequence
  FESIDETREE                   = fe_base + 1181,  // NonStop Services Data Access Manager sidetree insert protocol error
  // ???? FERFORKLOCKED                = fe_base + 1182,
  FENOSQLMX                    = fe_base + 1183,
  FECPUSWITCHED                = fe_base + 1184,  
  FESQLMXINTERNALERR           = fe_base + 1185,
  FENOEIDSPACE                 = fe_base + 1187,  // out of memory in EID 
  FENOTREVIVED                 = fe_base + 1188,  // file not revived
  FEIOBLOCKED                  = fe_base + 1189,  // EID is waiting (blocked) on I/O
  FEBADVERIFY                  = fe_base + 1190,  // error detected during file verify
// error 1191 and 1192 are defined above
//FENOSELFREFERENCE            = fe_base + 1191,  // Lock escalated, cannot detect FELOCKSELF
//FELOCKSELF                   = fe_base + 1192,  // Requestor already locked row, self-ref insert.
  FEFSALLOCATIONFAILURE        = fe_base + 1193,  // This is a new error for 
                                                  // SeaQuest to indicate that
                                                  // there was a failure allocting
                                                  // an FS session from NAMemory  
  FEREOPEN                     = fe_base + 1194,  //
  FEBROKENACCESSPATH           = fe_base + 1195, // Rows returned out of order
            // Please keep this file in sync with sql/executor/dfs2fe.h
            // * * * DON'T FORGET TO CHANGE AFS2FE TO MATCH THIS FILE * * *
  /*, fe<error>           = <nnnn> */                // <comment>
//*-------------------------------------------------------------*
//* NOTE:  RANGE 1797 TO 2047 IS RESERVED FOR ASSERTIONS        *
//*-------------------------------------------------------------*
//*                                                             *
//* the range 1797 to 2047 (250 error numbers) have been        *
//* reserved for reporting assertion numbers in case an         *
//* assertion is violated.                                      *
//*                                                             *
//* FEAssertNumBase (= 2047) is used as a base                  *
//* FEMaxAssertNums (= 250)  is the range reserved to report    *
//*       assertion numbers.                                    *
//*                                                             *
//*                                                             *
//* Example:   What Happens if assertion number 5 fails.        *
//*                                                             *
//*  The assertion evaluation procedures will subtract 5        *
//*  from the feAssertNumBase and return that as an errornum    *
//*  to the invoker DiscProcess procedures.                     *
//*                                                             *
//*    errornum := FEAssertNumBase - 5  (=2042) reported to DP  *
//*                                                             *
//*  This errornum is returned to the FileSystem by the         *
//*  DiscProcess which in turn, passes it to the SQL Executor.  *
//*  The Executor, on getting an error from the FileSystem,     *
//*  checks to see if the errornum is in the reserved range     *
//*  of error numbers for assertions.  It then translates back  *
//*  the 'encoded' errornum to get back the assertion number.   *
//*                                                             *
//*    assertion^num := (feAssertNumBase - errornum)            *
//*                                                             *
//*  This assertion^num is then reported back to the user.      *
//*                                                             *
//* If the table had more than FEMaxAssertNums assertions and   *
//* an assertion > the 250th (FEMaxAssertNums) fails, then      *
//* the generic error 'feConstrainViol' is set to indicate that *
//* an assertion failed.                                        *
//*                                                             *
//* This is being done this way, as the current DiscProcess-    *
//* FileSystem communications protocol doesnot allow for an     *
//* easier way to report multiple data for an error; in this    *
//* case-- error that an assertion was violated AND the         *
//* assertion number of that assertion -- hence, the need to    *
//* encode and decode errornums.                                *
//* Note that it may change in some future release.             *
//*                                                             *
//*-------------------------------------------------------------*


  FEASSERTNUMBASE              = fe_base + 2047,  // is used as a base to encode and decode assertion numbers = fefs2lasterr
  FEMAXASSERTNUMS              = 250,             //is the number of error numbers reserved to report assertion violations
  FEFS2LASTERR                 = fe_base + 2047   // This is the last error number
  };                                 // reserved for use by FS2.
                                     //------------------------------

#endif
