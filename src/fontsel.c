/* $Id$ */
/* Copyright (c) 2006-2019 Pierre Pronchery <khorben@defora.org> */
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



#include <unistd.h>
#include <stdio.h>
#include <locale.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include "../config.h"
#define _(string) gettext(string)

/* constants */
#ifndef PROGNAME_FONTSEL
# define PROGNAME_FONTSEL	"fontsel"
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


/* Fontsel */
/* private */
/* prototypes */
static int _fontsel(void);

static int _error(char const * message, int ret);
static int _usage(void);


/* functions */
/* fontsel */
static void _fontsel_on_close(gpointer data);
static gboolean _fontsel_on_closex(GtkWidget * widget);

static int _fontsel(void)
{
	GtkWidget * window;
	GtkWidget * vbox;
	GtkWidget * bbox;
	GtkWidget * widget;

#if GTK_CHECK_VERSION(4, 0, 0)
	window = gtk_window_new();
#else
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
#endif
	gtk_window_set_title(GTK_WINDOW(window), _("Font browser"));
	g_signal_connect(window, "delete-event", G_CALLBACK(_fontsel_on_closex),
			NULL);
#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
	vbox = gtk_vbox_new(FALSE, 4);
#endif
#if !GTK_CHECK_VERSION(4, 0, 0)
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);
#endif
#if GTK_CHECK_VERSION(3, 2, 0)
	widget = gtk_font_chooser_widget_new();
#else
	widget = gtk_font_selection_new();
#endif
#if GTK_CHECK_VERSION(4, 0, 0)
	gtk_box_append(GTK_BOX(vbox), widget);
#else
	gtk_container_add(GTK_CONTAINER(vbox), widget);
#endif
#if GTK_CHECK_VERSION(4, 0, 0)
	bbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
# if GTK_CHECK_VERSION(3, 0, 0)
	bbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
# else
	bbox = gtk_hbutton_box_new();
# endif
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
#endif
#if GTK_CHECK_VERSION(4, 0, 0)
	widget = gtk_button_new_with_label(_("Close"));
#elif GTK_CHECK_VERSION(3, 10, 0)
	widget = gtk_button_new_with_label(_("Close"));
	gtk_button_set_image(GTK_BUTTON(widget),
			gtk_image_new_from_icon_name("gtk-close",
				GTK_ICON_SIZE_BUTTON));
#else
	widget = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
#endif
	g_signal_connect_swapped(widget, "clicked", G_CALLBACK(
				_fontsel_on_close), window);
#if GTK_CHECK_VERSION(4, 0, 0)
	gtk_box_append(GTK_BOX(bbox), widget);
	gtk_box_append(GTK_BOX(vbox), bbox);
	gtk_box_append(GTK_BOX(window), vbox);
#else
	gtk_container_add(GTK_CONTAINER(bbox), widget);
	gtk_box_pack_end(GTK_BOX(vbox), bbox, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox);
#endif
#if GTK_CHECK_VERSION(4, 0, 0)
	while(g_list_model_get_n_items(gtk_window_get_toplevels()) > 0)
		g_main_context_iteration(NULL, TRUE);
#else
	gtk_widget_show_all(window);
	gtk_main();
#endif
	return 0;
}

static void _fontsel_on_close(gpointer data)
{
	GtkWidget * widget = data;

#if GTK_CHECK_VERSION(4, 0, 0)
	gtk_window_destroy(GTK_WINDOW(widget));
#else
	gtk_widget_hide(widget);
	gtk_main_quit();
#endif
}

static gboolean _fontsel_on_closex(GtkWidget * widget)
{
	_fontsel_on_close(widget);
	return TRUE;
}


/* error */
static int _error(char const * message, int ret)
{
	fputs(PROGNAME_FONTSEL ": ", stderr);
	perror(message);
	return ret;
}


/* usage */
static int _usage(void)
{
	fprintf(stderr, _("Usage: %s\n"), PROGNAME_FONTSEL);
	return 1;
}


/* main */
int main(int argc, char * argv[])
{
	int o;

	if(setlocale(LC_ALL, "") == NULL)
		_error("setlocale", 1);
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
#if GTK_CHECK_VERSION(4, 0, 0)
	gtk_init();
#else
	gtk_init(&argc, &argv);
#endif
	while((o = getopt(argc, argv, "")) != -1)
		switch(o)
		{
			default:
				return _usage();
		}
	return (_fontsel() == 0) ? 0 : 2;
}
