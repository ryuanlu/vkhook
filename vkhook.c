#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#ifdef __linux
#define VK_USE_PLATFORM_XLIB_KHR
#define VK_USE_PLATFORM_XCB_KHR
#endif

#include <vulkan/vulkan.h>

#include "capture.h"
#include "offscreen_swapchain.h"
#include "glwindow.h"

#define LIBVULKAN_SONAME	"libvulkan.so.1"
#define SKIP_MESSAGE_TIMES	(60)

#ifndef CONFIG_USE_OFFSCREEN_SWAPCHAIN
#define CONFIG_USE_OFFSCREEN_SWAPCHAIN	(0)
#endif

#ifndef CONFIG_USE_XPUTIMAGE
#define CONFIG_USE_XPUTIMAGE	(0)
#endif

#ifndef CONFIG_USE_XSHMPUTIMAGE
#define CONFIG_USE_XSHMPUTIMAGE	(0)
#endif

#define prompt(type)		fprintf(stderr, "%s%s\n", type, __FUNCTION__)
#define prompt_type(type)	prompt("["#type"] ")
#define prompt_hook()		prompt_type(HOOK)


struct libvulkan_functions
{
	void*				handle;

	PFN_vkCreateInstance		vkCreateInstance;
	PFN_vkDestroyInstance		vkDestroyInstance;

	PFN_vkCreateDevice		vkCreateDevice;
	PFN_vkDestroyDevice		vkDestroyDevice;

#ifdef __linux
	PFN_vkCreateXlibSurfaceKHR				vkCreateXlibSurfaceKHR;
	PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR	vkGetPhysicalDeviceXlibPresentationSupportKHR;

	PFN_vkCreateXcbSurfaceKHR				vkCreateXcbSurfaceKHR;
	PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR	vkGetPhysicalDeviceXcbPresentationSupportKHR;
#endif

#ifdef _WIN32
	PFN_vkCreateWin32SurfaceKHR				vkCreateWin32SurfaceKHR;
	PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR	vkGetPhysicalDeviceWin32PresentationSupportKHR;
#endif

	PFN_vkDestroySurfaceKHR		vkDestroySurfaceKHR;

	PFN_vkGetPhysicalDeviceSurfaceSupportKHR	vkGetPhysicalDeviceSurfaceSupportKHR;
	PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR	vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
	PFN_vkGetPhysicalDeviceSurfaceFormatsKHR	vkGetPhysicalDeviceSurfaceFormatsKHR;
	PFN_vkGetPhysicalDeviceSurfacePresentModesKHR	vkGetPhysicalDeviceSurfacePresentModesKHR;

	PFN_vkCreateSwapchainKHR	vkCreateSwapchainKHR;
	PFN_vkDestroySwapchainKHR	vkDestroySwapchainKHR;
	PFN_vkGetSwapchainImagesKHR	vkGetSwapchainImagesKHR;

	PFN_vkAcquireNextImageKHR	vkAcquireNextImageKHR;
	PFN_vkQueuePresentKHR		vkQueuePresentKHR;
	PFN_vkQueueSubmit		vkQueueSubmit;

	PFN_vkGetMemoryFdKHR		vkGetMemoryFdKHR;
};

static int use_offscreen_swapchain = CONFIG_USE_OFFSCREEN_SWAPCHAIN;
static int use_xputimage = CONFIG_USE_XPUTIMAGE;
static int use_xshmputimage = CONFIG_USE_XSHMPUTIMAGE;

static struct libvulkan_functions* vulkan = NULL;
static capture_context* capture = NULL;
static VkImage swapchain_images[3] = {0};
static int swapchain_index = 0;
static int nr_swapchain_images = 0;
static glwindow* gl = NULL;

/* libvulkan functions */

#define load_symbol(func)	vulkan->func = dlsym(vulkan->handle, #func)

struct libvulkan_functions* libvulkan_functions_init(void)
{
	struct libvulkan_functions* vulkan = NULL;
	vulkan = calloc(1, sizeof(struct libvulkan_functions));
	vulkan->handle = dlopen(LIBVULKAN_SONAME, RTLD_NOW);

	load_symbol(vkCreateInstance);
	load_symbol(vkDestroyInstance);

	load_symbol(vkCreateDevice);
	load_symbol(vkDestroyDevice);

#ifdef __linux
	load_symbol(vkCreateXlibSurfaceKHR);
	load_symbol(vkGetPhysicalDeviceXlibPresentationSupportKHR);

	load_symbol(vkCreateXcbSurfaceKHR);
	load_symbol(vkGetPhysicalDeviceXcbPresentationSupportKHR);
#endif
#ifdef _WIN32
	load_symbol(vkCreateWin32SurfaceKHR);
	load_symbol(vkGetPhysicalDeviceWin32PresentationSupportKHR);
#endif

