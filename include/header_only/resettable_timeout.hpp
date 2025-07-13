//
// Created by blomq on 2025-07-13.
//

#ifndef RESETTABLE_TIMEOUT_HPP
#define RESETTABLE_TIMEOUT_HPP
#include <boost/asio.hpp>

class resettable_timeout
{
public:
	explicit resettable_timeout(boost::asio::any_io_executor ex)
		: _timer(std::move(ex)) {}

	void start(const std::chrono::steady_clock::duration duration)
	{
		_timer.expires_after(duration);
	}

	void cancel()
	{
		_timer.cancel();
	}

	boost::asio::awaitable<bool> wait()
	{
		boost::system::error_code ec;
		co_await _timer.async_wait(boost::asio::redirect_error(boost::asio::use_awaitable, ec));
		co_return !ec; // true if the timer expired, false if canceled
	}

private:
	boost::asio::steady_timer _timer;
};

#endif //RESETTABLE_TIMEOUT_HPP
