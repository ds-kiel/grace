#include "libusb.h"
#include <sys/time.h>
#include <fx2.h>
#include <glib.h>
#include <stdio.h>
#include <unistd.h>

/* #define G_LOG_DOMAIN "FX2" */


#define FX2_PROG_RAM_TOP 0x3FFF
#define FX2_PROG_RAM_BOT 0x0000

#define FX2_DATA_RAM_TOP 0xE1FF
#define FX2_DATA_RAM_BOT 0xE000

struct fx2_usb_device {
  int vendorID;
  int productID;
  char *deviceString;
} candidate_devices[] = {
    {0x0925, 0x3881, "Vendor-Specific Device"},
    {0x1991, 0x0111, "FX2 Logic Analyzer"},
    {0x04b4, 0x8613, "Cypress Semiconductor Vendor-Specific Device"},
    {0x04b4, 0x0082, "Cypress Semiconductor Vendor-Specific Device"},            
    {0x04b4, 0x0083, "Cypress Semiconductor Usb Term"},
    {0x0000, 0x0000, NULL},
};

struct fx2_transfer_stat {
  int bytes_transfered;
};

void pretty_print_memory(char *mem, int starting_addr, size_t bytes) {
  const int bytes_per_line = 16;
  int written = 0;

  printf("87654321  ");
  for(int b = 0; b < bytes_per_line; b++) {
    printf("%02x ", 0xFF & b);
  } printf("\n");

  for(int l_addr = starting_addr; l_addr < starting_addr+bytes; l_addr+=bytes_per_line) {
    printf("0x%04x    ", l_addr);
    for(int b = 0; b < bytes_per_line; b++) {
      printf("%02x ", 0xFF & mem[written]);
      if(++written >= bytes) {
        printf("\n");
        return;
      }
    } printf("\n");
  }
}


int fx2_init_manager(struct fx2_device_manager *manager_instc) {
  int ret;
  manager_instc->fx2_dev = NULL;
  manager_instc->fx2_dev_handl = NULL;


  if ((ret = libusb_init(&manager_instc->libusb_cntxt)) != 0) {
    g_error("could not initialize libusb context: %s", libusb_error_name(ret));
    return -1;
  }

  manager_instc->active_bulk_transfers = NULL;
  manager_instc->finished_bulk_transfers = NULL;

  return 0;
}

int fx2_deinit_manager(struct fx2_device_manager *manager_instc) {
  libusb_exit(manager_instc->libusb_cntxt);

  return 0;
}

static int device_is_candidate(struct libusb_device_descriptor *desc) {
  for (struct fx2_usb_device *candidate = candidate_devices;
       candidate->deviceString != NULL; candidate++) {
    if (desc->idVendor == candidate->vendorID &&
        desc->idProduct == candidate->productID) {
      g_message("Found candidate!");
      return 1;
    }
  }

  return 0;
}

// precondition: device is open
static int fx2_get_string_descriptor(struct fx2_device_manager *manager_instc) {
  int ret;

  struct libusb_device *dev;
  struct libusb_device_handle *dev_handl;
  struct libusb_device_descriptor desc;
  unsigned char desc_str[200];

  if (manager_instc->fx2_dev_handl == NULL) {
    g_error("Could not acquire string descriptor. Device is not open");
    return -1;
  }

  dev = manager_instc->fx2_dev;
  dev_handl = manager_instc->fx2_dev_handl;
  desc = manager_instc->fx2_desc;

  if ((ret = libusb_get_string_descriptor_ascii(dev_handl, desc.iManufacturer,
                                                desc_str, 200)) < 0) {
    g_message("could not acquire string descriptor for device %s",
              libusb_error_name(ret));
    return -1;
  } else if (ret > 0) {
    g_message("found matching device: len: %d, %s ", ret, desc_str);
  }

  return 0;
}


