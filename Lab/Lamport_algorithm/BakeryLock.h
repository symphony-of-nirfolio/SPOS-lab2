#pragma once


#include "FixnumLock.h"
#include <array>
#include <algorithm> 
#include <thread>
#include <chrono>
#include <mutex>
#include <numeric>
#include <assert.h>


namespace bakery_lock
{
	template<size_t MaxThreadsCount>
	class BakeryLock : public FixnumLock
	{
	public:
		void lock() override
		{
			size_t id = getId();
			assert(id != std::numeric_limits<size_t>::max());

			is_enter_array[id] = true;

			size_t max_item = number_array[0];
			for (size_t i = 0; i < MaxThreadsCount; ++i)
			{
				max_item = std::max(max_item, number_array[i]);
			}
			number_array[id] = max_item;

			is_enter_array[id] = false;

			for (size_t i = 0; i < MaxThreadsCount; ++i)
			{
				while (is_enter_array[i])
				{
					std::this_thread::yield();
				}

				while ((number_array[i] != 0) 
					&& (number_array[id] > number_array[i] || (number_array[id] == number_array[i] && id > i)))
				{
					std::this_thread::yield();
				}
			}
		}

		void unlock() override
		{
			size_t id = getId();
			assert(id != std::numeric_limits<size_t>::max());

			number_array[id] = 0;
		}

		bool tryLock() override
		{
			size_t id = getId();
			assert(id != std::numeric_limits<size_t>::max());



			return false;
		}

		bool tryLock(size_t timeout_in_milliseconds) override
		{
			size_t id = getId();
			assert(id != std::numeric_limits<size_t>::max());



			return false;
		}


		size_t getId() override
		{
			std::lock_guard<std::mutex> lock(thread_id_to_number_lock);

			std::thread::id id = std::this_thread::get_id();
			for (size_t i = 0; i < thread_id_to_number_array.size(); ++i)
			{
				if (thread_id_to_number_array[i].first == id)
				{
					return thread_id_to_number_array[i].second;
				}
			}

			return std::numeric_limits<size_t>::max();
		}

		void register_thread() override
		{
			register_thread(std::this_thread::get_id());
		}

		void register_thread(std::thread::id id) override
		{
			std::lock_guard<std::mutex> lock(thread_id_to_number_lock);

			for (size_t i = 0; i < thread_id_to_number_array.size(); ++i)
			{
				if (thread_id_to_number_array[i].first == std::thread::id{})
				{
					thread_id_to_number_array[i].first = id;
					thread_id_to_number_array[i].second = i;
					return;
				}
			}
		}

		void unregister_thread() override
		{
			unregister_thread(std::this_thread::get_id());
		}


		void unregister_thread(std::thread::id id) override
		{
			std::lock_guard<std::mutex> lock(thread_id_to_number_lock);

			for (size_t i = 0; i < thread_id_to_number_array.size(); ++i)
			{
				if (thread_id_to_number_array[i].first == id)
				{
					thread_id_to_number_array[i].first = std::thread::id{};
					thread_id_to_number_array[i].second = 0;
					return;
				}
			}
		}

	private:
		std::array<bool, MaxThreadsCount> is_enter_array{ false };
		std::array<size_t, MaxThreadsCount> number_array{ 0 };

		std::array<std::pair<std::thread::id, size_t>, MaxThreadsCount> thread_id_to_number_array{};
		std::mutex thread_id_to_number_lock{};
	};
}