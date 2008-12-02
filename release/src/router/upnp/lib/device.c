/*
	Copyright 2005, Broadcom Corporation      
	All Rights Reserved.      
		  
	THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY      
	KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM      
	SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS      
	FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.      
*/

#include <stdarg.h>

#include "upnp_osl.h"
#include "upnp_dbg.h"
#include "upnp.h"
#include <bcmnvram.h>
#include <shared.h>

extern PService init_service(PServiceTemplate svctmpl, PDevice pdev);
extern void destroy_service(PService);

void device_devicelist(PDevice pdev, UFILE *up);
void device_servicelist(PDevice pdev, UFILE *up);

PDevice root_devices = NULL;

/* recursive routine to initialize a device, and all its subdevices.
   Also all services within a device are intialized by calling their
   specific initialization function, if available.  */
PDevice init_device(PDevice parent, PDeviceTemplate pdevtmpl, ...)
{
	static int indent = 0;
	int i;
	PFDEVINIT func;
	PDevice pdev, subdev;
	PService psvc;
	va_list ap;

	UPNP_TRACE(("%*sInitializing %sdevice \"%s\".\r\n", indent, "", (parent ? "" : "root "), pdevtmpl->type));

	if (pdevtmpl->schema == NULL)
		pdevtmpl->schema = "schemas-upnp-org";

	pdev = (Device *) malloc(sizeof(Device));
	memset(pdev, 0, sizeof(Device));
	pdev->parent = parent;
	pdev->template = pdevtmpl;

	// call the device's intialization function, if defined.
	if ((func = pdevtmpl->devinit) != NULL) {
		va_start(ap, pdevtmpl);
		(*func)(pdev, DEVICE_CREATE, ap);
		va_end(ap);
	}

	// we do a top down, depth-first traversal of the device heirarchy.
	// sub-devices will be initialized before we complete initialization of the root device.
	//
	for (i = 0; i < pdevtmpl->ndevices; i++) {
		indent += 4;
		subdev = init_device(pdev, &(pdevtmpl->devicelist[i]));
		indent -= 4;
		subdev->next = pdev->subdevs;
		pdev->subdevs = subdev;
	}

	// Initialize each service in this device.
	//
	for (i = 0; i < pdevtmpl->nservices; i++) {
		psvc = init_service(pdevtmpl->services[i], pdev);

		psvc->next = pdev->services;
		pdev->services = psvc;
	}

	if (ISROOT(pdev)) {
		pdev->next = root_devices;
		root_devices = pdev;
	}

	return pdev;
}

void destroy_device(PDevice pdev)
{
	static int indent = 0;
	PFDEVINIT func;
	PDevice psubdev, nextdev, *ppdev;
	PService psvc, nextsvc;

	UPNP_TRACE(("%*sDestroying %sdevice \"%s\".\r\n", indent, "", (ISROOT(pdev) ? "root " : ""), pdev->template->type));

	// destroy all subdevices in this device.
	if (pdev->subdevs) {
		for (psubdev = pdev->subdevs; psubdev; psubdev = nextdev) {
			nextdev = psubdev->next;
			indent += 4;
			destroy_device(psubdev);
			indent -= 4;
		}
	}

	// destroy all service in this device.
	if (pdev->services) {
		for (psvc = pdev->services; psvc; psvc = nextsvc) {
			nextsvc = psvc->next;
			destroy_service(psvc);
		}
	}

	// call this device's destroy function, if defined.
	if ((func = pdev->template->devinit) != NULL) {
		(*func)(pdev, DEVICE_DESTROY, (va_list) NULL );
	}

	// remove the device from the root device list
	if (ISROOT(pdev)) {
		for (ppdev = &root_devices; *ppdev; ppdev = &(*ppdev)->next) {
			if (*ppdev == pdev) {
				*ppdev = (*ppdev)->next;
				break;
			}
		}
	}

	// finally, free the memory allocated in init_device();
	free(pdev);
}


/* Print an XML device description for a device and all its subdevices.
   We used to just print the static XML device description from a file, but now that the 
   IGD is more dynamic and can adjust to different gateway configurations,
   we must dynamically generate the XML.
 */
