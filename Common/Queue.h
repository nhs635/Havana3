#ifndef _QQUEUE_H_
#define _QQUEUE_H_

#include <iostream>
#include <condition_variable>
#include <mutex>
#include <queue>

template <typename T>
class Queue
{
public:
	T pop() // 대기
	{
		std::unique_lock<std::mutex> mlock(mutex_);
		while (queue_.empty())
			cond_.wait(mlock);

		auto item = queue_.front();
		queue_.pop();
		return item;
	}

	void pop(T& item) // 대기
	{
		std::unique_lock<std::mutex> mlock(mutex_);
		while (queue_.empty())
			cond_.wait(mlock);

		item = queue_.front();
		queue_.pop();
	}

	void push(const T& item) // 동작
	{
		std::unique_lock<std::mutex> mlock(mutex_);
		queue_.push(item);
		mlock.unlock();
		cond_.notify_one();
	}

	void push(T&& item) // 동작
	{
		std::unique_lock<std::mutex> mlock(mutex_);
		queue_.push(std::move(item));
		mlock.unlock();
		cond_.notify_one();
	}

	size_t size()
	{
		return queue_.size();
	}

private:
	std::queue<T> queue_;
	std::mutex mutex_;
	std::condition_variable cond_;
};

#endif
