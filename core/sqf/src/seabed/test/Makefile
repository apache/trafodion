# @@@ START COPYRIGHT @@@
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
# @@@ END COPYRIGHT @@@

CHK		= $(OUTDIR) tcheck testfiles
PROGS		= t2 t3 t4 t5 t6 t9 t14 t15 t16 t17 t18 t19 \
		  t20 t21 t22 t23 t24 t25     t27 \
		  t30 t31 t32         t35 t36 t37 t38 t39 \
		  t40 t41 t42 t43 t44             t48 t49 \
		  t50     t52             t56 t57 t58 \
		      t61 t62 t63 t64 t65 t66 t67 t68 t69 \
		  t70 t71 t72 t73 t74 t75 t76 t77 t78 t79 \
		  t80         t83 t84 t85 t86     t88 \
		      t91 t92 t93 t94     t96     t98 \
		  t100 t101 t102 t103 t104                          \
		                           t115           t118      \
		                      t124           t127 t128 t129 \
		  t130 t131 t132 t133      t135      t137 t138 t139 \
		       t141 t142           t145                t149 \
		       t151 t152 t153      t155 t156 t157           \
		  t160 t161 t162 t163 t164 t165      t167 t168 t169 \
		  t170      t172                t176                \
		  t180      t182 t183 t184 t185 t186 t187 t188 \
		  t190      t192      t194 t195      t197 t198 \
		       t201 t202           t205 t206 t207 t208 t209 \
		  t210      t212 t213 t214 t215 t216 t217      t219 \
		  t220 t221 t222                t226 t227 t228 \
		       t231      t233 t234 t235           t238      \
		  t240 t241 t242           t245                \
		                 t253                               \
		            t262                                    \
		                                     t277      t279 \
		  t280 t281 t282      t284      t286 t287\
		  tcheck tdelta tmerge tsock ttee
PROGSI		= t2fs t3ms t4ms \
		  t5ms t6ms t9thread t14ms \
		  t15ms t16ms t17cli t17srv t17svc t18cli t18srv t19ms \
		  t20srv t20c1 t20c2 t21ms t22ms t23cli t23dtm t23tse \
		  t24cli t24srv \
		  t25cli t25srv t27cli t27srv \
		  t30cli t30srv t31fs t32fsx \
		  t35ms t36ms t37fs t38fs t39fs \
		  t40fs t41cli t41srv t42cli t42srv t43ms t44ms \
		  t48fs t49cli t49srv \
		  t50fs t51ms t52ms t56fs t57ms t58ms \
		  t61ms t62fs t63ms t64ms \
		  t65fs t66ms t67fs t68ms t69ms \
		  t70cli t70srv t71ms t72fs t73ms t74ms \
		  t75ms t76fs t77fs t78fs t79fs \
		  t80fs t83fs t84fs t85ms t86ms t88ms \
		  t91ms t92ms t93ms t94fs t96ms t98ms \
		  t100ms t101fs t102fs t103fs t104ms \
		  t105ms \
		  t115ms t118ms \
		  t124fs \
		  t127ms t128fs t129ms \
		  t130map t131ms t132ms t133cli t133srv \
		  t135ms t137ms t138ms t139cl \
		  t141ms t142cli t142srv \
		  t145ms t149ms \
		  t151ms t152cli t152srv t153fs \
		  t155ms t156fs t157ms \
		  t160ms t161ms t162fs t163ms t164fs \
		  t165fs t167ms t168cli t168srv t169cli t169srv \
		  t170ms t172ms \
		  t176fs \
		  t180ms t182fs t183ms t184fs \
		  t185fs t186fs t187ms t188ms \
		  t190fs t192fs t194fs \
		  t195fs t197fs t198fs \
		  t201ms t202ms t202s \
		  t205fs t206fs t207fs t208fs t209ms \
		  t210ms t212ms t213fs t214fs \
		  t215fs t216ms t217fs t219ms \
		  t220fs t221ms t222ms \
		  t226fs t227fs t228ms \
		  t231fs \
		  t233ms t234ms \
		  t235ms t238ms \
		  t240ms t241ms t242fs \
		  t245ms \
		  t253ms \
		  t261ms t262ms \
		  t277fs t279ms \
		  t280fs t281ms t282ms t284cli t284srv

