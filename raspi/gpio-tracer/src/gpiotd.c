 // reference https://github.com/joprietoe/gdbus/blob/master/gdbus-example-server.c

#include "gpiot.h"

#include <tracing.h>
#include <chunked_output.h>
#include <types.h>
#include <inttypes.h>

#ifdef WITH_TIMESYNC
#include <radio.h>
#endif

#include <gio/gio.h>
#include <glib.h>
#include <glib/gprintf.h>

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>


static GDBusNodeInfo* introspection_data = NULL;

static tracing_instance_t *tracing_task;
static chunked_output_t *chunked_output;

static gchar *trace_path;
static gchar *firmware_location;

static const GOptionEntry entries[] = {
  {"firmware", 'f', 0, G_OPTION_ARG_STRING, &firmware_location, "location for firmware (has to be in .bix format)", NULL},
  { NULL }
};

/* Introspection data for the service we are exporting */
static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='org.cau.gpiot.ControllerInterface'>"
  "    <annotation name='org.cau.gpiot.Annotation' value='OnInterface'/>"
  "    <annotation name='org.cau.gpiot.Annotation' value='AlsoOnInterface'/>"
  "    <method name='Start'>"
  "      <annotation name='org.cau.gpiot.Annotation' value='OnMethod'/>"
  "      <arg type='s' name='jobLogDir' direction='in'/>"
  "      <arg type='s' name='result' direction='out'/>"
  "    </method>"
  "    <method name='Stop'>"
  "      <annotation name='org.cau.gpiot.Annotation' value='OnMethod'/>"
  "      <arg type='s' name='result' direction='out'/>"
  "    </method>"
  "    <method name='getState'>"
  "      <annotation name='org.cau.gpiot.Annotation' value='OnMethod'/>"
  "      <arg type='i' name='result' direction='out'/>"
  "    </method>"
  "    <property type='s' name='Status' access='read'/>"
  "  </interface>"
  "</node>";


static int start_tasks(struct channel_configuration chan_conf) {
  int ret;
  struct stat st = {0};

  if (stat(trace_path, &st)) {
    g_printf("creating path for storing traces!\n");
    if (mkdir(trace_path, 0700) < 0) {
      g_printf("could not create/open target trace directory!\n");
      return -1;
    }
  }

  chunked_output = chunked_output_new();

  if ((ret = chunked_output_init(chunked_output, trace_path))) {
    return -1;
  }
 
  if ((tracing_task = tracing_init((output_module_t*) chunked_output, firmware_location)) == NULL) {
    chunked_output_deinit(chunked_output);
    return -1;
  }

  sleep(1); // give device some time for reNumeration

  tracing_start(tracing_task, chan_conf);

  return 0;
}

static int stop_tasks() {
  tracing_stop(tracing_task);
  tracing_deinit(tracing_task);
  
  chunked_output_deinit(chunked_output);

  return 0;
}

// dbus handlers
static void handle_method_call(GDBusConnection *connnection,
                               const gchar *sender, const gchar *object_path,
                               const gchar *interface_name,
                               const gchar *method_name, GVariant *parameters,
                               GDBusMethodInvocation *invocation,
                               gpointer user_data) {

  // TODO should the error case return an error value?
  gchar *result;
  g_printf("handle Method invocation with %s\n", method_name);
  if (!g_strcmp0(method_name, "Start")) {
    int ret;

    g_printf("Invocation: Start\n");

    g_variant_get(parameters, "(&s)", &trace_path);
    g_printf("Parameters: %s\n", trace_path);

    struct channel_configuration chan_conf = {
      .ch1 = SAMPLE_ALL,
      .ch2 = SAMPLE_ALL,
      .ch3 = SAMPLE_ALL,
      .ch4 = SAMPLE_ALL,
      .ch5 = SAMPLE_ALL,
      .ch6 = SAMPLE_ALL,
      .ch7 = SAMPLE_ALL,
      .ch8 = SAMPLE_RADIO,
    };
    
    if ((ret = start_tasks(chan_conf)) < 0) {
      result = g_strdup_printf("unable to start instance");
    } else if (ret > 0) {
      result = g_strdup_printf("instance already running");
    } else {
      result = g_strdup_printf("started collection on device");
    }

    g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", result));

  } else if (!g_strcmp0(method_name, "Stop")) {
    g_printf("Invocation: Stop\n");

    if (tracing_running(tracing_task)) {
      int ret;
      if ((ret = stop_tasks()) >= 0) {
        result = g_strdup_printf("stopped running instance");
      } else {
        result = g_strdup_printf("could not stop instance");
      }
    } else {
      result = g_strdup_printf("no instance running");
    }
    g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", result));
  } else if (!g_strcmp0(method_name, "getState")) {
    gpiot_daemon_state_t state;
    if(tracing_running(tracing_task))
      state = GPIOTD_COLLECTING;
    else state = GPIOTD_IDLE;

    g_dbus_method_invocation_return_value(invocation, g_variant_new("(i)", state));
  }
}