void device_xml(PDevice pdev, UFILE *up)
{
	PFDEVXML func;
	char *friendlyname;
	char *myip;
	int https;

	// call the device's xml function, if defined.
	if ((func = pdev->template->devxml) != NULL) {
		(*func)(pdev, up);
		return;
	}

	myip = nvram_safe_get("lan_ipaddr");

	if (ISROOT(pdev)) {
		uprintf(up, 
			"<?xml version=\"1.0\"?>\r\n"
			"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">\r\n"
			"<specVersion>\r\n"
			"<major>1</major>\r\n"
			"<minor>0</minor>\r\n"
			"</specVersion>\r\n"
			"<URLBase>http://%s:5431/</URLBase>\r\n",
			myip);
	}

	if (pdev->friendlyname)	friendlyname = pdev->friendlyname;
		else friendlyname = pdev->template->type;

	uprintf(up,
		"<device>\r\n"
		"<deviceType>%s</deviceType>\r\n"
		"<friendlyName>%s</friendlyName>\r\n"
		"<UDN>%s</UDN>\r\n",
		pdev->template->type,
		friendlyname,
		pdev->udn);
		
	uprintf(up,
		"<manufacturer>Tomato</manufacturer>\r\n"
		"<manufacturerURL>http://www.polarcloud.com/tomato/</manufacturerURL>\r\n"
		"<modelDescription>Router/Gateway</modelDescription>\r\n"
		"<modelName>%s</modelName>\r\n"
		"<modelNumber>v%s</modelNumber>\r\n"
		"<modelURL>http://www.polarcloud.com/tomato/</modelURL>\r\n",
		nvram_safe_get("t_model_name"), tomato_version);

	if (nvram_match("upnp_mnp", "1")) {
		https = nvram_match("https_enable", "1");
		uprintf(up,
			"<presentationURL>http%s://%s:%s/forward-upnp.asp</presentationURL>\r\n",
			https ? "s" : "",
			myip,
			nvram_safe_get(https ? "https_lanport" : "http_lanport"));
	}

	// generate XML for any services in this device.
	device_servicelist(pdev, up);

	// generate XML for any subdevices in this device.
	device_devicelist(pdev, up);

	uprintf(up, "</device>\r\n");

	if (ISROOT(pdev)) {
		uprintf(up, "</root>\r\n");
	}
}


void device_devicelist(PDevice pdev, UFILE *up)
{
	PDevice  psubdev;

	// generate XML for any subdevices in this device.
	if (pdev->subdevs) {
		uprintf(up, "<deviceList>\r\n");
		for (psubdev = pdev->subdevs; psubdev; psubdev = psubdev->next)
			device_xml(psubdev, up);
		uprintf(up, "</deviceList>\r\n");
	}
}


void device_servicelist(PDevice pdev, UFILE *up)
{
	char svcurl[200];
	PService psvc;

	// generate XML for any services in this device.
	if (pdev->services) {
		uprintf(up, "<serviceList>\r\n");
		forall_services(pdev, psvc) {
			snprintf(svcurl, sizeof(svcurl), "/%s/%s", pdev->udn, psvc->template->name);

			uprintf(up, "<service>\r\n");
			uprintf(up, "<serviceType>urn:%s:service:%s</serviceType>\r\n", 
					psvc->template->schema, psvc->template->name);
			if (psvc->template->serviceid) {
				uprintf(up, "<serviceId>%s%d</serviceId>\r\n", 
						psvc->template->serviceid, psvc->instance);
			} else {
				uprintf(up, "<serviceId>urn:upnp-org:serviceId:%s%d</serviceId>\r\n", 
						psvc->template->name, psvc->instance);
			}
			uprintf(up, "<controlURL>/%s/%s</controlURL>\r\n", pdev->udn, psvc->template->name);
			uprintf(up, "<eventSubURL>/%s/%s</eventSubURL>\r\n", pdev->udn, psvc->template->name);
			uprintf(up, "<SCPDURL>/dynsvc/%s.xml</SCPDURL>\r\n", psvc->template->name);
			uprintf(up, "</service>\r\n");
		}
		uprintf(up, "</serviceList>\r\n");
	}
}


/* Given a device pointer, return the root device for that device. */
PDevice rootdev(PDevice pdev)
{
	while (pdev->parent)
		pdev = pdev->parent;
	return pdev;
}


/* Device iterator used by the forall_devices() macro.
   
   The first call to device_iterator() should have NULL as its argument.  
   Subsequent calls will return the next device in depth first, pre-order.  
*/
PDevice device_iterator(PDevice pdev)
{
	PDevice nextdev;

	if (pdev == NULL) {
		nextdev = root_devices;
	} else {
		if (pdev->subdevs) {
			nextdev = pdev->subdevs;
		} else {
			nextdev = pdev;
			while (nextdev) {
				if (nextdev->next) {
					nextdev = nextdev->next;
					break;
				} else {
					nextdev = nextdev->parent;
				}
			}
		} 
	}

	return nextdev;
}

