/*
 * Copyright 2018 GoPro Inc.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <string.h>
#include <inttypes.h>
#include <stdlib.h>

#if defined(TARGET_LINUX)
# define VK_USE_PLATFORM_XLIB_KHR
#elif defined(TARGET_ANDROID)
# define VK_USE_PLATFORM_ANDROID_KHR
#elif defined(TARGET_DARWIN)
# define VK_USE_PLATFORM_MACOS_MVK
#elif defined(TARGET_IPHONE)
# define VK_USE_PLATFORM_IOS_MVK
#elif defined(TARGET_MINGW_W64)
// TODO
# define VK_USE_PLATFORM_WIN32_KHR
#endif

#if defined(TARGET_DARWIN) || defined(TARGET_IPHONE)
#define USE_MOLTENVK 1
#else
#define USE_MOLTENVK 0
#endif

#include <vulkan/vulkan.h>

#include "nodes.h"
#include "vkcontext.h"
#include "gctx_vk.h"
#include "log.h"
#include "memory.h"
#include "pgcache.h"

/* FIXME: missing includes probably */
#include "buffer_vk.h"
#include "texture_vk.h"
#include "rendertarget_vk.h"
#include "program_vk.h"
#include "pipeline_vk.h"
#include "gtimer_vk.h"

#if USE_MOLTENVK || defined(TARGET_ANDROID)
#define ENABLE_DEBUG 0
#else
#define ENABLE_DEBUG 1
#endif

#include <limits.h>

static const VkApplicationInfo app_info = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pEngineName = "node.gl",
    .engineVersion = NODEGL_VERSION_INT,
#if !defined(VK_API_VERSION_1_1) || USE_MOLTENVK
    .apiVersion = VK_API_VERSION_1_0,
#else
    .apiVersion = VK_API_VERSION_1_1,
#endif
};

static const char *my_device_extension_names[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

static const char *vk_res2str(VkResult res)
{
    switch (res) {
        case VK_SUCCESS:                        return "sucess";
        case VK_NOT_READY:                      return "not ready";
        case VK_TIMEOUT:                        return "timeout";
        case VK_EVENT_SET:                      return "event set";
        case VK_EVENT_RESET:                    return "event reset";
        case VK_INCOMPLETE:                     return "incomplete";
        case VK_ERROR_OUT_OF_HOST_MEMORY:       return "out of host memory";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:     return "out of device memory";
        case VK_ERROR_INITIALIZATION_FAILED:    return "initialization failed";
        case VK_ERROR_DEVICE_LOST:              return "device lost";
        case VK_ERROR_MEMORY_MAP_FAILED:        return "memory map failed";
        case VK_ERROR_LAYER_NOT_PRESENT:        return "layer not present";
        case VK_ERROR_EXTENSION_NOT_PRESENT:    return "extension not present";
        case VK_ERROR_FEATURE_NOT_PRESENT:      return "feature not present";
        case VK_ERROR_INCOMPATIBLE_DRIVER:      return "incompatible driver";
        case VK_ERROR_TOO_MANY_OBJECTS:         return "too many objects";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:     return "format not supported";
        case VK_ERROR_FRAGMENTED_POOL:          return "fragmented pool";
#ifdef VK_ERROR_OUT_OF_POOL_MEMORY
        case VK_ERROR_OUT_OF_POOL_MEMORY:       return "out of pool memory";
#endif
#ifdef VK_ERROR_INVALID_EXTERNAL_HANDLE
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:  return "invalid external handle";
#endif
        case VK_ERROR_SURFACE_LOST_KHR:         return "surface lost (KHR)";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "native window in use (KHR)";
        case VK_SUBOPTIMAL_KHR:                 return "suboptimal (KHR)";
        case VK_ERROR_OUT_OF_DATE_KHR:          return "out of date (KHR)";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "incompatible display (KHR)";
        case VK_ERROR_VALIDATION_FAILED_EXT:    return "validation failed ext";
        case VK_ERROR_INVALID_SHADER_NV:        return "invalid shader nv";
#ifdef VK_ERROR_FRAGMENTATION_EXT
        case VK_ERROR_FRAGMENTATION_EXT:        return "fragmentation ext";
#endif
#ifdef VK_ERROR_NOT_PERMITTED_EXT
        case VK_ERROR_NOT_PERMITTED_EXT:        return "not permitted ext";
#endif
        default:                                return "unknown";
    }
}

#if ENABLE_DEBUG
static const char *my_layers[] = {
    "VK_LAYER_KHRONOS_validation",
};

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                     VkDebugUtilsMessageTypeFlagsEXT type,
                                                     const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
                                                     void *user_data)
{
    int log_level = NGL_LOG_INFO;
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        log_level = NGL_LOG_WARNING;
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        log_level = NGL_LOG_ERROR;

    const char *msg_type = "GENERAL";
    if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
        msg_type = "VALIDATION";
    if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
        msg_type = "PERFORMANCE";

    ngli_log_print(log_level, __FILE__, __LINE__, "debug_callback", "%s: %s", msg_type, callback_data->pMessage);
    return VK_FALSE;
}
#endif

static VkSurfaceFormatKHR select_swapchain_surface_format(const VkSurfaceFormatKHR *formats,
                                                          uint32_t nb_formats)
{
    VkSurfaceFormatKHR target_fmt = {
        .format = VK_FORMAT_B8G8R8A8_UNORM,
        .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
    };
    if (nb_formats == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
        return target_fmt;
    for (uint32_t i = 0; i < nb_formats; i++)
        if (formats[i].format == target_fmt.format &&
            formats[i].colorSpace == target_fmt.colorSpace)
            return formats[i];
    return formats[0];
}

static VkPresentModeKHR select_swapchain_present_mode(const VkPresentModeKHR *present_modes,
                                                      uint32_t nb_present_modes)
{
    VkPresentModeKHR target_mode = VK_PRESENT_MODE_FIFO_KHR;

    for (uint32_t i = 0; i < nb_present_modes; i++) {

        /* triple buffering, best mode possible */
        if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
            return present_modes[i];

        /* some drivers may not actually have VK_PRESENT_MODE_FIFO_KHR */
        if (present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
            target_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    }
    return target_mode;
}

static uint32_t clip_u32(uint32_t x, uint32_t min, uint32_t max)
{
    if (x < min)
        return min;
    if (x > max)
        return max;
    return x;
}

static VkExtent2D select_swapchain_current_extent(const struct vkcontext *vk,
                                                  VkSurfaceCapabilitiesKHR caps)
{
    // XXX: we want to rely on value passed by the user (set to vk->width, vk->height in vk_resize()
    // so the viewport and scissor matches the extent
#if 0
    if (caps.currentExtent.width != UINT32_MAX) {
        LOG(DEBUG, "current extent: %dx%d", caps.currentExtent.width, caps.currentExtent.height);
        return caps.currentExtent;
    }
#endif

    VkExtent2D ext = {
        .width  = clip_u32(vk->width,  caps.minImageExtent.width,  caps.maxImageExtent.width),
        .height = clip_u32(vk->height, caps.minImageExtent.height, caps.maxImageExtent.height),
    };
    LOG(DEBUG, "swapchain extent %dx%d", ext.width, ext.height);
    return ext;
}

static VkResult query_swapchain_support(struct vk_swapchain_support *swap,
                                        VkSurfaceKHR surface,
                                        VkPhysicalDevice phy_device)
{
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phy_device, surface, &swap->caps);

    ngli_free(swap->formats);
    swap->formats = NULL;
    vkGetPhysicalDeviceSurfaceFormatsKHR(phy_device, surface, &swap->nb_formats, NULL);
    if (swap->nb_formats) {
        swap->formats = ngli_calloc(swap->nb_formats, sizeof(*swap->formats));
        if (!swap->formats)
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        vkGetPhysicalDeviceSurfaceFormatsKHR(phy_device, surface, &swap->nb_formats, swap->formats);
    }

    ngli_free(swap->present_modes);
    swap->present_modes = NULL;
    vkGetPhysicalDeviceSurfacePresentModesKHR(phy_device, surface, &swap->nb_present_modes, NULL);
    if (swap->nb_present_modes) {
        swap->present_modes = ngli_calloc(swap->nb_present_modes, sizeof(*swap->present_modes));
        if (!swap->present_modes)
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        vkGetPhysicalDeviceSurfacePresentModesKHR(phy_device, surface, &swap->nb_present_modes, swap->present_modes);
    }

    return VK_SUCCESS;
}

static VkResult probe_vulkan_extensions(struct gctx *s)
{
    struct gctx_vk *s_priv = (struct gctx_vk *)s;

    uint32_t ext_count = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &ext_count, NULL);
    VkExtensionProperties *ext_props = ngli_calloc(ext_count, sizeof(*ext_props));
    if (!ext_props)
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    vkEnumerateInstanceExtensionProperties(NULL, &ext_count, ext_props);
    LOG(DEBUG, "Vulkan extensions available:");
    for (uint32_t i = 0; i < ext_count; i++) {
        LOG(DEBUG, "  %d/%d: %s v%d", i+1, ext_count,
               ext_props[i].extensionName, ext_props[i].specVersion);
#if defined(VK_USE_PLATFORM_XLIB_KHR)
        if (!strcmp(ext_props[i].extensionName, VK_KHR_XLIB_SURFACE_EXTENSION_NAME))
            s_priv->surface_create_type = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
        if (!strcmp(ext_props[i].extensionName, VK_KHR_ANDROID_SURFACE_EXTENSION_NAME))
            s_priv->surface_create_type = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
        if (!strcmp(ext_props[i].extensionName, VK_MVK_MACOS_SURFACE_EXTENSION_NAME))
            s_priv->surface_create_type = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
#elif defined(VK_USE_PLATFORM_IOS_MVK)
        if (!strcmp(ext_props[i].extensionName, VK_MVK_IOS_SURFACE_EXTENSION_NAME))
            s_priv->surface_create_type = VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_MVK;
#endif
    }
    ngli_free(ext_props);
    return VK_SUCCESS;
}

