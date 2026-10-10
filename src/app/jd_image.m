/* jd_image.m — bring your own picture (3.4).
 *
 * J (2026-08-16): "say I wanted to have a picture of Amanda floated into the
 * screen saver."  And 2026-10-06: "import pictures to use in the kaleidoscope —
 * they would have to be modified to fit the vibe."
 *
 * The picture is never shown as a photo.  It is decoded ONCE into a 512x512
 * luminance plane plus a Sobel edge plane, and pattern 611 folds those through
 * the kaleidoscope and colours them from the live palette — so the photo's
 * SHAPE is on screen, wearing whatever the engine is wearing (the "DAZZLED"
 * treatment from docs/IDEAS_ARRIVALS.md).  No RGB is kept; nothing is written
 * anywhere except the user's own textures folder.
 *
 * Sources, first found wins:
 *   1. $JD_IMAGE                       (a path; handy for testing)
 *   2. ~/Library/Application Support/JellyDazzle/textures/ — png, jpg, jpeg, heic
 *      — the sandbox container's own folder, so it works in the App Store
 *      build with no extra entitlement; File > Open Picture… copies into it.
 *      Several files: a random one per launch, then (3.5) the engine asks for the
 *      next one each time it brings a picture back — a different picture every
 *      appearance, decoded ahead of time off the main thread so the swap is a
 *      pointer exchange, never a dropped frame.
 *
 * Decoding is ImageIO + CoreGraphics (already linked via Cocoa), so there is
 * no new dependency and no third-party decoder in the tree. */
#import <Foundation/Foundation.h>
#import <ImageIO/ImageIO.h>
#import <CoreGraphics/CoreGraphics.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define JD_IMG_N 512

const uint8_t *jd_img_luma = NULL;    /* JD_IMG_N*JD_IMG_N, contrast-stretched */
const uint8_t *jd_img_edge = NULL;    /* same size, Sobel magnitude           */
int            jd_img_ready = 0;
char           jd_img_name[96] = "";

static NSString *textures_dir(void)
{
    NSArray *dirs = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
    if ([dirs count] == 0) return nil;
    NSString *d = [[dirs objectAtIndex:0] stringByAppendingPathComponent:@"JellyDazzle/textures"];
    [[NSFileManager defaultManager] createDirectoryAtPath:d withIntermediateDirectories:YES attributes:nil error:NULL];
    return d;
}

/* aspect-FILL the picture into a square gray plane (centre crop), high quality */
static int decode_gray(const char *path, uint8_t *gray)
{
    CFURLRef url = CFURLCreateFromFileSystemRepresentation(NULL, (const UInt8 *)path, (CFIndex)strlen(path), false);
    if (!url) return 0;
    CGImageSourceRef src = CGImageSourceCreateWithURL(url, NULL);
    CFRelease(url);
    if (!src) return 0;
    CGImageRef img = CGImageSourceCreateImageAtIndex(src, 0, NULL);
    CFRelease(src);
    if (!img) return 0;
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceGray();
    CGContextRef ctx = CGBitmapContextCreate(gray, JD_IMG_N, JD_IMG_N, 8, JD_IMG_N, cs, (CGBitmapInfo)kCGImageAlphaNone);
    int ok = 0;
    if (ctx) {
        size_t iw = CGImageGetWidth(img), ih = CGImageGetHeight(img);
        double s = fmax((double)JD_IMG_N / (double)iw, (double)JD_IMG_N / (double)ih);
        double dw = iw * s, dh = ih * s;
        CGContextSetInterpolationQuality(ctx, kCGInterpolationHigh);
        CGContextSetGrayFillColor(ctx, 0.0, 1.0);
        CGContextFillRect(ctx, CGRectMake(0, 0, JD_IMG_N, JD_IMG_N));
        CGContextDrawImage(ctx, CGRectMake((JD_IMG_N - dw) * 0.5, (JD_IMG_N - dh) * 0.5, dw, dh), img);
        CGContextRelease(ctx);
        ok = 1;
    }
    CGColorSpaceRelease(cs);
    CGImageRelease(img);
    return ok;
}

/* Fit the vibe: stretch contrast to the 2nd..98th percentile so a flat phone
 * photo spans the whole palette, soften JPEG grit with one 3x3 box pass, then
 * take Sobel edges so line structure survives the fold. */
