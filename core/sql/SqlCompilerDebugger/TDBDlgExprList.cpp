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
#include "TDBDlgExprList.h"
#include "ui_TDBDlgExprList.h"

TDBDlgExprList::TDBDlgExprList(QWidget * parent, ComTdb* currTdb, const NAString & tdbNodeName):QDialog(parent), ui(new Ui::TDBDlgExprList)
{
  ui->setupUi(this);
  m_tdb = currTdb;
  m_tdbNodeName = tdbNodeName;
  size_t BUFSIZE = 512;
  char buf[BUFSIZE];
  snprintf(buf, BUFSIZE, "%s node has %d expressions", m_tdbNodeName.data(), m_tdb ? m_tdb->numExpressions() : 0);
  ui->label->setText(QString(buf));
  DisplayExprList();
}

TDBDlgExprList::~TDBDlgExprList()
{
  delete ui;
}

// -----------------------------------------------------------------------
// inline standalone method to create text info for an exp_expr object
// -----------------------------------------------------------------------
NAString TDBDlgExprList::ExprNodeTypeToString(ex_expr::exp_node_type nodeType)
{
   NAString string;
   switch (nodeType) {
   case ex_expr::exp_ARITH_EXPR : 
	                           string =  "Arithmetic";
                                   break;
   case ex_expr::exp_SCAN_PRED :
                                   string = "Scan Pred.";
                                   break;
   case ex_expr::exp_INPUT_OUTPUT :
                                   string = "Input Ouput";
                                   break;
   case ex_expr::exp_AGGR :
                                   string = "Aggregate";
                                   break;
   case ex_expr::exp_DP2_EXPR :
                                   string = "Dp2";
                                   break;
   default :	   
                                   string = "Unknown";
                                   break;
   }
   return string;
}

NAString TDBDlgExprList::ExprNodeFlagsToString(ex_expr* exprNode)
{
   NAString string;
   
   if (exprNode->getFixupConstsAndTemps())
    string += "FIXUP_CONSTS_AND_TEMPS|";

   if (exprNode->generateNoPCode())
     string += "GENERATE_NO_PCODE|";

   if (exprNode->getPCodeGenCompile())
     string += "PCODE_GEN_COMPILE|";
   
   if (exprNode->getPCodeMoveFastpath())
     string += "PCODE_MOVE_FASTPATH|";

   if (exprNode->forInsertUpdate())
     string += "FOR_INSERT_UPDATE|";

   if (exprNode->usePCodeEvalAligned())
     string += "PCODE_EVAL_ALIGNED|";

   if (exprNode->handleIndirectVC())
     string += "HANDLE_INDIRECT_VC|";

   if (exprNode-> getPCodeNative())
     string += "PCODE_EVAL_NATIVE";
   
   int len = string.length();
   string.remove(len-1);

   return string;
}

void TDBDlgExprList::DisplayExprList() 
{
  size_t BufSize = 512;
  char buf[BufSize];
  NAString exprType;
  void* exprClauses;
  Int32 exprVersion;
  NAString exprFlags;
  Int32 exprLength;
  ex_expr* exprNode;
  NAString exprName;
  QString item_str;
  //set row count
  int imax = m_tdb ? m_tdb->numExpressions() : 0;
  ui->tableWidget->setRowCount(imax);  

  for (int i = 0; i < imax; i++)
  {
      
      exprNode = m_tdb->getExpressionNode(i);
      exprName = m_tdb->getExpressionName(i);

      if(exprNode)
      {
        exprType = ExprNodeTypeToString(exprNode->getType());
        exprClauses = (void*)exprNode->getClauses();
        exprVersion = (Int32)exprNode->getClassVersionID();
        exprFlags = ExprNodeFlagsToString(exprNode);
        exprLength = exprNode->getLength();
      }
      else 
      {
        exprType = "Null";
        exprClauses = 0;
        exprVersion = 0;
        exprFlags = "0";
        exprLength = 0;
      }

      //expression name & node pointer
      ::snprintf(buf, BufSize, "%s @ %p", exprName.data(), exprNode);
      QTableWidgetItem * pItem0 = new QTableWidgetItem(QIcon(":/file/Resource/Main/Exprlist.bmp"), QString::fromAscii(buf));

      //expression type
      QTableWidgetItem *pItem1 = new QTableWidgetItem(QString(exprType.data()));
      
      //Clauses
      ::snprintf(buf, BufSize, "@ %p", exprClauses);
      QTableWidgetItem *pItem2 = new QTableWidgetItem(QString::fromAscii(buf));

      //Version
      ::snprintf(buf, BufSize, "%d", exprVersion);
      QTableWidgetItem *pItem3 = new QTableWidgetItem(QString::fromAscii(buf));

      //Flag
      ::snprintf(buf, BufSize, "%s", exprFlags.data());
      QTableWidgetItem *pItem4 = new QTableWidgetItem(QString::fromAscii(buf));

      //Len
      ::snprintf(buf, BufSize, "%d", exprLength);
      QTableWidgetItem *pItem5 = new QTableWidgetItem(QString::fromAscii(buf));

      ui->tableWidget->setItem (imax-1-i, 0, pItem0);
      ui->tableWidget->setItem (imax-1-i, 1, pItem1);
      ui->tableWidget->setItem (imax-1-i, 2, pItem2);
      ui->tableWidget->setItem (imax-1-i, 3, pItem3);
      ui->tableWidget->setItem (imax-1-i, 4, pItem4);
      ui->tableWidget->setItem (imax-1-i, 5, pItem5);
  }//for (Int32 i = 0, ...)
  //adjust column width
  for(int i=0; i<ui->tableWidget->columnCount(); i++){
    ui->tableWidget->resizeColumnToContents(i);
    int width = ui->tableWidget->columnWidth (i);
    ui->tableWidget->setColumnWidth (i,width + 30);
  }
}

void TDBDlgExprList::on_btnOK_clicked() 
{
  this->close();
}
