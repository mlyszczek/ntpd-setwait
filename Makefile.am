ACLOCAL_AMFLAGS=-I m4

EXTRA_DIST = readme.md

bin_PROGRAMS = ntpd-setwait
ntpd_setwait_SOURCES = main.c daemonize.c daemonize.h
ntpd_setwait_CFLAGS = -I$(top_srcdir)
ntpd_setwait_LDFLAGS =

# static code analyzer

if ENABLE_ANALYZER

analyze_plists = main.plist daemonize.plist
MOSTLYCLEANFILES = $(analyze_plists)

$(analyze_plists): %.plist: %.c
	@echo "  CCSA  " $@
	@clang --analyze -I.. $< -o $@

analyze: $(analyze_plists)

endif # ENABLE_ANALYZER

.PHONY: analyze
