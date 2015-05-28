// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
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
#include "PropDialog.h"
#include "ui_PropDialog.h"

PropDialog::PropDialog(CascadesPlan * p, ExprNode * e, NAString title, QWidget * parent):QDialog(parent),
ui(new
   Ui::PropDialog)
{
  ui->setupUi(this);
  //setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint);
  m_plan = p;
  m_expr = e;
  m_propertiesListText = "Properties of Node : " + title;
  ui->m_label->setText(QString(m_propertiesListText));
  ui->m_listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
}

PropDialog::~PropDialog()
{
  delete ui;
}

const Int32 MAX_STR_LEN = 1001;

void PropDialog::setLabel(QString & lbl)
{
  ui->m_label->setText(lbl);
}

void PropDialog::FreePropMemory()
{
  ui->m_listWidget->clear();
}

void PropDialog::addSimpleCostVectorDetail(const SimpleCostVector &scv, const Cost *cost)
{
  NAString propText;

  char validascii[MAX_STR_LEN];

  // Depending on the cost model in effect display the cost details
  if (CmpCommon::getDefault(SIMPLE_COST_MODEL) == DF_ON)
  {
    double cpu, io, msg, idle, seqIOs, randIOs, total;
    Lng32 probes;
    cost->getExternalCostAttr(cpu, io, msg, idle, seqIOs, randIOs, total, probes);

    if ((CmpCommon::getDefault(EXPLAIN_DISPLAY_FORMAT) == DF_EXTERNAL) ||
        (CmpCommon::getDefault(EXPLAIN_DISPLAY_FORMAT) == DF_EXTERNAL_DETAILED))
    {
      sprintf(validascii, "  cpu_et =%.6f", cpu);
      propText = validascii;
      AddListItem(propText, COST);
      
      sprintf(validascii, "  io_et =%.6f", io);
      propText = validascii;
      AddListItem(propText, COST);
 
      sprintf(validascii, "  msg_et =%.6f", msg);
      propText = validascii;
      AddListItem(propText, COST);

      sprintf(validascii, "  idl_et =%.6f", idle);
      propText = validascii;
      AddListItem(propText, COST);
    }
    if (CmpCommon::getDefault(EXPLAIN_DISPLAY_FORMAT) == DF_EXTERNAL_DETAILED)
    {
      sprintf(validascii, "  IO_SEQ =%.6f", seqIOs);
      propText = validascii;
      AddListItem(propText, COST);

      sprintf(validascii, "  IO_RAND =%.6f", randIOs);
      propText = validascii;
      AddListItem(propText, COST);
    }
    if (CmpCommon::getDefault(EXPLAIN_DISPLAY_FORMAT) == DF_INTERNAL)
    {
      sprintf(validascii, "  TC_PROC =%g", scv.getTcProc().getValue());
      propText = validascii;
      AddListItem(propText, COST);

      sprintf(validascii, "  TC_PROD =%g", scv.getTcProd().getValue());
      propText = validascii;
      AddListItem(propText, COST);

      sprintf(validascii, "  TC_SENT =%g", scv.getTcSent().getValue());
      propText = validascii;
      AddListItem(propText, COST);

      sprintf(validascii, "  IO_SEQ =%g", scv.getIoSeq().getValue());
      propText = validascii;
      AddListItem(propText, COST);

      sprintf(validascii, "  IO_RAND =%g", scv.getIoRand().getValue());
      propText = validascii;
      AddListItem(propText, COST);
    }
  }
  else
  {
    sprintf(validascii, "  cpu_et =%.6f", scv.getCPUTime().getValue());
    propText = validascii;
    AddListItem(propText, COST);

    sprintf(validascii, "  io_et =%.6f", scv.getIOTime().getValue());
    propText = validascii;
    AddListItem(propText, COST);

    sprintf(validascii, "  msg_et =%.6f", scv.getMessageTime().getValue());
    propText = validascii;
    AddListItem(propText, COST);

    sprintf(validascii, "  idl_et =%.6f", scv.getIdleTime().getValue());
    propText = validascii;
    AddListItem(propText, COST);
  }
} // PropDialog::addSimpleCostVectorDetail(....)

void PropDialog::addSimpleCostVector(const char* header
                              ,const SimpleCostVector &scv)
{
  NAString propText;
  propText = header;
  propText += "(";
  propText += scv.getDetailDesc(DF_OFF);
  propText += ")";
  AddListItem(propText, COST);

} // PropDialog::addSimpleCostVector(...)

QIcon PropDialog::propIcon(PropertyTypes t)
{
  switch(t)
  {
    case CHARIP:
       return QIcon(":/file/Resource/Main/I.bmp");
    case CHAROP:
       return QIcon(":/file/Resource/Main/O.bmp");
    case CHARESSOP:
       return QIcon(":/file/Resource/Main/sqldbg.ico");
    case CONSTRAINTS:
       return QIcon(":/file/Resource/Main/sqldbg.ico");
    case COST:
       return QIcon(":/file/Resource/Main/C.bmp");
    case LOGICAL:
       return QIcon(":/file/Resource/Main/L.bmp");
    case PHYSICAL:
       return QIcon(":/file/Resource/Main/P.bmp");
    case PropDialog::CONTEXT:
       return QIcon(":/file/Resource/Main/P.ico");
    case ASM:
       return QIcon(":/file/Resource/Main/A.bmp");
    case GROUPANALYSIS:
       return QIcon(":/file/Resource/Main/G.bmp");
    case TABLEANALYSIS:
       return QIcon(":/file/Resource/Main/T.bmp");
    case JBBC_PT:
       return QIcon(":/file/Resource/Main/J.bmp");
    case QGRAPH:
       return QIcon(":/file/Resource/Main/Q.bmp");
    case CASCADESTRACEINFO:
       return QIcon(":/file/Resource/Main/P.ico");
    default :
       return QIcon(":/file/Resource/Main/P.ico");
  }
}

