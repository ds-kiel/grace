 // reference https://github.com/joprietoe/gdbus/blob/master/gdbus-example-server.c

#include "gpiot.h"

#include <la_sigrok.h>
#include <types.h>
#include <inttypes.h>

#include <stdio.h>
#include <stdlib.h>
#include <gio/gio.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <unistd.h>

static GDBusNodeInfo* introspection_data = NULL;

/* Introspection data for the service we are exporting */
static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='org.cau.gpiot.ControllerInterface'>"
  "    <annotation name='org.cau.gpiot.Annotation' value='OnInterface'/>"
  "    <annotation name='org.cau.gpiot.Annotation' value='AlsoOnInterface'/>"
  "    <method name='Start'>"
  "      <annotation name='org.cau.gpiot.Annotation' value='OnMethod'/>"
  "      <arg type='s' name='logPath' direction='in'/>"
  "      <arg type='b' name='waitForSync' direction='in'/>"
  "      <arg type='s' name='result' direction='out'/>"
  "    </method>"
  "    <method name='Stop'>"
  "      <annotation name='org.cau.gpiot.Annotation' value='OnMethod'/>"
  "      <arg type='b' name='waitForSync' direction='in'/>"
  "      <arg type='s' name='result' direction='out'/>"
  "    </method>"
  "    <method name='Sync'>"
  "      <annotation name='org.cau.gpiot.Annotation' value='OnMethod'/>"
  "      <arg type='s' name='result' direction='out'/>"
  "    </method>"
  "    <method name='getState'>"
  "      <annotation name='org.cau.gpiot.Annotation' value='OnMethod'/>"
  "      <arg type='i' name='result' direction='out'/>"
  "    </method>"
  "    <signal name='Something'>"
  "      <annotation name='org.cau.gpiot.Annotation' value='Onsignal'/>"
  "      <arg type='d' name='speed_in_mph'/>"
  "      <arg type='s' name='speed_as_string'>"
  "        <annotation name='org.cau.gpiot.Annotation' value='OnArg_NonFirst'/>"
  "      </arg>"
  "    </signal>"
  "    <property type='s' name='Status' access='read'/>"
  "  </interface>"
  "</node>";


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
    const gchar *logpath;
    gboolean wait_sync;

    g_printf("Invocation: Start\n");

    g_variant_get(parameters, "(&sb)", &logpath, &wait_sync);
    g_printf("Parameters: %s %c\n", logpath, wait_sync);

    GVariantBuilder* channel_modes_builder = g_variant_builder_new(G_VARIANT_TYPE("a(yy)"));
    GVariant* channel_modes;
    g_variant_builder_add(channel_modes_builder, "(yy)", 0, MATCH_FALLING | MATCH_RISING);
    g_variant_builder_add(channel_modes_builder, "(yy)", 1, MATCH_FALLING | MATCH_RISING);
    g_variant_builder_add(channel_modes_builder, "(yy)", 2, MATCH_FALLING | MATCH_RISING);
    channel_modes = g_variant_new("a(yy)", channel_modes_builder);
    g_variant_builder_unref(channel_modes_builder);

    g_print(g_variant_print(channel_modes,TRUE));
    if ((ret = la_sigrok_run_instance(wait_sync, logpath, channel_modes)) < 0) {
      result = g_strdup_printf("Unable to run instance");
    } else if (ret > 0) {
      result = g_strdup_printf("Instance already running");
    } else {
      result = g_strdup_printf("Started collection on device");
    }

    g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", result));

  } else if (!g_strcmp0(method_name, "Stop")) {
    gboolean wait_sync;

    g_printf("Invocation: Stop\n");

    g_variant_get(parameters, "(b)", &wait_sync);
    g_printf("got: %c\n", wait_sync);

    if (la_sigrok_running()) {
      int ret;
      if ((ret = la_sigrok_stop_instance(wait_sync)) >= 1) {
        result = g_strdup_printf("Waiting for sync");
      } else if (ret == 0) {
        result = g_strdup_printf("Stopped");
      } else {
        result = g_strdup_printf("Could not stop instance");
      }
    } else {
      result = g_strdup_printf("No instance running");
    }
    g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", result));
  } else if (!g_strcmp0(method_name, "getState")) {
    gpiot_daemon_state_t state;
    if(la_sigrok_running() && la_sigrok_waiting_sync()) state = GPIOTD_PENDING_SYNC; // TODO spaghetti code. Handle some other way
    else if (la_sigrok_running()) state = GPIOTD_COLLECTING;
    else state = GPIOTD_IDLE;

    g_dbus_method_invocation_return_value(invocation, g_variant_new("(i)", state));
  } else if (!g_strcmp0(method_name, "Sync")) {
    if(la_sigrok_running()) {
      if(!la_sigrok_waiting_sync()) {
        la_sigrok_announce_sync();
        result = g_strdup_printf("Expecting Sync");
      }
    }
    g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", result));
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
  GMainContext* main_context;
  guint owner_id;
  GBusNameOwnerFlags flags;

  g_printf("Started gpiot daemon!\n");

  main_context  =  g_main_context_new();
  g_main_context_push_thread_default(main_context);

  /* if(la_sigrok_init_instance(8000000) < 0) { */
  /*   g_printf("Could not initialize sigrok gpiot instance!\n"); */
  /*   goto cleanup; */
  /* } */

  introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, NULL);
  g_assert(introspection_data != NULL);

  // take name from other connection, but also allow others to take this connection
  flags = G_BUS_NAME_OWNER_FLAGS_REPLACE | G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT;
  owner_id = g_bus_own_name(G_BUS_TYPE_SYSTEM, "org.cau.gpiot", flags, on_bus_acquired, on_name_acquired, on_name_lost, NULL, NULL);
  g_printf("Setup gpio pin for gps flooding!\n");

  while (1) { // TODO main loop
    g_main_context_iteration(main_context, TRUE);
  }

  // ------ destruction ------
cleanup:
    g_bus_unown_name(owner_id);
    g_dbus_node_info_unref(introspection_data);
    g_main_context_pop_thread_default(main_context);

  return 0;
}
