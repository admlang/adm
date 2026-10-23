// The Vulkan entry points std.ui.renderer.vulkan.vk binds. Nothing links
// against the Vulkan loader: the host (SDL, which opens libvulkan itself) or
// adm_vk_load_system hands over vkGetInstanceProcAddr, and every function the
// renderer calls is fetched through it, instance-level ones once the instance
// exists and device-level ones through vkGetDeviceProcAddr once the device
// does. One instance and one device at a time per process, which is what the
// renderer makes.
//
// A Vulkan call takes structures that point at other structures and arrays;
// ADM passes scalars and arrays, so each call the renderer makes is a flat
// function here that fills the structures. The logic (what is drawn, when,
// with which resources) stays in the ADM modules; this file only translates.
// Only the sidecar of std.ui.renderer.vulkan.vk includes it, so a program holds one
// definition of each function (they have external linkage: the ADM calls bind to them).
#ifndef ADM_VK_LOADER_H
#define ADM_VK_LOADER_H

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

typedef uint64_t adm_vk_handle;

// The entry points, fetched by adm_vk_load / adm_vk_load_instance /
// adm_vk_load_device.
static struct {
    PFN_vkGetInstanceProcAddr getInstanceProcAddr;
    PFN_vkGetDeviceProcAddr getDeviceProcAddr;
    PFN_vkEnumerateInstanceVersion enumerateInstanceVersion;
    PFN_vkEnumerateInstanceLayerProperties enumerateInstanceLayerProperties;
    PFN_vkEnumerateInstanceExtensionProperties enumerateInstanceExtensionProperties;
    PFN_vkCreateInstance createInstance;
    PFN_vkDestroyInstance destroyInstance;
    PFN_vkEnumeratePhysicalDevices enumeratePhysicalDevices;
    PFN_vkGetPhysicalDeviceProperties getPhysicalDeviceProperties;
    PFN_vkGetPhysicalDeviceFeatures2 getPhysicalDeviceFeatures2;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties getPhysicalDeviceQueueFamilyProperties;
    PFN_vkGetPhysicalDeviceMemoryProperties getPhysicalDeviceMemoryProperties;
    PFN_vkEnumerateDeviceExtensionProperties enumerateDeviceExtensionProperties;
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR getPhysicalDeviceSurfaceSupportKHR;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR getPhysicalDeviceSurfaceCapabilitiesKHR;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR getPhysicalDeviceSurfaceFormatsKHR;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR getPhysicalDeviceSurfacePresentModesKHR;
    PFN_vkCreateDevice createDevice;
    PFN_vkCreateDebugUtilsMessengerEXT createDebugUtilsMessengerEXT;
    PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugUtilsMessengerEXT;

    PFN_vkDestroyDevice destroyDevice;
    PFN_vkGetDeviceQueue getDeviceQueue;
    PFN_vkDeviceWaitIdle deviceWaitIdle;
    PFN_vkQueueWaitIdle queueWaitIdle;
    PFN_vkQueueSubmit queueSubmit;
    PFN_vkCreateBuffer createBuffer;
    PFN_vkDestroyBuffer destroyBuffer;
    PFN_vkGetBufferMemoryRequirements getBufferMemoryRequirements;
    PFN_vkBindBufferMemory bindBufferMemory;
    PFN_vkCreateImage createImage;
    PFN_vkDestroyImage destroyImage;
    PFN_vkGetImageMemoryRequirements getImageMemoryRequirements;
    PFN_vkBindImageMemory bindImageMemory;
    PFN_vkCreateImageView createImageView;
    PFN_vkDestroyImageView destroyImageView;
    PFN_vkAllocateMemory allocateMemory;
    PFN_vkFreeMemory freeMemory;
    PFN_vkMapMemory mapMemory;
    PFN_vkUnmapMemory unmapMemory;
    PFN_vkCreateSampler createSampler;
    PFN_vkDestroySampler destroySampler;
    PFN_vkCreateDescriptorSetLayout createDescriptorSetLayout;
    PFN_vkDestroyDescriptorSetLayout destroyDescriptorSetLayout;
    PFN_vkCreatePipelineLayout createPipelineLayout;
    PFN_vkDestroyPipelineLayout destroyPipelineLayout;
    PFN_vkCreateDescriptorPool createDescriptorPool;
    PFN_vkDestroyDescriptorPool destroyDescriptorPool;
    PFN_vkResetDescriptorPool resetDescriptorPool;
    PFN_vkAllocateDescriptorSets allocateDescriptorSets;
    PFN_vkUpdateDescriptorSets updateDescriptorSets;
    PFN_vkCreateShaderModule createShaderModule;
    PFN_vkDestroyShaderModule destroyShaderModule;
    PFN_vkCreateGraphicsPipelines createGraphicsPipelines;
    PFN_vkDestroyPipeline destroyPipeline;
    PFN_vkCreateCommandPool createCommandPool;
    PFN_vkDestroyCommandPool destroyCommandPool;
    PFN_vkResetCommandPool resetCommandPool;
    PFN_vkAllocateCommandBuffers allocateCommandBuffers;
    PFN_vkBeginCommandBuffer beginCommandBuffer;
    PFN_vkEndCommandBuffer endCommandBuffer;
    PFN_vkCreateFence createFence;
    PFN_vkDestroyFence destroyFence;
    PFN_vkWaitForFences waitForFences;
    PFN_vkResetFences resetFences;
    PFN_vkCreateSemaphore createSemaphore;
    PFN_vkDestroySemaphore destroySemaphore;
    PFN_vkCmdPipelineBarrier2 cmdPipelineBarrier2;
    PFN_vkCmdCopyBufferToImage cmdCopyBufferToImage;
    PFN_vkCmdCopyImageToBuffer cmdCopyImageToBuffer;
    PFN_vkCmdCopyImage cmdCopyImage;
    PFN_vkCmdClearColorImage cmdClearColorImage;
    PFN_vkCmdBeginRendering cmdBeginRendering;
    PFN_vkCmdEndRendering cmdEndRendering;
    // Null on a device without VK_KHR_external_memory_fd.
    PFN_vkGetMemoryFdKHR getMemoryFdKHR;
    PFN_vkCmdBindPipeline cmdBindPipeline;
    PFN_vkCmdBindDescriptorSets cmdBindDescriptorSets;
    PFN_vkCmdSetViewport cmdSetViewport;
    PFN_vkCmdSetScissor cmdSetScissor;
    PFN_vkCmdDraw cmdDraw;
    PFN_vkCreateSwapchainKHR createSwapchainKHR;
    PFN_vkDestroySwapchainKHR destroySwapchainKHR;
    PFN_vkGetSwapchainImagesKHR getSwapchainImagesKHR;
    PFN_vkAcquireNextImageKHR acquireNextImageKHR;
    PFN_vkQueuePresentKHR queuePresentKHR;
    PFN_vkDestroySurfaceKHR destroySurfaceKHR;

    VkInstance instance;
    VkDebugUtilsMessengerEXT messenger;
    VkDevice device;
    VkPhysicalDeviceMemoryProperties memory;
    void* library;
} adm_vk;

#define ADM_VK_INSTANCE_PROC(field, name) adm_vk.field = (PFN_##name)adm_vk.getInstanceProcAddr(adm_vk.instance, #name)
#define ADM_VK_DEVICE_PROC(field, name) adm_vk.field = (PFN_##name)adm_vk.getDeviceProcAddr(adm_vk.device, #name)

// Flags ADM passes (`debug`, `host`, `linear`, `repeat`, `clear`) are uint32_t, 0 or
// not: an ADM `true` reaches C as 0xFF, which is not a value of C's `bool`.

// Takes vkGetInstanceProcAddr and fetches the entry points that need no
// instance. False when it is 0.
bool adm_vk_load(adm_vk_handle getInstanceProcAddr) {
    memset(&adm_vk, 0, sizeof adm_vk);
    adm_vk.getInstanceProcAddr = (PFN_vkGetInstanceProcAddr)(uintptr_t)getInstanceProcAddr;
    if (!adm_vk.getInstanceProcAddr) return false;
    ADM_VK_INSTANCE_PROC(enumerateInstanceVersion, vkEnumerateInstanceVersion);
    ADM_VK_INSTANCE_PROC(enumerateInstanceLayerProperties, vkEnumerateInstanceLayerProperties);
    ADM_VK_INSTANCE_PROC(enumerateInstanceExtensionProperties, vkEnumerateInstanceExtensionProperties);
    ADM_VK_INSTANCE_PROC(createInstance, vkCreateInstance);
    return adm_vk.createInstance != 0;
}

// Opens the machine's Vulkan loader (for a renderer with no host giving one:
// the suites) and returns its vkGetInstanceProcAddr, 0 when there is none.
adm_vk_handle adm_vk_load_system(void) {
#if defined(_WIN32)
    HMODULE lib = LoadLibraryA("vulkan-1.dll");
    if (!lib) return 0;
    adm_vk.library = (void*)lib;
    return (adm_vk_handle)(uintptr_t)GetProcAddress(lib, "vkGetInstanceProcAddr");
#else
#if defined(__APPLE__)
    const char* names[] = {"libvulkan.1.dylib", "libvulkan.dylib", "libMoltenVK.dylib", 0};
#else
    const char* names[] = {"libvulkan.so.1", "libvulkan.so", 0};
#endif
    for (int i = 0; names[i]; i++) {
        void* lib = dlopen(names[i], RTLD_NOW | RTLD_LOCAL);
        if (!lib) continue;
        void* proc = dlsym(lib, "vkGetInstanceProcAddr");
        if (proc) {
            adm_vk.library = lib;
            return (adm_vk_handle)(uintptr_t)proc;
        }
        dlclose(lib);
    }
    return 0;
#endif
}

