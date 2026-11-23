/* psprecomp -- firmware entry points added for Dragon Ball Z: Tenkaichi Tag
 * Team (see UPSTREAM.md). Written from the documented firmware behaviour; no
 * emulator code. Grouped by library. Where a feature is not modelled yet
 * (movie and music decoding, ad hoc networking) the call returns what the
 * firmware returns when that hardware has nothing to give, and says so once
 * on stderr, so the game takes its "no data / not connected" path. */
#include "psprecomp/hle.h"
#include "psprecomp/mem.h"
#include "psprecomp/cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

uint64_t psp_display_vblanks(void);
uint32_t psp_threadman_wait(int (*ready)(void *), void *ctx, uint64_t timeout_us, const char *what);
const char *psp_io_root(void);

static void ok(void) { psp_ret(0); }

static void note_once(const char *what) {
    /* A small fixed set of names; repeated calls only count. */
    static const char *seen[64];
    for (int i = 0; i < 64; i++) {
        if (seen[i] == what) return;
        if (!seen[i]) { seen[i] = what; fprintf(stderr, "psprecomp: %s (not modelled yet)\n", what); return; }
    }
}

/* ---- Kernel_Library / UtilsForUser / sceSuspendForUser / SysMem ---- */

static void hle_Memset(void) {
    uint32_t dst = psp_arg(0), n = psp_arg(2);
    uint8_t *p = (uint8_t *)psp_mem_ptr(dst, n);
    if (p) memset(p, (int)(psp_arg(1) & 0xFF), n);
    else for (uint32_t i = 0; i < n; i++) psp_write8(dst + i, (uint8_t)psp_arg(1));
    psp_ret(dst);
}
static void hle_Memcpy(void) {
    uint32_t dst = psp_arg(0), src = psp_arg(1), n = psp_arg(2);
    uint8_t *d = (uint8_t *)psp_mem_ptr(dst, n);
    const uint8_t *s = (const uint8_t *)psp_mem_ptr(src, n);
    if (d && s) memmove(d, s, n);
    else for (uint32_t i = 0; i < n; i++) psp_write8(dst + i, psp_read8(src + i));
    psp_ret(dst);
}
static void hle_CheckThreadStack(void) { psp_ret(0x4000); }   /* bytes left: plenty */
/* The 4 MB "volatile" block the firmware lends games at 0x08400000. */
static void hle_VolatileMemLock(void) {
    if (psp_arg(1)) psp_write32(psp_arg(1), 0x08400000u);
    if (psp_arg(2)) psp_write32(psp_arg(2), 0x00400000u);
    psp_ret(0);
}

/* Model query: nonzero means a later model than the PSP-1000. DBZ TTT warns
 * about the 222 MHz clock when it reads 0. */
static void hle_IsNewModel(void) { psp_ret(1); }

/* ---- sceDisplay / sceGe_user ---- */

static void hle_WaitVblankStart(void) { psp_hle_call(0x36CDFADEu); }  /* sceDisplayWaitVblank */
static void hle_WaitVblankStartMulti(void) {
    uint32_t n = psp_arg(0);
    if (n < 1) n = 1;
    for (uint32_t i = 0; i < n; i++) psp_hle_call(0x36CDFADEu);
    psp_ret(0);
}
static void hle_GetVcount(void) { psp_ret((uint32_t)psp_display_vblanks()); }
static uint32_t g_edram_translation = 0x400;
static void hle_EdramSetAddrTranslation(void) {
    uint32_t prev = g_edram_translation;
    g_edram_translation = psp_arg(0);
    psp_ret(prev);
}

/* ---- IoFileMgrForUser: device controls ---- */

