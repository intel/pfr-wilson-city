/******************************************************************************
 * Copyright (c) 2021 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************/

#ifndef INC_GTEST_HEADERS_H
#define INC_GTEST_HEADERS_H

// Because of the conflict on the io.h name, include it explicitly here before googletest
#ifdef _WIN32
#include "C:\Program Files (x86)\Windows Kits\10\Include\10.0.10240.0\ucrt\io.h"
#endif

#include "gtest/gtest.h"

#include <future>
#include <chrono>
#define ASSERT_DURATION_LE(secs, stmt)                                                       \
    {                                                                                        \
        std::promise<bool> completed;                                                        \
        auto stmt_future = completed.get_future();                                           \
        std::thread(                                                                         \
            [&](std::promise<bool>& completed) {                                             \
                stmt;                                                                        \
                completed.set_value(true);                                                   \
            },                                                                               \
            std::ref(completed))                                                             \
            .detach();                                                                       \
        if (stmt_future.wait_for(std::chrono::seconds(secs)) == std::future_status::timeout) \
		{ \
        	std::cout << "Internal Error: Test exceeded timeout of " << #secs << " seconds. Check code for infinite loops. Unit test exe will now crash, File: " << __FILE__ << ", Line: " << __LINE__ << std::endl; \
			std::abort(); \
		} \
    }


#endif  // INC_GTEST_HEADERS_H