###########################################################################

include ../../../macros.gmk # top level
include ../../macros.gmk
include ../macros.gmk
# overwrite
INCLUDES	= -I$(INCEXPDIR)
COVER_FLGS      =

# possible defines
#   DEBUG
DEBUG		=
DEFINES		= -D_MPICC_H -DDEBUG

# Tests always need assert
CXXFLAGS       += -UNDEBUG
INCLUDESX	= $(INCLUDES)
INCLUDESJNIX	= $(INCLUDES) -I$(OUTDIR) -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux
INCLUDESMONX	= $(INCLUDES) -I$(INCMONDIR)
INCLUDESSRCX	= -I../src $(INCLUDESX)
LIBS		= $(LIBTMSUTIL) \
		  $(LIBTUTIL) \
		  $(LIBTUTILP)
LIBTMSUTIL	= $(LIBEXPDIR)/libsbztmsfsutil.so
LIBTMSUTILX	= -L$(LIBEXPDIR) -lsbztmsfsutil
LIBTUTIL	= $(LIBEXPDIR)/libsbztutil.so
LIBTUTILX	= -L$(LIBEXPDIR) -lsbztutil
LIBTUTILP	= $(LIBEXPDIR)/libsbztutilp.so
LIBTUTILPX	= -L$(LIBEXPDIR) -lsbztutilp
LINKITCXX	= $(CXX) $(LNK_FLGS) $(LINKSYMBOLS)
LINKSYMBOLS	= -g3
OBJTMSUTIL	= $(OUTDIR)/tmsfsutil.o
OBJTUTIL	= $(OUTDIR)/tutil.o
OBJTUTILP	= $(OUTDIR)/targ.o \
		  $(OUTDIR)/thook.o \
		  $(OUTDIR)/tstats.o
