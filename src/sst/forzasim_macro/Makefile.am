# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libforzasim_macro.la
libforzasim_macro_la_SOURCES = \
    Precinct.h \
    Precinct.cc \
    ZAP_PE.h \
    ZAP_PE.cc \
    ZAP_memctrl_simple.h \
    ZAP_memctrl_simple.cc \
    Zone.h \
    Zone.cc

EXTRA_DIST = \
    dummy.py

libforzasim_macro_la_LDFLAGS = -module -avoid-version

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_FORZASIM_MACRO_SOURCE forzasim_macro=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_FORZASIM_MACRO_TESTS  forzasim_macro=$(abs_srcdir)

##########################################################################
##########################################################################
##########################################################################


