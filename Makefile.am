SUBDIRS = www

EXTRA_DIST = readme.md init.d/ntpd-setwait.conf init.d/ntpd-setwait.openrc \
	ntpd-setwait.1

sysconf_DATA = init.d/ntpd-setwait.conf
init_ddir = $(sysconfdir)/init.d
dist_init_d_SCRIPTS = init.d/ntpd-setwait
man_MANS = ntpd-setwait.1

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

www:
	./gen-download-page.sh
	./man2html.sh
	make www -C www

.PHONY: analyze www