static void hle_Devctl(void) {
    const uint32_t cmd = psp_arg(1), in = psp_arg(2), out = psp_arg(4);
    switch (cmd) {
    case 0x02025806u:            /* memory stick inserted? */
    case 0x02025801u:            /* memory stick state */
        if (out) psp_write32(out, cmd == 0x02025806u ? 1 : 4);
        break;
    case 0x02425818u:            /* memory stick free space: in -> struct pointer */
        if (in) {
            uint32_t s = psp_read32(in);
            if (s) {
                psp_write32(s + 0, 0x8000);     /* max clusters */
                psp_write32(s + 4, 0x8000);     /* free clusters */
                psp_write32(s + 8, 0x8000);     /* max sectors */
                psp_write32(s + 12, 512);       /* sector size */
                psp_write32(s + 16, 64);        /* sectors per cluster */
            }
        }
        break;
    case 0x01F20001u:            /* UMD: disc type -> game disc */
        if (out) { psp_write32(out, 0xFFFFFFFFu); psp_write32(out + 4, 0x10); }
        break;
    default:
        break;
    }
    psp_ret(0);
}

/* ---- sceUmdUser: the disc is always in and readable ---- */

#define UMD_READY 0x32u     /* present | ready | readable */
static void hle_UmdGetDriveStat(void) { psp_ret(UMD_READY); }

/* ---- sceUtility: system settings and dialogs ---- */

static void hle_GetSystemParamInt(void) {
    uint32_t v = 0;
    switch (psp_arg(0)) {
    case 8: v = 1; break;     /* language: English */
    case 9: v = 1; break;     /* confirm button: Cross */
    default: v = 0; break;    /* channel, power save, date/time formats, zone */
    }
    if (psp_arg(1)) psp_write32(psp_arg(1), v);
    psp_ret(0);
}
static void hle_GetSystemParamString(void) {
    const char *nick = "PLAYER";
    uint32_t buf = psp_arg(1), len = psp_arg(2);
    for (uint32_t i = 0; buf && i < len; i++) {
        psp_write8(buf + i, (uint8_t)nick[i]);
        if (!nick[i]) break;
    }
    psp_ret(0);
}

/* Every dialog walks the same states: 0 none, 1 initialising, 2 visible,
 * 3 finished, 4 shutting down. Ours finish at once. */
enum { DLG_MSG, DLG_SAVE, DLG_OSK, DLG_INSTALL, DLG_COUNT };
static uint32_t g_dlg[DLG_COUNT];
static void dlg_init(int d) { g_dlg[d] = 1; }
static void dlg_status(int d) {
    uint32_t s = g_dlg[d];
    if (s == 1) g_dlg[d] = 2;
    else if (s == 2) g_dlg[d] = 3;
    else if (s == 4) g_dlg[d] = 0;
    psp_ret(s);
}
static void dlg_shutdown(int d) { g_dlg[d] = 4; psp_ret(0); }

#define BASE_RESULT 0x1C
static void hle_MsgInit(void) {
    uint32_t p = psp_arg(0);
    psp_write32(p + BASE_RESULT, 0);
    psp_write32(p + 0x23C, 1);                     /* button pressed: yes */
    dlg_init(DLG_MSG);
    psp_ret(0);
}
static void hle_MsgStatus(void)   { dlg_status(DLG_MSG); }
static void hle_MsgShutdown(void) { dlg_shutdown(DLG_MSG); }
static void hle_MsgUpdate(void)   { psp_ret(0); }

