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

#include "BloomFilter.h"
#include "exp_function.h"
#include "NAMemory.h"
#include "ComASSERT.h"
#include <math.h>
#include <fstream>

static float ln(float x)
{
   return (float)(log(x) / log(2.0));
}

static UInt32 bitsNeeded(UInt64 maxVal)
{
  UInt32 bits = (UInt32)ceil(ln((float)maxVal));

  // Since the MAX value in freqsL_ is used as
  // the indicator for overflow, we need extra 1
  // bit to cover maxVal # of distinct values.

  return MAXOF(bits, 1) + 1; 
}


void computeParams(UInt32 maxHashFuncs, // max hash funcs
                   UInt32 n, // Input: # of distinct elements
                   float p,  // Input: probability of false positive
                   UInt32& m, // output
                   UInt16& k  // output
                  )
{
   // m (the number of bits for hash table, given n, p and optimal k):
   // m = -(n Ln(p)) / (Ln(2)^2
   m = UInt32(-(n * ln(p)) / pow(ln(2),2));

   // optimal k
   // k (the optimial # of hash functions, given m and n) = (m/n) Ln(2)
   k = (UInt16) ceil(( m * ln(2) ) / n);

   if ( k > maxHashFuncs ) {
     k = maxHashFuncs;

     // recompute m:  
     m = UInt32(- (float)k * (float)n / ln( 1 - powf(p, 1/(float)k)));
   }
}

//
// n = # of distinct elements (INPUT)
// p = the probability of false positives (INPUT)
//
// m (the number of bits for hash table, given n and p) = -(n Ln(p)) / (Ln(2))^2
// k (the optimial # of hash functions, given m and n) = (m/n) Ln(2)
//
// If k is capped at MAX_NUM_HASH_FUNCS, then m is computed as
//    m = -MAX_NUM_HASH_FUNCS * n / (Ln(1-p^(1/MAX_NUM_HASH_FUNCS)))
//
// Parameters for the count table (freqsL_)
//    #entries = m
//    #bits per entry = BitsCT = ceil(Ln(non-overflow frequency (INPUT))) 
//
// Parameters for freq of freq array (freqFreqC_)
//    #entries = 2^(bitsCT) 
//    #bits per entry = ceil(Ln(m))
//    freqFreqC_ only records freqFerq stored in the count table that has not been
//    overflowed. The true freq of freq needs to consider frequency stored in the
//    overflow table. 
//
// Parameters for overflow table
//    #entries = # of skewed elements in n (INPUT)
//    #bits per entry = UInt32
//
//
BloomFilter::BloomFilter(NAHeap* heap, UInt32 maxHashFuncs,
                                       UInt32 n, // # of distinct elements 
                                       float p   // probability of false positives
                        ):
    heap_(heap), keyLenInfo_(0)
{
   computeParams(maxHashFuncs, n, p, m_, k_);
}

BloomFilter::BloomFilter(NAHeap* heap) :
    heap_(heap), keyLenInfo_(0), m_(0), k_(0)
{
}

BloomFilter::~BloomFilter()
{
}

UInt32 BloomFilter::computeFinalHash(UInt32 hash, Lng32 hashFunc)
{
   UInt32 flags = ExHDPHash::NO_FLAGS;
   char hashNum = (char) hashFunc;
   UInt32 hashValue2 = ExHDPHash::hash((char*)&hashNum, flags, sizeof(hashNum));
   return (UInt32)(((hash<<1 | hash>>31) ^ hashValue2) % m_);
}

double BloomFilter::density()
{ 
   // TBD
   return 0;
}

ULng32 BloomFilter::packIntoBuffer(char*& buffer, NABoolean swapBytes)
{
   ULng32 size = pack(buffer, m_, swapBytes);
   size += pack(buffer, k_, swapBytes);
   size += pack(buffer, keyLenInfo_, swapBytes);
   return size;
}

ULng32 BloomFilter::unpackBuffer(char*& buffer)
{
   ULng32 size = unpack(buffer, m_);
   size += unpack(buffer, k_);
   size += unpack(buffer, keyLenInfo_);
   return size;
}

///////////////////////////////////////////////////////////////
//
//
//
///////////////////////////////////////////////////////////////

ULng32 scbfHashFunc(const simple_cbf_key& key)
{
   return ExHDPHash::hash(key.key_, ExHDPHash::NO_FLAGS, key.keyLen_);
}

ULng32 cbfHashFunc(const cbf_key& key)
{
   return ExHDPHash::hash(key.key_, ExHDPHash::NO_FLAGS, key.keyLen_);
}


//    #entries = 2^(bitsCT) 
//    #bits per entry = ceil(Ln(m))

UInt64 CountingBloomFilter::estimateMemoryInBytes(
                                UInt32 maxHashFuncs,
                                UInt32 n, // # of distinct elements
                                float p,  // probability of false positives
                                 // non-overflow freq of n elements
                                UInt32 nonOverflowFreq,
                                UInt32 avgSkewedElements,
                                // # of buckets that keys will fall in.
                                // Assume each key maps to one
                                // bucket only
                                UInt32 numBuckets
                      )
{

   UInt32 m;  
   UInt16 k; 

   computeParams(maxHashFuncs, n, p, m, k);

   // for freqsL_
   UInt64 totalMem = VarUIntArray::estimateMemoryInBytes(
                    m, (UInt32)bitsNeeded(nonOverflowFreq));


   // for uecs_
   totalMem += VarUIntArray::estimateMemoryInBytes(
                    numBuckets, (UInt32)bitsNeeded(n));

   // for totalFreqs_
   totalMem += VarUIntArray::estimateMemoryInBytes(
                    numBuckets, 
                    (UInt32)MINOF(32, nonOverflowFreq * n / numBuckets));

   // for freq2sL_ array
   totalMem += sizeof(VarUIntArrayPtr) * numBuckets;

   // for each VarUIntArray pointed by each element of the array 
   totalMem += numBuckets * VarUIntArray::estimateMemoryInBytes(
                       (1 << (UInt32)bitsNeeded(nonOverflowFreq))+1, 
                       (UInt32)bitsNeeded(m)
                                                  );

   totalMem += sizeof(CountingBloomFilter);


   // for bucketNums_
   totalMem += VarUIntArray::estimateMemoryInBytes(
                    m, (UInt32)bitsNeeded(numBuckets));

   return totalMem;
}


