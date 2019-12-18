/* $Id$ */
/* Copyright (c) 2011-2019 Pierre Pronchery <khorben@defora.org> */
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
#include <string.h>
#include <locale.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include "../config.h"
#define _(string) gettext(string)

/* constants */
#ifndef PROGNAME_COMPARE
# define PROGNAME_COMPARE	"compare"
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


/* Compare */
/* private */
/* types */
typedef struct _Compare
{
	GtkWidget * entry1;
	GtkWidget * entry2;
#if GTK_CHECK_VERSION(2, 18, 0)
	GtkWidget * infobar;
#endif
	GtkWidget * label;
} Compare;


/* prototypes */
static int _compare(char const * string1, char const * string2);

static int _error(char const * message, int ret);
static int _usage(void);

/* callbacks */
static void _compare_on_changed(gpointer data);
static void _compare_on_close(gpointer data);
static gboolean _compare_on_closex(gpointer data);


/* functions */
/* compare */
static int _compare(char const * string1, char const * string2)
{
	Compare compare;
	GtkWidget * window;
	GtkWidget * vbox;
	GtkWidget * bbox;
	GtkWidget * widget;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER(window), 4);
	gtk_window_set_title(GTK_WINDOW(window), _("Compare strings"));
	g_signal_connect_swapped(G_OBJECT(window), "delete-event", G_CALLBACK(
				_compare_on_closex), window);
#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
	vbox = gtk_vbox_new(FALSE, 4);
#endif
	compare.entry1 = gtk_entry_new();
	if(string1 != NULL)
		gtk_entry_set_text(GTK_ENTRY(compare.entry1), string1);
	g_signal_connect_swapped(compare.entry1, "changed", G_CALLBACK(
				_compare_on_changed), &compare);
	gtk_box_pack_start(GTK_BOX(vbox), compare.entry1, FALSE, TRUE, 0);
	compare.entry2 = gtk_entry_new();
	if(string2 != NULL)
		gtk_entry_set_text(GTK_ENTRY(compare.entry2), string2);
	g_signal_connect_swapped(compare.entry2, "changed", G_CALLBACK(
				_compare_on_changed), &compare);
	gtk_box_pack_start(GTK_BOX(vbox), compare.entry2, FALSE, TRUE, 0);
	compare.label = gtk_label_new(NULL);
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(compare.label, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(compare.label), 0.0, 0.5);
#endif
#if GTK_CHECK_VERSION(2, 18, 0)
	compare.infobar = gtk_info_bar_new();
	widget = gtk_info_bar_get_content_area(GTK_INFO_BAR(compare.infobar));
	gtk_info_bar_set_message_type(GTK_INFO_BAR(compare.infobar),
			GTK_MESSAGE_OTHER);
	gtk_container_add(GTK_CONTAINER(widget), compare.label);
	gtk_box_pack_start(GTK_BOX(vbox), compare.infobar, FALSE, TRUE, 0);
#else
	gtk_box_pack_start(GTK_BOX(vbox), compare.label, FALSE, TRUE, 0);
#endif
#if GTK_CHECK_VERSION(3, 0, 0)
	bbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
#else
	bbox = gtk_hbutton_box_new();
#endif
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
#if GTK_CHECK_VERSION(3, 10, 0)
	widget = gtk_button_new_with_label(_("Close"));
	gtk_button_set_image(GTK_BUTTON(widget),
			gtk_image_new_from_icon_name(GTK_STOCK_CLOSE,
				GTK_ICON_SIZE_BUTTON));
#else
	widget = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
#endif
	g_signal_connect_swapped(widget, "clicked", G_CALLBACK(
				_compare_on_close), window);
	gtk_container_add(GTK_CONTAINER(bbox), widget);
	gtk_box_pack_end(GTK_BOX(vbox), bbox, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox);
	gtk_widget_show_all(window);
	gtk_main();
	return 0;
}


/* error */
static int _error(char const * message, int ret)
{
	fputs(PROGNAME_COMPARE ": ", stderr);
	perror(message);
	return ret;
}


/* usage */
static int _usage(void)
{
	fprintf(stderr, _("Usage: %s [string1 [string2]]\n"), PROGNAME_COMPARE);
	return 1;
}


/* callbacks */
/* compare_on_changed */
static void _compare_on_changed(gpointer data)
{
	Compare * compare = data;
	GtkLabel * label = GTK_LABEL(compare->label);
	char const * string1;
	char const * string2;
	GtkMessageType type = GTK_MESSAGE_OTHER;

	string1 = gtk_entry_get_text(GTK_ENTRY(compare->entry1));
	string2 = gtk_entry_get_text(GTK_ENTRY(compare->entry2));
	if(string1[0] == '\0' && string2[0] == '\0')
		gtk_label_set_text(label, "");
	else if(strcmp(string1, string2) == 0)
	{
		gtk_label_set_text(label, _("The strings MATCH"));
		type = GTK_MESSAGE_INFO;
	}
	else
	{
		gtk_label_set_text(label, _("The strings do NOT match"));
		type = GTK_MESSAGE_ERROR;
	}
#if GTK_CHECK_VERSION(2, 18, 0)
	gtk_info_bar_set_message_type(GTK_INFO_BAR(compare->infobar), type);
#endif
}


/* compare_on_close */
static void _compare_on_close(gpointer data)
{
	GtkWidget * widget = data;

	gtk_widget_hide(widget);
	gtk_main_quit();
}


/* compare_on_closex */
static gboolean _compare_on_closex(gpointer data)
{
	GtkWidget * widget = data;

	_compare_on_close(widget);
	return FALSE;
}


/* main */
int main(int argc, char * argv[])
{
	int o;
	char const * string1;
	char const * string2 = NULL;

	if(setlocale(LC_ALL, "") == NULL)
		_error("setlocale", 1);
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	gtk_init(&argc, &argv);
	while((o = getopt(argc, argv, "")) != -1)
		switch(o)
		{
			default:
				return _usage();
		}
	if((string1 = argv[optind]) != NULL)
		string2 = argv[optind + 1];
	return (_compare(string1, string2) == 0) ? 0 : 2;
}
