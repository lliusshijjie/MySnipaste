#pragma once

#include <future>
#include <thread>
#include <type_traits>
#include <utility>
#include <winrt/base.h>

namespace mysnip::ocr {

class OcrThreadRunner {
public:
    template <typename Operation>
    static auto Run(Operation&& operation) {
        using Result = std::invoke_result_t<Operation>;

        std::packaged_task<Result()> task(
            [operation = std::forward<Operation>(operation)]() mutable -> Result {
                winrt::init_apartment(winrt::apartment_type::multi_threaded);
                try {
                    if constexpr (std::is_void_v<Result>) {
                        operation();
                        winrt::uninit_apartment();
                    } else {
                        Result result = operation();
                        winrt::uninit_apartment();
                        return result;
                    }
                } catch (...) {
                    winrt::uninit_apartment();
                    throw;
                }
            });

        auto result = task.get_future();
        std::jthread worker(std::move(task));
        return result.get();
    }
};

} // namespace mysnip::ocr
