#!/bin/bash
# version changing script
NAME=php-ext-libtrie
VERSION=0.1.3

FROM="$NAME v[0-9]{1,2}.[0-9]{1,2}.[0-9]{1,2}"
TO="$NAME v$VERSION"
FROM2="PHP_LIBTRIE_VERSION \"[0-9]{1,2}.[0-9]{1,2}.[0-9]{1,2}\""
TO2="PHP_LIBTRIE_VERSION \"$VERSION\""


sed -ri "s#$FROM#$TO#" *.c
sed -ri "s#$FROM#$TO#" *.h
# php extension version replace
sed -ri "s#$FROM2#$TO2#" *.h
#
sed -ri "s#$FROM#$TO#" README.md
echo $VERSION