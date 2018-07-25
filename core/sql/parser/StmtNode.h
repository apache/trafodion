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
/* -*-C++-*-
******************************************************************************
*
* File:         StmtNode.h
* Description:  Stmt nodes implementation
* Created:      3/6/95
* Language:     C++
*
*
*
*
******************************************************************************
*/


#ifndef STMTNODE_H
#define STMTNODE_H

#include "ItemColRef.h"
#include "RelControl.h"
#include "SQLCLIdev.h"
#include "TableDesc.h"
#include "ValueDesc.h"
#include "mbdact.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtNode;

// -----------------------------------------------------------------------
// forward references
//
// - ShortStringSequence is defined in SqlParser.y, and not in this file.
//   However, by placing a forward reference here, then in other files,
//   such as sqlcomp's parser.C file, where they include Y.tab.h,
//   "ShortStringSequence" won't cause a problem for its being undefined.
// -----------------------------------------------------------------------
class ShortStringSequence;

// -----------------------------------------------------------------------
// Helpful Definitions
// -----------------------------------------------------------------------
typedef NASimpleArray<NAString*>                   NAStringList;

// -----------------------------------------------------------------------
// A generic statement node.
// -----------------------------------------------------------------------

class StmtNode : public ExprNode
{

public:

  // default constructor
  StmtNode(OperatorTypeEnum otype
          // QSTUFF
          , NABoolean holdable = FALSE
          // QSTUFF 
          ) : ExprNode(otype)
          // QSTUFF
          , holdable_(holdable)
          // QSTUFF
  { setNonCacheable(); };

  // virtual destructor
  virtual ~StmtNode() {};

  virtual StmtNode *castToStatementExpr();
  virtual const StmtNode *castToStatementExpr() const;

  // method required for traversing an ExprNode tree
  // access a child of an ExprNode 
  virtual ExprNode * getChild(Lng32 index);

  // Method for replacing a particular child
  virtual void setChild(Lng32 index, ExprNode *);

  NABoolean isAQueryStatement() const { return getOperatorType() == STM_QUERY; }
  virtual RelExpr * getQueryExpression() const;

  StaticOnly isAStaticOnlyStatement() const
  {
    OperatorTypeEnum op = getOperatorType();
    if (op == STM_MODULE || op == STM_TIMESTAMP)
      return STATIC_ONLY_WITH_WORK_FOR_PREPROCESSOR;
    RelExpr *re = getQueryExpression();
    while (re && re->getOperatorType() == REL_ROOT) 
      re = re->child(0);
    if (!re) return NOT_STATIC_ONLY;
    return re->isAStaticOnlyStatement();
  }

  ExprNode *getStaticOnlyStatement()
  {
    OperatorTypeEnum op = getOperatorType();
    if (op == STM_MODULE || op == STM_TIMESTAMP)
      return this;
    RelExpr *re = getQueryExpression();
    while (re && re->getOperatorType() == REL_ROOT) 
      re = re->child(0);
    if (!re) return NULL;
    return re->isAStaticOnlyStatement() ? re : NULL;
  }
 
  virtual Int32 getArity() const;

  // QSTUFF
  // is used to indicate whether a cursor is holdable or not. This is put
  // into the statement node to account for all three different types of
  // cursors which are all derived from StmtNode
  NABoolean isHoldable() const { return holdable_; };
  // QSTUFF

private:

  // QSTUFF
  NABoolean holdable_;
  // QSTUFF

};

class StmtQuery : public StmtNode
{

public:
  StmtQuery( RelExpr * aQueryTree ) : StmtNode(STM_QUERY),
                                      queryExpr_(aQueryTree)
  {
  }

  virtual RelExpr * getQueryExpression() const;
  
  // method required for traversing an ExprNode tree
  // access a child of an ExprNode 
  virtual ExprNode * getChild(Lng32 index);

  // Method for replacing a particular child
  virtual void setChild(Lng32 index, ExprNode *);

  virtual const NAString getText() const;

  virtual Int32 getArity() const;

  virtual void print(FILE * f,const char * prefix,const char *) const;

private:
  RelExpr * queryExpr_;
};


enum  { INPUT_DIRECTION, OUTPUT_DIRECTION };


/* Host Variable Expression Type Structure.                                 */
/*                                                                          */

struct HostVarExprType {
   NAType    *the_type;
   NABoolean isNullable;
};


