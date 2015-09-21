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
********************************************************************/
// DSList.cpp: implementation of the CDSList class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DSList.h"
#include <string.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
const int CDSList::NAME = 0;
const int CDSList::VALUE = 1;

CDSList::CDSList():
m_iIndex(0), m_pNext(NULL), m_pPrev(NULL),
m_pName(NULL), m_pValue(NULL), m_pAttribs(NULL)
{
}

CDSList::CDSList(CDSList *pParent):
m_iIndex(pParent->getIndex() + 1), m_pNext(NULL), m_pPrev(pParent),
m_pName(NULL), m_pValue(NULL), m_pAttribs(NULL)
{
}

CDSList::~CDSList()
{
	// Don't delete the previous object since its destructor is
	// destroying *this* object through its m_pNext member.
	delete [] m_pName;
	delete [] m_pValue;
	delete m_pNext;
	delete m_pAttribs;
}

bool CDSList::append(const char* name, const char* value /* = NULL */)
{
	// Check to make sure that this name is unique to the list. Names
	// are always added to the "bottom" of the list. Since this function
	// is be called recursivly, turn the search option off to save on
	// repetitive searches.
	if (name == NULL || findName(name, false) >= 0)
		return false;

	// Append only if a value of name hasn't already been assigned to it.
	if (m_pName == NULL)
	{
		// set the name
		int length = strlen(name) + 1;
		m_pName = new char[length];
		memset(m_pName, 0, length);
		strcpy(m_pName, name);

		// set the optional value
		if (value != NULL)
		{
			length = strlen(value) + 1;
			m_pValue = new char[length];
			memset(m_pValue, 0, length);
			strcpy(m_pValue, value);
		}

		return true;
	}
	else
	{
		if (m_pNext == NULL)
			m_pNext = new CDSList(this);
		return m_pNext->append(name, value);
	}

}

int CDSList::getIndex()
{
	return m_iIndex;
}

int CDSList::findName(const char* name, bool Search /* = true */)
{
	// If there is no name, then there will be no children, so don't
	// bother checking.
	if (name == NULL || m_pName == NULL)
		return -1;

	// If we don't have the name then traverse the list if the search
	// option is set.
	if (strcmp(m_pName, name) == 0)
		return m_iIndex;
	else if (m_pNext != NULL && Search)
		return m_pNext->findName(name);
	else
		return -1;
}

int CDSList::getCount()
{
	CDSList* pIndex = this;
	int count = 0;

	while (pIndex != NULL)
	{
		// Only objects with names count.
		if (pIndex->m_pName != NULL)
			count++;
		pIndex = pIndex->m_pNext;
	}
	return count;
}

int CDSList::getAttribCount(const char* name)
{
	int index = findName(name);
	return getAttribCount(index);
}

int CDSList::getAttribCount(int index)
{
	if (m_iIndex == index)
	{
		if (m_pAttribs != NULL)
			return m_pAttribs->getCount();
		else
			return 0;
	}
	else
	{
		if (m_pNext != NULL)
			return m_pNext->getAttribCount(index);
		else
			return -1;
	}
}

bool CDSList::addAttrib(int index, const char* attrib,
					   const char* attrib_value /* = NULL */)
{
	// Verify that the index is valid.
	if (index < 0)
		return false;

	// Get the right CDSList object based on index
	if (m_iIndex == index)
	{
		// If necessary, create an attribute list before adding.
		if (m_pAttribs == NULL)
			m_pAttribs = new CDSList();
		return m_pAttribs->append(attrib, attrib_value);
	}
	else
	{
		if (m_pNext != NULL)
			return m_pNext->addAttrib(index, attrib, attrib_value);
		else
			return false;
	}

}

bool CDSList::addAttrib(const char* name, const char* attrib,
					   const char* attrib_value /* = NULL */)
{
	int index = findName(name);
	return addAttrib(index, attrib, attrib_value);
}

char* CDSList::getAttribValue(int index, const char* attrib)
{
	if (index < 0)
		return NULL;

	if (index == m_iIndex)
	{
		if (m_pAttribs != NULL)
		{
			int att_index = m_pAttribs->findName(attrib);
			return m_pAttribs->getAt(att_index, CDSList::VALUE);
		}
	}
	else
	{
		if (m_pNext != NULL)
			return m_pNext->getAttribValue(index, attrib);
	}
	

	return NULL;
}

char* CDSList::getAt(int index, int type /* = CDSList::NAME */)
{
	// Index values are zero or higher.
	if (index < 0)
		return NULL;

	if (index == m_iIndex)
	{
		char* copy = NULL;

		if (type == NAME && m_pName != NULL)
		{
			copy = new char[strlen(m_pName) + 1];
			strcpy(copy, m_pName);
		}
		else if (type == VALUE && m_pValue != NULL)
		{
			copy = new char[strlen(m_pValue) + 1];
			strcpy(copy, m_pValue);
		}

		return copy;
	}
	else
	{
		if (m_pNext != NULL)
			return m_pNext->getAt(index, type);
		else
			return NULL;
	}
}