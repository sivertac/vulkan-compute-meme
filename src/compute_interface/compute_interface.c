#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "compute_interface.h"

static const char *validation_layers[] = {"VK_LAYER_KHRONOS_validation"};
static const size_t num_validation_layers = 1;

static VkResult create_debug_utils_messenger_ext(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger)
{
    PFN_vkCreateDebugUtilsMessengerEXT func =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != NULL) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

static void
destroy_debug_utils_messenger_ext(VkInstance instance,
                                  VkDebugUtilsMessengerEXT debugMessenger,
                                  const VkAllocationCallbacks *pAllocator)
{
    PFN_vkDestroyDebugUtilsMessengerEXT func =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != NULL) {
        func(instance, debugMessenger, pAllocator);
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
{
    (void)messageSeverity;
    (void)messageType;
    (void)pUserData;
    fprintf(stderr, "validation layer: %s\n", pCallbackData->pMessage);

    return VK_FALSE;
}

static void populate_debug_messenger_create_info(
    VkDebugUtilsMessengerCreateInfoEXT *createInfo)
{
    memset(createInfo, 0, sizeof(VkDebugUtilsMessengerCreateInfoEXT));
    createInfo->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo->messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo->pfnUserCallback = debug_callback;
}

static VkResult setup_debug_messenger(VkInstance instance,
                                      VkDebugUtilsMessengerEXT *debug_messenger)
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populate_debug_messenger_create_info(&createInfo);

    return create_debug_utils_messenger_ext(instance, &createInfo, NULL,
                                            debug_messenger);
}

static bool check_validation_layer_support()
{
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, NULL);

    VkLayerProperties *available_layers = NULL;
    available_layers =
        (VkLayerProperties *)malloc(layer_count * sizeof(VkLayerProperties));

    vkEnumerateInstanceLayerProperties(&layer_count, available_layers);

    for (size_t layer_index = 0; layer_index < num_validation_layers;
         ++layer_index) {
        const char *layer_name = validation_layers[layer_index];
        bool layer_found = false;

        for (size_t i = 0; i < layer_count; ++i) {
            VkLayerProperties *layer_properties = &available_layers[i];
            if (strcmp(layer_name, layer_properties->layerName) == 0) {
                layer_found = true;
                break;
            }
        }

        if (!layer_found) {
            free(available_layers);
            return false;
        }
    }
    free(available_layers);
    return true;
}

static VkResult create_instance(bool enable_validation_layers,
                                VkInstance *instance)
{
    *instance = VK_NULL_HANDLE;

    VkApplicationInfo app_info = {0};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Compute Shader Meme";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    if (enable_validation_layers) {
        const char *extention_names[] = {VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
        create_info.enabledExtensionCount = 1;
        create_info.ppEnabledExtensionNames = extention_names;
        create_info.enabledLayerCount = num_validation_layers;
        create_info.ppEnabledLayerNames = validation_layers;
    }

    return vkCreateInstance(&create_info, NULL, instance);
}

static VkResult pick_physical_device(VkInstance instance,
                                     VkPhysicalDevice *physical_device)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);

    if (deviceCount == 0) {
        fprintf(stderr, "No Vulkan-compatible physical devices found!\n");
        return VK_ERROR_UNKNOWN;
    }

    VkPhysicalDevice *devices =
        (VkPhysicalDevice *)malloc(deviceCount * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices);

    // print device names and memory sizes
    for (uint32_t i = 0; i < deviceCount; i++) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);
        printf("Device %d: %s\n", i, deviceProperties.deviceName);
    }

    printf("Selecting device at index 0\n");
    *physical_device = devices[0]; // Select the first available device

    free(devices);
    return VK_SUCCESS;
}

