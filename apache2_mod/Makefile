all: mod_zydeflate

mod_zydeflate: mod_zydeflate.c
	apxs  -Wall -i  -a -c mod_zydeflate.c -lz
	#-lcrypt
