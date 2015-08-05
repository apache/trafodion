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
#ifndef TDBDLGEXPRLIST_H
#define TDBDLGEXPRLIST_H

#include <QtGui>

#include "CommonSqlCmpDbg.h"

namespace Ui
{
  class TDBDlgExprList;
}

class TDBDlgExprList : public QDialog
{
  Q_OBJECT
public:
  explicit TDBDlgExprList(QWidget * parent = 0, ComTdb* currTdb = 0, const NAString & tdbNodeName = NAString());
  ~TDBDlgExprList();
  void DisplayExprList();
  NAString ExprNodeTypeToString(ex_expr::exp_node_type nodeType);
  NAString ExprNodeFlagsToString(ex_expr* exprNode);
private slots:
  void on_btnOK_clicked();

private:
  Ui::TDBDlgExprList* ui;
  ComTdb* m_tdb;
  NAString m_tdbNodeName;
};
#endif
