# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2011-2014 Hewlett-Packard Development Company, L.P.
#
# @@@ END COPYRIGHT @@@
#
# DO NOT DELETE
 
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2010-2014 Hewlett-Packard Development Company, L.P.
#
# @@@ END COPYRIGHT @@@
#

$(OUTDIR)/secsrvr.o: inc/secsrvrmxo.h
$(OUTDIR)/secsrvr.o: src/secsrvr.h

$(OUTDIR)/verssqcert.o: ../shared/inc/verslib.h
$(OUTDIR)/verssqcert.o: $(INCEXPDIR)/SCMBuildStr.h
$(OUTDIR)/verssqcert.o: $(INCEXPDIR)/SCMVersHelp.h
