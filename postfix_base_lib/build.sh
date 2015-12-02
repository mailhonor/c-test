#!/bin/sh

_E()
{
	echo $@
	exit 1
}

pdir=$1
[ -z $pdir ] && _E USAGE:"\n\t"$0 postfix_source_dir

selfdir=`pwd`

cd $pdir || exit 1

which indent >/dev/null || _E echo "please install indent"

[ -f .postfix_dev_indent ] || find src -type f -name "*.h" \
	-exec indent -npro -kr -i8 -ts8 -sob -ss -ncs -cp1 --blank-lines-after-procedures -l00000 {} \;

touch  .postfix_dev_indent

make -f Makefile.init makefiles \
	        'CCARGS=-DHAS_MYSQL -DHAS_PCRE -DNO_NIS' \
		        'AUXLIBS= -lmysqlclient -lz -lm -lpcre -lresolv ' || exit 1

make || exit 1

rm -f libpostfix_dev.a
ar r libpostfix_dev.a src/tls/*.o src/master/*.o src/global/*.o src/util/*.o
ranib libpostfix_dev.a
mv libpostfix_dev.a /usr/local/lib/

mkdir -p /usr/local/include/postfix_dev/
php $selfdir/convert_include.php

echo "OK"