/* Save data: files under <root>/ms/PSP/SAVEDATA/<game><save>/<file>, plain. */
#define SD_ERR_LOAD_NO_DATA 0x80110307u
static void sd_string(uint32_t addr, size_t n, char *out) {
    size_t i = 0;
    for (; i + 1 < n; i++) {
        char c = (char)psp_read8(addr + (uint32_t)i);
        if (!c) break;
        out[i] = (c == '/' || c == '\\' || c == ':') ? '_' : c;
    }
    out[i] = 0;
}
static void sd_path(uint32_t p, char *out, size_t cap, int with_file) {
    char game[16], save[24], file[16];
    sd_string(p + 0x3C, sizeof game, game);
    sd_string(p + 0x4C, sizeof save, save);
    if (!strcmp(save, "<>") && psp_read32(p + 0x60)) {   /* list modes: first listed name */
        sd_string(psp_read32(p + 0x60), sizeof save, save);
    }
    sd_string(p + 0x64, sizeof file, file);
    if (with_file) snprintf(out, cap, "%s/ms/PSP/SAVEDATA/%s%s/%s", psp_io_root(), game, save, file);
    else snprintf(out, cap, "%s/ms/PSP/SAVEDATA/%s%s", psp_io_root(), game, save);
}
static void make_dirs(const char *path) {
    char tmp[1024];
    snprintf(tmp, sizeof tmp, "%s", path);
    for (char *c = tmp + 1; *c; c++) {
        if (*c == '/' || *c == '\\') {
            char keep = *c;
            *c = 0;
#ifdef _WIN32
            extern int _mkdir(const char *);
            _mkdir(tmp);
#else
            extern int mkdir(const char *, unsigned);
            mkdir(tmp, 0777);
#endif
            *c = keep;
        }
    }
}
static void hle_SaveInit(void) {
    uint32_t p = psp_arg(0), mode = psp_read32(p + 0x30), result = 0;
    char path[1024];
    sd_path(p, path, sizeof path, 1);
    uint32_t buf = psp_read32(p + 0x74), cap = psp_read32(p + 0x78), size = psp_read32(p + 0x7C);
    switch (mode) {
    case 0: case 2: case 4: {                             /* (auto/list) load */
        FILE *f = fopen(path, "rb");
        if (!f) { result = SD_ERR_LOAD_NO_DATA; break; }
        uint32_t n = 0;
        int c;
        while (n < cap && (c = fgetc(f)) != EOF) psp_write8(buf + n++, (uint8_t)c);
        fclose(f);
        psp_write32(p + 0x7C, n);
        break;
    }
    case 1: case 3: case 5: {                             /* (auto/list) save */
        make_dirs(path);
        FILE *f = fopen(path, "wb");
        if (!f) { result = 0x80110385u; break; }          /* save: no space/error */
        for (uint32_t i = 0; i < size; i++) fputc(psp_read8(buf + i), f);
        fclose(f);
        break;
    }
    case 8: case 22: {                                    /* SIZES / GETSIZE */
        /* msFree, msData, utilityData pointers at +0x5D0/+0x5D4/+0x5D8. */
        uint32_t ms_free = psp_read32(p + 0x5D0), ms_data = psp_read32(p + 0x5D4);
        uint32_t util = psp_read32(p + 0x5D8);
        if (ms_free) {                                    /* 1 GB free in 32 KB clusters */
            psp_write32(ms_free + 0, 0x8000);
            psp_write32(ms_free + 4, 0x8000);
            psp_write32(ms_free + 8, 0x100000);
            const char *kb = "1 GB";
            for (int i = 0; i < 8; i++) psp_write8(ms_free + 12 + (uint32_t)i, (uint8_t)(i < 4 ? kb[i] : 0));
        }
        if (ms_data) {
            /* The save named in msData exists if its folder exists. */
            char game[16], save[24], path[1024];
            sd_string(ms_data, sizeof game, game);
            sd_string(ms_data + 16, sizeof save, save);
            snprintf(path, sizeof path, "%s/ms/PSP/SAVEDATA/%s%s", psp_io_root(), game, save);
            struct stat st;
            for (uint32_t i = 36; i < 36 + 32; i++) psp_write8(ms_data + i, 0);
            if (stat(path, &st) != 0) result = 0x801103C7u;  /* SIZES_NO_DATA */
        }
        if (util) {                                       /* space a new save needs: 1 cluster */
            psp_write32(util + 0, 1);
            psp_write32(util + 4, 32);
            psp_write32(util + 16, 32);
        }
        break;
    }
    default:                                              /* lists etc.: report ok */
        note_once("sceUtilitySavedata mode other than load/save/size");
        break;
    }
    if (getenv("DBZ_LOG_SAVES")) fprintf(stderr, "[save] mode %u %s -> 0x%08X\n", mode, path, result);
    psp_write32(p + BASE_RESULT, result);
    dlg_init(DLG_SAVE);
    psp_ret(0);
}
static void hle_SaveStatus(void)   { dlg_status(DLG_SAVE); }
static void hle_SaveShutdown(void) { dlg_shutdown(DLG_SAVE); }

/* The on-screen keyboard: no typing yet, so it "types" a configured name.
 * Params: +0x30 field count, +0x34 field array. Each field (0x34 bytes):
 * +0x24 out length, +0x28 UTF-16 out text, +0x2C result (2 = changed),
 * +0x30 out limit. */