void PropDialog::addCost(const NAString& header, const Cost *cost)
{

  NAString propText;
  char validascii[MAX_STR_LEN];

  // Print the number of CPUs:
  sprintf(validascii,"count of CPUs: %d", cost->getCountOfCPUs());
  propText = validascii;
  AddListItem(propText, COST);

  // Print the number plan fragments per cpu:
  sprintf(validascii,"plan fragments per CPU: %d",
          cost->getPlanFragmentsPerCPU());
  propText = validascii;
  AddListItem(propText, COST);

  // Print Priority
  sprintf(validascii,"plan priority: level = %d , demotion = %d, risk prem= %.4f",
        cost->getPlanPriority().getLevel(),
        cost->getPlanPriority().getDemotionLevel(),
        cost->getPlanPriority().riskPremium().getValue()
         );

  propText = validascii;
  AddListItem(propText, COST);

  // Display header:
  sprintf(validascii,"@@@@@@@@@@@@@@@@@@@@@@@@@@");
  propText = validascii;
  AddListItem(propText, COST);

  propText = header;
  AddListItem(propText, COST);

  // Print the elapsed time for this cost object:
  sprintf(validascii,"elapsed time(%g)",
          cost->displayTotalCost().value());
  propText = validascii;
  AddListItem(propText, COST);

  // Depending on the cost model in effect display the cost vectors
  if (CmpCommon::getDefault(SIMPLE_COST_MODEL) == DF_ON)
  {
    // Display cost:
    // The order in which vectors are displayed reflects current usage:
    // SCMLR

    const SimpleCostVector &scm   = cost->getScmCplr();

    addSimpleCostVector("scm",scm);
    addSimpleCostVectorDetail(scm, cost);
  }
  else
  {
    // Display cost:
    // The order in which vectors are displayed reflects current usage:
    // CPLR, CPTB, CPFR, CPB, OPLR, OPFR, total cost.

    const SimpleCostVector &cplr = cost->getCplr();
    const SimpleCostVector &cptb = cost->getCpbcTotal();
    const SimpleCostVector &cpfr = cost->getCpfr();
    const SimpleCostVector &cpfb  = cost->getCpbc1();
    const SimpleCostVector &tc   = cost->getTotalCost();

    addSimpleCostVector("c.p. last row",cplr);
    addSimpleCostVectorDetail(cplr);

    addSimpleCostVector("c.p. total blocking",cptb);
    addSimpleCostVectorDetail(cptb);

    addSimpleCostVector("c.p. first row",cplr);
    addSimpleCostVectorDetail(cpfr);

    addSimpleCostVector("c.p. first blk.",cpfb);
    addSimpleCostVectorDetail(cpfb);

    addSimpleCostVector("total",tc);
    addSimpleCostVectorDetail(tc);
  }
} // PropDialog::addCost(...)


void PropDialog::displayEstLogProp(EstLogPropSharedPtr estLogProp)
{
  //EstLogProp* estLogProp = shared_estLogProp;
  if (estLogProp == NULL) return;

  char validascii[MAX_STR_LEN];

  NAString propText;

  if ( estLogProp->getInputForSemiTSJ() )
  {
      sprintf (validascii, "Input For Semi-TSJ" );
      propText = validascii;
      AddListItem( propText, LOGICAL);
  }

  if (estLogProp->isCacheable())
    propText += estLogProp->getNodeSet()->getText();
  else
  {
    sprintf (validascii, "CANodeIdSet :" );
    propText = validascii;
    sprintf (validascii, "NULL" );
    propText += validascii;
  }
  AddListItem( propText, LOGICAL);

  const ColStatDescList & stats = estLogProp->getColStats();
  for (Int32 i = 0; i < (Int32)stats.entries(); i++)
  {
    if ( i > 0 )
    {
        sprintf (validascii, "---------------------------------") ;
        propText = validascii ;
        AddListItem (propText, LOGICAL) ;
     }

    ColStatsSharedPtr colStats = stats[i]->getColStats();

    // if colStat is for a virtual column (example: constant) then do not
    // look for the column name in NATable, instead display the ValueId 
    // of the columns

    if (colStats->isVirtualColForHist() )
    {
      ValueId x = stats[i]->getColumn();

      sprintf(validascii, "virtual histogram for valueid: %4u", (CollIndex)(x)); 
      propText = validascii;      

      x.getItemExpr()->unparse(propText);
    }
    else
    {
      sprintf (validascii, "Table Column: ") ;
      propText = validascii;

      propText += colStats->getStatColumns()[0]->getFullColRefNameAsAnsiString();
    }
    
    AddListItem(propText, LOGICAL); 

    if (colStats->isFakeHistogram())
    {
        sprintf (validascii, "*** FAKE HISTOGRAM ***") ;
        propText = validascii;
        AddListItem(propText, LOGICAL);
    }

    if (colStats->isUnique())
    {
        sprintf (validascii, "*** UNIQUE COLUMN ***") ;
        propText = validascii;
        AddListItem(propText, LOGICAL);
    }

    sprintf (validascii,
                   "Histogram ID = %lu: uec = %.4f; rowcount = %.4f",
                  (unsigned long)colStats->getHistogramId().get_value(),
                   colStats->getTotalUec().getValue(),
                   colStats->getRowcount().getValue());
    propText = validascii;
    AddListItem(propText, LOGICAL);

    sprintf (validascii, "MinValue = ");
    propText = validascii;
    propText += colStats->getMinValue().getText();
    AddListItem(propText, LOGICAL);

    sprintf (validascii, "MaxValue = ");
    propText = validascii;
    propText += colStats->getMaxValue().getText();
    AddListItem(propText, LOGICAL);

    sprintf (validascii, "RowRedFactor = %.6g; UecRedFactor = %.6g",
                   colStats->getRedFactor().getValue(),
                   colStats->getUecRedFactor().getValue());
    propText = validascii;
    AddListItem(propText, LOGICAL);

    char * s = validascii;
    char * t = s;

    ColStatDescSharedPtr tempStatDesc = stats[i];

    if (tempStatDesc->getAppliedPreds().entries() > 0)
    {
        sprintf (validascii, "Applied Predicates:");
        propText = validascii;
        AddListItem(propText, LOGICAL);

        // we'd like to be able to see *ALL* of the applied predicates!
        ValueIdSet thePreds = tempStatDesc->getAppliedPreds() ;
        ValueIdList predList = thePreds ;
        for ( CollIndex i = 0 ; i < predList.entries() ; i++ )
        {
            sprintf(validascii, "       ") ;
            propText = validascii ;
            ValueIdSet tempSet ;
            tempSet.insert (predList[i]) ;
            tempSet.unparse(propText) ;

            // now we want to filter out any unnecessary references to
            // SYSKEY's, INDEXes, etc., in the applied-predicate list

            Int32 loop_escape = 0 ;
            while ( propText.contains ("\\NSK") )
            {
                if ( loop_escape++ > 100 ) break ; // avoid infinite loops!

                UInt32 i ;
                size_t pos = 0 ;
                size_t length = 0 ;

                // filter out anything that looks like the regular expression
                //      "\\NSK...([0-9]+)"

                pos = propText.index("\\NSK") ;
                if (propText(pos-1) == ',' ) pos-- ; // kill extra comma
                for ( i = pos ; i < propText.length() ; i++, length++ )
                {
                    if ( propText(i) == ')' ) break ;
                }

                propText.replace ( pos, length+1, "") ;
            }
            AddListItem (propText, LOGICAL) ;
         } //for ( CollIndex i = 0 ; i < predList.entries() ; i++ )
    }

    const FrequentValueList & fvList = colStats -> getFrequentValues();

    sprintf (validascii, "Max Frequency: %.2f", fvList.getMaxFrequency().getValue());
    propText = validascii;
    AddListItem(propText, LOGICAL);

    sprintf (validascii, "Frequent Values: ");
    propText = validascii;
    AddListItem(propText, LOGICAL);

    for (CollIndex i = 0; i < fvList.entries(); i++) {
      const FrequentValue fv = fvList[i];
      t = validascii; // validascii
      sprintf (t, "   Hash Val=%u, ", fv.getHash());
      t += strlen(t);
      sprintf (t, "Encoded Val=");
      t += strlen(t);
      sprintf (t, "%.2f, ", fv.getEncodedValue().getDblValue());
      t += strlen(t);
      sprintf (t, "Freq.=%.2f, ", fv.getFrequency().value());
      t += strlen(t);
      sprintf (t, "Probab.=%.2f, ", fv.getProbability().value());
      t += strlen(t);
      sprintf (t, "Ave freq.=%.2f\n", fv.getFrequency().value() * fv.getProbability().value());

      propText = validascii;
      AddListItem(propText, LOGICAL);
    }

    HistogramSharedPtr hist = colStats->getHistogram();
    if ( !( hist == NULL ) )
    for (CollIndex i = 0; i < hist->entries(); i++)
    {
          t = s; // validascii
          sprintf (t, "Bound %d ", i);
          t += strlen (s);
          if ((*hist)[i].isBoundIncl())
            sprintf (t, "<= ");
          else
            sprintf (t, "<  ");
          t = s + strlen(s);
          sprintf (t, "%s: rows=%.2f,uec=%.2f",
                   (*hist)[i].getBoundary().getText().data(),
                   (*hist)[i].getCardinality().getValue(),
                   (*hist)[i].getUec().getValue());
          propText = validascii;
          AddListItem(propText, LOGICAL);
     }
  }//for (Int32 i = 0; i < (Int32)stats.entries(); i++)
}