// The highest instance version the loader supports, as VK_MAKE_API_VERSION.
uint32_t adm_vk_instance_version(void) {
    uint32_t v = VK_API_VERSION_1_0;
    if (adm_vk.enumerateInstanceVersion) adm_vk.enumerateInstanceVersion(&v);
    return v;
}

// Whether the loader has the instance layer.
bool adm_vk_has_layer(const char* name) {
    uint32_t n = 0;
    if (adm_vk.enumerateInstanceLayerProperties(&n, 0) != VK_SUCCESS || n == 0) return false;
    VkLayerProperties* props = (VkLayerProperties*)calloc(n, sizeof *props);
    if (!props) return false;
    bool found = false;
    if (adm_vk.enumerateInstanceLayerProperties(&n, props) == VK_SUCCESS) {
        for (uint32_t i = 0; i < n && !found; i++) found = strcmp(props[i].layerName, name) == 0;
    }
    free(props);
    return found;
}

// Whether the loader has the instance extension.
bool adm_vk_has_instance_extension(const char* name) {
    uint32_t n = 0;
    if (adm_vk.enumerateInstanceExtensionProperties(0, &n, 0) != VK_SUCCESS || n == 0) return false;
    VkExtensionProperties* props = (VkExtensionProperties*)calloc(n, sizeof *props);
    if (!props) return false;
    bool found = false;
    if (adm_vk.enumerateInstanceExtensionProperties(0, &n, props) == VK_SUCCESS) {
        for (uint32_t i = 0; i < n && !found; i++) found = strcmp(props[i].extensionName, name) == 0;
    }
    free(props);
    return found;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL adm_vk_debug_message(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT types, const VkDebugUtilsMessengerCallbackDataEXT* data, void* user) {
    (void)types;
    (void)user;
    const char* level = severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ? "error" : severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ? "warning" : "info";
    fprintf(stderr, "vulkan %s: %s\n", level, data && data->pMessage ? data->pMessage : "");
    return VK_FALSE;
}

// Creates the instance with the extensions and layers named (as many as
// `extensionCount` and `layerCount`), fetches its entry points, and with
// `debug` reports validation messages on stderr (VK_EXT_debug_utils must be
// among the extensions). The instance goes to out[0]. A VkResult.
int32_t adm_vk_create_instance(uint32_t apiVersion, const char* const* extensions, uint32_t extensionCount, const char* const* layers, uint32_t layerCount, uint32_t debug, adm_vk_handle* out) {
    VkApplicationInfo app = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "adm";
    app.pEngineName = "std.ui.renderer.vulkan";
    app.apiVersion = apiVersion;
    VkInstanceCreateInfo info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    info.pApplicationInfo = &app;
    info.enabledExtensionCount = extensionCount;
    info.ppEnabledExtensionNames = extensions;
    info.enabledLayerCount = layerCount;
    info.ppEnabledLayerNames = layers;
    // MoltenVK's devices are listed only when portability enumeration is
    // asked for (the extension among the names, and this flag).
    for (uint32_t i = 0; i < extensionCount; i++) {
        if (strcmp(extensions[i], VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0) info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    }
    VkInstance instance = 0;
    VkResult r = adm_vk.createInstance(&info, 0, &instance);
    if (r != VK_SUCCESS) return r;
    adm_vk.instance = instance;
    ADM_VK_INSTANCE_PROC(getDeviceProcAddr, vkGetDeviceProcAddr);
    ADM_VK_INSTANCE_PROC(destroyInstance, vkDestroyInstance);
    ADM_VK_INSTANCE_PROC(enumeratePhysicalDevices, vkEnumeratePhysicalDevices);
    ADM_VK_INSTANCE_PROC(getPhysicalDeviceProperties, vkGetPhysicalDeviceProperties);
    ADM_VK_INSTANCE_PROC(getPhysicalDeviceFeatures2, vkGetPhysicalDeviceFeatures2);
    ADM_VK_INSTANCE_PROC(getPhysicalDeviceQueueFamilyProperties, vkGetPhysicalDeviceQueueFamilyProperties);
    ADM_VK_INSTANCE_PROC(getPhysicalDeviceMemoryProperties, vkGetPhysicalDeviceMemoryProperties);
    ADM_VK_INSTANCE_PROC(enumerateDeviceExtensionProperties, vkEnumerateDeviceExtensionProperties);
    ADM_VK_INSTANCE_PROC(getPhysicalDeviceSurfaceSupportKHR, vkGetPhysicalDeviceSurfaceSupportKHR);
    ADM_VK_INSTANCE_PROC(getPhysicalDeviceSurfaceCapabilitiesKHR, vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    ADM_VK_INSTANCE_PROC(getPhysicalDeviceSurfaceFormatsKHR, vkGetPhysicalDeviceSurfaceFormatsKHR);
    ADM_VK_INSTANCE_PROC(getPhysicalDeviceSurfacePresentModesKHR, vkGetPhysicalDeviceSurfacePresentModesKHR);
    ADM_VK_INSTANCE_PROC(destroySurfaceKHR, vkDestroySurfaceKHR);
    ADM_VK_INSTANCE_PROC(createDevice, vkCreateDevice);
    ADM_VK_INSTANCE_PROC(createDebugUtilsMessengerEXT, vkCreateDebugUtilsMessengerEXT);
    ADM_VK_INSTANCE_PROC(destroyDebugUtilsMessengerEXT, vkDestroyDebugUtilsMessengerEXT);
    if (debug && adm_vk.createDebugUtilsMessengerEXT) {
        VkDebugUtilsMessengerCreateInfoEXT d = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
        d.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        d.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        d.pfnUserCallback = adm_vk_debug_message;
        adm_vk.createDebugUtilsMessengerEXT(instance, &d, 0, &adm_vk.messenger);
    }
    out[0] = (adm_vk_handle)(uintptr_t)instance;
    return VK_SUCCESS;
}

void adm_vk_destroy_instance(adm_vk_handle instance) {
    if (adm_vk.messenger && adm_vk.destroyDebugUtilsMessengerEXT) adm_vk.destroyDebugUtilsMessengerEXT((VkInstance)(uintptr_t)instance, adm_vk.messenger, 0);
    adm_vk.messenger = 0;
    if (adm_vk.destroyInstance) adm_vk.destroyInstance((VkInstance)(uintptr_t)instance, 0);
    adm_vk.instance = 0;
}

// The physical devices, up to `max` of them into out; how many there are.
uint32_t adm_vk_physical_devices(adm_vk_handle instance, adm_vk_handle* out, uint32_t max) {
    uint32_t n = 0;
    if (adm_vk.enumeratePhysicalDevices((VkInstance)(uintptr_t)instance, &n, 0) != VK_SUCCESS || n == 0) return 0;
    VkPhysicalDevice* devices = (VkPhysicalDevice*)calloc(n, sizeof *devices);
    if (!devices) return 0;
    adm_vk.enumeratePhysicalDevices((VkInstance)(uintptr_t)instance, &n, devices);
    for (uint32_t i = 0; i < n && i < max; i++) out[i] = (adm_vk_handle)(uintptr_t)devices[i];
    free(devices);
    return n;
}

// What the renderer asks of a physical device: info[0] its API version,
// info[1] its type (VkPhysicalDeviceType), info[2] whether it has 64-bit
// floats in shaders, info[3] dynamic rendering, info[4] synchronization2,
// info[5] the smallest uniform buffer offset alignment, info[6] the largest
// 2D image side, info[7] whether it has VK_KHR_swapchain, info[8] the buffer-image
// granularity, info[9] whether it reads its colour attachment back within a
// render pass (VK_KHR_dynamic_rendering_local_read), info[10] whether it is
// a portability subset (MoltenVK: VK_KHR_portability_subset, which the device
// must then enable), info[11] whether its images' memory can be exported as
// a file descriptor (VK_KHR_external_memory_fd), info[12] its vendor id
// (0x10DE NVIDIA); `name` (at least 256 bytes) its name, NUL-terminated. A Vulkan 1.2 device answers info[3] and info[4]
// from the KHR extensions that became core in 1.3.
void adm_vk_physical_device_info(adm_vk_handle physical, uint32_t* info, uint8_t* name) {
    VkPhysicalDevice dev = (VkPhysicalDevice)(uintptr_t)physical;
    VkPhysicalDeviceProperties props;
    adm_vk.getPhysicalDeviceProperties(dev, &props);
    info[0] = props.apiVersion;
    info[1] = (uint32_t)props.deviceType;
    info[5] = (uint32_t)props.limits.minUniformBufferOffsetAlignment;
    info[6] = props.limits.maxImageDimension2D;
    info[8] = (uint32_t)props.limits.bufferImageGranularity;
    info[12] = props.vendorID;
    size_t n = strlen(props.deviceName);
    if (n > 255) n = 255;
    memcpy(name, props.deviceName, n);
    name[n] = 0;
    info[7] = 0;
    info[10] = 0;
    info[11] = 0;
    int localReadExt = 0, dynamicExt = 0, sync2Ext = 0;
    uint32_t count = 0;
    if (adm_vk.enumerateDeviceExtensionProperties(dev, 0, &count, 0) == VK_SUCCESS && count > 0) {
        VkExtensionProperties* exts = (VkExtensionProperties*)calloc(count, sizeof *exts);
        if (exts && adm_vk.enumerateDeviceExtensionProperties(dev, 0, &count, exts) == VK_SUCCESS) {
            for (uint32_t i = 0; i < count; i++) {
                const char* name_ = exts[i].extensionName;
                if (strcmp(name_, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) info[7] = 1;
                if (strcmp(name_, VK_KHR_DYNAMIC_RENDERING_LOCAL_READ_EXTENSION_NAME) == 0) localReadExt = 1;
                if (strcmp(name_, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME) == 0) dynamicExt = 1;
                if (strcmp(name_, VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME) == 0) sync2Ext = 1;
                if (strcmp(name_, "VK_KHR_portability_subset") == 0) info[10] = 1;
                if (strcmp(name_, VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME) == 0) info[11] = 1;
            }
        }
        free(exts);
    }
    int core13 = props.apiVersion >= VK_API_VERSION_1_3;
    VkPhysicalDeviceDynamicRenderingLocalReadFeaturesKHR local = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_LOCAL_READ_FEATURES_KHR};
    VkPhysicalDeviceVulkan13Features f13 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    VkPhysicalDeviceDynamicRenderingFeaturesKHR dyn = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR};
    VkPhysicalDeviceSynchronization2FeaturesKHR sync2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR};
    VkPhysicalDeviceFeatures2 f2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    void** tail = &f2.pNext;
    if (core13) { *tail = &f13; tail = &f13.pNext; }
    else {
        if (dynamicExt) { *tail = &dyn; tail = &dyn.pNext; }
        if (sync2Ext) { *tail = &sync2; tail = &sync2.pNext; }
    }
    if (localReadExt) { *tail = &local; tail = &local.pNext; }
    adm_vk.getPhysicalDeviceFeatures2(dev, &f2);
    info[2] = f2.features.shaderFloat64;
    info[3] = core13 ? f13.dynamicRendering : dyn.dynamicRendering;
    info[4] = core13 ? f13.synchronization2 : sync2.synchronization2;
    info[9] = localReadExt && local.dynamicRenderingLocalRead;
}

// The first queue family with graphics (and presentation to `surface`, when
// it is not 0); -1 when there is none.
int32_t adm_vk_queue_family(adm_vk_handle physical, adm_vk_handle surface) {
    VkPhysicalDevice dev = (VkPhysicalDevice)(uintptr_t)physical;
    uint32_t n = 0;
    adm_vk.getPhysicalDeviceQueueFamilyProperties(dev, &n, 0);
    if (n == 0) return -1;
    VkQueueFamilyProperties* families = (VkQueueFamilyProperties*)calloc(n, sizeof *families);
    if (!families) return -1;
    adm_vk.getPhysicalDeviceQueueFamilyProperties(dev, &n, families);
    int32_t found = -1;
    for (uint32_t i = 0; i < n && found < 0; i++) {
        if (!(families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) continue;
        if (surface) {
            VkBool32 presents = VK_FALSE;
            adm_vk.getPhysicalDeviceSurfaceSupportKHR(dev, i, (VkSurfaceKHR)(uintptr_t)surface, &presents);
            if (!presents) continue;
        }
        found = (int32_t)i;
    }
    free(families);
    return found;
}

// Whether the queue family of the device can present to the surface.
bool adm_vk_presents(adm_vk_handle physical, uint32_t family, adm_vk_handle surface) {
    VkBool32 presents = VK_FALSE;
    adm_vk.getPhysicalDeviceSurfaceSupportKHR((VkPhysicalDevice)(uintptr_t)physical, family, (VkSurfaceKHR)(uintptr_t)surface, &presents);
    return presents == VK_TRUE;
}

// Creates the device on the queue family with the extensions named, with
// dynamic rendering and synchronization2 on (through the Vulkan 1.3 features
// or, on a 1.2 device, the KHR extensions among the names), 64-bit shader
// floats when `float64` and the local read of colour attachments when
// `localRead` (its extension among the names), and fetches its entry points,
// the KHR-suffixed ones on a 1.2 device: out[0] the device, out[1] its queue.
// A VkResult.
int32_t adm_vk_create_device(adm_vk_handle physical, uint32_t family, const char* const* extensions, uint32_t extensionCount, uint32_t localRead, uint32_t float64, adm_vk_handle* out) {
    VkPhysicalDevice dev = (VkPhysicalDevice)(uintptr_t)physical;
    VkPhysicalDeviceProperties props;
    adm_vk.getPhysicalDeviceProperties(dev, &props);
    int core13 = props.apiVersion >= VK_API_VERSION_1_3;
    float priority = 1.0f;
    VkDeviceQueueCreateInfo queue = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queue.queueFamilyIndex = family;
    queue.queueCount = 1;
    queue.pQueuePriorities = &priority;
    VkPhysicalDeviceDynamicRenderingLocalReadFeaturesKHR local = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_LOCAL_READ_FEATURES_KHR};
    local.dynamicRenderingLocalRead = VK_TRUE;
    VkPhysicalDeviceVulkan13Features f13 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    f13.dynamicRendering = VK_TRUE;
    f13.synchronization2 = VK_TRUE;
    VkPhysicalDeviceDynamicRenderingFeaturesKHR dyn = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR};
    dyn.dynamicRendering = VK_TRUE;
    VkPhysicalDeviceSynchronization2FeaturesKHR sync2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR};
    sync2.synchronization2 = VK_TRUE;
    VkPhysicalDeviceFeatures2 f2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    void** tail = &f2.pNext;
    if (core13) { *tail = &f13; tail = &f13.pNext; }
    else { *tail = &dyn; dyn.pNext = &sync2; tail = &sync2.pNext; }
    if (localRead) { *tail = &local; tail = &local.pNext; }
    f2.features.shaderFloat64 = float64 ? VK_TRUE : VK_FALSE;
    VkDeviceCreateInfo info = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    info.pNext = &f2;
    info.queueCreateInfoCount = 1;
    info.pQueueCreateInfos = &queue;
    info.enabledExtensionCount = extensionCount;
    info.ppEnabledExtensionNames = extensions;
    VkDevice device = 0;
    VkResult r = adm_vk.createDevice(dev, &info, 0, &device);
    if (r != VK_SUCCESS) return r;
    adm_vk.device = device;
    adm_vk.getPhysicalDeviceMemoryProperties(dev, &adm_vk.memory);
    ADM_VK_DEVICE_PROC(destroyDevice, vkDestroyDevice);
    ADM_VK_DEVICE_PROC(getDeviceQueue, vkGetDeviceQueue);
    ADM_VK_DEVICE_PROC(deviceWaitIdle, vkDeviceWaitIdle);
    ADM_VK_DEVICE_PROC(queueWaitIdle, vkQueueWaitIdle);
    ADM_VK_DEVICE_PROC(queueSubmit, vkQueueSubmit);
    ADM_VK_DEVICE_PROC(createBuffer, vkCreateBuffer);
    ADM_VK_DEVICE_PROC(destroyBuffer, vkDestroyBuffer);
    ADM_VK_DEVICE_PROC(getBufferMemoryRequirements, vkGetBufferMemoryRequirements);
    ADM_VK_DEVICE_PROC(bindBufferMemory, vkBindBufferMemory);
    ADM_VK_DEVICE_PROC(createImage, vkCreateImage);
    ADM_VK_DEVICE_PROC(destroyImage, vkDestroyImage);
    ADM_VK_DEVICE_PROC(getImageMemoryRequirements, vkGetImageMemoryRequirements);
    ADM_VK_DEVICE_PROC(bindImageMemory, vkBindImageMemory);
    ADM_VK_DEVICE_PROC(createImageView, vkCreateImageView);
    ADM_VK_DEVICE_PROC(destroyImageView, vkDestroyImageView);
    ADM_VK_DEVICE_PROC(allocateMemory, vkAllocateMemory);
    ADM_VK_DEVICE_PROC(freeMemory, vkFreeMemory);
    ADM_VK_DEVICE_PROC(mapMemory, vkMapMemory);
    ADM_VK_DEVICE_PROC(unmapMemory, vkUnmapMemory);
    ADM_VK_DEVICE_PROC(createSampler, vkCreateSampler);
    ADM_VK_DEVICE_PROC(destroySampler, vkDestroySampler);
    ADM_VK_DEVICE_PROC(createDescriptorSetLayout, vkCreateDescriptorSetLayout);
    ADM_VK_DEVICE_PROC(destroyDescriptorSetLayout, vkDestroyDescriptorSetLayout);
    ADM_VK_DEVICE_PROC(createPipelineLayout, vkCreatePipelineLayout);
    ADM_VK_DEVICE_PROC(destroyPipelineLayout, vkDestroyPipelineLayout);
    ADM_VK_DEVICE_PROC(createDescriptorPool, vkCreateDescriptorPool);
    ADM_VK_DEVICE_PROC(destroyDescriptorPool, vkDestroyDescriptorPool);
    ADM_VK_DEVICE_PROC(resetDescriptorPool, vkResetDescriptorPool);
    ADM_VK_DEVICE_PROC(allocateDescriptorSets, vkAllocateDescriptorSets);
    ADM_VK_DEVICE_PROC(updateDescriptorSets, vkUpdateDescriptorSets);
    ADM_VK_DEVICE_PROC(createShaderModule, vkCreateShaderModule);
    ADM_VK_DEVICE_PROC(destroyShaderModule, vkDestroyShaderModule);
    ADM_VK_DEVICE_PROC(createGraphicsPipelines, vkCreateGraphicsPipelines);
    ADM_VK_DEVICE_PROC(destroyPipeline, vkDestroyPipeline);
    ADM_VK_DEVICE_PROC(createCommandPool, vkCreateCommandPool);
    ADM_VK_DEVICE_PROC(destroyCommandPool, vkDestroyCommandPool);
    ADM_VK_DEVICE_PROC(resetCommandPool, vkResetCommandPool);
    ADM_VK_DEVICE_PROC(allocateCommandBuffers, vkAllocateCommandBuffers);
    ADM_VK_DEVICE_PROC(beginCommandBuffer, vkBeginCommandBuffer);
    ADM_VK_DEVICE_PROC(endCommandBuffer, vkEndCommandBuffer);
    ADM_VK_DEVICE_PROC(createFence, vkCreateFence);
    ADM_VK_DEVICE_PROC(destroyFence, vkDestroyFence);
    ADM_VK_DEVICE_PROC(waitForFences, vkWaitForFences);
    ADM_VK_DEVICE_PROC(resetFences, vkResetFences);
    ADM_VK_DEVICE_PROC(createSemaphore, vkCreateSemaphore);
    ADM_VK_DEVICE_PROC(destroySemaphore, vkDestroySemaphore);
    ADM_VK_DEVICE_PROC(cmdPipelineBarrier2, vkCmdPipelineBarrier2);
    if (!adm_vk.cmdPipelineBarrier2) ADM_VK_DEVICE_PROC(cmdPipelineBarrier2, vkCmdPipelineBarrier2KHR);
    ADM_VK_DEVICE_PROC(cmdCopyBufferToImage, vkCmdCopyBufferToImage);
    ADM_VK_DEVICE_PROC(cmdCopyImageToBuffer, vkCmdCopyImageToBuffer);
    ADM_VK_DEVICE_PROC(cmdCopyImage, vkCmdCopyImage);
    ADM_VK_DEVICE_PROC(cmdClearColorImage, vkCmdClearColorImage);
    ADM_VK_DEVICE_PROC(cmdBeginRendering, vkCmdBeginRendering);
    if (!adm_vk.cmdBeginRendering) ADM_VK_DEVICE_PROC(cmdBeginRendering, vkCmdBeginRenderingKHR);
    ADM_VK_DEVICE_PROC(cmdEndRendering, vkCmdEndRendering);
    if (!adm_vk.cmdEndRendering) ADM_VK_DEVICE_PROC(cmdEndRendering, vkCmdEndRenderingKHR);
    ADM_VK_DEVICE_PROC(getMemoryFdKHR, vkGetMemoryFdKHR);
    ADM_VK_DEVICE_PROC(cmdBindPipeline, vkCmdBindPipeline);
    ADM_VK_DEVICE_PROC(cmdBindDescriptorSets, vkCmdBindDescriptorSets);
    ADM_VK_DEVICE_PROC(cmdSetViewport, vkCmdSetViewport);
    ADM_VK_DEVICE_PROC(cmdSetScissor, vkCmdSetScissor);
    ADM_VK_DEVICE_PROC(cmdDraw, vkCmdDraw);
    ADM_VK_DEVICE_PROC(createSwapchainKHR, vkCreateSwapchainKHR);
    ADM_VK_DEVICE_PROC(destroySwapchainKHR, vkDestroySwapchainKHR);
    ADM_VK_DEVICE_PROC(getSwapchainImagesKHR, vkGetSwapchainImagesKHR);
    ADM_VK_DEVICE_PROC(acquireNextImageKHR, vkAcquireNextImageKHR);
    ADM_VK_DEVICE_PROC(queuePresentKHR, vkQueuePresentKHR);
    VkQueue q = 0;
    adm_vk.getDeviceQueue(device, family, 0, &q);
    out[0] = (adm_vk_handle)(uintptr_t)device;
    out[1] = (adm_vk_handle)(uintptr_t)q;
    return VK_SUCCESS;
}