ULng32 CountingBloomFilter::packIntoBuffer(char*& buffer, NABoolean swapBytes)
{
  ULng32 sz = BloomFilter::packIntoBuffer(buffer, swapBytes);

  sz += freqsL_.packIntoBuffer(buffer, swapBytes);

  sz += pack(buffer, numBuckets_, swapBytes);

  for ( CollIndex i=0; i<numBuckets_; i++ )
     sz += freq2sL_[i]->packIntoBuffer(buffer, swapBytes);

  sz += uecs_.packIntoBuffer(buffer, swapBytes);
  sz += totalFreqs_.packIntoBuffer(buffer, swapBytes);

  sz += pack(buffer, actualOverflowF2s_, swapBytes);

  sz += bucketNums_.packIntoBuffer(buffer, swapBytes);

  // Not packed in this class. Subclass CBFWithKnownSkew packs freqsH_.
  // freqStruct* freqsH_;          

  // Not packed in this class. Subclass GeneralCountingBloomFilter packs freq2sH_.
  // UInt64* freq2sH_;             

  // Not packed, since saveFi() is not used.
  // VarUIntArrayPtr* savedFreq2sL_;

  return sz;
}

ULng32 CountingBloomFilter::unpackBuffer(char*& buffer)
{
  ULng32 sz = BloomFilter::unpackBuffer(buffer);

  sz += freqsL_.unpackBuffer(buffer);

  sz += unpack(buffer, numBuckets_);

  freq2sL_ = new (heap_) VarUIntArrayPtr[numBuckets_];

  for ( CollIndex i=0; i<numBuckets_; i++ ) {
     freq2sL_[i] = new (heap_) VarUIntArray(heap_);
     sz += freq2sL_[i]->unpackBuffer(buffer);
  }

  sz += uecs_.unpackBuffer(buffer);
  sz += totalFreqs_.unpackBuffer(buffer);

  sz += unpack(buffer, actualOverflowF2s_);

  sz += bucketNums_.unpackBuffer(buffer);

  // Rebuild freqsH_ and freq2sH_
  computeOverflowF2s();

  return sz;
}



CountingBloomFilter::CountingBloomFilter(NAHeap* heap,
                                         UInt32 maxHashFuncs,
                                         UInt32 n, // # of distinct elements 
                                         float p,  // probability of false positives
                                         UInt32 nonOverflowFreq, // non-overflow freq of n elements
                                         UInt32 avgSkewedElements,
                                         UInt32 numBuckets 
                                         ):
  BloomFilter(heap, maxHashFuncs, n, p), 
  freqsL_(m_, (UInt32)bitsNeeded(nonOverflowFreq), heap_),
  freqsH_(NULL),
  freq2sH_(NULL),
  actualOverflowF2s_(0),
  uecs_(numBuckets, (UInt32)bitsNeeded(n), heap_),
  totalFreqs_(numBuckets, (UInt32)MINOF(32, nonOverflowFreq * n / numBuckets), heap_), 
                                                     // total freq per bucket is estimated as
                                                     // avg. freq * n / numBuckets
  numBuckets_(numBuckets),
  savedFreq2sL_(NULL),
  totalMemSz_(0),
  bucketNums_(m_, (UInt32)bitsNeeded(numBuckets), heap_),
  filename_(NULL)
{
  freq2sL_ = new (heap) VarUIntArrayPtr[numBuckets];

  for ( CollIndex i=0; i<numBuckets; i++ )
    freq2sL_[i] = new VarUIntArray((1 << (UInt32)bitsNeeded(nonOverflowFreq))+1, 
                                    (UInt32)bitsNeeded(m_), heap_);


  sprintf(paramBuf, "maxHash=%d, n=%d, falsePosProb=%f, lfkeys=%d, skews=%d, buckets=%d, #slots=%d",
                     maxHashFuncs, n, p, nonOverflowFreq, avgSkewedElements, numBuckets, m_);
}


CountingBloomFilter::CountingBloomFilter(NAHeap* heap) :
  BloomFilter(heap),
  freqsL_(heap),
  freqsH_(NULL),
  freq2sH_(NULL),
  actualOverflowF2s_(0),
  numBuckets_(0),
  savedFreq2sL_(NULL),
  freq2sL_(NULL),
  uecs_(heap),
  totalFreqs_(heap),
  bucketNums_(heap),
  filename_(NULL)
{
  paramBuf[0] = '\0';
}

void CountingBloomFilter::clear()
{
  freqsL_.clear();
  uecs_.clear();
  totalFreqs_.clear();
  bucketNums_.clear();

  for ( CollIndex i=0; i<numBuckets_; i++ )
    freq2sL_[i]->clear();

  if ( freqsH_ )
    memset(freqsH_, 0, sizeof(freqStruct) * getOverflowEntries());

  if ( freq2sH_ )
    memset(freq2sH_, 0, sizeof(UInt64) * getOverflowEntries());
}


CountingBloomFilter::~CountingBloomFilter()
{
   NADELETEARRAY(freq2sL_, numBuckets_, VarUIntArrayPtr, heap_);
   NADELETEBASIC(freqsH_, heap_);
   NADELETEBASIC(freq2sH_, heap_);
}

