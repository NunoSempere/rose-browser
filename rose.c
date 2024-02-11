#include <stdlib.h>
#include <string.h>
#include <webkit2/webkit2.h>
#include "config.h"
#include "plugins/libre_redirect/libre_redirect.h"
#include "plugins/readability/readability.h"
#include "plugins/shortcuts/shortcuts.h"
#include "plugins/style/style.h"

/* Global declarations */
static GtkNotebook* notebook;
static GtkWindow* window;

// Search, find and url bar
static struct {
    GtkHeaderBar *header;
    GtkEntry *bar_line;
    GtkEntryBuffer *bar_line_text;
    enum { _SEARCH, _FIND, _HIDDEN } entry_mode;
} bar;

/* Plugins */
#define LIBRE_REDIRECT_ENABLED true
#define READABILITY_ENABLED true
#define CUSTOM_STYLE_ENABLED true
#define CUSTOM_USER_AGENT false
static int num_tabs = 0;
/*
To disable plugins:
1. set their corresponding variable to false
2. you could also look into this file at commit afe93518a for an approach using stand-in code.
3. recompile 

To remove plugins completely;
1. Remove the corresponding code in this file by looking for the variables above.
2. Remove PLUGIN and $(PLUGIN) from the makefiel
3. Recompile
*/

WebKitWebView* webview_new()
{
    char* style;
    WebKitSettings* settings;
    WebKitWebContext* web_context;
    WebKitCookieManager* cookiemanager;
    WebKitUserContentManager* contentmanager;

    settings = webkit_settings_new_with_settings(WEBKIT_DEFAULT_SETTINGS, NULL);
    if (CUSTOM_USER_AGENT) {
        webkit_settings_set_user_agent(
            settings,
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, "
            "like Gecko) Chrome/110.0.0.0 Safari/537.36");
        // See: <https://www.useragents.me/> for some common user agents
    }
    web_context = webkit_web_context_new_with_website_data_manager(
        webkit_website_data_manager_new(CACHE, NULL));
    contentmanager = webkit_user_content_manager_new();
    cookiemanager = webkit_web_context_get_cookie_manager(web_context);

    webkit_cookie_manager_set_persistent_storage(
        cookiemanager, CACHE_DIR "/cookies.sqlite",
        WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE);

    webkit_cookie_manager_set_accept_policy(cookiemanager,
        WEBKIT_COOKIE_POLICY_ACCEPT_ALWAYS);

    webkit_web_context_set_process_model(
        web_context, WEBKIT_PROCESS_MODEL_MULTIPLE_SECONDARY_PROCESSES);

    if (g_file_get_contents("~/.config/rose/style.css", &style, NULL, NULL))
        webkit_user_content_manager_add_style_sheet(
            contentmanager, webkit_user_style_sheet_new(style, WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES, WEBKIT_USER_STYLE_LEVEL_USER, NULL, NULL));

    return g_object_new(WEBKIT_TYPE_WEB_VIEW, "settings", settings, "web-context",
        web_context, "user-content-manager", contentmanager,
        NULL);
}

WebKitWebView* notebook_get_webview(GtkNotebook* notebook)
{
    return WEBKIT_WEB_VIEW(gtk_notebook_get_nth_page(
        notebook, gtk_notebook_get_current_page(notebook)));
}

void load_uri(WebKitWebView* view, const char* uri)
{
    if (g_str_has_prefix(uri, "http://") || g_str_has_prefix(uri, "https://") || g_str_has_prefix(uri, "file://") || g_str_has_prefix(uri, "about:")) {
        webkit_web_view_load_uri(view, uri);
    } else {
        // Check for shortcuts
        int l = SHORTCUT_N + strlen(uri) + 1;
        char uri_expanded[l];
        str_init(uri_expanded, l);
        int check = shortcut_expand(uri, uri_expanded);
        if (check == 2) {
            webkit_web_view_load_uri(view, uri_expanded);
        } else {
            // Feed into search engine.
            char tmp[strlen(uri) + strlen(SEARCH)];
            snprintf(tmp, sizeof(tmp), SEARCH, uri);
            webkit_web_view_load_uri(view, tmp);
        }
    }
}

void redirect_if_annoying(WebKitWebView* view, const char* uri)
{
    int l = LIBRE_N + strlen(uri) + 1;
    char uri_filtered[l];
    str_init(uri_filtered, l);

    int check = libre_redirect(uri, uri_filtered);

    if (check == 2) {
        webkit_web_view_load_uri(view, uri_filtered);
    }
}

