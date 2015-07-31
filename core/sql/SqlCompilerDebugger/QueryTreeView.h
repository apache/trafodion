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
#ifndef QUERYTREEVIEW_H
#define QUERYTREEVIEW_H

#include <QtGui>

#include <iostream>
using namespace std;

#include "CommonSqlCmpDbg.h"

namespace Ui
{
  class QueryTreeView;
}

/*By inherit QObject,
 *we can automatically delete objects hierarchy. */
typedef struct tagDFTDetails : public QObject
{
  ExprNode *currNode;
  CascadesPlan *currPlan;
} DFTDETAILS;

Q_DECLARE_METATYPE (tagDFTDetails *);	// This is for QTreeWidgetItem.setData()

class QueryTreeView:public QWidget
{
Q_OBJECT public:
    explicit QueryTreeView (QWidget * parent = 0);
  void UpdateView ();
   ~QueryTreeView ();
private:
    Ui::QueryTreeView * ui;
  void Free (void);
  void DisplayQueryTree (void *tree, void *plan =
			 NULL, QTreeWidgetItem * parentTreeItem = NULL);
  void GetOperatorImageText (void *tree, Int32 & bitmapIndex,
			     NAString & nodeText);
  void FillGroupAttribs (void *tree, void *plan, QTreeWidgetItem * treeItem);
  void FillExprType (void *tree, void *plan, QTreeWidgetItem * treeItem);
  void FillRowCost (void *tree, void *plan, QTreeWidgetItem * treeItem);
  private slots:
    //Display ContextMenu
  void on_m_tree_customContextMenuRequested (const QPoint & pos);
};

#endif // QUERYTREEVIEW_H
