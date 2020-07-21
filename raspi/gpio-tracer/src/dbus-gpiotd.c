 // reference https://github.com/joprietoe/gdbus/blob/master/gdbus-example-server.c

#include "gpiot.h"

#include <la_sigrok.h>
#include <la_pigpio.h>
#include <types.h>
#include <inttypes.h>

#include <stdio.h>
#include <stdlib.h>
#include <gio/gio.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <unistd.h>


gpiot_daemon_state_t state = GPIOTD_IDLE;
gpiot_devices_t device = GPIOT_DEVICE_NONE;

static GDBusNodeInfo* introspection_data = NULL;

/* Introspection data for the service we are exporting */
static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='org.cau.gpiot.ControllerInterface'>"
  "    <annotation name='org.cau.gpiot.Annotation' value='OnInterface'/>"
  "    <annotation name='org.cau.gpiot.Annotation' value='AlsoOnInterface'/>"
  "    <method name='Start'>"
  "      <annotation name='org.cau.gpiot.Annotation' value='OnMethod'/>"
  "      <arg type='s' name='device' direction='in'/>"
  "      <arg type='u' name='samplerate' direction='in'/>"
  "      <arg type='s' name='logPath' direction='in'/>"
  "      <arg type='b' name='waitForSync' direction='in'/>"
  "      <arg type='s' name='result' direction='out'/>"
  "    </method>"
  "    <method name='Stop'>"
  "      <annotation name='org.cau.gpiot.Annotation' value='OnMethod'/>"
  "      <arg type='s' name='device' direction='in'/>"
  "      <arg type='b' name='waitForSync' direction='in'/>"
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
    const gchar *device;
    const gchar *logpath;
    guint32 samplerate;
    gboolean wait_sync;

    g_printf("Invocation: Start\n");

    g_variant_get(parameters, "(&su&sb)", &device, &samplerate, &logpath, &wait_sync);
    g_printf("Parameters: %s %" PRIu32 " %s %c\n", device, samplerate, logpath, wait_sync);

    if (!g_strcmp0(device, "sigrok")) {
      GVariantBuilder* channel_modes_builder = g_variant_builder_new(G_VARIANT_TYPE("a(yy)"));
      GVariant* channel_modes;
      g_variant_builder_add(channel_modes_builder, "(yy)", 0, MATCH_FALLING | MATCH_RISING);
      g_variant_builder_add(channel_modes_builder, "(yy)", 1, MATCH_FALLING | MATCH_RISING);
      g_variant_builder_add(channel_modes_builder, "(yy)", 2, MATCH_FALLING | MATCH_RISING);
      channel_modes = g_variant_new("a(yy)", channel_modes_builder);
      g_variant_builder_unref(channel_modes_builder);

      g_print(g_variant_print(channel_modes,TRUE));

      // owner ship of channel_modes is transfered to la_sigrok_init
      if (la_sigrok_init_instance(samplerate, logpath, channel_modes) < 0) {
        result = g_strdup_printf("Unable to init sigrok instance\n");
      } else {
        g_printf("run sigrok instance\n");

        if (la_sigrok_run_instance(wait_sync) < 0) {
          result = g_strdup_printf("Unable to run sigrok instance %s!", device);
        } else {
          result = g_strdup_printf("Started collecting on device %s", device);
          state = GPIOTD_COLLECTING;
        }
      }
    } else if (!g_strcmp0(device, "pigpio")) {
      if(la_pigpio_init_instance(logpath) < 0) {
        result = g_strdup_printf("Unable to create pigpio instance");
      } else {
        la_pigpio_run_instance(); // TODO implement error checking when needed
        state = GPIOTD_COLLECTING;
        result = g_strdup_printf("Started collecting on device %s", device);
      }
    } else {
      result = g_strdup_printf("Device %s not known!", device);
    }

    g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", result));

  } else if (!g_strcmp0(method_name, "Stop")) {
    const gchar *device;
    gboolean wait_sync;

    g_printf("Invocation: Stop\n");

    g_variant_get(parameters, "(&sb)", &device, &wait_sync);
    g_printf("got: %s %c\n", device, wait_sync);

    if (state == GPIOTD_COLLECTING) { // TODO track what device exactly is collecting!
      if (!g_strcmp0(device, "pigpio")) {
        la_pigpio_stop_instance();
      } else if (!g_strcmp0(device, "sigrok")) {
        la_sigrok_stop_instance(wait_sync);
      }
      state = GPIOTD_IDLE;
      result = g_strdup_printf("Stopped collecting on device %s", device);
    } else {
      result = g_strdup_printf("No instance running for %s", device);
    }
    g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", result));
  } else if (!g_strcmp0(method_name, "getState")) {
    gpiot_daemon_state_t _state = state;
    if(state == GPIOTD_COLLECTING && !la_sigrok_get_sync_state()) _state = GPIOTD_PENDING_SYNC; // TODO spaghetti code. Handle some other way

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
  GMainContext* main_context;
  guint owner_id;
  GBusNameOwnerFlags flags;

  g_printf("Started gpiot daemon!\n");

  main_context  =  g_main_context_new();
  g_main_context_push_thread_default(main_context);

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

  g_bus_unown_name(owner_id);
  g_dbus_node_info_unref(introspection_data);
  g_main_context_pop_thread_default(main_context);

  return 0;
}