/* class StaticDescItem
 * --------------------
 * Purpose: This class represents a variant record.  From the outside
 * it can contain either a pointer to a HostVarExprType, or a
 * ConstValue (but not both).  The client code can check which case
 * this variant record is.  If the client code "gets" a pointer then
 * the referenced data belongs to the client code.  This class's
 * destructor will delete allocated data if no "get" is ever performed.
 */

class StaticDescItem  : public NABasicObject
{
  public:
                  StaticDescItem         (ConstValue*);
                  StaticDescItem         (HostVarExprType*);
                 ~StaticDescItem         ();

 NABoolean        isDataTypePointer      () const;
 NABoolean        isLiteralPointer       () const;
 ConstValue      *getConstValuePtr       ();
 HostVarExprType *getDataTypePtr         ();


  private:
    ConstValue        *literalPointer;
    HostVarExprType   *dataTypePointer;
};

// three values, from an enumeration telling status of
// structure: no name, cursor name, or statement
// name.

enum StmtOrCursEnum { NAME_ABSENT, NAME_CURSOR, NAME_STATEMENT };

struct StmtOrCursTag {
   NAString         *theName;  
   StmtOrCursEnum   tag;
};


/* We need a vector type in order to supply a data structure for            */
/* host_var_type_list.  We use the Collections.h ARRAY type to do this.     */
/* Note that we use pointers to everything here.  This will mean            */
/* that we might want some clever way to delete everything without          */
/* leaking all over the place.                                              */

// Note that we have several instantiations of NASimpleArray where the
// actual implementation code should be identical, since we want
// the enums to be treated funamentally as long values.  

typedef NASimpleArray<StaticDescItem*>             DescTypeList;

// The following is provided as a derived class rather than
// as a typedef.  This is so that we may write a forward reference:
//
//    class SequenceOfLong;
//
// You can't do that with a typedef.

class SequenceOfLong : public NASimpleArray<Long*>
{
public:
  SequenceOfLong (CollHeap * h=0) : NASimpleArray<Long*>(h) {}

  // copy ctor
  SequenceOfLong (const SequenceOfLong & orig, CollHeap * h=0) :
       NASimpleArray<Long*>(orig,h) {}
};

//
// More Statement Nodes follow....
//

class StmtTimeStamp : public StmtNode
{
public:
   Int64     timeStampValue;

   StmtTimeStamp(Int64 new_value) : timeStampValue(new_value),
        StmtNode(STM_TIMESTAMP)
    {};
};

class StmtModule : public StmtNode
{
public:
   StmtModule(CollHeap *h=0) :
     StmtNode(STM_MODULE), name_(h), charSet_(h)
     {}

   StmtModule(const QualifiedName &mn, const NAString &cs, CollHeap *h) :
     StmtNode(STM_MODULE), name_(mn,h), charSet_(cs,h)
     { assert(!cs.isNull()); }

   StmtModule & operator=(const StmtModule &sm)
   {
     if (this != &sm) {
       name_    = sm.name_;
       charSet_ = sm.charSet_;
     }
     return *this;
   }

   QualifiedName &name()        { return name_; }
   NAString &charSet()          { return charSet_; }
   NABoolean isEmpty() const    { return  name_.getObjectName().isNull();  }
   NABoolean isFull() const     { return !name_.getCatalogName().isNull(); }

   NABoolean applyDefaults(NABoolean wantR18behavior);
   NABoolean unparse(NAString &result, NABoolean wantR18behavior);
   NABoolean unparseSimple(NAString &result, NABoolean wantR18behavior);
  
  void applyModuleCatalogSchema(const NAString& cat, const NAString& sch);

private:
   StmtModule(const StmtModule &sm, CollHeap *h);       // not written

   QualifiedName name_;
   NAString      charSet_;
};

class StmtSourceFile : public StmtNode
{
public:
  StmtSourceFile(NAString *src) : StmtNode(STM_SOURCE_FILE), pathname(src) {}

  NAString *pathname;
};


class StmtProcedure : public StmtNode
{
public:
   StmtProcedure(NAString *newName, 
                 StmtNode  *newBody) : StmtNode(STM_PROCEDURE),
      name(newName), bodyStatement(newBody)
     { };

   NAString     *name;
   StmtNode     *bodyStatement;
};


