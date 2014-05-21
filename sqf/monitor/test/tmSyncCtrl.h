#ifndef TMSYNCCTRL_H_
#define TMSYNCCTRL_H_

typedef struct tmSyncResults_s
{
    int nid; int pid; int testNum;
    int transStart; int transAbort; int transCommit; int transCount;
} tmSyncResults_t;

#endif // TMSYNCCTRL_H_