static void hle_OskInit(void) {
    uint32_t p = psp_arg(0);
    const char *name = getenv("PSP_PLAYER_NAME") ? getenv("PSP_PLAYER_NAME") : "Player";
    uint32_t count = psp_read32(p + 0x30), fields = psp_read32(p + 0x34);
    for (uint32_t i = 0; i < count && i < 8 && fields; i++) {
        uint32_t d = fields + 0x34u * i, out = psp_read32(d + 0x28);
        uint32_t limit = psp_read32(d + 0x30);
        if (!limit) limit = psp_read32(d + 0x24);
        uint32_t n = 0;
        for (; out && name[n] && n + 1 < limit; n++) psp_write16(out + 2 * n, (uint8_t)name[n]);
        if (out && limit) psp_write16(out + 2 * n, 0);
        psp_write32(d + 0x2C, 2);
    }
    psp_write32(p + BASE_RESULT, 0);
    dlg_init(DLG_OSK);
    psp_ret(0);
}
static void hle_OskStatus(void)   { dlg_status(DLG_OSK); }
static void hle_OskShutdown(void) { dlg_shutdown(DLG_OSK); }

/* Optional install to the memory stick: report it as cancelled by the user so
 * the game keeps reading the disc. */
static void hle_InstallInit(void) {
    note_once("sceUtilityGamedataInstall (reported as cancelled)");
    psp_write32(psp_arg(0) + BASE_RESULT, 1);
    dlg_init(DLG_INSTALL);
    psp_ret(0);
}
static void hle_InstallStatus(void)   { dlg_status(DLG_INSTALL); }
static void hle_InstallShutdown(void) { dlg_shutdown(DLG_INSTALL); }

/* ---- sceMpeg: movies are not decoded yet; report "no picture" ---- */

#define MPEG_NO_DATA 0x80618001u
static void hle_MpegQueryMemSize(void) { psp_ret(0x10000); }
static void hle_MpegRingbufferQueryMemSize(void) { psp_ret(psp_arg(0) * 2048u + 0x1000u); }
static void hle_MpegRingbufferConstruct(void) {
    /* (ringbuffer, packets, data, size, callback, cbArg) */
    uint32_t rb = psp_arg(0);
    psp_write32(rb + 0, psp_arg(1));      /* packets */
    psp_write32(rb + 4, 0);               /* packets read */
    psp_write32(rb + 8, 0);               /* packets written */
    psp_write32(rb + 12, 0);              /* packets free */
    psp_write32(rb + 16, 2048);           /* packet size */
    psp_write32(rb + 20, psp_arg(2));     /* data */
    psp_write32(rb + 24, psp_arg(4));     /* callback */
    psp_write32(rb + 28, psp_arg(5));
    psp_ret(0);
}
static void hle_MpegRingbufferAvailableSize(void) { psp_ret(psp_read32(psp_arg(0))); }
static void hle_MpegRingbufferPut(void) { psp_ret(0); }
static void hle_MpegQueryStreamOffset(void) {
    /* PSMF header: big-endian data offset at +8. */
    uint32_t b = psp_arg(1), o = psp_arg(2);
    uint32_t v = (uint32_t)psp_read8(b + 8) << 24 | (uint32_t)psp_read8(b + 9) << 16 |
                 (uint32_t)psp_read8(b + 10) << 8 | psp_read8(b + 11);
    if (o) psp_write32(o, v);
    psp_ret(0);
}
static void hle_MpegQueryStreamSize(void) {
    uint32_t b = psp_arg(0), o = psp_arg(1);
    uint32_t v = (uint32_t)psp_read8(b + 12) << 24 | (uint32_t)psp_read8(b + 13) << 16 |
                 (uint32_t)psp_read8(b + 14) << 8 | psp_read8(b + 15);
    if (o) psp_write32(o, v);
    psp_ret(0);
}
static uint32_t g_stream_id;
static void hle_MpegRegistStream(void) { psp_ret(++g_stream_id); }
static void hle_MpegMallocAvcEsBuf(void) { psp_ret(1); }
static void hle_MpegNoData(void) { note_once("sceMpeg (movies are skipped)"); psp_ret(MPEG_NO_DATA); }
static void hle_MpegAvcDecode(void) {
    note_once("sceMpeg (movies are skipped)");
    if (psp_arg(4)) psp_write32(psp_arg(4), 0);     /* no picture this call */
    psp_ret(MPEG_NO_DATA);
}

