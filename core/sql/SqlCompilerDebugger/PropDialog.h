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
#ifndef PROPDIALOG_H
#define PROPDIALOG_H

#include <QtGui>

#include "CommonSqlCmpDbg.h"

namespace Ui
{
  class PropDialog;
}

typedef struct tagPropDetails
{
  NAString propText;
} PROPDETAILS;

class PropDialog:public QDialog
{
  Q_OBJECT
enum PropertyTypes
  { CHARIP, CHAROP, CHARESSOP, CONSTRAINTS, COST,
    LOGICAL, PHYSICAL,
    CONTEXT, ASM, GROUPANALYSIS, TABLEANALYSIS, JBBC_PT, QGRAPH,
    CASCADESTRACEINFO
  };
public:
    explicit PropDialog (CascadesPlan * p, ExprNode * e, NAString title,
			 QWidget * parent = 0);
   ~PropDialog ();
  void UpdatePropList ();
  void displayEstLogProp (EstLogPropSharedPtr estLogProp);
  Int32 AddListItem (const NAString & s, PropertyTypes t);
  void FreePropMemory ();
  void setLabel (QString & lbl);
  void FreezeDisplay();
private:
  // helper methods for displaying cost in dlgprop window:
  void addSimpleCostVectorDetail (const SimpleCostVector & scv,
				  const Cost * cost = NULL);
  void addSimpleCostVector (const char *header, const SimpleCostVector & scv);
  void addCost (const NAString & header, const Cost * cost);
  void AddSeparatorLine(const QString &, Qt::GlobalColor color = Qt::white);
  QIcon propIcon (PropertyTypes t);

  Ui::PropDialog * ui;
  CascadesPlan *m_plan;
  ExprNode *m_expr;
  NAString m_propertiesListText;

  NABoolean m_displayInputs;
  NABoolean m_displayOutputs;
  NABoolean m_displayEssOutputs;
  NABoolean m_displayConstraints;
  NABoolean m_displayCost;
  NABoolean m_displayLogical;
  NABoolean m_displayPhysical;
  NABoolean m_displayContext;
  NABoolean m_displayASM;
  NABoolean m_displayGroupAnalysis;
  NABoolean m_displayJBBC;
  NABoolean m_displayTableAnalysis;
  NABoolean m_displayQueryGraph;
  NABoolean m_displayCascadesTraceInfo;
  NABoolean m_displayWizDirect;

private slots:

  void on_m_ClrAll_clicked ();
  void on_m_ShowAll_clicked ();
  void on_m_OK_clicked ();

  void on_m_CharInputs_toggled ();
  void on_m_CharOutputs_toggled ();
  void on_m_EssOutputs_toggled ();
  void on_m_Constraints_toggled ();
  void on_m_Cost_toggled ();
  void on_m_LogicalProp_toggled ();
  void on_m_PhysicalProp_toggled ();
  void on_m_Context_toggled ();
  void on_m_ASM_toggled ();
  void on_m_GroupAnalysis_toggled ();
  void on_m_JBBC_toggled ();
  void on_m_TableAnalysis_toggled ();
  void on_m_QueryGraph_toggled ();
  void on_m_CascadesTrcInfo_toggled ();
  void on_m_WizDirect_toggled ();
};

#endif //PROPDIALOG_H
