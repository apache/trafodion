// **********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2007-2014 Hewlett-Packard Development Company, L.P.
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
// **********************************************************************

#ifndef _QRSHAREDPTR_H_
#define _QRSHAREDPTR_H_

#include "SharedPtr.h"
#include "NAString.h"
#include "QRLogger.h"
#include "Collections.h"

/** \file 
 * Contains classes and defines used to implement the memory management schemes
 * used in the Query Rewrite implementation, including extensions of the shared
 * pointer mechanism. Three different protocols are utilized for dynamic memory
 * allocation/deallocation. A brief description of each is given below.
 * \par Shared pointers:
 * Heap memory is managed using QRIntrusiveSharedPtr, a derivative of the
 * \c SharedPtr class. Pointers are not explicitly deleted; memory is
 * automatically deallocated when the referenced object's reference count reaches
 * 0. The reference count is decremented automatically when a shared pointer
 * that references the object goes out of scope. This scheme will be in effect
 * when the code is compiled with _MEMSHAREDPTR defined and _MEMCHECK not defined.
 * \par Shared pointers with memory checking:
 * This scheme also uses QRIntrusiveSharedPtr, but always passes an instance of
 * QRMemCheckDeleter to its constructor to use as the "deleter" object for the
 * shared pointer. The deleter object is invoked when the reference count goes
 * to 0, rather than simply deleting the pointer. %QRMemCheckDeleter will, before
 * deleting the pointer itself, determine whether the object referenced by the
 * pointer has been marked as "deleted", indicating that a pseudo-delete has
 * been performed on the pointer. This protocol requires adding code to
 * explicitly handle memory deallocation, but only a simulation of it; the
 * shared pointer still deletes things, but through %QRMemCheckDeleter notifies
 * us if our manual deletion is thorough. When using this scheme, a class using
 * shared pointers should subclass QRMemCheckMarker, and perform the simulated
 * deletion by calling QRMemCheckMarker::mark. Shared pointers with memory checking
 * will be in effect when the code is compiled with both _MEMSHAREDPTR and
 * _MEMCHECK defined.
 * \par Conventional pointers:
 * In this scheme, shared pointers are not used, and all deletion of dynamically
 * allocated objects is done manually. The plan is to enable this mode once we
 * are reasonably confident that memory leaks have been eliminated, based on the
 * results of using shared pointers with memory checking. The conventional pointer
 * scheme will be in effect when the code is compiled with _MEMSHAREDPTR not
 * defined.
 * \n\n
 * The choice of a memory management protocol is made at compile time, and
 * carried out through conditional compilation. A number of defines are used to
 * limit the number of \c ifdef occurrences in the code.
 */

class QRMemCheckMarker;
template <class T> class QRMemCheckDeleter;
class NAIntrusiveSharedPtrObject;
template <class T> class QRIntrusiveSharedPtr;
template <class T> class QRIntrusiveSharedRefCountDel;

/**
 * Declares the ISHP_ variable (used by the intrusive shared pointer mechanism)
 * as an instance of QRIntrusiveSharedRefCountDel.
 *
 * @param[in] TYPE Type argument for instantiation of the QRIntrusiveSharedRefCountDel
 *                 class template.
 */
#define MEMCHECK_INTRUSIVE_SHARED_PTR(TYPE) QRIntrusiveSharedRefCountDel<TYPE> ISHP_

/**
 * \def _MEMSHAREDPTR
 * When defined, shared pointers will be used. If _MEMCHECK is also defined,
 * the shared pointers are used to detect memory leaks that would occur if the
 * shared pointers were not used.
 */

/**
 * \def _MEMCHECK
 * When this is defined in addition to _MEMSHAREDPTR, objects dynamically
 * allocated will be marked as "deleted" rather than actually deleting them,
 * to allow the shared pointer for the object to detect potential memory leaks.
 * This define has no effect if _MEMSHAREDPTR is not also defined.
 */

/**
 * \def createInstance(cls, var, heap)
 * Declares \c var as a pointer and initializes it with the address of a
 * dynamically allocated instance of class \c cls using the \c NAHeap pointed to
 * by \c heap. The actual expansion of this define depends on the memory
 * management protocol the code was compiled under.
 * \par Shared pointers:
 * The pointer is a shared pointer, implemented by the QRIntrusiveSharedPtr class.
 * \par Shared pointers with memory checking:
 * Also uses QRIntrusiveSharedPtr, but passes __FILE__ and __LINE__ to the
 * constructor for the new object, for memory tracking purposes.
 * \par Conventional pointers:
 * The pointer is a regular pointer instead of a shared pointer.
 * \n\n
 * Separate defines are used for constructors with 0, 1, or 2 parameters (not
 * counting the file and line that are added on for memory checking).
 */

/**
 * \def SHARED_PTR_REF_COUNT(X)
 * Declares a reference-counting object (an instance of some subclass of
 * \c SharedRefCountBase), if shared pointers are being used. The details are
 * determined by which of the three memory management protocols the code was
 * compiled with.
 * \par Shared pointers:
 * Declares ISHP_ as a IntrusiveSharedRefCount that references the containing class.
 * \par Shared pointers with memory checking:
 * Declares ISHP_ as a QRIntrusiveSharedPtr that references the containing class.
 * This will cause potential memory errors to be logged in addition to freeing
 * the memory when the reference count goes to 0.
 * \par Conventional pointers:
 * Does nothing.
 */

/**
 * \def deletePtr(ptr)
 * Performs any manual deletion activity needed for a pointer, depending on the
 * memory management protocol the code was compiled under.
 * \par Shared pointers:
 * Does nothing.
 * \par Shared pointers with memory checking:
 * Simulates manual deletion by marking the object.
 * \par Conventional pointers:
 * Deletes the pointer.
 */

#ifdef _MEMSHAREDPTR
#ifdef _MEMCHECK
#define createInstance(cls, var, heap) \
  QRIntrusiveSharedPtr<cls> var(new (heap) cls(__FILE__,__LINE__))
