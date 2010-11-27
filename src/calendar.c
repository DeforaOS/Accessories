/* $Id$ */
/* Copyright (c) 2010 Pierre Pronchery <khorben@defora.org> */
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
#include <stdio.h>
#include <gtk/gtk.h>


/* Calendar */
static gboolean _calendar_on_closex(gpointer data);

static int _calendar(void)
{
	GtkWidget * window;
	GtkWidget * calendar;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Calendar");
	g_signal_connect_swapped(G_OBJECT(window), "delete-event", G_CALLBACK(
				_calendar_on_closex), window);
	calendar = gtk_calendar_new();
	gtk_container_add(GTK_CONTAINER(window), calendar);
	gtk_widget_show_all(window);
	gtk_main();
	return 0;
}

static gboolean _calendar_on_closex(gpointer data)
{
	GtkWidget * widget = data;

	gtk_widget_hide(widget);
	gtk_main_quit();
	return FALSE;
}


/* usage */
static int _usage(void)
{
	fputs("Usage: calendar\n", stderr);
	return 1;
}


/* main */
int main(int argc, char * argv[])
{
	int o;

	gtk_init(&argc, &argv);
	while((o = getopt(argc, argv, "")) != -1)
		switch(o)
		{
			default:
				return _usage();
		}
	return (_calendar() == 0) ? 0 : 2;
}
