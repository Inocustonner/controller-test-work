#include <deque>
#include <mutex>


template <typename T>
struct SQueue
{
	using guard = std::lock_guard<std::mutex>;

	template<typename ...Args>
	SQueue(Args&& ...args) : deque{std::forward<Args>(args)...}
	{}
	
	void push(T t)
	{
		guard grd(wrt_mut);

		deque.push_front(std::forward<T>(t));
	}

	T pop()
	{
		guard grd(wrt_mut);

		T t = deque.back();
		deque.pop_back();
		return t;
	}

	constexpr
	size_t size() const
	{
		guard grd(wrt_mut);

		return std::size(deque);
	}

private:
	std::deque<T> deque;
	mutable std::mutex wrt_mut;
};
