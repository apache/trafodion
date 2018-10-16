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
#include "QueryTreeView.h"
#include "ui_QueryTreeView.h"
#include "MainWindow.h"
#include "QueryData.h"

// defined in MainWindow.cpp
extern MainWindow *GlobGuiMainWindow;

QueryTreeView::QueryTreeView(QWidget * parent):
QWidget(parent), ui(new Ui::QueryTreeView)
{
    ui->setupUi(this);
    QStringList header;
    header << "SQL Query Tree" << "Cost" << "Operator Cost" << "Rows" <<
        "Expr Type";
    QTreeWidgetItem *hdrItem = new QTreeWidgetItem(header);
    hdrItem->setIcon(0, QIcon(":/file/R_SQLNODE"));
    hdrItem->setIcon(1, QIcon(":/file/R_SQLNODE"));
    hdrItem->setIcon(2, QIcon(":/file/R_SQLNODE"));
    hdrItem->setIcon(3, QIcon(":/file/R_SQLNODE"));
    hdrItem->setIcon(4, QIcon(":/file/R_SQLNODE"));
    ui->m_tree->setHeaderItem(hdrItem);
    ui->m_tree->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->m_tree->setIconSize(QSize(32, 32));
    ui->m_tree->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->m_tree->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
}

QueryTreeView::~QueryTreeView()
{
    delete ui;
}

/* Public Method Begin */
void QueryTreeView::UpdateView()
{
    //------------------------------------------------------------------
    // Delete the existing Tree.
    //------------------------------------------------------------------
    Free();
    //------------------------------------------------------------------
    // Now we do the tree walk to display the tree.
    //------------------------------------------------------------------
    void *tree;
    void *plan;
    GlobGuiMainWindow->m_querydata->GetData(&tree, &plan);
    DisplayQueryTree(tree, plan, NULL);
    ui->m_tree->expandAll();
    for (int i = 0; i < ui->m_tree->columnCount(); i++)
    {
        ui->m_tree->resizeColumnToContents(i);
        int width = ui->m_tree->columnWidth(i);
        ui->m_tree->setColumnWidth(i, width + 30);
    }
}

/* Public Method End */

/* Private Method Start */

void QueryTreeView::Free(void)
{
    ui->m_tree->clear();
}

void QueryTreeView::DisplayQueryTree(void *tree, void *plan,
                                     QTreeWidgetItem * parentTreeItem)
{
    ExprNode *qTree = (ExprNode *) tree;

    CascadesPlan *cPlan = (CascadesPlan *) plan;
    DFTDETAILS *pDftDetails;

    if (qTree != NULL)
    {
        Int32 bitmapIndex;
        NAString nodeText;

        GetOperatorImageText(qTree, bitmapIndex, nodeText);

        QString nodeTextData = QString(QLatin1String(nodeText.data()));
        QStringList rowValues;
        rowValues.append(nodeTextData);
        QTreeWidgetItem *treeItem;
        treeItem = new QTreeWidgetItem(rowValues);
        treeItem->setIcon(0, QIcon(":/file/Resource/Main/Tdbnodes.bmp"));

        if (parentTreeItem == NULL)
        {
            ui->m_tree->addTopLevelItem(treeItem);
        }
        else
        {
            parentTreeItem->addChild(treeItem);
        }

        pDftDetails = new DFTDETAILS;
        pDftDetails->currNode = qTree;
        pDftDetails->currPlan = cPlan;
        QVariant itemData = qVariantFromValue(pDftDetails);
        treeItem->setData(0, Qt::UserRole, itemData);
        //set column alignment for each row
        treeItem->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
        treeItem->setTextAlignment(2, Qt::AlignRight | Qt::AlignVCenter);
        treeItem->setTextAlignment(3, Qt::AlignRight | Qt::AlignVCenter);

        if (nodeTextData.compare(QString("NULL")))
        {
            FillExprType(qTree, cPlan, treeItem);
            FillRowCost(qTree, cPlan, treeItem);
        }

        for (Int32 i = qTree->getArity() - 1; i >= 0; i--)
        {
            if (cPlan == NULL)
            {
                RelExpr *relExpr = (RelExpr *) qTree;
                if ((relExpr->child(i)).getMode() == ExprGroupId::MEMOIZED)
                {
                    // display a MEMO group as a leaf op
                    CutOp *l =::new CutOp(99, CmpCommon::statementHeap());
                    l->setGroupId((relExpr->child(i)).getGroupId());
                    DisplayQueryTree(l, NULL, treeItem);
                }
                else
                {
                    DisplayQueryTree(qTree->getChild(i), NULL, treeItem);
                }
            }
            else
            {
                const CascadesPlan *childPlan;
                if (cPlan->getContextForChild(i) == NULL)
                {
                    childPlan = NULL;
                }
                else
                {
                    childPlan = cPlan->getSolutionForChild(i);
                }

                if (childPlan != NULL)
                {
                    DisplayQueryTree(childPlan->getPhysicalExpr(),
                                     (void *)childPlan, treeItem);
                }
                else
                {
                    DisplayQueryTree(::new ConstValue(), (void *)childPlan,
                                     treeItem);
                }
            }                   // if (cPlan == NULL)
        }                       // for loop
    }                           //qTree != NULL
}

