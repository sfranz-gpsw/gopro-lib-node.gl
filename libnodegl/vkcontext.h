/*
 * Copyright 2016 GoPro Inc.
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

#ifndef VKCONTEXT_H
#define VKCONTEXT_H

#include <stdlib.h>

#include <shaderc/shaderc.h>
#include <vulkan/vulkan.h>

#if defined(TARGET_LINUX)
#include <X11/Xlib.h>
#endif

#if defined(HAVE_WAYLAND)
#include <wayland-client.h>
#endif

#include "darray.h"
#include "nodegl.h"
#include "rendertarget.h"
#include "texture.h"

struct vk_swapchain_support {
    VkSurfaceCapabilitiesKHR caps;
    VkSurfaceFormatKHR *formats;
    uint32_t nb_formats;
    VkPresentModeKHR *present_modes;
    uint32_t nb_present_modes;
};

#define VK_FUN(name) PFN_vk##name

#define VK_LOAD_FUN(instance, name) \
    VK_FUN(name) name = (VK_FUN(name))vkGetInstanceProcAddr(instance, "vk" #name);

struct vkcontext {
    uint32_t api_version;
    VkInstance instance;
    VkLayerProperties *layers;
    uint32_t nb_layers;
    VkExtensionProperties *extensions;
    uint32_t nb_extensions;
    VkDebugUtilsMessengerEXT debug_callback;
    VkSurfaceKHR surface;

    VkExtensionProperties *device_extensions;
    uint32_t nb_device_extensions;

    int own_display;
#if defined(TARGET_LINUX)
    Display *display;
#endif

    VkPhysicalDevice *phy_devices;
    uint32_t nb_phy_devices;
    VkPhysicalDevice phy_device;
    VkPhysicalDeviceProperties phy_device_props;
    uint32_t graphics_queue_index;
    uint32_t present_queue_index;
    VkQueue graphic_queue;
    VkQueue present_queue;
    VkDevice device;

    VkFormat prefered_depth_format;
    VkFormat prefered_depth_stencil_format;

    VkPhysicalDeviceFeatures dev_features;
    VkPhysicalDeviceMemoryProperties phydev_mem_props;

    VkSurfaceCapabilitiesKHR surface_caps;
    VkSurfaceFormatKHR *surface_formats;
    uint32_t nb_surface_formats;
    VkPresentModeKHR *present_modes;
    uint32_t nb_present_modes;

    /* User options */
    int platform;
    int backend;
    int offscreen;
    int width;
    int height;
    int samples;

    /* Vulkan api */
    int version;

    VkDebugUtilsMessengerEXT report_callback;
    VkPhysicalDevice physical_device;
    int queue_family_graphics_id;
    int queue_family_present_id;
    struct vk_swapchain_support swapchain_support;
    VkSurfaceFormatKHR surface_format;
    VkPresentModeKHR present_mode;
    VkSwapchainKHR swapchain;

    VkFormat prefered_vk_depth_stencil_format;
};

struct vkcontext *ngli_vkcontext_create(void);
int ngli_vkcontext_init(struct vkcontext *s, const struct ngl_config *config);
void *ngli_vkcontext_get_proc_addr(struct vkcontext *vk, const char *name);
VkFormat ngli_vkcontext_find_supported_format(struct vkcontext *s, const VkFormat *formats,
                                              VkImageTiling tiling, VkFormatFeatureFlags features);
void ngli_vkcontext_freep(struct vkcontext **sp);

int ngli_vk_find_memory_type(struct vkcontext *vk, uint32_t type_filter, VkMemoryPropertyFlags props);

#endif /* VKCONTEXT_H */
