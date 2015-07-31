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
#ifndef TDBTREEVIEW_H
#define TDBTREEVIEW_H_H

#include <QtGui>

#include <iostream>
using namespace std;

#include "CommonSqlCmpDbg.h"

namespace Ui
{
  class TDBTreeView;
}

class TDBTreeView:public QWidget
{
Q_OBJECT
public:
  explicit TDBTreeView (QWidget * parent = 0);
  ~TDBTreeView ();
  NAString TDBNodeTypeToString(ComTdb::ex_node_type nodeType);
  void UpdateView();
  void FreeItemMemory(void);
  void DisplayTDBTree(const class ComTdb* curr,
                                    ExFragDir* fragDir,
                                    Lng32 baseAddr,
                                    QTreeWidgetItem * parentTreeItem);
private slots:
  void on_treeWidget_itemClicked ( QTreeWidgetItem * item, int column );
protected:
  void closeEvent (QCloseEvent *);
private:
    Ui::TDBTreeView * ui;
};

#endif // TDBTREEVIEW_H