static GVariant *handle_get_property(GDBusConnection *connection,
                                     const gchar *sender,
                                     const gchar *object_path,
                                     const gchar *interface_name,
                                     const gchar *property_name, GError **error,
                                     gpointer user_data) {
}

static gboolean handle_set_property(GDBusConnection *connection,
                                     const gchar *sender,
                                     const gchar *object_path,
                                     const gchar *interface_name,
                                     const gchar *property_name,
                                     GVariant* value,
                                     GError **error,
                                     gpointer user_data) {
}

static const GDBusInterfaceVTable interface_vtable = {
  handle_method_call,
  handle_get_property,
  handle_set_property,
};

static void on_bus_acquired(GDBusConnection* connection, const gchar* name, gpointer user_data) {
  // export objects from here
  guint registration_id;

  registration_id = g_dbus_connection_register_object(
      connection, "/org/cau/gpiot/Controller",
      introspection_data->interfaces[0], &interface_vtable, NULL, NULL, NULL);

  g_assert(registration_id > 0);
}

static void on_name_acquired(GDBusConnection* connection, const gchar* name, gpointer user_data) {
  g_printf("acquired dbus name %s\n", name);
}

static void on_name_lost(GDBusConnection* connection, const gchar* name, gpointer user_data) {
  g_printf("lost dbus name %s\n", name);
}


int main(int argc, char *argv[])
{
  GMainContext *main_context;
  GOptionContext *option_context;  
  GError *error;  
  guint owner_id;
  GBusNameOwnerFlags flags;

  g_printf("Started gpiot daemon!\n");
  

  main_context  =  g_main_context_new();
  g_main_context_push_thread_default(main_context);

  // parse command line arguments
  option_context = g_option_context_new("control gpio tracer daemon");    
  g_option_context_add_main_entries(option_context, entries, NULL);
  if(!g_option_context_parse(option_context, &argc, &argv, &error)) {
    g_printf("option parsing failed: %s\n", error->message);
    exit(1);
  }

  introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, NULL);
  g_assert(introspection_data != NULL);

  // take name from other connection, but also allow others to take this connection
  flags = G_BUS_NAME_OWNER_FLAGS_REPLACE | G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT;
  owner_id = g_bus_own_name(G_BUS_TYPE_SYSTEM, "org.cau.gpiot", flags, on_bus_acquired, on_name_acquired, on_name_lost, NULL, NULL); // needed if run as a dbus service
  /* owner_id = g_bus_own_name(G_BUS_TYPE_SESSION, "org.cau.gpiot", flags, on_bus_acquired, on_name_acquired, on_name_lost, NULL, NULL);   */

  while (1) { // TODO main loop
    g_main_context_iteration(main_context, FALSE);
    
    if(tracing_task != NULL) {
      tracing_handle_events(tracing_task);
    }
  }

  // ------ destruction ------
cleanup:
    g_bus_unown_name(owner_id);
    g_dbus_node_info_unref(introspection_data);
    g_main_context_pop_thread_default(main_context);
    g_option_context_free(option_context);

  return 0;
}