void adm_vk_destroy_device(adm_vk_handle device) {
    if (adm_vk.destroyDevice) adm_vk.destroyDevice((VkDevice)(uintptr_t)device, 0);
    adm_vk.device = 0;
}

int32_t adm_vk_device_wait_idle(adm_vk_handle device) {
    return adm_vk.deviceWaitIdle((VkDevice)(uintptr_t)device);
}

void adm_vk_destroy_surface(adm_vk_handle instance, adm_vk_handle surface) {
    adm_vk.destroySurfaceKHR((VkInstance)(uintptr_t)instance, (VkSurfaceKHR)(uintptr_t)surface, 0);
}

// A memory type with the properties, from the types `mask` allows; -1 when
// there is none.
int32_t adm_vk_memory_type(uint32_t mask, VkMemoryPropertyFlags wanted) {
    for (uint32_t i = 0; i < adm_vk.memory.memoryTypeCount; i++) {
        if ((mask & (1u << i)) && (adm_vk.memory.memoryTypes[i].propertyFlags & wanted) == wanted) return (int32_t)i;
    }
    return -1;
}

// A buffer of `size` bytes for `usage`, with its memory bound: out[0] the
// buffer, out[1] its memory, out[2] where it is mapped (`host`: visible and
// coherent memory, mapped for good; else device memory, 0). A VkResult.
int32_t adm_vk_create_buffer(adm_vk_handle device, uint64_t size, uint32_t usage, uint32_t host, adm_vk_handle* out) {
    VkDevice dev = (VkDevice)(uintptr_t)device;
    VkBufferCreateInfo info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    info.size = size;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkBuffer buffer = 0;
    VkResult r = adm_vk.createBuffer(dev, &info, 0, &buffer);
    if (r != VK_SUCCESS) return r;
    VkMemoryRequirements req;
    adm_vk.getBufferMemoryRequirements(dev, buffer, &req);
    VkMemoryPropertyFlags wanted = host ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    int32_t type = adm_vk_memory_type(req.memoryTypeBits, wanted);
    if (type < 0 && !host) type = adm_vk_memory_type(req.memoryTypeBits, 0);
    if (type < 0) {
        adm_vk.destroyBuffer(dev, buffer, 0);
        return VK_ERROR_FEATURE_NOT_PRESENT;
    }
    VkMemoryAllocateInfo alloc = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    alloc.allocationSize = req.size;
    alloc.memoryTypeIndex = (uint32_t)type;
    VkDeviceMemory memory = 0;
    r = adm_vk.allocateMemory(dev, &alloc, 0, &memory);
    if (r != VK_SUCCESS) {
        adm_vk.destroyBuffer(dev, buffer, 0);
        return r;
    }
    r = adm_vk.bindBufferMemory(dev, buffer, memory, 0);
    void* mapped = 0;
    if (r == VK_SUCCESS && host) r = adm_vk.mapMemory(dev, memory, 0, VK_WHOLE_SIZE, 0, &mapped);
    if (r != VK_SUCCESS) {
        adm_vk.destroyBuffer(dev, buffer, 0);
        adm_vk.freeMemory(dev, memory, 0);
        return r;
    }
    out[0] = (adm_vk_handle)(uintptr_t)buffer;
    out[1] = (adm_vk_handle)(uintptr_t)memory;
    out[2] = (adm_vk_handle)(uintptr_t)mapped;
    return VK_SUCCESS;
}