static VkFormat find_supported_format(struct vkcontext *vk,
                                      const VkFormat *formats,
                                      VkImageTiling tiling,
                                      VkFormatFeatureFlags features)
{
    int i = 0;
    while (formats[i]) {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(vk->physical_device, formats[i], &properties);
        if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)
            return formats[i];
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)
            return formats[i];
        i++;
    }
    return VK_FORMAT_UNDEFINED;
}

static VkResult probe_prefered_depth_stencil_format(struct vkcontext *vk)
{
    const VkFormat formats[] = {
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        0
    };
    const VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
    const VkFormatFeatureFlags features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    vk->prefered_vk_depth_stencil_format = find_supported_format(vk, formats, tiling, features);
    if (!vk->prefered_vk_depth_stencil_format)
        return VK_ERROR_FORMAT_NOT_SUPPORTED;

    switch (vk->prefered_vk_depth_stencil_format) {
    case VK_FORMAT_D24_UNORM_S8_UINT:
        vk->prefered_depth_stencil_format = NGLI_FORMAT_D24_UNORM_S8_UINT;
        break;
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        vk->prefered_depth_stencil_format = NGLI_FORMAT_D32_SFLOAT_S8_UINT;
        break;
    case VK_FORMAT_D32_SFLOAT:
        vk->prefered_depth_stencil_format = NGLI_FORMAT_D32_SFLOAT;
        break;
    default:
        ngli_assert(0);
    };

    return VK_SUCCESS;
}

static VkResult list_vulkan_layers(void)
{
    uint32_t layer_count = 0;
    vkEnumerateInstanceLayerProperties(&layer_count, NULL);
    VkLayerProperties *layer_props = ngli_calloc(layer_count, sizeof(*layer_props));
    if (!layer_props)
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    vkEnumerateInstanceLayerProperties(&layer_count, layer_props);
    LOG(DEBUG, "Vulkan layers available:");
    for (uint32_t i = 0; i < layer_count; i++)
        LOG(DEBUG, "  %d/%d: %s", i+1, layer_count, layer_props[i].layerName);
    ngli_free(layer_props);
    return VK_SUCCESS;
}

static const char *platform_ext_names[] = {
    [NGL_PLATFORM_XLIB]    = "VK_KHR_xlib_surface",
    [NGL_PLATFORM_ANDROID] = "VK_KHR_android_surface",
    [NGL_PLATFORM_MACOS]   = "VK_MVK_macos_surface",
    [NGL_PLATFORM_IOS]     = "VK_MVK_ios_surface",
    [NGL_PLATFORM_WINDOWS] = "VK_KHR_win32_surface",
};

static VkResult create_vulkan_instance(struct vkcontext *vk)
{
    const char *surface_ext_name = platform_ext_names[vk->platform];
    const char *my_extension_names[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        surface_ext_name,
#if ENABLE_DEBUG
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
    };

    LOG(DEBUG, "surface extension name: %s", surface_ext_name);

    VkInstanceCreateInfo instance_create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = NGLI_ARRAY_NB(my_extension_names),
        .ppEnabledExtensionNames = my_extension_names,
#if ENABLE_DEBUG
        .enabledLayerCount = NGLI_ARRAY_NB(my_layers),
        .ppEnabledLayerNames = my_layers,
#endif
    };
    VkResult ret = vkCreateInstance(&instance_create_info, NULL, &vk->instance);
    return ret;
}

static void *vulkan_get_proc_addr(struct vkcontext *vk, const char *name)
{
    void *proc_addr = vkGetInstanceProcAddr(vk->instance, name);
    if (!proc_addr) {
        LOG(ERROR, "can not find %s extension", name);
        return NULL;
    }
    return proc_addr;
}

#if ENABLE_DEBUG
static VkResult setup_vulkan_debug_callback(struct vkcontext *vk)
{
    void *proc_addr = vulkan_get_proc_addr(vk, "vkCreateDebugUtilsMessengerEXT");
    if (!proc_addr)
        return VK_ERROR_EXTENSION_NOT_PRESENT;

    PFN_vkCreateDebugUtilsMessengerEXT func = proc_addr;
    VkDebugUtilsMessengerCreateInfoEXT callback_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debug_callback,
        .pUserData = NULL,
    };
    return func(vk->instance, &callback_create_info, NULL, &vk->report_callback);
}
#endif

static int string_in(const char *target, const char * const *str_list, int nb_str)
{
    for (int i = 0; i < nb_str; i++)
        if (!strcmp(target, str_list[i]))
            return 1;
    return 0;
}

static VkResult get_filtered_ext_props(const VkExtensionProperties *src_props, uint32_t nb_src_props,
                                       VkExtensionProperties **dst_props_p, uint32_t *nb_dst_props_p,
                                       const char * const *filtered_names, int nb_filtered_names)
{
    *dst_props_p = NULL;
    *nb_dst_props_p = 0;

    VkExtensionProperties *dst_props = ngli_calloc(nb_src_props, sizeof(*dst_props));
    if (!dst_props_p)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    int nb_dst_props = 0;
    for (int j = 0; j < nb_src_props; j++) {
        if (!string_in(src_props[j].extensionName, filtered_names, nb_filtered_names))
            continue;
        dst_props[nb_dst_props++] = src_props[j];
        if (nb_dst_props == nb_filtered_names)
            break;
    }

    *dst_props_p = dst_props;
    *nb_dst_props_p = nb_dst_props;
    return VK_SUCCESS;
}

