#pragma once
#ifndef EXMAPLES_COMMON_HEADER
#define EXMAPLES_COMMON_HEADER

#include <binary_util.hpp>
#include <compute_interface.h>
#include <vulkan/vk_enum_string_helper.h>

#include <optional>
#include <type_traits>

#define UNWRAP_VKRESULT(result)                                                \
    do {                                                                       \
        if (result != VK_SUCCESS) {                                            \
            fprintf(stderr, "Vulkan error: %s\n", string_VkResult(result));    \
            exit(EXIT_FAILURE);                                                \
        }                                                                      \
    } while (0)

void print_data_buffers(const ComputeDevice *compute_device,
                        size_t num_elements, VkDeviceMemory input_buffer_memory,
                        VkDeviceMemory output_buffer_memory);

std::optional<std::string> read_file(const char *filepath);

// template <typename T, typename ...Params>
// constexpr void fill_across_bins(T total, Params& ... params) {
//     static_assert(std::conjunction<std::is_convertible<Params, T>...>::value,
//     "All Params need to be convertible");
// }

template <typename T, typename U, typename S>
constexpr void fill_until(T total, U &target, S max_size)
{
    static_assert(std::is_convertible<U, T>::value,
                  "Params need to be convertible");
    target = (total > max_size) ? max_size : total;
}

#endif // EXMAPLES_COMMON_HEADER