/* This is the thing with the DescTypeList bound to a string thus representing */
/* a descriptor, whether it is input_or_output, and its name.               */
/*                                                                          */
/* This file declares a million of these structures that are                */
/* aggregates: some data bound together along with a constructor for getting */
/* at it, and the client code gets to manage the dynamically allocated      */
/* data.                                                                    */
/*                                                                          */

struct NamePlusDesc {
   NamePlusDesc(NAString*, NABoolean, DescTypeList*);

   NAString          *descriptorName;
   NABoolean          isInput;
   DescTypeList      *descriptorArray;
}; 


/* The AllocStaticDesc statement node must hold members for the following   */
/* items: the NamePlusDescriptor, the entity_tag, and the entity_name.      */
/*                                                                          */
/* Note that if entityTag == NAME_ABSENT then entityName is NULL.           */
/*                                                                          */
/* Also note that the client is responsible for managing the storage        */
/* of the member data.                                                      */
/*                                                                          */

class StmtAllocStaticDesc : public StmtNode 
{
public:
   NamePlusDesc     *mainDescriptor;
   StmtOrCursEnum    entityTag;
   NAString         *entityName;

   StmtAllocStaticDesc( NamePlusDesc*, StmtOrCursEnum, NAString*);
   ~StmtAllocStaticDesc() {};
   /* we'll probably want the get() methods as well in here */
};


/* Now for the declaration of some statment classes to represent            */
/* cursor declarations.                                                     */
/*                                                                          */

class StmtDeclStatCurs : public StmtNode
{
public:
   StmtDeclStatCurs(NAString*,RelExpr*
                    // QSTUFF
                    ,NABoolean holdable = FALSE
                    // QSTUFF
                    );

   NAString   *cursorName;
   RelExpr    *cursorSpec;
};

class StmtDeclDynCurs : public StmtNode
{
public:
   StmtDeclDynCurs(NAString*, NAString*
                   // QSTUFF
                   , NABoolean holdable = FALSE
                   // QSTUFF
                   );

   NAString   *cursorName;
   NAString   *stmtName;
};

/* This one is easy, rather like BEGIN/END DECLARE SECTION: just put        */
/* an enumeration into a Statement Node in order to identify what           */
/* kind of statement it is.                                                 */
/*                                                                          */
class StmtXdynCurs : public StmtNode
{
public:
   StmtXdynCurs(
           // QSTUFF
           NABoolean holdable = FALSE
           // QSTUFF
           ) : StmtNode(STM_DECL_XDYNCURS
           // QSTUFF
           , holdable
           // QSTUFF
           )  { };
};

/* Here's what we do then for entity_name, extended_input_designation,      */
/* and open_cursor as well.  For entity_name, the data is simply a pointer  */
/* to a string (NAString, I DO NOT mean char*!).  If the pointer is NULL,   */
/* that means there is no string and that a host variable gives the name.   */
/* If the pointer is NOT NULL then the string it points to is the literal   */
/* name of the entity.                                                      */
/*                            
 * DT_NULL -- no input designation specified at all                                 
 * DT_DESC_VIA_HV -- a host variable names a descriptor, so descName is NULL.
 * DT_HVLIST -- there is a host variable list, so descName is NULL.
 * DT_DESCRIPTOR -- the literal name of the descriptor is held by an NAString
 *                  pointed to by descName.
 */


enum designator_tag { DT_NULL, DT_DESC_VIA_HV, DT_HVLIST, DT_DESCRIPTOR};

class designation : public NABasicObject
{
public:
   designation(designator_tag);  // where the new tag != DT_DESCRIPTOR
   designation(NAString*);       // for setting theTag to DT_DESCRIPTOR
   designation();
   
   NAString           *descName;
   designator_tag     theTag;
};


/* The data structure associated with the open_cursor non-terminal          */
/* is a structure with two members: 1) the cursor name, 2) the input        */
/* designation.  The cursor name is a pointer to a NAString.  If the        */
/* pointer is null, then a host variable names the cursor.  Else, the       */
/* pointer points to an NAString whose value is the cursor name.            */
/* The input designation is a flag and a similar pointer.  The flag         */
/* tells if any using clause was present.  If not, the pointer tells        */
/* if/how a descriptor is named (literally, or via a host variable).        */
/*                                                                          */