void adm_vk_destroy_buffer(adm_vk_handle device, adm_vk_handle buffer, adm_vk_handle memory) {
    VkDevice dev = (VkDevice)(uintptr_t)device;
    if (buffer) adm_vk.destroyBuffer(dev, (VkBuffer)(uintptr_t)buffer, 0);
    if (memory) adm_vk.freeMemory(dev, (VkDeviceMemory)(uintptr_t)memory, 0);
}

// Copies `n` bytes of `data` into mapped memory at `offset`.
void adm_vk_write_mapped(adm_vk_handle mapped, uint64_t offset, const void* data, uint64_t n) {
    memcpy((uint8_t*)(uintptr_t)mapped + offset, data, (size_t)n);
}

// Copies `n` bytes of mapped memory at `offset` into `out`.
void adm_vk_read_mapped(adm_vk_handle mapped, uint64_t offset, uint8_t* out, uint64_t n) {
    memcpy(out, (const uint8_t*)(uintptr_t)mapped + offset, (size_t)n);
}

// A 2D image of w x h in `format` for `usage`, one level, with no memory
// yet: out[0] the image, out[1] the bytes it needs, out[2] their alignment,
// out[3] the memory types that may hold it (a bit each). A VkResult.
int32_t adm_vk_create_image(adm_vk_handle device, uint32_t w, uint32_t h, uint32_t format, uint32_t usage, adm_vk_handle* out) {
    VkDevice dev = (VkDevice)(uintptr_t)device;
    VkImageCreateInfo info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = (VkFormat)format;
    info.extent.width = w;
    info.extent.height = h;
    info.extent.depth = 1;
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImage image = 0;
    VkResult r = adm_vk.createImage(dev, &info, 0, &image);
    if (r != VK_SUCCESS) return r;
    VkMemoryRequirements req;
    adm_vk.getImageMemoryRequirements(dev, image, &req);
    out[0] = (adm_vk_handle)(uintptr_t)image;
    out[1] = req.size;
    out[2] = req.alignment;
    out[3] = req.memoryTypeBits;
    return VK_SUCCESS;
}

// Binds the image to the memory at the offset and makes a view over it:
// out[0] the view. A VkResult.
int32_t adm_vk_bind_image(adm_vk_handle device, adm_vk_handle image, uint32_t format, adm_vk_handle memory, uint64_t offset, adm_vk_handle* out) {
    VkDevice dev = (VkDevice)(uintptr_t)device;
    VkResult r = adm_vk.bindImageMemory(dev, (VkImage)(uintptr_t)image, (VkDeviceMemory)(uintptr_t)memory, offset);
    if (r != VK_SUCCESS) return r;
    VkImageViewCreateInfo view = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view.image = (VkImage)(uintptr_t)image;
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view.format = (VkFormat)format;
    view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view.subresourceRange.levelCount = 1;
    view.subresourceRange.layerCount = 1;
    VkImageView v = 0;
    r = adm_vk.createImageView(dev, &view, 0, &v);
    if (r == VK_SUCCESS) out[0] = (adm_vk_handle)(uintptr_t)v;
    return r;
}