#define createInstance1(cls, var, heap, arg1) \
  QRIntrusiveSharedPtr<cls> var(new (heap) cls(arg1,__FILE__,__LINE__))
#define createInstance2(cls, var, heap, arg1, arg2) \
  QRIntrusiveSharedPtr<cls> var(new (heap) cls(arg1,arg2,__FILE__,__LINE__))
#define SHARED_PTR_REF_COUNT(X) MEMCHECK_INTRUSIVE_SHARED_PTR(X);
#define deletePtr(ptr) if (ptr) { ptr->mark(__FILE__, __LINE__); }
#else  /* _MEMSHAREDPTR but not _MEMCHECK */
#define createInstance(cls, var, heap) \
  QRIntrusiveSharedPtr<cls> var(new (heap) cls(), heap)
#define createInstance1(cls, var, heap, arg1) \
  QRIntrusiveSharedPtr<cls> var(new (heap) cls(arg1), heap)
#define createInstance2(cls, var, heap, arg1, arg2) \
  QRIntrusiveSharedPtr<cls> var(new (heap) cls(arg1,arg2), heap)
#define SHARED_PTR_REF_COUNT(X) INTRUSIVE_SHARED_PTR(X);
#define deletePtr(ptr)
#endif
#else  /* neither _MEMSHAREDPTR nor _MEMCHECK */
#define createInstance(cls, var, heap) cls* var = new (heap) cls
#define createInstance1(cls, var, heap, arg1) cls* var = new (heap) cls(arg1)
#define createInstance2(cls, var, heap, arg1, arg2) cls* var = new (heap) cls(arg1, arg2)
#define SHARED_PTR_REF_COUNT(X)
#define deletePtr(ptr) delete ptr
#endif

#ifdef _MEMSHAREDPTR
#define NAPtrList  NAShPtrList
#define NAPtrArray NAShPtrArray
#define PTR_TO_TYPE(T) QRIntrusiveSharedPtr<T>
#else
#define NAPtrList  NAList
#define NAPtrArray NAArray
#define PTR_TO_TYPE(T) T*
#endif

#if defined(_MEMSHAREDPTR) && defined(_MEMCHECK)
#define MEMCHECK_ARGS __FILE__, __LINE__
#define ADD_MEMCHECK_ARGS(arg) arg, __FILE__, __LINE__
#define CHILD_ELEM_ARGS(arg) this, atts, arg, __FILE__, __LINE__
#define ADD_MEMCHECK_ARGS_DECL(arg) arg, char *fileName = __FILE__, Int32 lineNumber = __LINE__
#define ADD_MEMCHECK_ARGS_DEF(arg)  arg, char *fileName, Int32 lineNumber
#define ADD_MEMCHECK_ARGS_PASS(arg) arg, fileName, lineNumber
#else
#define MEMCHECK_ARGS
#define ADD_MEMCHECK_ARGS(arg) arg
#define CHILD_ELEM_ARGS(arg) this, atts, arg
#define ADD_MEMCHECK_ARGS_DECL(arg) arg
#define ADD_MEMCHECK_ARGS_DEF(arg)  arg
#define ADD_MEMCHECK_ARGS_PASS(arg) arg
#endif


/**
 * Class used in conjunction with shared pointers to mark objects in lieu of
 * deleting them, so the shared pointer mechanism can be used to locate memory
 * errors. When compiled to perform this kind of memory checking (both _MEMSHAREDPTR
 * and _MEMCHECK defined), an instance of a class derived from \c QRMemCheckMarker
 * that is referenced by a \c QRIntrusiveSharedPtr can be marked as "deleted" with the
 * \c deletePtr macro. When the shared pointer object goes out of scope, its
 * associated deleter object (an instance of QRMemCheckDeleter) will check whether
 * the contained pointer has been marked before actually deleting it, and log it
 * as a memory leak if not. In addition, when marking an object as deleted,
 * QRMemcheckMarker will log a "double deletion" memory error if the object is
 * already marked.
 */
class QRMemCheckMarker : public NABasicObject
{
  public:
    /**
     * Creates an instance of this class without the file name and line number at
     * which the referenced object was allocated.
     */
    QRMemCheckMarker() 
      : marked_(FALSE) 
      {}

    /**
     * Creates an instance of this class with the file name and line number at
     * which the referenced object was allocated.
     *
     * @param[in] allocFileName   Name of the file in which the instance was allocated.
     * @param[in] allocLineNumber Line number at which the instance was allocated.
     */
    QRMemCheckMarker(NAMemory *heap, char* allocFileName, Int32 allocLineNumber) 
      : allocFileName_((allocFileName ? allocFileName : ""), heap),
        allocLineNumber_(allocLineNumber),
        marked_(FALSE) 
      {}

    /**
     * Tells if this instance of the class has been marked as deleted.
     *
     * @return \c TRUE if the instance has been marked.
     */
    NABoolean isMarked()
      {
        return marked_;
      }

    /**
     * Requests that this instance be marked as deleted. Although not actually
     * deleted until the referencing shared pointer goes out of scope, marking
     * an object indicates that it will be appropriately deleted if shared
     * pointers are not used. If the object is already marked, a log entry will
     * be posted indicating that a memory error would have occurred if conventional
     * pointer were being used.
     *
     * @param[in] delFileName   Name of file from which the call came.
     * @param[in] delLineNumber Line number at which the call was issued.
     */
    void mark(char* delFileName, Int32 delLineNumber)
      {
        if (marked_)
          {
            QRLogger::log(CAT_SQL_MEMORY, LL_WARN,
              ">>> !!! %s instance deleted more than once: line %d in %s",
			className(), delLineNumber, delFileName);
          }
        else
          marked_ = TRUE;
      }

    void checkDanglingPtr() const
      {
        if (marked_)
          {
            QRLogger::log(CAT_SQL_MEMORY, LL_WARN,
              ">>> !!! Pointer to %s instance used after deletion", className());
          }
      }