static VkResult create_logical_device(VkPhysicalDevice physical_device,
                                      VkDevice *device)
{
    float queuePriority = 1.0f; // Priority of the compute queue (0.0 to 1.0)

    VkDeviceQueueCreateInfo queueCreateInfo = {0};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex =
        0; // Assuming compute queue is in the first family
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures physical_device_features = {0};
    physical_device_features.shaderInt64 = true;

    VkDeviceCreateInfo deviceCreateInfo = {0};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.pEnabledFeatures = &physical_device_features;

    return vkCreateDevice(physical_device, &deviceCreateInfo, NULL, device);
}

static VkResult create_shader_module(VkDevice device, const char *shader_source,
                                     size_t shader_source_size,
                                     VkShaderModule *shader_module)
{
    // Create shader module
    VkShaderModuleCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shader_source_size;
    createInfo.pCode = (const uint32_t *)shader_source;

    return vkCreateShaderModule(device, &createInfo, NULL, shader_module);
}

static VkResult
create_descriptor_set_layout(VkDevice device, uint32_t num_bindings,
                             VkDescriptorSetLayout *descriptor_set_layout,
                             VkDescriptorType type)
{

    VkDescriptorSetLayoutBinding *bindings = NULL;
    bindings = malloc(sizeof(VkDescriptorSetLayoutBinding) * num_bindings);
    if (bindings == NULL) {
        return VK_ERROR_UNKNOWN;
    }
    memset(bindings, 0, sizeof(VkDescriptorSetLayoutBinding) * num_bindings);

    for (size_t i = 0; i < num_bindings; ++i) {
        VkDescriptorSetLayoutBinding *inputBinding = &bindings[i];
        inputBinding->binding = i;
        inputBinding->descriptorType = type;
        inputBinding->descriptorCount = 1;
        inputBinding->stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo = {0};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = num_bindings;
    layoutInfo.pBindings = bindings;

    VkResult res = vkCreateDescriptorSetLayout(device, &layoutInfo, NULL,
                                               descriptor_set_layout);
    free(bindings);
    return res;
}

static VkResult
create_descriptor_pool(VkDevice device,
                       VkDescriptorPoolSize *descriptor_pool_sizes,
                       uint32_t num_descriptor_pool_sizes, uint32_t max_sets,
                       VkDescriptorPool *descriptor_pool)
{

    VkDescriptorPoolCreateInfo poolInfo;
    memset(&poolInfo, 0, sizeof(VkDescriptorPoolCreateInfo));
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = num_descriptor_pool_sizes;
    poolInfo.pPoolSizes = descriptor_pool_sizes;
    poolInfo.maxSets = max_sets;

    return vkCreateDescriptorPool(device, &poolInfo, NULL, descriptor_pool);
}

static VkResult create_compute_pipeline_impl(
    VkDevice device, const VkDescriptorSetLayout *descriptor_set_layouts,
    uint32_t num_descriptor_set_layouts, VkShaderModule shader_module,
    const VkSpecializationInfo *specialization_info,
    VkPipelineLayout *pipeline_layout, VkPipeline *compute_pipeline)
{
    VkResult res = VK_SUCCESS;

    VkPipelineLayoutCreateInfo layoutInfo = {0};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = num_descriptor_set_layouts;
    layoutInfo.pSetLayouts = descriptor_set_layouts;

    res = vkCreatePipelineLayout(device, &layoutInfo, NULL, pipeline_layout);
    if (res != VK_SUCCESS) {
        return res;
    }

    VkComputePipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = *pipeline_layout;

    VkPipelineShaderStageCreateInfo stageInfo = {0};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = shader_module;
    stageInfo.pName = "main";
    stageInfo.pSpecializationInfo = specialization_info;

    pipelineInfo.stage = stageInfo;

    res = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                   NULL, compute_pipeline);
    if (res != VK_SUCCESS) {
        return res;
    }

    return res;
}

static VkResult create_command_buffer(VkDevice device,
                                      VkCommandPool *command_pool,
                                      VkCommandBuffer *command_buffer)
{
    VkResult res = VK_SUCCESS;

    VkCommandPoolCreateInfo poolInfo = {0};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex =
        0; // Replace with the appropriate queue family index
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    res = vkCreateCommandPool(device, &poolInfo, NULL, command_pool);
    if (res != VK_SUCCESS) {
        return res;
    }

    VkCommandBufferAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = *command_pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    res = vkAllocateCommandBuffers(device, &allocInfo, command_buffer);
    if (res != VK_SUCCESS) {
        return res;
    }

    return res;
}