// The memory type (an index) of the device's with the properties (a device
// local one when `deviceLocal`), among the types `mask` allows; -1 for none.
int32_t adm_vk_choose_memory(uint32_t mask, uint32_t deviceLocal) {
    int32_t type = adm_vk_memory_type(mask, deviceLocal ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : 0);
    if (type < 0 && deviceLocal) type = adm_vk_memory_type(mask, 0);
    return type;
}

// Allocates `size` bytes of the memory type: out[0]. A VkResult.
int32_t adm_vk_allocate(adm_vk_handle device, uint64_t size, uint32_t type, adm_vk_handle* out) {
    VkMemoryAllocateInfo alloc = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    alloc.allocationSize = size;
    alloc.memoryTypeIndex = type;
    VkDeviceMemory memory = 0;
    VkResult r = adm_vk.allocateMemory((VkDevice)(uintptr_t)device, &alloc, 0, &memory);
    if (r == VK_SUCCESS) out[0] = (adm_vk_handle)(uintptr_t)memory;
    return r;
}

void adm_vk_free(adm_vk_handle device, adm_vk_handle memory) {
    if (memory) adm_vk.freeMemory((VkDevice)(uintptr_t)device, (VkDeviceMemory)(uintptr_t)memory, 0);
}

// Destroys an image and its view; its memory is the pool's.
void adm_vk_destroy_image(adm_vk_handle device, adm_vk_handle image, adm_vk_handle view) {
    VkDevice dev = (VkDevice)(uintptr_t)device;
    if (view) adm_vk.destroyImageView(dev, (VkImageView)(uintptr_t)view, 0);
    if (image) adm_vk.destroyImage(dev, (VkImage)(uintptr_t)image, 0);
}

// An image whose memory another API can import (CUDA, for a decoded
// picture copied into it on the GPU): optimal tiling, a dedicated allocation
// of device-local memory exportable as an opaque file descriptor
// (VK_KHR_external_memory_fd), bound, with a view: out[0] the image, out[1]
// its memory, out[2] the view, out[3] the memory's size. A VkResult; an
// error leaves nothing behind.
int32_t adm_vk_create_exported_image(adm_vk_handle device, uint32_t w, uint32_t h, uint32_t format, uint32_t usage, adm_vk_handle* out) {
    VkDevice dev = (VkDevice)(uintptr_t)device;
    VkExternalMemoryImageCreateInfo external = {VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO};
    external.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
    VkImageCreateInfo info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    info.pNext = &external;
    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = (VkFormat)format;
    info.extent.width = w;
    info.extent.height = h;
    info.extent.depth = 1;
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImage image = 0;
    VkResult r = adm_vk.createImage(dev, &info, 0, &image);
    if (r != VK_SUCCESS) return r;
    VkMemoryRequirements req;
    adm_vk.getImageMemoryRequirements(dev, image, &req);
    int32_t type = adm_vk_memory_type(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (type < 0) type = adm_vk_memory_type(req.memoryTypeBits, 0);
    if (type < 0) {
        adm_vk.destroyImage(dev, image, 0);
        return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    }
    VkMemoryDedicatedAllocateInfo dedicated = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO};
    dedicated.image = image;
    VkExportMemoryAllocateInfo export_ = {VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO};
    export_.pNext = &dedicated;
    export_.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
    VkMemoryAllocateInfo alloc = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    alloc.pNext = &export_;
    alloc.allocationSize = req.size;
    alloc.memoryTypeIndex = (uint32_t)type;
    VkDeviceMemory memory = 0;
    r = adm_vk.allocateMemory(dev, &alloc, 0, &memory);
    if (r != VK_SUCCESS) {
        adm_vk.destroyImage(dev, image, 0);
        return r;
    }
    adm_vk_handle view[1] = {0};
    r = adm_vk_bind_image(device, (adm_vk_handle)(uintptr_t)image, format, (adm_vk_handle)(uintptr_t)memory, 0, view);
    if (r != VK_SUCCESS) {
        adm_vk.freeMemory(dev, memory, 0);
        adm_vk.destroyImage(dev, image, 0);
        return r;
    }
    out[0] = (adm_vk_handle)(uintptr_t)image;
    out[1] = (adm_vk_handle)(uintptr_t)memory;
    out[2] = view[0];
    out[3] = req.size;
    return VK_SUCCESS;
}

// A file descriptor over the exported memory, for another API to import
// (which then owns it); -1 when the device cannot export memory.
int32_t adm_vk_export_memory_fd(adm_vk_handle device, adm_vk_handle memory) {
    if (!adm_vk.getMemoryFdKHR) return -1;
    VkMemoryGetFdInfoKHR info = {VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR};
    info.memory = (VkDeviceMemory)(uintptr_t)memory;
    info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
    int fd = -1;
    if (adm_vk.getMemoryFdKHR((VkDevice)(uintptr_t)device, &info, &fd) != VK_SUCCESS) return -1;
    return fd;
}

// A view over an image the renderer did not make (a swapchain's). A VkResult.
int32_t adm_vk_create_view(adm_vk_handle device, adm_vk_handle image, uint32_t format, adm_vk_handle* out) {
    VkImageViewCreateInfo view = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view.image = (VkImage)(uintptr_t)image;
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view.format = (VkFormat)format;
    view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view.subresourceRange.levelCount = 1;
    view.subresourceRange.layerCount = 1;
    VkImageView v = 0;
    VkResult r = adm_vk.createImageView((VkDevice)(uintptr_t)device, &view, 0, &v);
    if (r == VK_SUCCESS) out[0] = (adm_vk_handle)(uintptr_t)v;
    return r;
}

void adm_vk_destroy_view(adm_vk_handle device, adm_vk_handle view) {
    if (view) adm_vk.destroyImageView((VkDevice)(uintptr_t)device, (VkImageView)(uintptr_t)view, 0);
}

// A sampler reading texels as they are (nearest, clamped to the edge, or
// `linear` with repeating edges when asked). A VkResult.
int32_t adm_vk_create_sampler(adm_vk_handle device, uint32_t linear, uint32_t repeat, adm_vk_handle* out) {
    VkSamplerCreateInfo info = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    info.magFilter = linear ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    info.minFilter = info.magFilter;
    info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    info.addressModeU = repeat ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeV = info.addressModeU;
    info.addressModeW = info.addressModeU;
    info.maxLod = 0.0f;
    VkSampler s = 0;
    VkResult r = adm_vk.createSampler((VkDevice)(uintptr_t)device, &info, 0, &s);
    if (r == VK_SUCCESS) out[0] = (adm_vk_handle)(uintptr_t)s;
    return r;
}

void adm_vk_destroy_sampler(adm_vk_handle device, adm_vk_handle sampler) {
    if (sampler) adm_vk.destroySampler((VkDevice)(uintptr_t)device, (VkSampler)(uintptr_t)sampler, 0);
}

// A descriptor set layout of `n` bindings, 0 to n - 1: `types[i]` a
// VkDescriptorType, `stages[i]` the VkShaderStageFlags that read it. Then a
// pipeline layout of that one set: out[0] the set layout, out[1] the pipeline
// layout. A VkResult.
int32_t adm_vk_create_layouts(adm_vk_handle device, const uint32_t* types, const uint32_t* stages, uint32_t n, adm_vk_handle* out) {
    VkDevice dev = (VkDevice)(uintptr_t)device;
    VkDescriptorSetLayoutBinding* bindings = (VkDescriptorSetLayoutBinding*)calloc(n ? n : 1, sizeof *bindings);
    if (!bindings) return VK_ERROR_OUT_OF_HOST_MEMORY;
    for (uint32_t i = 0; i < n; i++) {
        bindings[i].binding = i;
        bindings[i].descriptorType = (VkDescriptorType)types[i];
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = stages[i];
    }
    VkDescriptorSetLayoutCreateInfo info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    info.bindingCount = n;
    info.pBindings = bindings;
    VkDescriptorSetLayout layout = 0;
    VkResult r = adm_vk.createDescriptorSetLayout(dev, &info, 0, &layout);
    free(bindings);
    if (r != VK_SUCCESS) return r;
    VkPipelineLayoutCreateInfo pl = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pl.setLayoutCount = 1;
    pl.pSetLayouts = &layout;
    VkPipelineLayout pipelineLayout = 0;
    r = adm_vk.createPipelineLayout(dev, &pl, 0, &pipelineLayout);
    if (r != VK_SUCCESS) {
        adm_vk.destroyDescriptorSetLayout(dev, layout, 0);
        return r;
    }
    out[0] = (adm_vk_handle)(uintptr_t)layout;
    out[1] = (adm_vk_handle)(uintptr_t)pipelineLayout;
    return VK_SUCCESS;
}

void adm_vk_destroy_layouts(adm_vk_handle device, adm_vk_handle setLayout, adm_vk_handle pipelineLayout) {
    VkDevice dev = (VkDevice)(uintptr_t)device;
    if (pipelineLayout) adm_vk.destroyPipelineLayout(dev, (VkPipelineLayout)(uintptr_t)pipelineLayout, 0);
    if (setLayout) adm_vk.destroyDescriptorSetLayout(dev, (VkDescriptorSetLayout)(uintptr_t)setLayout, 0);
}

