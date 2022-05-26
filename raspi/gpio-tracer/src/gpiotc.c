#include "gpiot.h"

#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <sys/types.h>
#include <unistd.h>

#include <time.h>

#include <gio-unix-2.0/gio/gunixfdlist.h>
#include <gio/gio.h>

static gboolean start = FALSE;
static gboolean stop = FALSE;
static gchar *trace_dir = NULL;

static const GOptionEntry entries[] = {
  {"start",      's'    , 0, G_OPTION_ARG_NONE,     &start      , "Tell daemon to start gpio tracing",           NULL},
  {"stop",       'k'    , 0, G_OPTION_ARG_NONE,     &stop       , "Tell daemon to start gpio tracing",           NULL},
  {"tracedir",   'l'    , 0, G_OPTION_ARG_STRING,   &trace_dir  , "directory where traces will be written to",   NULL},
  { NULL }
};


static  GMainLoop *loop;

/* see gdbus-example-server.c for the server implementation */
static const gchar* daemon_call_control_interface_method(GDBusConnection *connection,
                                                         const gchar *name_owner,
                                                         const gchar* method,
                                                         GVariant* parameters,
                                                         GError **error) {
  g_printf("trying run %s\n", method);
  GDBusMessage *method_call_message;
  GDBusMessage *method_reply_message;

  const gchar *response;

  method_call_message = NULL;
  method_reply_message = NULL;

  method_call_message = g_dbus_message_new_method_call(
      name_owner, "/org/cau/gpiot/Controller", "org.cau.gpiot.ControllerInterface",
      method
  );

  g_dbus_message_set_body(method_call_message,
                          parameters);

  method_reply_message = g_dbus_connection_send_message_with_reply_sync(
      connection, method_call_message, G_DBUS_SEND_MESSAGE_FLAGS_NONE, -1,
      NULL, /* out_serial */
      NULL, /* cancellable */
      error);

  if (method_reply_message == NULL)
    goto fin;

  if (g_dbus_message_get_message_type(method_reply_message) ==
      G_DBUS_MESSAGE_TYPE_ERROR) {
    g_dbus_message_to_gerror(method_reply_message, error);
    goto fin;
  }

  response = g_dbus_message_get_arg0(method_reply_message);

  g_printf("Response: %s\n", response);

fin:
  g_object_unref(method_call_message);
  g_object_unref(method_reply_message);

  return response;
}

static void on_name_appeared(GDBusConnection *connection, const gchar *name,
                             const gchar *name_owner, gpointer user_data) {
  gint fd;
  GError *error;
  GVariant *parameters;
  const gchar* method;

  error = NULL;
  g_printf("name %s appeared\n", name);

  if(start) {
    method = "Start";

    if (trace_dir == NULL) {
      trace_dir = "/tmp/gpio-traces/";
      // the daemon is responsible to check wether the trace directory exists and is writeable
    }

    parameters = g_variant_new("(s)", trace_dir);
  } else if(stop) {
    method = "Stop";
    parameters = g_variant_new("()");
  }

  g_printf("%s/n", daemon_call_control_interface_method(connection, name_owner, method, parameters, &error));

  g_main_loop_quit(loop);
}

static void on_name_vanished(GDBusConnection *connection, const gchar *name,
                             gpointer user_data) {
  g_printerr("Failed to get name owner for %s\n"
             "Is the gpio-tracer daemon running?\n",
             name);
  exit(1);
}


int main(int argc, char *argv[]) {
  guint watcher_id;
  GError *error;
  GOptionContext *option_context;

  error = NULL;

  option_context = g_option_context_new("control gpio tracer daemon");
  g_option_context_add_main_entries(option_context, entries, NULL);

  if(!g_option_context_parse(option_context, &argc, &argv, &error)) {
    g_printf("option parsing failed: %s\n", error->message);
    exit(1);
  }


  /* watcher_id = g_bus_watch_name(G_BUS_TYPE_SESSION, "org.cau.gpiot", */
  /*                               G_BUS_NAME_WATCHER_FLAGS_NONE, on_name_appeared, */
  /*                               on_name_vanished, NULL, NULL); */

  watcher_id = g_bus_watch_name(G_BUS_TYPE_SYSTEM, "org.cau.gpiot",
                                G_BUS_NAME_WATCHER_FLAGS_NONE, on_name_appeared,
                                on_name_vanished, NULL, NULL);

  loop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(loop);

  g_option_context_free(option_context);
  g_bus_unwatch_name(watcher_id);
  exit(0);
}
