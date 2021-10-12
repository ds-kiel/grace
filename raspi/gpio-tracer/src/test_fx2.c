#include <fx2.h>
#include <glib.h>

int main(int argc, char *argv[]) {
  g_message("starting fx2 test program");
  struct fx2_device_manager manager;

  fx2_init_manager(&manager);
  fx2_find_devices(&manager);
  fx2_open_device(&manager);

  unsigned char data[2];

  send_control_command(&manager,
                       LIBUSB_RECIPIENT_DEVICE | LIBUSB_REQUEST_TYPE_STANDARD |
                           LIBUSB_REQUEST_HOST_TO_DEVICE,
                       LIBUSB_REQUEST_GET_STATUS, 0, 0, data, 2);

  g_message("Status: %x", data[0] << 4 | data[1]);

  unsigned char *fw;
  size_t len;

  fx2_cpu_set_reset(&manager);
  fx2_download_firmware(&manager,
                        data, len, 0);
    /* fx2_upload_fw(&manager, NULL, 0); */
  fx2_cpu_unset_reset(&manager);
  return 0;
}