int fx2_find_devices(struct fx2_device_manager *manager_instc) {
  struct libusb_context *libusb_cntxt;
  libusb_device **dev_list;
  int devs;

  libusb_cntxt = manager_instc->libusb_cntxt;

  // enumerate available devices
  if ((devs = libusb_get_device_list(libusb_cntxt, &dev_list)) < 0) {
    g_error("Could not enumerate devices %s", libusb_error_name(devs));
    return -1;
  }

  for (libusb_device **curr_device = dev_list; *curr_device != NULL;
       curr_device++) {
    struct libusb_device_descriptor desc;
    struct libusb_device_handle *dev_handl;

    libusb_get_device_descriptor(*curr_device, &desc);
    g_message("Found USB device: VendorID: %x, ProductID: %x", desc.idVendor,
              desc.idProduct);

    if (device_is_candidate(&desc)) {
      g_message("Found device with string descriptors: ");
      g_message("Manufacturer desc. index: %d ", desc.iManufacturer);
      g_message("Product desc. index: %d ", desc.iProduct);

      manager_instc->fx2_dev = *curr_device;
      manager_instc->fx2_desc = desc;
    }
  }

  libusb_free_device_list(dev_list, 1); // second argument > 0 if devices in
                                        // lists should be unreferenced as well

  return -1;
}


int fx2_open_device(struct fx2_device_manager *manager_instc) {
  int ret;
  struct libusb_device *dev;
  struct libusb_device_handle **dev_handl;
  struct libusb_device_descriptor *desc;

  if (manager_instc->fx2_dev == NULL) {
    g_error("No candidate device to open. Did you run fx2_find_devices ?");
    return -1;
  }

  dev = manager_instc->fx2_dev;
  desc = &manager_instc->fx2_desc;
  dev_handl = &manager_instc->fx2_dev_handl;

  g_message("Trying to open device: 0x%x/0x%x", desc->idProduct,
            desc->idVendor);

  if (libusb_open(dev, dev_handl) != 0) {
    g_error("unable to open device");
    return -1;
  }


  // before claiming usb device, remove any attached kernel driver
  if (libusb_kernel_driver_active(*dev_handl, 0)) {
    if ( (ret = libusb_detach_kernel_driver(*dev_handl, 0)) < 0) {
      g_error("Could not detach kernel driver for interface 0: %s", libusb_error_name(ret));
      return -1;
    }
  }

  if ( (ret = libusb_claim_interface(*dev_handl, 0)) < 0) {
    g_error("Could not claim interface 0: %s", libusb_error_name(ret));
    return -1;
  }

  /* // Sets EP0 into alternate mode 0 */
  /* if ( (ret = libusb_set_interface_alt_setting(*dev_handl, 0, 0)) < 0) { */
  /*   g_warning("Could not set interface alt setting: %s", libusb_error_name(ret)); */
  /*   return -1; */
  /* } */

  return 0;
}

int fx2_close_device(struct fx2_device_manager *manager_instc) {
  struct libusb_device_handle **dev_handl;

  g_message("Closing device");

  // TODO error checking
  dev_handl = &manager_instc->fx2_dev_handl;

  libusb_release_interface(*dev_handl, 0);

  libusb_close(*dev_handl);

  *dev_handl = NULL;

  return 0;
}

// maybe change naming this should check whether a device is opened claimed and
// able to receive data
static int fx2_device_ready(struct fx2_device_manager *manager_instc) {
  if (manager_instc->fx2_dev_handl == NULL) {
    return -1;
  }

  return 0;
}

// Simple wrapper around libusb_control_transfer that checks whether a valid
// device is claimed. Returns amount of bytes transffered on success
int send_control_command(struct fx2_device_manager *manager_instc,
                         guint8 bmRequestType, guint8 bRequest,
                         guint16 wValue, guint16 wIndex, unsigned char *data,
                         guint16 wLength) {
  int ret;
  libusb_device_handle *dev_handl;

  if (fx2_device_ready(manager_instc) < 0) {
    g_warning("unable to send control command to device.");
    return -1;
  }

  dev_handl = manager_instc->fx2_dev_handl;

  if ((ret = libusb_control_transfer(dev_handl, bmRequestType, bRequest, wValue,
                                     wIndex, data, wLength, 0)) < 0) {
    g_warning("Control transfer failed: %s", libusb_error_name(ret));
    return -1;
  }

  return ret;
}

static int valid_addr_range(guint16 addr);

// writes single byte to addr
static int fx2_mem_write_byte(struct fx2_device_manager *manager_instc,
                              guint16 addr, unsigned char byte) {

  int ret;
  unsigned char _byte[1] = {byte};
  if ((ret = send_control_command(
           manager_instc,
           LIBUSB_REQUEST_TYPE_VENDOR,
           LIBUSB_FX2_LOAD_FIRMWARE, addr, 0, _byte, 1)) < 0) {
    g_error("could not write to addr 0x%x err: %s", addr, libusb_error_name(ret));
    return -1;
  }

  return 0;
}


