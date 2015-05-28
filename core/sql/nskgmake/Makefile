# ****************************************************************************
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2007-2015 Hewlett-Packard Development Company, L.P.
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
# File:         Makefile
# Description:  Makefile for SQL/MX, this file provides an easy to use
#               interface for building SQL/MX for various targets.
#
# Language:     GNU make
# Date:         October 3, 2007
#
# ****************************************************************************

MAKEFLAGS += --no-print-directory

TARGTYPES = linux 
HELP_TARGETS = all help $(TARGTYPES:%=%help)
GLOBAL_TARGETS = clobber distclean

# The simple way to do this work is to recursively call another make process
# with the proper TARGTYPE and FLAVOR.  However, GNU make isn't supported
# real well under Windows (primarily related to job support).  In order to
# provide a good build environment for SQL/MX, we try to run everything within
# the context of one make process.  This gets pretty tricky.
#
# This top-level makefile's main goal is to determine the correct TARGTYPE
# and FLAVOR before including Makerules.mk.

# Only support one goal.  For now, it would probably be too hard to ensure that
# we could build targets of two different TARGTYPEs correctly.  It probably
# isn't too important that we are able to do that.
ifneq (1,$(words $(MAKECMDGOALS)))
  ifneq (0,$(words $(MAKECMDGOALS)))
    $(error We dont currently support more than one target... Sorry)
  endif
endif

# Determine the target that we are building for.
TARGTYPE := $(strip $(foreach targ,$(TARGTYPES),$(if $(filter $(targ)%,\
    $(MAKECMDGOALS)),$(targ))))

# Try to find the TARGTYPE from a string like one of the following:
#   ../lib/nt/debug/tdm_arkcmp.exe
#   optimizer/nt/debug/OptAll.obj
ifndef TARGTYPE
TARGTYPE := $(strip $(foreach targ,$(TARGTYPES),$(if $(findstring /$(targ)/,\
    $(MAKECMDGOALS)),$(targ))))
endif

# Determine if this is a debug or release target.
ifneq (,$(findstring release,$(MAKECMDGOALS)))
  FLAVOR := release
else
  ifneq (,$(findstring debug,$(MAKECMDGOALS)))
    FLAVOR := debug
  else 
    ifneq (,$(findstring doc,$(MAKECMDGOALS)))
      FLAVOR := doc
    endif
  endif
endif

# Determine if the target is for security
ifneq (,$(findstring sec,$(MAKECMDGOALS)))
  SECURITY := true
else
  SECURITY := false
endif

# Set machine type for linux system, for other system, ARCHBITS is empty
ifeq ($(TARGTYPE),linux)
ifeq ($(SQ_MTYPE),64)
ARCHBITS := 64bit
else
ARCHBITS := 32bit
endif
endif

# Save a copy of the CLASSPATH of the environment.
ENV_CLASSPATH := $(CLASSPATH)

# This is needed to prevent a syntax problem when including the
# Makerules.$(target) below
define incMRtemplate
  include $(1)
endef

# Handle help targets
ifneq (,$(findstring $(MAKECMDGOALS),$(HELP_TARGETS)))
  ifdef TARGTYPE
    include Makerules.$(TARGTYPE)
  else
    $(foreach target,$(TARGTYPES),$(eval $(call incMRtemplate,Makerules.$(target))))
  endif
endif

ifdef MAKECMDGOALS
  # If a rule was given that is not handled directly by this file, then
  # give an error if either TARGTYPE or FLAVOR could not be determined.
  # Otherwise, everything should be OK for building so Makerules.mk is
  # included.
  ifeq (,$(filter $(MAKECMDGOALS),$(HELP_TARGETS) $(GLOBAL_TARGETS)))
    ifndef TARGTYPE
      $(error Cannot determine the target type)
    endif
    include Makerules.mk
  endif
else
  .DEFAULT_GOAL=help
  $(foreach target,$(TARGTYPES),$(eval $(call incMRtemplate,Makerules.$(target))))
endif

help::
	@echo "$(MAKE) clobber              Removes all objects from all targets"

clobber distclean:
	@echo $@ is not yet implemented.