    /**
     * Returns the name of the file where the \c new of this instance took place.
     * This is used by \c QRMemCheckDeleter if it needs to log information for a
     * memory leak.
     *
     * @return Name of the file from which this instance was allocated.
     */
    NAString& getAllocFileName()
      {
        return allocFileName_;
      }

    /**
     * Returns the line number where the \c new of this instance took place.
     * This is used by \c QRMemCheckDeleter if it needs to log information for a
     * memory leak.
     *
     * @return Line number at which this instance was allocated.
     */
    Int32 getAllocLineNumber()
      {
        return allocLineNumber_;
      }

    /**
     * Returns the class name of the allocated instance. 
     * In _MEMCHECK mode, this method uses RTTI to return its own class name. 
     * To enable RTTI compile with /GR. 
     * This is used by \c QRMemCheckDeleter if it needs to log information for a
     * memory leak.
     *
     * @return Class name of the allocated instance.
     */
    virtual const char* className() const
      {
#if defined _MEMSHAREDPTR && _MEMCHECK
	return typeid(*this).name();
#else
        return "";
#endif
      }

  private:
    NAString allocFileName_;
    Int32 allocLineNumber_;
    NABoolean marked_;
};

/**
 * A deleter class for use with shared pointers. If an instance of this class
 * is passed to the constructor of a \c SharedPtr, then before deleting the
 * object referenced by the shared pointer, it will check to see if the object
 * has been marked (by \c QRMemCheckMarker). If not, the objet would have been
 * leaked if not using a shared pointer.
 */
template <class T>
class QRMemCheckDeleter
{
  public:
    /**
     * Returns a preexisting instance of \c QRMemCheckDeleter. Since the choice
     * of an instance is arbitrary, we provide one (for each template
     * instantiation) that will always be at hand. This class is not implemented
     * as a singleton class, so it is also possible to create additional instances.
     *
     * @return A static instance of \c QRMemCheckDeleter.
     */
    static QRMemCheckDeleter& instance()
      {
        static QRMemCheckDeleter deleter;
        return deleter;
      }

    /**
     * Constructs a default instance of QRMemCheckDeleter. This is generally
     * not needed by clients, who can use the static instance returned by
     * #instance.
     */
    QRMemCheckDeleter()
      {}

    /**
     * Deletes the passed pointer after checking to see if it has been marked.
     * Marking is done by calling QRMemCheckMarker#mark (<tt>ptr</tt>'s type must be
     * a subclass of QRMemCheckMarker), and indicates that the
     * object would have been manually deleted if shared pointers were not in use.
     * If an unmarked object is deleted, this function logs an entry indicating
     * a potential memory leak.
     *
     * @param[in] ptr Pointer to the object being deleted.
     */
    void operator()(T* ptr) const
      {
        if (!ptr->isMarked())
          {
#if defined _MEMSHAREDPTR && _MEMCHECK
            const char* clsName = typeid(*ptr).name();
#else
            const char* clsName = ptr->className();
#endif
            if (!clsName)
              {
                QRLogger::log(CAT_SQL_MEMORY, LL_ERROR,
                                          ">>> !!! memory leak");
              }
              else
              {
                QRLogger::log(CAT_SQL_MEMORY, LL_ERROR,
                  ">>> !!! Instance of %s created at line %d in %s was leaked",
                  clsName, ptr->getAllocLineNumber(), ptr->getAllocFileName().data());
              }
          }
        delete ptr;
      }
};

/**
 * Subclass of \c IntrusiveSharedPtr that allows assignment from a conventional
 * pointer. The same assignment syntax can be used with \c IntrusiveSharedPtr,
 * but this causes an interim shared pointer object to be constructed, which is
 * then assigned to the target shared pointer object using copy assignment. This
 * subclass provides an operator= that allows it to be done without the extra
 * object creation.
 */
template<class T>
class QRIntrusiveSharedPtr : public IntrusiveSharedPtr<T>
{
  public:
    QRIntrusiveSharedPtr() {}

    template<class Y>
    QRIntrusiveSharedPtr(Y *t)
      : IntrusiveSharedPtr<T>(t)
      {}

    template<class Y>
    QRIntrusiveSharedPtr(const IntrusiveSharedPtr<Y> &rhs)
      : IntrusiveSharedPtr<T>()
      {
        SharedPtr<T>::objectP_ = static_cast<T*>(rhs.get());
        SharedPtr<T>::refCount_ = (SharedRefCountBase<T>*)&SharedPtr<T>::objectP_->ISHP_;
        if (SharedPtr<T>::refCount_)
          {
            SharedPtr<T>::refCount_->objectP_ = SharedPtr<T>::objectP_;
            SharedPtr<T>::refCount_->useCount_ = rhs.getUseCount();
            SharedPtr<T>::refCount_->incrUseCount();
          }
      }

    /**
     * Allows construction with a NULL, creates a null-valued shared pointer.
     * A separate constructor is required because using the macro NULL (which
     * is 0) will not cause the "normal" constructor to be invoked.
     *
     * @param[in] i This will generally be 0 (NULL). 
     */
    QRIntrusiveSharedPtr(const Int32 i) : IntrusiveSharedPtr<T>(i) {} 

    //@ZX
    virtual ~QRIntrusiveSharedPtr()
      {
      }

    /**
     * Reinitializes this shared pointer to reference \c ptr. If this shared
     * pointer currently references a different object, the object's reference
     * counter (which is a member of that object since this is an intrusive
     * shared pointer) is decremented. This shared pointer will henceforth use
     * the reference counter for \c ptr, which is incremented to reflect the
     * new reference it has gained.
     *
     * @param[in] ptr Pointer to object to be referenced by this shared pointer.
     * @return A reference to the assigned-to shared pointer.
     */
    QRIntrusiveSharedPtr<T>& operator=(T *ptr)
      {
        if (SharedPtr<T>::refCount_)
          SharedPtr<T>::refCount_->decrUseCount();

        // Reconstruct the reference count from object
        SharedPtr<T>::objectP_ = (T*)ptr;
        SharedPtr<T>::refCount_ = (SharedRefCountBase<T>*)&ptr->ISHP_;
        SharedPtr<T>::refCount_->objectP_ = (T*)ptr;
        SharedPtr<T>::refCount_->useCount_++;
        return *this;
      }

