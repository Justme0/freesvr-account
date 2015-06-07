prefix		= /opt/freesvr/audit/account
sbindir		= $(prefix)/sbin
sysconfdir	= $(prefix)/etc
logdir		= $(prefix)/log
target		= freesvr-account
srcdir		= .

INSTALL		= /usr/bin/install -c
DESTDIR		=
STRIP_OPT	= -s

OBJS		= connect.o global.o openssl.o process.o db.o main.o log.o config.o
CC		= gcc
CFLAGS		= -DNDEBUG -Werror -Wall -Wextra -std=gnu99 -c
INCLUDE		= -I/opt/freesvr/sql/include/mysql
LDFLAGS		= -L/opt/freesvr/sql/lib/mysql
LIBS		= -lmysqlclient -lcrypto

CONFILE		= \"$(sysconfdir)/freesvr_account_config\"
PROGRAM		= \"$(target)\"

all		: $(target)

$(target)	: $(OBJS)
	$(CC) $(LDFLAGS) $(LIBS) -o $@ $(OBJS)

connect.o	: connect.c
	$(CC) $(CFLAGS) connect.c

global.o	: global.c
	$(CC) $(CFLAGS) global.c

openssl.o	: openssl.c
	$(CC) $(CFLAGS) openssl.c

process.o	: process.c
	$(CC) $(CFLAGS) process.c

db.o		: db.c
	$(CC) $(CFLAGS) db.c		$(INCLUDE)

main.o		: main.c
	$(CC) $(CFLAGS) main.c		-DPROGRAM_NAME=$(PROGRAM)

log.o		: log.c
	$(CC) $(CFLAGS) log.c		-DPROGRAM_NAME=$(PROGRAM)

config.o	: config.c
	$(CC) $(CFLAGS) config.c	-DCONFIG_FILE=$(CONFILE)

clean		:
	rm -f $(OBJS) $(target)

install		: $(target) account_config
	$(srcdir)/mkinstalldirs $(DESTDIR)$(sbindir)
	(umask 022 ; $(srcdir)/mkinstalldirs $(DESTDIR)$(PRIVSEP_PATH))
	$(INSTALL) -m 0755 $(STRIP_OPT) $(target) $(DESTDIR)$(sbindir)/$(target)

	$(srcdir)/mkinstalldirs $(DESTDIR)$(sysconfdir)
	(umask 022 ; $(srcdir)/mkinstalldirs $(DESTDIR)$(PRIVSEP_PATH))
	$(INSTALL) -m 0644 account_config $(DESTDIR)$(sysconfdir)/freesvr_account_config

	$(srcdir)/mkinstalldirs $(DESTDIR)$(logdir)

uninstall:
	rm -f $(DESTDIR)$(sbindir)/$(target)
