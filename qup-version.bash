#!/usr/bin/env bash

echo "The command sed may fail on MacOS."

VERSION=$1

if [ -z "$VERSION" ]
then
    echo "Please specify the version: $0 <VERSION>."
    exit 1
fi

FILE="source/qup.cc"

sed -i \
    's/\(QString qup::VERSION = "\)[0-9]\+\(\.[0-9]\+\)*"/\1'"$VERSION"'"/' \
    $FILE
sed -i \
's/\(QString qup::VERSION_LTS = "\)[0-9]\+\(\.[0-9]\+\)*"/\1'"$VERSION"'"/' \
$FILE

echo "Please modify the release notes."
