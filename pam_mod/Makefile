# Modified Makefile to allow for building of the standalone module

TITLE=pam_diremail

CC = gcc
LD = gcc
CFLAGS = -fPIC -c -g -Wall -Wformat-security -fvisibility=hidden
LDFLAGS = -Wl,-x --shared
LDLIBS = -lcrypt -lpam -lpam_misc
LIBSRC = $(TITLE).c
LIBOBJ = $(TITLE).o 
LIBSHARED = $(TITLE).so
PAM_LIB_DIR = $(DESTDIR)/lib/security
INSTALL = /usr/bin/install

ifneq (,$(findstring noopt,$(DEB_BUILD_OPTIONS)))
   CFLAGS += -O0
else
   CFLAGS += -O2
endif


all: $(LIBSHARED)


$(LIBSHARED): $(LIBOBJ)
	$(LD) $(LDFLAGS) -o $@ $(LIBOBJ) $(LDLIBS)

install: $(LIBSHARED)
	$(INSTALL) -m 0755 -d $(PAM_LIB_DIR)
	$(INSTALL) -m 0644 pam_diremail.so $(PAM_LIB_DIR)

clean:
	rm -f $(LIBOBJS) $(LIBSHARED) core *~ *.o

extraclean: clean
	rm -f *.a *.o *.so *.bak 

.c.o:	
	$(CC) $(CFLAGS) $< -o $@