class StmtOpen : public StmtNode
{
public:
  StmtOpen(NAString* cn, designation* d=NULL) : StmtNode(STM_OPEN),
           theCursorName(cn), inputDesignation(d)
 { };

  NAString      *theCursorName;
  designation   *inputDesignation;
};

/* For StmtFetch, again, we copy/paste/fix-up based on StmtOpen.            */
/*                                                                          */


class StmtFetch : public StmtNode
{
public:
  StmtFetch(NAString* cn, designation* d) :  StmtNode(STM_FETCH),
           theCursorName(cn), outputDesignation(d)
 { };

  NAString      *theCursorName;
  designation   *outputDesignation;
};

/* The StmtNode for StmtPrepare is pretty simple.  It merely needs          */
/* to record the statement name, as entity (either literal, or              */
/* via descriptor, meaning that a string host variable names it).           */
/*                                                                          */


class StmtPrepare : public StmtNode
{
public:
   StmtPrepare(NAString   *stmt_name) : StmtNode(STM_PREPARE),
     theStatementName(stmt_name)
    {  };

   NAString       *theStatementName;
};

/* For StmtExecute, again, we copy/paste/fix-up based on StmtOpen.          */
/*                                                                          */


class StmtExecute : public StmtNode
{
public:
  StmtExecute(NAString* sn, designation* d=NULL, designation* d2=NULL) 
     : StmtNode(STM_EXECUTE), theStatementName(sn), inputDesignation(d),
       outputDesignation(d2)
 { };

  NAString      *theStatementName;
  designation   *inputDesignation;
  designation   *outputDesignation;
};


/* We need a definition for StmtWhenever.                                   */
/*                                                                          */


class StmtWhenever : public StmtNode
{
public:
   StmtWhenever(MBD_CONDITION  con,  action*  ap) :  StmtNode(STM_WHENEVER),
      theCondition(con), theAction(ap)
   { };

   MBD_CONDITION    theCondition;
   action           *theAction;
};


class StmtXactCtl : public StmtNode 
{
public:
   StmtXactCtl(SQLTRANS_COMMAND   nc) :  
     StmtNode(STM_XACT_CTL),theCommand(nc)
   { }; 

   SQLTRANS_COMMAND   theCommand;
};


class StmtDescribe : public StmtNode
{
public:
   StmtDescribe(UInt32 dir, NAString *snp, NAString *dnp) :
     StmtNode(STM_DESCRIBE), 
     direction(dir), statementName(snp), descriptorName(dnp)
   { };

   NAString   *statementName;
   NAString   *descriptorName;
   UInt32   direction;
};

class StmtDeallocStm : public StmtNode
{
public:
   StmtDeallocStm(NAString* theStatementName) :
     StmtNode(STM_DEALLOC_STM), statementName(theStatementName)  { };

   NAString *statementName;
};

class StmtDeallocDesc : public StmtNode
{
public:
   StmtDeallocDesc(NAString* theDescriptorName) :
     StmtNode(STM_DEALLOC_DESC), descriptorName(theDescriptorName)  { };

   NAString *descriptorName;
};

class StmtClose : public StmtNode {
public:
  StmtClose(NAString *theCursorName) :
     StmtNode(STM_CLOSE), cursorName(theCursorName) { };

  NAString*  cursorName;
};


class StmtStmtDiag : public StmtNode {
public:
   StmtStmtDiag(SequenceOfLong *inputList) :
          StmtNode(STM_STMT_DIAGS), theList(inputList)
      {assert(inputList !=NULL);}
   ~StmtStmtDiag()
      { delete theList; }

   SequenceOfLong* getListPointer()
     {
        SequenceOfLong  *result = theList;
        assert(result!=NULL);
        theList = NULL;
        return result;
     }

private:
    SequenceOfLong  *theList;
};

class StmtCondDiag : public StmtNode {
public:
   StmtCondDiag(SequenceOfLong *inputList) :
          StmtNode(STM_COND_DIAGS), theList(inputList)
      {assert(inputList !=NULL);}
   ~StmtCondDiag()
      { delete theList; }

   SequenceOfLong* getListPointer()
     {
        SequenceOfLong  *result = theList;
        assert(result!=NULL);
        theList = NULL;
        return result;
     }

private:
    SequenceOfLong  *theList;
};

