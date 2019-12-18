/* $Id$ */
/* Copyright (c) 2007-2018 Pierre Pronchery <khorben@defora.org> */
/* This file is part of DeforaOS Desktop Accessories */
/* All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */
/* TODO:
 * - report errors when relevant upon EOF when outputting */



#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <locale.h>
#include <libintl.h>
#include <gtk/gtk.h>
#if GTK_CHECK_VERSION(3, 0, 0)
# include <gtk/gtkx.h>
#endif
#include "../config.h"
#define _(string) gettext(string)

/* constants */
#ifndef PROGNAME_GUNZIP
# define PROGNAME_GUNZIP	"gunzip"
#endif
#ifndef PROGNAME_GZIP
# define PROGNAME_GZIP		"gzip"
#endif
#ifndef PROGNAME_PROGRESS
# define PROGNAME_PROGRESS	"progress"
#endif
#ifndef PREFIX
# define PREFIX			"/usr/local"
#endif
#ifndef DATADIR
# define DATADIR		PREFIX "/share"
#endif
#ifndef LOCALEDIR
# define LOCALEDIR		DATADIR "/locale"
#endif


/* types */
typedef struct _Prefs
{
	int flags;
	ssize_t bufsiz;
	char const * filename;
	char const * prefix;
	char const * title;
	size_t length;
} Prefs;
#define PREFS_x 0x1
#define PREFS_z 0x2


/* progress */
/* types */
typedef struct _Progress
{
	Prefs * prefs;			/* preferences		*/

	struct timeval tv;		/* start time		*/
	int fd;				/* read descriptor	*/
	int eof;			/* end of file 		*/
	int ret;
	int fds[2];			/* for the pipe		*/
	pid_t pid;			/* child's pid		*/
	size_t cnt;			/* bytes written	*/
	char * buf;
	size_t bufsiz;
	size_t buf_cnt;
	GIOChannel * in_channel;
	guint in_id;
	GIOChannel * out_channel;
	guint out_id;

	/* widgets */
	GtkWidget * window;
	GtkWidget * done;
	GtkWidget * remaining;
	GtkWidget * progress;
	int pulse;			/* tells when to pulse	*/
} Progress;


/* prototypes */
static int _error(char const * message, int ret);
static int _usage(void);


/* functions */
/* progress */
static int _progress_error(Progress * progress, char const * message, int ret);
static int _progress_gerror(Progress * progress, char const * message,
		GError * error, int ret);
static int _progress_exec(Progress * progress, char * argv[]);

/* callbacks */
static gboolean _progress_closex(gpointer data);
static void _progress_cancel(void);
static gboolean _progress_channel(GIOChannel * source, GIOCondition condition,
		gpointer data);
static void _progress_embedded(gpointer data);
static gboolean _progress_idle_in(gpointer data);
static gboolean _progress_idle_out(gpointer data);
static gboolean _progress_timeout(gpointer data);

