#include <fx2.h>
#include <glib.h>
#include <stdio.h>
#include <unistd.h>
#include <tracing.h>

int print_bix = 0;

void packet_callback(uint8_t *packet_data, int length, void *user_data) {
  g_message("Got %d\n", length);
}

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
  struct fx2_bulk_transfer_config transfer_cnfg;

  fx2_init_manager(&manager);
  fx2_find_devices(&manager);
  fx2_open_device(&manager);


  fx2_create_bulk_transfer(&manager, &transfer_cnfg, 20, (1 << 17));

  fx2_set_bulk_transfer_packet_callback(&transfer_cnfg, &packet_callback, NULL);

  fx2_submit_bulk_transfer(&transfer_cnfg);

  unsigned char data[2];

  // write firmware to device
  fx2_cpu_set_reset(&manager);

  g_message("uploading firmware");

  fx2_download_firmware(&manager, bix, sizeof(bix), 1);

  /* fx2_upload_fw(&manager, NULL, 0); */
  fx2_cpu_unset_reset(&manager);

  sleep(1);
  fx2_close_device(&manager);

  sleep(4);
  fx2_find_devices(&manager);

  fx2_open_device(&manager);

  sleep(2);


  /* #define VC_START_SAMP 0xB2 */
  /* send_control_command( */
  /*                      &manager, */
  /*                      LIBUSB_REQUEST_TYPE_VENDOR, */
  /*                      VC_START_SAMP, 0x00, 0, NULL, 0);  */

  for(size_t k = 0; k < 100; k++) {
    send_control_command(
                         &manager,
                         LIBUSB_REQUEST_TYPE_VENDOR,
                         VC_UART_TEST, 0x00, 0, NULL, 0);
    sleep(1);
  }


  fx2_deinit_manager(&manager);

  fx2_close_device(&manager);

  return 0;
}
