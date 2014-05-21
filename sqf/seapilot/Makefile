# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2006-2014 Hewlett-Packard Development Company, L.P.
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

# Top level Seapilot Make

include macros.gmk

TARGETS :=

ALL_COMPONENTS := seabed-dependent sql-dependent standalone


STANDALONE_COMPONENTS := publications wrapper wrapper_sq wrapper_mon \
	consumer_wrapper chameleon event_templates mal \
	protobuf_serialization_sq


.PHONY: all
all:  $(ALL_COMPONENTS)

.PHONY: sql-dependent
sql-dependent:


seabed-dependent: text_protocol_adapter PTPA

.PHONY: standalone
standalone: $(STANDALONE_COMPONENTS)

.PHONY: wrapper
wrapper: publications
	cd source/wrapper; $(MAKE)

.PHONY: wrapper_sq
wrapper_sq: publications
	cd source/wrapper_sq; $(MAKE)

.PHONY: wrapper_mon
wrapper_mon: publications
	cd source/wrapper_mon; $(MAKE)

.PHONY: consumer_wrapper
consumer_wrapper: publications
	cd source/consumer_wrapper; $(MAKE)


.PHONY: mal
mal:
	cd source/mal; $(MAKE)

.PHONY: publications
publications:
	cd source/publications; $(MAKE)

.PHONY: text_protocol_adapter
text_protocol_adapter: chameleon
	cd source/text_protocol_adapter; $(MAKE)

.PHONY: PTPA
PTPA: chameleon publications protobuf_serialization_sq wrapper_sq
	cd source/PTPA; $(MAKE)


.PHONY: event_templates
event_templates: source/event_templates/sql.error_event.template
	cd source/event_templates; $(MAKE)
source/event_templates/sql.error_event.template: ../../sql/bin/SqlciErrors.txt
	awk '/^[[:digit:]]/ {printf "109%06d SQL Error Message : $$~message_text\\nSQL Query id : $$~sqlid\n", $$1}' ../../sql/bin/SqlciErrors.txt > source/event_templates/sql.error_event.template


.PHONY: protobuf_serialization_sq
protobuf_serialization_sq: wrapper_sq mal
	cd source/protobuf_serialization_sq; $(MAKE)
.PHONY: chameleon
chameleon: wrapper_sq publications
	cd source/chameleon; $(MAKE)

.PHONY: clean
clean:
	cd source/mal; $(MAKE) clean
	cd source/publications; $(MAKE) clean
	cd source/chameleon; $(MAKE) clean
	cd source/PTPA; $(MAKE) clean
	cd source/wrapper; $(MAKE) clean
	cd source/wrapper_mon; $(MAKE) clean
	cd source/wrapper_sq; $(MAKE) clean
	cd source/consumer_wrapper; $(MAKE) clean
	cd source/text_protocol_adapter; $(MAKE) clean
	cd source/event_templates; $(MAKE) clean
	cd source/protobuf_serialization_sq; $(MAKE) clean

	find . -name '*.gcda' -exec rm -f {} \;
	find . -name '*.gcno' -exec rm -f {} \;
	find . -name '*.gcov' -exec rm -f {} \;
	find . -name '*.gcda' -exec rm -f {} \;
	rm -f build.log build.log.old
	rm -rf $(SP_EXPORT_LIB)
	rm -rf $(SP_EXPORT_BIN)