static VkResult select_vulkan_physical_device(struct gctx *s, struct vkcontext *vk)
{
    struct ngl_ctx *ctx = s->ctx;
    const struct ngl_config *config = &ctx->config;

    uint32_t phydevice_count = 0;
    vkEnumeratePhysicalDevices(vk->instance, &phydevice_count, NULL);
    if (!phydevice_count) {
        LOG(ERROR, "no physical device available");
        return VK_ERROR_DEVICE_LOST;
    }
    VkPhysicalDevice *phy_devices = ngli_calloc(phydevice_count, sizeof(*phy_devices));
    if (!phy_devices)
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    vkEnumeratePhysicalDevices(vk->instance, &phydevice_count, phy_devices);
    LOG(INFO, "Vulkan physical devices available:");
    for (uint32_t i = 0; i < phydevice_count; i++) {
        VkPhysicalDevice phy_device = phy_devices[i];

        /* Device properties */
        static const char *physical_device_types[] = {
            [VK_PHYSICAL_DEVICE_TYPE_OTHER] = "other",
            [VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU] = "integrated",
            [VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU] = "discrete",
            [VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU] = "virtual",
            [VK_PHYSICAL_DEVICE_TYPE_CPU] = "cpu",
        };
        VkPhysicalDeviceProperties dev_props;
        vkGetPhysicalDeviceProperties(phy_device, &dev_props);
        LOG(INFO, "  %d/%d: %s (%s)", i+1, phydevice_count,
               dev_props.deviceName,
               physical_device_types[dev_props.deviceType]);

        /* Device features */
        VkPhysicalDeviceFeatures dev_features;
        vkGetPhysicalDeviceFeatures(phy_device, &dev_features);

        /* Device queue families */
        int queue_family_graphics_id = -1;
        int queue_family_present_id = -1;
        uint32_t qfamily_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(phy_device, &qfamily_count, NULL);
        VkQueueFamilyProperties *qfamily_props = ngli_calloc(qfamily_count, sizeof(*qfamily_props));
        if (!qfamily_props)
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        vkGetPhysicalDeviceQueueFamilyProperties(phy_device, &qfamily_count, qfamily_props);
        LOG(DEBUG, "  queue props:");
        for (uint32_t j = 0; j < qfamily_count; j++) {
            VkQueueFamilyProperties props = qfamily_props[j];
            LOG(DEBUG, "    family %d/%d:%s%s%s%s%s (count: %d)", j+1, qfamily_count,
                   props.queueFlags & VK_QUEUE_GRAPHICS_BIT       ? " Graphics"      : "",
                   props.queueFlags & VK_QUEUE_COMPUTE_BIT        ? " Compute"       : "",
                   props.queueFlags & VK_QUEUE_TRANSFER_BIT       ? " Transfer"      : "",
                   props.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT ? " SparseBinding" : "",
#ifdef VK_QUEUE_PROTECTED_BIT
                   props.queueFlags & VK_QUEUE_PROTECTED_BIT      ? " Protected"     : "",
#else
                   "",
#endif
                   props.queueCount);
            if ((props.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (props.queueFlags & VK_QUEUE_COMPUTE_BIT))
                queue_family_graphics_id = j;

            if (!config->offscreen) {
            VkBool32 surface_support;
            vkGetPhysicalDeviceSurfaceSupportKHR(phy_device, j, vk->surface, &surface_support);
            if (surface_support)
                queue_family_present_id = j;
            }
        }
        ngli_free(qfamily_props);

        /* Device extensions */
        uint32_t extprops_count;
        vkEnumerateDeviceExtensionProperties(phy_device, NULL, &extprops_count, NULL);
        VkExtensionProperties *ext_props = ngli_calloc(extprops_count, sizeof(*ext_props));
        if (!ext_props)
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        vkEnumerateDeviceExtensionProperties(phy_device, NULL, &extprops_count, ext_props);
        LOG(DEBUG, "  extensions available:");
        for (uint32_t j = 0; j < extprops_count; j++) {
            LOG(DEBUG, "    %d/%d: %s v%d", j+1, extprops_count,
                   ext_props[j].extensionName, ext_props[j].specVersion);
        }

        /* Filtered device extensions */
        VkExtensionProperties *my_ext_props;
        uint32_t my_ext_props_count;
        uint32_t my_ext_props_target_count = NGLI_ARRAY_NB(my_device_extension_names);
        VkResult ret = get_filtered_ext_props(ext_props, extprops_count,
                                              &my_ext_props, &my_ext_props_count,
                                              my_device_extension_names, NGLI_ARRAY_NB(my_device_extension_names));
        ngli_free(ext_props);
        ngli_free(my_ext_props);
        if (ret != VK_SUCCESS)
            return ret;

        /* Swapchain support */
        if (!config->offscreen) {
        ret = query_swapchain_support(&vk->swapchain_support, vk->surface, phy_device);
        if (ret != VK_SUCCESS)
            return ret;
        LOG(DEBUG, "  Swapchain: %d formats, %d presentation modes",
               vk->swapchain_support.nb_formats, vk->swapchain_support.nb_present_modes);
        }

        /* Device selection criterias */
        LOG(DEBUG, "  Graphics/Compute:%d Present:%d DeviceEXT:%d/%d",
               queue_family_graphics_id, queue_family_present_id,
               my_ext_props_count, my_ext_props_target_count);
        if (!vk->physical_device &&
            queue_family_graphics_id != -1 &&
            (config->offscreen || queue_family_present_id != -1) &&
            my_ext_props_count == my_ext_props_target_count &&
            (config->offscreen ||
            (vk->swapchain_support.nb_formats &&
            vk->swapchain_support.nb_present_modes))) {
            LOG(DEBUG, "  -> device selected");
            vk->physical_device = phy_device;
            vk->dev_features = dev_features;
            vk->queue_family_graphics_id = queue_family_graphics_id;
            vk->queue_family_present_id = queue_family_present_id;
        }
    }
    ngli_free(phy_devices);
    if (!vk->physical_device) {
        LOG(ERROR, "no valid physical device found");
        return VK_ERROR_DEVICE_LOST;
    }

    vkGetPhysicalDeviceMemoryProperties(vk->physical_device, &vk->phydev_mem_props);

    return VK_SUCCESS;
}

static VkResult create_vulkan_device(struct vkcontext *vk)
{
    /* Device Queue info */
    int nb_queues = 1;
    float queue_priority = 1.0;

    VkDeviceQueueCreateInfo graphic_queue_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = vk->queue_family_graphics_id,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority,
    };

    VkDeviceQueueCreateInfo queues_create_info[2];
    queues_create_info[0] = graphic_queue_create_info;
    if (vk->queue_family_graphics_id != vk->queue_family_present_id &&
        vk->queue_family_present_id != -1) {
        // XXX: needed?
        VkDeviceQueueCreateInfo present_queue_create_info = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = vk->queue_family_present_id,
            .queueCount = 1,
            .pQueuePriorities = &queue_priority,
        };
        queues_create_info[nb_queues++] = present_queue_create_info;
    }

    VkPhysicalDeviceFeatures dev_features = {0};

    /* TODO: Check mandatory features earlier (in select_[...]()) */

#define ENABLE_FEATURE(feature)                                                  \
    if (vk->dev_features.feature) {                                              \
        dev_features.feature = VK_TRUE;                                          \
    } else {                                                                     \
        LOG(ERROR, "Mandatory feature " #feature " is not supported by device"); \
        return -1;                                                               \
    }                                                                            \

    ENABLE_FEATURE(samplerAnisotropy)
    //Not supported on Intel Haswell GPU
    //ENABLE_FEATURE(vertexPipelineStoresAndAtomics)
    ENABLE_FEATURE(fragmentStoresAndAtomics)
    ENABLE_FEATURE(shaderStorageImageExtendedFormats)

#undef ENABLE_FEATURE

    VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = queues_create_info,
        .queueCreateInfoCount = nb_queues,
        .enabledExtensionCount = NGLI_ARRAY_NB(my_device_extension_names),
        .ppEnabledExtensionNames = my_device_extension_names,
        .pEnabledFeatures = &dev_features,
    };
    VkResult ret = vkCreateDevice(vk->physical_device, &device_create_info, NULL, &vk->device);
    if (ret != VK_SUCCESS)
        return ret;

    vkGetDeviceQueue(vk->device, vk->queue_family_graphics_id, 0, &vk->graphic_queue);
    if (vk->queue_family_present_id != -1)
    vkGetDeviceQueue(vk->device, vk->queue_family_present_id, 0, &vk->present_queue);
    return ret;
}

static VkResult create_swapchain(struct gctx *s)
{
    struct gctx_vk *s_priv = (struct gctx_vk *)s;
    struct vkcontext *vk = s_priv->vkcontext;

    // set maximum in flight frames
    // FIXME: nb_in_flight_frames needs nb_in_flight_frames sets of dynamic resources
    s_priv->nb_in_flight_frames = 1;

    // re-query the swapchain to get current extent
    VkResult ret = query_swapchain_support(&vk->swapchain_support, vk->surface, vk->physical_device);
    if (ret != VK_SUCCESS)
        return ret;

    const struct vk_swapchain_support *swap = &vk->swapchain_support;

    vk->surface_format = select_swapchain_surface_format(swap->formats, swap->nb_formats);
    vk->present_mode = select_swapchain_present_mode(swap->present_modes, swap->nb_present_modes);
    s_priv->extent = select_swapchain_current_extent(vk, swap->caps);
    vk->width = s_priv->extent.width;
    vk->height = s_priv->extent.height;
    LOG(INFO, "current extent: %dx%d", s_priv->extent.width, s_priv->extent.height);

    uint32_t img_count = swap->caps.minImageCount + 1;
    if (swap->caps.maxImageCount && img_count > swap->caps.maxImageCount)
        img_count = swap->caps.maxImageCount;
    LOG(INFO, "swapchain image count: %d [%d-%d]", img_count,
           swap->caps.minImageCount, swap->caps.maxImageCount);

    VkSwapchainCreateInfoKHR swapchain_create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = vk->surface,
        .minImageCount = img_count,
        .imageFormat = vk->surface_format.format,
        .imageColorSpace = vk->surface_format.colorSpace,
        .imageExtent = s_priv->extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = swap->caps.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = vk->present_mode,
        .clipped = VK_TRUE,
    };

    const uint32_t queue_family_indices[2] = {
        vk->queue_family_graphics_id,
        vk->queue_family_present_id,
    };
    if (queue_family_indices[0] != queue_family_indices[1]) {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_create_info.queueFamilyIndexCount = NGLI_ARRAY_NB(queue_family_indices);
        swapchain_create_info.pQueueFamilyIndices = queue_family_indices;
    }

    VkResult res = vkCreateSwapchainKHR(vk->device, &swapchain_create_info, NULL, &vk->swapchain);
    if (res != VK_SUCCESS)
        return res;

    return res;
}

