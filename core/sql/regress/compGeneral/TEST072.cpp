#include "sqludr.h"

SQLUDR_LIBFUNC SQLUDR_INT32 add2(SQLUDR_INT32 *in1,
                                 SQLUDR_INT32 *in2,
                                 SQLUDR_INT32 *out1,
                                 SQLUDR_INT16 *inInd1,
                                 SQLUDR_INT16 *inInd2,
                                 SQLUDR_INT16 *outInd1,
                                 SQLUDR_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;
  if (SQLUDR_GETNULLIND(inInd1) == SQLUDR_NULL ||
      SQLUDR_GETNULLIND(inInd2) == SQLUDR_NULL)
    SQLUDR_SETNULLIND(outInd1);
  else
    (*out1) = (*in1) + (*in2);
  return SQLUDR_SUCCESS;
}

