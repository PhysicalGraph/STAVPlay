#!/bin/bash

set -o errexit
set -o nounset

repo=https://s3-us-west-2.amazonaws.com/smartthings-av-webports
dl_dir=third/dl
stavports_ver="2016-09-01"
stavports_md5="acc4df38b80fde2065510b4fe5c590b7"

if   which md5 >/dev/null;    then md5cmd="md5 -r"
elif which md5sum >/dev/null; then md5cmd="md5sum"
else
    echo "ERROR: No supported md5 tool found" 1>&2
    exit 1
fi
do_md5() {
    $md5cmd "$1" | cut -d' ' -f1
}

status() {
    printf "\n>> %s\n" "$*"
}

dl_file() {
    local filename=$1
    local md5sum=$2
    local dl_url=$3
    local filepath=$dl_dir/$filename

    if [ -e $filepath ] && [ $md5sum = "$(do_md5 $filepath)" ]; then
	return
    fi

    status "Downloading $filename ..."
    curl -# -k -L "$dl_url" >$filepath

    if [ $md5sum != "$(do_md5 $filepath)" ]; then
	echo "ERROR: $filename did not have expected md5 hash" 1>&2
	exit 1
    fi
}

check_ver_file() {
    local pkgver=$1
    local versionfile=third/version.txt

    if [ -e $versionfile ] && [ $pkgver = $(cat $versionfile) ]; then
	return 0
    fi
    return 1
}

mkdir -p $dl_dir
fname="stavports_${stavports_ver}.tar.gz"
dl_file $fname $stavports_md5 "$repo/${fname}"

# clean-up old locations
rm -rf third/include
rm -rf third/lib

if ! check_ver_file $stavports_ver; then
    status "Extracting stavports ..."
    tar -C third/ -xvf $dl_dir/$fname
    printf "%s" "$stavports_ver" >third/version.txt
fi

status "SUCCESS"
