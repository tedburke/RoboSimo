#include <gtk/gtk.h>

void hello(GtkWidget *widget, gpointer data)
{
	g_print("Hello world!\n");
}

gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	g_print("delete event occurred\n");
	return FALSE; // Do destroy the window
}

void destroy(GtkWidget *widget, gpointer data)
{
	gtk_main_quit();
}

int main(int argc, char *argv[])
{
	// These are the widgets we will use
    GtkWidget *window;
	GtkWidget *button;
    
	// Intialise GTK as usual
    gtk_init (&argc, &argv);
    
	// Create and configure the window
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(delete_event), NULL);
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(destroy), NULL);
	gtk_container_set_border_width(GTK_CONTAINER(window), 10);
	
	// Create the button and add it to the window
	button = gtk_button_new_with_label("Hello world!");
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(hello), NULL);	
	gtk_container_add(GTK_CONTAINER(window), button);
	
	// Show all widgets
	gtk_widget_show  (button);
	gtk_widget_show  (window);
    
	// Enter the main GTK event loop
    gtk_main ();
    
    return 0;
}