	load_symbol(vkDestroySurfaceKHR);

	load_symbol(vkGetPhysicalDeviceSurfaceSupportKHR);
	load_symbol(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
	load_symbol(vkGetPhysicalDeviceSurfaceFormatsKHR);
	load_symbol(vkGetPhysicalDeviceSurfacePresentModesKHR);

	load_symbol(vkCreateSwapchainKHR);
	load_symbol(vkDestroySwapchainKHR);
	load_symbol(vkGetSwapchainImagesKHR);

	load_symbol(vkAcquireNextImageKHR);
	load_symbol(vkQueuePresentKHR);

	load_symbol(vkQueueSubmit);

	prompt("");
	return vulkan;
}

#undef load_symbol


void libvulkan_functions_deinit(struct libvulkan_functions* vulkan)
{
	dlclose(vulkan->handle);
	free(vulkan);
	prompt("");
}


/* Hooks */

VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance)
{
	VkResult result = VK_SUCCESS;
	VkInstanceCreateInfo info = {0};
	const char** extensions = NULL;
	char* original_display = getenv("DISPLAY");
	char* env_vhook_use_offscreen_swapchain = getenv("VKHOOK_USE_OFFSCREEN_SWAPCHAIN");

	use_offscreen_swapchain = env_vhook_use_offscreen_swapchain ? !strcmp("1", env_vhook_use_offscreen_swapchain) : use_offscreen_swapchain;

	if(!vulkan)
		vulkan = libvulkan_functions_init();

	if(!gl)
		gl = glwindow_create();

	prompt_hook();

	if(use_offscreen_swapchain)
	{
		// unsetenv("DISPLAY");
		setenv("DISPLAY", ":0", 1);
	}

	memcpy(&info, pCreateInfo, sizeof(VkInstanceCreateInfo));
	extensions = calloc(info.enabledExtensionCount + 1, sizeof(char*));
	memcpy(&extensions[0], info.ppEnabledExtensionNames, sizeof(char*) * info.enabledExtensionCount);

	info.enabledExtensionCount += 1;
	extensions[info.enabledExtensionCount - 1] = "VK_KHR_external_memory_capabilities";
	info.ppEnabledExtensionNames = extensions;

	result = vulkan->vkCreateInstance(pCreateInfo, pAllocator, pInstance);

	free(extensions);

	if(use_offscreen_swapchain)
	{
		setenv("DISPLAY", original_display, 1);
	}

	return result;
}


VKAPI_ATTR void VKAPI_CALL vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator)
{
	prompt_hook();
	vulkan->vkDestroyInstance(instance, pAllocator);
	glwindow_destroy(gl);
	libvulkan_functions_deinit(vulkan);
	vulkan = NULL;

	return;
}


VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice)
{
	VkDeviceCreateInfo info;
	const char** extensions = NULL;
	VkResult result = VK_SUCCESS;

	prompt_hook();

	memcpy(&info, pCreateInfo, sizeof(VkDeviceCreateInfo));
	extensions = calloc(info.enabledExtensionCount + 2, sizeof(char*));
	memcpy(&extensions[0], info.ppEnabledExtensionNames, sizeof(char*) * info.enabledExtensionCount);

	info.enabledExtensionCount += 2;
	extensions[info.enabledExtensionCount - 2] = " VK_KHR_external_memory";
	extensions[info.enabledExtensionCount - 1] = " VK_KHR_external_memory_fd";
	info.ppEnabledExtensionNames = extensions;

	result = vulkan->vkCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);

	free(extensions);

	vulkan->vkGetMemoryFdKHR = (PFN_vkGetMemoryFdKHR)vkGetDeviceProcAddr(*pDevice, "vkGetMemoryFdKHR");

	if(!capture)
		capture = capture_context_init(physicalDevice, *pDevice, pCreateInfo->pQueueCreateInfos->queueFamilyIndex);

	return result;
}


VKAPI_ATTR void VKAPI_CALL vkDestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator)
{
	prompt_hook();

	if(capture)
	{
		capture_context_deinit(capture);
		capture = NULL;
	}

	vulkan->vkDestroyDevice(device, pAllocator);
}


#ifdef __linux
VKAPI_ATTR VkResult VKAPI_CALL vkCreateXlibSurfaceKHR(VkInstance instance, const VkXlibSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface)
{
	prompt_hook();
	return vulkan->vkCreateXlibSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
}


VKAPI_ATTR VkBool32 VKAPI_CALL vkGetPhysicalDeviceXlibPresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, Display* dpy, VisualID visualID)
{
	prompt_hook();
	return vulkan->vkGetPhysicalDeviceXlibPresentationSupportKHR(physicalDevice, queueFamilyIndex, dpy, visualID);
}


