#pragma once

#ifndef MX_AUDIO_LISTENER_H_
#define MX_AUDIO_LISTENER_H_

#include "../Behaviour/MxBehaviour.h"
#include "../../Definitions/MxAudio.h"

// todo: Coordinate with Rigidbody

namespace Mix {
    /*class Rigidbody;*/

    /**
     *  @note Each @code Scene @endcode should only have 1 @code AudioListener @endcode.
     */
    class AudioListener final : public Behaviour {
    MX_DECLARE_RTTI
    MX_DECLARE_CLASS_FACTORY

    public:
        AudioListener() : velocityUpdateMode(Audio::VelocityUpdateMode::AUTO),
                          mListenerIdx(0),
                          mCore(nullptr),
                          mLastPos(0),
                          mUseFixedUpdate(false) {}

        ~AudioListener() = default;

        Audio::VelocityUpdateMode velocityUpdateMode;

    private:
        // static int sListenerNum; /**< Number of listeners. Obsolete. */
        int mListenerIdx; /**< Index for multiple listeners. Deprecated. */
        FMOD::System* mCore;
        glm::vec3 mLastPos;
        bool mUseFixedUpdate;

        void init() override;
        void lateUpdate() override;
        void fixedUpdate() override;

        void updatePosAndVel(const float _deltaTime);
    };
}

#endif