void QueryTreeView::GetOperatorImageText(void *tree, Int32 & bitmapIndex,
                                         NAString & nodeText)
{
    ExprNode *qTree = (ExprNode *) tree;
    OperatorTypeEnum opType = qTree->getOperatorType();
    switch (opType)
    {
    case REL_ROOT:
        bitmapIndex = IDX_ROOT;
        nodeText = qTree->getText();
        break;
    case REL_JOIN:
        bitmapIndex = IDX_JOIN;
        nodeText = qTree->getText();
        break;
    case REL_SCAN:
        bitmapIndex = IDX_SCAN;
        nodeText = qTree->getText();
        break;
    default:
        bitmapIndex = IDX_GENERIC;
        nodeText = qTree->getText();
    }
}

//---------------------------------------------------------------
// This member function fills in the Group Attribute
// information for the list view.
//---------------------------------------------------------------
void QueryTreeView::FillGroupAttribs(void *tree, void *plan,
                                     QTreeWidgetItem * treeItem)
{
    /* Terry: To be restored
       GroupAttributes *ga = NULL;
       QString labelString;

       ExprNode* qTree    = (ExprNode*) tree;
       CascadesPlan* qPlan = (CascadesPlan*) plan;

       ga = qTree->castToRelExpr()->getGroupAttr();

       if (ga!=NULL) {
       Lng32 groupId = (Lng32) qTree->castToRelExpr()->getGroupId();
       if (groupId == NULL_COLL_INDEX)
       labelString  = QString("in: %d, out: %d").arg(ga->getCharacteristicInputs().entries()).arg(ga->getCharacteristicOutputs().entries());
       else
       labelString  = QString("grp: %d, in: %d, out: %d").arg(groupId).arg(ga->getCharacteristicInputs().entries()).arg(ga->getCharacteristicOutputs().entries());
       }
       else
       {
       labelString = QString("No Group Properties");
       }

       treeItem->setText(3, labelString);
     */
}

//---------------------------------------------------------------
// This member function fills in the Expression type
// information for the list view.
//---------------------------------------------------------------
void QueryTreeView::FillExprType(void *tree, void *plan,
                                 QTreeWidgetItem * treeItem)
{
    QString labelString;
    ExprNode *qTree = (ExprNode *) tree;
    CascadesPlan *qPlan = (CascadesPlan *) plan;

    if (qTree->castToRelExpr() != NULL)
    {
        if (qTree->castToRelExpr()->isPhysical())
        {
            if (qPlan != NULL)
            {
                if (qPlan->getPhysicalProperty() != NULL
                    && qPlan->getRollUpCost() != NULL)
                    labelString = QString("  plan");
                else
                    labelString = QString("  incomplete plan");
            }
            else
            {                   // (qPlan != NULL)
                if (qTree->castToRelExpr()->getPhysicalProperty() != NULL
                    && qTree->castToRelExpr()->getRollUpCost() != NULL)
                    labelString = QString("  plan");
                else
                    labelString = QString("  physical");
            }
        }
        else
        {                       // If not physical
            labelString = QString("  Logical");
        }
    }
    else
    {                           // (qTree->castToRelExpr() != NULL)
        labelString = QString("  Unknown");
    }
    treeItem->setText(4, labelString);
}

