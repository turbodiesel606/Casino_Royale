#ifndef JOBTRACKER_SRC_CVS_CVMUTATIONQUEUE_HPP
#define JOBTRACKER_SRC_CVS_CVMUTATIONQUEUE_HPP

#include "common/CancellationState.hpp"

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>

class CvLockWrapper final
{
public:
	std::mutex& getMutex() const { return mutex_; }
private:
	mutable std::mutex mutex_;
};

#endif // JOBTRACKER_SRC_CVS_CVMUTATIONQUEUE_HPP
