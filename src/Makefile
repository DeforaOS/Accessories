TARGETS	= $(OBJDIR)compare $(OBJDIR)fontsel $(OBJDIR)progress
OBJDIR	=
PREFIX	= /usr/local
DESTDIR	=
BINDIR	= $(PREFIX)/bin
SBINDIR	= $(PREFIX)/sbin
CC	= cc
CPPFLAGSF=
CPPFLAGS=
CFLAGSF	= -W `pkg-config gtk+-2.0 --cflags`
CFLAGS	= -Wall -g -O2 -pedantic
LDFLAGSF= `pkg-config gtk+-2.0 --libs`
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

compare_OBJS = $(OBJDIR)compare.o
compare_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
compare_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)compare: $(compare_OBJS)
	$(CC) -o $(OBJDIR)compare $(compare_OBJS) $(compare_LDFLAGS)

fontsel_OBJS = $(OBJDIR)fontsel.o
fontsel_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
fontsel_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)fontsel: $(fontsel_OBJS)
	$(CC) -o $(OBJDIR)fontsel $(fontsel_OBJS) $(fontsel_LDFLAGS)

progress_OBJS = $(OBJDIR)progress.o
progress_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
progress_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)progress: $(progress_OBJS)
	$(CC) -o $(OBJDIR)progress $(progress_OBJS) $(progress_LDFLAGS)

$(OBJDIR)compare.o: compare.c
	$(CC) $(compare_CFLAGS) -o $(OBJDIR)compare.o -c compare.c

$(OBJDIR)fontsel.o: fontsel.c
	$(CC) $(fontsel_CFLAGS) -o $(OBJDIR)fontsel.o -c fontsel.c

$(OBJDIR)progress.o: progress.c
	$(CC) $(progress_CFLAGS) -o $(OBJDIR)progress.o -c progress.c

clean:
	$(RM) -- $(compare_OBJS) $(fontsel_OBJS) $(progress_OBJS)

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 $(OBJDIR)compare $(DESTDIR)$(BINDIR)/compare
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 $(OBJDIR)fontsel $(DESTDIR)$(BINDIR)/fontsel
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 $(OBJDIR)progress $(DESTDIR)$(BINDIR)/progress

uninstall:
	$(RM) -- $(DESTDIR)$(BINDIR)/compare
	$(RM) -- $(DESTDIR)$(BINDIR)/fontsel
	$(RM) -- $(DESTDIR)$(BINDIR)/progress

.PHONY: all clean distclean install uninstall