VKAPI_ATTR VkResult VKAPI_CALL vkCreateXcbSurfaceKHR(VkInstance instance, const VkXcbSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface)
{
	prompt_hook();

	if(use_offscreen_swapchain)
	{
		*pSurface = (VkSurfaceKHR)OFFSCREEN_SWAPCHAIN_VKSURFACE_MAGIC;
		return VK_SUCCESS;
	}else
	{
		return vulkan->vkCreateXcbSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
	}
}


VKAPI_ATTR VkBool32 VKAPI_CALL vkGetPhysicalDeviceXcbPresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, xcb_connection_t* connection, xcb_visualid_t visual_id)
{
	prompt_hook();

	if(use_offscreen_swapchain)
	{
		return VK_TRUE;
	}else
	{
		return vulkan->vkGetPhysicalDeviceXcbPresentationSupportKHR(physicalDevice, queueFamilyIndex, connection, visual_id);
	}
}
#endif


#ifdef _WIN32
VKAPI_ATTR VkResult VKAPI_CALL vkCreateWin32SurfaceKHR(VkInstance instance, const VkWin32SurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface)
{
	prompt_hook();
	return vulkan->vkCreateWin32SurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
}

VKAPI_ATTR VkBool32 VKAPI_CALL vkGetPhysicalDeviceWin32PresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex)
{
	prompt_hook();
	return vulkan->vkGetPhysicalDeviceWin32PresentationSupportKHR(physicalDevice, queueFamilyIndex);
}
#endif


VKAPI_ATTR void VKAPI_CALL vkDestroySurfaceKHR(VkInstance  instance, VkSurfaceKHR surface, const VkAllocationCallbacks* pAllocator)
{
	prompt_hook();
	if(use_offscreen_swapchain)
	{
		if(surface != (VkSurfaceKHR)OFFSCREEN_SWAPCHAIN_VKSURFACE_MAGIC)
			fprintf(stderr, "surface (%p) != OFFSCREEN_SWAPCHAIN_VKSURFACE_MAGIC", surface);
	}else
	{
		vulkan->vkDestroySurfaceKHR(instance, surface, pAllocator);
	}
}


VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, VkSurfaceKHR surface, VkBool32* pSupported)
{
	prompt_hook();

	if(use_offscreen_swapchain)
	{
		*pSupported = VK_TRUE;
		return VK_SUCCESS;
	}else
	{
		return vulkan->vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, surface, pSupported);
	}
}


VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR* pSurfaceCapabilities)
{
	prompt_hook();
	if(use_offscreen_swapchain)
	{
		*pSurfaceCapabilities = (VkSurfaceCapabilitiesKHR)
		{
			.supportedCompositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		};
		return VK_SUCCESS;
	}else
	{
		return vulkan->vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, pSurfaceCapabilities);
	}
}


VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t* pSurfaceFormatCount, VkSurfaceFormatKHR* pSurfaceFormats)
{
	prompt_hook();

	if(use_offscreen_swapchain)
	{
		VkSurfaceFormatKHR formats[] =
		{
			{
				.format = VK_FORMAT_R8G8B8A8_UNORM,
				.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
			},
			{
				.format = VK_FORMAT_B8G8R8A8_UNORM,
				.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
			},
			{
				.format = VK_FORMAT_R8G8B8A8_SRGB,
				.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
			},
			{
				.format = VK_FORMAT_B8G8R8A8_SRGB,
				.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
			},
		};

		*pSurfaceFormatCount = 4;
		if(pSurfaceFormats)
			memcpy(pSurfaceFormats, formats, sizeof(formats));
		return VK_SUCCESS;
	}else
	{
		return vulkan->vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, pSurfaceFormatCount, pSurfaceFormats);
	}
}


VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t* pPresentModeCount, VkPresentModeKHR* pPresentModes)
{
	prompt_hook();

	if(use_offscreen_swapchain)
	{
		*pPresentModeCount = 1;

		if(pPresentModes)
			*pPresentModes = VK_PRESENT_MODE_FIFO_KHR;

		return VK_SUCCESS;
	}else
	{
		return vulkan->vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, pPresentModeCount, pPresentModes);
	}
}