class StmtDescBase : public StmtNode {
public:
   StmtDescBase(NAString* theDescriptorName,OperatorTypeEnum otype) : 
        StmtNode(otype), descriptorName(theDescriptorName) { }
   ~StmtDescBase();  
    NAString  *theDescriptorLiteral() const; 

protected:
   NAString  *descriptorName;
};

inline  NAString *StmtDescBase::theDescriptorLiteral() const
{
  return descriptorName;
}


// we need:
// get descriptor count -- the descriptor just needs to be specified
//         in the StmtNode.  This is just like for ALLOC and DEALLOC.
//         I don't know how much sense it would make, but, you could
//         have a template for these things, which takes a parameter of
//         the oper-type-enum involved. 

class StmtGetDescCount : public StmtDescBase {
public:
   StmtGetDescCount(NAString* theDescriptorName) :
      StmtDescBase(theDescriptorName,STM_GET_DESCCOUNT) { }
private:

};

class StmtGetRowsetSize : public StmtDescBase {
public:
   StmtGetRowsetSize(NAString* theDescriptorName) :
      StmtDescBase(theDescriptorName,STM_GET_ROWSETSIZE) { }
private:

};
// get descriptor item -- all we need is the list of the enum stuff, and
//         we need the name of the descriptor.  All the literal and
//         hostvar stuff gets taken care of by other means.

class StmtGetDescItem : public StmtDescBase {
public:
   StmtGetDescItem(NAString* theDescriptorName, SequenceOfLong* theItemList) :
      theItems(theItemList),
      StmtDescBase(theDescriptorName,STM_GET_DESCITEM) { }
   SequenceOfLong   *getItemsPtr() const  
     {
       return theItems;
     }
private:
   SequenceOfLong   *theItems;
};


// set descriptor item -- only need the descriptor name, and the list
//         of items, much like for "get descriptor."  The interesting
//         thing here is: don't check for host var types until AFTER
//         we know what statement it is (change in the preprocessor),
//         since ANY type is allowed for the DATA item and only the
//         runtime can type check that one; second thing is that
//         there needs to be some clever way of routing so that
//         the condition number gets routed separately from the
//         other simple value specs.

// This class is virtually identical to the one for "get desc items."
// This similarity might be used to factor the code somehow.

class StmtSetDescItem : public StmtDescBase {
public:
   StmtSetDescItem(NAString* theDescriptorName, SequenceOfLong*theItemList) :
      theItems(theItemList),
      StmtDescBase(theDescriptorName,STM_SET_DESCITEM) { }
   SequenceOfLong   *getItemsPtr() const  
     {
       return theItems;
     }
private:
   SequenceOfLong   *theItems;
};



// set descriptor count -- like many other statements, merely needs the
//         name of the descriptor.

class StmtSetDescCount : public StmtDescBase {
public:
   StmtSetDescCount(NAString* theDescriptorName) :
      StmtDescBase(theDescriptorName,STM_SET_DESCCOUNT) { }
private:

};

class StmtSetRowsetSize : public StmtDescBase {
public:
   StmtSetRowsetSize(NAString* theDescriptorName) :
      StmtDescBase(theDescriptorName,STM_SET_ROWSETSIZE) { }
private:

};

// alloc descriptor -- all the parser has to do is to route the literal,
//       if there is one, to the StmtNode::NAStringList, and do the
//       host var role for the literal or the actual host var.  This is
//       for the simple value spec. 
//       And for the descriptor (which is either a literal or given by
//       a simple host var) just the usual string for the designation,
//       and possible routing that is needed in case of host variable.

class StmtAllocDesc : public StmtDescBase {
public:
   StmtAllocDesc(NAString* theDescriptorName) :
      StmtDescBase(theDescriptorName,STM_ALLOC_DESC) { }
private:

};

class StmtTransaction : public StmtNode {
public:
   StmtTransaction( SequenceOfLong *theCmd ) : 
      StmtNode( STM_TRANSACTION ), theCommand_( theCmd ) 
         { assert( theCmd != NULL ); }
   ~StmtTransaction()
      { delete theCommand_; }
 
   SequenceOfLong* getXactCommand() 
      {
         SequenceOfLong *result = theCommand_;
         assert( result != NULL );
         theCommand_ = NULL;
         return result;
      }
 
private:
   SequenceOfLong *theCommand_;
};

#endif /* STMTNODE_H */