/* ---- sceAtrac3plus: low-level decoding gives silence for now ---- */

static uint32_t g_atrac_frame[8];
static int g_atrac_used[8];
static void hle_AtracGetID(void) {
    for (int i = 0; i < 8; i++) if (!g_atrac_used[i]) { g_atrac_used[i] = 1; psp_ret((uint32_t)i); return; }
    psp_ret(0x80630006u);   /* no free ID */
}
static void hle_AtracRelease(void) {
    uint32_t id = psp_arg(0);
    if (id < 8) g_atrac_used[id] = 0;
    psp_ret(0);
}
static void hle_AtracLowLevelInit(void) {
    uint32_t id = psp_arg(0), params = psp_arg(1);
    if (id < 8 && params) g_atrac_frame[id] = psp_read32(params + 8);
    psp_ret(0);
}
static void hle_AtracLowLevelDecode(void) {
    /* (id, source, consumedAddr, samples, sampleBytesAddr) */
    uint32_t id = psp_arg(0), out = psp_arg(3);
    note_once("sceAtrac (music decodes as silence)");
    const uint32_t bytes = 2048u * 2u * 2u;
    uint8_t *p = (uint8_t *)psp_mem_ptr(out, bytes);
    if (p) memset(p, 0, bytes);
    if (psp_arg(2)) psp_write32(psp_arg(2), id < 8 ? g_atrac_frame[id] : 0);
    if (psp_arg(4)) psp_write32(psp_arg(4), bytes);
    psp_ret(0);
}

/* ---- networking: no ad hoc yet; the WLAN switch reads as off ---- */

#define NET_ERR 0x80410B80u
static void hle_NetFail(void) { note_once("ad hoc networking"); psp_ret(NET_ERR); }
static void hle_AdhocctlGetState(void) { if (psp_arg(0)) psp_write32(psp_arg(0), 0); psp_ret(0); }
static void hle_AdhocctlGetPeerList(void) { if (psp_arg(0)) psp_write32(psp_arg(0), 0); psp_ret(0); }
static uint32_t g_handler_id;
static void hle_AdhocctlAddHandler(void) { psp_ret(g_handler_id++); }
static void hle_WlanGetEtherAddr(void) {
    static const uint8_t mac[6] = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x01 };
    for (int i = 0; i < 6 && psp_arg(0); i++) psp_write8(psp_arg(0) + (uint32_t)i, mac[i]);
    psp_ret(0);
}

/* ---- module manager ---- */

static void hle_StopUnloadSelf(void) {
    fprintf(stderr, "psprecomp: the game asked to exit (status 0x%08X)\n", psp_arg(1));
    fflush(stdout);
    exit(0);
}

