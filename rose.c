#include "rose.h"

#define MSGBUFSZ 8
#define LENGTH(x) (sizeof(x) / sizeof(x[0]))

Display *glob_dpy;  /* defined in rose.h */

static guint glob_xid;
static Atom glob_atoms[AtomLast];


void setatom(int a, const char *v)
{
	XChangeProperty(glob_dpy, glob_xid,
		glob_atoms[a], glob_atoms[AtomUTF8], 8, PropModeReplace,
		(unsigned char *)v, strlen(v) + 1);
	XSync(glob_dpy, False);
}

char* getatom(int a)
{
	Atom adummy;
	int idummy;
	unsigned long ldummy;
	unsigned char *p = NULL;
	char *ret;

	XSync(glob_dpy, False);
	XGetWindowProperty(glob_dpy, glob_xid,
		glob_atoms[a], 0L, BUFSIZ, False, glob_atoms[AtomUTF8],
		&adummy, &idummy, &ldummy, &ldummy, &p);

	/* We need to strdup() the returned string, because the spec says we _have_
	   to use XFree(). Although it's probably a regular free() call, we can make
	   sure by just strdup'ing it into our own buffer. */
	ret = strdup((char *) p);
	XFree(p);
	return ret;
}

static void setup()
{
	if (!(glob_dpy = XOpenDisplay(NULL))) {
		puts("Can't open default display");
		exit(1);
	}

	glob_atoms[AtomFind] = XInternAtom(glob_dpy, "_ROSE_FIND", False);
	glob_atoms[AtomGo] = XInternAtom(glob_dpy, "_ROSE_GO", False);
	glob_atoms[AtomUri] = XInternAtom(glob_dpy, "_ROSE_URI", False);
}

static void run(GtkApplication *app)
{
	RoseWindow *window = rose_window_new(app);

	if (appearance[DARKMODE])
		g_object_set(gtk_settings_get_default(), "gtk-application-prefer-dark-theme", true, NULL);

	if (!options[HOMEPAGE])
		options[HOMEPAGE] = "https://duckduckgo.com";

	glob_xid = rose_window_show(app, window, options[HOMEPAGE]);
}

int main(int argc, char **argv)
{
	if (argc == 2) {
		options[HOMEPAGE] = argv[1];
		argv++; argc--;
	}

	setup();
	GtkApplication *app = gtk_application_new("org.gtk.rose", G_APPLICATION_NON_UNIQUE);
	g_signal_connect(app, "activate", G_CALLBACK(run), NULL);
	g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);
}
