#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# Local source snapshot through the real ports framework. No package publishing.
set -eu
export PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
[ "$(uname -s)" = FreeBSD ] || { echo 'FreeBSD required' >&2; exit 1; }
repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$repo_root"
work_root=$(mktemp -d)
trap 'rm -rf "$work_root"' EXIT HUP INT TERM
ports_root=${PORTSDIR:-/usr/ports}
if [ ! -f "$ports_root/Mk/bsd.port.mk" ]; then
	ports_root="$work_root/ports"
	git clone --depth 1 https://git.freebsd.org/ports.git "$ports_root"
fi
port="$work_root/overlay/deskutils/gmarkdown"
mkdir -p "$port" "$work_root/distfiles" "$repo_root/dist/packages"
cp -R packaging/freebsd/gmarkdown/. "$port/"
distname=$(make -C "$port" PORTSDIR="$ports_root" -V DISTNAME)
pkgname=$(make -C "$port" PORTSDIR="$ports_root" -V PKGNAME)
case "$distname:$pkgname" in *[!A-Za-z0-9_.:+-]*|:*|*:) exit 1 ;; esac
source_root="$work_root/source/$distname"
mkdir -p "$source_root"
for input in meson.build LICENSE README.md PLAN.md THIRD_PARTY.md src data tests subprojects; do
	cp -R "$input" "$source_root/"
done
tar -C "$work_root/source" -czf "$work_root/distfiles/$distname.tar.gz" "$distname"
run_port() {
	make -C "$port" PORTSDIR="$ports_root" DISTDIR="$work_root/distfiles" \
		WRKDIRPREFIX="$work_root/work" PACKAGES="$1" \
		MASTER_SITES="file://$work_root/distfiles/" makesum
	make -C "$port" PORTSDIR="$ports_root" DISTDIR="$work_root/distfiles" \
		WRKDIRPREFIX="$work_root/work" PACKAGES="$1" \
		MASTER_SITES="file://$work_root/distfiles/" DEVELOPER=yes \
		stage test check-plist stage-qa package
}
if [ "$(id -u)" -eq 0 ]; then
	actor=${GMARKDOWN_PORTS_TEST_USER:-circleci}
	case "$actor" in ''|-*|*[!A-Za-z0-9_.-]*) echo 'Invalid ports test user' >&2; exit 64 ;; esac
	actor_uid=$(id -u "$actor")
	[ "$actor_uid" -gt 0 ] || { echo 'Ordinary ports test user required' >&2; exit 77; }
	mkdir -p "$work_root/packages" "$work_root/home"
	chown -R "$actor_uid:$(id -g "$actor")" "$work_root"
	cd "$work_root"
	sudo -H -u "$actor" /usr/bin/env -i HOME="$work_root/home" \
		USER="$actor" LOGNAME="$actor" PATH="$PATH" LANG=C.UTF-8 \
		PORTSDIR="$ports_root" \
		/bin/sh -c 'cd "$1" && shift && "$@"' \
		_ "$repo_root" true
	# shellcheck disable=SC2016
	sudo -H -u "$actor" /usr/bin/env -i HOME="$work_root/home" \
		USER="$actor" LOGNAME="$actor" PATH="$PATH" LANG=C.UTF-8 \
		/bin/sh -c "port=\"$port\"; ports_root=\"$ports_root\"; work_root=\"$work_root\"; \
		make -C \"\$port\" PORTSDIR=\"\$ports_root\" DISTDIR=\"\$work_root/distfiles\" \
		WRKDIRPREFIX=\"\$work_root/work\" PACKAGES=\"\$work_root/packages\" \
		MASTER_SITES=\"file://\$work_root/distfiles/\" makesum && \
		make -C \"\$port\" PORTSDIR=\"\$ports_root\" DISTDIR=\"\$work_root/distfiles\" \
		WRKDIRPREFIX=\"\$work_root/work\" PACKAGES=\"\$work_root/packages\" \
		MASTER_SITES=\"file://\$work_root/distfiles/\" DEVELOPER=yes \
		stage test check-plist stage-qa package"
	cd "$repo_root"
	package="$work_root/packages/All/$pkgname.pkg"
	[ -f "$package" ] && [ ! -L "$package" ] || { echo 'Regular package artifact required' >&2; exit 1; }
	mkdir -p "$repo_root/dist/packages/All"
	cp "$package" "$repo_root/dist/packages/All/$pkgname.pkg"
else
	run_port "$repo_root/dist/packages"
fi
test -f "$repo_root/dist/packages/All/$pkgname.pkg"
cp "$port/distinfo" "$repo_root/dist/snapshot.distinfo"
cp "$work_root/distfiles/$distname.tar.gz" "$repo_root/dist/"
git -c safe.directory="$ports_root" -C "$ports_root" rev-parse HEAD >"$repo_root/dist/ports-revision.txt" 2>/dev/null || \
	echo 'unversioned-ports' >"$repo_root/dist/ports-revision.txt"
(cd "$repo_root/dist/packages/All" && sha256 -r "$pkgname.pkg") >"$repo_root/dist/SHA256SUMS"
(cd "$repo_root/dist" && sha256 -r "$distname.tar.gz") >>"$repo_root/dist/SHA256SUMS"
