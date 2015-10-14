/* $Id$ */
/* Copyright (c) 2011-2015 Pierre Pronchery <khorben@defora.org> */
/* This file is part of DeforaOS Desktop Accessories */
/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. */



#include <unistd.h>
#include <string.h>
#include <gtk/gtk.h>

#ifndef PROGNAME
# define PROGNAME "compare"
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
	gtk_window_set_title(GTK_WINDOW(window), "Compare strings");
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
	gtk_misc_set_alignment(GTK_MISC(compare.label), 0.0, 0.5);
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
	widget = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	g_signal_connect_swapped(widget, "clicked", G_CALLBACK(
				_compare_on_close), window);
	gtk_container_add(GTK_CONTAINER(bbox), widget);
	gtk_box_pack_end(GTK_BOX(vbox), bbox, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox);
	gtk_widget_show_all(window);
	gtk_main();
	return 0;
}


/* usage */
static int _usage(void)
{
	fputs("Usage: " PROGNAME " [string1 [string2]]\n", stderr);
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
		gtk_label_set_text(label, "The strings MATCH");
		type = GTK_MESSAGE_INFO;
	}
	else
	{
		gtk_label_set_text(label, "The strings do NOT match");
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
