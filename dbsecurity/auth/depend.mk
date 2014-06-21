# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2010-2014 Hewlett-Packard Development Company, L.P.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
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
