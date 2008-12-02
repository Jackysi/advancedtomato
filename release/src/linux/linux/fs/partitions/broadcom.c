/*
 * Broadcom NAS Kernel Partition Table Support
 *
 * Copyright 2003, Broadcom Corporation
 * All Rights Reserved.
 */


#include "broadcom.h"
#include <linux/pagemap.h>
#include "check.h"

extern int disk_repartition_minor_base;
extern int disk_repartition_minor_range;


#define BROADCOM_PARTITON_TABLE_HEADER "Broadcom NAS Version 1.1 MBR Tag"


int broadcom_partition(struct gendisk *hd, struct block_device *bdev,
		       unsigned long first_sector, int first_part_minor)
{
	unsigned char *raw_data;
	Sector sector_info;
	size_t byte_num;
	size_t block_offset;
	size_t block_num;

	raw_data = read_dev_sector(bdev, 0, &sector_info);
	if (raw_data == NULL)
		return -1;
	for (byte_num = 0; byte_num < sizeof(BROADCOM_PARTITON_TABLE_HEADER);
	     ++byte_num) {
		if (raw_data[byte_num] !=
		    BROADCOM_PARTITON_TABLE_HEADER[byte_num]) {
			put_dev_sector(sector_info);
			return 0;
		}
	}
	if (raw_data[100] == 0)
		block_offset = 1;
	else
		block_offset = 3;
	put_dev_sector(sector_info);

	for (block_num = 0; block_num < 2; ++block_num) {
		size_t partition_num;

		raw_data = read_dev_sector(bdev, block_num + block_offset,
					   &sector_info);
		if (raw_data == NULL)
			return -1;
		for (partition_num = 0; partition_num < 32; ++partition_num) {
			int minor_num;
			int block_count;
			int start_block_num;

			minor_num = first_part_minor + (block_num * 32) +
				    partition_num;
			if ((raw_data[(partition_num * 16) + 8] != 0) ||
			    (raw_data[(partition_num * 16) + 9] != 0) ||
			    (raw_data[(partition_num * 16) + 10] != 0) ||
			    (raw_data[(partition_num * 16) + 11] != 0) ||
			    (raw_data[(partition_num * 16) + 12] >= 0x80)) {
				printk(KERN_ERR
					"Broadcom partition size of "
					"0x%02lx%02lx%02lx%02lx%02lx%02lx%02lx"
					"%02lx blocks overflows signed 32-bit "
					"type used in the kernel for partition"
					" sizes.\n",
					(unsigned long)(raw_data[
						(partition_num * 16) + 8]),
					(unsigned long)(raw_data[
						(partition_num * 16) + 9]),
					(unsigned long)(raw_data[
						(partition_num * 16) + 10]),
					(unsigned long)(raw_data[
						(partition_num * 16) + 11]),
					(unsigned long)(raw_data[
						(partition_num * 16) + 12]),
					(unsigned long)(raw_data[
						(partition_num * 16) + 13]),
					(unsigned long)(raw_data[
						(partition_num * 16) + 14]),
					(unsigned long)(raw_data[
						(partition_num * 16) + 15]));
				put_dev_sector(sector_info);
				return -1;
			}
			block_count =
				((((int)(raw_data[
					(partition_num * 16) + 12])) << 24) |
				 (((int)(raw_data[
					(partition_num * 16) + 13])) << 16) |
				 (((int)(raw_data[
					(partition_num * 16) + 14])) << 8) |
				 (((int)(raw_data[
					(partition_num * 16) + 15])) << 0));
			if ((((block_num * 32) + partition_num) + 1) >=
				disk_repartition_minor_range) {
				if (block_count > 0) {
					printk(KERN_ERR
						"Broadcom partition entry %lu is non-empty but the "
						"device only supports %lu partitions.\n",
						(unsigned long)((block_num * 32) + partition_num),
						(unsigned long)(disk_repartition_minor_range - 1));
					put_dev_sector(sector_info);
					return -1;
				}
				continue;
			}
			if (block_count == 0) {
				if (hd->part[minor_num].nr_sects > 0) {
					kdev_t devp = MKDEV(hd->major,
						disk_repartition_minor_base +
						minor_num);
					invalidate_device(devp, 1);
				}
				hd->part[minor_num].start_sect = 0;
				hd->part[minor_num].nr_sects   = 0;
				continue;
			}
			if ((raw_data[(partition_num * 16) + 0] != 0) ||
			    (raw_data[(partition_num * 16) + 1] != 0) ||
			    (raw_data[(partition_num * 16) + 2] != 0) ||
			    (raw_data[(partition_num * 16) + 3] != 0) ||
			    (raw_data[(partition_num * 16) + 4] >= 0x80)) {
				printk(KERN_ERR
					"Broadcom partition starting block "
					"number "
					"0x%02lx%02lx%02lx%02lx%02lx%02lx%02lx"
					"%02lx overflows signed 32-bit type "
					"used in the kernel for partition "
					"start block numbers.\n",
					(unsigned long)(raw_data[
						(partition_num * 16) + 0]),
					(unsigned long)(raw_data[
						(partition_num * 16) + 1]),
					(unsigned long)(raw_data[
						(partition_num * 16) + 2]),
					(unsigned long)(raw_data[
						(partition_num * 16) + 3]),
					(unsigned long)(raw_data[
						(partition_num * 16) + 4]),
					(unsigned long)(raw_data[
						(partition_num * 16) + 5]),
					(unsigned long)(raw_data[
						(partition_num * 16) + 6]),
					(unsigned long)(raw_data[
						(partition_num * 16) + 7]));
				put_dev_sector(sector_info);
				return -1;
			}
			start_block_num =
				((((int)(raw_data[
					(partition_num * 16) + 4])) << 24) |
				 (((int)(raw_data[
					(partition_num * 16) + 5])) << 16) |
				 (((int)(raw_data[
					(partition_num * 16) + 6])) << 8) |
				 (((int)(raw_data[
					(partition_num * 16) + 7])) << 0));
			add_gd_partition(hd, minor_num,
				first_sector + start_block_num, block_count);
		}
		put_dev_sector(sector_info);
	}

	return 1;
}
