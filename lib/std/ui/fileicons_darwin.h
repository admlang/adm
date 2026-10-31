// NSWorkspace's icons for std.ui's systemFileIcon on macOS: a kind of file's icon by its
// content type, so the file need not exist, drawn into an RGBA square. Written in C on the
// Objective-C runtime, as the native-support unit that std/ui/fileicons_darwin.adm binds is
// C; its compiled code calls the function it names: that has external linkage. Empty
// elsewhere.
#ifndef ADM_FILEICONS_DARWIN_H
#define ADM_FILEICONS_DARWIN_H

#ifdef __APPLE__

#include <CoreGraphics/CoreGraphics.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <stdint.h>
#include <string.h>

extern void* objc_autoreleasePoolPush(void);
extern void objc_autoreleasePoolPop(void* pool);

static id adm_file_icon_send(id target, const char* selector) {
    return ((id (*)(id, SEL))objc_msgSend)(target, sel_registerName(selector));
}

static id adm_file_icon_send1(id target, const char* selector, id argument) {
    return ((id (*)(id, SEL, id))objc_msgSend)(target, sel_registerName(selector), argument);
}

static id adm_file_icon_string(const char* text) {
    return ((id (*)(id, SEL, const char*))objc_msgSend)((id)objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"), text);
}

// The content type of a kind: `folder`, `file` or a `.extension`.
static id adm_file_icon_type(const char* kind) {
    id types = (id)objc_getClass("UTType");
    if (!types) return NULL;
    if (strcmp(kind, "folder") == 0) return adm_file_icon_send1(types, "typeWithIdentifier:", adm_file_icon_string("public.folder"));
    if (kind[0] == '.') {
        id found = adm_file_icon_send1(types, "typeWithFilenameExtension:", adm_file_icon_string(kind + 1));
        if (found) return found;
    }
    return adm_file_icon_send1(types, "typeWithIdentifier:", adm_file_icon_string("public.data"));
}

int32_t adm_file_icon_read(const char* kind, int64_t side, uint8_t* out) {
    int32_t ok = 0;
    void* pool = objc_autoreleasePoolPush();
    id type = adm_file_icon_type(kind ? kind : "file");
    id workspace = adm_file_icon_send((id)objc_getClass("NSWorkspace"), "sharedWorkspace");
    id icon = type && workspace ? adm_file_icon_send1(workspace, "iconForContentType:", type) : NULL;
    if (icon) {
        CGRect box = CGRectMake(0, 0, (CGFloat)side, (CGFloat)side);
        CGImageRef drawn = ((CGImageRef (*)(id, SEL, CGRect*, id, id))objc_msgSend)(icon, sel_registerName("CGImageForProposedRect:context:hints:"), &box, NULL, NULL);
        CGColorSpaceRef colors = CGColorSpaceCreateDeviceRGB();
        CGContextRef target = drawn && colors ? CGBitmapContextCreate(out, (size_t)side, (size_t)side, 8, (size_t)side * 4, colors, (uint32_t)kCGImageAlphaPremultipliedLast | (uint32_t)kCGBitmapByteOrder32Big) : NULL;
        if (target) {
            CGContextSetInterpolationQuality(target, kCGInterpolationHigh);
            CGContextDrawImage(target, box, drawn);
            CGContextRelease(target);
            // The context premultiplies; an image holds straight alpha.
            int64_t count = side * side;
            for (int64_t i = 0; i < count; i++) {
                uint8_t* px = out + i * 4;
                uint8_t alpha = px[3];
                if (alpha == 0 || alpha == 255) continue;
                for (int c = 0; c < 3; c++) {
                    int value = ((int)px[c] * 255 + alpha / 2) / alpha;
                    px[c] = (uint8_t)(value > 255 ? 255 : value);
                }
            }
            ok = 1;
        }
        if (colors) CGColorSpaceRelease(colors);
    }
    objc_autoreleasePoolPop(pool);
    return ok;
}

#endif // __APPLE__

#endif // ADM_FILEICONS_DARWIN_H