Int32 compareTwoFreqStructs(const void* x, const void* y)
{
   return ((freqStruct*)x)->operator==(*((freqStruct*)y));
}

Int32 freqStruct::operator==(freqStruct& y)
{
   if ( getBucket() == y.getBucket() ) {
      if ( getFreq() == y.getFreq() ) 
         return 0;
      else {
        if ( getFreq() < y.getFreq() ) 
          return -1;
        else
          return 1;
      }
   } else {
     if ( getBucket() < y.getBucket() ) 
       return -1;
     else 
       return 1;
   }
}

/*
ULng32 freqStruct::pack(char* buffer, NABoolean swapBytes)
{
   ULng32 sz = pack(buffer, freq_, swapBytes);
   sz += pack(buffer, bucket_, swapBytes);
   return sz;
}

ULng32 freqStruct::unpack(char* buffer)
{
   ULng32 sz = unpack(buffer, freq_);
   sz += unpack(buffer, bucket_);
   return sz;
}
*/

void CountingBloomFilter::printfreqfreq()
{
   cout << endl;
   cout << "***** CBF statistics ***********" << endl;
   cout << "In low freq area:" << std::endl;

   for (UInt32 b=0; b < numBuckets_; b++) {
      cout << "In bucket " << b << ":" << endl;

      UInt32 uc = uec(b);       
      UInt32 tf  = totalFreq(b); 

      if ( tf > 0 )
        cout << "uec=" << uc << ", total_freq=" << tf << endl;

      for (UInt32 i=1; i < freq2sL_[b]->entries(); i++)
      {
        if ((*freq2sL_[b])[i] >0)
        std::cout << "number of low freq. entries f" << i << "=" 
                  << (*freq2sL_[b])[i] << std::endl;
      }
   }

   computeOverflowF2s();
   UInt32 highF2s = getOverflowEntries();
   std::cout << endl << "In high freq area:" << std::endl;
   cout << "#highF2s=" << highF2s << endl;

   for ( CollIndex i = 0 ; i < highF2s; i++) {
     UInt64 freq;
     UInt32 bucket;
     UInt64 f2 = highF2(i, freq, bucket);
     if ( freq > 0 )
        std::cout << "In bucket " << bucket 
                  << ", number of entries with high freq. f" 
                  << freq << "=" << f2 << endl;
   }

   cout << endl;
}


UInt64 CountingBloomFilter::highF2(UInt32 k, UInt64& i, UInt32& bucket)
{
  ComASSERT(k <= actualOverflowF2s_);

  i = freqsH_[k].getFreq();
  bucket = freqsH_[k].getBucket();

  return freq2sH_[k];
}


#if 0
#define DISPLAY_KEY(x,y,z) display_key(x,y,z)
#define DISPLAY_KEY4(x,y,z, u) display_key(x,y,z, u)
#define DISPLAY_HASH_ENTRY(x,y) display_hash_entry(x,y)
#define DISPLAY_MSG(x) display_msg(x)
#else
#define DISPLAY_KEY(x,y,z) 
#define DISPLAY_KEY4(x,y,z,u) 
#define DISPLAY_HASH_ENTRY(x,y) 
#define DISPLAY_MSG(x) 
#endif

void CountingBloomFilter::display_key(const char* msg, char * key, UInt32 key_len, UInt32 bucket)
{
  cout << msg << endl;

  cout << "bucket=" << bucket << ", " << "key=" ;

  for (UInt32 k=0;k<key_len; k++) 
    cout << key[k];
  cout << endl;
}

void CountingBloomFilter::display_hash_entry(const char* msg, UInt32 hash)
{
   cout << msg << endl << "hash=" << hash << ", count[" << hash << "] =" << freqsL_[hash] << endl;
}

simple_cbf_key::simple_cbf_key(char* key, UInt32 key_len, NAHeap* heap) :
keyLen_(key_len), heap_(heap)
{
   key_ = new (heap_) char[keyLen_];
   memmove(key_, key, keyLen_);
}

simple_cbf_key::simple_cbf_key(const simple_cbf_key& key) :
heap_(key.heap_), keyLen_(key.keyLen_)
{
   key_ = new (heap_) char[keyLen_];
   memmove(key_, key.key_, keyLen_);
}


simple_cbf_key::~simple_cbf_key()
{
   NADELETEBASIC(key_, heap_);
}

simple_cbf_key& simple_cbf_key::operator=(const simple_cbf_key& other)
{
   heap_ = other.heap_;
   keyLen_ = other.keyLen_;
   key_ = new (heap_) char[keyLen_];
   memmove(key_, other.key_, keyLen_);
   return *this;
}

void CountingBloomFilter::display_msg(const char* msg)
{
   cout << msg << endl;
}

cbf_key::cbf_key(char* key, UInt32 key_len, UInt32 bucket, MFV_ENUM mfv, NAHeap* heap) : 
simple_cbf_key(key, key_len), bucket_(bucket), mfv_(mfv)
{
}

cbf_key::cbf_key(const cbf_key& key) :
simple_cbf_key(key), bucket_(key.bucket_), mfv_(key.mfv_)
{
}

cbf_key& cbf_key::operator=(const cbf_key& other)
{
   simple_cbf_key::operator=(other);
   bucket_ = other.bucket_;
   mfv_ = other.mfv_;

   return *this;
}

