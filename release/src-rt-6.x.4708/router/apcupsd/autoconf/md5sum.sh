#!/bin/sh
#
# To help track down file changes, I have decided to generate for each
# release a manifest file containing the md5 checksums of all the files
# distributed.
# To verify the checksum, simply call 'md5sum MANIFEST'.
#
find . -type f ! -name MANIFEST -exec md5sum {} \; > MANIFEST