static int _progress(Prefs * prefs, char * argv[])
{
	Progress p;
	struct stat st;
	GtkWidget * vbox;
	GtkWidget * hbox;
	GtkSizeGroup * left;
	GtkSizeGroup * right;
	GtkWidget * widget;
	PangoFontDescription * bold;
	char const * q;
	unsigned long id;
  
	memset(&p, 0, sizeof(p));
	p.prefs = prefs;
	if(prefs->bufsiz == 0)
		errno = EINVAL;
	if(prefs->bufsiz == 0 || (p.buf = malloc(prefs->bufsiz)) == NULL)
		return _progress_error(&p, "malloc", 1);
	p.bufsiz = prefs->bufsiz;
	if(pipe(p.fds) != 0)
		return _progress_error(&p, "pipe", 1);
	if((p.pid = fork()) == -1)
	{
		close(p.fds[0]);
		close(p.fds[1]);
		return _progress_error(&p, "fork", 1);
	}
	if(p.pid != 0)
		return _progress_exec(&p, argv);
	close(p.fds[0]);
	if(gettimeofday(&p.tv, NULL) != 0)
		return _progress_error(&p, "gettimeofday", 1);
	if(prefs->filename == NULL)
		prefs->filename = _("Standard input");
	else if((p.fd = open(prefs->filename, O_RDONLY)) < 0)
		return _progress_error(&p, prefs->filename, 1);
	else if(fstat(p.fd, &st) == 0 && S_ISREG(st.st_mode))
		prefs->length = st.st_size;
	p.in_channel = g_io_channel_unix_new(p.fd);
	g_io_channel_set_encoding(p.in_channel, NULL, NULL);
	p.in_id = 0;
	g_idle_add(_progress_idle_in, &p);
	p.out_channel = g_io_channel_unix_new(p.fds[1]);
	g_io_channel_set_encoding(p.out_channel, NULL, NULL);
	p.out_id = 0;
	/* graphical interface */
	if((prefs->flags & PREFS_x) == 0)
	{
		p.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
#if GTK_CHECK_VERSION(3, 0, 0) && !GTK_CHECK_VERSION(3, 14, 0)
		gtk_window_set_has_resize_grip(GTK_WINDOW(p.window), FALSE);
#endif
		gtk_window_set_title(GTK_WINDOW(p.window), prefs->title != NULL
				? prefs->title : _("Progress"));
		g_signal_connect_swapped(p.window, "delete-event", G_CALLBACK(
					_progress_closex), p.window);
	}
	else
	{
		p.window = gtk_plug_new(0);
		g_signal_connect_swapped(p.window, "embedded", G_CALLBACK(
					_progress_embedded), &p);
	}
#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
	vbox = gtk_vbox_new(FALSE, 0);
	hbox = gtk_hbox_new(FALSE, 0);
#endif
	left = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	right = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	/* file */
	widget = gtk_label_new(_("File: "));
	bold = pango_font_description_new();
	pango_font_description_set_weight(bold, PANGO_WEIGHT_BOLD);
#if GTK_CHECK_VERSION(3, 0, 0)
	gtk_widget_override_font(widget, bold);
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_widget_modify_font(widget, bold);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(left, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	if((q = g_filename_to_utf8(prefs->filename, -1, NULL, NULL, NULL))
			== NULL)
		q = prefs->filename;
	widget = gtk_label_new(q);
	gtk_label_set_ellipsize(GTK_LABEL(widget), PANGO_ELLIPSIZE_MIDDLE);
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(right, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	/* done */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
	hbox = gtk_hbox_new(FALSE, 0);
#endif
	widget = gtk_label_new(_("Done: "));
#if GTK_CHECK_VERSION(3, 0, 0)
	gtk_widget_override_font(widget, bold);
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_widget_modify_font(widget, bold);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(left, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	p.done = gtk_label_new(_("0.0 kB"));
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(p.done, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(p.done), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(right, p.done);
	gtk_box_pack_start(GTK_BOX(hbox), p.done, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 4);
	/* remaining */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
	hbox = gtk_hbox_new(FALSE, 0);
#endif
	widget = gtk_label_new(_("Remaining: "));
#if GTK_CHECK_VERSION(3, 0, 0)
	gtk_widget_override_font(widget, bold);
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_widget_modify_font(widget, bold);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(left, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	p.remaining = gtk_label_new("");
	g_timeout_add(250, _progress_timeout, &p);
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(p.remaining, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(p.remaining), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(right, p.remaining);
	gtk_box_pack_start(GTK_BOX(hbox), p.remaining, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 4);
	/* progress */
	p.progress = gtk_progress_bar_new();
	p.pulse = 0;
	if(prefs->prefix != NULL)
	{
#if GTK_CHECK_VERSION(3, 0, 0)
		hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
		hbox = gtk_hbox_new(FALSE, 0);
#endif
		widget = gtk_label_new(prefs->prefix);
#if GTK_CHECK_VERSION(3, 0, 0)
		g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
		gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
		gtk_size_group_add_widget(left, widget);
		gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
		gtk_size_group_add_widget(right, p.progress);
		gtk_box_pack_start(GTK_BOX(hbox), p.progress, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 4);
	}
	else
		gtk_box_pack_start(GTK_BOX(vbox), p.progress, TRUE, TRUE, 4);
	/* cancel */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
	hbox = gtk_hbox_new(FALSE, 0);
#endif
#if GTK_CHECK_VERSION(3, 10, 0)
	widget = gtk_button_new_with_label(_("Cancel"));
	gtk_button_set_image(GTK_BUTTON(widget),
			gtk_image_new_from_icon_name(GTK_STOCK_CANCEL,
				GTK_ICON_SIZE_BUTTON));
#else
	widget = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
#endif
	g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(
				_progress_cancel), NULL);
	gtk_box_pack_end(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(p.window), vbox);
	gtk_container_set_border_width(GTK_CONTAINER(p.window), 4);
	gtk_widget_show_all(vbox);
	if((prefs->flags & PREFS_x) == 0)
		/* show the window */
		gtk_widget_show(p.window);
	else
	{
		/* print the window ID and force a flush */
		id = gtk_plug_get_id(GTK_PLUG(p.window));
		printf("%lu\n", id);
		fclose(stdout);
	}
	gtk_main();
	close(p.fd);
	close(p.fds[1]);
	return p.ret;
}

static int _error_do(Progress * progress, char const * message,
		char const * error, int ret);

static int _progress_error(Progress * progress, char const * message,
		int ret)
{
	return _error_do(progress, message, strerror(errno), ret);
}

static int _progress_gerror(Progress * progress, char const * message,
		GError * error, int ret)
{
	_error_do(progress, message, error->message, ret);
	g_error_free(error);
	return ret;
}

static int _error_do(Progress * progress, char const * message,
		char const * error, int ret)
{
	GtkWidget * dialog;

	if(progress != NULL)
		progress->ret = ret;
	if(ret < 0)
	{
		fputs(PROGNAME_PROGRESS ": ", stderr);
		perror(message);
		return -ret;
	}
	dialog = gtk_message_dialog_new((progress != NULL)
			? GTK_WINDOW(progress->window) : NULL,
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
#if GTK_CHECK_VERSION(2, 6, 0)
			"%s", _("Error"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
#endif
			"%s: %s", message, error);
	gtk_window_set_title(GTK_WINDOW(dialog), _("Error"));
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	return ret;
}

static void _progress_embedded(gpointer data)
{
	Progress * p = data;

	gtk_widget_show(p->window);
}

static gboolean _progress_idle_in(gpointer data)
{
	Progress * p = data;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() in_id=%u\n", __func__, p->in_id);
#endif
	if(p->in_id == 0)
		p->in_id = g_io_add_watch(p->in_channel, G_IO_IN,
				_progress_channel, p);
	return FALSE;
}

static gboolean _progress_idle_out(gpointer data)
{
	Progress * p = data;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() out_id=%u\n", __func__, p->out_id);
#endif
	if(p->out_id == 0)
		p->out_id = g_io_add_watch(p->out_channel, G_IO_OUT,
				_progress_channel, p);
	return FALSE;
}


/* progress_exec */
static int _exec_gunzip(Progress * progress, char * argv[]);

static int _progress_exec(Progress * progress, char * argv[])
{
	close(progress->fds[1]);
	if(dup2(progress->fds[0], 0) == -1)
	{
		perror("dup2");
		exit(1); /* FIXME warn user */
	}
	if(progress->prefs->flags & PREFS_z)
		_exec_gunzip(progress, argv);
	else
		execvp(argv[0], argv);
	exit(_progress_error(NULL, argv[0], -1));
	return 1;
}

static int _exec_gunzip(Progress * progress, char * argv[])
{
	static Progress tmp;

	tmp.prefs = progress->prefs;
	tmp.prefs->flags -= PREFS_z;
	if(pipe(tmp.fds) != 0)
		exit(_progress_error(NULL, "pipe", -1));
	if((tmp.pid = fork()) == -1)
		exit(_progress_error(NULL, "fork", -1));
	if(tmp.pid == 0)
		return _progress_exec(&tmp, argv);
	close(tmp.fds[0]);
	if(dup2(tmp.fds[1], 1) == -1)
		exit(_progress_error(NULL, "dup2", -1));
	execlp(PROGNAME_GUNZIP, PROGNAME_GUNZIP, NULL);
	execlp(PROGNAME_GZIP, PROGNAME_GZIP, "-d", NULL);
	exit(_progress_error(NULL, PROGNAME_GUNZIP, -1));
	return 1;
}


/* callbacks */
static gboolean _progress_closex(gpointer data)
{
	GtkWidget * widget = data;

	gtk_widget_hide(widget);
	gtk_main_quit();
	return FALSE;
}

static void _progress_cancel(void)
{
	gtk_main_quit();
}


/* progress_out */
static gboolean _channel_in(Progress * p, GIOChannel * source);
static gboolean _channel_out(Progress * p, GIOChannel * source);
static void _out_rate(Progress * p);

static gboolean _progress_channel(GIOChannel * source, GIOCondition condition,
		gpointer data)
{
	Progress * p = data;

	if(condition == G_IO_IN)
		return _channel_in(p, source);
	if(condition == G_IO_OUT)
		return _channel_out(p, source);
	_progress_error(p, p->prefs->filename, 0);
	gtk_main_quit();
	return FALSE;
}

static gboolean _channel_in(Progress * p, GIOChannel * source)
{
	GIOStatus status;
	gsize read;
	GError * error = NULL;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	p->in_id = 0;
	status = g_io_channel_read_chars(source, &p->buf[p->buf_cnt],
			p->bufsiz - p->buf_cnt, &read, &error);
	if(status == G_IO_STATUS_ERROR)
	{
		_progress_gerror(p, p->prefs->filename, error, 1);
		g_io_channel_shutdown(source, TRUE, NULL);
		gtk_main_quit();
		return FALSE;
	}
	else if(status == G_IO_STATUS_EOF)
	{
		p->eof = 1; /* reached end of input file */
		g_io_channel_shutdown(source, TRUE, NULL);
	}
	else if(p->buf_cnt + read != p->bufsiz)
		g_idle_add(_progress_idle_in, p); /* continue to read */
	if(p->buf_cnt == 0)
		g_idle_add(_progress_idle_out, p); /* begin to write */
	p->buf_cnt += read;
	return FALSE;
}

static gboolean _channel_out(Progress * p, GIOChannel * source)
{
	GIOStatus status;
	gsize written;
	GError * error = NULL;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	p->out_id = 0;
	/* write data */
	status = g_io_channel_write_chars(source, p->buf, p->buf_cnt, &written,
				&error);
	if(status == G_IO_STATUS_ERROR)
	{
		_progress_gerror(p, p->prefs->filename, error, 1);
		gtk_main_quit();
		return FALSE;
	}
	else if(status == G_IO_STATUS_EOF)
	{
		p->eof = 1; /* reached end of output file */
		_progress_error(p, p->prefs->filename, 1);
		g_io_channel_shutdown(source, TRUE, NULL);
	}
	else if(p->buf_cnt == p->bufsiz)
		g_idle_add(_progress_idle_in, p); /* read again */
	p->buf_cnt -= written;
	memmove(p->buf, &p->buf[written], p->buf_cnt);
	p->cnt += written;
	_out_rate(p);
	if(p->buf_cnt > 0)
		g_idle_add(_progress_idle_out, p); /* continue to write */
	else if(p->eof == 1) /* reached end of output */
	{
		g_io_channel_shutdown(p->out_channel, TRUE, NULL);
		gtk_main_quit();
	}
	return FALSE;
}

static void _out_rate(Progress * p)
{
	gdouble fraction;
	GtkProgressBar * bar = GTK_PROGRESS_BAR(p->progress);
	char buf[16];

	if(p->prefs->length == 0 || p->cnt == 0)
	{
		p->pulse = 1;
		return;
	}
	fraction = p->cnt;
	fraction /= p->prefs->length;
	if(gtk_progress_bar_get_fraction(bar) == fraction)
		return;
	gtk_progress_bar_set_fraction(bar, fraction);
#if GTK_CHECK_VERSION(3, 0, 0)
	gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(p->progress), TRUE);
#endif
	snprintf(buf, sizeof(buf), "%.1f%%", fraction * 100);
	gtk_progress_bar_set_text(bar, buf);
}


/* progress_timeout */
static void _timeout_done(Progress * progress, guint64 * rate);
static void _timeout_progress(Progress * progress);
static void _timeout_remaining(Progress * progress, guint64 rate);

static gboolean _progress_timeout(gpointer data)
{
	Progress * progress = data;
	guint64 rate = 0;

	_timeout_done(progress, &rate);
	_timeout_remaining(progress, rate);
	_timeout_progress(progress);
	return TRUE;
}

static void _timeout_done(Progress * progress, guint64 * rate)
{
	double cnt = progress->cnt / 1024;
	double total = progress->prefs->length / 1024;
	char buf[48];
	char const * dunit = _("kB");
	struct timeval tv;
	double r;
	char const * sunit = _("kB");

	if(progress->prefs->length > 1048576 || progress->cnt > 1048576)
	{
		cnt /= 1024;
		total /= 1024;
		dunit = _("MB");
	}
	if(gettimeofday(&tv, NULL) != 0)
	{
		_progress_error(progress, "gettimeofday", FALSE);
		return;
	}
	if((tv.tv_sec = tv.tv_sec - progress->tv.tv_sec) < 0)
		tv.tv_sec = 0;
	if((tv.tv_usec = tv.tv_usec - progress->tv.tv_usec) < 0)
	{
		tv.tv_sec--;
		tv.tv_usec += 1000000;
	}
	if((r = (tv.tv_sec * 1000) + (tv.tv_usec / 1000)) > 0.0)
	{
		*rate = (progress->cnt * 1024) / r;
		if((r = progress->cnt / r) > 1024.0)
		{
			r /= 1024.0;
			sunit = _("MB");
		}
	}
	if(progress->prefs->length == 0)
		snprintf(buf, sizeof(buf), _("%.1f %s (%.1f %s/s)"), cnt, dunit,
				r, sunit);
	else
		snprintf(buf, sizeof(buf), _("%.1f of %.1f %s (%.1f %s/s)"),
				cnt, total, dunit, r, sunit);
	gtk_label_set_text(GTK_LABEL(progress->done), buf);
}

static void _timeout_progress(Progress * progress)
{
	if(progress->pulse != 1)
		return; /* setting the fraction is done somewhere else */
	gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progress->progress));
	progress->pulse = 0;
}

static void _timeout_remaining(Progress * progress, guint64 rate)
{
	char buf[32];
	guint64 remaining;
	struct tm tm;

	if(progress->prefs->length == 0 || rate == 0)
	{
		gtk_label_set_text(GTK_LABEL(progress->remaining),
				_("Unknown"));
		return;
	}
	remaining = (progress->prefs->length - progress->cnt) / rate;
	memset(&tm, 0, sizeof(tm));
	tm.tm_sec = remaining;
	/* minutes */
	if(tm.tm_sec > 60)
	{
		tm.tm_min = tm.tm_sec / 60;
		tm.tm_sec = tm.tm_sec - (tm.tm_min * 60);
	}
	/* hours */
	if(tm.tm_min > 60)
	{
		tm.tm_hour = tm.tm_min / 60;
		tm.tm_min = tm.tm_min - (tm.tm_hour * 60);
	}
	strftime(buf, sizeof(buf), _("%H:%M:%S"), &tm);
	gtk_label_set_text(GTK_LABEL(progress->remaining), buf);
}


/* error */
static int _error(char const * message, int ret)
{
	fputs(PROGNAME_PROGRESS ": ", stderr);
	perror(message);
	return ret;
}


/* usage */
static int _usage(void)
{
	fprintf(stderr, _("Usage: %s [-x][-ez][-b buffer size][-f file]"
"[-l length][-p prefix]\n"
"                [-t title] command [args...]\n"
"  -e	Ignored (for compatibility)\n"
"  -x	Start in embedded mode\n"), PROGNAME_PROGRESS);
	return 1;
}


/* main */
int main(int argc, char * argv[])
{
	Prefs prefs;
	int o;
	char * p;

	memset(&prefs, 0, sizeof(prefs));
	prefs.bufsiz = 65536;
	if(setlocale(LC_ALL, "") == NULL)
		_error("setlocale", 1);
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	gtk_init(&argc, &argv);
	while((o = getopt(argc, argv, "b:ef:l:p:t:xz")) != -1)
		switch(o)
		{
			case 'b':
				prefs.bufsiz = strtol(optarg, &p, 0);
				if(optarg[0] == '\0' || *p != '\0'
						|| prefs.bufsiz <= 0)
					return _usage();
				break;
			case 'e':
				break;
			case 'f':
				prefs.filename = optarg;
				break;
			case 'l':
				prefs.length = strtol(optarg, &p, 0);
				if(optarg[0] == '\0' || *p != '\0')
					return _usage();
				break;
			case 'p':
				prefs.prefix = optarg;
				break;
			case 't':
				prefs.title = optarg;
				break;
			case 'x':
				prefs.flags |= PREFS_x;
				break;
			case 'z':
				prefs.flags |= PREFS_z;
				break;
			default:
				return _usage();
		}
	if(argc - optind < 1)
		return _usage();
	return (_progress(&prefs, &argv[optind]) == 0) ? 0 : 2;
}