//
// For each bucket, compute the sum of all frequencies squared.
//
void CountingBloomFilter::computeSumOfFrequencySquared(double * sumSq, Int32 sz)
{
  sz = MINOF(sz, (Int32)numBuckets_);

  for ( Int32 i=0; i<sz; i++ ) 
     sumSq[i] = 0.0;

  //
  // Use freqsL_ which records the frequencies for all low freq keys
  // Use bucketNums_ which records the bucket number for all low freq keys
  //
  // Accumulate low frequencies per bucket
  //
  for ( Int32 b=1; b<sz; b++ ) {

     VarUIntArray& lowf2s = lowF2s(b);
     for (UInt32 j=1; j<lowf2s.entries(); j++)
      {
        // fj is the total # of keys each appearing j times.
        // For these fj keys, Sum freq^2 = j * j * fj.
        UInt32 fj = lowf2s[j]; 

        sumSq[b] += j * j * fj;  
      }
  }

  computeSumOfFrequencySquaredHighFreq(sumSq);
}


void GeneralCountingBloomFilter::computeSumOfFrequencySquaredHighFreq(double * sumSq)
{
  assert(0); // not implemented yet.
}

void CountingBloomFilterWithKnownSkews::computeSumOfFrequencySquaredHighFreq(double * sumSq)
{
  // Accumulate high frequencies per bucket.
  // For bucket i, its MFV is stored at 2*i, and 2MFV at 2*i+1.
  for ( UInt32 i=0; i<getOverflowEntries()/2; i++ ) {

    // MFV's index is 2*i
    double fd = (double)freq2sH_[2*i];
    sumSq[i] += fd * fd;

    // 2MFV's index is 2*i+1
    fd = (double)freq2sH_[2*i+1];
    sumSq[i] += fd * fd;
  }
}


CountingBloomFilter::INSERT_ENUM
CountingBloomFilter::insert(char * key, UInt32 key_len, UInt32 bucket, cbf_key::MFV_ENUM mfv)
{
   DISPLAY_KEY4("insert start", key, key_len, bucket);

   if (bucket >= numBuckets_)
       return PARAM_ERROR;

   UInt32 flags = ExHDPHash::NO_FLAGS;
   UInt32 hashValueCommon = ExHDPHash::hash(key, keyLenInfo_, key_len);

   UInt32 f = 0xFFFFFFFF;
   ULng32 newFreq=0;
   NABoolean slotFound = FALSE;

   for(Lng32 i = 0; i < k_; i++)
   {
      UInt32 hash_index = computeFinalHash(hashValueCommon, i);

#if 0     
      if ( filename_ && (bucket == 44)) {
       ofstream fileout(filename_, ios::app);

       UInt32 flags = ExHDPHash::NO_FLAGS;
         char hashNum = (char) i;
         UInt32 hashValue2 = ExHDPHash::hash((char*)&hashNum, flags, sizeof(hashNum));
         UInt32 hash = ((hashValueCommon<<1 | hashValueCommon>>31) ^ hashValue2) % m_;

       fileout << " {(" << (hashValueCommon<<1 | hashValueCommon>>31) 
               << " ^ " << hashValue2 << ") % " 
               << m_ << " = " << hash << "}" << endl;
      }
#endif

      UInt32 b= bucketNums_[hash_index];

      // Compute the min frequency in those slots that have
      // not been occupied or occupted by keys from the same
      // bucket.

#if 0     
      if ( filename_ && (bucket == 44)) {
       ofstream fileout(filename_, ios::app);
       fileout  << "\t[" << i << "]" << "\t" << hash << "," << b;
      }
#endif

      if ( b == 0 || b-1 == bucket ) {

         freqsL_.add(hash_index, 1, newFreq);

         if ( b==0 )
           bucketNums_.put(hash_index, bucket+1);

         if ( i == 0 || f > newFreq ) {
            f = newFreq;
            slotFound = TRUE;
         }
      }
   }

   if ( !slotFound ) {

#if 0
    if ( filename_ && (bucket == 44)) {
      ofstream fileout(filename_, ios::app);
      fileout << "\tfail to insert" << endl;
    }
#endif

     return NO_SLOT;
   }

#if 0
   if ( filename_ && (bucket == 44)) {
      ofstream fileout(filename_, ios::app);
      fileout << endl;
   }
#endif

   if ( f >= freqsL_.getMaxVal() ) {

      // The key overflows 

      // If this is not a MFV and we can not handle arbitrary skews, bail out
      if ( mfv == cbf_key::NONE && !canHandleArbitrarySkewedValue() )
        return NEW_MFV;

      // Check if it is the first time to insert the key in the high freq area
      cbf_key ckey(key, key_len, bucket, mfv, heap_);
      UInt64* freq = searchOverflowTable(ckey);
      if ( freq ) {
         (*freq)++; // yes, increment the frequency
      } else {

         // No, insert the key 
         insertIntoOverflowTable(ckey);

         // Remove the freqfreq influence from the key
         (*freq2sL_[bucket]).sub(f-1,1, newFreq); 
      }

   }
   else 
   {
      // The key stays in the low freq area.

      // Adjust frequency of frequency. 
      (*freq2sL_[bucket]).add(f,1,newFreq);
      (*freq2sL_[bucket]).sub(f-1,1, newFreq);

      // For first time inserted key, change the UEC
      if ( f == 1 ) {
        uecs_.add(bucket,1,newFreq); 
      }

      // Adjust total RC.
      totalFreqs_.add(bucket,1,newFreq); 
   } 

   DISPLAY_MSG("insert complete");

   return NORMAL;
}


