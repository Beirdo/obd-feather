#include <zephyr.h>
#include <kernel.h>
#include <device.h>
#include <fs/fs.h>
#include <storage/disk_access.h>
#include <logging/log.h>
#include <ff.h>

#include "sdcard.h"

LOG_MODULE_REGISTER(sdcard, 3);

static FATFS fatfs;
static struct fs_mount_t mp = {
    .type = FS_FATFS,
    .fs_data = &fatfs,
};

const char *disk_mount_pt = "/" CONFIG_SDMMC_VOLUME_NAME ":";
const char *disk_pdrv = CONFIG_SDMMC_VOLUME_NAME;

void sdcard_init(void)
{
    int status;
    uint64_t memory_size_mb;
    uint32_t block_count;
    uint32_t block_size;

    do {
        if (disk_access_init(disk_pdrv)) {
            LOG_ERR("%s", "Storage init ERROR!");
            break;
        }

        if (disk_access_ioctl(disk_pdrv, DISK_IOCTL_GET_SECTOR_COUNT, &block_count)) {
            LOG_ERR("%s", "Unable to get block count");
            break;
        }
        LOG_INF("Block count %u", block_count);

        if (disk_access_ioctl(disk_pdrv, DISK_IOCTL_GET_SECTOR_SIZE, &block_size)) {
            LOG_ERR("%s", "Unable to get block size");
            break;
        }
        LOG_INF("Block size %u", block_size);

        memory_size_mb = (uint64_t)block_count * block_size;
        LOG_INF("Memory Size (MB) %u", (uint32_t)(memory_size_mb >> 20));
    } while(0);

    mp.mnt_point = disk_mount_pt;

    status = fs_mount(&mp);
    if (status != FR_OK) {
        LOG_ERR("%s", "Error mounting disk.");
    } else {
        LOG_INF("%s mounted", disk_mount_pt);
    }
}