static VkResult find_memory_type(VkPhysicalDevice physical_device,
                                 uint32_t type_filter,
                                 VkMemoryPropertyFlags properties,
                                 uint32_t *output_index)
{
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) &&
            (mem_properties.memoryTypes[i].propertyFlags & properties) ==
                properties) {
            *output_index = i;
            return VK_SUCCESS;
        }
    }

    return VK_ERROR_UNKNOWN;
}

VkResult create_compute_device(bool enable_validation_layers,
                               ComputeDevice *compute_device)
{
    VkResult res = VK_SUCCESS;

    // zero init
    memset(compute_device, 0, sizeof(ComputeDevice));

    // check if validation layers are supported
    compute_device->m_validation_layers_enabled = enable_validation_layers;
    if (compute_device->m_validation_layers_enabled) {
        if (!check_validation_layer_support()) {
            fprintf(stderr, "Validation layers not supported\n");
            return VK_ERROR_UNKNOWN;
        }
    }

    // get vulkan instance
    res =
        create_instance(enable_validation_layers, &compute_device->m_instance);
    if (res != VK_SUCCESS) {
        return res;
    }

    // enable validation layers
    if (compute_device->m_validation_layers_enabled) {
        setup_debug_messenger(compute_device->m_instance,
                              &compute_device->m_debug_messenger);
    }

    // pick physical device
    res = pick_physical_device(compute_device->m_instance,
                               &compute_device->m_physical_device);
    if (res != VK_SUCCESS) {
        return res;
    }

    // create logical device
    res = create_logical_device(compute_device->m_physical_device,
                                &compute_device->m_device);
    if (res != VK_SUCCESS) {
        return res;
    }

    return res;
}

void destroy_compute_device(ComputeDevice *compute_device)
{
    vkDestroyDevice(compute_device->m_device, NULL);

    if (compute_device->m_validation_layers_enabled) {
        destroy_debug_utils_messenger_ext(compute_device->m_instance,
                                          compute_device->m_debug_messenger,
                                          NULL);
    }

    vkDestroyInstance(compute_device->m_instance, NULL);
}