    /**
     * Sets the referenced object of this shared pointer to that of another one.
     * The reference count for the current value is decremented, and the count
     * for the new one is incremented. After the assignment takes place, both
     * shared pointers reference the same object.
     *
     * @param[in] rhs Shared pointer to assign to this one.
     * @return A reference to the assigned-to shared pointer.
     */
    template<class Y>
    // @ZX
    //QRIntrusiveSharedPtr<T>& operator=(const QRIntrusiveSharedPtr<Y>  &rhs)
    QRIntrusiveSharedPtr<T>& operator=(const IntrusiveSharedPtr<Y>  &rhs)
      {
        if (SharedPtr<T>::objectP_ != rhs.get())  // don't copy if same object
          {
            if (SharedPtr<T>::refCount_) 
              {
#ifdef _DEBUG
                assert(SharedPtr<T>::objectP_ == SharedPtr<T>::refCount_->objectP_);
#endif
                SharedPtr<T>::refCount_->decrUseCount();
              }
            
            // @ZX
            //objectP_ = rhs.get();
            SharedPtr<T>::objectP_ = static_cast<T*>(rhs.get());
            SharedPtr<T>::refCount_ = (SharedRefCountBase<T>*)&SharedPtr<T>::objectP_->ISHP_;
            if (SharedPtr<T>::refCount_)
              {
                SharedPtr<T>::refCount_->objectP_ = SharedPtr<T>::objectP_;
                SharedPtr<T>::refCount_->useCount_ = rhs.getUseCount();
                SharedPtr<T>::refCount_->incrUseCount();
              }
          }
        return *this;
      }

    /**
     * Assigns an integer value (should be 0) to this shared pointer. This
     * overload of operator= is necessary because "sp = NULL", where sp is a
     * shared pointer will not match the version that take a pointer.
     *
     * @param[in] i Should be 0 (NULL) in almost all cases.
     * @return A reference to the assigned-to shared pointer.
     */
    QRIntrusiveSharedPtr<T>& operator= (const Int32 i)
      {
        (void)SharedPtr<T>::operator=(i);
        return *this;
      }

#if defined(_MEMSHAREDPTR) && defined(_MEMCHECK)
    T& operator*() const
      {
#ifdef _DEBUG
        if (refCount_)
          assert(objectP_ == refCount_->objectP_);
#endif
        objectP_->checkDanglingPtr();
        return *objectP_;
      }

    T* operator->() const
      {
#ifdef _DEBUG
        if (refCount_)
          assert(objectP_ == refCount_->objectP_);
#endif
        objectP_->checkDanglingPtr();
        return objectP_;
      }
#endif
};

/**
 * A reference counter for QRIntrusiveSharedObject that uses QRMemCheckDeleter
 * to delete the contained pointer when the reference count goes to 0. Intrusive
 * shared pointers use a reference counting object that is a member of the
 * class pointed to by the object pointer with a distinguished name (ISHP_).
 * When ISHP_ is an instance of QRIntrusiveSharedRefCountDel, it will delete
 * the underlying pointer using QRMemCheckDeleter rather than doing a simple
 * delete.
 */
template<class T>
class QRIntrusiveSharedRefCountDel : public SharedRefCountBase<T> 
{
  public:
    /**
     * Constructs an uninitialized instance of this class.
     */
    QRIntrusiveSharedRefCountDel() {}

    /**
     * Constructs a reference counter for the given pointer with the given
     * initial use count.
     *
     * @param[in] t         Pointer to object for which references will be counted.
     * @param[in] use_count Initial number of references to the object.
     * @return 
     */
    QRIntrusiveSharedRefCountDel(T *t, Int32 use_count)
      : SharedRefCountBase<T>(t, use_count)
      {}

    /**
     * Uses the operator() of QRMemCheckDeleter to delete the object referenced
     * by the shared pointer. That operator also checks to see if the object has
     * been marked for deletion, solely for the purpose of tracking potential
     * memory leaks.
     */
    virtual void destroyObjects()
      {
        // Only the object pointer should be deleted for this class.
        QRMemCheckDeleter<T>::instance()(SharedRefCountBase<T>::objectP_);
      }
};

/**
 * Base class for any class that implements memory checking using shared pointers.
 * When conditionally compiled to use intrusive shared pointers (_MEMSHAREDPTR),
 * this class includes a reference counting structure. When additionally compiled
 * in memory-checking mode (_MEMCHECK), it will also subclass QRMemCheckMarker
 * to allow tracking potential memory leaks and double deletions.
 */
class NAIntrusiveSharedPtrObject
#if defined(_MEMSHAREDPTR) && defined(_MEMCHECK)
      : public QRMemCheckMarker
#else
      : public NABasicObject
#endif
{
  public:
    /**
     * Constructs an object usable with intrusive shared pointers, and possibly
     * enabled memory checking, depending on defines.
     *
     * @param[in] fileName Name of the file from which this constructor was called.
     * @param[in] lineNumber Line number at which this constructor was called.
     */
#if defined(_MEMSHAREDPTR) && defined(_MEMCHECK)
    NAIntrusiveSharedPtrObject(NAMemory* heap, char *fileName = NULL, Int32 lineNumber = 0)
      : QRMemCheckMarker(heap, fileName, lineNumber)
      {}
#else
    NAIntrusiveSharedPtrObject(NAMemory* heap)
      {}
#endif

    /** \var ISHP_
     * This is a reference-counting object generated as a member variable when
     * the code is compiled with _MEMSHAREDPTR defined.
     */

    SHARED_PTR_REF_COUNT(NAIntrusiveSharedPtrObject);
};

