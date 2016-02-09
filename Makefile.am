SUBDIRS = src

nobase_release_bin__FILES = release/bin/analyseStream.sh \
		release/bin/radiator_daemon.sh
nobase_release_init_d__FILES = release/etc/init.d/radiator.sh
nobase_release_scripts__FILES = release/scripts/setup.sh
CLEANFILES=$(nobase_release_bin__FILES) $(nobase_release_init_d__FILES) $(nobase_release_scripts__FILES)
EXTRA_DIST = release.in

do_subst = sed \
	-e 's|[@]prefix@|$(prefix)|g' \
	-e 's|[@]bindir@|$(bindir)|g' \
	-e 's|[@]sysconfdir@|$(sysconfdir)|g' \
	-e 's|[@]localstatedir@|$(localstatedir)|g'

release/%: release/%.in
	mkdir -p "$(shell dirname "$@")"
	$(do_subst) >"$@" <"$<"

install-data-hook-release-bin: $(nobase_release_bin__FILES)
	mkdir -p "$(bindir)"
	cp -- $(nobase_release_bin__FILES) "$(bindir)"

install-data-hook-release-init-d: $(nobase_release_init_d__FILES)
	mkdir -p "$(prefix)/etc/init.d"
	cp -- $(nobase_release_init_d__FILES) "$(prefix)/etc/init.d"

install-data-hook-release-scripts: $(nobase_release_scripts__FILES)
	mkdir -p "$(prefix)/scripts"
	cp -- $(nobase_release_scripts__FILES) "$(prefix)/scripts"

install-data-hook: install-data-hook-release-bin install-data-hook-release-init-d
	mkdir -p "$(localstatedir)/log"
	mkdir -p "$(localstatedir)/run"