static int __fx2_cpu_set_reset_state(struct fx2_device_manager *manager_instc, unsigned char state) {
  if (fx2_mem_write_byte(manager_instc, 0xE600, state) < 0) {
    g_error("could not set CPU reset state");
    return -1;
  }
  return 0;
}

int __fx2_get_status_bytes(struct fx2_device_manager *manager_instc) {
  int ret;
  unsigned char data[2];

  if((ret = send_control_command(manager_instc,
                                 LIBUSB_RECIPIENT_DEVICE | LIBUSB_REQUEST_TYPE_STANDARD |
                                 LIBUSB_REQUEST_DIR_IN,
                                 LIBUSB_REQUEST_GET_STATUS, 0, 0, data, 2)) < 0) {
    return -1;
  }

  g_message("Device Status: %x, %x", data[0], data[1]);

  if((ret = send_control_command(manager_instc,
                                 LIBUSB_RECIPIENT_ENDPOINT | LIBUSB_REQUEST_TYPE_STANDARD |
                                 LIBUSB_REQUEST_DIR_IN,
                                 LIBUSB_REQUEST_GET_STATUS, 0, 0, data, 2)) < 0) {
    return -1;
  }


  g_message("Endpoint Status: %x, %x", data[0], data[1]);

  return 0;
}

// TODO this is the public facing interface. This should allow specifying where
// status should be queried. maybe add enum for available queriable information.
int fx2_get_status(struct fx2_device_manager *manager_isntc) {
  int ret;

  ret = __fx2_get_status_bytes(manager_isntc);

  return ret;
}

// Set CPU into reset state by writing 1 to CPUCS
int fx2_cpu_set_reset(struct fx2_device_manager *manager_instc) {
  return __fx2_cpu_set_reset_state(manager_instc, 0x01);
}

// Set CPU out of reset state by writing 0 to CPUCS
int fx2_cpu_unset_reset(struct fx2_device_manager *manager_instc) {
  return __fx2_cpu_set_reset_state(manager_instc, 0x00);
}



#define __MAX_READ_BYTES 4096
// Upload/Download only possible when CPU is held in reset state
// CPUCS Register can be used to put system in and out of reset
// Precondition: CPU has to be set into RESET state; array pointed to by buf has size FX2_PROG_RAM_TOP - FX2_PROG_RAM_BOT
int fx2_upload_firmware(struct fx2_device_manager *manager_instc, unsigned char *buf) {
  int ret;
  size_t length = FX2_PROG_RAM_TOP - FX2_PROG_RAM_BOT;
  size_t received_bytes = 0;

  while(received_bytes < length) {
    int read = length-received_bytes > __MAX_READ_BYTES ? __MAX_READ_BYTES : length-received_bytes;

    // addresses have to be word aligned i.e. they have to be evenly divisible by two
    if(((FX2_PROG_RAM_BOT + received_bytes) & 1) > 0) {
      received_bytes -= 1;
      g_message("starting address not aligned to word boundary, new start address: 0x%lx", FX2_PROG_RAM_BOT + received_bytes);
    }

    if ((ret = send_control_command(
                                    manager_instc,
                                    LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_REQUEST_DIR_IN,
                                    LIBUSB_FX2_LOAD_FIRMWARE, FX2_PROG_RAM_BOT + received_bytes, 0, buf + received_bytes, read)) <= 0) {
      g_error("could not read ram contents, ret: %d / %s", ret,
              libusb_error_name(ret));
      return -1;
    }

    g_message("starting from 0x%lx read %d bytes from program memory", FX2_PROG_RAM_BOT + received_bytes, ret);

    received_bytes += ret;
  }

  return 0;
}

