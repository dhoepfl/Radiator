SUBDIRS = src

EXTRA_DIST = release.in

install-data-hook:
	mkdir -p "$(localstatedir)/log"
	mkdir -p "$(runstatedir)"
	cp -r -- "$(top_srcdir)/release.in/"* "$(prefix)"