//
// Return status:
//   TRUE: the item is in CBF before the deletion
//   FALSE : the item is NOT in CBF before the deletion
//
NABoolean
CountingBloomFilter::remove(char * key, UInt32 key_len, UInt32 bucket, cbf_key::MFV_ENUM mfv)
{
   DISPLAY_KEY4("remove start", key, key_len, bucket);

   if (bucket >= numBuckets_) 
     return FALSE;

   UInt32 flags = ExHDPHash::NO_FLAGS;
   UInt32 hashValueCommon = ExHDPHash::hash(key, keyLenInfo_, key_len);

   ULng32 currentFreq =0;
   UInt32 f = 0xFFFFFFFF;
   NABoolean slotFound = FALSE;

   //
   // max value for CQD USTAT_IUS_TOTAL_UEC_CHANGE_THRESHOLD is 6.
   // 
   UInt32 cached_hash[6];

   for(Lng32 i = 0; i < k_; i++)
   {
      UInt32 hash_index = cached_hash[i] = computeFinalHash(hashValueCommon, i);

      UInt32 b= bucketNums_[hash_index];

      // We only check hash slot that belongs to the bucket
      if ( b > 0 && b-1 == bucket ) {

        // If the content is zero, the key is not in CBF
        if ( freqsL_[hash_index] == 0 )
           return FALSE;

        // Decrement the counter, if the counter already overflows, sub is a no-op.
        // The minuend is saved in currentFreq.
        freqsL_.sub(hash_index,1,currentFreq);

        // If the last occurance of the key (before decrement) is 1, reset the bucket# to 0;
        if ( currentFreq == 1 )
            bucketNums_.put(hash_index, 0);

        // Compute the min freq among slots occupied by the keys from the same bucket.
        if ( i==0 || f > currentFreq ) {
          f=currentFreq;
          slotFound = TRUE;
        }
      }
   }

   if ( !slotFound )
     return FALSE;

   ULng32 dummy;

   NABoolean lowFreq = TRUE;

   if ( f >= freqsL_.getMaxVal() ) {

       cbf_key ckey(key, key_len, bucket, mfv, heap_);

       UInt64* freq = (mfv != cbf_key::NONE) ? searchOverflowTable(ckey) : NULL;
  
       // Really high frequency key
       if ( freq ) {
         lowFreq = FALSE;
         if ( *freq  == freqsL_.getMaxVal() ) {
  
           // The overflow condition is no longer true. remove it from overflow area
           removeFromOverflowTable(ckey);
    
           // Transfer f2 to low freq array 
           (*freq2sL_[bucket]).add(f-1,1, dummy); 
    
           // Set the count entry to max-1, as the content of the entry is 
           // sticky once reach the max value.
           for(Lng32 i = 0; i < k_; i++) 
             freqsL_.put(cached_hash[i], freqsL_.getMaxVal()-1);
         } else
           (*freq) --;
       }
   }

   if ( lowFreq ) {

      // A low freq key 
   
      // Adjust frequency of frequency
      (*freq2sL_[bucket]).add(f-1,1, dummy);
      (*freq2sL_[bucket]).sub(f,1, dummy);

      // Decrement #uec 
      if ( f == 1 ) 
        uecs_.sub(bucket,1,dummy); 

      // Decrement RC
      totalFreqs_.sub(bucket,1,dummy); 
   }
     
   DISPLAY_MSG("remove complete");
         
   return TRUE;
}



NABoolean CountingBloomFilter::contain(char *key, UInt32 key_len, UInt64* freq, UInt32* b)
{
   DISPLAY_KEY("contain() start", key, key_len);

   UInt32 flags = ExHDPHash::NO_FLAGS;
   UInt32 hashValueCommon = ExHDPHash::hash(key, keyLenInfo_, key_len);
   UInt32 f = 0;
   UInt32 hash_index = 0;

   for(Lng32 i = 0; i < k_; i++)
   {
      hash_index = computeFinalHash(hashValueCommon, i);

      DISPLAY_HASH_ENTRY("", hash_index);

      if ( freqsL_[hash_index] == 0 )
      {
        DISPLAY_MSG("contain(): not found");
        return FALSE;
      }

      if ( freq ) {
         UInt32 currentFreq = freqsL_[hash_index];

         if (i==0 || f > currentFreq) {
           f=currentFreq;
         }
      }
   }

   if ( freq ) {

      if ( f == freqsL_.getMaxVal() ) {
         cbf_key ckey(key, key_len, 0, cbf_key::NONE, heap_);
         UInt64* fptr = searchOverflowTable(ckey);
         if ( fptr ) {
            (*freq) = *fptr;
         } else {
            assert(fptr != 0);
         }
      } else {
         *freq = f;
      }
   }

   if ( b )
     *b = bucketNums_[hash_index];

   DISPLAY_MSG("contain(): found");
   return TRUE;
}


/////////////////////////////////////////////////////////////////////////////////
//
// Methods for General CBF.
//
/////////////////////////////////////////////////////////////////////////////////
GeneralCountingBloomFilter::GeneralCountingBloomFilter(NAHeap* heap,
                                         UInt32 maxHashFuncs,
                                         UInt32 n, // # of distinct elements
                                         float p,  // probability of false positives
                                         UInt32 nonOverflowFreq, // non-overflow freq of n elements
                                         UInt32 avgSkewedElements,
                                         UInt32 numFreqFreqBuckets
                                         ):
  CountingBloomFilter(heap, maxHashFuncs, n, p,nonOverflowFreq, avgSkewedElements, numFreqFreqBuckets),
  overflowCountTable_(cbfHashFunc, avgSkewedElements,
                      TRUE/*uniqueness enforced*/, heap)
{
}

GeneralCountingBloomFilter::~GeneralCountingBloomFilter() 
{
}

void GeneralCountingBloomFilter::insertIntoOverflowTable(const cbf_key& key)
{
   overflowCountTable_.insert(new (heap_)cbf_key(key), new UInt64(freqsL_.getMaxVal()));
}


void GeneralCountingBloomFilter::removeFromOverflowTable(const cbf_key& key)
{
   overflowCountTable_.remove((cbf_key*)(&key));
}