VKAPI_ATTR VkResult VKAPI_CALL vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks*pAllocator, VkSwapchainKHR* pSwapchain)
{
	prompt_hook();

	capture_context_init_image(capture, pCreateInfo->imageFormat, pCreateInfo->imageExtent.width, pCreateInfo->imageExtent.height);

	if(use_xputimage)
		glwindow_set_ximage_size(gl, pCreateInfo->imageExtent.width, pCreateInfo->imageExtent.height);
	else if(use_xshmputimage)
		glwindow_set_xshm_size(gl, pCreateInfo->imageExtent.width, pCreateInfo->imageExtent.height);
	else
		glwindow_set_fbo_size(gl, pCreateInfo->imageExtent.width, pCreateInfo->imageExtent.height);


	if(use_offscreen_swapchain)
	{
		*pSwapchain = (VkSwapchainKHR)offscreen_swapchain_create(capture_context_get_physical_device(capture), device, pCreateInfo->minImageCount, VK_FORMAT_R8G8B8A8_UNORM, pCreateInfo->imageExtent.width, pCreateInfo->imageExtent.height);
		return VK_SUCCESS;
	}else
	{
		return vulkan->vkCreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
	}
}


VKAPI_ATTR void VKAPI_CALL vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks* pAllocator)
{
	prompt_hook();

	capture_context_destroy_image(capture);

	if(use_offscreen_swapchain)
	{
		offscreen_swapchain_destroy(device, (offscreen_swapchain*)swapchain);
	}else
	{
		vulkan->vkDestroySwapchainKHR(device, swapchain, pAllocator);
	}

	memset(swapchain_images, 0, sizeof(VkImage) * nr_swapchain_images);
	nr_swapchain_images = 0;
}


VKAPI_ATTR VkResult VKAPI_CALL vkGetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t* pSwapchainImageCount, VkImage* pSwapchainImages)
{
	VkResult result = VK_SUCCESS;

	prompt_hook();

	if(use_offscreen_swapchain)
	{
		offscreen_swapchain_get_images((offscreen_swapchain*)swapchain, pSwapchainImageCount, pSwapchainImages);
	}else
	{
		result = vulkan->vkGetSwapchainImagesKHR(device, swapchain, pSwapchainImageCount, pSwapchainImages);
	}

	if(pSwapchainImages)
	{
		nr_swapchain_images = *pSwapchainImageCount;
		memcpy(&swapchain_images, pSwapchainImages, sizeof(VkImage) * nr_swapchain_images);
	}

	return result;
}


VKAPI_ATTR VkResult VKAPI_CALL vkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t* pImageIndex)
{
	VkResult result = VK_SUCCESS;
	static int counter = 0;

	if(counter == 0)
		prompt_hook();

	++counter;
	counter %= SKIP_MESSAGE_TIMES;

	if(use_offscreen_swapchain)
	{
		*pImageIndex = offscreen_swapchain_acquire_next_image((offscreen_swapchain*)swapchain);
	}else
	{
		result = vulkan->vkAcquireNextImageKHR(device, swapchain, timeout, semaphore, fence, pImageIndex);
	}

	swapchain_index = *pImageIndex;

	return result;
}


VKAPI_ATTR VkResult VKAPI_CALL vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pPresentInfo)
{
	const char* pixels = NULL;
	static int counter = 0;

	if(counter == 0)
		prompt_hook();

	capture_context_capture(capture, swapchain_images[swapchain_index]);
	// capture_context_read_pixles(capture, NULL);

#ifndef CONFIG_USE_GL_DRAW_VKIMAGE_NV
	pixels = capture_context_map_image(capture);

	if(use_xputimage)
		glwindow_xputimage(gl, pixels);
	else if(use_xshmputimage)
		glwindow_xshmputimage(gl, pixels);
	else
		glwindow_blit(gl, pixels);

	capture_context_unmap_image(capture);
#else
	glwindow_vkimage_blit(gl, capture_context_get_vkimage(capture));
#endif

	++counter;
	counter %= SKIP_MESSAGE_TIMES;

	if(use_offscreen_swapchain)
	{
		offscreen_swapchain_present((offscreen_swapchain*)*pPresentInfo->pSwapchains);
		return VK_SUCCESS;
	}else
	{
		return vulkan->vkQueuePresentKHR(queue, pPresentInfo);
	}
}


VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence)
{
	VkSubmitInfo info = *pSubmits;

	if(use_offscreen_swapchain)
	{
		info.waitSemaphoreCount = 0;
		info.signalSemaphoreCount = 0;
	}

	return vulkan->vkQueueSubmit(queue, submitCount, &info, fence);
}


VKAPI_ATTR VkResult VKAPI_CALL vkGetMemoryFdKHR(VkDevice device, const VkMemoryGetFdInfoKHR* pGetFdInfo, int* pFd)
{
	if(vulkan->vkGetMemoryFdKHR)
	{
		return vulkan->vkGetMemoryFdKHR(device, pGetFdInfo, pFd);
	}else
	{
		fprintf(stderr, "vkGetMemoryFdKHR == %p\n", vulkan->vkGetMemoryFdKHR);
		*pFd = -1;
		return VK_ERROR_FEATURE_NOT_PRESENT;
	}
}
