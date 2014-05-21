#######################################################################
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 1995-2014 Hewlett-Packard Development Company, L.P.
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
#######################################################################

CPPSRC= AbortAppender.cpp \
        Appender.cpp \
	AppenderSkeleton.cpp \
	AppendersFactory.cpp \
	BasicConfigurator.cpp \
	BasicLayout.cpp \
	BufferingAppender.cpp \
	Category.cpp \
	CategoryStream.cpp \
	Configurator.cpp \
	DllMain.cpp \
	DummyThreads.cpp \
	FactoryParams.cpp \
	FileAppender.cpp \
	Filter.cpp \
	FixedContextCategory.cpp \
	HierarchyMaintainer.cpp \
	IdsaAppender.cpp \
	LayoutAppender.cpp \
	LayoutsFactory.cpp \
	LevelEvaluator.cpp \
	Localtime.cpp \
	LoggingEvent.cpp \
	MSThreads.cpp \
	NDC.cpp \
	NTEventLogAppender.cpp \
	OmniThreads.cpp \
	OstreamAppender.cpp \
	PThreads.cpp \
	PassThroughLayout.cpp \
	PatternLayout.cpp \
	PortabilityImpl.cpp \
	Priority.cpp \
	Properties.cpp \
	PropertyConfigurator.cpp \
	PropertyConfiguratorImpl.cpp \
	RemoteSyslogAppender.cpp \
	RollingFileAppender.cpp \
	SimpleConfigurator.cpp \
	SimpleLayout.cpp \
	StringQueueAppender.cpp \
	SyslogAppender.cpp \
	StringUtil.cpp \
	TimeStamp.cpp \
	TriggeringEventEvaluatorFactory.cpp \
	Win32DebugAppender.cpp \

ifeq ($(TARGTYPE),linux)
CPPSRC += vers_log4cpp.cpp
endif


SRCPATH := bin log4cpp/log4cpp-1.0/src


ifeq ($(TARGTYPE),nt)
DEFS := /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_DLL" /D "_USRDLL" /D "LOG4CPP_HAS_DLL" /D "LOG4CPP_BUILD_DLL" /D "_WINDLL" /D "_MBCS" /Gm /EHsc /RTC1 /MDd

SYS_LIBS := kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib
endif
           
ifeq ($(TARGTYPE),yos)
DEFS := -DNA_NSK
endif

