SUBDIRS = src

nobase_release_bin__FILES = release/bin/radiator_mail.py \
                            release/bin/radiator_vu.py \
                            release/bin/radiator_pushed.py \
                            release/bin/radiator_rrdtool.py \
                            release/bin/radiator_sql.py \
                            release/bin/radiatord.sh
nobase_release_init_d__FILES = release/etc/init.d/radiator.sh
nobase_release_scripts__FILES = release/scripts/radiator_setup_sql.sh \
                                release/scripts/radiator_setup_rrd.sh
nobase_release_share_doc_examples__FILES = release/etc/radiator.conf
CLEANFILES=$(nobase_release_bin__FILES) $(nobase_release_init_d__FILES) $(nobase_release_scripts__FILES) $(nobase_release_share_doc_examples__FILES)
EXTRA_DIST = release

do_subst = sed \
	-e 's|[@]prefix@|$(prefix)|g' \
	-e 's|[@]bindir@|$(bindir)|g' \
	-e 's|[@]sysconfdir@|$(sysconfdir)|g' \
	-e 's|[@]docdir@|$(docdir)|g' \
	-e 's|[@]localstatedir@|$(localstatedir)|g'

release/%: release/%.in
	mkdir -p "$(shell dirname "$@")"
	cp -a "$<" "$@"
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

install-data-hook-release-share-doc-examples: $(nobase_release_share_doc_examples__FILES)
	mkdir -p "$(docdir)/examples"
	cp -- $(nobase_release_share_doc_examples__FILES) "$(docdir)/examples"

install-data-hook: install-data-hook-release-bin install-data-hook-release-init-d install-data-hook-release-scripts install-data-hook-release-share-doc-examples
	mkdir -p "$(localstatedir)/log"
	mkdir -p "$(localstatedir)/run"
