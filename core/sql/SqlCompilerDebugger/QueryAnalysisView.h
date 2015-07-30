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
#ifndef QUERYANALYSIS_H
#define QUERYANALYSIS_H

#include <QtGui>
#include "CommonSqlCmpDbg.h"

namespace Ui
{
  class QueryAnalysisView;
}

class QueryAnalysisView:public QWidget
{
Q_OBJECT
public:
  explicit QueryAnalysisView(QueryAnalysis* analysis, QWidget * parent = 0);
  ~QueryAnalysisView();
  void UpdateView();
protected:
  Int32 AddListItem(const NAString & s);
  void Free();
private:
  Ui::QueryAnalysisView* ui;
  QueryAnalysis* queryAnalysis_;
private slots:
  void on_btnOK_clicked();
  void on_btnCancel_clicked();
};

#endif // QUERYANALYSIS_H
