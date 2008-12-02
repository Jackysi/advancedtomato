
#include <asm/types.h>
#include <asm/ioctl.h>

#include <linux/usb_ch9.h>

/*
 * Filesystem based user-mode API to USB Gadget controller hardware
 *
 * Almost everything can be done with only read and write operations,
 * on endpoint files found in one directory.  They are configured by
 * writing descriptors, and then may be used for normal stream style
 * i/o requests.  When ep0 is configured, the device can enumerate;
 * when it's closed, the device disconnects from usb.
 */

/*
 * Events are delivered on the ep0 file descriptor, if the user mode driver
 * reads from this file descriptor after writing the descriptors.  Don't
 * stop polling this descriptor, if you write that kind of driver.
 */

enum usb_gadgetfs_event_type {
	GADGETFS_NOP = 0,

	GADGETFS_CONNECT,
	GADGETFS_DISCONNECT,
	GADGETFS_CONFIGURATION,
	GADGETFS_SETUP,
	GADGETFS_SUSPEND,
	// and likely more !
};

struct usb_gadgetfs_event {
	enum usb_gadgetfs_event_type	type;
	union {
		// NOP, DISCONNECT, SUSPEND: nothing

		// CONNECT: just the speed
		enum usb_device_speed	speed;

		// CONFIGURATION:  speed, config number
		struct {
			enum usb_device_speed	speed;
			__u8			bConfigurationValue;
		} config;

		// SETUP: packet; DATA phase i/o precedes next event
		// (setup.bmRequestType & USB_DIR_IN) flags direction 
		struct usb_ctrlrequest	setup;
	} u;
};


/* endpoint ioctls */

/* IN transfers may be reported to the gadget driver as complete
 * 	when the fifo is loaded, before the host reads the data;
 * OUT transfers may be reported to the host's "client" driver as
 * 	complete when they're sitting in the FIFO unread.
 * THIS returns how many bytes are "unclaimed" in the endpoint fifo
 */
#define	GADGETFS_FIFO_STATUS	_IO('g',1)

/* discards any unclaimed data in the fifo. */
#define	GADGETFS_FIFO_FLUSH	_IO('g',2)

/* resets endpoint halt+toggle, as with set_interface */
#define	GADGETFS_CLEAR_HALT	_IO('g',3)


