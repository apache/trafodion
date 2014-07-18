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
include ../../macros.gmk

PRO_COMMON      = ../../export/publications/common.qpid_header.proto \
		  ../../export/publications/common.info_header.proto \
		  ../../export/publications/common.event_header.proto \
		  ../../export/publications/common.health_header.proto \
		  ../../export/publications/common.health_template.proto \
		  ../../export/publications/common.text_event.proto 

PRO_OTHER       = ../../export/publications/dtm.events.proto \
                  ../../export/publications/dtm.perf_abort_count.proto \
                  ../../export/publications/dtm.perf_audit_write.proto \
                  ../../export/publications/dtm.perf_commit_count.proto \
                  ../../export/publications/dtm.perf_resources.proto \
                  ../../export/publications/dtm.perf_stats.proto \
                  ../../export/publications/dtm.perf_tx_begin.proto \
                  ../../export/publications/sql.space_usage.proto \
                  ../../export/publications/se.health_se.proto \
		  ../../export/publications/se.perf_stats.proto \
		  ../../export/publications/unc.events.proto \
		  ../../export/publications/summary.simple_query_stats.proto \
                  ../../export/publications/sql.error_event.proto \
		  ../../export/publications/sql.info_event.proto \
		  ../../export/publications/sql.tracking_compilers.proto \
                  ../../export/publications/ndcs.query_start_stats.proto \
                  ../../export/publications/ndcs.query_end_stats.proto \
                  ../../export/publications/ndcs.query_sql_text.proto \
                  ../../export/publications/ndcs.session_start_stats.proto \
                  ../../export/publications/ndcs.session_end_stats.proto \
                  ../../export/publications/wms.wms_stats.proto \
                  ../../export/publications/wms.wms_resources.proto \
                  ../../export/publications/wms.wms_pertablestats.proto \
		  ../../export/publications/chameleon.events.proto \
                  ../../export/publications/accesslayer.level_1_check.proto \
                  ../../export/publications/health.layer_check.proto

PROTOSRC       := $(PRO_COMMON) $(PRO_OTHER)

# dependency to build common first for header file inclusion
PROTO_COM_OBJS := $(patsubst %.proto, $(OUTDIR)/%.pb.o, $(notdir $(PRO_COMMON)))
PROTO_OTH_OBJS := $(patsubst %.proto, $(OUTDIR)/%.pb.o, $(notdir $(PRO_OTHER)))
$(PROTO_OTH_OBJS): $(PROTO_COM_OBJS)

$(OUTDIR)/accesslayer.level_1_check.pb.o: $(OUTDIR)/health.layer_check.pb.cc

INCLUDES 	= -I$(OUTDIR) \
		  -I$(MY_SPROOT)/export/include \
		  -I$(MY_SQROOT)/export/include \
		  -I$(PROTOBUFS_INC)

CPPSRC		:= vers.cpp
FINALOBJ := $(SP_EXPORT_LIB)/libpublications.so

include ../../build.gmk