void GeneralCountingBloomFilter::computeOverflowF2s()
{
   NADELETEBASIC(freqsH_, heap_);
   NADELETEBASIC(freq2sH_, heap_);
   freqsH_ = NULL;
   freq2sH_ = NULL;

   actualOverflowF2s_ = 0;

   UInt32 overflowed = getOverflowEntries();


   if ( overflowed == 0 )
     return;

   // cout << "overflowed=" << overflowed << endl;

   freqsH_ = (freqStruct*) (new (heap_) char[sizeof(freqStruct) * overflowed]);
   //freqsH_ = new (heap_) freqStruct[overflowed]; // does not work with NADELETEARRAY
                                                         // or NADELETEBASIC

   freq2sH_ = new (heap_) UInt64[overflowed];

   NAHashDictionaryIterator<cbf_key, UInt64> cItor(overflowCountTable_);

   cbf_key* key;
   UInt64* freq;
   CollIndex i;
   for ( i = 0 ; i < cItor.entries() ; i++) {
       cItor.getNext(key, freq);
       freqsH_[i].setFreq(*freq);
       freqsH_[i].setBucket(key->getBucket());
   }

   // do a quick sort
   qsort(freqsH_, cItor.entries(), sizeof(freqStruct), compareTwoFreqStructs);


   // compute freqOfFreq by walking through the sorted list in one pass
   UInt64 f = 0;
   i = 0;
   CollIndex j = 0;

   while (i<overflowed)
   {
      f=1;
      j=i+1;

      while ( j<overflowed && freqsH_[i] == freqsH_[j] ) {
        f++; j++;
      }

      freqsH_[actualOverflowF2s_] = freqsH_[i];
      freq2sH_[actualOverflowF2s_++] = f;

      i=j;
   }
}

/////////////////////////////////////////////////////////////////////////////////
//
// Methods for CBFWithKnownSkews.  
//
/////////////////////////////////////////////////////////////////////////////////

UInt64 CountingBloomFilterWithKnownSkews::estimateMemoryInBytes(
                                UInt32 maxHashFuncs,
                                UInt32 n, // # of distinct elements
                                float p,  // probability of false positives
                                 // non-overflow freq of n elements
                                UInt32 nonOverflowFreq,
                                UInt32 numSkewedElements,
                                UInt32 numFreqFreqBuckets
                      )
{
  UInt64 sz = CountingBloomFilter::estimateMemoryInBytes(
                                maxHashFuncs,
                                n, // # of distinct elements
                                p,  // probability of false positives
                                 // non-overflow freq of n elements
                                nonOverflowFreq,
                                numSkewedElements,
                                numFreqFreqBuckets
                                );

  sz += numSkewedElements * sizeof(UInt64);

  sz += sizeof(CountingBloomFilterWithKnownSkews);

  return sz;
}


CountingBloomFilterWithKnownSkews::CountingBloomFilterWithKnownSkews(NAHeap* heap,
                                         UInt32 maxHashFuncs,
                                         UInt32 n, // # of distinct elements
                                         float p,  // probability of false positives
                                         UInt32 nonOverflowFreq, // non-overflow freq of n elements
                                         UInt32 numSkewedElements,
                                         UInt32 numFreqFreqBuckets
                                         ):
  CountingBloomFilter(heap, maxHashFuncs, n, p,nonOverflowFreq, numSkewedElements, numFreqFreqBuckets)
{
   // We use freq2sH_ to record frequency in overflow area for this class and
   // make the following assumptions
   // 
   // 1). MVF and 2MFV per interval are distinct 
   // 2). Assume no new MVF and new 2MFV to come
   // 3). Frequency of frequency on MFV and 2MFV per interval is 1.

   freq2sH_ = new (heap_) UInt64[numSkewedElements];

   for ( CollIndex i = 0 ; i < numSkewedElements; i++) {
     freq2sH_[i] = 0;
   }

   actualOverflowF2s_ = numSkewedElements;

   
   Lng32 sz = 
     (Lng32)CountingBloomFilterWithKnownSkews::estimateMemoryInBytes( 
                            maxHashFuncs, 
                            n, // # of distinct elements
                            p,  // probability of false positives
                            // non-overflow freq of n elements
                            nonOverflowFreq,
                            numSkewedElements,
                            numFreqFreqBuckets 
                          );

   setTotalMemSize(sz);
}


CountingBloomFilterWithKnownSkews::CountingBloomFilterWithKnownSkews(NAHeap* heap) :
  CountingBloomFilter(heap)
{
}


CountingBloomFilterWithKnownSkews::~CountingBloomFilterWithKnownSkews() 
{
}


ULng32 CountingBloomFilterWithKnownSkews::packIntoBuffer(char*& buffer, NABoolean swapBytes)
{
   ULng32 sz = CountingBloomFilter::packIntoBuffer(buffer, swapBytes);

   for ( CollIndex i = 0 ; i < actualOverflowF2s_; i++) {
      sz += pack(buffer, freq2sH_[i], swapBytes);
   }

   return sz;
}

ULng32 CountingBloomFilterWithKnownSkews::unpackBuffer(char*& buffer)
{
   ULng32 sz = CountingBloomFilter::unpackBuffer(buffer);

   // actualOverflowF2s_ should be unpacked and available after 
   // calling the above method.
   freq2sH_ = new (heap_) UInt64[actualOverflowF2s_];

   for ( CollIndex i = 0 ; i < actualOverflowF2s_; i++) {
     sz += unpack(buffer, freq2sH_[i]);
   }

   setTotalMemSize(sz);

   return sz;
}

UInt64* CountingBloomFilterWithKnownSkews::searchOverflowTable(const cbf_key& key)
{
   UInt32 idx = indexInOverflowArray(key);
   ComASSERT(idx <= actualOverflowF2s_);

   return ( freq2sH_[idx] == 0 ) ? NULL : &freq2sH_[idx];
}