static void treat(uint8_t *g, uint8_t *e)
{
    const int N = JD_IMG_N, M = N * N;
    unsigned hist[256] = {0};
    for (int i = 0; i < M; i++) hist[g[i]]++;
    int lo = 0, hi = 255; unsigned acc = 0;
    for (int i = 0; i < 256; i++) { acc += hist[i]; if (acc >= (unsigned)M * 2 / 100) { lo = i; break; } }
    acc = 0;
    for (int i = 255; i >= 0; i--) { acc += hist[i]; if (acc >= (unsigned)M * 2 / 100) { hi = i; break; } }
    if (hi <= lo + 8) { lo = 0; hi = 255; }
    uint8_t *tmp = (uint8_t *)malloc((size_t)M);
    for (int i = 0; i < M; i++) {
        int v = ((int)g[i] - lo) * 255 / (hi - lo);
        tmp[i] = (uint8_t)(v < 0 ? 0 : v > 255 ? 255 : v);
    }
    for (int y = 1; y < N - 1; y++)
        for (int x = 1; x < N - 1; x++) {
            int s = 0;
            for (int dy = -1; dy <= 1; dy++) for (int dx = -1; dx <= 1; dx++) s += tmp[(y + dy) * N + x + dx];
            g[y * N + x] = (uint8_t)(s / 9);
        }
    for (int y = 1; y < N - 1; y++)
        for (int x = 1; x < N - 1; x++) {
            int p = y * N + x;
            int gx = -g[p - N - 1] - 2 * g[p - 1] - g[p + N - 1] + g[p - N + 1] + 2 * g[p + 1] + g[p + N + 1];
            int gy = -g[p - N - 1] - 2 * g[p - N] - g[p - N + 1] + g[p + N - 1] + 2 * g[p + N] + g[p + N + 1];
            int m = (abs(gx) + abs(gy)) / 3;
            e[p] = (uint8_t)(m > 255 ? 255 : m);
        }
    free(tmp);
}

static int load_path(const char *path)
{
    uint8_t *g = (uint8_t *)calloc((size_t)JD_IMG_N * JD_IMG_N, 1);
    uint8_t *e = (uint8_t *)calloc((size_t)JD_IMG_N * JD_IMG_N, 1);
    if (!g || !e || !decode_gray(path, g)) { free(g); free(e); return 0; }
    treat(g, e);
    /* swap in: the pattern reads these pointers once per frame on the main
     * thread, and this runs on the main thread too (between frames) */
    const uint8_t *og = jd_img_luma, *oe = jd_img_edge;
    jd_img_luma = g; jd_img_edge = e; jd_img_ready = 1;
    free((void *)og); free((void *)oe);
    const char *base = strrchr(path, '/'); base = base ? base + 1 : path;
    strncpy(jd_img_name, base, sizeof jd_img_name - 1); jd_img_name[sizeof jd_img_name - 1] = 0;
    return 1;
}

static NSArray *picture_files(NSString *dir)
{
    NSArray *all = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:dir error:NULL];
    NSMutableArray *out = [NSMutableArray array];
    for (NSString *f in all) {
        NSString *x = [[f pathExtension] lowercaseString];
        if ([x isEqualToString:@"png"] || [x isEqualToString:@"jpg"] || [x isEqualToString:@"jpeg"] || [x isEqualToString:@"heic"])
            [out addObject:[dir stringByAppendingPathComponent:f]];
    }
    return out;
}

/* ---- 3.5: a rotation of pictures --------------------------------------------
 * The folder is re-read at every turn, so pictures added in Finder (File > Show
 * Pictures Folder) join the rotation without a relaunch.  Order is a shuffled
 * pass through the folder; a picture never follows itself. */
static NSMutableArray *g_order = nil;          /* paths, this pass's order   */
static NSUInteger      g_opos  = 0;
static uint8_t *g_pend_g = NULL, *g_pend_e = NULL;   /* decoded next picture */
static char     g_pend_name[96] = "";
static int      g_pend_busy = 0;

static void reshuffle(NSArray *files)
{
    g_order = [NSMutableArray arrayWithArray:files];
    for (NSUInteger i = [g_order count]; i > 1; i--)
        [g_order exchangeObjectAtIndex:i - 1 withObjectAtIndex:(NSUInteger)arc4random_uniform((uint32_t)i)];
    g_opos = 0;
}

static NSString *next_path(void)
{
    NSString *dir = textures_dir();
    if (!dir) return nil;
    NSArray *files = picture_files(dir);
    if ([files count] < 2) return nil;                 /* one picture: nothing to rotate */
    if (!g_order || g_opos >= [g_order count] || [g_order count] != [files count]) reshuffle(files);
    for (NSUInteger tries = 0; tries < [g_order count] + 1; tries++) {
        if (g_opos >= [g_order count]) reshuffle(files);
        NSString *p = [g_order objectAtIndex:g_opos++];
        if (![[p lastPathComponent] isEqualToString:[NSString stringWithUTF8String:jd_img_name]]) return p;
    }
    return nil;
}

