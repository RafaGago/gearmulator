#!/bin/bash

errcho() { echo "$@" 1>&2; }

xwin_version="xwin-0.1.1"
xwin_prefix="xwin-xwin-0.1.1-x86_64-unknown-linux-musl"
xwin_bindir="$PWD/temp/xwin-0.1.1"
xwin_sha256="26ab4ec809a884f69c467edca77277d17c687e67d9b9003ee161aa7ae38201fa"
xwin_sdk_dst="${PWD}/temp/xwin-winsdk"

mkdir -p "${xwin_bindir}" && cd ${xwin_bindir}
wget "https://github.com/Jake-Shadle/xwin/releases/download/${xwin_version}/${xwin_prefix}.tar.gz"
if ! sha256sum "${xwin_prefix}.tar.gz" | grep -q "$xwin_sha256"; then
    errcho "xwin tampered or the author changed the contents. Aborting."
    exit 1
fi
tar -xf "${xwin_prefix}.tar.gz"
mv "$xwin_prefix/xwin" .
rm -r "$xwin_prefix" "${xwin_prefix}.tar.gz"

${xwin_bindir}/xwin --accept-license 1 --version 16 splat --output ${xwin_sdk_dst}

# Fix some symlinks that xwin didn't handle.
ln -s "${xwin_sdk_dst}/crt/lib/x86_64/msvcrt.lib" "${xwin_sdk_dst}/crt/lib/x86_64/msvcrtd.lib"
ln -s "${xwin_sdk_dst}/sdk/include/um/ole2.h" "${xwin_sdk_dst}/sdk/include/um/Ole2.h"
ln -s "${xwin_sdk_dst}/sdk/include/um/dbghelp.h" "${xwin_sdk_dst}/sdk/include/um/Dbghelp.h"
ln -s "${xwin_sdk_dst}/sdk/lib/um/x86_64/dbghelp.lib" "${xwin_sdk_dst}/sdk/lib/um/x86_64/DbgHelp.lib"