void CountingBloomFilterWithKnownSkews::insertIntoOverflowTable(const cbf_key& key)
{
   UInt32 idx = indexInOverflowArray(key);
   ComASSERT(idx <= actualOverflowF2s_);

   freq2sH_[idx] = freqsL_.getMaxVal();
}


void CountingBloomFilterWithKnownSkews::removeFromOverflowTable(const cbf_key& key)
{
   UInt32 idx = indexInOverflowArray(key);
   ComASSERT(idx <= actualOverflowF2s_);

   freq2sH_[idx] = 0;
}


UInt64 CountingBloomFilterWithKnownSkews::highF2(UInt32 k, UInt64& i, UInt32& bucket)
{
  ComASSERT(k <= actualOverflowF2s_);

  i = freq2sH_[k]; // freq2sH_ here stores the first order frequency
  bucket = k/2;  

  return 1;
}

UInt64 CountingBloomFilter::totalFreqForAll()
{
   UInt64 ct = 0;
   for ( CollIndex b = 0; b<numBuckets(); b++ ) {
       ct += totalFreq(b); // tf is total frequency in bucket b
  }

  computeOverflowF2s();

  UInt64 y;
  UInt32 bkt;
  for (CollIndex i=0; i<getOverflowEntries(); i++) {
     UInt64 x=highF2(i, y, bkt); // x, y and bkt are freqFreq, freq and
                                 // bucket# of the ith entry
                                 // in overflow area, respectively.
     ct += y;
  }
  return ct;
}

//--------------------------  code for faststats  ---------------------------------
void FastStatsCountingBloomFilter::computeSumOfFrequencySquaredHighFreq(double * sumSq)
{
  assert(0); // not implemented yet.
}

FastStatsCountingBloomFilter::FastStatsCountingBloomFilter(NAHeap* heap,
                                         UInt32 maxHashFuncs,
                                         UInt32 n, // # of distinct elements
                                         float p,  // probability of false positives
                                         UInt32 maxNonOverflowFreq
                                         ):
  BloomFilter(heap, maxHashFuncs, n, p),
  overflowCountTable_(scbfHashFunc, 1000 /* initial # of elements */,
                      TRUE/*uniqueness enforced*/, heap),
  counters_(m_, (UInt32)bitsNeeded(maxNonOverflowFreq), heap_),
  keys_(heap, n/* initial # of elements*/)
{
}

FastStatsCountingBloomFilter::~FastStatsCountingBloomFilter()
{
}


UInt64 FastStatsCountingBloomFilter::getSizeInBytes(
                                UInt32 maxHashFuncs,
                                UInt32 n, // # of distinct elements
                                float p,  // probability of false positives
                                 // non-overflow freq of n elements
                                UInt32 nonOverflowFreq,
                                NABoolean isChar, Int32 actualFixAmount)
{
   UInt32 m;
   UInt16 k;

   computeParams(maxHashFuncs, n, p, m, k);

   UInt64 totalMem = sizeof(BloomFilter);

   // for counters_
   totalMem += VarUIntArray::estimateMemoryInBytes(
                    m, (UInt32)bitsNeeded(nonOverflowFreq));


   // for keys_, covering all allocated memory
   totalMem += keys_.getByteSize();

   // include the keys
   if ( isChar ) {
      for (CollIndex i=0; i<keys_.entries(); i++ ) {
         const simple_cbf_key& key = keys_[i];
         totalMem += key.getKeyLen();
      }
   } else {
      // if it is a non-character key, subtract the amount if the actual
      // data is stored instead of the pointer.
      // -sizoef(key*) + actualFixAmount

      totalMem -= (keys_.entries() * sizeof(void*));
      totalMem += (keys_.entries() * actualFixAmount);
   }


   // for hash directory
   totalMem += overflowCountTable_.getByteSize();

   return totalMem;
}

NABoolean
FastStatsCountingBloomFilter::insert(char * key, UInt32 key_len, UInt32 freq)
{
   UInt32 hashValueCommon = ExHDPHash::hash(key, keyLenInfo_, key_len);

   UInt32 f = 0xFFFFFFFF;
   ULng32 newFreq=0;
   NABoolean slotFound = FALSE;

   for(Lng32 i = 0; i < k_; i++)
   {
      UInt32 hash_index = computeFinalHash(hashValueCommon, i);

      counters_.add(hash_index, freq, newFreq);

      if ( i == 0 || f > newFreq ) {
         f = newFreq;
         slotFound = TRUE;
      }
   }

   simple_cbf_key ckey(key, key_len, heap_);

   if ( f == 1 ) { // a new key, insert into keys_
      keys_.insert(ckey);
   }

   if ( f >= counters_.getMaxVal() ) {

      // The key overflows

      // Check if it is the first time to insert the key in the high freq area
      UInt64* currentFreq = searchOverflowTable(ckey);
      if ( currentFreq ) {
         (*currentFreq) += freq; // yes, increment the frequency by freq
      } else {

         // No, insert the key with a frquency of freq-1
         insertIntoOverflowTable(ckey, freq-1);
      }

   }
   else
   {
      // The key stays in the low freq area. Do nothing as we have already
      // increment the frequency for each bit touched by the key.
   }
   DISPLAY_MSG("insert complete");

   return TRUE;
}


