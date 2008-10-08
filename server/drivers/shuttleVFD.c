/*
 * ShuttleVFD driver
 *
 * Copyright (C) 2007 Thien Vu
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 *
 * Based on work from:
 *   LCDproc text driver
 *   setvfd from http://jeremy.infogami.com/SetVFD
 *   vfd from http://www.linuxowl.com/software/
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <usb.h>

#include "lcd.h"
#include "shuttleVFD.h"
#include "report.h"


// Variables in the driver module
MODULE_EXPORT char *api_version = API_VERSION;
MODULE_EXPORT int stay_in_foreground = 0;
MODULE_EXPORT int supports_multiple = 0;
MODULE_EXPORT char *symbol_prefix = "shuttleVFD_";


// Private data
typedef struct driver_private_data {
  usb_dev_handle *dev;
  int width;
  int height;
  int cellwidth;
  int cellheight;
  char *framebuf;
  char *last_framebuf;
  unsigned long icons;
  unsigned long last_icons;
} PrivateData;


//////////////////////////////////////////////////////////////////////////
//////////////////// For ShuttleVFD Output ///////////////////////////////
//////////////////////////////////////////////////////////////////////////

void send_packet(Driver *drvthis, char* packet) {
  PrivateData *p = drvthis->private_data;

  int i;
  for (i = 0; i < SHUTTLE_VFD_WRITE_ATTEMPTS; ++i) {
    if (usb_control_msg(p->dev,
                        0x21,
                        0x09,
                        0x0200,
                        0x0001,
                        packet,
                        SHUTTLE_VFD_PACKET_SIZE, 100) ==
	SHUTTLE_VFD_PACKET_SIZE) {
      usleep(SHUTTLE_VFD_SUCCESS_SLEEP_USEC);
      break;
    }
    report(RPT_ERR, "%s: write failed retrying...", drvthis->name);
    usleep(SHUTTLE_VFD_RETRY_SLEEP_USEC);
  }
}


MODULE_EXPORT int shuttleVFD_init(Driver *drvthis) {
  PrivateData *p;

  // allocate and store private data
  p = (PrivateData *)calloc(1, sizeof(PrivateData));
  if (p == NULL) {
    report(RPT_ERR, "%s: unable to allocate private data", drvthis->name);
    return -1;
  }
  if (drvthis->store_private_ptr(drvthis, p) < 0) {
    report(RPT_ERR, "%s: unable to store private data", drvthis->name);
    return -1;
  }

  // initialize private data
  p->dev = NULL;
  p->width = SHUTTLE_VFD_WIDTH;
  p->height = SHUTTLE_VFD_HEIGHT;
  p->cellwidth = SHUTTLE_VFD_CELLWIDTH;
  p->cellheight = SHUTTLE_VFD_CELLHEIGHT;
  p->framebuf = (char *)malloc(p->width * p->height);
  if (p->framebuf == NULL) {
    report(RPT_ERR, "%s: unable to create framebuffer", drvthis->name);
    return -1;
  }
  memset(p->framebuf, ' ', p->width * p->height);
  p->last_framebuf = (char *)malloc(p->width * p->height);
  if (p->last_framebuf == NULL) {
    report(RPT_ERR, "%s: unable to create second framebuffer", drvthis->name);
    return -1;
  }
  memset(p->last_framebuf, 0, p->width * p->height);
  p->icons = 0;
  p->last_icons = 0;

  // find VFD
  usb_init();
  usb_find_busses();
  usb_find_devices();
  struct usb_bus *bus;
  for (bus = usb_get_busses(); bus != NULL; bus = bus->next) {
    struct usb_device *dev;
    for (dev = bus->devices; dev != NULL; dev = dev->next) {
      if (dev->descriptor.idVendor == SHUTTLE_VFD_VENDOR_ID &&
          (dev->descriptor.idProduct == SHUTTLE_VFD_PRODUCT_ID1 || dev->descriptor.idProduct == SHUTTLE_VFD_PRODUCT_ID2)) {
        p->dev = usb_open(dev);
      }
    }
  }
  if (p->dev == NULL) {
    report(RPT_ERR, "%s: unable to find Shuttle VFD", drvthis->name);
    return -1;
  }
  int claim_rc = usb_claim_interface(p->dev, SHUTTLE_VFD_INTERFACE_NUM);
  if (claim_rc < 0) {
    report(RPT_ERR,
           "%s: unable to claim interface: %s",
	   drvthis->name, strerror(claim_rc));
    return -1;
  }

  report(RPT_DEBUG, "%s: init() done", drvthis->name);
  return 0;
}


MODULE_EXPORT void shuttleVFD_close(Driver *drvthis) {
  PrivateData *p = drvthis->private_data;
  if (p != NULL) {
    if (p->dev != NULL) {
      if (usb_release_interface(p->dev, SHUTTLE_VFD_INTERFACE_NUM) < 0) {
        report(RPT_ERR, "%s: unable to release interface", drvthis->name);
      }
      if (usb_close(p->dev) < 0) {
        report(RPT_ERR, "%s: unable to close device", drvthis->name);
      }
      p->dev = NULL;
    }
    if (p->framebuf != NULL) {
      free(p->framebuf);
    }
    if (p->last_framebuf != NULL) {
      free(p->last_framebuf);
    }
    free(p);
  }
  drvthis->store_private_ptr(drvthis, NULL);
}


MODULE_EXPORT int shuttleVFD_width(Driver *drvthis) {
  PrivateData *p = drvthis->private_data;
  return p->width;
}


MODULE_EXPORT int shuttleVFD_height(Driver *drvthis) {
  PrivateData *p = drvthis->private_data;
  return p->height;
}


MODULE_EXPORT void shuttleVFD_clear(Driver *drvthis) {
  PrivateData *p = drvthis->private_data;
  memset(p->framebuf, ' ', p->width * p->height);
  p->icons = 0;
}


MODULE_EXPORT void shuttleVFD_flush(Driver *drvthis) {
  PrivateData *p = drvthis->private_data;
  char packet[SHUTTLE_VFD_PACKET_SIZE];

  // set text
  if (strncmp(p->last_framebuf, p->framebuf, p->width * p->height) != 0) {
    // move cursor to front
    memset(packet, 0, SHUTTLE_VFD_PACKET_SIZE);
    packet[0] = (1 << 4) + 1;
    packet[1] = 2;
    send_packet(drvthis, packet);
  
    // write framebuf[0-6]
    memset(packet, 0, SHUTTLE_VFD_PACKET_SIZE);
    packet[0] = (9 << 4) + 7;
    strncpy(&packet[1], &p->framebuf[0], 7);
    send_packet(drvthis, packet);
  
    // write framebuf[7-13]
    memset(packet, 0, SHUTTLE_VFD_PACKET_SIZE);
    packet[0] = (9 << 4) + 7;
    strncpy(&packet[1], &p->framebuf[7], 7);
    send_packet(drvthis, packet);
  
    // write framebuf[14-20]
    memset(packet, 0, SHUTTLE_VFD_PACKET_SIZE);
    packet[0] = (9 << 4) + 6;
    strncpy(&packet[1], &p->framebuf[14], 6);
    send_packet(drvthis, packet);

    strncpy(p->last_framebuf, p->framebuf, p->width * p->height);
  }

  // set icons
  if (p->last_icons != p->icons) {
    memset(packet, 0, SHUTTLE_VFD_PACKET_SIZE);
    packet[0] = (7 << 4) + 4;
    packet[1] = (p->icons >> 15) & 0x1f;
    packet[2] = (p->icons >> 10) & 0x1f;
    packet[3] = (p->icons >> 5) & 0x1f;
    packet[4] = p->icons & 0x1f;
    send_packet(drvthis, packet);

    p->last_icons = p->icons;
  }
}

MODULE_EXPORT void shuttleVFD_string(
    Driver *drvthis, int x, int y, const char string[]) {
  PrivateData *p = drvthis->private_data;

  --x; --y;
  if (y < 0 || y >= p->height)
    return;

  int i;
  for (i = 0; (string[i] != '\0') && (x < p->width); ++i, ++x) {
    if (x >= 0) {    // no write left of left border
      p->framebuf[(y * p->width) + x] = string[i];
    }
  }
}


MODULE_EXPORT void shuttleVFD_chr(Driver *drvthis, int x, int y, char c) {
  PrivateData *p = drvthis->private_data;

  --x; --y;
  if (x >= 0 && x < p->width && y >= 0 && y < p->height) {
    p->framebuf[(y * p->width) + x] = c;
  }
}


MODULE_EXPORT int shuttleVFD_icon(Driver *drvthis, int x, int y, int icon) {
  PrivateData *p = drvthis->private_data;

  switch (icon) {
    case ICON_STOP:
      p->icons = SHUTTLE_VFD_ICON_STOP;
      return 0;
    case ICON_PAUSE:
      p->icons = SHUTTLE_VFD_ICON_PAUSE;
      return 0;
    case ICON_PLAY:
      p->icons = SHUTTLE_VFD_ICON_PLAY;
      return 0;
    case ICON_PLAYR:
      p->icons = SHUTTLE_VFD_ICON_REVERSE;
      return 0;
    case ICON_FF:
      p->icons = SHUTTLE_VFD_ICON_FASTFORWARD;
      return 0;
    case ICON_FR:
      p->icons = SHUTTLE_VFD_ICON_REWIND;
      return 0;
    case ICON_REC:
      p->icons = SHUTTLE_VFD_ICON_RECORD;
      return 0;
    default:
      return -1;
  }
}


MODULE_EXPORT int shuttleVFD_cellwidth(Driver *drvthis) {
  PrivateData *p = drvthis->private_data;
  return p->cellwidth;
}


MODULE_EXPORT int shuttleVFD_cellheight(Driver *drvthis) {
  PrivateData *p = drvthis->private_data;
  return p->cellheight;
}