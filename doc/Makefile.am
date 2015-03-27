MAN = castfs.8
POD = castfs.pod

EXTRA_DIST = $(POD) $(MAN)

man8_MANS = $(MAN)

$(MAN): $(POD)
	@$(POD2MAN) -u --section=8 --release="Source Mage GNU/Linux" \
		--center "System Administration" \
		--date "$$(date '+%B %Y' -d @`stat -c '%Y' $(POD)`)" $< >$@

CLEANFILES = $(MAN)
