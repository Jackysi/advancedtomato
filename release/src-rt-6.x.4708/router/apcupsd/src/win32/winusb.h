#ifndef __WINUSB_H
#define __WINUSB_H

#include "ddk/usb100.h"
#include "ddk/usbioctl.h"

// Windows API default is uppercase - ugh!
#if !defined(bool)
#define bool BOOLEAN
#endif
#if !defined(true)
#define true TRUE
#endif
#if !defined(false)
#define false FALSE
#endif


/*
 * Some of the EX stuff is not yet in MinGW => define it
 */
#ifndef USB_GET_NODE_CONNECTION_INFORMATION_EX
#define USB_GET_NODE_CONNECTION_INFORMATION_EX 274
#endif

#ifndef IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX
#define IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX \
        CTL_CODE(FILE_DEVICE_USB, USB_GET_NODE_CONNECTION_INFORMATION_EX, \
        METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

#ifndef USB_NODE_CONNECTION_INFORMATION_EX
typedef struct _USB_NODE_CONNECTION_INFORMATION_EX {
        ULONG ConnectionIndex;
        USB_DEVICE_DESCRIPTOR DeviceDescriptor;
        UCHAR CurrentConfigurationValue;
        UCHAR Speed;
        BOOLEAN DeviceIsHub;
        USHORT DeviceAddress;
        ULONG NumberOfOpenPipes;
        USB_CONNECTION_STATUS ConnectionStatus;
        USB_PIPE_INFO PipeList[0];
} USB_NODE_CONNECTION_INFORMATION_EX, *PUSB_NODE_CONNECTION_INFORMATION_EX;
#endif

#ifndef USB_HUB_CAP_FLAGS
typedef union _USB_HUB_CAP_FLAGS {
        ULONG ul;
        struct {
                ULONG HubIsHighSpeedCapable:1;
                ULONG HubIsHighSpeed:1;
                ULONG HubIsMultiTtCapable:1;
                ULONG HubIsMultiTt:1;
                ULONG HubIsRoot:1;
                ULONG HubIsArmedWakeOnConnect:1;
                ULONG ReservedMBZ:26;
        };
} USB_HUB_CAP_FLAGS, *PUSB_HUB_CAP_FLAGS;
#endif

#ifndef USB_HUB_CAPABILITIES_EX
typedef struct _USB_HUB_CAPABILITIES_EX {
        USB_HUB_CAP_FLAGS CapabilityFlags;
} USB_HUB_CAPABILITIES_EX, *PUSB_HUB_CAPABILITIES_EX;
#endif

#ifndef USB_GET_HUB_CAPABILITIES_EX
#define USB_GET_HUB_CAPABILITIES_EX 276
#endif

#ifndef IOCTL_USB_GET_HUB_CAPABILITIES_EX
#define IOCTL_USB_GET_HUB_CAPABILITIES_EX \
        CTL_CODE( FILE_DEVICE_USB, USB_GET_HUB_CAPABILITIES_EX, \
        METHOD_BUFFERED, FILE_ANY_ACCESS )
#endif

/*
 * WinUSB macros - from libusb-win32 1.x
 */

#define DLL_DECLARE(api, ret, name, args) \
   typedef ret (api * __dll_##name##_t)args; \
   static __dll_##name##_t name

#define DLL_LOAD(dll, name)                                   \
  do {                                                        \
  HMODULE h = GetModuleHandle(#dll);                          \
  if(!h)                                                      \
    h = LoadLibrary(#dll);                                    \
  if(!h)                                                      \
    break;                                                    \
  if((name = (__dll_##name##_t)GetProcAddress(h, #name)))     \
    break;                                                    \
  if((name = (__dll_##name##_t)GetProcAddress(h, #name "A"))) \
    break;                                                    \
  if((name = (__dll_##name##_t)GetProcAddress(h, #name "W"))) \
    break;                                                    \
  } while(0)


/* winusb.dll interface */

#define SHORT_PACKET_TERMINATE  0x01
#define AUTO_CLEAR_STALL        0x02
#define PIPE_TRANSFER_TIMEOUT   0x03
#define IGNORE_SHORT_PACKETS    0x04
#define ALLOW_PARTIAL_READS     0x05
#define AUTO_FLUSH              0x06
#define RAW_IO                  0x07
#define MAXIMUM_TRANSFER_SIZE   0x08
#define AUTO_SUSPEND            0x81
#define SUSPEND_DELAY           0x83
#define DEVICE_SPEED            0x01
#define LowSpeed                0x01
#define FullSpeed               0x02
#define HighSpeed               0x03

typedef enum _USBD_PIPE_TYPE {
        UsbdPipeTypeControl,
        UsbdPipeTypeIsochronous,
        UsbdPipeTypeBulk,
        UsbdPipeTypeInterrupt
} USBD_PIPE_TYPE;

typedef struct {
  USBD_PIPE_TYPE PipeType;
  UCHAR          PipeId;
  USHORT         MaximumPacketSize;
  UCHAR          Interval;
} WINUSB_PIPE_INFORMATION, *PWINUSB_PIPE_INFORMATION;

#pragma pack(1)
typedef struct {
  UCHAR  RequestType;
  UCHAR  Request;
  USHORT Value;
  USHORT Index;
  USHORT Length;
} WINUSB_SETUP_PACKET, *PWINUSB_SETUP_PACKET;
#pragma pack()

typedef void *WINUSB_INTERFACE_HANDLE, *PWINUSB_INTERFACE_HANDLE;

#endif // __WINUSB_H