#define __MAX_WRITE_BYTES 4096
// Download data into EZ-USB memory
int fx2_download_firmware(struct fx2_device_manager *manager_instc,
                          unsigned char *data, size_t length, int verify) {
  int ret;
  size_t transferred_bytes = 0;

  while(transferred_bytes < length) {
    int write = length-transferred_bytes > __MAX_WRITE_BYTES ? __MAX_WRITE_BYTES : length-transferred_bytes;

    if ((ret = send_control_command(
                                    manager_instc,
                                    LIBUSB_REQUEST_TYPE_VENDOR,
                                    LIBUSB_FX2_LOAD_FIRMWARE, FX2_PROG_RAM_BOT + transferred_bytes, 0, data  + transferred_bytes, write)) <= 0) {
      // although not strictly an USB error, transmission with 0 length;  should not occur when downloading to EZ-USB Ram
      g_error("could not write ram contents, ret: %d / %s", ret,
              libusb_error_name(ret));
      return -1;
    }

    transferred_bytes += ret;
    
    g_message("Wrote %zu of %zu bytes", transferred_bytes, length);
  }

  if (verify) {
    size_t read_length = FX2_PROG_RAM_TOP - FX2_PROG_RAM_BOT;
    unsigned char read_back[read_length];

    fx2_upload_firmware(manager_instc, read_back);

    if ((ret = memcmp((void *)read_back, (void *) data, length)) == 0) {
      g_message("Verification complete. Firmware downloaded successfully");
    } else {
      g_message("verification complete. Firmware download failed");
      return -2;
    }
  }

  return 0;
}

void __libusb_transfer_callback(struct libusb_transfer *transfer) {
  struct transfer_data *transfer_data = (struct transfer_data *) transfer->user_data;
  /* g_message("Transfer finished with: %d", transfer->status); */

  if (transfer->status == 0) {
    (*(transfer_data->transfer_cnfg->packet_handler_cb))(
        transfer->buffer, transfer->actual_length,
        transfer_data->transfer_cnfg->transfer_cb_user_data);

    libusb_submit_transfer(transfer);
  } else {
    g_message("Transfer failed with status code: %d, active submissions: %zu", transfer->status, transfer_data->transfer_cnfg->transfer_num_active);
    transfer_data->transfer_finished = 1;
    transfer_data->transfer_cnfg->transfer_num_active--;
  }
}

void __check_transfer_finished_func(gpointer data, gpointer user_data) {
  struct fx2_bulk_transfer_config *transfer_cnfg = (struct fx2_bulk_transfer_config *) data;
  struct fx2_device_manager *manager_instc = (struct fx2_device_manager *) user_data;

  if(transfer_cnfg->transfer_num_active <= 0) {
    manager_instc->active_bulk_transfers = g_list_remove(manager_instc->active_bulk_transfers, data);
    manager_instc->finished_bulk_transfers = g_list_append(manager_instc->finished_bulk_transfers, data);
  }
}

void __call_finished_cb(gpointer data, gpointer user_data) {
  struct fx2_bulk_transfer_config *transfer_cnfg = (struct fx2_bulk_transfer_config *) data;
  struct fx2_device_manager *manager_instc = (struct fx2_device_manager *) user_data;

  manager_instc->finished_bulk_transfers = g_list_remove(manager_instc->finished_bulk_transfers, data);

  (transfer_cnfg->finished_cb)(transfer_cnfg->finished_cb_user_data);
}

void fx2_set_bulk_transfer_packet_callback(struct fx2_bulk_transfer_config *transfer_cnfg, fx2_packet_callback_fn packet_handler, void *user_data) {
  for (size_t i = 0; i < transfer_cnfg->transfer_num; ++i) {
    transfer_cnfg->packet_handler_cb = packet_handler;
    transfer_cnfg->transfer_cb_user_data = user_data;
  }
}



/*
 * Function: fx2_set_bulk_transfer_finished_callback
 * ----------------------------
 *
 *   transfer_cnfg: pointer to transfer configuration structure
 *   finished_callback: callback to be called after no more submitted transfers exists
 *   transfer_data: optional user data to be passed to the callback
 *
 */
void fx2_set_bulk_transfer_finished_callback(struct fx2_bulk_transfer_config *transfer_cnfg, fx2_finished_callback_fn finished_callback, void *transfer_data) {
    transfer_cnfg->finished_cb = finished_callback;
    transfer_cnfg->finished_cb_user_data = transfer_data;
}