// Define initial hash sizes that are prime numbers.
#define INIT_HASH_SIZE_SMALL  11
#define INIT_HASH_SIZE_LARGE 101

/**
 * SharedPtrValueHash: a NAHashDictionary for use with Shared Pointers.
 * The value class is expected to be derived from NAIntrusiveSharedPtrObject.
 * The key class is expected to be a non-NAIntrusiveSharedPtrObject class.
 * This class attempts to solve the problem that a NAHashDictionary<K,V> is 
 * actually handling pointers to K and V, and don't know how to correctly 
 * handle shared pointers. Therefore, this class actually stores pointers to 
 * shared pointer objects, and handles the dereferencing.
 */
#ifndef _MEMSHAREDPTR
  #define SharedPtrValueHash  NAHashDictionary
#else
  #define super NAHashDictionary<K, QRIntrusiveSharedPtr<V> >
  template <class K, class V>
  class SharedPtrValueHash : public super
  {
  public:
    /**
    * Constructor.
    * @param hashFunction Pointer to a hash function
    * @param initialHashSize Initial size of hash table.
    * @param enforceUniqueness When FALSE rejects multiple values for the same key
    * @param heap The heap pointer
    */
    SharedPtrValueHash(ULng32(*hashFunction)(const K&), 
		  ULng32 initialHashSize = NAHashDictionary_Default_Size,
		  NABoolean enforceUniqueness = FALSE,
		  CollHeap * heap=0)
    // Override constructor to save the heap pointer.
    :  super(hashFunction, initialHashSize, enforceUniqueness, heap)
      ,heap_(heap)
      ,nullSharedPtrVal(NULL)
      {}

    /**
    * Copy Constructor
    * @param other The object to copy
    * @param heap A Heap pointer
    */
    SharedPtrValueHash(const super& other, CollHeap * heap=0)
      : super(other, heap)
      ,heap_(heap)
      ,nullSharedPtrVal(NULL)
    {}

    /**
    * Destructor
    */
    virtual ~SharedPtrValueHash()
    {
      clear(FALSE);
    }

    /**
    * Check whether this dictionary contains a certain key, or a 
    * certain key value pair. 
    * @param key 
    * @param value 
    * @return 
    */
    inline NABoolean contains(const K* key, const QRIntrusiveSharedPtr<V> value = NULL) const
    { 
      if (value)
      {
	const QRIntrusiveSharedPtr<V>* vp = &value;
	return super::contains(key, vp);
      }
      else
	return super::contains(key);
    }

    /**
    * Insert a (key, value) pair. Allocate a pointer to the shared pointer 
    * to the value object, and insert the (key, QRIntrusiveSharedPtr<value>)
    * pair into the hash table.
    * @param key Pointer to key object.
    * @param value Shared pointer to value object.
    * @return The pointer to the key object.
    */
    K* insert(K* key, QRIntrusiveSharedPtr<V> value)
    {
      QRIntrusiveSharedPtr<V>* vp = new(heap_) QRIntrusiveSharedPtr<V>;
      *vp = value;
      return super::insert(key, vp);
    }

    /**
    * Get the first value corresponding to the input key. Dereference the
    * value on the way out.
    * @param key Pointer to key value.
    * @return Shared pointer to value.
    */
    QRIntrusiveSharedPtr<V> getFirstValue(const K* key) const
    {
      // Avoid dereferencing null value key is not in the hash table.
      QRIntrusiveSharedPtr<V>* firstValPtr = super::getFirstValue(key);
      if (firstValPtr)
        return *firstValPtr;
      else
        return nullSharedPtrVal;
      //return *(super::getFirstValue(key));
    }

    /**
    * Remove the (key, value) pair, and delete the pointer to the 
    * shared pointer to the value.
    * @param key 
    * @return 
    */
    K* remove(K* key)
    {
      QRIntrusiveSharedPtr<V>* vp = super::getFirstValue(key);
      K* result = super::remove(key);
      if (result != NULL)
	NADELETEBASIC(vp, heap_);
      return result;
    }

    /**
    * Clear the contents of the hash table, and delete memory used for 
    * internal pointers.
    * @param deleteContents 
    */
    void clear(NABoolean deleteContents = FALSE)
    {
      // Iterate on the hash table.
      QRIntrusiveSharedPtr<V>* vp;
      K* k; 
      NAHashDictionaryIterator<K, QRIntrusiveSharedPtr<V> > iter(*this);
      for ( CollIndex i = 0 ; i < iter.entries() ; i++) 
      {
	// For each (key, value) pair
	iter.getNext (k, vp); 
	// Delete the shared pointer if requested to.
	if (deleteContents)
	{
	  QRIntrusiveSharedPtr<V> v = *vp;
	  deletePtr(v);
	}
	// Delete the pointer to the shared pointer.
	NADELETEBASIC(vp, heap_);
      }
      // Now, really clear the contents.
      super::clear(FALSE);
    }

  private:
    // Unfortunatly, the heap_ member in NAHashDictionary is private, not protected.
    CollHeap* heap_;

    // This is a value for getFirstValue() to return when a key is looked up that
    // is not in the hash table.
    QRIntrusiveSharedPtr<V> nullSharedPtrVal;
  }; // class SharedPtrValueHash
  #undef super
#endif // #ifdef _MEMSHAREDPTR

/**
 * SharedPtrValueHashIterator: a NAHashDictionaryIterator for use with Shared Pointers.
 */
#ifndef _MEMSHAREDPTR
  #define SharedPtrValueHashIterator  NAHashDictionaryIterator
