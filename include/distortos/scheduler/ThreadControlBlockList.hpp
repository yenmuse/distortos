/**
 * \file
 * \brief ThreadControlBlockList class header
 *
 * \author Copyright (C) 2014 Kamil Szczygiel http://www.distortec.com http://www.freddiechopin.info
 *
 * \par License
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * \date 2014-08-06
 */

#ifndef INCLUDE_DISTORTOS_SCHEDULER_THREADCONTROLBLOCKLIST_HPP_
#define INCLUDE_DISTORTOS_SCHEDULER_THREADCONTROLBLOCKLIST_HPP_

#include "distortos/scheduler/ThreadControlBlock.hpp"

#include "distortos/containers/SortedContainer.hpp"

#include <list>

namespace distortos
{

namespace scheduler
{

/// type held by ThreadControlBlockList
using ThreadControlBlockListValueType = std::reference_wrapper<ThreadControlBlock>;

/**
 * \brief ThreadControlBlockList is a sorted list of ThreadControlBlock references that configures state of kept objects
 *
 * \param Compare is a type of functor used for comparison, std::less or similar results in descending order,
 * std::greater or similar - in ascending order.
 */

template<typename Compare>
class ThreadControlBlockList : private containers::SortedContainer<std::list<ThreadControlBlockListValueType>, Compare>
{
public:

	template<typename OtherContainer, typename OtherCompare>
	friend class containers::SortedContainer;

	/// base of ThreadControlBlockList
	using Base = containers::SortedContainer<std::list<ThreadControlBlockListValueType>, Compare>;

	using typename Base::iterator;
	using typename Base::value_type;

	using Base::begin;
	using Base::empty;
	using Base::end;
	using Base::size;

	/**
	 * \brief ThreadControlBlockList's constructor
	 *
	 * \param [in] state is the state of ThreadControlBlock objects kept in this list
	 */

	ThreadControlBlockList(const ThreadControlBlock::State state) :
			Base{},
			state_{state}
	{

	}

	/**
	 * \brief Wrapper for sortedEmplace()
	 *
	 * Sets state of emplaced element.
	 *
	 * \param Args are types of argument for value_type constructor
	 *
	 * \param [in] args are arguments for value_type constructor
	 *
	 * \return iterator to emplaced element
	 */

	template<typename... Args>
	iterator sortedEmplace(Args&&... args)
	{
		const auto it = Base::sortedEmplace(std::forward<Args>(args)...);
		it->get().setState(state_);
		return it;
	}

	/**
	 * \brief Wrapper for sortedSplice()
	 *
	 * Sets state of transfered element.
	 *
	 * \param OtherContainer is the type of container from which the object is transfered
	 * \param OtherIterator is the type of iterator in OtherContainer
	 *
	 * \param [in] other is the container from which the object is transfered
	 * \param [in] other_position is the position of the transfered object in the other container
	 */

	template<typename Container, typename Iterator>
	void sortedSplice(Container &other, Iterator other_position)
	{
		Base::sortedSplice(other, other_position);
		other_position->get().setState(state_);
	}

private:

	/// state of ThreadControlBlock objects kept in this list
	const ThreadControlBlock::State state_;
};

/// functor which gives descending priority order of elements on the list
struct ThreadControlBlockDescendingPriority
{
	/**
	 * \brief operator()
	 *
	 * \param [in] left is the object on the left side of comparison
	 * \param [in] right is the object on the right side of comparison
	 *
	 * \return true if left's priority is less than right's priority
	 */

	bool operator() (const ThreadControlBlockListValueType &left, const ThreadControlBlockListValueType &right)
	{
		return left.get().getPriority() < right.get().getPriority();
	}
};

/// functor which gives ascending "sleep until" order of elements on the list
struct ThreadControlBlockAscendingSleepUntil
{
	/**
	 * \brief operator()
	 *
	 * \param [in] left is the object on the left side of comparison
	 * \param [in] right is the object on the right side of comparison
	 *
	 * \return true if left's "sleep until" is greater than right's "sleep until"
	 */

	bool operator() (const ThreadControlBlockListValueType &left, const ThreadControlBlockListValueType &right)
	{
		return left.get().getSleepUntil() > right.get().getSleepUntil();
	}
};

/// list of ThreadControlBlock objects in descending order of priority
using PriorityThreadControlBlockList = ThreadControlBlockList<ThreadControlBlockDescendingPriority>;

/// list of ThreadControlBlock objects in ascending order of "sleep until"
using SleepUntilThreadControlBlockList = ThreadControlBlockList<ThreadControlBlockAscendingSleepUntil>;

}	// namespace scheduler

}	// namespace distortos

#endif	// INCLUDE_DISTORTOS_SCHEDULER_THREADCONTROLBLOCKLIST_HPP_
