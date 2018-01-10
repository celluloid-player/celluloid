#!/usr/bin/env sh

cd "${MESON_BUILD_ROOT}/data"
appstream-util validate io.github.GnomeMpv.appdata.xml || exit 1
desktop-file-validate io.github.GnomeMpv.desktop || exit 1

cd "${MESON_SOURCE_ROOT}/po"
for i in *.po
do
	msgfmt -o /dev/null -vc --check-accelerator=_ "$i" || exit 1
done