/* decode the next picture on a background queue; publish it on the main queue */
static void preload_next(void)
{
    if (g_pend_busy || g_pend_g) return;
    NSString *p = next_path();
    if (!p) return;
    g_pend_busy = 1;
    char *path = strdup([p UTF8String]);
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY, 0), ^{
        uint8_t *g = (uint8_t *)calloc((size_t)JD_IMG_N * JD_IMG_N, 1);
        uint8_t *e = (uint8_t *)calloc((size_t)JD_IMG_N * JD_IMG_N, 1);
        int ok = g && e && decode_gray(path, g);
        if (ok) treat(g, e);
        dispatch_async(dispatch_get_main_queue(), ^{
            if (ok) {
                free(g_pend_g); free(g_pend_e); g_pend_g = g; g_pend_e = e;
                const char *base = strrchr(path, '/'); base = base ? base + 1 : path;
                strncpy(g_pend_name, base, sizeof g_pend_name - 1); g_pend_name[sizeof g_pend_name - 1] = 0;
            } else { free(g); free(e); }
            g_pend_busy = 0; free(path);
        });
    });
}

/* The engine calls this just before it brings a picture back.  If the next one
 * is decoded, swap it in (pointer exchange, main thread, between frames). */
void jd_image_next(void)
{ @autoreleasepool {
    if (g_pend_g) {
        const uint8_t *og = jd_img_luma, *oe = jd_img_edge;
        jd_img_luma = g_pend_g; jd_img_edge = g_pend_e; jd_img_ready = 1;
        g_pend_g = g_pend_e = NULL;
        memcpy(jd_img_name, g_pend_name, sizeof jd_img_name);
        free((void *)og); free((void *)oe);
    }
    preload_next();
}}

void jd_image_load(void)
{ @autoreleasepool {
    const char *env = getenv("JD_IMAGE");
    if (env && *env && load_path(env)) return;
    NSString *dir = textures_dir();
    if (!dir) return;
    NSArray *files = picture_files(dir);
    if ([files count] == 0) return;
    unsigned idx = (unsigned)(time(NULL) ^ (unsigned)getpid()) % (unsigned)[files count];
    load_path([[files objectAtIndex:idx] UTF8String]);
    preload_next();                                   /* 3.5: have the next one ready */
}}

/* File > Show Pictures Folder (3.5) */
const char *jd_image_dir(void)
{
    static char buf[1024];
    NSString *d = textures_dir();
    if (!d) return NULL;
    strncpy(buf, [d UTF8String], sizeof buf - 1); buf[sizeof buf - 1] = 0;
    return buf;
}

/* File > Open Picture…: copy into the textures folder (so it survives the
 * sandbox and the next launch), then show it now. */
int jd_image_import(const char *path)
{ @autoreleasepool {
    NSString *dir = textures_dir();
    if (!dir) return 0;
    NSString *src = [NSString stringWithUTF8String:path];
    NSString *dst = [dir stringByAppendingPathComponent:[src lastPathComponent]];
    NSFileManager *fm = [NSFileManager defaultManager];
    if (![src isEqualToString:dst]) {
        [fm removeItemAtPath:dst error:NULL];
        if (![fm copyItemAtPath:src toPath:dst error:NULL]) dst = src;   /* still show it */
    }
    int ok = load_path([dst UTF8String]);
    g_order = nil; free(g_pend_g); free(g_pend_e); g_pend_g = g_pend_e = NULL;   /* 3.5: new set, new pass */
    preload_next();
    return ok;
}}

/* 3.5: copy without loading — for multi-file / folder imports; returns files copied */
int jd_image_add(const char *path)
{ @autoreleasepool {
    NSString *dir = textures_dir();
    if (!dir) return 0;
    NSString *src = [NSString stringWithUTF8String:path];
    NSFileManager *fm = [NSFileManager defaultManager];
    BOOL isDir = NO;
    if (![fm fileExistsAtPath:src isDirectory:&isDir]) return 0;
    NSArray *list = isDir ? picture_files(src) : @[src];
    int n = 0;
    for (NSString *f in list) {
        NSString *x = [[f pathExtension] lowercaseString];
        if (!([x isEqualToString:@"png"] || [x isEqualToString:@"jpg"] || [x isEqualToString:@"jpeg"] || [x isEqualToString:@"heic"])) continue;
        NSString *dst = [dir stringByAppendingPathComponent:[f lastPathComponent]];
        if ([f isEqualToString:dst]) { n++; continue; }
        [fm removeItemAtPath:dst error:NULL];
        if ([fm copyItemAtPath:f toPath:dst error:NULL]) n++;
    }
    return n;
}}