// A pool for `sets` descriptor sets holding up to `uniforms` uniform buffers
// and `samplers` combined image samplers in all. A VkResult.
int32_t adm_vk_create_descriptor_pool(adm_vk_handle device, uint32_t sets, uint32_t uniforms, uint32_t samplers, adm_vk_handle* out) {
    VkDescriptorPoolSize sizes[3];
    sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    sizes[0].descriptorCount = uniforms ? uniforms : 1;
    sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sizes[1].descriptorCount = samplers ? samplers : 1;
    sizes[2].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    sizes[2].descriptorCount = sets;
    VkDescriptorPoolCreateInfo info = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    info.maxSets = sets;
    info.poolSizeCount = 3;
    info.pPoolSizes = sizes;
    VkDescriptorPool pool = 0;
    VkResult r = adm_vk.createDescriptorPool((VkDevice)(uintptr_t)device, &info, 0, &pool);
    if (r == VK_SUCCESS) out[0] = (adm_vk_handle)(uintptr_t)pool;
    return r;
}

void adm_vk_destroy_descriptor_pool(adm_vk_handle device, adm_vk_handle pool) {
    if (pool) adm_vk.destroyDescriptorPool((VkDevice)(uintptr_t)device, (VkDescriptorPool)(uintptr_t)pool, 0);
}

int32_t adm_vk_reset_descriptor_pool(adm_vk_handle device, adm_vk_handle pool) {
    return adm_vk.resetDescriptorPool((VkDevice)(uintptr_t)device, (VkDescriptorPool)(uintptr_t)pool, 0);
}

// One descriptor set of the layout from the pool into out[0]. A VkResult
// (VK_ERROR_OUT_OF_POOL_MEMORY when the pool is used up).
int32_t adm_vk_allocate_set(adm_vk_handle device, adm_vk_handle pool, adm_vk_handle layout, adm_vk_handle* out) {
    VkDescriptorSetLayout l = (VkDescriptorSetLayout)(uintptr_t)layout;
    VkDescriptorSetAllocateInfo info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    info.descriptorPool = (VkDescriptorPool)(uintptr_t)pool;
    info.descriptorSetCount = 1;
    info.pSetLayouts = &l;
    VkDescriptorSet set = 0;
    VkResult r = adm_vk.allocateDescriptorSets((VkDevice)(uintptr_t)device, &info, &set);
    if (r == VK_SUCCESS) out[0] = (adm_vk_handle)(uintptr_t)set;
    return r;
}

