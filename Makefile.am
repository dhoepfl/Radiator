SUBDIRS = src

EXTRA_DIST = release.in

install-data-hook:
	mkdir -p "$(localstatedir)"
	cp -r -- "$(top_srcdir)/release.in/"* "$(prefix)"
