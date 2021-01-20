#!/bin/bash
rm -fr `pwd`/build_rpm
mkdir -p `pwd`/build_rpm/SOURCES
git archive --output `pwd`/build_rpm/SOURCES/harbour-quake2.tar.gz HEAD

sfdk engine exec sb2 -t SailfishOS-3.4.0.24-armv7hl rpmbuild --define "_topdir `pwd`/build_rpm" --define "_arch armv7hl" -ba spec/quake2.spec