// Writes a set: `uniforms` uniform buffers at bindings 0.. (buffers[i] from
// offsets[i], ranges[i] bytes), then `images` images at the bindings after
// them (views[i] with sampler samplers[i]): the first `inputs` of them input
// attachments (their sampler unused), the rest combined image samplers.
void adm_vk_write_set(adm_vk_handle device, adm_vk_handle set, const adm_vk_handle* buffers, const uint64_t* offsets, const uint64_t* ranges, uint32_t uniforms, const adm_vk_handle* views, const adm_vk_handle* samplers, uint32_t images, uint32_t inputs) {
    uint32_t n = uniforms + images;
    VkWriteDescriptorSet* writes = (VkWriteDescriptorSet*)calloc(n ? n : 1, sizeof *writes);
    VkDescriptorBufferInfo* bufs = (VkDescriptorBufferInfo*)calloc(uniforms ? uniforms : 1, sizeof *bufs);
    VkDescriptorImageInfo* imgs = (VkDescriptorImageInfo*)calloc(images ? images : 1, sizeof *imgs);
    if (!writes || !bufs || !imgs) {
        free(writes);
        free(bufs);
        free(imgs);
        return;
    }
    for (uint32_t i = 0; i < uniforms; i++) {
        bufs[i].buffer = (VkBuffer)(uintptr_t)buffers[i];
        bufs[i].offset = offsets[i];
        bufs[i].range = ranges[i];
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = (VkDescriptorSet)(uintptr_t)set;
        writes[i].dstBinding = i;
        writes[i].descriptorCount = 1;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[i].pBufferInfo = &bufs[i];
    }
    for (uint32_t i = 0; i < images; i++) {
        imgs[i].sampler = (VkSampler)(uintptr_t)samplers[i];
        imgs[i].imageView = (VkImageView)(uintptr_t)views[i];
        imgs[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        VkWriteDescriptorSet* w = &writes[uniforms + i];
        w->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        w->dstSet = (VkDescriptorSet)(uintptr_t)set;
        w->dstBinding = uniforms + i;
        w->descriptorCount = 1;
        w->descriptorType = i < inputs ? VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT : VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        if (i < inputs) imgs[i].sampler = 0;
        w->pImageInfo = &imgs[i];
    }
    adm_vk.updateDescriptorSets((VkDevice)(uintptr_t)device, n, writes, 0, 0);
    free(writes);
    free(bufs);
    free(imgs);
}

// A shader module from `size` bytes of SPIR-V (a multiple of 4). A VkResult.
int32_t adm_vk_create_shader(adm_vk_handle device, const void* code, uint64_t size, adm_vk_handle* out) {
    VkShaderModuleCreateInfo info = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    info.codeSize = (size_t)size;
    info.pCode = (const uint32_t*)code;
    VkShaderModule m = 0;
    VkResult r = adm_vk.createShaderModule((VkDevice)(uintptr_t)device, &info, 0, &m);
    if (r == VK_SUCCESS) out[0] = (adm_vk_handle)(uintptr_t)m;
    return r;
}

void adm_vk_destroy_shader(adm_vk_handle device, adm_vk_handle module) {
    if (module) adm_vk.destroyShaderModule((VkDevice)(uintptr_t)device, (VkShaderModule)(uintptr_t)module, 0);
}

// A graphics pipeline drawing triangles from the vertex and fragment modules
// (entry points `main`) with no vertex input, into one colour attachment of
// `format` through dynamic rendering, viewport and scissor dynamic, no
// blending, no depth; with `localRead` the fragment stage reads that
// attachment as input attachment 0. A VkResult.
int32_t adm_vk_create_pipeline(adm_vk_handle device, adm_vk_handle layout, adm_vk_handle vertex, adm_vk_handle fragment, uint32_t format, uint32_t localRead, adm_vk_handle* out) {
    VkPipelineShaderStageCreateInfo stages[2] = {{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO}, {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO}};
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = (VkShaderModule)(uintptr_t)vertex;
    stages[0].pName = "main";
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = (VkShaderModule)(uintptr_t)fragment;
    stages[1].pName = "main";
    VkPipelineVertexInputStateCreateInfo vin = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    VkPipelineInputAssemblyStateCreateInfo ia = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPipelineViewportStateCreateInfo vp = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    vp.viewportCount = 1;
    vp.scissorCount = 1;
    VkPipelineRasterizationStateCreateInfo rs = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_NONE;
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth = 1.0f;
    VkPipelineMultisampleStateCreateInfo ms = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineColorBlendAttachmentState att = {0};
    att.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo cb = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    cb.attachmentCount = 1;
    cb.pAttachments = &att;
    VkDynamicState dynamics[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dyn = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dyn.dynamicStateCount = 2;
    dyn.pDynamicStates = dynamics;
    VkFormat colorFormat = (VkFormat)format;
    VkPipelineRenderingCreateInfo rendering = {VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    rendering.colorAttachmentCount = 1;
    rendering.pColorAttachmentFormats = &colorFormat;
    uint32_t inputIndex = 0;
    VkRenderingInputAttachmentIndexInfoKHR inputs = {VK_STRUCTURE_TYPE_RENDERING_INPUT_ATTACHMENT_INDEX_INFO_KHR};
    inputs.colorAttachmentCount = 1;
    inputs.pColorAttachmentInputIndices = &inputIndex;
    if (localRead) rendering.pNext = &inputs;
    VkGraphicsPipelineCreateInfo info = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    info.pNext = &rendering;
    info.stageCount = 2;
    info.pStages = stages;
    info.pVertexInputState = &vin;
    info.pInputAssemblyState = &ia;
    info.pViewportState = &vp;
    info.pRasterizationState = &rs;
    info.pMultisampleState = &ms;
    info.pColorBlendState = &cb;
    info.pDynamicState = &dyn;
    info.layout = (VkPipelineLayout)(uintptr_t)layout;
    VkPipeline p = 0;
    VkResult r = adm_vk.createGraphicsPipelines((VkDevice)(uintptr_t)device, 0, 1, &info, 0, &p);
    if (r == VK_SUCCESS) out[0] = (adm_vk_handle)(uintptr_t)p;
    return r;
}

void adm_vk_destroy_pipeline(adm_vk_handle device, adm_vk_handle pipeline) {
    if (pipeline) adm_vk.destroyPipeline((VkDevice)(uintptr_t)device, (VkPipeline)(uintptr_t)pipeline, 0);
}

// A command pool on the queue family with one primary command buffer and a
// fence (unsignalled): out[0] the pool, out[1] the buffer, out[2] the fence.
// A VkResult.
int32_t adm_vk_create_commands(adm_vk_handle device, uint32_t family, adm_vk_handle* out) {
    VkDevice dev = (VkDevice)(uintptr_t)device;
    VkCommandPoolCreateInfo info = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    info.queueFamilyIndex = family;
    VkCommandPool pool = 0;
    VkResult r = adm_vk.createCommandPool(dev, &info, 0, &pool);
    if (r != VK_SUCCESS) return r;
    VkCommandBufferAllocateInfo alloc = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    alloc.commandPool = pool;
    alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc.commandBufferCount = 1;
    VkCommandBuffer cmd = 0;
    r = adm_vk.allocateCommandBuffers(dev, &alloc, &cmd);
    if (r != VK_SUCCESS) {
        adm_vk.destroyCommandPool(dev, pool, 0);
        return r;
    }
    VkFenceCreateInfo fi = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    VkFence fence = 0;
    r = adm_vk.createFence(dev, &fi, 0, &fence);
    if (r != VK_SUCCESS) {
        adm_vk.destroyCommandPool(dev, pool, 0);
        return r;
    }
    out[0] = (adm_vk_handle)(uintptr_t)pool;
    out[1] = (adm_vk_handle)(uintptr_t)cmd;
    out[2] = (adm_vk_handle)(uintptr_t)fence;
    return VK_SUCCESS;
}

void adm_vk_destroy_commands(adm_vk_handle device, adm_vk_handle pool, adm_vk_handle fence) {
    VkDevice dev = (VkDevice)(uintptr_t)device;
    if (fence) adm_vk.destroyFence(dev, (VkFence)(uintptr_t)fence, 0);
    if (pool) adm_vk.destroyCommandPool(dev, (VkCommandPool)(uintptr_t)pool, 0);
}

// Resets the pool (its command buffer with it) and begins the buffer for one
// submission. A VkResult.
int32_t adm_vk_begin_commands(adm_vk_handle device, adm_vk_handle pool, adm_vk_handle cmd) {
    VkResult r = adm_vk.resetCommandPool((VkDevice)(uintptr_t)device, (VkCommandPool)(uintptr_t)pool, 0);
    if (r != VK_SUCCESS) return r;
    VkCommandBufferBeginInfo info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    return adm_vk.beginCommandBuffer((VkCommandBuffer)(uintptr_t)cmd, &info);
}

int32_t adm_vk_end_commands(adm_vk_handle cmd) {
    return adm_vk.endCommandBuffer((VkCommandBuffer)(uintptr_t)cmd);
}

// Submits the command buffer: after `wait` (a semaphore, or 0) is signalled,
// signalling `signal` (or 0) and `fence` (or 0) when done. A VkResult.
int32_t adm_vk_submit(adm_vk_handle queue, adm_vk_handle cmd, adm_vk_handle wait, adm_vk_handle signal, adm_vk_handle fence) {
    VkCommandBuffer buffer = (VkCommandBuffer)(uintptr_t)cmd;
    VkSemaphore waitSem = (VkSemaphore)(uintptr_t)wait;
    VkSemaphore signalSem = (VkSemaphore)(uintptr_t)signal;
    VkPipelineStageFlags stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    info.waitSemaphoreCount = wait ? 1 : 0;
    info.pWaitSemaphores = &waitSem;
    info.pWaitDstStageMask = &stage;
    info.commandBufferCount = 1;
    info.pCommandBuffers = &buffer;
    info.signalSemaphoreCount = signal ? 1 : 0;
    info.pSignalSemaphores = &signalSem;
    return adm_vk.queueSubmit((VkQueue)(uintptr_t)queue, 1, &info, (VkFence)(uintptr_t)fence);
}

// Waits for the fence (at most `timeout` nanoseconds) and resets it. A VkResult.
int32_t adm_vk_wait_fence(adm_vk_handle device, adm_vk_handle fence, uint64_t timeout) {
    VkDevice dev = (VkDevice)(uintptr_t)device;
    VkFence f = (VkFence)(uintptr_t)fence;
    VkResult r = adm_vk.waitForFences(dev, 1, &f, VK_TRUE, timeout);
    if (r != VK_SUCCESS) return r;
    return adm_vk.resetFences(dev, 1, &f);
}

int32_t adm_vk_create_semaphore(adm_vk_handle device, adm_vk_handle* out) {
    VkSemaphoreCreateInfo info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkSemaphore s = 0;
    VkResult r = adm_vk.createSemaphore((VkDevice)(uintptr_t)device, &info, 0, &s);
    if (r == VK_SUCCESS) out[0] = (adm_vk_handle)(uintptr_t)s;
    return r;
}

void adm_vk_destroy_semaphore(adm_vk_handle device, adm_vk_handle semaphore) {
    if (semaphore) adm_vk.destroySemaphore((VkDevice)(uintptr_t)device, (VkSemaphore)(uintptr_t)semaphore, 0);
}

// A barrier making every write so far visible to every later read and write
// (all stages, all accesses): between an upload and the draw that reads it,
// between a draw and a copy out of its target. Not inside rendering.
void adm_vk_cmd_barrier(adm_vk_handle cmd) {
    VkMemoryBarrier2 b = {VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
    b.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    b.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;
    b.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    b.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;
    VkDependencyInfo dep = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dep.memoryBarrierCount = 1;
    dep.pMemoryBarriers = &b;
    adm_vk.cmdPipelineBarrier2((VkCommandBuffer)(uintptr_t)cmd, &dep);
}

// Inside a render pass instance reading its colour attachment back: what the
// draws so far wrote to a pixel is seen by the draws after, pixel by pixel.
void adm_vk_cmd_local_barrier(adm_vk_handle cmd) {
    VkMemoryBarrier2 b = {VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
    b.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    b.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    b.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    b.dstAccessMask = VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT;
    VkDependencyInfo dep = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dep.memoryBarrierCount = 1;
    dep.pMemoryBarriers = &b;
    adm_vk.cmdPipelineBarrier2((VkCommandBuffer)(uintptr_t)cmd, &dep);
}

// Moves the image from one layout to another, after everything before and
// before everything after.
void adm_vk_cmd_layout(adm_vk_handle cmd, adm_vk_handle image, uint32_t oldLayout, uint32_t newLayout) {
    VkImageMemoryBarrier2 b = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    b.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    b.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;
    b.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    b.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;
    b.oldLayout = (VkImageLayout)oldLayout;
    b.newLayout = (VkImageLayout)newLayout;
    b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = (VkImage)(uintptr_t)image;
    b.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    b.subresourceRange.levelCount = 1;
    b.subresourceRange.layerCount = 1;
    VkDependencyInfo dep = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dep.imageMemoryBarrierCount = 1;
    dep.pImageMemoryBarriers = &b;
    adm_vk.cmdPipelineBarrier2((VkCommandBuffer)(uintptr_t)cmd, &dep);
}

// Copies w x h texels from the buffer at `offset` (rows `rowLength` texels
// apart, 0 for w) into the image at (x, y). The image is in the general layout.
void adm_vk_cmd_copy_to_image(adm_vk_handle cmd, adm_vk_handle buffer, uint64_t offset, uint32_t rowLength, adm_vk_handle image, int32_t x, int32_t y, uint32_t w, uint32_t h) {
    VkBufferImageCopy region = {0};
    region.bufferOffset = offset;
    region.bufferRowLength = rowLength;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageOffset.x = x;
    region.imageOffset.y = y;
    region.imageExtent.width = w;
    region.imageExtent.height = h;
    region.imageExtent.depth = 1;
    adm_vk.cmdCopyBufferToImage((VkCommandBuffer)(uintptr_t)cmd, (VkBuffer)(uintptr_t)buffer, (VkImage)(uintptr_t)image, VK_IMAGE_LAYOUT_GENERAL, 1, &region);
}

// Copies the w x h box of the image at (x, y) into the buffer at `offset`,
// rows packed. The image is in the general layout.
void adm_vk_cmd_copy_to_buffer(adm_vk_handle cmd, adm_vk_handle image, int32_t x, int32_t y, uint32_t w, uint32_t h, adm_vk_handle buffer, uint64_t offset) {
    VkBufferImageCopy region = {0};
    region.bufferOffset = offset;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageOffset.x = x;
    region.imageOffset.y = y;
    region.imageExtent.width = w;
    region.imageExtent.height = h;
    region.imageExtent.depth = 1;
    adm_vk.cmdCopyImageToBuffer((VkCommandBuffer)(uintptr_t)cmd, (VkImage)(uintptr_t)image, VK_IMAGE_LAYOUT_GENERAL, (VkBuffer)(uintptr_t)buffer, 1, &region);
}

// Copies the w x h box at (sx, sy) of one image to (dx, dy) of another (or
// the same, where the boxes do not overlap), both in the general layout.
void adm_vk_cmd_copy_image(adm_vk_handle cmd, adm_vk_handle src, int32_t sx, int32_t sy, adm_vk_handle dst, int32_t dx, int32_t dy, uint32_t w, uint32_t h) {
    VkImageCopy region = {0};
    region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.srcSubresource.layerCount = 1;
    region.srcOffset.x = sx;
    region.srcOffset.y = sy;
    region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.dstSubresource.layerCount = 1;
    region.dstOffset.x = dx;
    region.dstOffset.y = dy;
    region.extent.width = w;
    region.extent.height = h;
    region.extent.depth = 1;
    adm_vk.cmdCopyImage((VkCommandBuffer)(uintptr_t)cmd, (VkImage)(uintptr_t)src, VK_IMAGE_LAYOUT_GENERAL, (VkImage)(uintptr_t)dst, VK_IMAGE_LAYOUT_GENERAL, 1, &region);
}

// Fills the whole image (general layout) with the colour.
void adm_vk_cmd_clear(adm_vk_handle cmd, adm_vk_handle image, float r, float g, float b, float a) {
    VkClearColorValue color;
    color.float32[0] = r;
    color.float32[1] = g;
    color.float32[2] = b;
    color.float32[3] = a;
    VkImageSubresourceRange range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    adm_vk.cmdClearColorImage((VkCommandBuffer)(uintptr_t)cmd, (VkImage)(uintptr_t)image, VK_IMAGE_LAYOUT_GENERAL, &color, 1, &range);
}

// Begins rendering into the view (an image in `layout`) over the w x h area
// at (x, y), keeping what the attachment holds (or clearing it to the colour
// when `clear`).
void adm_vk_cmd_begin_rendering(adm_vk_handle cmd, adm_vk_handle view, uint32_t layout, int32_t x, int32_t y, uint32_t w, uint32_t h, uint32_t clear, float r, float g, float b, float a) {
    VkRenderingAttachmentInfo att = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    att.imageView = (VkImageView)(uintptr_t)view;
    att.imageLayout = (VkImageLayout)layout;
    att.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    att.clearValue.color.float32[0] = r;
    att.clearValue.color.float32[1] = g;
    att.clearValue.color.float32[2] = b;
    att.clearValue.color.float32[3] = a;
    VkRenderingInfo info = {VK_STRUCTURE_TYPE_RENDERING_INFO};
    info.renderArea.offset.x = x;
    info.renderArea.offset.y = y;
    info.renderArea.extent.width = w;
    info.renderArea.extent.height = h;
    info.layerCount = 1;
    info.colorAttachmentCount = 1;
    info.pColorAttachments = &att;
    adm_vk.cmdBeginRendering((VkCommandBuffer)(uintptr_t)cmd, &info);
}

void adm_vk_cmd_end_rendering(adm_vk_handle cmd) {
    adm_vk.cmdEndRendering((VkCommandBuffer)(uintptr_t)cmd);
}

// Binds the pipeline and its one descriptor set.
void adm_vk_cmd_bind(adm_vk_handle cmd, adm_vk_handle pipeline, adm_vk_handle layout, adm_vk_handle set) {
    VkCommandBuffer c = (VkCommandBuffer)(uintptr_t)cmd;
    adm_vk.cmdBindPipeline(c, VK_PIPELINE_BIND_POINT_GRAPHICS, (VkPipeline)(uintptr_t)pipeline);
    VkDescriptorSet s = (VkDescriptorSet)(uintptr_t)set;
    adm_vk.cmdBindDescriptorSets(c, VK_PIPELINE_BIND_POINT_GRAPHICS, (VkPipelineLayout)(uintptr_t)layout, 0, 1, &s, 0, 0);
}

// Sets the viewport (vw x vh at the origin, y down as the image's rows run)
// and the scissor box.
void adm_vk_cmd_viewport_scissor(adm_vk_handle cmd, uint32_t vw, uint32_t vh, int32_t sx, int32_t sy, uint32_t sw, uint32_t sh) {
    VkCommandBuffer c = (VkCommandBuffer)(uintptr_t)cmd;
    VkViewport vp = {0.0f, 0.0f, (float)vw, (float)vh, 0.0f, 1.0f};
    adm_vk.cmdSetViewport(c, 0, 1, &vp);
    VkRect2D sc;
    sc.offset.x = sx;
    sc.offset.y = sy;
    sc.extent.width = sw;
    sc.extent.height = sh;
    adm_vk.cmdSetScissor(c, 0, 1, &sc);
}

// Draws `vertices` vertices once.
void adm_vk_cmd_draw(adm_vk_handle cmd, uint32_t vertices) {
    adm_vk.cmdDraw((VkCommandBuffer)(uintptr_t)cmd, vertices, 1, 0, 0);
}

// What the surface allows: out[0] the least and out[1] the most swapchain
// images (0: no limit), out[2] and out[3] its current size (0xFFFFFFFF: the
// window's), out[4] the composite alpha modes (VkCompositeAlphaFlagsKHR),
// out[5] the present modes as bits (1 << VkPresentModeKHR), out[6] the
// format chosen (VkFormat: B8G8R8A8_UNORM or R8G8B8A8_UNORM when offered,
// else the first) and out[7] its colour space. A VkResult.
int32_t adm_vk_surface_info(adm_vk_handle physical, adm_vk_handle surface, uint32_t* out) {
    VkPhysicalDevice dev = (VkPhysicalDevice)(uintptr_t)physical;
    VkSurfaceKHR surf = (VkSurfaceKHR)(uintptr_t)surface;
    VkSurfaceCapabilitiesKHR caps;
    VkResult r = adm_vk.getPhysicalDeviceSurfaceCapabilitiesKHR(dev, surf, &caps);
    if (r != VK_SUCCESS) return r;
    out[0] = caps.minImageCount;
    out[1] = caps.maxImageCount;
    out[2] = caps.currentExtent.width;
    out[3] = caps.currentExtent.height;
    out[4] = caps.supportedCompositeAlpha;
    uint32_t n = 0;
    out[5] = 0;
    if (adm_vk.getPhysicalDeviceSurfacePresentModesKHR(dev, surf, &n, 0) == VK_SUCCESS && n > 0) {
        VkPresentModeKHR* modes = (VkPresentModeKHR*)calloc(n, sizeof *modes);
        if (modes && adm_vk.getPhysicalDeviceSurfacePresentModesKHR(dev, surf, &n, modes) == VK_SUCCESS) {
            for (uint32_t i = 0; i < n; i++) {
                if ((uint32_t)modes[i] < 32) out[5] |= 1u << (uint32_t)modes[i];
            }
        }
        free(modes);
    }
    n = 0;
    out[6] = VK_FORMAT_B8G8R8A8_UNORM;
    out[7] = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    if (adm_vk.getPhysicalDeviceSurfaceFormatsKHR(dev, surf, &n, 0) == VK_SUCCESS && n > 0) {
        VkSurfaceFormatKHR* formats = (VkSurfaceFormatKHR*)calloc(n, sizeof *formats);
        if (formats && adm_vk.getPhysicalDeviceSurfaceFormatsKHR(dev, surf, &n, formats) == VK_SUCCESS) {
            int32_t pick = -1;
            for (uint32_t i = 0; i < n && pick < 0; i++) {
                if (formats[i].format == VK_FORMAT_B8G8R8A8_UNORM || formats[i].format == VK_FORMAT_R8G8B8A8_UNORM) pick = (int32_t)i;
            }
            if (pick < 0) pick = 0;
            out[6] = (uint32_t)formats[pick].format;
            out[7] = (uint32_t)formats[pick].colorSpace;
        }
        free(formats);
    }
    return VK_SUCCESS;
}

// A swapchain of at least `minImages` images of w x h in the format and
// colour space, presented in `presentMode` with `compositeAlpha`, usable as
// colour attachments and copy targets, replacing `old` (or 0). A VkResult.
int32_t adm_vk_create_swapchain(adm_vk_handle device, adm_vk_handle surface, uint32_t w, uint32_t h, uint32_t format, uint32_t colorSpace, uint32_t presentMode, uint32_t compositeAlpha, uint32_t minImages, adm_vk_handle old, adm_vk_handle* out) {
    VkSwapchainCreateInfoKHR info = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    info.surface = (VkSurfaceKHR)(uintptr_t)surface;
    info.minImageCount = minImages;
    info.imageFormat = (VkFormat)format;
    info.imageColorSpace = (VkColorSpaceKHR)colorSpace;
    info.imageExtent.width = w;
    info.imageExtent.height = h;
    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    info.compositeAlpha = (VkCompositeAlphaFlagBitsKHR)compositeAlpha;
    info.presentMode = (VkPresentModeKHR)presentMode;
    info.clipped = VK_TRUE;
    info.oldSwapchain = (VkSwapchainKHR)(uintptr_t)old;
    VkSwapchainKHR sc = 0;
    VkResult r = adm_vk.createSwapchainKHR((VkDevice)(uintptr_t)device, &info, 0, &sc);
    if (r == VK_SUCCESS) out[0] = (adm_vk_handle)(uintptr_t)sc;
    return r;
}

void adm_vk_destroy_swapchain(adm_vk_handle device, adm_vk_handle swapchain) {
    if (swapchain) adm_vk.destroySwapchainKHR((VkDevice)(uintptr_t)device, (VkSwapchainKHR)(uintptr_t)swapchain, 0);
}

// The swapchain's images, up to `max` into out; how many there are.
uint32_t adm_vk_swapchain_images(adm_vk_handle device, adm_vk_handle swapchain, adm_vk_handle* out, uint32_t max) {
    uint32_t n = 0;
    VkDevice dev = (VkDevice)(uintptr_t)device;
    VkSwapchainKHR sc = (VkSwapchainKHR)(uintptr_t)swapchain;
    if (adm_vk.getSwapchainImagesKHR(dev, sc, &n, 0) != VK_SUCCESS || n == 0) return 0;
    VkImage* images = (VkImage*)calloc(n, sizeof *images);
    if (!images) return 0;
    adm_vk.getSwapchainImagesKHR(dev, sc, &n, images);
    for (uint32_t i = 0; i < n && i < max; i++) out[i] = (adm_vk_handle)(uintptr_t)images[i];
    free(images);
    return n;
}

// Takes the next image of the swapchain, signalling the semaphore when it
// may be drawn into: its index into out[0]. A VkResult (VK_SUBOPTIMAL_KHR and
// VK_ERROR_OUT_OF_DATE_KHR say the swapchain no longer fits the window).
int32_t adm_vk_acquire(adm_vk_handle device, adm_vk_handle swapchain, adm_vk_handle semaphore, uint32_t* out) {
    return adm_vk.acquireNextImageKHR((VkDevice)(uintptr_t)device, (VkSwapchainKHR)(uintptr_t)swapchain, UINT64_MAX, (VkSemaphore)(uintptr_t)semaphore, 0, out);
}

// Presents the swapchain's image once the semaphore is signalled. A VkResult.
int32_t adm_vk_present(adm_vk_handle queue, adm_vk_handle swapchain, uint32_t index, adm_vk_handle semaphore) {
    VkSwapchainKHR sc = (VkSwapchainKHR)(uintptr_t)swapchain;
    VkSemaphore sem = (VkSemaphore)(uintptr_t)semaphore;
    VkPresentInfoKHR info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    info.waitSemaphoreCount = semaphore ? 1 : 0;
    info.pWaitSemaphores = &sem;
    info.swapchainCount = 1;
    info.pSwapchains = &sc;
    info.pImageIndices = &index;
    return adm_vk.queuePresentKHR((VkQueue)(uintptr_t)queue, &info);
}

int32_t adm_vk_queue_wait_idle(adm_vk_handle queue) {
    return adm_vk.queueWaitIdle((VkQueue)(uintptr_t)queue);
}

#endif