static VkResult create_swapchain_images(struct gctx *s)
{
    struct gctx_vk *s_priv = (struct gctx_vk *)s;
    struct vkcontext *vk = s_priv->vkcontext;
    struct ngl_ctx *ctx = s->ctx;
    struct ngl_config *config = &ctx->config;

    vkGetSwapchainImagesKHR(vk->device, vk->swapchain, &s_priv->nb_images, NULL);
    VkImage *imgs = ngli_realloc(s_priv->images, s_priv->nb_images * sizeof(*s_priv->images));
    if (!imgs)
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    s_priv->images = imgs;
    vkGetSwapchainImagesKHR(vk->device, vk->swapchain, &s_priv->nb_images, s_priv->images);

    struct texture **wrapped_textures = ngli_realloc(s_priv->wrapped_textures, s_priv->nb_images * sizeof(*s_priv->wrapped_textures));
    if (!wrapped_textures)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    struct texture **depth_textures = ngli_realloc(s_priv->depth_textures, s_priv->nb_images * sizeof(*s_priv->depth_textures));
    if (!depth_textures)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    struct texture **textures = ngli_realloc(s_priv->textures, s_priv->nb_images * sizeof(*s_priv->textures));
    if (!textures)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    struct rendertarget **rts = ngli_realloc(s_priv->rts, s_priv->nb_images * sizeof(*s_priv->rts));
    if (!rts)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    memset(wrapped_textures, 0, s_priv->nb_images * sizeof(*s_priv->wrapped_textures));
    memset(textures, 0, s_priv->nb_images * sizeof(s_priv->textures));
    memset(depth_textures, 0, s_priv->nb_images * sizeof(*s_priv->depth_textures));
    memset(rts, 0, s_priv->nb_images * sizeof(*s_priv->rts));

    s_priv->wrapped_textures = wrapped_textures;
    s_priv->textures = textures;
    s_priv->depth_textures = depth_textures;
    s_priv->rts = rts;
    s_priv->nb_wrapped_textures = s_priv->nb_images;

    for (uint32_t i = 0; i < s_priv->nb_images; i++) {
        s_priv->wrapped_textures[i] = ngli_texture_create(s);
        if (!s_priv->wrapped_textures[i])
            return VK_ERROR_OUT_OF_HOST_MEMORY;

        struct texture_params params = {
            .type = NGLI_TEXTURE_TYPE_2D,
            .format = NGLI_FORMAT_B8G8R8A8_UNORM,
            .width = s_priv->extent.width,
            .height = s_priv->extent.height,
            .external_storage = 1,
            .usage = NGLI_TEXTURE_USAGE_ATTACHMENT_ONLY,
        };

        int ret = ngli_texture_vk_wrap(s_priv->wrapped_textures[i], &params, s_priv->images[i], VK_IMAGE_LAYOUT_UNDEFINED);
        if (ret < 0)
            return ret;

        s_priv->depth_textures[i] = ngli_texture_create(s);
        if (!s_priv->depth_textures[i])
            return VK_ERROR_OUT_OF_HOST_MEMORY;

        struct texture_params depth_params = {
            .type = NGLI_TEXTURE_TYPE_2D,
            .format = NGLI_FORMAT_D32_SFLOAT_S8_UINT,
            .width = s_priv->extent.width,
            .height = s_priv->extent.height,
            .samples = config->samples,
            .usage = NGLI_TEXTURE_USAGE_ATTACHMENT_ONLY,
        };

        ret = ngli_texture_vk_init(s_priv->depth_textures[i], &depth_params);
        if (ret < 0)
            return ret;

        struct rendertarget_params rt_params = {
            .width = s_priv->extent.width,
            .height = s_priv->extent.height,
            .nb_colors = 1,
            .colors[0].attachment = s_priv->wrapped_textures[i],
            .depth_stencil.attachment = s_priv->depth_textures[i],
        };

#if 1
        if (config->samples) {
            struct texture_params texture_params = NGLI_TEXTURE_PARAM_DEFAULTS;
            texture_params.width = s_priv->extent.width;
            texture_params.height = s_priv->extent.height;
            texture_params.format = NGLI_FORMAT_B8G8R8A8_UNORM;
            texture_params.samples = config->samples;
            texture_params.usage = NGLI_TEXTURE_USAGE_ATTACHMENT_ONLY,
            s_priv->rt_resolve_color = ngli_texture_create(s);
            if (!s_priv->rt_resolve_color)
                return NGL_ERROR_MEMORY;

            s_priv->textures[i] = ngli_texture_create(s);
            if (!s_priv->textures[i])
                return NGL_ERROR_MEMORY;

            ret = ngli_texture_init(s_priv->textures[i], &texture_params);
            if (ret < 0)
                return ret;
            rt_params.colors[0].attachment = s_priv->textures[i];
            rt_params.colors[0].resolve_target = s_priv->wrapped_textures[i];
        }
#endif

        s_priv->rts[i] = ngli_rendertarget_create(s);
        if (!s_priv->rts[i])
            return NGL_ERROR_MEMORY;

        ret = ngli_rendertarget_init(s_priv->rts[i], &rt_params);
        if (ret < 0)
            return ret;
    }

    return VK_SUCCESS;
}

int ngli_gctx_vk_begin_transient_command(struct gctx *s, VkCommandBuffer *command_buffer)
{
    struct gctx_vk *s_priv = (struct gctx_vk *)s;
    struct vkcontext *vk = s_priv->vkcontext;

    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = s_priv->transient_command_buffer_pool,
        .commandBufferCount = 1,
    };

    vkAllocateCommandBuffers(vk->device, &alloc_info, command_buffer);

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    vkBeginCommandBuffer(*command_buffer, &beginInfo);

    return 0;
}

int ngli_gctx_vk_execute_transient_command(struct gctx *s, VkCommandBuffer command_buffer)
{
    struct gctx_vk *s_priv = (struct gctx_vk *)s;
    struct vkcontext *vk = s_priv->vkcontext;

    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
    };

    vkResetFences(vk->device, 1, &s_priv->transient_command_buffer_fence);

    VkQueue graphic_queue;
    vkGetDeviceQueue(vk->device, vk->queue_family_graphics_id, 0, &graphic_queue);

    VkResult ret = vkQueueSubmit(graphic_queue, 1, &submit_info, s_priv->transient_command_buffer_fence);
    if (ret != VK_SUCCESS)
        return ret;
    vkWaitForFences(vk->device, 1, &s_priv->transient_command_buffer_fence, 1, UINT64_MAX);

    vkQueueWaitIdle(graphic_queue);
    vkFreeCommandBuffers(vk->device, s_priv->transient_command_buffer_pool, 1, &command_buffer);

    return ret;
}

static int create_command_pool_and_buffers(struct gctx *s)
{
    struct gctx_vk *s_priv = (struct gctx_vk *)s;
    struct vkcontext *vk = s_priv->vkcontext;

    VkCommandPoolCreateInfo command_pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = vk->queue_family_graphics_id,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, // FIXME
    };

    VkResult vkret = vkCreateCommandPool(vk->device, &command_pool_create_info, NULL, &s_priv->command_buffer_pool);
    if (vkret != VK_SUCCESS)
        return NGL_ERROR_EXTERNAL;

    s_priv->command_buffers = ngli_calloc(s_priv->nb_images, sizeof(*s_priv->command_buffers));
    if (!s_priv->command_buffers)
        return NGL_ERROR_MEMORY;

    VkCommandBufferAllocateInfo command_buffers_allocate_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = s_priv->command_buffer_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = s_priv->nb_images,
    };

    VkResult ret = vkAllocateCommandBuffers(vk->device, &command_buffers_allocate_info, s_priv->command_buffers);
    if (ret != VK_SUCCESS)
        return NGL_ERROR_EXTERNAL;

    return 0;
}

static void destroy_command_pool_and_buffers(struct gctx *s)
{
    struct gctx_vk *s_priv = (struct gctx_vk *)s;
    struct vkcontext *vk = s_priv->vkcontext;

    vkFreeCommandBuffers(vk->device, s_priv->command_buffer_pool, s_priv->nb_images, s_priv->command_buffers);
    ngli_free(s_priv->command_buffers);

    vkDestroyCommandPool(vk->device, s_priv->command_buffer_pool, NULL);
}

