#include "MxTime.h"

namespace Mix {
    float Time::mDeltaTime = 0.0f,
          Time::mTime = 0.0f,
          Time::mFixedDeltaTime = 1.0f / 60,
          Time::mFixedTime = 0.0f,
          Time::mMaximumDeltaTime = 1.0f / 10;

    int Time::mFixedSteps = 0;

    Time::TimePoint Time::mStart = Time::Clock::now(),
                    Time::mPrev = Time::Clock::now(),
                    Time::mCurr = Time::Clock::now();

    void Time::Tick() {
        mCurr = Clock::now();
        mDeltaTime = DurationToSecond(mCurr - mPrev);
        mTime = DurationToSecond(mCurr - mStart);
        if(mTime - mFixedTime >= mFixedDeltaTime) {
            mFixedSteps = static_cast<int>((mTime - mFixedTime) / mFixedDeltaTime);
            mFixedTime += mFixedSteps * mFixedDeltaTime;
        }
        mPrev = mCurr;
    }
}