NABoolean FastStatsCountingBloomFilter::remove(char * key, UInt32 key_len)
{
   DISPLAY_KEY4("remove start", key, key_len, bucket);

   UInt32 flags = ExHDPHash::NO_FLAGS;
   UInt32 hashValueCommon = ExHDPHash::hash(key, keyLenInfo_, key_len);

   ULng32 currentFreq =0;
   UInt32 f = 0xFFFFFFFF;
   NABoolean slotFound = FALSE;

   //
   // max value for CQD USTAT_IUS_TOTAL_UEC_CHANGE_THRESHOLD is 6.
   //
   UInt32 cached_hash[6];

   for(Lng32 i = 0; i < k_; i++)
   {
      UInt32 hash_index = cached_hash[i] = computeFinalHash(hashValueCommon, i);

      // If the content is zero, the key is not in CBF
      if ( counters_[hash_index] == 0 )
         return FALSE;

      // Decrement the counter, if the counter already overflows, sub is a no-op.
      // The minuend is saved in currentFreq.
      counters_.sub(hash_index,1,currentFreq);

      // Compute the min freq among slots occupied by the keys from the same bucket.
      if ( i==0 || f > currentFreq ) {
        f=currentFreq;
        slotFound = TRUE;
      }
   }

   ULng32 dummy;

   NABoolean lowFreq = TRUE;

   if ( f >= counters_.getMaxVal() ) {

       simple_cbf_key ckey(key, key_len, heap_);

       UInt64* freq = searchOverflowTable(ckey);

       if ( freq ) {      // test if a high frequency key
         lowFreq = FALSE;
         if ( *freq  == counters_.getMaxVal() ) {

           // The overflow condition is no longer true. Remove it from overflow area
           removeFromOverflowTable(ckey);

           // Set the count entry to max-1, as the content of the entry is
           // sticky once reach the max value.
           for(Lng32 i = 0; i < k_; i++)
             counters_.put(cached_hash[i], counters_.getMaxVal()-1);
         } else
           (*freq) --;
       }
   }

   if ( lowFreq ) {
      // A low freq key. Do nothing here as we have already decrement the
      // counters.
   }

   return TRUE;
}

NABoolean
FastStatsCountingBloomFilter::contain(char * key, UInt32 key_len, UInt64* freq)
{
   DISPLAY_KEY("contain() start", key, key_len);

   UInt32 hashValueCommon = ExHDPHash::hash(key, keyLenInfo_, key_len);
   UInt32 f = 0;
   UInt32 hash_index = 0;

   for(Lng32 i = 0; i < k_; i++)
   {
      hash_index = computeFinalHash(hashValueCommon, i);

      DISPLAY_HASH_ENTRY("", hash_index);

      if ( counters_[hash_index] == 0 )
      {
        DISPLAY_MSG("contain(): not found");
        return FALSE;
      }

      if ( freq ) {
         UInt32 currentFreq = counters_[hash_index];

         if (i==0 || f > currentFreq) {
           f=currentFreq;
         }
      }
   }

   if ( freq ) {

      if ( f == counters_.getMaxVal() ) {
         simple_cbf_key ckey(key, key_len, heap_);
         UInt64* fptr = searchOverflowTable(ckey);
         if ( fptr ) {
            (*freq) = *fptr;
         } else {

            // the key is not in hash table, whicih means that
            // all bits associated with the key is set by collisions.
            // Here we have to guess the frequency to be the
            // max value -1.
            (*freq) = counters_.getMaxVal() -1;
         }
      } else {
         *freq = f;
      }
   }

   DISPLAY_MSG("contain(): found");
   return TRUE;
}

void
FastStatsCountingBloomFilter::insertIntoOverflowTable(
              const simple_cbf_key& key, UInt32 freq)
{
   overflowCountTable_.insert(new (heap_)simple_cbf_key(key),
                              new UInt64(counters_.getMaxVal() + freq - 1)
                             );
}


void FastStatsCountingBloomFilter::removeFromOverflowTable(const simple_cbf_key& key)
{
   overflowCountTable_.remove((cbf_key*)(&key));
}

void FastStatsCountingBloomFilter::printfreq(const char* colNames)
{
   printf("\nCBF keys/frequencies for %s\n", colNames);

   for (CollIndex i=0; i < 100;/*keys_.entries();*/ i++) {
      const simple_cbf_key& ckey = keys_.at(i);
      UInt64 freq;
      NABoolean ok = contain(ckey.getKey(), ckey.getKeyLen(), &freq);

      if ( ok ) {
        printf("key=%d, freq=" PF64 "\n", *((Int32*)ckey.getKey()), freq);
      }
   }
}

void FastStatsCountingBloomFilter::computeOverflowF2s()
{
/*
   NADELETEBASIC(freqsH_, heap_);
   NADELETEBASIC(freq2sH_, heap_);
   freqsH_ = NULL;
   freq2sH_ = NULL;

   actualOverflowF2s_ = 0;

   UInt32 overflowed = getOverflowEntries();


   if ( overflowed == 0 )
     return;

   // cout << "overflowed=" << overflowed << endl;

   freqsH_ = (freqStruct*) (new (heap_) char[sizeof(freqStruct) * overflowed]);
   //freqsH_ = new (heap_) freqStruct[overflowed]; // does not work with NADELETEARRAY
                                                         // or NADELETEBASIC

   freq2sH_ = new (heap_) UInt64[overflowed];

   NAHashDictionaryIterator<cbf_key, UInt64> cItor(overflowCountTable_);

   cbf_key* key;
   UInt64* freq;
   CollIndex i;
   for ( i = 0 ; i < cItor.entries() ; i++) {
       cItor.getNext(key, freq);
       freqsH_[i].setFreq(*freq);
       freqsH_[i].setBucket(key->getBucket());
   }

   // do a quick sort
   qsort(freqsH_, cItor.entries(), sizeof(freqStruct), compareTwoFreqStructs);


   // compute freqOfFreq by walking through the sorted list in one pass
   UInt64 f = 0;
   i = 0;
   CollIndex j = 0;

   while (i<overflowed)
   {
      f=1;
      j=i+1;

      while ( j<overflowed && freqsH_[i] == freqsH_[j] ) {
        f++; j++;
      }

      freqsH_[actualOverflowF2s_] = freqsH_[i];
      freq2sH_[actualOverflowF2s_++] = f;

      i=j;
   }
*/
}