static VkResult create_semaphores(struct gctx *s)
{
    struct gctx_vk *s_priv = (struct gctx_vk *)s;
    struct vkcontext *vk = s_priv->vkcontext;
    VkResult ret;

    s_priv->sem_img_avail = ngli_calloc(s_priv->nb_in_flight_frames, sizeof(VkSemaphore));
    if (!s_priv->sem_img_avail)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    s_priv->sem_render_finished = ngli_calloc(s_priv->nb_in_flight_frames, sizeof(VkSemaphore));
    if (!s_priv->sem_render_finished)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    s_priv->fences = ngli_calloc(s_priv->nb_in_flight_frames, sizeof(VkFence));
    if (!s_priv->fences)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    VkSemaphoreCreateInfo semaphore_create_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    VkFenceCreateInfo fence_create_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    for (int i = 0; i < s_priv->nb_in_flight_frames; i++) {
        if ((ret = vkCreateSemaphore(vk->device, &semaphore_create_info, NULL,
                                    &s_priv->sem_img_avail[i])) != VK_SUCCESS ||
            (ret = vkCreateSemaphore(vk->device, &semaphore_create_info, NULL,
                                    &s_priv->sem_render_finished[i])) != VK_SUCCESS ||
            (ret = vkCreateFence(vk->device, &fence_create_info, NULL,
                                 &s_priv->fences[i])) != VK_SUCCESS) {
            return ret;
        }
    }
    return VK_SUCCESS;
}

static VkResult create_window_surface(struct gctx *s,
                                      uintptr_t display,
                                      uintptr_t window,
                                      VkSurfaceKHR *surface)
{
    struct gctx_vk *s_priv = (struct gctx_vk *)s;
    struct vkcontext *vk = s_priv->vkcontext;

    VkResult ret = VK_ERROR_FEATURE_NOT_PRESENT;

    if (s_priv->surface_create_type == VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR) {
#if defined(VK_USE_PLATFORM_XLIB_KHR)
        VkXlibSurfaceCreateInfoKHR surface_create_info = {
            .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
            .dpy = (Display *)display,
            .window = window,
        };

        if (!surface_create_info.dpy) {
            surface_create_info.dpy = XOpenDisplay(NULL);
            if (!surface_create_info.dpy) {
                LOG(ERROR, "could not retrieve X display");
                return -1;
            }
            // TODO ngli_free display
            //x11->own_display = 1;
        }

        PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR = vulkan_get_proc_addr(vk, "vkCreateXlibSurfaceKHR");
        if (!vkCreateXlibSurfaceKHR)
            return VK_ERROR_EXTENSION_NOT_PRESENT;

        ret = vkCreateXlibSurfaceKHR(vk->instance, &surface_create_info, NULL, surface);
#endif
    } else if (s_priv->surface_create_type == VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR) {
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
        VkAndroidSurfaceCreateInfoKHR surface_create_info = {
            .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
            .window = (void *)window,
        };

        PFN_vkCreateAndroidSurfaceKHR vkCreateAndroidSurfaceKHR = vulkan_get_proc_addr(vk, "vkCreateAndroidSurfaceKHR");
        if (!vkCreateAndroidSurfaceKHR)
            return VK_ERROR_EXTENSION_NOT_PRESENT;

        ret = vkCreateAndroidSurfaceKHR(vk->instance, &surface_create_info, NULL, surface);
#endif
    } else if (s_priv->surface_create_type == VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK) {
#if defined(VK_USE_PLATFORM_MACOS_MVK)
        VkMacOSSurfaceCreateInfoMVK surface_create_info = {
            .sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK,
            .pView = (const void *)window,
        };

        PFN_vkCreateMacOSSurfaceMVK vkCreateMacOSSurfaceMVK = vulkan_get_proc_addr(vk, "vkCreateMacOSSurfaceMVK");
        if (!vkCreateMacOSSurfaceMVK)
            return VK_ERROR_EXTENSION_NOT_PRESENT;

        ret = vkCreateMacOSSurfaceMVK(vk->instance, &surface_create_info, NULL, surface);
#endif
    } else if (s_priv->surface_create_type == VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_MVK) {
#if defined(VK_USE_PLATFORM_IOS_MVK)
        VkIOSSurfaceCreateInfoMVK surface_create_info = {
            .sType = VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_MVK,
            .pView = (const void *)window,
        };

        PFN_vkCreateIOSSurfaceMVK vkCreateIOSSurfaceMVK = vulkan_get_proc_addr(vk, "vkCreateIOSSurfaceMVK");
        if (!vkCreateIOSSurfaceMVK)
            return VK_ERROR_EXTENSION_NOT_PRESENT;

        ret = vkCreateIOSSurfaceMVK(vk->instance, &surface_create_info, NULL, surface);
#endif
    } else {
        // TODO
        ngli_assert(0);
    }

    return ret;
}

static int vulkan_init(struct gctx *s, struct vkcontext *vk, uintptr_t display, uintptr_t window)
{
    VkResult ret;
    struct ngl_ctx *ctx = s->ctx;
    const struct ngl_config *config = &ctx->config;
    struct gctx_vk *s_priv = (struct gctx_vk *)s;

    s_priv->xxx = ngli_vkcontext_create();
    if (!s_priv->xxx)
        return NGL_ERROR_MEMORY;

    VkResult res = ngli_vkcontext_init(s_priv->xxx, config);
    if (res != VK_SUCCESS)
        return NGL_ERROR_EXTERNAL;

    vk->platform = config->platform;
    vk->backend = config->backend;
    vk->offscreen = config->offscreen;
    vk->width = config->width;
    vk->height = config->height;
    vk->samples = config->samples;

    ngli_darray_init(&s_priv->wait_semaphores, sizeof(VkSemaphore), 0);
    ngli_darray_init(&s_priv->wait_stages, sizeof(VkPipelineStageFlagBits), 0);
    ngli_darray_init(&s_priv->signal_semaphores, sizeof(VkSemaphore), 0);

    s_priv->spirv_compiler      = shaderc_compiler_initialize();
    s_priv->spirv_compiler_opts = shaderc_compile_options_initialize();
    if (!s_priv->spirv_compiler || !s_priv->spirv_compiler_opts)
        return -1;

    const shaderc_env_version env_version = app_info.apiVersion == VK_API_VERSION_1_0 ? shaderc_env_version_vulkan_1_0
                                                                                      : shaderc_env_version_vulkan_1_1;
    shaderc_compile_options_set_target_env(s_priv->spirv_compiler_opts, shaderc_target_env_vulkan, env_version);
    shaderc_compile_options_set_invert_y(s_priv->spirv_compiler_opts, 1);
    shaderc_compile_options_set_optimization_level(s_priv->spirv_compiler_opts, shaderc_optimization_level_performance);

    //ngli_darray_init(&s_priv->command_buffers, sizeof(VkCommandBuffer), 0);

    if ((ret = probe_vulkan_extensions(s)) != VK_SUCCESS ||
        (ret = list_vulkan_layers()) != VK_SUCCESS ||
        (ret = create_vulkan_instance(vk)) != VK_SUCCESS)
        return -1;

#if ENABLE_DEBUG
    if ((ret = setup_vulkan_debug_callback(vk)) != VK_SUCCESS)
        return -1;
#endif

    if (!config->offscreen) {
        if ((ret = create_window_surface(s, display, window, &vk->surface)) != VK_SUCCESS)
            return -1;
    }

    if ((ret = select_vulkan_physical_device(s, vk)) != VK_SUCCESS ||
        (ret = create_vulkan_device(vk)) != VK_SUCCESS)
        return -1;

    VkCommandPoolCreateInfo command_pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = vk->queue_family_graphics_id,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    };

    /* FIXME: check return */
    vkCreateCommandPool(vk->device, &command_pool_create_info, NULL, &s_priv->transient_command_buffer_pool);

    VkFenceCreateInfo fence_create_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
    };

    vkCreateFence(vk->device, &fence_create_info, NULL, &s_priv->transient_command_buffer_fence);

    ret = probe_prefered_depth_stencil_format(vk);
    if (ret != VK_SUCCESS)
        return -1;

    if (config->offscreen) {
        struct texture_params texture_params = NGLI_TEXTURE_PARAM_DEFAULTS;
        texture_params.width = config->width;
        texture_params.height = config->height;
        texture_params.format = NGLI_FORMAT_R8G8B8A8_UNORM;
        texture_params.samples = config->samples;
        s_priv->rt_color = ngli_texture_create(s);
        if (!s_priv->rt_color)
            return NGL_ERROR_MEMORY;

        ret = ngli_texture_init(s_priv->rt_color, &texture_params);
        if (ret < 0)
            return ret;

        texture_params.format = vk->prefered_depth_stencil_format;
        s_priv->rt_depth_stencil = ngli_texture_create(s);
        if (!s_priv->rt_depth_stencil)
            return NGL_ERROR_MEMORY;

        ret = ngli_texture_init(s_priv->rt_depth_stencil, &texture_params);
        if (ret < 0)
            return ret;

        struct rendertarget_params rt_params = {
            .width = config->width,
            .height = config->height,
            .nb_colors = 1,
            .colors[0].attachment = s_priv->rt_color,
            .depth_stencil.attachment = s_priv->rt_depth_stencil,
            .readable = 1,
        };

        if (config->samples) {
            struct texture_params texture_params = NGLI_TEXTURE_PARAM_DEFAULTS;
            texture_params.width = config->width;
            texture_params.height = config->height;
            texture_params.format = NGLI_FORMAT_R8G8B8A8_UNORM;
            texture_params.samples = 1;
            s_priv->rt_resolve_color = ngli_texture_create(s);
            if (!s_priv->rt_resolve_color)
                return NGL_ERROR_MEMORY;

            ret = ngli_texture_init(s_priv->rt_resolve_color, &texture_params);
            if (ret < 0)
                return ret;
            rt_params.colors[0].resolve_target = s_priv->rt_resolve_color;
        }

        s_priv->rt = ngli_rendertarget_create(s);
        if (!s_priv->rt)
            return NGL_ERROR_MEMORY;

        ret = ngli_rendertarget_init(s_priv->rt, &rt_params);
        if (ret < 0)
            return ret;

        s_priv->nb_images = 1;
        s_priv->nb_in_flight_frames = 1;
    } else {
        if ((ret = create_swapchain(s)) != VK_SUCCESS ||
            (ret = create_swapchain_images(s)) != VK_SUCCESS)
            return -1;
    }

    if ((ret = create_semaphores(s)) != VK_SUCCESS) {
        return -1;
    }

    create_command_pool_and_buffers(s);

    return 0;
}