void psp_extra_register(void) {
    psp_hle_register(0xA089ECA4, "Kernel_Library", "sceKernelMemset", hle_Memset);
    psp_hle_register(0x1839852A, "Kernel_Library", "sceKernelMemcpy", hle_Memcpy);
    psp_hle_register(0xD13BDE95, "Kernel_Library", "sceKernelCheckThreadStack", hle_CheckThreadStack);
    psp_hle_register(0xC2DF770E, "UtilsForUser", "sceKernelIcacheInvalidateRange", ok);
    psp_hle_register(0x3EE30821, "UtilsForUser", "sceKernelDcacheWritebackRange", ok);
    psp_hle_register(0x3E0271D3, "sceSuspendForUser", "sceKernelVolatileMemLock", hle_VolatileMemLock);
    psp_hle_register(0xA569E425, "sceSuspendForUser", "sceKernelVolatileMemUnlock", ok);
    psp_hle_register_unnamed(0x1B4217BC, "SysMemUserForUser", ok);     /* SDK version setter */
    psp_hle_register_unnamed(0x8F2DF740, "ModuleMgrForUser", hle_StopUnloadSelf);

    psp_hle_register(0x984C27E7, "sceDisplay", "sceDisplayWaitVblankStart", hle_WaitVblankStart);
    psp_hle_register(0x77ED8B3A, "sceDisplay", "sceDisplayWaitVblankStartMultiCB", hle_WaitVblankStartMulti);
    psp_hle_register(0x9C6EAAD7, "sceDisplay", "sceDisplayGetVcount", hle_GetVcount);
    psp_hle_register(0xB77905EA, "sceGe_user", "sceGeEdramSetAddrTranslation", hle_EdramSetAddrTranslation);

    psp_hle_register(0x54F5FB11, "IoFileMgrForUser", "sceIoDevctl", hle_Devctl);
    psp_hle_register(0x63632449, "IoFileMgrForUser", "sceIoIoctl", ok);

    psp_hle_register(0xC6183D47, "sceUmdUser", "sceUmdActivate", ok);
    psp_hle_register(0x6B4A146C, "sceUmdUser", "sceUmdGetDriveStat", hle_UmdGetDriveStat);
    psp_hle_register(0x4A9E5E29, "sceUmdUser", "sceUmdWaitDriveStatCB", ok);
    psp_hle_register(0xAEE7404D, "sceUmdUser", "sceUmdRegisterUMDCallBack", ok);
    psp_hle_register(0xBD2BDE07, "sceUmdUser", "sceUmdUnRegisterUMDCallBack", ok);

    psp_hle_register(0x36AA6E91, "sceImpose", "sceImposeSetLanguageMode", ok);
    psp_hle_register(0x72189C48, "sceImpose", "sceImposeSetUMDPopup", ok);
    psp_hle_register_unnamed(0x469989AD, "scePower", ok);     /* clock setting */
    psp_hle_register_unnamed(0xA85880D0, "scePower", hle_IsNewModel);  /* 1: not the original PSP-1000 */

    psp_hle_register(0x2A2B3DE0, "sceUtility", "sceUtilityLoadModule", ok);
    psp_hle_register(0xE49BFE92, "sceUtility", "sceUtilityUnloadModule", ok);
    psp_hle_register(0xA5DA2406, "sceUtility", "sceUtilityGetSystemParamInt", hle_GetSystemParamInt);
    psp_hle_register(0x34B78343, "sceUtility", "sceUtilityGetSystemParamString", hle_GetSystemParamString);
    psp_hle_register(0x2AD8E239, "sceUtility", "sceUtilityMsgDialogInitStart", hle_MsgInit);
    psp_hle_register(0x9A1C91D7, "sceUtility", "sceUtilityMsgDialogGetStatus", hle_MsgStatus);
    psp_hle_register(0x67AF3428, "sceUtility", "sceUtilityMsgDialogShutdownStart", hle_MsgShutdown);
    psp_hle_register(0x95FC253B, "sceUtility", "sceUtilityMsgDialogUpdate", hle_MsgUpdate);
    psp_hle_register(0x50C4CD57, "sceUtility", "sceUtilitySavedataInitStart", hle_SaveInit);
    psp_hle_register(0x8874DBE0, "sceUtility", "sceUtilitySavedataGetStatus", hle_SaveStatus);
    psp_hle_register(0x9790B33C, "sceUtility", "sceUtilitySavedataShutdownStart", hle_SaveShutdown);
    psp_hle_register(0xD4B95FFB, "sceUtility", "sceUtilitySavedataUpdate", ok);
    psp_hle_register(0xF6269B82, "sceUtility", "sceUtilityOskInitStart", hle_OskInit);
    psp_hle_register(0xF3F76017, "sceUtility", "sceUtilityOskGetStatus", hle_OskStatus);
    psp_hle_register(0x3DFAEBA9, "sceUtility", "sceUtilityOskShutdownStart", hle_OskShutdown);
    psp_hle_register(0x4B85C861, "sceUtility", "sceUtilityOskUpdate", ok);
    psp_hle_register(0x24AC31EB, "sceUtility", "sceUtilityGamedataInstallInitStart", hle_InstallInit);
    psp_hle_register(0xB57E95D9, "sceUtility", "sceUtilityGamedataInstallGetStatus", hle_InstallStatus);
    psp_hle_register(0x32E32DCB, "sceUtility", "sceUtilityGamedataInstallShutdownStart", hle_InstallShutdown);
    psp_hle_register(0x4AECD179, "sceUtility", "sceUtilityGamedataInstallUpdate", ok);
    psp_hle_register(0x180F7B62, "sceUtility", "sceUtilityGamedataInstallAbort", ok);

    psp_hle_register(0x682A619B, "sceMpeg", "sceMpegInit", ok);
    psp_hle_register(0x874624D6, "sceMpeg", "sceMpegFinish", ok);
    psp_hle_register(0xC132E22F, "sceMpeg", "sceMpegQueryMemSize", hle_MpegQueryMemSize);
    psp_hle_register(0xD8C5F121, "sceMpeg", "sceMpegCreate", ok);
    psp_hle_register(0x606A4649, "sceMpeg", "sceMpegDelete", ok);
    psp_hle_register(0xD7A29F46, "sceMpeg", "sceMpegRingbufferQueryMemSize", hle_MpegRingbufferQueryMemSize);
    psp_hle_register(0x37295ED8, "sceMpeg", "sceMpegRingbufferConstruct", hle_MpegRingbufferConstruct);
    psp_hle_register(0x13407F13, "sceMpeg", "sceMpegRingbufferDestruct", ok);
    psp_hle_register(0xB240A59E, "sceMpeg", "sceMpegRingbufferPut", hle_MpegRingbufferPut);
    psp_hle_register(0xB5F6DC87, "sceMpeg", "sceMpegRingbufferAvailableSize", hle_MpegRingbufferAvailableSize);
    psp_hle_register(0x21FF80E4, "sceMpeg", "sceMpegQueryStreamOffset", hle_MpegQueryStreamOffset);
    psp_hle_register(0x611E9E11, "sceMpeg", "sceMpegQueryStreamSize", hle_MpegQueryStreamSize);
    psp_hle_register(0x42560F23, "sceMpeg", "sceMpegRegistStream", hle_MpegRegistStream);
    psp_hle_register(0x591A4AA2, "sceMpeg", "sceMpegUnRegistStream", ok);
    psp_hle_register(0xA780CF7E, "sceMpeg", "sceMpegMallocAvcEsBuf", hle_MpegMallocAvcEsBuf);
    psp_hle_register(0xCEB870B1, "sceMpeg", "sceMpegFreeAvcEsBuf", ok);
    psp_hle_register(0x167AFD9E, "sceMpeg", "sceMpegInitAu", ok);
    psp_hle_register(0xF8DCB679, "sceMpeg", "sceMpegQueryAtracEsSize", ok);
    psp_hle_register(0xFE246728, "sceMpeg", "sceMpegGetAvcAu", hle_MpegNoData);
    psp_hle_register(0xE1CE83A7, "sceMpeg", "sceMpegGetAtracAu", hle_MpegNoData);
    psp_hle_register(0x0E3C2E9D, "sceMpeg", "sceMpegAvcDecode", hle_MpegAvcDecode);
    psp_hle_register(0x740FCCD1, "sceMpeg", "sceMpegAvcDecodeStop", ok);
    psp_hle_register(0x800C44DF, "sceMpeg", "sceMpegAtracDecode", hle_MpegNoData);

    psp_hle_register(0x780F88D1, "sceAtrac3plus", "sceAtracGetAtracID", hle_AtracGetID);
    psp_hle_register(0x61EB33F5, "sceAtrac3plus", "sceAtracReleaseAtracID", hle_AtracRelease);
    psp_hle_register(0x132F1ECA, "sceAtrac3plus", "sceAtracReinit", ok);
    psp_hle_register(0x1575D64B, "sceAtrac3plus", "sceAtracLowLevelInitDecoder", hle_AtracLowLevelInit);
    psp_hle_register(0x0C116E1B, "sceAtrac3plus", "sceAtracLowLevelDecode", hle_AtracLowLevelDecode);

    psp_hle_register(0x39AF39A6, "sceNet", "sceNetInit", ok);
    psp_hle_register(0x281928A9, "sceNet", "sceNetTerm", ok);
    psp_hle_register(0xE1D621D7, "sceNetAdhoc", "sceNetAdhocInit", ok);
    psp_hle_register(0xA62C6F57, "sceNetAdhoc", "sceNetAdhocTerm", ok);
    psp_hle_register(0x6F92741B, "sceNetAdhoc", "sceNetAdhocPdpCreate", hle_NetFail);
    psp_hle_register(0x7F27BB5E, "sceNetAdhoc", "sceNetAdhocPdpDelete", ok);
    psp_hle_register(0xABED3790, "sceNetAdhoc", "sceNetAdhocPdpSend", hle_NetFail);
    psp_hle_register(0xDFE53E03, "sceNetAdhoc", "sceNetAdhocPdpRecv", hle_NetFail);
    psp_hle_register(0xE26F226E, "sceNetAdhocctl", "sceNetAdhocctlInit", ok);
    psp_hle_register(0x9D689E13, "sceNetAdhocctl", "sceNetAdhocctlTerm", ok);
    psp_hle_register(0x20B317A0, "sceNetAdhocctl", "sceNetAdhocctlAddHandler", hle_AdhocctlAddHandler);
    psp_hle_register(0x6402490B, "sceNetAdhocctl", "sceNetAdhocctlDelHandler", ok);
    psp_hle_register(0x75ECD386, "sceNetAdhocctl", "sceNetAdhocctlGetState", hle_AdhocctlGetState);
    psp_hle_register(0xE162CB14, "sceNetAdhocctl", "sceNetAdhocctlGetPeerList", hle_AdhocctlGetPeerList);
    psp_hle_register(0x08FFF7A0, "sceNetAdhocctl", "sceNetAdhocctlScan", hle_NetFail);
    psp_hle_register(0x0AD043ED, "sceNetAdhocctl", "sceNetAdhocctlConnect", hle_NetFail);
    psp_hle_register(0x5E7F79C9, "sceNetAdhocctl", "sceNetAdhocctlJoin", hle_NetFail);
    psp_hle_register(0x34401D65, "sceNetAdhocctl", "sceNetAdhocctlDisconnect", ok);
    psp_hle_register(0x81AEE1BE, "sceNetAdhocctl", "sceNetAdhocctlGetScanInfo", hle_NetFail);
    psp_hle_register(0x8DB83FDC, "sceNetAdhocctl", "sceNetAdhocctlGetPeerInfo", hle_NetFail);
    psp_hle_register(0x2A2A1E07, "sceNetAdhocMatching", "sceNetAdhocMatchingInit", ok);
    psp_hle_register(0x7945ECDA, "sceNetAdhocMatching", "sceNetAdhocMatchingTerm", ok);
    psp_hle_register(0xCA5EDA6F, "sceNetAdhocMatching", "sceNetAdhocMatchingCreate", hle_NetFail);
    psp_hle_register(0xF16EAF4F, "sceNetAdhocMatching", "sceNetAdhocMatchingDelete", ok);
    psp_hle_register(0x93EF3843, "sceNetAdhocMatching", "sceNetAdhocMatchingStart", hle_NetFail);
    psp_hle_register(0x32B156B3, "sceNetAdhocMatching", "sceNetAdhocMatchingStop", ok);
    psp_hle_register(0x5E3D4B79, "sceNetAdhocMatching", "sceNetAdhocMatchingSelectTarget", hle_NetFail);
    psp_hle_register(0x8F58BEDF, "sceNetAdhocMatching", "sceNetAdhocMatchingCancelTargetWithOpt", ok);
    psp_hle_register(0xB58E61B7, "sceNetAdhocMatching", "sceNetAdhocMatchingSetHelloOpt", ok);
    psp_hle_register(0xF79472D7, "sceNetAdhocMatching", "sceNetAdhocMatchingSendData", hle_NetFail);
    psp_hle_register(0xEC19337D, "sceNetAdhocMatching", "sceNetAdhocMatchingAbortSendData", ok);
    psp_hle_register(0xD7763699, "sceWlanDrv", "sceWlanGetSwitchState", ok);   /* 0 = off */
    psp_hle_register(0x0C622081, "sceWlanDrv", "sceWlanGetEtherAddr", hle_WlanGetEtherAddr);
}