PURGE		= *.a *.so $(OUTDIR)/*.o $(OUTDIR)/com/hp/traf/*.class $(LIBS) $(PROGS) $(PROGSI) \
                  $(OUTDIR)/*.gcda $(OUTDIR)/*.gcno *.gcno $(OUTDIR)/*.gcov \
		  c core.* dgt* m monitor.* s \
		  $(OUTDIR)/com_hp_traf_t284cli.h \
		  test.status \
		  z70.*file z136sdp-port z204 \
		  zjunk zos zrpc zns
# rule macros
RCXXINCLUDES	= $(CXX) $(CDEPFLAGS) $(CXXFLAGS) $(INCLUDESX) -o $@ -c $<
RCXXINCLUDESSRC	= $(CXX) $(CDEPFLAGS) $(CXXFLAGS) $(INCLUDESSRCX) -o $@ -c $<
RFS		= $(LIBTMSUTILX) $(LIBTUTILX) $(LIBTUTILPX) $(LIBSBUTILX) $(LIBSBFSX) $(DEBUG)
RMS		= $(LIBTMSUTILX) $(LIBTUTILX) $(LIBTUTILPX) $(LIBSBUTILX) $(LIBSBMSX) $(DEBUG)
RMSEG		= $(LIBSBMSX) $(DEBUG)
RMSUTP		= $(LIBSBMSX) $(LIBTUTILPX) $(LIBSBUTILX) $(DEBUG)
RMX		= $(LIBTUTILX) $(LIBTUTILPX) $(LIBSBUTILX) $(DEBUG)
# target macros
TFS		= $(LIBTMSUTIL) $(LIBTUTIL) $(LIBTUTILP) $(LIBSBFS)
TMS		= $(LIBTMSUTIL) $(LIBTUTIL) $(LIBTUTILP) $(LIBSBMS)
TMSEG		= $(LIBSBMS)
TMSUTP		= $(LIBSBMS) $(LIBTUTILP)
TMX		= $(LIBTUTIL) $(LIBTUTILP)

# Targets
# Uncomment the following target to disable parallel make
.NOTPARALLEL:

all: $(PROGS)

t2: $(CHK) t2fs
t2fs: $(OUTDIR)/t2fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

zz: $(OUTDIR)/zz.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t3: $(CHK) t3ms
t3ms: $(OUTDIR)/t3ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t3x: $(CHK) t3msx
t3msx: $(OUTDIR)/t3msx.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t4: $(CHK) t4ms
t4ms: $(OUTDIR)/t4ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t5: $(CHK) t5ms
t5ms: $(OUTDIR)/t5ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t6: $(CHK) t6ms
t6ms: $(OUTDIR)/t6ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t9: $(CHK) t9thread
t9thread: $(OUTDIR)/t9thread.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)
$(OUTDIR)/t9thread.o: t9thread.cpp
	$(RCXXINCLUDESSRC)

t14: $(CHK) t14ms
t14ms: $(OUTDIR)/t14ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)
$(OUTDIR)/t14ms.o: t14ms.cpp
	$(RCXXINCLUDESSRC)

t15: $(CHK) t15ms
t15ms: $(OUTDIR)/t15ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)
$(OUTDIR)/t15ms.o: t15ms.cpp
	$(RCXXINCLUDESSRC)

t16: $(CHK) t16ms
t16ms: $(OUTDIR)/t16ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)
$(OUTDIR)/t16ms.o: t16ms.cpp
	$(RCXXINCLUDESSRC)

t17: $(CHK) t17cli t17srv t17svc
t17cli: $(OUTDIR)/t17cli.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t17srv: $(OUTDIR)/t17srv.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t17svc: $(OUTDIR)/t17svc.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t18: $(CHK) t18cli t18srv
t18cli: $(OUTDIR)/t18cli.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t18srv: $(OUTDIR)/t18srv.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t19: $(CHK) t19ms
t19ms: $(OUTDIR)/t19ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t20: $(CHK) t20srv t20c1 t20c2
t20c1: $(OUTDIR)/t20cli.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t20c2: $(OUTDIR)/t20cli.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t20srv: $(OUTDIR)/t20srv.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t21: $(CHK) t21ms
t21ms: $(OUTDIR)/t21ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t22: $(CHK) t22ms
t22ms: $(OUTDIR)/t22ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t23: $(CHK) t23cli t23dtm t23tse
t23cli: $(OUTDIR)/t23cli.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t23dtm: $(OUTDIR)/t23dtm.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t23tse: $(OUTDIR)/t23tse.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t24: $(CHK) t24cli t24srv
t24cli: $(OUTDIR)/t24cli.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t24srv: $(OUTDIR)/t24srv.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t25: $(CHK) t25cli t25srv
t25cli: $(OUTDIR)/t25cli.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t25srv: $(OUTDIR)/t25srv.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t27: $(CHK) t27cli t27srv
t27cli: $(OUTDIR)/t27ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t27srv: $(OUTDIR)/t27ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t30: $(CHK) t30cli t30srv
t30cli: $(OUTDIR)/t30cli.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t30srv: $(OUTDIR)/t30srv.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t31: $(CHK) t31fs
t31fs: $(OUTDIR)/t31fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t32: $(CHK) t32fsx
t32fsx: $(OUTDIR)/t32fsx.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t35: $(CHK) t35ms
t35ms: $(OUTDIR)/t35ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t36: $(CHK) t36ms
t36ms: $(OUTDIR)/t36ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t37: $(CHK) t37fs
t37fs: $(OUTDIR)/t37fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t38: $(CHK) t38fs
t38fs: $(OUTDIR)/t38fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t39: $(CHK) t39fs
t39fs: $(OUTDIR)/t39fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t40: $(CHK) t40fs
t40fs: $(OUTDIR)/t40fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t41: $(CHK) t41cli t41srv
t41cli: $(OUTDIR)/t41cli.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t41srv: $(OUTDIR)/t41srv.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t42: $(CHK) t42cli t42srv
t42cli: $(OUTDIR)/t42cli.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t42srv: $(OUTDIR)/t42srv.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t43: $(CHK) t43ms
t43ms: $(OUTDIR)/t43ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t44: $(CHK) t44ms
t44ms: $(OUTDIR)/t44ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t45: $(CHK) t45fs
t45fs: $(OUTDIR)/t45fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t48: $(CHK) t48fs
t48fs: $(OUTDIR)/t48fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t49: $(CHK) t49cli t49srv
t49cli: $(OUTDIR)/t49fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t49srv: $(OUTDIR)/t49fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t50: $(CHK) t50fs
t50fs: $(OUTDIR)/t50fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t51: $(CHK) t51ms
t51ms: $(OUTDIR)/t51ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)
$(OUTDIR)/t51ms.o: t51ms.cpp
	$(RCXXINCLUDESSRC)

t52: $(CHK) t52ms
t52ms: $(OUTDIR)/t52ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t53: $(CHK) t53fs
t53fs: $(OUTDIR)/t53fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t54: $(CHK) t54fs
t54fs: $(OUTDIR)/t54fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t55: $(CHK) t55fs
t55fs: $(OUTDIR)/t55fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t56: $(CHK) t56fs
t56fs: $(OUTDIR)/t56fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t57: $(CHK) t57ms
t57ms: $(OUTDIR)/t57ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t58: $(CHK) t58ms
t58ms: $(OUTDIR)/t58ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t61: $(CHK) t61ms
t61ms: $(OUTDIR)/t61ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t62: $(CHK) t62fs
t62fs: $(OUTDIR)/t62fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t63: $(CHK) t63ms
t63ms: $(OUTDIR)/t63ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t64: $(CHK) t64ms
t64ms: $(OUTDIR)/t64ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t65: $(CHK) t65fs
t65fs: $(OUTDIR)/t65fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t66: $(CHK) t66ms
t66ms: $(OUTDIR)/t66ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t67: $(CHK) t67fs
t67fs: $(OUTDIR)/t67fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t68: $(CHK) t68ms
t68ms: $(OUTDIR)/t68ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t69: $(CHK) t69ms
t69ms: $(OUTDIR)/t69ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t70: $(CHK) t70cli t70srv
t70cli: $(OUTDIR)/t70cli.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t70srv: $(OUTDIR)/t70srv.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t71: $(CHK) t71ms
t71ms: $(OUTDIR)/t71ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t72: $(CHK) t72fs
t72fs: $(OUTDIR)/t72fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t73: $(CHK) t73ms
t73ms: $(OUTDIR)/t73ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)
$(OUTDIR)/t73ms.o: t73ms.cpp
	$(RCXXINCLUDESSRC)

t74: $(CHK) t74ms
t74ms: $(OUTDIR)/t74ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t75: $(CHK) t75ms
t75ms: $(OUTDIR)/t75ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)
$(OUTDIR)/t75ms.o: t75ms.cpp
	$(RCXXINCLUDESSRC)

t76: $(CHK) t76fs
t76fs: $(OUTDIR)/t76fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t77: $(CHK) t77fs
t77fs: $(OUTDIR)/t77fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t78: $(CHK) t78fs
t78fs: $(OUTDIR)/t78fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t79: $(CHK) t79fs
t79fs: $(OUTDIR)/t79fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t80: $(CHK) t80fs
t80fs: $(OUTDIR)/t80fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t83: $(CHK) t83fs
t83fs: $(OUTDIR)/t83fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t84: $(CHK) t84fs
t84fs: $(OUTDIR)/t84fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t85: $(CHK) t85ms
t85ms: $(OUTDIR)/t85ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t86: $(CHK) t86ms
t86ms: $(OUTDIR)/t86ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t88: $(CHK) t88ms
t88ms: $(OUTDIR)/t88ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t91: $(CHK) t91ms
t91ms: $(OUTDIR)/t91ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t92: $(CHK) t92ms
t92ms: $(OUTDIR)/t92ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)
$(OUTDIR)/t92ms.o: t92ms.cpp
	$(RCXXINCLUDESSRC)

t93: $(CHK) t93ms
t93ms: $(OUTDIR)/t93ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t94: $(CHK) t94fs
t94fs: $(OUTDIR)/t94fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t95: $(CHK) t95fs
t95fs: $(OUTDIR)/t95fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t96: $(CHK) t96ms
t96ms: $(OUTDIR)/t96ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t98: $(CHK) t98ms
t98ms: $(OUTDIR)/t98ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t100: $(CHK) t100ms
t100ms: $(OUTDIR)/t100ms.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t101: $(CHK) t101fs
t101fs: $(OUTDIR)/t101fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t102: $(CHK) t102fs
t102fs: $(OUTDIR)/t102fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t103: $(CHK) t103fs
t103fs: $(OUTDIR)/t103fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t104: $(CHK) t104ms
t104ms: $(OUTDIR)/t104ms.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t105: $(CHK) t105ms
t105ms: $(OUTDIR)/t105ms.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)
$(OUTDIR)/t105ms.o: t105ms.cpp
	$(RCXXINCLUDESSRC)

t115: $(CHK) t115ms
t115ms: $(OUTDIR)/t115ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t118: $(CHK) t118ms
t118ms: $(OUTDIR)/t118ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)
$(OUTDIR)/t118ms.o: t118ms.cpp
	$(RCXXINCLUDES)

t124: $(CHK) t124fs
t124fs: $(OUTDIR)/t124fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t126: $(CHK) t126fs
t126fs: $(OUTDIR)/t126fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t127: $(CHK) t127ms
t127ms: $(OUTDIR)/t127ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t128: $(CHK) t128fs
t128fs: $(OUTDIR)/t128fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t129: $(CHK) t129ms
t129ms: $(OUTDIR)/t129ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t130: $(CHK) t130map
t130map: $(OUTDIR)/t130map.o $(TMX) $(TMSEG)
	$(LINKITCXX) -o $@ $< $(RMX) $(RMSEG)

t131: $(CHK) t131ms
t131ms: $(OUTDIR)/t131ms.o $(OUTDIR)/tsock.o $(TMS)
	$(LINKITCXX) -o $@ $< $(OUTDIR)/tsock.o $(RMS)

t132: $(CHK) t132ms
t132ms: $(OUTDIR)/t132ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t133: $(CHK) t133cli t133srv
t133cli: $(OUTDIR)/t133cli.o $(TMSEG)
	$(LINKITCXX) -o $@ $< $(RMSEG)
t133srv: $(OUTDIR)/t133srv.o $(TMSEG)
	$(LINKITCXX) -o $@ $< $(RMSEG)

t135: $(CHK) t135ms
t135ms: $(OUTDIR)/t135ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t137: $(CHK) t137ms
t137ms: $(OUTDIR)/t137ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t138: $(CHK) t138ms
t138ms: $(OUTDIR)/t138ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t139: $(CHK) t139cl
t139cl: $(OUTDIR)/t139cl.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)
$(OUTDIR)/t139cl.o: t139cl.cpp
	$(RCXXINCLUDESSRC)

t141: $(CHK) t141ms
t141ms: $(OUTDIR)/t141ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)
$(OUTDIR)/t141ms.o: t141ms.cpp
	$(RCXXINCLUDESSRC)

t142: $(CHK) t142cli t142srv
t142cli: $(OUTDIR)/t142cli.o $(TMSUTP)
	$(LINKITCXX) -o $@ $< $(RMSUTP)
t142srv: $(OUTDIR)/t142srv.o $(TMSUTP)
	$(LINKITCXX) -o $@ $< $(RMSUTP)

t145: $(CHK) t145ms
t145ms: $(OUTDIR)/t145ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)
$(OUTDIR)/t145ms.o: t145ms.cpp
	$(RCXXINCLUDES)

t149: $(CHK) t149ms
t149ms: $(OUTDIR)/t149ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t151: $(CHK) t151ms
t151ms: $(OUTDIR)/t151ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t152: $(CHK) t152cli t152srv
t152cli: $(OUTDIR)/t152cli.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)
t152srv: $(OUTDIR)/t152srv.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t153: $(CHK) t153fs
t153fs: $(OUTDIR)/t153fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t155: $(CHK) t155ms
t155ms: $(OUTDIR)/t155ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t156: $(CHK) t156fs
t156fs: $(OUTDIR)/t156fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t157: $(CHK) t157ms
t157ms: $(OUTDIR)/t157ms.o $(TMX)
	$(LINKITCXX) -o $@ $< $(RMX) -lpthread -lrt

t160: $(CHK) t160ms
t160ms: $(OUTDIR)/t160ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t161: $(CHK) t161ms
t161ms: $(OUTDIR)/t161ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t162: $(CHK) t162fs
t162fs: $(OUTDIR)/t162fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t163: $(CHK) t163ms
t163ms: $(OUTDIR)/t163ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t164: $(CHK) t164fs
t164fs: $(OUTDIR)/t164fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t165: $(CHK) t165fs
t165fs: $(OUTDIR)/t165fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t167: $(CHK) t167ms
t167ms: $(OUTDIR)/t167ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t168: $(CHK) t168cli t168srv
t168cli: $(OUTDIR)/t168cli.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)
t168srv: $(OUTDIR)/t168srv.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t169: $(CHK) t169cli t169srv
t169cli: $(OUTDIR)/t169cli.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)
t169srv: $(OUTDIR)/t169srv.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t170: $(CHK) t170ms
t170ms: $(OUTDIR)/t170ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t172: $(CHK) t172ms
t172ms: $(OUTDIR)/t172ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t176: $(CHK) t176fs
t176fs: $(OUTDIR)/t176fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t180: $(CHK) t180ms
t180ms: $(OUTDIR)/t180ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t182: $(CHK) t182fs
t182fs: $(OUTDIR)/t182fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t183: $(CHK) t183ms
t183ms: $(OUTDIR)/t183ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t184: $(CHK) t184fs
t184fs: $(OUTDIR)/t184fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS) -lpthread

t185: $(CHK) t185fs
t185fs: $(OUTDIR)/t185fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t186: $(CHK) t186fs
t186fs: $(OUTDIR)/t186fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t187: $(CHK) t187ms
t187ms: $(OUTDIR)/t187ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t188: $(CHK) t188ms
t188ms: $(OUTDIR)/t188ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t190: $(CHK) t190fs
t190fs: $(OUTDIR)/t190fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t192: $(CHK) t192fs
t192fs: $(OUTDIR)/t192fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t194: $(CHK) t194fs
t194fs: $(OUTDIR)/t194fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t195: $(CHK) t195fs
t195fs: $(OUTDIR)/t195fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t197: $(CHK) t197fs
t197fs: $(OUTDIR)/t197fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t198: $(CHK) t198fs
t198fs: $(OUTDIR)/t198fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t201: $(CHK) t201ms
t201ms: $(OUTDIR)/t201ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t202: $(CHK) t202ms t202s
t202ms: $(OUTDIR)/t202ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)
t202s: $(OUTDIR)/t202s.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t205: $(CHK) t205fs
t205fs: $(OUTDIR)/t205fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t206: $(CHK) t206fs
t206fs: $(OUTDIR)/t206fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t207: $(CHK) t207fs
t207fs: $(OUTDIR)/t207fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t208: $(CHK) t208fs
t208fs: $(OUTDIR)/t208fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t209: $(CHK) t209ms
t209ms: $(OUTDIR)/t209ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t210: $(CHK) t210ms
t210ms: $(OUTDIR)/t210ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t212: $(CHK) t212ms
t212ms: $(OUTDIR)/t212ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)
$(OUTDIR)/t212ms.o: t212ms.cpp
	$(RCXXINCLUDESSRC)

t213: $(CHK) t213fs
t213fs: $(OUTDIR)/t213fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t214: $(CHK) t214fs
t214fs: $(OUTDIR)/t214fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t215: $(CHK) t215fs
t215fs: $(OUTDIR)/t215fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t216: $(CHK) t216ms
t216ms: $(OUTDIR)/t216ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t217: $(CHK) t217fs
t217fs: $(OUTDIR)/t217fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t219: $(CHK) t219ms
t219ms: $(OUTDIR)/t219ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t220: $(CHK) t220fs
t220fs: $(OUTDIR)/t220fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t221: $(CHK) t221ms
t221ms: $(OUTDIR)/t221ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t222: $(CHK) t222ms
t222ms: $(OUTDIR)/t222ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t226: $(CHK) t226fs
t226fs: $(OUTDIR)/t226fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t227: $(CHK) t227fs
t227fs: $(OUTDIR)/t227fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t228: $(CHK) t228ms
t228ms: $(OUTDIR)/t228ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t231: $(CHK) t231fs
t231fs: $(OUTDIR)/t231fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t233: $(CHK) t233ms
t233ms: $(OUTDIR)/t233ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t234: $(CHK) t234ms
t234ms: $(OUTDIR)/t234ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t235: $(CHK) t235ms
t235ms: $(OUTDIR)/t235ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t238: $(CHK) t238ms
t238ms: $(OUTDIR)/t238ms.o $(LIBEXPDIR)/libsblogalt.so
	$(LINKITCXX) -o $@ $< -L$(LIBEXPDIR) -lsblogalt -lpthread

t240: $(CHK) t240ms
t240ms: $(OUTDIR)/t240ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t241: $(CHK) t241ms
t241ms: $(OUTDIR)/t241ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t242: $(CHK) t242fs
t242fs: $(OUTDIR)/t242fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t245: $(CHK) t245ms
t245ms: $(OUTDIR)/t245ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)
$(OUTDIR)/t245ms.o: t245ms.cpp
	$(RCXXINCLUDESSRC)

t246: $(CHK) t246ms
t246ms: $(OUTDIR)/t246ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t249: $(CHK) t249ms
t249ms: $(OUTDIR)/t249ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t250: $(CHK) t250ms
t250ms: $(OUTDIR)/t250ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t253: $(CHK) t253ms
t253ms: $(OUTDIR)/t253ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t261: $(CHK) t261ms
t261ms: $(OUTDIR)/t261ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t262: $(CHK) t262ms
t262ms: $(OUTDIR)/t262ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t266: $(CHK) t266ms
t266ms: $(OUTDIR)/t266ms.o $(TMX) $(TMSEG)
	$(LINKITCXX) -o $@ $< $(RMX) $(RMSEG)
$(OUTDIR)/t266ms.o: t266ms.cpp
	$(RCXXINCLUDESSRC)

t277: $(CHK) t277fs
t277fs: $(OUTDIR)/t277fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t279: $(CHK) t279ms
t279ms: $(OUTDIR)/t279ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)
$(OUTDIR)/t279ms.o: t279ms.cpp
	$(RCXXINCLUDESSRC)

t280: $(CHK) t280fs
t280fs: $(OUTDIR)/t280fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t281: $(CHK) t281ms
t281ms: $(OUTDIR)/t281ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t282: $(CHK) t282ms
t282ms: $(OUTDIR)/t282ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t284: $(CHK) t284cli t284srv t284clij t284jni
t284cli: $(OUTDIR)/t284cli.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t284srv: $(OUTDIR)/t284srv.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS) -lrt

t284clij: $(OUTDIR)/com/hp/traf/t284cli.class $(OUTDIR)/com/hp/traf/t284cb.class $(OUTDIR)/com/hp/traf/t284exc.class $(OUTDIR)/com/hp/traf/t284id.class
$(OUTDIR)/com/hp/traf/t284cli.class $(OUTDIR)/com/hp/traf/t284cb.class $(OUTDIR)/com/hp/traf/t284exc.class $(OUTDIR)/com/hp/traf/t284id.class: t284cli.java t284cb.java t284exc.java t284id.java
	$(JAVAC) -cp $(OUTDIR) -d $(OUTDIR) t284cli.java t284cb.java t284exc.java t284id.java

t284jni: $(LIBEXPDIR)/libsbzt284.so
$(LIBEXPDIR)/libsbzt284.so: $(OUTDIR)/t284lib.o
	$(CXX) $(OPTIM_FLGS) -shared -o $@ $< $(LIBSBMSX)
$(OUTDIR)/t284lib.o: t284lib.cpp $(OUTDIR)/com_hp_traf_t284cli.h
	$(CXX) $(CDEPFLAGS) $(CXXFLAGS) $(INCLUDESJNIX) -o $@ -c $<
$(OUTDIR)/com_hp_traf_t284cli.h: $(OUTDIR)/com/hp/traf/t284cli.class
	$(JAVAH) -d $(OUTDIR) -cp $(OUTDIR) com.hp.traf.t284cli
	@touch $(OUTDIR)/com_hp_traf_t284cli.h

t285: $(CHK) t285fs
t285fs: $(OUTDIR)/t285fs.o $(TFS)
	$(LINKITCXX) -o $@ $< $(RFS)

t286: $(CHK) t286ms
t286ms: $(OUTDIR)/t286ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)

t287: $(CHK) t287ms
t287ms: $(OUTDIR)/t287ms.o $(TMS)
	$(LINKITCXX) -o $@ $< $(RMS)


tcheck: $(OUTDIR)/tcheck.o
	$(LINKITCXX) $(INCLUDESX) -o $@ $<
$(OUTDIR)/tcheck.o: tcheck.cpp
	$(RCXXINCLUDESSRC)

testfiles:
	@if [ ! -r $(TRAF_VAR)/sqconfig.db ]; then \
		echo "creating sqconfig.db"; \
		./gocleandb; \
	fi

tmerge: tmerge.o
	$(LINKITCXX) $(INCLUDESX) -o $@ $<
$(OUTDIR)/tmerge.o: tmerge.cpp
	$(RCXXINCLUDES)

tdelta: tdelta.o
	$(LINKITCXX) $(INCLUDESX) -o $@ $<
$(OUTDIR)/tdelta.o: tdelta.cpp
	$(RCXXINCLUDES)

tsock: tsock.cpp
	$(LINKITCXX) -DMAIN $(INCLUDESX) -o $@ $<
$(OUTDIR)/tsock.o: tsock.cpp
	$(RCXXINCLUDES)

ttee: $(OUTDIR)/ttee.o
	$(LINKITCXX) -o $@ $<

$(LIBEXPDIR)/libsbztmsfsutil.so: $(OBJTMSUTIL)
	$(CXX) $(OPTIM_FLGS) -shared -o $@ $(OBJTMSUTIL) $(LIBSBMSX)

$(LIBEXPDIR)/libsbztutil.so: $(OBJTUTIL)
	$(CXX) $(OPTIM_FLGS) -shared -o $@ $(OBJTUTIL)

$(LIBEXPDIR)/libsbztutilp.so: $(OBJTUTILP)
	$(CXX) $(OPTIM_FLGS) -shared -o $@ $(OBJTUTILP)

trace.cpp: vproc.h
$(OUTDIR)/vproc.o: vproc.h
vproc.h: vproc.h.in
	$(DOVERS) vproc.h.in vproc.h

stop:
	-killall shell monitor
	@for prog in $(PROGS); do killall $$prog; done

setup:
	@echo "do nothing"

clean:
	$(RM) $(PURGE) $(CDEPFILES)
	@grep 'HOOK_ENABLE=1' ms.env >/dev/null 2>/dev/null; if [ $$? == 0 ]; then echo "touch c s"; touch c s; fi

cleanall: clean
	$(RM) -fr $(BUILD_PLAT) stdout_* t9thread.rh6x.supp $(TRAF_VAR)/sqconfig.db

cleaner: clean
	$(RM) *~

cleanprogs:
	$(RM) $(PROGS) $(PROGSI)

-include $(CDEP_DIR)/*.dep
_dummy := $(shell if [ ! -d $(CDEP_DIR) ]; then mkdir -p $(CDEP_DIR); fi)