//---------------------------------------------------------------
// GSH : This member function fills in the rows and cost
// information for the list view.
//---------------------------------------------------------------
void QueryTreeView::FillRowCost(void *tree, void *plan,
                                QTreeWidgetItem * treeItem)
{
    GroupAttributes *ga = NULL;
    double rows;

    ExprNode *qTree = (ExprNode *) tree;
    CascadesPlan *qPlan = (CascadesPlan *) plan;

    float cost = 0.0f;
    float cost2 = 0.0f;
    if (qTree->castToRelExpr() != NULL && (ga = qTree->castToRelExpr()->getGroupAttr()) != NULL)
    {
        CostScalar numRows = (CostScalar) - 1;
        if (qPlan != NULL)
        {
            ga = qPlan->getPhysicalExpr()->castToRelExpr()->getGroupAttr();
            EstLogPropSharedPtr inputLP =
                qPlan->getContext()->getInputLogProp();
            if (ga->existsLogExprForSynthesis())
                numRows =
                    ga->outputLogProp(inputLP)->getResultCardinality();
            rows = numRows.value();
            const Cost *planCost = qPlan->getRollUpCost();
            if (planCost)
                cost = float (planCost->convertToElapsedTime().getValue());
            const Cost *operatorCost = qPlan->getOperatorCost();
            if (operatorCost)
                cost2 =
                    float (operatorCost->convertToElapsedTime().
                           getValue());
        }
        else
        {
            rows = numRows.value();
            const Cost *planCost = qTree->castToRelExpr()->getRollUpCost();
            if (planCost)
                cost = float (planCost->convertToElapsedTime().getValue());
            const Cost *operatorCost =
                qTree->castToRelExpr()->getOperatorCost();
            if (operatorCost)
                cost2 =
                    float (operatorCost->convertToElapsedTime().
                           getValue());
            //--------------------------------------------------------
        }
    }
    else
    {
        rows = (float)-1;
        cost = (float)-1;
        cost2 = (float)-1;
    }
    treeItem->setText(1, QString("%1").arg(cost, 0, 'f', 8));
    treeItem->setText(2, QString("%1").arg(cost2, 0, 'f', 8));
    // --------------------------------------------------------------------
    // We want to print "rows" with commas in the appropriate places
    // --> It's amazing how much work this is!  There's got to be some built-in
    // primitive!!!
    // --------------------------------------------------------------------
    QString string = QString("");
    if (rows >= 10000)
    {
        UInt32 num_commas = ((UInt32) log10(rows)) / 3;
        double remains = rows;  // don't modify rows
        // first:  print each <int, comma> part
        for (UInt32 i = num_commas; i > 0; i--)
        {
            double p = pow((double)10, (Int32) i * 3);
            ULng32 head = (ULng32) (remains / p);
            remains = remains - (head * p);
            if (i == num_commas)  // first thing printed
            {
                string.append(QString("%1").arg(head));
                //Previous MFC:       tmp.Format("%u,", head) ;
            }
            else                // we may need to print place-holding zero's
            {
                if (head < 10)
                {
                    string.append(QString("00%1").arg(head));
                    //Previous MFC:       tmp.Format("00%u,", head) ;
                }
                else if (head < 100)
                {
                    string.append(QString("0%1").arg(head));
                    //Previous MFC:       tmp.Format("0%u,", head) ;
                }
                else
                {
                    string.append(QString("%1").arg(head));
                    //Previous MFC:       tmp.Format("%u,", head) ;
                }
            }
        }                       //for
        // last:  print the last part (which is a double)
        if (remains < 10)
        {
            string.append(QString("00%1").arg(remains, 0, 'f', 2));
            //Previous MFC:       tmp.Format ("00%.2f", remains) ;
        }
        else if (remains < 100)
        {
            string.append(QString("0%1").arg(remains, 0, 'f', 2));
            //Previous MFC:       tmp.Format ("0%.2f", remains) ;
        }
        else
        {
            string.append(QString("%1").arg(remains, 0, 'f', 2));
            //Previous MFC:       tmp.Format ("%.2f", remains) ;
        }
    }
    else
    {
        string = QString("%1").arg(rows, 0, 'f', 2);
        //Previous MFC:       string.Format("%0.2f", rows);
    }
    treeItem->setText(3, string);
}

/* Private Method End */
void QueryTreeView::
on_m_tree_customContextMenuRequested(const QPoint & pos)
{
    QTreeWidgetItem *curItem = ui->m_tree->itemAt(pos);
    if (curItem == NULL)
        return;
    QVariant v = curItem->data(0, Qt::UserRole);
    DFTDETAILS *pDft = v.value < DFTDETAILS * >();
    if (NULL != pDft)
    {
        GlobGuiMainWindow->m_querydata->SetCurrExprAndPlan(pDft->currNode,
                                                           pDft->currPlan);
        GlobGuiMainWindow->popUpContextMenu(QCursor::pos());
    }
}