void load_changed(WebKitWebView* self, WebKitLoadEvent load_event,
    GtkNotebook* notebook)
{
    switch (load_event) {
        /* see <https://webkitgtk.org/reference/webkit2gtk/2.5.1/WebKitWebView.html>
     */
    case WEBKIT_LOAD_STARTED:
        if (CUSTOM_STYLE_ENABLED) {
            char* style_js = malloc(STYLE_N + 1);
            read_style_js(style_js);
            webkit_web_view_run_javascript(notebook_get_webview(notebook), style_js,
                NULL, NULL, NULL);
            free(style_js);
        }
        if (LIBRE_REDIRECT_ENABLED) {
            redirect_if_annoying(self, webkit_web_view_get_uri(self));
        }
        break;
    case WEBKIT_LOAD_REDIRECTED:
        if (LIBRE_REDIRECT_ENABLED) {
            redirect_if_annoying(self, webkit_web_view_get_uri(self));
        }
        break;
    case WEBKIT_LOAD_COMMITTED:
        if (LIBRE_REDIRECT_ENABLED) {
            redirect_if_annoying(self, webkit_web_view_get_uri(self));
        }
        if (CUSTOM_STYLE_ENABLED) {
            char* style_js = malloc(STYLE_N + 1);
            read_style_js(style_js);
            webkit_web_view_run_javascript(notebook_get_webview(notebook), style_js,
                NULL, NULL, NULL);
            free(style_js);
        }
        break;
    case WEBKIT_LOAD_FINISHED: {
        /* Add gtk tab title */
        const char* webpage_title = webkit_web_view_get_title(self);
        const int max_length = 25;
        char tab_title[max_length + 1];
        if (webpage_title != NULL) {
            for (int i = 0; i < (max_length); i++) {
                tab_title[i] = webpage_title[i];
                if (webpage_title[i] == '\0') {
                    break;
                }
            }
            tab_title[max_length] = '\0';
        }

        gtk_notebook_set_tab_label_text(notebook, GTK_WIDGET(self),
            webpage_title == NULL ? "—" : tab_title);
        // gtk_widget_hide(GTK_WIDGET(bar));
    }
    }
}

void notebook_append(GtkNotebook* notebook, const char* uri);
/* notebook_append calls handle_create, but  handle_create also calls
 * notebook_append. Therefore we need to declare notebook_append, so that
 * handle_create_new_tab knows its type.
 */

GtkWidget* handle_create_new_tab(WebKitWebView* self,
    WebKitNavigationAction* navigation_action,
    GtkNotebook* notebook)
{
    if (num_tabs < MAX_NUM_TABS || num_tabs == 0) {
        WebKitURIRequest* uri_request = webkit_navigation_action_get_request(navigation_action);
        const char* uri = webkit_uri_request_get_uri(uri_request);
        printf("Creating new window: %s\n", uri);
        notebook_append(notebook, uri);
        gtk_notebook_set_show_tabs(notebook, true);
        return NULL;
    } else {
        webkit_web_view_run_javascript(notebook_get_webview(notebook),
            "alert('Too many tabs, not opening a new one')", NULL, NULL, NULL);
        return NULL;
    }
    /* WebKitGTK documentation recommends returning the new webview.
   * I imagine that this might allow e.g., to go back in a new tab
   * or generally to keep track of history.
   * However, this would require either modifying notebook_append
   * or duplicating its contents, for unclear gain.
   */
}

void notebook_append(GtkNotebook* notebook, const char* uri)
{
    if (num_tabs < MAX_NUM_TABS || num_tabs == 0) {
        GdkScreen* screen = gtk_window_get_screen(GTK_WINDOW(window));
        GdkVisual* rgba_visual = gdk_screen_get_rgba_visual(screen);
        GdkRGBA rgba;

        gdk_rgba_parse(&rgba, BG_COLOR);

        WebKitWebView* view = webview_new();

        gtk_widget_set_visual(GTK_WIDGET(window), rgba_visual);
        g_signal_connect(view, "load_changed", G_CALLBACK(load_changed), notebook);
        g_signal_connect(view, "create", G_CALLBACK(handle_create_new_tab), notebook);

        int n = gtk_notebook_append_page(notebook, GTK_WIDGET(view), NULL);
        gtk_notebook_set_tab_reorderable(notebook, GTK_WIDGET(view), true);
        gtk_widget_show_all(GTK_WIDGET(window));
        gtk_widget_hide(GTK_WIDGET(bar.header));
        webkit_web_view_set_background_color(view, &rgba);
        load_uri(view, (uri) ? uri : HOME);

        if (CUSTOM_STYLE_ENABLED) {
            char* style_js = malloc(STYLE_N + 1);
            read_style_js(style_js);
            webkit_web_view_run_javascript(notebook_get_webview(notebook), style_js,
                NULL, NULL, NULL);
            free(style_js);
        }

        gtk_notebook_set_current_page(notebook, n);
        gtk_notebook_set_tab_label_text(notebook, GTK_WIDGET(view), "-");
        webkit_web_view_set_zoom_level(view, ZOOM);
        num_tabs += 1;
    } else {
        webkit_web_view_run_javascript(notebook_get_webview(notebook),
            "alert('Too many tabs, not opening a new one')", NULL, NULL, NULL);
    }
}

