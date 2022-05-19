#include <fx2.h>
#include <glib.h>
#include <stdio.h>
#include <unistd.h>
#include <tracing.h>
#include <libusb.h>

#define EEPROM_FIRMWARE_LOCATION "../external/fx2lib/examples/eeprom/firmware/build/eeprom.iic"
#define MAX_WRITE_BYTES 32

int print_bix = 0;

void packet_callback(uint8_t *packet_data, int length, void *user_data) {
  g_message("Got %d\n", length);
}


int write_eeprom(struct fx2_device_manager *manager_instc, unsigned char *bytes, size_t length) {
  int written = 0;
  size_t transferred_bytes = 0;

  while(transferred_bytes < length) {
    int ret;
    int write = length-transferred_bytes > MAX_WRITE_BYTES ? MAX_WRITE_BYTES : length-transferred_bytes;

    if ((ret = send_control_command(
                                    manager_instc,
                                    0x40,
                                    0xb1, transferred_bytes, 0, bytes  + transferred_bytes, write)) <= 0) {
      // although not strictly an USB error, transmission with 0 length;  should not occur when downloading to EZ-USB Ram
      g_message("could not write to rom");
      return -1;
    }

    g_message("Wrote %zu of %zu bytes", transferred_bytes, length);

    transferred_bytes += ret;
  }

}

int main(int argc, char *argv[]) {
  g_message("download fx2 firmware to mcu");

  if (argc < 2) {
    printf("missing arguments. Usage: test_fx2 <path_to_iic>\n");
    return 0;
  }

  char *fname_iic = argv[1]; 
  char *eeprom_bix = EEPROM_FIRMWARE_LOCATION;  // TODO for now assume firmware is already installed on device

  // read firmware to write to eeprom
  if (access(fname_iic, R_OK) != 0) {
    g_error("Could not read file %s", fname_iic);
    return -1;
  }

  FILE *f_iic;
  size_t f_size;
  f_iic = fopen(fname_iic, "rb");

  // determine size of file
  fseek(f_iic, 0, SEEK_END);
  f_size = ftell(f_iic);
  fseek(f_iic, 0, SEEK_SET);

  unsigned char iic[f_size];
  fread(iic, sizeof(iic), 1, f_iic);

  // enumerate candidate device
  struct fx2_device_manager manager;
  struct fx2_bulk_transfer_config transfer_cnfg;

  fx2_init_manager(&manager);
  fx2_find_devices(&manager);
  fx2_open_device(&manager);

  write_eeprom(&manager, iic, sizeof(iic));

  sleep(1);
  fx2_close_device(&manager);

  fx2_deinit_manager(&manager);
  
  return 0;
}