int fx2_create_bulk_transfer(struct fx2_device_manager *manager_instc,
                           struct fx2_bulk_transfer_config *transfer_cnfg,
                           size_t transfer_num,
                           size_t transfer_size) {
  transfer_cnfg->transfer_buffers = malloc(transfer_num * sizeof(unsigned char*));
  transfer_cnfg->transfers = malloc(transfer_num * sizeof(struct libusb_transfer*));
  transfer_cnfg->transfer_data = malloc(transfer_num * sizeof(struct transfer_data));

  transfer_cnfg->transfer_num = transfer_num;
  transfer_cnfg->transfer_size = transfer_size;

  // 1. Allocate transfers
  // 2. Fill transfers with transfer information
  // 3. submit transfers
  // 4. Examine transfer structure from callback
  // 5. Deallocate resources
  for (size_t i = 0; i < transfer_num; ++i) {
    // allocate memory
    transfer_cnfg->transfer_buffers[i] = malloc(transfer_size*sizeof(unsigned char));

    transfer_cnfg->transfers[i] = libusb_alloc_transfer(0);
    transfer_cnfg->transfer_data[i].transfer_finished = 0;
    transfer_cnfg->transfer_data[i].transfer_cnfg = transfer_cnfg; // give transfers access to transfer configuration data

    libusb_fill_bulk_transfer(
        transfer_cnfg->transfers[i], manager_instc->fx2_dev_handl, 0x82,
        transfer_cnfg->transfer_buffers[i],
        transfer_size * sizeof(unsigned char), &__libusb_transfer_callback,
        (void *)&(transfer_cnfg->transfer_data[i]), 500); // timeout currently hardcoded to 4 seconds.
  }

  return 0;
}

int fx2_submit_bulk_out_transfer(struct fx2_device_manager *manager_instc, struct fx2_bulk_transfer_config *transfer_cnfg) {
  if(g_list_find(manager_instc->active_bulk_transfers, transfer_cnfg) != NULL) {
    g_message("transfer already submitted");
    return 1;
  }

  for (size_t i = 0; i < transfer_cnfg->transfer_num; ++i) {
    transfer_cnfg->transfer_num_active++;
    libusb_submit_transfer(transfer_cnfg->transfers[i]);
  }

  manager_instc->active_bulk_transfers = g_list_append(manager_instc->active_bulk_transfers, transfer_cnfg);

  return 0;
}

int __fx2_check_libusb_transfers_canceled(struct fx2_bulk_transfer_config *transfer_cnfg) {
  for (size_t i = 0; i < transfer_cnfg->transfer_num; ++i) {
    libusb_cancel_transfer(transfer_cnfg->transfers[i]);
  }

  uint8_t all_finished = 0;
  while (!all_finished) {
    all_finished = 1;
    for (size_t i = 0; i < transfer_cnfg->transfer_num; ++i) {
      if(transfer_cnfg->transfer_data[i].transfer_finished==0) {
        all_finished = 0;
      }
    }
  }

  g_message("Canceled all pending transfers");

  return 0;
}

void fx2_stop_bulk_out_transfer(struct fx2_bulk_transfer_config *transfer_cnfg) {
  for (size_t i = 0; i < transfer_cnfg->transfer_num; ++i) {
    libusb_cancel_transfer(transfer_cnfg->transfers[i]);
  }
}

int fx2_free_bulk_transfer(struct fx2_bulk_transfer_config *transfer_cnfg) {
  // blocks until all transfers have finished
  /* __fx2_check_libusb_transfers_canceled(transfer_cnfg); */

  // free inner allocations
  for (size_t i = 0; i < transfer_cnfg->transfer_num; ++i) {
    libusb_free_transfer(transfer_cnfg->transfers[i]);
    free(transfer_cnfg->transfer_buffers[i]);
  }

  // free outer allocations
  free(transfer_cnfg->transfer_data);
  free(transfer_cnfg->transfers);
  free(transfer_cnfg->transfer_buffers);

  g_message("Successfully freed resources");

  return 0;
}

int fx2_reset_device(struct fx2_device_manager *manager_instc) {
  libusb_reset_device(manager_instc->fx2_dev_handl);
  return -1;
}

void fx2_handle_events(struct fx2_device_manager *manager_instc) {
  static struct timeval timeout = { .tv_sec = 0, .tv_usec = 1000};

  g_list_foreach(manager_instc->active_bulk_transfers,
                 &__check_transfer_finished_func,
                 manager_instc);

  if (g_list_length(manager_instc->finished_bulk_transfers) > 0) {
    g_list_foreach(manager_instc->finished_bulk_transfers,
                   &__call_finished_cb,
                   manager_instc);
  }

  libusb_handle_events_timeout(manager_instc->libusb_cntxt, &timeout);
}