static VkResult vulkan_swap_buffers(struct gctx *s)
{
    struct gctx_vk *s_priv = (struct gctx_vk *)s;
    struct vkcontext *vk = s_priv->vkcontext;

    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = ngli_darray_count(&s_priv->signal_semaphores),
        .pWaitSemaphores = ngli_darray_data(&s_priv->signal_semaphores),
        .swapchainCount = 1,
        .pSwapchains = &vk->swapchain,
        .pImageIndices = &s_priv->img_index,
    };

    int ret = vkQueuePresentKHR(vk->present_queue, &present_info);

    s_priv->signal_semaphores.count = 0;

    if (ret == VK_ERROR_OUT_OF_DATE_KHR) {
        LOG(ERROR, "PRESENT OUT OF DATE");
    } else if (ret == VK_SUBOPTIMAL_KHR) {
        LOG(ERROR, "PRESENT SUBOPTIMAL");
    } else if (ret != VK_SUCCESS) {
        LOG(ERROR, "failed to present image %s", vk_res2str(ret));
    }

    s_priv->current_frame = (s_priv->current_frame + 1) % s_priv->nb_in_flight_frames;
    return ret;
}

static void cleanup_swapchain(struct gctx *s)
{
    struct gctx_vk *s_priv = (struct gctx_vk *)s;
    struct vkcontext *vk = s_priv->vkcontext;

    for (uint32_t i = 0; i < s_priv->nb_wrapped_textures; i++) {
        ngli_rendertarget_freep(&s_priv->rts[i]);
        ngli_texture_freep(&s_priv->wrapped_textures[i]);
        ngli_texture_freep(&s_priv->textures[i]);
        ngli_texture_freep(&s_priv->depth_textures[i]);
    }

    vkDestroySwapchainKHR(vk->device, vk->swapchain, NULL);
}

// XXX: window minimizing? (fb gets zero width or height)
static int reset_swapchain(struct gctx *gctx, struct vkcontext *vk)
{
    VkResult ret;

    vkDeviceWaitIdle(vk->device);
    cleanup_swapchain(gctx);
    if ((ret = create_swapchain(gctx)) != VK_SUCCESS ||
        (ret = create_swapchain_images(gctx)) != VK_SUCCESS)
        return -1;

    return 0;
}

static void vulkan_uninit(struct gctx *s)
{
    struct ngl_ctx *ctx = s->ctx;
    const struct ngl_config *config = &ctx->config;
    struct gctx_vk *s_priv = (struct gctx_vk *)s;
    struct vkcontext *vk = s_priv->vkcontext;

    /* FIXME */
    if (!vk->device)
        return;

    vkDeviceWaitIdle(vk->device);

    destroy_command_pool_and_buffers(s);

    for (int i = 0; i < s_priv->nb_in_flight_frames; i++) {
        vkDestroySemaphore(vk->device, s_priv->sem_render_finished[i], NULL);
        vkDestroySemaphore(vk->device, s_priv->sem_img_avail[i], NULL);
        vkDestroyFence(vk->device, s_priv->fences[i], NULL);
    }
    ngli_free(s_priv->sem_render_finished);
    ngli_free(s_priv->sem_img_avail);
    ngli_free(s_priv->fences);

    if (config->offscreen) {
        ngli_rendertarget_freep(&s_priv->rt);
        ngli_texture_freep(&s_priv->rt_color);
        ngli_texture_freep(&s_priv->rt_resolve_color);
        ngli_texture_freep(&s_priv->rt_depth_stencil);
    }

    if (!config->offscreen)
        cleanup_swapchain(s);

    vkDestroyCommandPool(vk->device, s_priv->transient_command_buffer_pool, NULL);
    vkDestroyFence(vk->device, s_priv->transient_command_buffer_fence, NULL);

    ngli_free(vk->swapchain_support.formats);
    ngli_free(vk->swapchain_support.present_modes);
    if (!config->offscreen)
        vkDestroySurfaceKHR(vk->instance, vk->surface, NULL);
    ngli_free(s_priv->images);
    vkDestroyDevice(vk->device, NULL);
#if ENABLE_DEBUG
    void *proc_addr = vulkan_get_proc_addr(vk, "vkDestroyDebugUtilsMessengerEXT");
    if (proc_addr) {
        PFN_vkDestroyDebugUtilsMessengerEXT func = proc_addr;
        func(vk->instance, vk->report_callback, NULL);
    }
#endif
    vkDestroyInstance(vk->instance, NULL);

    shaderc_compiler_release(s_priv->spirv_compiler);
    shaderc_compile_options_release(s_priv->spirv_compiler_opts);

    ngli_darray_reset(&s_priv->wait_semaphores);
    ngli_darray_reset(&s_priv->wait_stages);
    ngli_darray_reset(&s_priv->signal_semaphores);
}

static struct gctx *vk_create(struct ngl_ctx *ctx)
{
    struct gctx_vk *s = ngli_calloc(1, sizeof(*s));
    if (!s)
        return NULL;
    return (struct gctx *)s;
}

static int vk_init(struct gctx *s)
{
    struct ngl_ctx *ctx = s->ctx;
    const struct ngl_config *config = &ctx->config;
    struct gctx_vk *s_priv = (struct gctx_vk *)s;

    s_priv->vkcontext = ngli_calloc(1, sizeof(*s_priv->vkcontext));
    if (!s_priv->vkcontext)
        return -1;
    struct vkcontext *vk = s_priv->vkcontext;

    /* FIXME */
    s->features = -1;

    static const NGLI_ALIGNED_MAT(projection_matrix) = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f,-1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.5f, 0.0f,
        0.0f, 0.0f, 0.5f, 1.0f,
    };
    if (!ngli_darray_push(&ctx->projection_matrix_stack, projection_matrix))
        return NGL_ERROR_MEMORY;

    int ret = vulkan_init(s, vk, config->display, config->window);
    if (ret < 0) {
        ngli_free(vk);
        return ret;
    }

    ret = ngli_pgcache_init(&s->pgcache, ctx);
    if (ret < 0)
        return ret;

    const int *viewport = config->viewport;
    if (viewport[2] > 0 && viewport[3] > 0) {
        ngli_gctx_set_viewport(s, viewport);
    } else {
        const int default_viewport[] = {0, 0, config->width, config->height};
        ngli_gctx_set_viewport(s, default_viewport);
    }

    const int scissor[] = {0, 0, config->width, config->height};
    ngli_gctx_set_scissor(s, scissor);

    ngli_gctx_set_clear_color(s, config->clear_color);

    struct graphicstate *graphicstate = &ctx->graphicstate;
    ngli_graphicstate_init(graphicstate);

    if (config->offscreen) {
        s_priv->default_rendertarget_desc.nb_colors = 1;
        s_priv->default_rendertarget_desc.colors[0].format = NGLI_FORMAT_R8G8B8A8_UNORM;
        s_priv->default_rendertarget_desc.colors[0].samples = config->samples;
        s_priv->default_rendertarget_desc.colors[0].resolve = 1;
        s_priv->default_rendertarget_desc.depth_stencil.format = vk->prefered_depth_stencil_format;
        s_priv->default_rendertarget_desc.depth_stencil.samples = config->samples;
        s_priv->default_rendertarget_desc.depth_stencil.resolve = 0;
        ctx->rendertarget_desc = &s_priv->default_rendertarget_desc;
        ngli_gctx_set_rendertarget(s, s_priv->rt);
    } else {
        s_priv->default_rendertarget_desc.nb_colors = 1;
        s_priv->default_rendertarget_desc.colors[0].format = NGLI_FORMAT_B8G8R8A8_UNORM;
        s_priv->default_rendertarget_desc.colors[0].samples = config->samples;
        s_priv->default_rendertarget_desc.colors[0].resolve = config->samples > 0 ? 1 : 0;
        s_priv->default_rendertarget_desc.depth_stencil.format = vk->prefered_depth_stencil_format;
        s_priv->default_rendertarget_desc.depth_stencil.samples = config->samples;
        ctx->rendertarget_desc = &s_priv->default_rendertarget_desc;
        ngli_gctx_set_rendertarget(s, s_priv->rts[0]);
    }

    return 0;
}

