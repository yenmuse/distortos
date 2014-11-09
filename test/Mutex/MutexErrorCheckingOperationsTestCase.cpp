/**
 * \file
 * \brief MutexErrorCheckingOperationsTestCase class implementation
 *
 * \author Copyright (C) 2014 Kamil Szczygiel http://www.distortec.com http://www.freddiechopin.info
 *
 * \par License
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * \date 2014-11-09
 */

#include "MutexErrorCheckingOperationsTestCase.hpp"

#include "waitForNextTick.hpp"
#include "mutexTestUnlockFromWrongThread.hpp"

#include "distortos/Mutex.hpp"
#include "distortos/ThisThread.hpp"

#include <cerrno>

namespace distortos
{

namespace test
{

namespace
{

/*---------------------------------------------------------------------------------------------------------------------+
| local constants
+---------------------------------------------------------------------------------------------------------------------*/

/// single duration used in tests
constexpr auto singleDuration = TickClock::duration{1};

/// priority of current test thread
constexpr uint8_t testThreadPriority {UINT8_MAX - 1};

/*---------------------------------------------------------------------------------------------------------------------+
| local functions
+---------------------------------------------------------------------------------------------------------------------*/

/**
 * \brief Runs the test case.
 *
 * \attention this function expects the priority of test thread to be testThreadPriority
 *
 * \return true if the test case succeeded, false otherwise
 */

bool testRunner_()
{
	Mutex mutex {Mutex::Type::ErrorChecking};

	{
		// simple lock - must succeed immediately
		waitForNextTick();
		const auto start = TickClock::now();
		const auto ret = mutex.lock();
		if (ret != 0 || start != TickClock::now())
			return false;
	}

	{
		// re-lock attempt - must fail with EDEADLK immediately
		waitForNextTick();
		const auto start = TickClock::now();
		const auto ret = mutex.lock();
		if (ret != EDEADLK || start != TickClock::now())
			return false;
	}

	{
		// re-lock attempt - must fail with EDEADLK immediately
		waitForNextTick();
		const auto start = TickClock::now();
		const auto ret = mutex.tryLockFor(singleDuration);
		if (ret != EDEADLK || start != TickClock::now())
			return false;
	}

	{
		// re-lock attempt - must fail with EDEADLK immediately
		waitForNextTick();
		const auto start = TickClock::now();
		const auto ret = mutex.tryLockUntil(start + singleDuration);
		if (ret != EDEADLK || start != TickClock::now())
			return false;
	}

	{
		const auto ret = mutexTestUnlockFromWrongThread(mutex);
		if (ret != true)
			return ret;
	}

	{
		// simple unlock - must succeed immediately
		waitForNextTick();
		const auto start = TickClock::now();
		const auto ret = mutex.unlock();
		if (ret != 0 || start != TickClock::now())
			return false;
	}

	{
		// excessive unlock - must fail with EPERM immediately
		waitForNextTick();
		const auto start = TickClock::now();
		const auto ret = mutex.unlock();
		if (ret != EPERM || start != TickClock::now())
			return false;
	}

	return true;
}

}	// namespace

/*---------------------------------------------------------------------------------------------------------------------+
| private functions
+---------------------------------------------------------------------------------------------------------------------*/

bool MutexErrorCheckingOperationsTestCase::run_() const
{
	const auto thisThreadPriority = ThisThread::getPriority();
	ThisThread::setPriority(testThreadPriority);
	const auto ret = testRunner_();
	ThisThread::setPriority(thisThreadPriority);
	return ret;
}

}	// namespace test

}	// namespace distortos