#else
  #define super NAHashDictionaryIterator<K, QRIntrusiveSharedPtr<V> >
  template <class K, class V>
  class SharedPtrValueHashIterator : public super
  {
  public:
    /**
    * Constructor when both key and value are supplied.
    * @param dict SharedPtrValueHash object to iterate on  
    * @param key Initial key value
    * @param value Initial value value
    */
    SharedPtrValueHashIterator (const SharedPtrValueHash<K,V> & dict, 
				const K* key,
				const QRIntrusiveSharedPtr<V> value)
    : super(dict, key, &value)
    {}

    /**
    * Constructor when no value is given.
    * @param dict SharedPtrValueHash object to iterate on  
    * @param key Initial key value
    */
    SharedPtrValueHashIterator (const SharedPtrValueHash<K,V> & dict, 
				const K* key = NULL)
    : super(dict, key)
    {}

    /**
    * Copy ctor
    * @param other Object to copy
    * @param heap Heap pointer
    */
    SharedPtrValueHashIterator (const SharedPtrValueHashIterator<K,V> & other,
				CollHeap * heap=0)
    : super(other, heap)
    {}

    /**
    * Destructor
    */
    virtual ~SharedPtrValueHashIterator() {}

    /**
    * Get the next (key, value) pair, and dereference the value pointer
    * on the way out.
    * @param key /out
    * @param value /out
    */
    void getNext(K*& key, QRIntrusiveSharedPtr<V>& value)
    {
      QRIntrusiveSharedPtr<V>* vp;
      super::getNext(key, vp);
      value = *vp;
    }
  }; // class SharedPtrValueHashIterator
  #undef super
#endif // #ifdef _MEMSHAREDPTR

#if 0
// Not used for now...
// Using a shared pointer as the key object almost works....
// The problem is that the NAHashDictionary template uses the key object's 
// operator== for comparisons of key objects. In the shared pointer version,
// this translates into comparing the internal pointer values, which works 
// only sometimes...
// I also tried defining operator== for the QRIntrusiveSharedPtr<> template,
// but this has two problems: first, it requires ALL the classes that inherit 
// from NAIntrusiveSharedPtrObject to implement operator==, and second, sometimes
// using just pointer comparison is the needed semantics.

/**
 * SharedPtrHash: a NAHashDictionary for use with Shared Pointers.
 * Both value and key classes are expected to be derived from NAIntrusiveSharedPtrObject.
 * This class attempts to solve the problem that a NAHashDictionary<K,V> is 
 * actually handling pointers to K and V, and don't know how to correctly 
 * handle shared pointers. Therefore, this class actually stores pointers to 
 * shared pointer objects, and handles the dereferencing.
 */
#ifndef _MEMSHAREDPTR
  #define SharedPtrHash  NAHashDictionary
#else
  #define super NAHashDictionary<QRIntrusiveSharedPtr<K>, QRIntrusiveSharedPtr<V> >

  template <class K, class V>
  class SharedPtrHash : public super
  {
  public:
    /**
    * Constructor.
    * @param hashFunction Pointer to a hash function
    * @param initialHashSize Initial size of hash table.
    * @param enforceUniqueness When FALSE rejects multiple values for the same key
    * @param heap The heap pointer
    */
    SharedPtrHash(ULng32(*hashFunction)(const QRIntrusiveSharedPtr<K>&), 
		  ULng32 initialHashSize = NAHashDictionary_Default_Size,
		  NABoolean enforceUniqueness = FALSE,
		  CollHeap * heap=0)
    // Override constructor to save the heap pointer.
    :  super(hashFunction, initialHashSize, enforceUniqueness, heap)
      ,heap_(heap)
      ,nullSharedPtrVal(NULL)
      {}

    /**
    * Copy Constructor
    * @param other The object to copy
    * @param heap A Heap pointer
    */
    SharedPtrHash(const super& other, CollHeap * heap=0)
      : super(other, heap)
      ,heap_(heap)
      ,nullSharedPtrVal(NULL)
    {}

    /**
    * Destructor
    */
    virtual ~SharedPtrHash()
    {
      clear(FALSE);
    }

    /**
    * Check whether this dictionary contains a certain key, or a 
    * certain key value pair. 
    * @param key 
    * @param value 
    * @return 
    */
    inline NABoolean contains(const QRIntrusiveSharedPtr<K> key, 
			      const QRIntrusiveSharedPtr<V> value = NULL) const
    { 
      const QRIntrusiveSharedPtr<K>* kp = &key;

      if (value)
      {
	const QRIntrusiveSharedPtr<V>* vp = &value;
	return super::contains(kp, vp);
      }
      else
	return super::contains(kp);
    }

    /**
    * Insert a (key, value) pair. Allocate a pointer to the shared pointer 
    * to the key and value objects, and insert the pair:
    * (QRIntrusiveSharedPtr<value>, QRIntrusiveSharedPtr<value>) into the hash table.
    * @param key Pointer to key object.
    * @param value Shared pointer to value object.
    * @return The pointer to the key object.
    */
    QRIntrusiveSharedPtr<K> insert(QRIntrusiveSharedPtr<K> key, QRIntrusiveSharedPtr<V> value)
    {
      QRIntrusiveSharedPtr<K>* kp = new(heap_) QRIntrusiveSharedPtr<K>;
      *kp = key;
      QRIntrusiveSharedPtr<V>* vp = new(heap_) QRIntrusiveSharedPtr<V>;
      *vp = value;
      QRIntrusiveSharedPtr<K>* result = super::insert(kp, vp);
      return *result;
    }

    /**
    * Get the first value corresponding to the input key. Dereference the
    * value on the way out.
    * @param key Pointer to key value.
    * @return Shared pointer to value.
    */
    QRIntrusiveSharedPtr<V> getFirstValue(const QRIntrusiveSharedPtr<K> key) const
    {
      const QRIntrusiveSharedPtr<K>* kp = &key;
      // Avoid dereferencing null value key is not in the hash table.
      QRIntrusiveSharedPtr<V>* firstValPtr = super::getFirstValue(kp);
      if (firstValPtr)
        return *firstValPtr;
      else
        return nullSharedPtrVal;
    }

    /**
    * Remove the (key, value) pair, and delete the pointers to the 
    * shared pointers.
    * @param key 
    * @return 
    */
    QRIntrusiveSharedPtr<K> remove(QRIntrusiveSharedPtr<K> key)
    {
      QRIntrusiveSharedPtr<K>* kp = &key;
      QRIntrusiveSharedPtr<V>* vp = super::getFirstValue(kp);
      if (vp == NULL)
	return nullSharedPtrVal;

      QRIntrusiveSharedPtr<K>* resultp = super::remove(key);
      // result cannot be NULL here because getFirstValue() just returned a value.
      QRIntrusiveSharedPtr<K> result = *resultp

      NADELETEBASIC(result, heap_);
      NADELETEBASIC(vp, heap_);

      return result;
    }

    /**
    * Clear the contents of the hash table, and delete memory used for 
    * internal pointers.
    * @param deleteContents 
    */
    void clear(NABoolean deleteContents = FALSE)
    {
      // Iterate on the hash table.
      QRIntrusiveSharedPtr<V>* vp;
      QRIntrusiveSharedPtr<K>* kp; 
      NAHashDictionaryIterator<QRIntrusiveSharedPtr<K>, QRIntrusiveSharedPtr<V> > iter(*this);
      for ( CollIndex i = 0 ; i < iter.entries() ; i++) 
      {
	// For each (key, value) pair
	iter.getNext (kp, vp); 
	// Delete the shared pointer if requested to.
	if (deleteContents)
	{
	  QRIntrusiveSharedPtr<K> k = *kp;
	  deletePtr(k);
	  QRIntrusiveSharedPtr<V> v = *vp;
	  deletePtr(v);
	}
	// Delete the pointer to the shared pointer.
	NADELETEBASIC(kp, heap_);
	NADELETEBASIC(vp, heap_);
      }
      // Now, really clear the contents.
      super::clear(FALSE);
    }

  private:
    // Unfortunatly, the heap_ member in NAHashDictionary is private, not protected.
    CollHeap* heap_;

    // This is a value for getFirstValue() to return when a key is looked up that
    // is not in the hash table.
    QRIntrusiveSharedPtr<V> nullSharedPtrVal;
  }; // class SharedPtrHash
  #undef super