static int vk_resize(struct gctx *s, int width, int height, const int *viewport)
{
    struct gctx_vk *s_priv = (struct gctx_vk *)s;
    struct vkcontext *vk = s_priv->vkcontext;

    vk->width = width;
    vk->height = height;

    if (viewport && viewport[2] > 0 && viewport[3] > 0) {
        ngli_gctx_set_viewport(s, viewport);
    } else {
        const int default_viewport[] = {0, 0, width, height};
        ngli_gctx_set_viewport(s, default_viewport);
    }

    const int scissor[] = {0, 0, width, height};
    ngli_gctx_set_scissor(s, scissor);

    return 0;
}

static int vk_pre_draw(struct gctx *s, double t)
{
    int ret = 0;
    struct ngl_ctx *ctx = s->ctx;
    const struct ngl_config *config = &ctx->config;
    struct gctx_vk *s_priv = (struct gctx_vk *)s;
    struct vkcontext *vk = s_priv->vkcontext;

    vkWaitForFences(vk->device, 1, &s_priv->fences[s_priv->current_frame], VK_TRUE, UINT64_MAX);
    vkResetFences(vk->device, 1, &s_priv->fences[s_priv->current_frame]);

    struct rendertarget *rt = config->offscreen ? s_priv->rt : NULL;
    if (!config->offscreen) {
        /* TODO: swapchain_swap_buffers() | swapchain_acquire_image() */
        VkResult vkret = vkAcquireNextImageKHR(vk->device, vk->swapchain, UINT64_MAX, s_priv->sem_img_avail[s_priv->current_frame], NULL, &s_priv->img_index);
        if (vkret == VK_ERROR_OUT_OF_DATE_KHR) {
            ret = reset_swapchain(s, vk);
            if (ret >= 0) {
                VkResult vkret = vkAcquireNextImageKHR(vk->device, vk->swapchain, UINT64_MAX, s_priv->sem_img_avail[s_priv->current_frame], NULL, &s_priv->img_index);
                if (vkret != VK_SUCCESS) {
                    LOG(ERROR, "failed to acquire image after resetting the swap chain");
                    ret = -1;
                }
            }
        } else if (vkret == VK_SUBOPTIMAL_KHR) {
            ret = reset_swapchain(s, vk);
        } else if (vkret != VK_SUCCESS) {
            LOG(ERROR, "failed to acquire image");
            ret = -1;
        } else {
            ret = 0;
        }

        if (ret < 0)
            return ret;

        rt = s_priv->rts[s_priv->img_index];

        if (!ngli_darray_push(&s_priv->wait_semaphores, &s_priv->sem_img_avail[s_priv->current_frame]))
            return NGL_ERROR_MEMORY;

        VkPipelineStageFlagBits wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        if (!ngli_darray_push(&s_priv->wait_stages, &wait_stage))
            return NGL_ERROR_MEMORY;

        if (!ngli_darray_push(&s_priv->signal_semaphores, &s_priv->sem_render_finished[s_priv->current_frame]))
            return NGL_ERROR_MEMORY;
    }

    s_priv->cur_command_buffer = s_priv->command_buffers[s_priv->img_index];
    VkCommandBufferBeginInfo command_buffer_begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
    };
    VkResult vkret = vkBeginCommandBuffer(s_priv->cur_command_buffer, &command_buffer_begin_info);
    if (vkret != VK_SUCCESS)
        return -1;
    s_priv->cur_command_buffer_state = 1;

    ngli_gctx_set_rendertarget(s, rt);
    ngli_gctx_clear_color(s);
    ngli_gctx_clear_depth_stencil(s);
    ngli_gctx_clear_depth_stencil(s);

    return ret;
}

