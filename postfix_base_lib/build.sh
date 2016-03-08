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
	        'CCARGS=-DNO_NIS -DHAS_PCRE -DHAS_MYSQL -I/usr/include/mysql/' \
		        'AUXLIBS= -lmysqlclient -lpcre -lz -lm -lresolv ' || exit 1

make || exit 1

rm -f libpostfix_dev.a
ar r libpostfix_dev.a src/tls/*.o src/master/*.o src/global/*.o src/util/*.o
ranlib libpostfix_dev.a

mkdir -p postfix_dev
php $selfdir/convert_include.php

cd $selfdir
cp -a  $pdir/postfix_dev .
cp -a  $pdir/libpostfix_dev.a .

echo "OK"

