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

#define LIBVULKAN_SONAME	"libvulkan.so.1"
#define SKIP_MESSAGE_TIMES	(60)

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

};


static struct libvulkan_functions* vulkan = NULL;
static struct capture_context* capture = NULL;
static VkImage swapchain_images[3] = {0};
static int swapchain_index = 0;
static int nr_swapchain_images = 0;


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
	if(!vulkan)
		vulkan = libvulkan_functions_init();

	prompt_hook();
	return vulkan->vkCreateInstance(pCreateInfo, pAllocator, pInstance);
}


VKAPI_ATTR void VKAPI_CALL vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator)
{
	prompt_hook();
	vulkan->vkDestroyInstance(instance, pAllocator);

	libvulkan_functions_deinit(vulkan);
	vulkan = NULL;

	return;
}


VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice)
{
	VkResult result = VK_SUCCESS;

	prompt_hook();
	result = vulkan->vkCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);

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
	return vulkan->vkCreateXcbSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
}


VKAPI_ATTR VkBool32 VKAPI_CALL vkGetPhysicalDeviceXcbPresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, xcb_connection_t* connection, xcb_visualid_t visual_id)
{
	prompt_hook();
	return vulkan->vkGetPhysicalDeviceXcbPresentationSupportKHR(physicalDevice, queueFamilyIndex, connection, visual_id);
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
	vulkan->vkDestroySurfaceKHR(instance, surface, pAllocator);
}


VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, VkSurfaceKHR surface, VkBool32* pSupported)
{
	prompt_hook();
	return vulkan->vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, surface, pSupported);
}


VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR* pSurfaceCapabilities)
{
	prompt_hook();
	return vulkan->vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, pSurfaceCapabilities);
}


VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t* pSurfaceFormatCount, VkSurfaceFormatKHR* pSurfaceFormats)
{
	prompt_hook();
	return vulkan->vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, pSurfaceFormatCount, pSurfaceFormats);
}


VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t* pPresentModeCount, VkPresentModeKHR* pPresentModes)
{
	prompt_hook();
	return vulkan->vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, pPresentModeCount, pPresentModes);
}


VKAPI_ATTR VkResult VKAPI_CALL vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks*pAllocator, VkSwapchainKHR* pSwapchain)
{
	prompt_hook();

	capture_context_init_image(capture, pCreateInfo->imageFormat, pCreateInfo->imageExtent.width, pCreateInfo->imageExtent.height);

	return vulkan->vkCreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
}


VKAPI_ATTR void VKAPI_CALL vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks* pAllocator)
{
	prompt_hook();

	capture_context_destroy_image(capture);

	vulkan->vkDestroySwapchainKHR(device, swapchain, pAllocator);
}


VKAPI_ATTR VkResult VKAPI_CALL vkGetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t* pSwapchainImageCount, VkImage* pSwapchainImages)
{
	VkResult result = VK_SUCCESS;

	prompt_hook();

	result = vulkan->vkGetSwapchainImagesKHR(device, swapchain, pSwapchainImageCount, pSwapchainImages);

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

	result = vulkan->vkAcquireNextImageKHR(device, swapchain, timeout, semaphore, fence, pImageIndex);

	swapchain_index = *pImageIndex;

	return result;
}


VKAPI_ATTR VkResult VKAPI_CALL vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pPresentInfo)
{
	static int counter = 0;

	if(counter == 0)
		prompt_hook();

	++counter;
	counter %= SKIP_MESSAGE_TIMES;

	capture_context_capture(capture, swapchain_images[swapchain_index]);
	capture_context_read_pixles(capture, NULL);

	return vulkan->vkQueuePresentKHR(queue, pPresentInfo);
}
