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

$(OUTDIR)/verssqauth.o: ../shared/inc/verslib.h
$(OUTDIR)/verssqauth.o: $(INCEXPDIR)/SCMBuildStr.h
$(OUTDIR)/verssqauth.o: $(INCEXPDIR)/SCMVersHelp.h

$(OUTDIR)/dbUserAuth.o: inc/dbUserAuth.h

$(OUTDIR)/dbUserAuth.o: inc/auth.h
$(OUTDIR)/dbUserAuth.o: inc/token.h
$(OUTDIR)/dbUserAuth.o: inc/tokenkey.h
$(OUTDIR)/dbUserAuth.o: inc/ldapconfignode.h
$(OUTDIR)/dbUserAuth.o: inc/ld_globals.h

$(OUTDIR)/ldapconfignode.o: inc/ldapconfignode.h
$(OUTDIR)/ldapconfignode.o: inc/ld_globals.h

$(OUTDIR)/ld_port.o: inc/ld_globals.h

$(OUTDIR)/token.o: inc/token.h
$(OUTDIR)/token.o: inc/tokenkey.h

$(OUTDIR)/tokenkey.o: inc/tokenkey.h

$(OUTDIR)/verssqauthlf.o: ../shared/inc/verslib.h
$(OUTDIR)/verssqauthlf.o: $(INCEXPDIR)/SCMBuildStr.h
$(OUTDIR)/verssqauthlf.o: $(INCEXPDIR)/SCMVersHelp.h
