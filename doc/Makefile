TARGETS	= $(OBJDIR)progress.1 $(OBJDIR)progress.html
OBJDIR	=
PREFIX	= /usr/local
DESTDIR	=
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

$(OBJDIR)progress.1: progress.xml
	./docbook.sh -P "$(PREFIX)" -- "$(OBJDIR)progress.1"

$(OBJDIR)progress.html: progress.css.xml progress.xml manual.css.xml
	./docbook.sh -P "$(PREFIX)" -- "$(OBJDIR)progress.html"

clean:
	$(RM) -- $(progress.1_OBJS) $(progress.html_OBJS)
	./docbook.sh -c -P "$(PREFIX)" -- "progress.1"
	./docbook.sh -c -P "$(PREFIX)" -- "progress.html"

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)progress.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)progress.html"

uninstall:
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "progress.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "progress.html"

.PHONY: all clean distclean install uninstall