static int vk_post_draw(struct gctx *s, double t)
{
    struct ngl_ctx *ctx = s->ctx;
    const struct ngl_config *config = &ctx->config;
    struct gctx_vk *s_priv = (struct gctx_vk *)s;

    if (config->offscreen) {
        if (config->capture_buffer)
            ngli_rendertarget_read_pixels(s_priv->rt, config->capture_buffer);
        ngli_gctx_flush(s);
        s_priv->current_frame = (s_priv->current_frame + 1) % s_priv->nb_in_flight_frames;
    } else {
        ngli_gctx_vk_end_render_pass(s);

        struct rendertarget *rt = s_priv->rts[s_priv->img_index];
        struct rendertarget_params *params = &rt->params;
        ngli_texture_vk_transition_layout(s_priv->wrapped_textures[s_priv->img_index], VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        ngli_gctx_flush(s);
        VkResult vkret = vulkan_swap_buffers(s);
        if (vkret != VK_SUCCESS)
            return -1;
    }

    /* Reset cur_command_buffer so updating resources will use a transient
     * command buffer */
    s_priv->cur_command_buffer = VK_NULL_HANDLE;

    return 0;
}

static void vk_destroy(struct gctx *s)
{
    struct ngl_ctx *ctx = s->ctx;
    struct gctx_vk *s_priv = (struct gctx_vk *)s;
    ngli_vkcontext_freep(&s_priv->xxx);

    ngli_pgcache_reset(&s->pgcache);
    vulkan_uninit(s);
    ngli_darray_pop(&ctx->projection_matrix_stack);
}

int ngli_vk_find_memory_type(struct vkcontext *vk, uint32_t type_filter, VkMemoryPropertyFlags props)
{
    for (int i = 0; i < vk->phydev_mem_props.memoryTypeCount; i++)
        if ((type_filter & (1<<i)) && (vk->phydev_mem_props.memoryTypes[i].propertyFlags & props) == props)
            return i;
    return -1;
}

static void vk_wait_idle(struct gctx *s)
{
    struct gctx_vk *s_priv = (struct gctx_vk *)s;
    struct vkcontext *vk = s_priv->vkcontext;
    vkDeviceWaitIdle(vk->device);
}

void ngli_gctx_vk_commit_render_pass(struct gctx *s)
{
    struct gctx_vk *s_priv = (struct gctx_vk *)s;

    if (s_priv->cur_render_pass_state == 1)
        return;

    if (s_priv->rendertarget) {
        struct rendertarget_params *params = &s_priv->rendertarget->params;
        for (int i = 0; i < params->nb_colors; i++)
            ngli_texture_vk_transition_layout(params->colors[i].attachment, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        if (params->depth_stencil.attachment)
            ngli_texture_vk_transition_layout(params->depth_stencil.attachment, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

        for (int i = 0; i < params->nb_colors; i++) {
            struct texture_vk *resolve_target_vk = (struct texture_vk *)params->colors[i].resolve_target;
            if (resolve_target_vk)
                resolve_target_vk->image_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }

        struct texture_vk *resolve_target_vk = (struct texture_vk *)params->depth_stencil.resolve_target;
        if (resolve_target_vk)
            resolve_target_vk->image_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    VkCommandBuffer cmd_buf = s_priv->cur_command_buffer;
    VkRenderPassBeginInfo render_pass_begin_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = s_priv->cur_render_pass,
        .framebuffer = s_priv->cur_framebuffer,
        .renderArea = {
            .extent = s_priv->cur_render_area,
        },
        .clearValueCount = 0,
        .pClearValues = NULL,
    };
    vkCmdBeginRenderPass(cmd_buf, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    s_priv->cur_render_pass_state = 1;
}

void ngli_gctx_vk_end_render_pass(struct gctx *s)
{
    struct gctx_vk *s_priv = (struct gctx_vk *)s;

    if (s_priv->cur_render_pass_state != 1)
        return;

    VkCommandBuffer cmd_buf = s_priv->cur_command_buffer;
    vkCmdEndRenderPass(cmd_buf);
    s_priv->cur_render_pass_state = 0;
}

static void vk_set_rendertarget(struct gctx *s, struct rendertarget *rt)
{
    /* FIXME */
    int conservative = 0;
    struct gctx_vk *s_priv = (struct gctx_vk *)s;
    if (s_priv->cur_render_pass && rt != s_priv->rendertarget) {
        VkCommandBuffer cmd_buf = s_priv->cur_command_buffer;
        if (s_priv->cur_render_pass_state == 1)
            vkCmdEndRenderPass(cmd_buf);
    }

    s_priv->rendertarget = rt;
    if (rt) {
        struct rendertarget_vk *rt_vk = (struct rendertarget_vk*)rt;
        s_priv->cur_framebuffer = rt_vk->framebuffer;
        s_priv->cur_render_pass = conservative ? rt_vk->conservative_render_pass : rt_vk->render_pass;
        s_priv->cur_render_area = rt_vk->render_area;
    } else {
        ngli_assert(0);
    }
    s_priv->cur_render_pass_state = 0;
}

static struct rendertarget *vk_get_rendertarget(struct gctx *s)
{
    struct gctx_vk *s_priv = (struct gctx_vk *)s;
    return s_priv->rendertarget;
}

static void vk_set_viewport(struct gctx *s, const int *viewport)
{
    struct gctx_vk *s_priv = (struct gctx_vk *)s;
    memcpy(s_priv->viewport, viewport, sizeof(s_priv->viewport));
}

static void vk_get_viewport(struct gctx *s, int *viewport)
{
    struct gctx_vk *s_priv = (struct gctx_vk *)s;
    memcpy(viewport, &s_priv->viewport, sizeof(s_priv->viewport));
}

static void vk_set_scissor(struct gctx *s, const int *scissor)
{
    struct gctx_vk *s_priv = (struct gctx_vk *)s;
    memcpy(&s_priv->scissor, scissor, sizeof(s_priv->scissor));
}

static void vk_get_scissor(struct gctx *s, int *scissor)
{
    struct gctx_vk *s_priv = (struct gctx_vk *)s;
    memcpy(scissor, &s_priv->scissor, sizeof(s_priv->scissor));
}

static void vk_set_clear_color(struct gctx *s, const float *color)
{
    struct gctx_vk *s_priv = (struct gctx_vk *)s;
    memcpy(s_priv->clear_color, color, sizeof(s_priv->clear_color));
}

static void vk_get_clear_color(struct gctx *s, float *color)
{
    struct gctx_vk *s_priv = (struct gctx_vk *)s;
    memcpy(color, &s_priv->clear_color, sizeof(s_priv->clear_color));
}

static void vk_clear_color(struct gctx *s)
{
    struct gctx_vk *s_priv = (struct gctx_vk *)s;
    ngli_gctx_vk_commit_render_pass(s);

    VkClearAttachment clear_attachments = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .colorAttachment = 0,
        .clearValue = {
            .color={.float32={s_priv->clear_color[0], s_priv->clear_color[1], s_priv->clear_color[2], s_priv->clear_color[3]}},
        },
    };

    VkClearRect clear_rect = {
        .rect = {
            .offset = {0},
            .extent = s_priv->cur_render_area,
        },
        .baseArrayLayer = 0,
        .layerCount = 1,
    };

    VkCommandBuffer cmd_buf = s_priv->cur_command_buffer;
    vkCmdClearAttachments(cmd_buf, 1, &clear_attachments, 1, &clear_rect);
}

static void vk_clear_depth_stencil(struct gctx *s)
{
    struct gctx_vk *s_priv = (struct gctx_vk *)s;

    if (s_priv->rendertarget) {
        struct rendertarget_params *params = &s_priv->rendertarget->params;
        if (!params->depth_stencil.attachment)
            return;
    }

    ngli_gctx_vk_commit_render_pass(s);

    // FIXME: no-oop if there is not depth stencil

    VkClearAttachment clear_attachments = {
        .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
        .clearValue = {
            .depthStencil = {
                .depth = 1.0f,
                .stencil = 0,
            },
        },
    };

    VkClearRect clear_rect = {
        .rect = {
            .offset = {0},
            .extent = s_priv->cur_render_area,
        },
        .baseArrayLayer = 0,
        .layerCount = 1,
    };

    VkCommandBuffer cmd_buf = s_priv->cur_command_buffer;
    vkCmdClearAttachments(cmd_buf, 1, &clear_attachments, 1, &clear_rect);
}

static void vk_invalidate_depth_stencil(struct gctx *s)
{
    /* TODO: it needs change of API to implement this feature on Vulkan */
}

static void vk_flush(struct gctx *s)
{
    struct gctx_vk *s_priv = (struct gctx_vk *)s;
    struct vkcontext *vk = s_priv->vkcontext;

    VkCommandBuffer cmd_buf = s_priv->cur_command_buffer;
    if (s_priv->cur_render_pass && s_priv->cur_render_pass_state == 1) {
        vkCmdEndRenderPass(cmd_buf);
        s_priv->cur_render_pass_state = 0;
    }

        VkResult vkret = vkEndCommandBuffer(cmd_buf);
        if (vkret != VK_SUCCESS)
            return;
        s_priv->cur_command_buffer_state = 0;

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = ngli_darray_count(&s_priv->wait_semaphores),
        .pWaitSemaphores = ngli_darray_data(&s_priv->wait_semaphores),
        .pWaitDstStageMask = ngli_darray_data(&s_priv->wait_stages),
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd_buf,
        .signalSemaphoreCount = ngli_darray_count(&s_priv->signal_semaphores),
        .pSignalSemaphores = ngli_darray_data(&s_priv->signal_semaphores),
    };

    VkResult ret = vkQueueSubmit(vk->graphic_queue, 1, &submit_info, s_priv->fences[s_priv->current_frame]);
    if (ret != VK_SUCCESS) {
        return;
    }

    s_priv->wait_semaphores.count = 0;
    s_priv->wait_stages.count = 0;
    //s_priv->command_buffers.count = 0;
}

static int vk_get_prefered_depth_format(struct gctx *s)
{
    return NGLI_FORMAT_D16_UNORM;
}

static int vk_get_prefered_depth_stencil_format(struct gctx *s)
{
    struct gctx_vk *s_priv = (struct gctx_vk *)s;
    struct vkcontext *vk = s_priv->vkcontext;
    return vk->prefered_depth_stencil_format;
}

const struct gctx_class ngli_gctx_vk = {
    .name         = "Vulkan",
    .create       = vk_create,
    .init         = vk_init,
    .resize       = vk_resize,
    .pre_draw     = vk_pre_draw,
    .post_draw    = vk_post_draw,
    .wait_idle    = vk_wait_idle,
    .destroy      = vk_destroy,

    .set_rendertarget         = vk_set_rendertarget,
    .get_rendertarget         = vk_get_rendertarget,
    .set_viewport             = vk_set_viewport,
    .get_viewport             = vk_get_viewport,
    .set_scissor              = vk_set_scissor,
    .get_scissor              = vk_get_scissor,
    .set_clear_color          = vk_set_clear_color,
    .get_clear_color          = vk_get_clear_color,
    .clear_color              = vk_clear_color,
    .clear_depth_stencil      = vk_clear_depth_stencil,
    .invalidate_depth_stencil = vk_invalidate_depth_stencil,
    .get_prefered_depth_format= vk_get_prefered_depth_format,
    .get_prefered_depth_stencil_format=vk_get_prefered_depth_stencil_format,
    .flush                    = vk_flush,

    .buffer_create = ngli_buffer_vk_create,
    .buffer_init   = ngli_buffer_vk_init,
    .buffer_upload = ngli_buffer_vk_upload,
    .buffer_freep  = ngli_buffer_vk_freep,

    .gtimer_create = ngli_gtimer_vk_create,
    .gtimer_init   = ngli_gtimer_vk_init,
    .gtimer_start  = ngli_gtimer_vk_start,
    .gtimer_stop   = ngli_gtimer_vk_stop,
    .gtimer_read   = ngli_gtimer_vk_read,
    .gtimer_freep  = ngli_gtimer_vk_freep,

    .pipeline_create         = ngli_pipeline_vk_create,
    .pipeline_init           = ngli_pipeline_vk_init,
    .pipeline_update_uniform = ngli_pipeline_vk_update_uniform,
    .pipeline_update_texture = ngli_pipeline_vk_update_texture,
    .pipeline_exec           = ngli_pipeline_vk_exec,
    .pipeline_freep          = ngli_pipeline_vk_freep,

    .program_create = ngli_program_vk_create,
    .program_init   = ngli_program_vk_init,
    .program_freep  = ngli_program_vk_freep,

    .rendertarget_create      = ngli_rendertarget_vk_create,
    .rendertarget_init        = ngli_rendertarget_vk_init,
    .rendertarget_blit        = ngli_rendertarget_vk_blit,
    .rendertarget_resolve     = ngli_rendertarget_vk_resolve,
    .rendertarget_read_pixels = ngli_rendertarget_vk_read_pixels,
    .rendertarget_freep       = ngli_rendertarget_vk_freep,

    .texture_create           = ngli_texture_vk_create,
    .texture_init             = ngli_texture_vk_init,
    .texture_has_mipmap       = ngli_texture_vk_has_mipmap,
    .texture_match_dimensions = ngli_texture_vk_match_dimensions,
    .texture_upload           = ngli_texture_vk_upload,
    .texture_generate_mipmap  = ngli_texture_vk_generate_mipmap,
    .texture_freep            = ngli_texture_vk_freep,
};