#endif // #ifdef _MEMSHAREDPTR

/**
 * SharedPtrHashIterator: a NAHashDictionaryIterator for use with Shared Pointers.
 */
#ifndef _MEMSHAREDPTR
  #define SharedPtrHashIterator  NAHashDictionaryIterator
#else
  #define super NAHashDictionaryIterator<QRIntrusiveSharedPtr<K>, QRIntrusiveSharedPtr<V> >
  template <class K, class V>
  class SharedPtrHashIterator : public super
  {
  public:
    /**
    * Constructor when both key and value are supplied.
    * @param dict SharedPtrHash object to iterate on  
    * @param key Initial key value
    * @param value Initial value value
    */
    SharedPtrHashIterator (const SharedPtrHash<K,V> & dict, 
			   const QRIntrusiveSharedPtr<K> key,
			   const QRIntrusiveSharedPtr<V> value)
    : super(dict, (key == NULL ? NULL : &key), &value)
    {}

    /**
    * Constructor when no value is given.
    * @param dict SharedPtrHash object to iterate on  
    * @param key Initial key value
    */
    SharedPtrHashIterator (const SharedPtrHash<K,V> & dict, 
			   const QRIntrusiveSharedPtr<K> key = NULL)
    : super(dict, (key == NULL ? NULL : &key))
    {}

    /**
    * Copy ctor
    * @param other Object to copy
    * @param heap Heap pointer
    */
    SharedPtrHashIterator (const SharedPtrHashIterator<K,V> & other,
			   CollHeap * heap=0)
    : super(other, heap)
    {}

    /**
    * Destructor
    */
    virtual ~SharedPtrHashIterator() {}

    /**
    * Get the next (key, value) pair, and dereference the value pointer
    * on the way out.
    * @param key /out
    * @param value /out
    */
    void getNext(QRIntrusiveSharedPtr<K>& key, QRIntrusiveSharedPtr<V>& value)
    {
      QRIntrusiveSharedPtr<K>* kp;
      QRIntrusiveSharedPtr<V>* vp;
      super::getNext(kp, vp);
      key   = *kp;
      value = *vp;
    }
  }; // class SharedPtrHashIterator
  #undef super
#endif // #ifdef _MEMSHAREDPTR

/**
 * SharedPtrKeyHash: a NAHashDictionary for use with Shared Pointers.
 * The key class must be a NAIntrusiveSharedPtrObject derivative that supports
 * the hash() method. There is no value class, since this data structure is 
 * used for checking for the existance of keys within the hash, rather than 
 * finding the actual value.
 * This class attempts to solve the problem that a NAHashDictionary<K,V> is 
 * actually handling pointers to K and V, and don't know how to correctly 
 * handle shared pointers. Therefore, this class actually stores pointers to 
 * shared pointer objects, and handles the dereferencing.
 */
#define super NAHashDictionary<QRIntrusiveSharedPtr<K>, QRIntrusiveSharedPtr<K> >
template <class K>
class SharedPtrKeyHash : public super
{
public:
  /**
   * Constructor.
   * @param hashFunction Pointer to a hash function
   * @param initialHashSize Initial size of hash table.
   * @param enforceUniqueness When FALSE rejects multiple values for the same key
   * @param heap The heap pointer
   */
  SharedPtrKeyHash(ULng32(*hashFunction)(const K&), 
		ULng32 initialHashSize = NAHashDictionary_Default_Size,
		NABoolean enforceUniqueness = FALSE,
		CollHeap * heap=0)
  // Override constructor to save the heap pointer.
  :  super(hashFunction, initialHashSize, enforceUniqueness, heap)
    ,heap_(heap)
    {}

  /**
   * Copy Constructor
   * @param other The object to copy
   * @param heap A Heap pointer
   */
  SharedPtrKeyHash(const super& other, CollHeap * heap=0)
    : super(other, heap)
     ,heap_(heap)
  {}

