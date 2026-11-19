#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <tamtypes.h>
#include <fileXio_rpc.h>

#include "nexus_filexio.h"

unsigned char iomanX_irx[16] __attribute__((aligned(16)));
unsigned int size_iomanX_irx = sizeof(iomanX_irx);
unsigned char fileXio_irx[16] __attribute__((aligned(16)));
unsigned int size_fileXio_irx = sizeof(fileXio_irx);

static int iomanx_loaded;
static int filexio_loaded;
static int sif_init_calls;
static int exec_calls;
static int patch_lmb_calls;
static int patch_prefix_calls;
static int filexio_init_calls;
static int dopen_calls;
static int dclose_calls;
static int cd_entry_position;

void sceSifInitRpc(int mode)
{
    (void)mode;
    ++sif_init_calls;
}

int SifSearchModuleByName(const char *name)
{
    if (strcmp(name, "IOX/File_Manager") == 0)
        return iomanx_loaded ? 1 : -1;
    if (strcmp(name, "IOX/File_Manager_Rpc") == 0)
        return filexio_loaded ? 2 : -1;
    return -1;
}

int SifExecModuleBuffer(void *image, u32 size, u32 arg_len,
                        const char *args, int *module_result)
{
    (void)size;
    (void)arg_len;
    (void)args;
    if (image == iomanX_irx)
        iomanx_loaded = 1;
    else if (image == fileXio_irx)
        filexio_loaded = 1;
    else
        return -1;
    ++exec_calls;
    if (module_result != 0)
        *module_result = 0;
    return 1;
}

void sbv_patch_enable_lmb(void) { ++patch_lmb_calls; }
void sbv_patch_disable_prefix_check(void) { ++patch_prefix_calls; }

int fileXioInit(void)
{
    ++filexio_init_calls;
    return 0;
}

int fileXioDopen(const char *path)
{
    ++dopen_calls;
    if (strcmp(path, "mass:/") == 0)
        return 1;
    if (strcmp(path, "mass:/CD") == 0)
        return 2;
    if (strcmp(path, "mass:/DVD") == 0)
        return -ENOENT;
    return -ENOENT;
}

int fileXioDread(int fd, iox_dirent_t *entry)
{
    if (fd == 1)
        return 0;
    if (fd != 2)
        return -EBADF;

    if (cd_entry_position == 0) {
        memset(entry, 0, sizeof(*entry));
        entry->stat.mode = FIO_S_IFREG;
        entry->stat.size = 1234567U;
        strcpy(entry->name, "NEXUS_TEST.iso");
        ++cd_entry_position;
        return 1;
    }

    if (cd_entry_position == 1) {
        memset(entry, 0, sizeof(*entry));
        entry->stat.mode = FIO_S_IFDIR;
        strcpy(entry->name, "SUBFOLDER");
        ++cd_entry_position;
        return 1;
    }

    return 0;
}

int fileXioDclose(int fd)
{
    assert(fd == 1 || fd == 2);
    ++dclose_calls;
    return 0;
}

int main(void)
{
    NexusScanConfig config;
    NexusScanReport report;
    const NexusLibraryEntry *entry;

    memset(&config, 0, sizeof(config));
    config.source = NEXUS_LIBRARY_SOURCE_USB;
    config.scan_ps2 = 1;
    strcpy(config.root, "mass:/");

    nexus_library_clear();
    assert(nexus_filexio_ready() == 0);
    assert(nexus_filexio_scan_root(&config, &report) == -ENODEV);
    assert(dopen_calls == 0);

    /* The IOP-facing services start only after this explicit activation. */
    assert(nexus_filexio_activate() == 0);
    assert(nexus_filexio_ready() == 1);
    assert(sif_init_calls == 1);
    assert(exec_calls == 2);
    assert(patch_lmb_calls == 1);
    assert(patch_prefix_calls == 1);
    assert(filexio_init_calls == 1);

    /* Activation is idempotent; it does not patch or reload a second time. */
    assert(nexus_filexio_activate() == 0);
    assert(exec_calls == 2);
    assert(patch_lmb_calls == 1);
    assert(patch_prefix_calls == 1);
    assert(filexio_init_calls == 1);

    assert(nexus_filexio_scan_root(&config, &report) == 0);
    assert(report.root_available == 1U);
    assert(report.directories_opened == 1U);
    assert(report.missing_directories == 1U);
    assert(report.indexed == 1U);
    assert(report.entries_seen == 2U);
    assert(nexus_library_count() == 1U);
    assert(dopen_calls == 3);
    assert(dclose_calls == 2);

    entry = nexus_library_find(
        nexus_library_visible_at(&(NexusLibraryView){
            0, NEXUS_LIBRARY_FILTER_ANY, NEXUS_LIBRARY_FILTER_ANY,
            0, NEXUS_LIBRARY_VIEW_GRID}, 0)->key,
        NEXUS_LIBRARY_SOURCE_USB);
    assert(entry != 0);
    assert(strcmp(entry->title, "NEXUS TEST") == 0);
    assert(strcmp(entry->path, "mass:/CD/NEXUS_TEST.iso") == 0);
    assert(entry->size_bytes == 1234567U);

    strcpy(config.root, "mass:/../SYS-CONF");
    assert(nexus_filexio_scan_root(&config, &report) < 0);
    assert(nexus_library_count() == 1U);
    return 0;
}
