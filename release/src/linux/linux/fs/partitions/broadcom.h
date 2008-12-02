/*
 * Broadcom NAS Kernel Partition Table Support
 *
 * Copyright 2003, Broadcom Corporation
 * All Rights Reserved.
 */


#ifndef BROADCOM_PARTITION_H
#define BROADCOM_PARTITION_H

#include <linux/fs.h>
#include <linux/genhd.h>

int broadcom_partition(struct gendisk *hd, struct block_device *bdev,
		       unsigned long first_sector, int first_part_minor);

#endif /* BROADCOM_PARTITION_H */
