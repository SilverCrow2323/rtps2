#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <string.h>
#include <errno.h>

#include <sbv_patches.h>

#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>

#include "nexus_filexio.h"

extern unsigned char iomanX_irx[] __attribute__((aligned(16)));
extern unsigned int size_iomanX_irx;
extern unsigned char fileXio_irx[] __attribute__((aligned(16)));
extern unsigned int size_fileXio_irx;

#define IOMANX_MODULE_NAME "IOX/File_Manager"
#define FILEXIO_MODULE_NAME "IOX/File_Manager_Rpc"

static int filexio_ready;

static int ensure_module(const char *name, void *image, unsigned int image_size)
{
    int module_id = SifSearchModuleByName(name);
    int result;

    if (module_id >= 0)
        return 0;

    result = SifExecModuleBuffer(image, image_size, 0U, 0, 0);
    if (result < 0)
        return result;
    return 0;
}

int nexus_filexio_activate(void)
{
    int result;

    if (filexio_ready)
        return 0;

    /*
     * This is intentionally not an IOP reset. The IOMANX build bundled by
     * PS2SDK has its legacy IOMAN hook enabled so it can coexist with the
     * standard retail IOMAN. Activation is explicit and never automatic.
     */
    sceSifInitRpc(0);
    sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check();

    result = ensure_module(IOMANX_MODULE_NAME,
                           iomanX_irx, size_iomanX_irx);
    if (result < 0)
        return -1;

    result = ensure_module(FILEXIO_MODULE_NAME,
                           fileXio_irx, size_fileXio_irx);
    if (result < 0)
        return -2;

    result = fileXioInit();
    if (result < 0)
        return -3;

    filexio_ready = 1;
    return 0;
}

int nexus_filexio_ready(void)
{
    return filexio_ready;
}

static int map_filexio_error(int result)
{
    if (result == -ENOENT || result == -ENOTDIR)
        return NEXUS_SCAN_IO_NOT_FOUND;
    return result;
}

static int filexio_open_dir(void *context, const char *path, int *handle)
{
    int fd;
    (void)context;

    if (!filexio_ready || path == 0 || handle == 0)
        return -ENODEV;

    fd = fileXioDopen(path);
    if (fd < 0)
        return map_filexio_error(fd);

    *handle = fd;
    return 0;
}

static int filexio_read_dir(void *context, int handle,
                            NexusScanDirEntry *entry)
{
    iox_dirent_t native_entry __attribute__((aligned(64)));
    size_t name_length;
    int result;
    (void)context;

    if (!filexio_ready || entry == 0)
        return -ENODEV;

    memset(&native_entry, 0, sizeof(native_entry));
    result = fileXioDread(handle, &native_entry);
    if (result <= 0)
        return result == 0 ? 0 : map_filexio_error(result);

    memset(entry, 0, sizeof(*entry));
    if (FIO_S_ISDIR(native_entry.stat.mode)) {
        entry->type = NEXUS_SCAN_ENTRY_DIRECTORY;
    } else if (FIO_S_ISREG(native_entry.stat.mode)) {
        entry->type = NEXUS_SCAN_ENTRY_FILE;
    } else {
        /* Symlinks and device-like entries are never followed. */
        entry->type = 0;
    }

    entry->size_bytes =
        ((uint64_t)native_entry.stat.hisize << 32) |
        (uint64_t)native_entry.stat.size;

    name_length = strnlen(native_entry.name, sizeof(native_entry.name));
    if (name_length >= sizeof(entry->name)) {
        /* Preserve an invalid, unterminated sentinel for scanner rejection. */
        memset(entry->name, 'X', sizeof(entry->name));
    } else {
        memcpy(entry->name, native_entry.name, name_length);
        entry->name[name_length] = '\0';
    }

    return 1;
}

static int filexio_close_dir(void *context, int handle)
{
    (void)context;
    if (!filexio_ready)
        return -ENODEV;
    return fileXioDclose(handle);
}

int nexus_filexio_scan_root(const NexusScanConfig *config,
                            NexusScanReport *report)
{
    static const NexusScanFsOps ops = {
        filexio_open_dir,
        filexio_read_dir,
        filexio_close_dir
    };

    if (!filexio_ready)
        return -ENODEV;

    return nexus_library_scan_root(&ops, 0, config, report);
}
