//-------------------------------------------------------------------------------------------------------
// Copyright (C) 2017 HHD Software Ltd.
// Written by Alexander Bessonov
//
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <chrono>
#include <iostream>

#include <experimental/resumable>
#include <corsl/all.h>

#include <future>
#include <sstream>

using namespace corsl::timer;

//#pragma comment(lib, "windowsapp")

using namespace std::chrono_literals;

const auto first_timer_duration = 3s;
const auto second_timer_duration = 5s;
const auto third_timer_duration = 7s;

corsl::future<void> void_timer(winrt::Windows::Foundation::TimeSpan duration)
{
	co_await duration;
}

corsl::future<bool> bool_timer(winrt::Windows::Foundation::TimeSpan duration)
{
	co_await duration;
	co_return true;
}

corsl::future<int> int_timer(winrt::Windows::Foundation::TimeSpan duration)
{
	co_await duration;
	co_return 10;
}

corsl::future<void> test_when_all_void()
{
	// Test when_all with awaitables
	co_await corsl::when_all(std::experimental::suspend_never{}, std::experimental::suspend_never{});

	// Test when_all with future
	co_await corsl::when_all(void_timer(first_timer_duration), void_timer(second_timer_duration));
}

corsl::future<void> test_when_all_void_range()
{
	std::vector<std::experimental::suspend_never> tasks1(2);

	co_await corsl::when_all_range(tasks1.begin(), tasks1.end());

	std::vector<corsl::future<void>> tasks2
	{
		void_timer(first_timer_duration),
		void_timer(second_timer_duration)
	};

	co_await corsl::when_all_range(tasks2.begin(), tasks2.end());
}

corsl::future<void> test_when_all_bool_range()
{
	std::vector<corsl::future<bool>> tasks2
	{
		bool_timer(first_timer_duration),
		bool_timer(second_timer_duration)
	};

	auto result = co_await corsl::when_all_range(tasks2.begin(), tasks2.end());
}

corsl::future<void> test_when_any_void_range()
{
	std::vector<std::experimental::suspend_never> tasks1(2);

	co_await corsl::when_any_range(tasks1.begin(), tasks1.end());

	std::vector<corsl::future<void>> tasks2
	{
		void_timer(first_timer_duration),
		void_timer(second_timer_duration)
	};

	co_await corsl::when_any_range(tasks2.begin(), tasks2.end());
}

corsl::future<void> test_when_any_bool_range()
{
	std::vector<corsl::future<bool>> tasks2
	{
		bool_timer(first_timer_duration),
		bool_timer(second_timer_duration)
	};

	auto result = co_await corsl::when_any_range(tasks2.begin(), tasks2.end());
}


corsl::future<void> test_when_all_mixed()
{
	std::tuple<corsl::no_result, bool> result = co_await corsl::when_all(void_timer(first_timer_duration), bool_timer(second_timer_duration));
	std::tuple<bool, int, corsl::no_result> result2 = co_await corsl::when_all(bool_timer(first_timer_duration), int_timer(second_timer_duration), corsl::resume_after{ third_timer_duration });
}

corsl::future<void> test_when_all_bool()
{
	// Test when_all with IAsyncOperation<T>
	std::promise<bool> promise;
	promise.set_value(true);
	co_await corsl::when_all(bool_timer(first_timer_duration), bool_timer(second_timer_duration), promise.get_future());
}

corsl::future<void> test_when_any_void()
{
	// Test when_any with awaitables
	co_await corsl::when_any(std::experimental::suspend_never{}, std::experimental::suspend_never{});

	auto timer1 = void_timer(first_timer_duration);
	// Test when_any with IAsyncAction
	co_await corsl::when_any(timer1, void_timer(second_timer_duration));
}

corsl::future<void> test_when_any_bool()
{
	// Test when_any with IAsyncOperation<T>
	co_await corsl::when_any(bool_timer(first_timer_duration), bool_timer(second_timer_duration));
}

corsl::future<void> test_async_timer()
{
	// Test cancellable async_timer. Start a timer for 20 minutes and cancel it after 2 seconds
	corsl::async_timer atimer;

	auto timer_task = corsl::start(atimer.wait(20min));
	co_await 2s;
	atimer.cancel();
	try
	{
		co_await timer_task;
	}
	catch (corsl::hresult_error)
	{
		std::wcout << L"Timer cancelled. ";
	}
}

template<class F>
void measure(const wchar_t *name, const F &f)
{
	std::wcout << L"Starting operation " << name << L" ... ";
	auto start = std::chrono::high_resolution_clock::now();
	f();
	auto stop = std::chrono::high_resolution_clock::now();

	auto seconds = std::chrono::duration_cast<std::chrono::duration<double>>(stop - start);
	std::wcout << seconds.count() << L" seconds\r\n";
}

template<class F>
corsl::future<void> measure_async(const wchar_t *name, const F &f)
{
	std::wstringstream ws;

	ws << L"Operation " << name << L" ... ";
	auto start = std::chrono::high_resolution_clock::now();
	co_await f();
	auto stop = std::chrono::high_resolution_clock::now();

	auto seconds = std::chrono::duration_cast<std::chrono::duration<double>>(stop - start);
	ws << seconds.count() << L" seconds\r\n";
	std::wcout << ws.str();
}

void sequential_test()
{
	std::wcout << L"Running all tests sequentially...\n";
	measure(L"test async_timer", [] { test_async_timer().get(); });
	measure(L"test when_all_void", [] { test_when_all_void().get(); });
	measure(L"test when_all_bool", [] { test_when_all_bool().get(); });
	measure(L"test when_all_mixed", [] { test_when_all_mixed().get(); });
	measure(L"test when_any_void", [] { test_when_any_void().get(); });
	measure(L"test when_any_bool", [] {test_when_any_bool().get(); });

	measure(L"test when_all_void_range", [] { test_when_all_void_range().get(); });
	measure(L"test when_all_bool_range", [] { test_when_all_bool_range().get(); });

	measure(L"test when_any_void_range", [] { test_when_any_void_range().get(); });
	measure(L"test when_any_bool_range", [] { test_when_any_bool_range().get(); });
}

void concurrent_test()
{
	std::wcout << L"Running all tests in parallel...\n";

	corsl::start(
		corsl::when_all(
			measure_async(L"test async_timer", [] { return test_async_timer(); }),
			measure_async(L"test when_all_void", [] { return test_when_all_void(); }),
			measure_async(L"test when_all_bool", [] { return test_when_all_bool(); }),
			measure_async(L"test when_all_mixed", [] { return test_when_all_mixed(); }),
			measure_async(L"test when_any_void", [] { return test_when_any_void(); }),
			measure_async(L"test when_any_bool", [] {return test_when_any_bool(); }),
			measure_async(L"test when_all_void_range", [] { return test_when_all_void_range(); }),
			measure_async(L"test when_all_bool_range", [] { return test_when_all_bool_range(); }),
			measure_async(L"test when_any_void_range", [] { return test_when_any_void_range(); }),
			measure_async(L"test when_any_bool_range", [] { return test_when_any_bool_range(); })
		)
	).get();
}

int main()
{
	sequential_test();
	concurrent_test();
}