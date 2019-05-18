#include "MxTime.h"

#include <algorithm>

namespace Mix {
    float Time::mDeltaTime = 0.0f,
          Time::mTime = 0.0f,
          Time::mFixedDeltaTime = 1.0f / 60,
          Time::mFixedTime = 0.0f,
          Time::mMaximumDeltaTime = 1.0f / 3;

    Time::TimePoint Time::mStart = Time::Clock::now(),
                    Time::mPrev = Time::Clock::now(),
                    Time::mCurr = Time::Clock::now();

    float Time::mSmoothing = 0.0f;
    int Time::mFixedClampedSteps = 0;

    void Time::Tick() {
        mCurr = Clock::now();
        mDeltaTime = DurationToSecond(mCurr - mPrev);
        mTime = DurationToSecond(mCurr - mStart);

        const int steps = static_cast<int>(std::floor((mTime - mFixedTime) / mFixedDeltaTime));
        // avoid rounding issues
        if(steps)
            mFixedTime += steps * mFixedDeltaTime;

        mSmoothing = (mTime - mFixedTime) / mFixedDeltaTime;

        const int maxSteps = static_cast<int>(std::floor((mMaximumDeltaTime - mDeltaTime) / mFixedDeltaTime));
        // in case delta time is already larger than max delta time
        mFixedClampedSteps = std::max(std::min(steps, maxSteps), 0);

        mPrev = mCurr;
    }
}