VkResult create_compute_pipeline(
    const ComputeDevice *compute_device, const char *shader_source,
    const size_t shader_source_size, const int num_input_buffers,
    const int num_output_buffers, const int num_uniform_buffers,
    const VkSpecializationInfo *specialization_info,
    ComputePipeline *compute_pipeline)
{
    VkResult res = VK_SUCCESS;

    // zero init
    memset(compute_pipeline, 0, sizeof(ComputePipeline));

    // get queue
    vkGetDeviceQueue(
        compute_device->m_device, 0, 0,
        &compute_pipeline->m_queue); // Queue family index 0, queue index 0

    // create descriptor set layouts
    res = create_descriptor_set_layout(
        compute_device->m_device, num_input_buffers,
        &compute_pipeline->m_input_descriptor_set_layout,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    if (res != VK_SUCCESS) {
        return res;
    }
    compute_pipeline->m_num_input_bindings = num_input_buffers;

    res = create_descriptor_set_layout(
        compute_device->m_device, num_output_buffers,
        &compute_pipeline->m_output_descriptor_set_layout,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    if (res != VK_SUCCESS) {
        return res;
    }
    compute_pipeline->m_num_output_bindings = num_output_buffers;

    res = create_descriptor_set_layout(
        compute_device->m_device, num_uniform_buffers,
        &compute_pipeline->m_uniform_descriptor_set_layout,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    if (res != VK_SUCCESS) {
        return res;
    }
    compute_pipeline->m_num_uniform_bindings = num_uniform_buffers;

    const uint32_t num_descriptor_set_layouts = 3;
    VkDescriptorSetLayout descriptor_set_layouts[] = {
        compute_pipeline->m_input_descriptor_set_layout,
        compute_pipeline->m_output_descriptor_set_layout,
        compute_pipeline->m_uniform_descriptor_set_layout};

    // create descriptor pool
    const int num_descriptor_types = 2;
    VkDescriptorPoolSize descriptor_pool_sizes[num_descriptor_types];
    memset(descriptor_pool_sizes, 0,
           sizeof(VkDescriptorPoolSize) * num_descriptor_types);
    descriptor_pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_pool_sizes[0].descriptorCount =
        (uint32_t)(num_input_buffers + num_output_buffers +
                   num_uniform_buffers);
    descriptor_pool_sizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptor_pool_sizes[1].descriptorCount =
        (uint32_t)(num_input_buffers + num_output_buffers +
                   num_uniform_buffers);
    res = create_descriptor_pool(
        compute_device->m_device, descriptor_pool_sizes, num_descriptor_types,
        num_descriptor_set_layouts, &compute_pipeline->m_descriptor_pool);
    if (res != VK_SUCCESS) {
        return res;
    }

    // create shader module
    res = create_shader_module(compute_device->m_device, shader_source,
                               shader_source_size,
                               &compute_pipeline->m_shader_module);
    if (res != VK_SUCCESS) {
        return res;
    }

    // create compute pipeline
    res = create_compute_pipeline_impl(
        compute_device->m_device, descriptor_set_layouts,
        num_descriptor_set_layouts, compute_pipeline->m_shader_module,
        specialization_info, &compute_pipeline->m_pipeline_layout,
        &compute_pipeline->m_compute_pipeline);
    if (res != VK_SUCCESS) {
        return res;
    }

    // create command buffer
    res = create_command_buffer(compute_device->m_device,
                                &compute_pipeline->m_command_pool,
                                &compute_pipeline->m_command_buffer);
    if (res != VK_SUCCESS) {
        return res;
    }

    return res;
}

static VkResult update_descriptor_set(VkDevice device,
                                      VkDescriptorSet descriptor_set,
                                      VkDescriptorType descriptor_type,
                                      uint32_t num_bindings, VkBuffer *buffers,
                                      VkDeviceSize *buffer_sizes)
{
    VkResult res = VK_SUCCESS;

    // Update input descriptor sets
    VkWriteDescriptorSet *descriptor_writes = NULL;
    size_t num_descriptor_writes = num_bindings;
    descriptor_writes = (VkWriteDescriptorSet *)malloc(
        num_descriptor_writes * sizeof(VkWriteDescriptorSet));
    if (descriptor_writes == NULL) {
        return VK_ERROR_UNKNOWN;
    }
    memset(descriptor_writes, 0,
           num_descriptor_writes * sizeof(VkWriteDescriptorSet));

    VkDescriptorBufferInfo *descriptor_buffer_infos;
    descriptor_buffer_infos = (VkDescriptorBufferInfo *)malloc(
        num_descriptor_writes * sizeof(VkDescriptorBufferInfo));
    if (descriptor_buffer_infos == NULL) {
        return VK_ERROR_UNKNOWN;
    }
    memset(descriptor_buffer_infos, 0,
           num_descriptor_writes * sizeof(VkDescriptorBufferInfo));

    for (uint32_t i = 0; i < num_descriptor_writes; ++i) {
        descriptor_buffer_infos[i].buffer = buffers[i];
        descriptor_buffer_infos[i].offset = 0;
        descriptor_buffer_infos[i].range = buffer_sizes[i];

        descriptor_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[i].dstSet = descriptor_set;
        descriptor_writes[i].dstBinding = i;
        descriptor_writes[i].dstArrayElement = 0;
        descriptor_writes[i].descriptorType = descriptor_type;
        descriptor_writes[i].descriptorCount = 1;
        descriptor_writes[i].pBufferInfo = &descriptor_buffer_infos[i];
    }

    vkUpdateDescriptorSets(device, num_descriptor_writes, descriptor_writes, 0,
                           NULL);

    free(descriptor_buffer_infos);
    free(descriptor_writes);

    return res;
}

VkResult update_compute_descriptor_sets(
    const ComputeDevice *compute_device,
    const ComputePipeline *compute_pipeline, const ComputeBuffer *input_buffers,
    const ComputeBuffer *output_buffers, const ComputeBuffer *uniform_buffers,
    ComputeDescriptorSets *compute_descriptor_sets)
{

    VkResult res = VK_SUCCESS;
    VkBuffer *buffers;
    VkDeviceSize *buffer_sizes;

    // Update input descriptor sets
    buffers = malloc(sizeof(VkBuffer) * compute_pipeline->m_num_input_bindings);
    buffer_sizes =
        malloc(sizeof(VkDeviceSize) * compute_pipeline->m_num_input_bindings);
    for (size_t i = 0; i < compute_pipeline->m_num_input_bindings; ++i) {
        buffers[i] = input_buffers->m_buffer;
        buffer_sizes[i] = input_buffers->m_size;
    }
    res = update_descriptor_set(compute_device->m_device,
                                compute_descriptor_sets->m_input_descriptor_set,
                                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                compute_pipeline->m_num_input_bindings, buffers,
                                buffer_sizes);
    if (res != VK_SUCCESS) {
        return res;
    }
    free(buffers);
    free(buffer_sizes);

    // Update output descriptor sets
    buffers =
        malloc(sizeof(VkBuffer) * compute_pipeline->m_num_output_bindings);
    buffer_sizes =
        malloc(sizeof(VkDeviceSize) * compute_pipeline->m_num_output_bindings);
    for (size_t i = 0; i < compute_pipeline->m_num_output_bindings; ++i) {
        buffers[i] = output_buffers->m_buffer;
        buffer_sizes[i] = output_buffers->m_size;
    }
    res = update_descriptor_set(
        compute_device->m_device,
        compute_descriptor_sets->m_output_descriptor_set,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        compute_pipeline->m_num_output_bindings, buffers, buffer_sizes);
    if (res != VK_SUCCESS) {
        return res;
    }
    free(buffers);
    free(buffer_sizes);

    // Update uniform descriptor sets
    buffers =
        malloc(sizeof(VkBuffer) * compute_pipeline->m_num_uniform_bindings);
    buffer_sizes =
        malloc(sizeof(VkDeviceSize) * compute_pipeline->m_num_uniform_bindings);
    for (size_t i = 0; i < compute_pipeline->m_num_uniform_bindings; ++i) {
        buffers[i] = uniform_buffers->m_buffer;
        buffer_sizes[i] = uniform_buffers->m_size;
    }
    res = update_descriptor_set(
        compute_device->m_device,
        compute_descriptor_sets->m_uniform_descriptor_set,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        compute_pipeline->m_num_uniform_bindings, buffers, buffer_sizes);
    if (res != VK_SUCCESS) {
        return res;
    }
    free(buffers);
    free(buffer_sizes);

    return res;
}

VkResult
run_compute_pipeline_sync(const ComputeDevice *compute_device,
                          const ComputePipeline *compute_pipeline,
                          const ComputeDescriptorSets *compute_descriptor_sets,
                          const uint32_t group_count_x,
                          const uint32_t group_count_y,
                          const uint32_t group_count_z)
{
    (void)compute_device;
    VkResult res = VK_SUCCESS;

    vkResetCommandBuffer(compute_pipeline->m_command_buffer, 0);

    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(compute_pipeline->m_command_buffer, &begin_info);

    // Bind the compute pipeline
    vkCmdBindPipeline(compute_pipeline->m_command_buffer,
                      VK_PIPELINE_BIND_POINT_COMPUTE,
                      compute_pipeline->m_compute_pipeline);

    // Bind descriptor sets
    VkDescriptorSet descriptor_sets[3];
    uint32_t num_descriptor_sets = 0;
    if (compute_pipeline->m_num_input_bindings > 0) {
        descriptor_sets[num_descriptor_sets] =
            compute_descriptor_sets->m_input_descriptor_set;
        ++num_descriptor_sets;
    }
    if (compute_pipeline->m_num_output_bindings > 0) {
        descriptor_sets[num_descriptor_sets] =
            compute_descriptor_sets->m_output_descriptor_set;
        ++num_descriptor_sets;
    }
    if (compute_pipeline->m_num_uniform_bindings > 0) {
        descriptor_sets[num_descriptor_sets] =
            compute_descriptor_sets->m_uniform_descriptor_set;
        ++num_descriptor_sets;
    }
    vkCmdBindDescriptorSets(compute_pipeline->m_command_buffer,
                            VK_PIPELINE_BIND_POINT_COMPUTE,
                            compute_pipeline->m_pipeline_layout, 0,
                            num_descriptor_sets, descriptor_sets, 0, NULL);

    // Dispatch the compute shader
    vkCmdDispatch(compute_pipeline->m_command_buffer, group_count_x,
                  group_count_y, group_count_z);

    vkEndCommandBuffer(compute_pipeline->m_command_buffer);

    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &compute_pipeline->m_command_buffer;

    res = vkQueueSubmit(compute_pipeline->m_queue, 1, &submitInfo,
                        VK_NULL_HANDLE);
    if (res != VK_SUCCESS) {
        return res;
    }

    vkQueueWaitIdle(
        compute_pipeline->m_queue); // Wait for the compute queue to finish

    return res;
}

void destroy_compute_pipeline(const ComputeDevice *compute_device,
                              ComputePipeline *compute_pipeline)
{
    vkDestroyDescriptorPool(compute_device->m_device,
                            compute_pipeline->m_descriptor_pool,
                            VK_NULL_HANDLE);
    vkDestroyDescriptorSetLayout(
        compute_device->m_device,
        compute_pipeline->m_input_descriptor_set_layout, VK_NULL_HANDLE);
    vkDestroyDescriptorSetLayout(
        compute_device->m_device,
        compute_pipeline->m_output_descriptor_set_layout, VK_NULL_HANDLE);
    vkDestroyDescriptorSetLayout(
        compute_device->m_device,
        compute_pipeline->m_uniform_descriptor_set_layout, VK_NULL_HANDLE);
    vkDestroyPipeline(compute_device->m_device,
                      compute_pipeline->m_compute_pipeline, NULL);
    vkDestroyPipelineLayout(compute_device->m_device,
                            compute_pipeline->m_pipeline_layout, NULL);
    vkDestroyShaderModule(compute_device->m_device,
                          compute_pipeline->m_shader_module, VK_NULL_HANDLE);
    vkFreeCommandBuffers(compute_device->m_device,
                         compute_pipeline->m_command_pool, 1,
                         &compute_pipeline->m_command_buffer);
    vkDestroyCommandPool(compute_device->m_device,
                         compute_pipeline->m_command_pool, NULL);
}

VkResult
create_compute_descriptor_sets(const ComputeDevice *compute_device,
                               const ComputePipeline *compute_pipeline,
                               ComputeDescriptorSets *compute_descriptor_sets)
{
    VkResult res = VK_SUCCESS;

    // zero init
    memset(compute_descriptor_sets, 0, sizeof(ComputeDescriptorSets));

    VkDescriptorSetAllocateInfo allocInfo = {0};

    // Create input descriptor set
    memset(&allocInfo, 0, sizeof(VkDescriptorSetAllocateInfo));
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = compute_pipeline->m_descriptor_pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &compute_pipeline->m_input_descriptor_set_layout;
    res = vkAllocateDescriptorSets(
        compute_device->m_device, &allocInfo,
        &compute_descriptor_sets->m_input_descriptor_set);
    if (res != VK_SUCCESS) {
        return res;
    }

    // Create output descriptor set
    memset(&allocInfo, 0, sizeof(VkDescriptorSetAllocateInfo));
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = compute_pipeline->m_descriptor_pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &compute_pipeline->m_output_descriptor_set_layout;
    res = vkAllocateDescriptorSets(
        compute_device->m_device, &allocInfo,
        &compute_descriptor_sets->m_output_descriptor_set);
    if (res != VK_SUCCESS) {
        return res;
    }

    // Create uniform descriptor set
    memset(&allocInfo, 0, sizeof(VkDescriptorSetAllocateInfo));
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = compute_pipeline->m_descriptor_pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &compute_pipeline->m_uniform_descriptor_set_layout;
    res = vkAllocateDescriptorSets(
        compute_device->m_device, &allocInfo,
        &compute_descriptor_sets->m_uniform_descriptor_set);
    if (res != VK_SUCCESS) {
        return res;
    }

    return res;
}

VkResult create_compute_buffer(const ComputeDevice *compute_device,
                               VkDeviceSize size, ComputeBuffer *compute_buffer,
                               VkBufferUsageFlagBits usage)
{
    VkResult res = VK_SUCCESS;

    // zero init
    memset(compute_buffer, 0, sizeof(ComputeBuffer));

    // create buffer
    VkBufferCreateInfo buffer_info;
    memset(&buffer_info, 0, sizeof(VkBufferCreateInfo));
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    res = vkCreateBuffer(compute_device->m_device, &buffer_info, NULL,
                         &compute_buffer->m_buffer);
    if (res != VK_SUCCESS) {
        return res;
    }

    // get memory requirements
    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(compute_device->m_device,
                                  compute_buffer->m_buffer, &mem_requirements);

    // find memory type
    uint32_t memory_type_index;
    res = find_memory_type(compute_device->m_physical_device,
                           mem_requirements.memoryTypeBits,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                           &memory_type_index);
    if (res != VK_SUCCESS) {
        return res;
    }

    VkMemoryAllocateInfo alloc_info;
    memset(&alloc_info, 0, sizeof(VkMemoryAllocateInfo));
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = memory_type_index;

    res = vkAllocateMemory(compute_device->m_device, &alloc_info, NULL,
                           &compute_buffer->m_buffer_memory);
    if (res != VK_SUCCESS) {
        return res;
    }

    res = vkBindBufferMemory(compute_device->m_device, compute_buffer->m_buffer,
                             compute_buffer->m_buffer_memory, 0);
    if (res != VK_SUCCESS) {
        return res;
    }

    compute_buffer->m_size = size;

    return res;
}

void destroy_compute_buffer(const ComputeDevice *compute_device,
                            ComputeBuffer *compute_buffer)
{
    vkFreeMemory(compute_device->m_device, compute_buffer->m_buffer_memory,
                 VK_NULL_HANDLE);
    vkDestroyBuffer(compute_device->m_device, compute_buffer->m_buffer,
                    VK_NULL_HANDLE);
}

VkResult write_to_compute_buffer(const ComputeDevice *compute_device,
                                 const ComputeBuffer *compute_buffer,
                                 VkDeviceSize offset, VkDeviceSize size,
                                 const void *data)
{
    VkResult res = VK_SUCCESS;
    void *ptr;

    res = vkMapMemory(compute_device->m_device, compute_buffer->m_buffer_memory,
                      offset, size, 0, &ptr);
    if (res != VK_SUCCESS) {
        return res;
    }
    memcpy(ptr, data, size);
    vkUnmapMemory(compute_device->m_device, compute_buffer->m_buffer_memory);

    return res;
}

VkResult read_from_compute_buffer(const ComputeDevice *compute_device,
                                  const ComputeBuffer *compute_buffer,
                                  VkDeviceSize offset, VkDeviceSize size,
                                  void *data)
{
    VkResult res = VK_SUCCESS;
    void *ptr;

    res = vkMapMemory(compute_device->m_device, compute_buffer->m_buffer_memory,
                      offset, size, 0, &ptr);
    if (res != VK_SUCCESS) {
        return res;
    }
    memcpy(data, ptr, size);
    vkUnmapMemory(compute_device->m_device, compute_buffer->m_buffer_memory);

    return res;
}