void show_bar(GtkNotebook* notebook)
{
    if (bar.entry_mode == _SEARCH) {
        const char* url = webkit_web_view_get_uri(notebook_get_webview(notebook));
        gtk_entry_set_placeholder_text(bar.bar_line, "Search");
        gtk_entry_buffer_set_text(bar.bar_line_text, url, strlen(url));
        gtk_widget_show(GTK_WIDGET(bar.header));
        gtk_window_set_focus(window, GTK_WIDGET(bar.bar_line));
    } else if (bar.entry_mode == _HIDDEN) {
        gtk_widget_hide(GTK_WIDGET(bar.header));
    } else {
        const char* search_text = webkit_find_controller_get_search_text(
            webkit_web_view_get_find_controller(notebook_get_webview(notebook)));

        if (search_text != NULL)
            gtk_entry_buffer_set_text(bar.bar_line_text, search_text, strlen(search_text));

        gtk_entry_set_placeholder_text(bar.bar_line, "Find");
        gtk_widget_show(GTK_WIDGET(bar.header));
        gtk_window_set_focus(window, GTK_WIDGET(bar.bar_line));
    }
}

int handle_key(func id, GtkNotebook* notebook)
{
    static double zoom = ZOOM;
    static bool is_fullscreen = 0;

    switch (id) {
    case goback:
        webkit_web_view_go_back(notebook_get_webview(notebook));
        break;
    case goforward:
        webkit_web_view_go_forward(notebook_get_webview(notebook));
        break;

    case refresh:
        webkit_web_view_reload(notebook_get_webview(notebook));
        break;
    case refresh_force:
        webkit_web_view_reload_bypass_cache(notebook_get_webview(notebook));
        break;

    case back_to_home:
        load_uri(notebook_get_webview(notebook), HOME);
        break;

    case zoomin:
        webkit_web_view_set_zoom_level(notebook_get_webview(notebook),
            (zoom += ZOOM_VAL));
        break;

    case zoomout:
        webkit_web_view_set_zoom_level(notebook_get_webview(notebook),
            (zoom -= ZOOM_VAL));
        break;

    case zoom_reset:
        webkit_web_view_set_zoom_level(notebook_get_webview(notebook),
            (zoom = ZOOM));
        break;

    case prev_tab:; // declarations aren't statements
        // <https://stackoverflow.com/questions/92396/why-cant-variables-be-declared-in-a-switch-statement>
        int n = gtk_notebook_get_n_pages(notebook);
        int k = gtk_notebook_get_current_page(notebook);
        int l = (n + k - 1) % n;
        gtk_notebook_set_current_page(notebook, l);
        break;

    case next_tab:;
        int m = gtk_notebook_get_n_pages(notebook);
        int i = gtk_notebook_get_current_page(notebook);
        int j = (i + 1) % m;
        gtk_notebook_set_current_page(notebook, j);
        break;

    case close_tab:
        gtk_notebook_remove_page(notebook, gtk_notebook_get_current_page(notebook));
        num_tabs -= 1;

        switch (gtk_notebook_get_n_pages(notebook)) {
        case 0:
            exit(0);
            break;
        case 1:
            gtk_notebook_set_show_tabs(notebook, false);
            break;
        }

        break;

    case toggle_fullscreen:
        if (is_fullscreen)
            gtk_window_unfullscreen(window);
        else
            gtk_window_fullscreen(window);

        is_fullscreen = !is_fullscreen;
        break;

    case show_searchbar:
        bar.entry_mode = _SEARCH;
        show_bar(notebook);
        break;

    case show_finder:
        bar.entry_mode = _FIND;
        show_bar(notebook);
        break;

    case finder_next:
        webkit_find_controller_search_next(
            webkit_web_view_get_find_controller(notebook_get_webview(notebook)));
        break;

    case finder_prev:
        webkit_find_controller_search_previous(
            webkit_web_view_get_find_controller(notebook_get_webview(notebook)));
        break;

    case new_tab:
        notebook_append(notebook, NULL);
        gtk_notebook_set_show_tabs(notebook, true);
        bar.entry_mode = _SEARCH;
        show_bar(notebook);
        break;

    case hide_bar:
        bar.entry_mode = _HIDDEN;
        show_bar(notebook);
        break;

    case prettify: {
        if (READABILITY_ENABLED) {
            char* readability_js = malloc(READABILITY_N + 1);
            read_readability_js(readability_js);
            webkit_web_view_run_javascript(notebook_get_webview(notebook),
                readability_js, NULL, NULL, NULL);
            free(readability_js);
        }
        break;
    }
    }

    return 1;
}