void PropDialog::AddSeparatorLine(const QString & s, Qt::GlobalColor color)
{
  QString qline = s;
  if(0 == qline.length())
    return;
  int index = qline.lastIndexOf("\n");
  if(index >= 0)
    qline[index] = ' ';
  QListWidgetItem* lwItem = new QListWidgetItem(qline);
  //set font, color
  lwItem->setFont(QFont("Helvetica", -1, 75, true));
  lwItem->setForeground(Qt::blue);
  lwItem->setBackground(color);
  ui->m_listWidget->addItem(lwItem);
}

Int32 PropDialog::AddListItem(const NAString & s, PropertyTypes t)
{
  if(0 == s.length())
    return -1;
  QString qline((const char*)s);
  int index = qline.lastIndexOf("\n");
  if(index >= 0)
    qline[index] = ' ';
  QListWidgetItem* lwItem = new QListWidgetItem(propIcon(t), qline);
  ui->m_listWidget->addItem(lwItem);
  return 0;
}

void PropDialog::UpdatePropList()
{
  NAString propText;
  //--------------------------------------------------------------------
  // GSH : First you have to delete all the existing items in the
  // m_properties list control.
  //--------------------------------------------------------------------
  FreePropMemory();
  GroupAttributes *ga = NULL;
  if (m_expr != NULL)
    ga = m_expr->castToRelExpr()->getGroupAttr();

  if (ga != NULL)
  {
      QString endstr = "---------- end ----------";
      char validascii[MAX_STR_LEN];
      if (m_displayOutputs)
      {   
          //add separator
          AddSeparatorLine(ui->m_CharOutputs->text() + ':', Qt::lightGray);
          for (ValueId x = ga->getCharacteristicOutputs().init();
               ga->getCharacteristicOutputs().next(x);
               ga->getCharacteristicOutputs().advance(x))
          {
              sprintf(validascii, "ValId #%d: ", (CollIndex)x);
              propText = validascii;
              x.getItemExpr()->unparse(propText);
              AddListItem(propText, CHAROP);
          } //end for
          AddSeparatorLine(endstr);
      }//endif (m_displayOutputs)
      if (m_displayEssOutputs)
      {
          //add separator
          AddSeparatorLine(ui->m_EssOutputs->text() + ':', Qt::lightGray);
          for (ValueId x = ga->getEssentialCharacteristicOutputs().init();
               ga->getEssentialCharacteristicOutputs().next(x);
               ga->getEssentialCharacteristicOutputs().advance(x))
            {
              sprintf(validascii, "ValId #%d: ", (CollIndex) x);
              propText = validascii;
              x.getItemExpr()->unparse(propText);
              AddListItem(propText, CHARESSOP);
            }  //end for
          AddSeparatorLine(endstr);
      }//endif (m_displayEssOutputs)
      if (m_displayInputs)
      {
          //add separator
          AddSeparatorLine(ui->m_CharInputs->text() + ':', Qt::lightGray);
          for (ValueId x = ga->getCharacteristicInputs().init();
               ga->getCharacteristicInputs().next(x);
               ga->getCharacteristicInputs().advance(x))
          {
              sprintf(validascii, "ValId #%d: ", (CollIndex) x);
              propText = validascii;
              x.getItemExpr()->unparse(propText);
              AddListItem(propText, CHARIP);
          }                   // end for
          AddSeparatorLine(endstr);
      }                       // endif (m_displayInputs)
      if (m_displayConstraints)
      {
          //add separator
          AddSeparatorLine(ui->m_Constraints->text() + ':', Qt::lightGray);
          for (ValueId x = ga->getConstraints().init();
               ga->getConstraints().next(x); ga->getConstraints().advance(x))
          {

              sprintf(validascii, "ValId #%d: ", (CollIndex) x);
              propText = validascii;
              x.getItemExpr()->unparse(propText);
              AddListItem(propText, CONSTRAINTS);
          }                   // end for
          AddSeparatorLine(endstr);
      }                       // endif (m_displayConstraints)
      if (m_displayPhysical)
      {
          //add separator
          AddSeparatorLine(ui->m_PhysicalProp->text() + ':', Qt::lightGray);
          const PhysicalProperty *pp;
          if (m_plan != NULL)
            pp = m_plan->getPhysicalProperty();
          else
            pp = m_expr->castToRelExpr()->getPhysicalProperty();

          if (pp != NULL)
          {
              const ValueIdList & sortKey = pp->getSortKey();
              //-------------------------------------------------------
              // GSH : Physical property -> sorted by ...
              //-------------------------------------------------------
              if (sortKey.entries() > 0)
              {
                  propText = "ordered_by(";
                  for (Int32 i = 0; i < (Int32) sortKey.entries(); i++)
                  {
                      if (i > 0)
                        propText += ", ";
                      sortKey[i].getItemExpr()->unparse(propText,
                                                        DEFAULT_PHASE,
                                                        EXPLAIN_FORMAT);
                  }
                  propText += ")";
                  AddListItem(propText, PHYSICAL);
                  //-----------------------------------------------------
                  // Physical Prop -> sort order type ...
                  // Only displayed if there was a sort key
                  //-----------------------------------------------------
                  switch (pp->getSortOrderType())
                  {
                    case NO_SOT:
                      propText = "sort order type: undetermined";
                      break;
                    case ESP_NO_SORT_SOT:
                      propText = "sort order type: index, valid in ESP";
                      break;
                    case ESP_VIA_SORT_SOT:
                      propText = "sort order type: sort";
                      break;
                    case DP2_SOT:
                      propText = "sort order type: index, valid in DP2 only";
                      break;
                    default:
                      // should never happen
                      propText = "";
                      break;
                  }
                  AddListItem(propText, PHYSICAL);

                  if (pp->getDp2SortOrderPartFunc() != NULL)
                  {
                      propText = "Dp2 Sort Order Partitioning Function: ";
                      propText += pp->getDp2SortOrderPartFunc()->getText();
                      //AddListItem(propText, PHYSICAL);
                  }
              }// endif (sortKey.entries() > 0)

              //-------------------------------------------------------
              // GSH : Physical Prop -> partitioning ...
              //-------------------------------------------------------
              if (pp->getPartitioningFunction())
              {
                  propText = "partitioning function: ";
                  propText += pp->getPartitioningFunction()->getText();
                  AddListItem(propText, PHYSICAL);

                  NodeMap *nodeMapPtr =
                      (NodeMap *) (pp->getPartitioningFunction()->getNodeMap());

                  if (nodeMapPtr)
                  {
                      char cStr[100];

                      // Active partitions
                      sprintf(cStr, "Active partitions: %d",
                              nodeMapPtr->getNumActivePartitions());
                      propText = cStr;
                      AddListItem(propText, PHYSICAL);

                      // Data volumes
                      sprintf(cStr, "Data volumes: %d",
                              nodeMapPtr->getNumOfDP2Volumes());
                      propText = cStr;
                      AddListItem(propText, PHYSICAL);
                  }
              }//if(pp->getPartitioningFunction())
              //-------------------------------------------------------
              // GSH : Physical Prop -> location for plan execution ...
              //-------------------------------------------------------

              switch (pp->getPlanExecutionLocation())
              {
                case EXECUTE_IN_MASTER_AND_ESP:
                  propText = "can execute in master and ESP";
                  break;
                case EXECUTE_IN_MASTER:
                  propText = "can execute in master only";
                  break;
                case EXECUTE_IN_ESP:
                  propText = "can execute in ESP only";
                  break;
                case EXECUTE_IN_DP2:
                  propText = "executes in DP2";
                  break;
                default:
                  propText = "executes in ???";
                  break;
              }
              AddListItem(propText, PHYSICAL);

              //-------------------------------------------------------
              // GSH : Physical Prop -> Source for Partitioned data ...
              //-------------------------------------------------------
              switch (pp->getDataSourceEnum())
              {
                case SOURCE_PERSISTENT_TABLE:
                  propText = "source: persistent table";
                  break;
                case SOURCE_TEMPORARY_TABLE:
                  propText = "source: temporary table";
                  break;
                case SOURCE_TRANSIENT_TABLE:
                  propText = "source: transient table";
                  break;
                case SOURCE_VIRTUAL_TABLE:
                  propText = "source: virtual table";
                  break;
                case SOURCE_TUPLE:
                  propText = "source: tuple";
                  break;
                case SOURCE_ESP_DEPENDENT:
                  propText = "source: esp, dependent";
                  break;
                case SOURCE_ESP_INDEPENDENT:
                  propText = "source: esp, independent";
                  break;
                default:
                  break;
              }
              AddListItem(propText, PHYSICAL);

              //-------------------------------------------------------
              // GSH : Physical Prop -> Index data
              //-------------------------------------------------------
              if (pp->getIndexDesc())
              {
                  char cStr[100];

                  // Index levels
                  sprintf(cStr, "Index levels: %d",
                          pp->getIndexDesc()->getIndexLevels());
                  propText = cStr;
                  AddListItem(propText, PHYSICAL);

                  // Record length
                  sprintf(cStr, "Record length: %d",
                          pp->getIndexDesc()->getRecordLength());
                  propText = cStr;
                  AddListItem(propText, PHYSICAL);

                  // Record size in kb:
                  sprintf(cStr, "Record size in kb: %.4f",
                          pp->getIndexDesc()->getRecordSizeInKb().getValue());
                  propText = cStr;
                  AddListItem(propText, PHYSICAL);

                  // block size:
                  sprintf(cStr, "Block size in kb: %.4f",
                          pp->getIndexDesc()->getBlockSizeInKb().getValue());
                  propText = cStr;
                  AddListItem(propText, PHYSICAL);

                  // Records per blk:
                  sprintf(cStr, "Estimated records per blk: %.4f",
                          pp->getIndexDesc()->getEstimatedRecordsPerBlock().getValue());
                  propText = cStr;
                  AddListItem(propText, PHYSICAL);

              }//endif (pp->getIndexDesc())
          } // endif (pp != NULL)
          AddSeparatorLine(endstr);
      } // end if(m_displayPhysical)
      if (m_displayContext)
      {
          //add separator
          AddSeparatorLine(ui->m_Context->text() + ':', Qt::lightGray);

          if (m_plan != NULL && m_plan->getContext() != NULL)
          {
              propText =
                  "Context :" + m_plan->getContext()->getRequirementsString();
              AddListItem(propText, PropDialog::CONTEXT);

              // Now display each of its child contexts.
              Int32 nc = m_expr->getArity();
              for (Int32 i = 0; i < nc; i++)
              {
                  if (m_plan->getContextForChild(i) != NULL)
                  {
                      sprintf(validascii, "Context[%d] :", i);
                      propText = validascii;
                      propText +=
                          m_plan->getContextForChild(i)->
                          getRequirementsString();
                      AddListItem(propText, PropDialog::CONTEXT);
                  }
              }//for
          }
          AddSeparatorLine(endstr);
      }  // end if(m_displayContext)
      if (m_displayCost)
      {
          //add separator
          AddSeparatorLine(ui->m_Cost->text() + ':', Qt::lightGray);

          const Cost *rollUpCost, *operatorCost;
          if (m_plan != NULL)
          {
              operatorCost = m_plan->getOperatorCost();
              rollUpCost = m_plan->getRollUpCost();
          } 
          else
          {
              operatorCost = m_expr->castToRelExpr()->getOperatorCost();
              rollUpCost = m_expr->castToRelExpr()->getRollUpCost();
          }
          if (rollUpCost != NULL && operatorCost != NULL)
          {
              // Display generic data:
              NAString propText;
              char validascii[MAX_STR_LEN];
              // Display scan specific data:
              OperatorTypeEnum ot = m_expr->castToRelExpr()->getOperatorType();
              switch (ot)
              {
                 case REL_FILE_SCAN:
                 {
                    // Print the number plan fragments per cpu:
                    sprintf(validascii, "Blocks to read per DP2 access: %d",
                            ((FileScan *) (m_expr->
                                           castToRelExpr
                                           ()))->getNumberOfBlocksToReadPerAccess
                            ());
                    propText = validascii;
                    AddListItem(propText, COST);
                 }
               } // switch
              // Display cost class dependant info:
              addCost("**** Roll-up Cost ****", rollUpCost);
              addCost("**** Operator Cost ****", operatorCost);
          }  // if rollUpCost and operatorCost not null
          AddSeparatorLine(endstr);
      } // endif (m_displayCost)

      if (m_displayLogical)
      {
          //add separator
          AddSeparatorLine(ui->m_LogicalProp->text() + ':', Qt::lightGray);

          // QSTUFF VV
          // display logical property: stream
          sprintf(validascii, "Stream Expression: %s",
                  (ga->isStream()? "TRUE" : "FALSE"));
          propText = validascii;
          AddListItem(propText, LOGICAL);

          // display logical property: stream
          sprintf(validascii, "Embedded Delete or Embedded Update: %s",
                  (ga->isEmbeddedUpdateOrDelete()? "TRUE" : "FALSE"));
          propText = validascii;
          AddListItem(propText, LOGICAL);

          // display logical property: enbedded insert
          sprintf(validascii, "Embedded Insert: %s",
                  (ga->isEmbeddedInsert()? "TRUE" : "FALSE"));
          propText = validascii;
          AddListItem(propText, LOGICAL);

          // display logical property: stream
          sprintf(validascii, "Root of Generic Update Tree: %s",
                  (ga->isGenericUpdateRoot()? "TRUE" : "FALSE"));
          propText = validascii;
          AddListItem(propText, LOGICAL);
          // QSTUFF ^^

          // display logical property: has non-deterministic UDRs
          sprintf(validascii, "Has non-deterministic UDRs: %s",
                  (ga->getHasNonDeterministicUDRs()? "TRUE" : "FALSE"));
          propText = validascii;
          AddListItem(propText, LOGICAL);

          CollIndex numEntries = ga->getInputLogPropList().entries();
          for (CollIndex i = 0; i < numEntries; i++)
          {
              if (i > 0)
              {
                  sprintf(validascii, "********************************");
                  propText = validascii;
                  AddListItem(propText, LOGICAL);
              }
              sprintf(validascii,
                      "Input Est. Log. Props: #%d; Estimated Rows = %.4f", i,
                      ga->getInputLogPropList()[i]->getResultCardinality().
                      getValue());
              propText = validascii;
              AddListItem(propText, LOGICAL);

              displayEstLogProp(ga->getInputLogPropList()[i]);

              sprintf(validascii, "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
              propText = validascii;
              AddListItem(propText, LOGICAL);

              sprintf(validascii,
                      "Output Est. Log. Props: #%d; Estimated Rows = %.4f"
                      " Max cardinality = %.4f", i,
                      ga->getOutputLogPropList()[i]->getResultCardinality().
                      getValue(),
                      ga->getOutputLogPropList()[i]->getMaxCardEst().
                      getValue());
              propText = validascii;
              AddListItem(propText, LOGICAL);

              displayEstLogProp(ga->getOutputLogPropList()[i]);
           } // end for
           AddSeparatorLine(endstr);
      }   //endif (m_displayLogical)
      // all expressions could have this
      if (m_displayASM)
      {
          //add separator
          AddSeparatorLine(ui->m_ASM->text() + ':', Qt::lightGray);
          // iterate through list and call displayEstLogProp;
#ifdef DONT_DO_THIS
          QueryAnalysis *QA = m_analysis;
          AppliedStatMan *ASMptr = (QA == NULL) ? NULL : QA->getASM();
          GroupAnalysis *groupAnalysis = ga->getGroupAnalysis();
          validascii[0] = '\0'; // just in case
          if (groupAnalysis)
          {
              const JBBSubset *jbbSubsetForThisGroup =
                  groupAnalysis->getLocalJBBView();
              if (jbbSubsetForThisGroup)
              {
                  if (ASMptr)
                  {
                     EstLogPropSharedPtr ASM_elp =
                          ASMptr->getStatsForJBBSubset(*jbbSubsetForThisGroup);
                     if (ASM_elp)
                        displayEstLogProp(ASM_elp);
                     else
                        sprintf(validascii, "No ASM_elp information.");
                   } 
                   else
                     sprintf(validascii, "No ASMptr information.");
               } 
               else
                 sprintf(validascii, "No jbbSubsetForThisGroup information.");
           } 
           else
             sprintf(validascii, "No groupAnalysis information.");

          propText = validascii;
          AddListItem(propText, ASM);
#endif
// list of EstLogProps, display them all

          SHPTR_LIST(EstLogPropSharedPtr) * statsList =
              ga->getCachedStatsList();
          Int32 entries = statsList->entries();

          for (Int32 i = 0; i < entries; i++)
          {
              EstLogPropSharedPtr elpPtr = (*statsList)[i];
              displayEstLogProp(elpPtr);
          }
          AddSeparatorLine(endstr);
      } // m_displayASM
      if (m_displayGroupAnalysis)
      {
          //add separator
          AddSeparatorLine(ui->m_GroupAnalysis->text() + ':', Qt::lightGray);
          GroupAnalysis *GA = ga->getGroupAnalysis();
          if (GA)
          {
              const NAString & GAText = GA->getText();
              NAString oneline(GAText);
              NAString tempascii;
              size_t idx;
              BOOL done = FALSE;
// Newline delimited text.  Need to split it up and add one line at a time
              idx = oneline.first('\n');
              if (idx == NA_NPOS && oneline.length() > 0)
              {
// it could just be one line
                  sprintf(validascii, oneline);
                  AddListItem(validascii, GROUPANALYSIS);
              } 
              else if (idx == NA_NPOS && oneline.length() == 0)
              {
                  sprintf(validascii, "GroupAnalysis getText is NULL!");
                  AddListItem(validascii, GROUPANALYSIS);
              }
              while (idx != NA_NPOS && !done)
              {
                  // find first newline if any
                  if (idx == NA_NPOS)
                  {
                      done = TRUE;
// still add the line, it may not be newline terminated
// only add if there is something there
                      if (oneline.length() > 0)
                      {
                          AddListItem(oneline, GROUPANALYSIS);
                          oneline.remove(0);  // empty list
                      }
                  } 
                  else
                  {
                      tempascii.remove(0);
                      tempascii.append(oneline, idx);
                      AddListItem(tempascii, GROUPANALYSIS);
                      oneline.remove(0, idx + 1); // remove newline too
                  }
                  idx = oneline.first('\n');
                }//while
            } 
            else
            {
              sprintf(validascii, "GroupAnalysis is NULL!");
              propText = validascii;
              AddListItem(propText, GROUPANALYSIS);
            }
            AddSeparatorLine(endstr);
      } //(m_displayGroupAnalysis)
      if (m_displayTableAnalysis)
      {
          //add separator
          AddSeparatorLine(ui->m_TableAnalysis->text() + ':', Qt::lightGray);

          GroupAnalysis *GA = ga->getGroupAnalysis();
          NodeAnalysis *NA = GA->getNodeAnalysis();
          if (NA)
          {
              TableAnalysis *TA = NA->getTableAnalysis();
              if (TA)
              {
                  const NAString & TAText = TA->getText();
                  NAString oneline(TAText);
                  NAString tempascii;
                  size_t idx;
                  BOOL done = FALSE;
// Newline delimited text.  Need to split it up and add one line at a time
                  idx = oneline.first('\n');
                  if (idx == NA_NPOS && oneline.length() > 0)
                  {
// it could just be one line
                      sprintf(validascii, oneline);
                      AddListItem(validascii, TABLEANALYSIS);
                  } 
                  else if (idx == NA_NPOS && oneline.length() == 0)
                  {
                      sprintf(validascii, "TableAnalysis getText is NULL!");
                      AddListItem(validascii, TABLEANALYSIS);
                  }
                  while (idx != NA_NPOS && !done)
                  {
                      // find first newline if any
                      if (idx == NA_NPOS)
                      {
                          done = TRUE;
// still add the line, it may not be newline terminated
// only add if there is something there
                          if (oneline.length() > 0)
                          {
                              AddListItem(oneline, TABLEANALYSIS);
                              oneline.remove(0);  // empty list
                          }
                       } 
                       else
                        {
                          tempascii.remove(0);
                          tempascii.append(oneline, idx);
                          AddListItem(tempascii, TABLEANALYSIS);
                          oneline.remove(0, idx + 1); // remove newline too
                        }
                      idx = oneline.first('\n');
                    }
                } 
                else
                {
                  sprintf(validascii, "TableAnalysis is NULL!");
                  propText = validascii;
                  AddListItem(propText, TABLEANALYSIS);
                }
            } 
            else
            {
              sprintf(validascii, "TableAnalysis NodeAnalysis is NULL!");
              propText = validascii;
              AddListItem(propText, TABLEANALYSIS);
            }
            AddSeparatorLine(endstr);
      }  // tableanalysis
      if (m_displayJBBC)
      {
          //add separator
          AddSeparatorLine(ui->m_JBBC->text() + ':', Qt::lightGray);

          // getGroupAnalysis then getJBBCs?  How to display textually?
          GroupAnalysis *GAptr = ga->getGroupAnalysis();
          NodeAnalysis *NAptr = GAptr->getNodeAnalysis();
          if (NAptr)
          {
              JBBC *jbbcptr = NAptr->getJBBC();
              if (jbbcptr)
              {
                  const NAString & JBBCText = jbbcptr->getText();
                  NAString oneline(JBBCText);
                  NAString tempascii;
                  size_t idx;
                  BOOL done = FALSE;
// Newline delimited text.  Need to split it up and add one line at a time
                  idx = oneline.first('\n');
                  if (idx == NA_NPOS && oneline.length() > 0)
                  {
// it could just be one line
                      sprintf(validascii, oneline);
                      AddListItem(validascii, JBBC_PT);
                  } 
                  else if (idx == NA_NPOS && oneline.length() == 0)
                  {
                      sprintf(validascii, "jbbcptr getText is NULL!");
                      AddListItem(validascii, JBBC_PT);
                  }
                  while (idx != NA_NPOS && !done)
                  {
                      // find first newline if any
                      if (idx == NA_NPOS)
                      {
                          done = TRUE;
// still add the line, it may not be newline terminated
// only add if there is something there
                          if (oneline.length() > 0)
                          {
                              AddListItem(oneline, JBBC_PT);
                              oneline.remove(0);  // empty list
                            }
                      } 
                      else
                      {
                          tempascii.remove(0);
                          tempascii.append(oneline, idx);
                          AddListItem(tempascii, JBBC_PT);
                          oneline.remove(0, idx + 1); // remove newline too
                      }
                      idx = oneline.first('\n');
                    }
                } 
                else
                {
                  sprintf(validascii, "JBBC jbbc is NULL!");
                  propText = validascii;
                  AddListItem(propText, JBBC_PT);
                }
            } 
            else
            {
              sprintf(validascii, "JBBC NodeAnalysis is NULL!");
              propText = validascii;
              AddListItem(propText, JBBC_PT);
            }
            AddSeparatorLine(endstr);
      }                       // JBBC
      if (m_displayQueryGraph)
      {
          //add separator
          AddSeparatorLine(ui->m_QueryGraph->text() + ':', Qt::lightGray);
          GroupAnalysis *GAptr = ga->getGroupAnalysis();
          NodeAnalysis *NAptr = GAptr->getNodeAnalysis();
          if (NAptr)
          {
              JBBC *jbbcptr = NAptr->getJBBC();
              if (jbbcptr)
                {
                  //const NAString &QGNodeText = jbbcptr->getQueryGraphNodeText(); //ksremove
                  //NAString oneline(QGNodeText); //ksremove
                  NAString oneline("Query Graph Text\n");
                  NAString tempascii;
                  size_t idx;
                  BOOL done = FALSE;
                  // Newline delimited text.  Need to split it up and add one line at a time
                  idx = oneline.first('\n');
                  if (idx == NA_NPOS && oneline.length() > 0)
                    {
                      // it could just be one line
                      sprintf(validascii, oneline);
                      AddListItem(validascii, QGRAPH);
                  } else if (idx == NA_NPOS && oneline.length() == 0)
                    {
                      sprintf(validascii,
                              "QueryGraph jbbcptr->getQueryGraphNodeText is NULL!");
                      AddListItem(validascii, QGRAPH);
                    }
                  while (idx != NA_NPOS && !done)
                    {
                      // find first newline if any
                      if (idx == NA_NPOS)
                        {
                          done = TRUE;
                          // still add the line, it may not be newline terminated
                          // only add if there is something there
                          if (oneline.length() > 0)
                            {
                              AddListItem(oneline, QGRAPH);
                              oneline.remove(0);  // empty list
                            }
                        } else
                        {
                          tempascii.remove(0);
                          tempascii.append(oneline, idx);
                          AddListItem(tempascii, QGRAPH);
                          oneline.remove(0, idx + 1); // remove newline too
                        }
                      idx = oneline.first('\n');
                    }
                } else
                {
                  sprintf(validascii, "QG jbbc is NULL!");
                  AddListItem(validascii, QGRAPH);
                }
            } else
            {
              sprintf(validascii, "QG NodeAnalysis is NULL!");
              AddListItem(validascii, QGRAPH);
            }
            AddSeparatorLine(endstr);
      }                       // QueryGraph
      if (m_displayCascadesTraceInfo)
      {
          //add separator
          AddSeparatorLine(ui->m_CascadesTrcInfo->text() + ':', Qt::lightGray);

          RelExpr *relexp = m_expr->castToRelExpr();
          if (relexp)
            {
              const NAString & TaskText = relexp->getCascadesTraceInfoStr();
              NAString oneline(TaskText);
              NAString tempascii;
              size_t idx;
              BOOL done = FALSE;
              // Newline delimited text.  Need to split it up and add one line at a time
              idx = oneline.first('\n');
              if (idx == NA_NPOS && oneline.length() > 0)
                {
              // it could just be one line
                  sprintf(validascii, oneline);
                  AddListItem(validascii, CASCADESTRACEINFO);
              } else if (idx == NA_NPOS && oneline.length() == 0)
                {
                  sprintf(validascii, "Cascades Trace info getText is NULL!");
                  AddListItem(validascii, CASCADESTRACEINFO);
                }
              while (idx != NA_NPOS && !done)
                {
                  // find first newline if any
                  if (idx == NA_NPOS)
                    {
                      done = TRUE;
                      // still add the line, it may not be newline terminated
                      // only add if there is something there
                      if (oneline.length() > 0)
                        {
                          AddListItem(oneline, CASCADESTRACEINFO);
                          oneline.remove(0);  // empty list
                        }
                    } else
                    {
                      tempascii.remove(0);
                      tempascii.append(oneline, idx);
                      AddListItem(tempascii, CASCADESTRACEINFO);
                      oneline.remove(0, idx + 1); // remove newline too
                    }
                  idx = oneline.first('\n');
                }
            } else
            {
              sprintf(validascii, "RelExpr is NULL!");
              propText = validascii;
              AddListItem(propText, CASCADESTRACEINFO);
            }
            AddSeparatorLine(endstr);
        }//if (m_displayCascadesTraceInfo)
    }  // ga != NULL
}

void PropDialog::FreezeDisplay()
{
  ui->m_GrpBox->setEnabled(false);
  ui->m_ClrAll->setEnabled(false);
  ui->m_ShowAll->setEnabled(false);
}

void PropDialog::on_m_ClrAll_clicked()
{
  m_displayInputs = FALSE;
  m_displayOutputs = FALSE;
  m_displayEssOutputs = FALSE;
  m_displayConstraints = FALSE;
  m_displayCost = FALSE;
  m_displayLogical = FALSE;
  m_displayPhysical = FALSE;
  m_displayContext = FALSE;
  m_displayASM = FALSE;
  m_displayJBBC = FALSE;
  m_displayGroupAnalysis = FALSE;
  m_displayTableAnalysis = FALSE;
  m_displayQueryGraph = FALSE;
  m_displayCascadesTraceInfo = FALSE;
  m_displayWizDirect = FALSE;

  ui->m_CharInputs->setChecked(false);
  ui->m_CharOutputs->setChecked(false);
  ui->m_EssOutputs->setChecked(false);
  ui->m_Constraints->setChecked(false);
  ui->m_Cost->setChecked(false);
  ui->m_LogicalProp->setChecked(false);
  ui->m_PhysicalProp->setChecked(false);
  ui->m_Context->setChecked(false);
  ui->m_ASM->setChecked(false);
  ui->m_GroupAnalysis->setChecked(false);
  ui->m_JBBC->setChecked(false);
  ui->m_TableAnalysis->setChecked(false);
  ui->m_QueryGraph->setChecked(false);
  ui->m_CascadesTrcInfo->setChecked(false);
  ui->m_WizDirect->setChecked(false);

  UpdatePropList();
}

void PropDialog::on_m_ShowAll_clicked()
{
  m_displayInputs = TRUE;
  m_displayOutputs = TRUE;
  m_displayEssOutputs = TRUE;
  m_displayConstraints = TRUE;
  m_displayCost = TRUE;
  m_displayLogical = TRUE;
  m_displayPhysical = TRUE;
  m_displayContext = TRUE;
  m_displayASM = TRUE;
  m_displayJBBC = TRUE;
  m_displayGroupAnalysis = TRUE;
  m_displayTableAnalysis = TRUE;
  m_displayQueryGraph = TRUE;
  m_displayCascadesTraceInfo = TRUE;
  m_displayWizDirect = TRUE;

  ui->m_CharInputs->setChecked(true);
  ui->m_CharOutputs->setChecked(true);
  ui->m_EssOutputs->setChecked(true);
  ui->m_Constraints->setChecked(true);
  ui->m_Cost->setChecked(true);
  ui->m_LogicalProp->setChecked(true);
  ui->m_PhysicalProp->setChecked(true);
  ui->m_Context->setChecked(true);
  ui->m_ASM->setChecked(true);
  ui->m_GroupAnalysis->setChecked(true);
  ui->m_JBBC->setChecked(true);
  ui->m_TableAnalysis->setChecked(true);
  ui->m_QueryGraph->setChecked(true);
  ui->m_CascadesTrcInfo->setChecked(true);
  ui->m_WizDirect->setChecked(true);

  UpdatePropList();
}

void PropDialog::on_m_OK_clicked()
{
   QObject* obj = parent();
   if(0==strcmp(obj->metaObject()->className(), "QMdiSubWindow"))
     ((QMdiSubWindow *)obj)->close();
}

void PropDialog::on_m_CharInputs_toggled()
{
  if (ui->m_CharInputs->isChecked())
    m_displayInputs = TRUE;
    else
    m_displayInputs = FALSE;
  UpdatePropList();
}

void PropDialog::on_m_CharOutputs_toggled()
{
  if (ui->m_CharOutputs->isChecked())
    m_displayOutputs = TRUE;
    else
    m_displayOutputs = FALSE;
  UpdatePropList();
}

void PropDialog::on_m_EssOutputs_toggled()
{
  if (ui->m_EssOutputs->isChecked())
    m_displayEssOutputs = TRUE;
    else
    m_displayEssOutputs = FALSE;
  UpdatePropList();
}

void PropDialog::on_m_Constraints_toggled()
{
  if (ui->m_Constraints->isChecked())
    m_displayConstraints = TRUE;
    else
    m_displayConstraints = FALSE;
  UpdatePropList();
}

void PropDialog::on_m_Cost_toggled()
{
  if (ui->m_Cost->isChecked())
    m_displayCost = TRUE;
    else
    m_displayCost = FALSE;
  UpdatePropList();
}

void PropDialog::on_m_LogicalProp_toggled()
{
  if (ui->m_LogicalProp->isChecked())
    m_displayLogical = TRUE;
    else
    m_displayLogical = FALSE;
  UpdatePropList();
}

void PropDialog::on_m_PhysicalProp_toggled()
{
  if (ui->m_PhysicalProp->isChecked())
    m_displayPhysical = TRUE;
    else
    m_displayPhysical = FALSE;
  UpdatePropList();
}

void PropDialog::on_m_Context_toggled()
{
  if (ui->m_Context->isChecked())
    m_displayContext = TRUE;
    else
    m_displayContext = FALSE;
  UpdatePropList();
}

void PropDialog::on_m_ASM_toggled()
{
  if (ui->m_ASM->isChecked())
    m_displayASM = TRUE;
    else
    m_displayASM = FALSE;
  UpdatePropList();
}

void PropDialog::on_m_GroupAnalysis_toggled()
{
  if (ui->m_GroupAnalysis->isChecked())
    m_displayGroupAnalysis = TRUE;
    else
    m_displayGroupAnalysis = FALSE;
  UpdatePropList();
}

void PropDialog::on_m_JBBC_toggled()
{
  if (ui->m_JBBC->isChecked())
    m_displayJBBC = TRUE;
    else
    m_displayJBBC = FALSE;
  UpdatePropList();
}

void PropDialog::on_m_TableAnalysis_toggled()
{
  if (ui->m_TableAnalysis->isChecked())
    m_displayTableAnalysis = TRUE;
    else
    m_displayTableAnalysis = FALSE;
  UpdatePropList();
}

void PropDialog::on_m_QueryGraph_toggled()
{
  if (ui->m_QueryGraph->isChecked())
    m_displayQueryGraph = TRUE;
    else
    m_displayQueryGraph = FALSE;
  UpdatePropList();
}

void PropDialog::on_m_CascadesTrcInfo_toggled()
{
  if (ui->m_CascadesTrcInfo->isChecked())
    m_displayCascadesTraceInfo = TRUE;
    else
    m_displayCascadesTraceInfo = FALSE;
  UpdatePropList();
}

void PropDialog::on_m_WizDirect_toggled()
{
  if (ui->m_WizDirect->isChecked())
    m_displayWizDirect = TRUE;
    else
    m_displayWizDirect = FALSE;
  UpdatePropList();
}
