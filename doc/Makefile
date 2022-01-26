-include ../config.mak

manpages = pg_ddm.1 pg_ddm.5

dist_man_MANS = $(manpages)

EXTRA_DIST = config.md usage.md Makefile $(manpages) \
	     frag-config-man.md frag-usage-man.md \
	     filter.py

CLEANFILES = pg_ddm_1.md pg_ddm_5.md

# make maintainer-clean removes those
MAINTAINERCLEANFILES = $(manpages)

SUBLOC = doc
abs_top_srcdir ?= $(CURDIR)/..
include $(abs_top_srcdir)/lib/mk/antimake.mk

export PACKAGE_VERSION

pg_ddm.%: pg_ddm_%.md
	$(PANDOC) -s -t man -o $@ $<

pg_ddm_1.md: filter.py frag-usage-man.md usage.md
	$(PYTHON) $^ >$@

pg_ddm_5.md: filter.py frag-config-man.md config.md
	$(PYTHON) $^ >$@

.INTERMEDIATE: pg_ddm_1.md pg_ddm_5.md

web:
	$(MAKE) -C ../../pg_ddm.github.io
