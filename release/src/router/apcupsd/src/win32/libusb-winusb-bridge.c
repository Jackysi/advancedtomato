/*
 * libusb-winusb-bridge.c
 *
 * Simple backend for libusb using MS WinUsb. Only the basic functions
 * necessary for apcupsd are implemented, although the others could be added
 * fairly easily.
 */

/*
 * Copyright (C) 2010 Adam Kropelin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#include "libusb-winusb-bridge.h"
#include <windows.h>
#include <rpc.h>
#include <setupapi.h>
#include "apc.h"

// winusb.dll entrypoints
DLL_DECLARE(WINAPI, BOOL, WinUsb_Initialize,
            (HANDLE, PWINUSB_INTERFACE_HANDLE));
DLL_DECLARE(WINAPI, BOOL, WinUsb_Free, (WINUSB_INTERFACE_HANDLE));
DLL_DECLARE(WINAPI, BOOL, WinUsb_GetAssociatedInterface,
            (WINUSB_INTERFACE_HANDLE, UCHAR, PWINUSB_INTERFACE_HANDLE));
DLL_DECLARE(WINAPI, BOOL, WinUsb_GetDescriptor,
            (WINUSB_INTERFACE_HANDLE, UCHAR, UCHAR, USHORT, PUCHAR,
             ULONG, PULONG));
DLL_DECLARE(WINAPI, BOOL, WinUsb_QueryInterfaceSettings,
            (WINUSB_INTERFACE_HANDLE, UCHAR, PUSB_INTERFACE_DESCRIPTOR));
DLL_DECLARE(WINAPI, BOOL, WinUsb_QueryDeviceInformation,
            (WINUSB_INTERFACE_HANDLE, ULONG, PULONG, PVOID));
DLL_DECLARE(WINAPI, BOOL, WinUsb_SetCurrentAlternateSetting,
            (WINUSB_INTERFACE_HANDLE, UCHAR));
DLL_DECLARE(WINAPI, BOOL, WinUsb_GetCurrentAlternateSetting,
            (WINUSB_INTERFACE_HANDLE, PUCHAR));
DLL_DECLARE(WINAPI, BOOL, WinUsb_QueryPipe,
            (WINUSB_INTERFACE_HANDLE, UCHAR, UCHAR,
             PWINUSB_PIPE_INFORMATION));
DLL_DECLARE(WINAPI, BOOL, WinUsb_SetPipePolicy,
            (WINUSB_INTERFACE_HANDLE, UCHAR, ULONG, ULONG, PVOID));
DLL_DECLARE(WINAPI, BOOL, WinUsb_GetPipePolicy,
            (WINUSB_INTERFACE_HANDLE, UCHAR, ULONG, PULONG, PVOID));
DLL_DECLARE(WINAPI, BOOL, WinUsb_ReadPipe,
            (WINUSB_INTERFACE_HANDLE, UCHAR, PUCHAR, ULONG, PULONG,
             LPOVERLAPPED));
DLL_DECLARE(WINAPI, BOOL, WinUsb_WritePipe,
            (WINUSB_INTERFACE_HANDLE, UCHAR, PUCHAR, ULONG, PULONG,
             LPOVERLAPPED));
DLL_DECLARE(WINAPI, BOOL, WinUsb_ControlTransfer,
            (WINUSB_INTERFACE_HANDLE, WINUSB_SETUP_PACKET, PUCHAR, ULONG,
             PULONG, LPOVERLAPPED));
DLL_DECLARE(WINAPI, BOOL, WinUsb_ResetPipe,
            (WINUSB_INTERFACE_HANDLE, UCHAR));
DLL_DECLARE(WINAPI, BOOL, WinUsb_AbortPipe,
            (WINUSB_INTERFACE_HANDLE, UCHAR));
DLL_DECLARE(WINAPI, BOOL, WinUsb_FlushPipe,
            (WINUSB_INTERFACE_HANDLE, UCHAR));

// GUID for Apcupsd-owned UPSes, as specified in our INF file
// Binary equivalent of {8c534620-f7e6-11de-8a39-0800200c9a66}
GUID APCUPSD_DEVICE_GUID =
{ 
   0x8c534620,
   0xf7e6,
   0x11de,
   {0x8a, 0x39, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66}
};

#define MAX_USB_DEVICES 10
static struct usb_device devices[MAX_USB_DEVICES];
static struct usb_bus bus = { NULL, NULL, "bus0", devices };

struct usb_dev_handle
{
   HANDLE hnd;
   WINUSB_INTERFACE_HANDLE fd;
};

void usb_init(void)
{
   // Initialize DLL functions
   if (!WinUsb_Initialize)
   {
      DLL_LOAD(winusb.dll, WinUsb_Initialize);
      DLL_LOAD(winusb.dll, WinUsb_Free);
      DLL_LOAD(winusb.dll, WinUsb_GetAssociatedInterface);
      DLL_LOAD(winusb.dll, WinUsb_GetDescriptor);
      DLL_LOAD(winusb.dll, WinUsb_QueryInterfaceSettings);
      DLL_LOAD(winusb.dll, WinUsb_QueryDeviceInformation);
      DLL_LOAD(winusb.dll, WinUsb_SetCurrentAlternateSetting);
      DLL_LOAD(winusb.dll, WinUsb_GetCurrentAlternateSetting);
      DLL_LOAD(winusb.dll, WinUsb_QueryPipe);
      DLL_LOAD(winusb.dll, WinUsb_SetPipePolicy);
      DLL_LOAD(winusb.dll, WinUsb_GetPipePolicy);
      DLL_LOAD(winusb.dll, WinUsb_ReadPipe);
      DLL_LOAD(winusb.dll, WinUsb_WritePipe);
      DLL_LOAD(winusb.dll, WinUsb_ControlTransfer);
      DLL_LOAD(winusb.dll, WinUsb_ResetPipe);
      DLL_LOAD(winusb.dll, WinUsb_AbortPipe);
      DLL_LOAD(winusb.dll, WinUsb_FlushPipe);
   }
}

int usb_find_busses(void)
{
   return 1;
}

static int usb_get_device_desc(struct usb_device *dev)
{
   struct usb_dev_handle *hnd = usb_open(dev);
   if (!hnd)
      return 1;

   ULONG actlen = 0;
   if (!WinUsb_GetDescriptor(hnd->fd, USB_DEVICE_DESCRIPTOR_TYPE, 0, 0, 
                             (unsigned char*)&dev->descriptor, 
                             sizeof(dev->descriptor), &actlen)
       || actlen != sizeof(dev->descriptor))
   {
      return 1;
   }

   // Descriptor as read from the device is in little-endian format. No need
   // to convert since this is guaranteed to be Windows which runs only on
   // little-endian processors.

   return usb_close(hnd);
}

int usb_find_devices(void)
{
   // Get the set of device interfaces that have been matched by our INF
   HDEVINFO deviceInfo = SetupDiGetClassDevs(
      &APCUPSD_DEVICE_GUID, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
   if (!deviceInfo)
   {
      return 0;
   }

   // Iterate over all interfaces
   int ndevs = 0;
   int devidx = 0;
   while (ndevs < MAX_USB_DEVICES)
   {
      // Get interface data for next interface and attempt to init it
      SP_DEVICE_INTERFACE_DATA interfaceData;
      interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
      if (!SetupDiEnumDeviceInterfaces(
            deviceInfo, NULL, &APCUPSD_DEVICE_GUID, devidx++, &interfaceData))
      {
         break;
      }

      // Determine required size for interface detail data
      ULONG requiredLength = 0;
      SetupDiGetDeviceInterfaceDetail(
         deviceInfo, &interfaceData, NULL, 0, &requiredLength, NULL);

      // Allocate storage for interface detail data
      PSP_DEVICE_INTERFACE_DETAIL_DATA detailData = 
         (PSP_DEVICE_INTERFACE_DETAIL_DATA) malloc(requiredLength);
      detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

      // Fetch interface detail data
      if (!SetupDiGetDeviceInterfaceDetail(
            deviceInfo, &interfaceData, detailData, requiredLength, 
            &requiredLength, NULL))
      {
         free(detailData);
         continue;
      }

      // Populate device structure
      struct usb_device *dev = devices+ndevs;
      memset(dev, 0, sizeof(*dev));
      astrncpy(dev->filename, detailData->DevicePath, sizeof(dev->filename));
      dev->bus = &bus;
      if (ndevs)
      {
         dev->prev = devices + ndevs - 1;
         dev->prev->next = dev;
      }

      // Fetch device descriptor structure
      if (usb_get_device_desc(dev) == 0)
         ndevs++;

      free(detailData);
   }

   SetupDiDestroyDeviceInfoList(deviceInfo);
   return ndevs;
}

struct usb_bus *usb_get_busses(void)
{
   return &bus;
}

usb_dev_handle *usb_open(struct usb_device *dev)
{
   // Open generic handle to device
   HANDLE hnd = CreateFile(dev->filename,
                           GENERIC_WRITE | GENERIC_READ,
                           FILE_SHARE_WRITE | FILE_SHARE_READ,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                           NULL);
   if (hnd == NULL)
      return NULL;

   // Initialize WinUSB for this device and get a WinUSB handle for it
   WINUSB_INTERFACE_HANDLE fd;
   if (!WinUsb_Initialize(hnd, &fd))
   {
      CloseHandle(hnd);
      return NULL;
   }

   // Device opened successfully. Allocate storage for handles.
   struct usb_dev_handle *usb = 
      (struct usb_dev_handle *)malloc(sizeof(struct usb_dev_handle));
   usb->hnd = hnd;
   usb->fd = fd;
   return usb;
}

int usb_close(usb_dev_handle *dev)
{
   WinUsb_Free(dev->fd);
   CloseHandle(dev->hnd);
   free(dev);
   return 0;
}

int usb_set_configuration(usb_dev_handle *dev, int configuration)
{
   return 0;
}

int usb_claim_interface(usb_dev_handle *dev, int iface)
{
   return 0;
}

int usb_control_msg(usb_dev_handle *dev, int requesttype, 
                    int request, int value, int index, 
                    char *bytes, int size, int timeout)
{
   // Format parameters into a ctrl setup packet
   // 'timeout' is ignored; WinUSB has a 5 second default timeout on the ctrl
   // pipe, which is good enough for us.
   WINUSB_SETUP_PACKET sp;
   sp.RequestType = requesttype;
   sp.Request = request;
   sp.Value = value;
   sp.Index = index;
   sp.Length = size;

   ULONG actlen = 0;
   if (!WinUsb_ControlTransfer(dev->fd, sp, (unsigned char*)bytes, size, &actlen, NULL))
      return -GetLastError();

   return actlen;
}

int usb_get_string_simple(usb_dev_handle *dev, int index, char *buf, 
                          size_t buflen)
{
   unsigned char temp[MAXIMUM_USB_STRING_LENGTH];

   ULONG actlen = 0;
   if (!WinUsb_GetDescriptor(dev->fd, USB_STRING_DESCRIPTOR_TYPE, index, 0x0409, 
                             temp, sizeof(temp), &actlen))
   {
      return -GetLastError();
   }

   // Skip first two bytes of result (descriptor id and length), then take 
   // every other byte as a cheap way to convert Unicode to ASCII
   unsigned int i, j;
   for (i = 2, j = 0; i < actlen && j < (buflen-1); i+=2, ++j)
      buf[j] = temp[i];
   buf[j] = '\0';

   return strlen(buf);
}

#define LIBUSB_ETIMEDOUT 116
int usb_interrupt_read(usb_dev_handle *dev, int ep, char *bytes, int size, 
                       int timeout)
{
   // Set timeout on endpoint
   // It might be more efficient to use async i/o here, but MS docs imply that
   // you have to create a new sync object for every call, which doesn't seem
   // efficient at all. So just set the pipe timeout and use sync i/o.
   ULONG tmp = timeout;
   if (!WinUsb_SetPipePolicy(dev->fd, ep, PIPE_TRANSFER_TIMEOUT, sizeof(tmp), &tmp))
      return -EINVAL;

   // Perform transfer
   tmp = 0;
   if (!WinUsb_ReadPipe(dev->fd, ep, (unsigned char*)bytes, size, &tmp, NULL))
   {
      tmp = GetLastError();
      if (tmp == ERROR_SEM_TIMEOUT)
         return -LIBUSB_ETIMEDOUT;
      else
         return -EINVAL;
   }

   return tmp;
}

char *usb_strerror(void)
{
   static char buf[256];
   snprintf(buf, sizeof(buf), "Windows Error #%d", GetLastError());
   return buf;
}

int usb_reset(usb_dev_handle *dev)
{
   return 0;
}

void usb_set_debug(int level)
{
}
