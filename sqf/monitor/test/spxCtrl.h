#ifndef SPX_CTRL_H_
#define SPX_CTRL_H_

typedef enum { CMD_GET_STATUS, CMD_END} spxProcCmds_t;

typedef struct spxmessage_def
{
    int noticeCount;
} replyMsg_t;

#endif // SPX_CTRL_H_