int keypress(void* self, GdkEvent* e, GtkNotebook* notebook)
{
    (void)self;

    for (int i = 0; i < sizeof(keys) / sizeof(keys[0]); i++)
        if ((e->key.state == keys[i].mod || keys[i].mod == 0x0) && e->key.keyval == keys[i].key)
            return handle_key(keys[i].id, notebook);
    /*
    printf("Event type: %d\n", e->type);
    printf("Keyval: %d\n", e->key.keyval);
    // Note: if I wanted to bind button presses, like the extra button in the mouse, 
    // I would have to bind the button-press-event signal instead.
    // Some links in case I go down that road: <https://docs.gtk.org/gtk3/signal.Widget.button-press-event.html>
    // https://docs.gtk.org/gdk3/union.Event.html
    // https://docs.gtk.org/gdk3/struct.EventButton.html
    */
    return 0;
}

void search_activate(GtkEntry* self, GtkNotebook* notebook)
{
    if (bar.entry_mode == _SEARCH)
        load_uri(notebook_get_webview(notebook),
            gtk_entry_buffer_get_text(bar.bar_line_text));
    else if (bar.entry_mode == _FIND)
        webkit_find_controller_search(
            webkit_web_view_get_find_controller(notebook_get_webview(notebook)),
            gtk_entry_buffer_get_text(bar.bar_line_text),
            WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE | WEBKIT_FIND_OPTIONS_WRAP_AROUND,
            G_MAXUINT);

    gtk_widget_hide(GTK_WIDGET(bar.header));
}

void window_init(GtkNotebook* notebook)
{
    GtkCssProvider* css = gtk_css_provider_new();
    gtk_css_provider_load_from_path(css, "/usr/share/themes/rose/style.css",
        NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css), 800);
    gtk_entry_buffer_new("", 0);
    gtk_entry_set_alignment(bar.bar_line, 0.48);
    gtk_widget_set_size_request(GTK_WIDGET(bar.bar_line), BAR_SIZE, -1);
    gtk_header_bar_set_custom_title(bar.header, GTK_WIDGET(bar.bar_line));
    gtk_window_set_titlebar(window, GTK_WIDGET(bar.header));
    g_signal_connect(bar.bar_line, "activate", G_CALLBACK(search_activate), notebook);
    g_signal_connect(window, "key-press-event", G_CALLBACK(keypress), notebook);
    g_signal_connect(window, "destroy", G_CALLBACK(exit), notebook);
}

void notebook_init(GtkNotebook* notebook, const char* uri)
{
    gtk_notebook_set_show_border(notebook, false);
    gtk_notebook_set_show_tabs(notebook, false);
    notebook_append(notebook, uri);
}

int main(int argc, char** argv)
{
    // <https://docs.gtk.org/gtk3/func.init.html>
    gtk_init(NULL, NULL);

    // Define GTK entities. These are declared globally
    window = GTK_WINDOW(gtk_window_new(0));
    bar.header = GTK_HEADER_BAR(gtk_header_bar_new());
    bar.bar_line_text = GTK_ENTRY_BUFFER(gtk_entry_buffer_new("", 0));
    bar.bar_line = GTK_ENTRY(gtk_entry_new_with_buffer(bar.bar_line_text));
    gtk_window_set_default_size(window, WIDTH, HEIGHT);
    notebook = GTK_NOTEBOOK(gtk_notebook_new());
    window_init(notebook);

    // Initialize with first uri
    char* first_uri = argc > 1 ? argv[1] : HOME;
    notebook_init(notebook, first_uri);
    g_object_set(gtk_settings_get_default(), GTK, NULL);

    // More GTK stuff
    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(notebook));
    gtk_widget_show_all(GTK_WIDGET(window));
    gtk_widget_hide(GTK_WIDGET(bar.header));

    // Deal with more uris, if this is necessary.
    if (argc > 2) {
        gtk_notebook_set_show_tabs(notebook, true);
        for (int i = 2; i < argc; i++) {
            notebook_append(notebook, argv[i]);
        }
    }

    gtk_main();
    // this point is never reached, since gtk_main(); never exits.
}