  /**
   * Destructor
   */
  virtual ~SharedPtrKeyHash()
  {
    clear(FALSE);
  }

  /**
   * Check whether this dictionary contains a certain key.
   * @param key 
   * @return 
   */
  inline NABoolean contains(const QRIntrusiveSharedPtr<K> key) const
  { 
    const QRIntrusiveSharedPtr<K>* kp = &key;
    return super::contains(kp);
  }

  /**
   * Insert a key. Allocate a pointer to the shared pointer 
   * to the key object, and insert the QRIntrusiveSharedPtr<key>
   * pair into the hash table.
   * @param key Pointer to key object.
   * @param value Shared pointer to value object.
   * @return The pointer to the key object.
   */
  QRIntrusiveSharedPtr<K> insert(QRIntrusiveSharedPtr<K> key)
  {
    QRIntrusiveSharedPtr<K>* kp = new(heap_) QRIntrusiveSharedPtr<K>;
    *kp = key;
    return super::insert(kp, kp);
  }

  /**
   * Get the first value corresponding to the input key. 
   * @param key Pointer to key value.
   * @return Shared pointer to value.
   */
  QRIntrusiveSharedPtr<K> getFirstValue(const QRIntrusiveSharedPtr<K> key) const
  {
    return *(super::getFirstValue(key));
  }

  /**
   * Remove the key, and delete the pointer to the shared pointer.
   * @param key 
   * @return 
   */
  QRIntrusiveSharedPtr<K> remove(QRIntrusiveSharedPtr<K> key)
  {
    QRIntrusiveSharedPtr<K> result = NULL;

    QRIntrusiveSharedPtr<K>* kp = super::remove(key);
    if (kp != NULL)
    {
      result = *kp;
      NADELETEBASIC(kp, heap_);
    }
    return result;
  }

  /**
   * Clear the contents of the hash table, and delete memory used for 
   * internal pointers.
   * @param deleteContents 
   */
  void clear(NABoolean deleteContents = FALSE)
  {
    // Iterate on the hash table.
    QRIntrusiveSharedPtr<K>* kp;
    QRIntrusiveSharedPtr<V>* vp;
    NAHashDictionaryIterator< QRIntrusiveSharedPtr<K>, QRIntrusiveSharedPtr<K> > iter(*this);
    for ( CollIndex i = 0 ; i < iter.entries() ; i++) 
    {
      // For each (key, value) pair
      iter.getNext (kp, vp); 
      assert(*kp == *vp);

      // Delete the shared pointer if requested to.
      if (deleteContents)
      {
        QRIntrusiveSharedPtr<V> v = *vp;
	deletePtr(v);
      }
      // Delete the pointer to the shared pointer.
      NADELETEBASIC(vp, heap_);
    }
    // Now, really clear the contents.
    super::clear(FALSE);
  }

private:
  // Unfortunatly, the heap_ member in NAHashDictionary is private, not protected.
  CollHeap* heap_;
}; // class SharedPtrKeyHash
#undef super

/**
 * SharedPtrKeyHashIterator: a NAHashDictionaryIterator for use with Shared Pointers.
 */
#define super NAHashDictionaryIterator<QRIntrusiveSharedPtr<K>, QRIntrusiveSharedPtr<K> >
template <class K>
class SharedPtrKeyHashIterator : public super
{
public:
  /**
   * Constructor
   * @param dict SharedPtrKeyHash object to iterate on  
   * @param key Initial key value
   * @param value Initial value value
   */
  SharedPtrKeyHashIterator (const SharedPtrKeyHash<K> & dict, 
                         const QRIntrusiveSharedPtr<K> key = NULL)
  : super(dict, &key)
  {}

  /**
   * Copy ctor
   * @param other Object to copy
   * @param heap Heap pointer
   */
  SharedPtrKeyHashIterator (const SharedPtrKeyHashIterator<K> & other,
                         CollHeap * heap=0)
  : super(other, heap)
  {}

  /**
   * Destructor
   */
  virtual ~SharedPtrKeyHashIterator() {}

  /**
   * Get the next (key, value) pair, and dereference the value pointer
   * on the way out.
   * @param key /out
   */
  void getNext(QRIntrusiveSharedPtr<K>& key, QRIntrusiveSharedPtr<K>& value)
  { assert(FALSE); }

  void getNext(QRIntrusiveSharedPtr<K>& key)
  {
    QRIntrusiveSharedPtr<K>* kp = &key;
    QRIntrusiveSharedPtr<K>* vp;
    super::getNext(kp, vp);
    key = *kp;
  }
}; // class SharedPtrKeyHashIterator
#undef super

// Not used for now.
#endif

class StringPtrSet : public NABasicObject
{
public:
  StringPtrSet(CollHeap* heap)
    : theList_(heap)
  { }

  CollIndex entries() const 
  {
    return theList_.entries();
  }

  const NAString* operator[](CollIndex i) const
  {
    return theList_[i];
  }

  NABoolean contains(const NAString* str)
  {
    CollIndex maxEntries = theList_.entries();
    for (CollIndex i=0; i<maxEntries; i++)
    {
      if (*theList_[i] == *str)
        return TRUE;
    }

    return FALSE;
  }

  void insert(const NAString* str)
  {
    // Check if duplicate
    if (contains(str))
      return;

    // Do the insert.
    theList_.insert(str);
  }

  void insert(const StringPtrSet& other)
  {
    CollIndex maxEntries = other.entries();
    for (CollIndex i=0; i<maxEntries; i++)
      insert(other[i]);
  }

  void remove(const NAString* str)
  {
    CollIndex maxEntries = theList_.entries();
    for (CollIndex i=0; i<maxEntries; i++)
      if (*theList_[i] == *str)
      {
        theList_.removeAt(i);
        return;
      }
  }

private:
  NAList<const NAString*>   theList_;
};  // class StringPtrSet


#endif  /* _QRSHAREDPTR_H_ */
