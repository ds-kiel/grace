#include <fx2.h>
#include <glib.h>
#include <stdio.h>
#include <unistd.h>

int print_bix = 0;

int main(int argc, char *argv[]) {
  g_message("starting fx2 test program");

  if (argc < 2) {
    printf("missing arguments. Usage: test_fx2 <path_to_firmware>\n");
    return 0;
  }

  char *fname_bix = argv[1];

  if (access(fname_bix, R_OK) != 0) {
    g_error("Could not read file %s", fname_bix);
    return -1;
  }

  FILE *f_bix;
  size_t f_size;
  f_bix = fopen(fname_bix, "rb");

  // determine size of file
  fseek(f_bix, 0, SEEK_END);
  f_size = ftell(f_bix);
  fseek(f_bix, 0, SEEK_SET);

  unsigned char bix[f_size];
  fread(bix, sizeof(bix), 1, f_bix);

  /* if(print_bix) */
    pretty_print_memory(bix, 0x0000, sizeof(bix));

  // enumerate candidate device
  struct fx2_device_manager manager;

  fx2_init_manager(&manager);
  fx2_find_devices(&manager);
  fx2_open_device(&manager);

  unsigned char data[2];

  // check status
  send_control_command(&manager,
                       LIBUSB_RECIPIENT_DEVICE | LIBUSB_REQUEST_TYPE_STANDARD |
                           LIBUSB_REQUEST_HOST_TO_DEVICE,
                       LIBUSB_REQUEST_GET_STATUS, 0, 0, data, 2);

  g_message("Status: %x", data[0] << 4 | data[1]);

  // write firmware to device
  fx2_cpu_set_reset(&manager);

  g_message("uploading firmware");

  fx2_download_firmware(&manager, bix, sizeof(bix), 1);

  /* fx2_upload_fw(&manager, NULL, 0); */
  fx2_cpu_unset_reset(&manager);

  sleep(1);
  fx2_close_device(&manager);

  sleep(2);
  fx2_find_devices(&manager);

  fx2_open_device(&manager);

  sleep(2);


  GThread *transfer_thread;
  transfer_thread = g_thread_new("bulk transfer thread", &fx2_transfer_loop_thread_func, (void *)&manager);

  fx2_start_sampling(&manager);

  g_thread_join(transfer_thread);


  fx2_close_device(&manager);

  return 0;
}
