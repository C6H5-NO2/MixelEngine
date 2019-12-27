#include "MixEngine.h"
#include "Mx/Window/MxWindow.h"
#include "Mx/Time/MxTime.h"
#include "Mx/Input/MxInput.h"
#include "Mx/Audio/MxAudioCore.h"
#include "Mx/Physics/MxPhysicsWorld.h"
#include "Mx/Coroutine/MxCoroMgr.h"
#include "Mx/Vulkan/Shader/MxVkStandardShader.h"
#include "Mx/GUI/MxGUi.h"
#include "Mx/Resource/MxResourceLoader.h"
#include "Mx/Graphics/MxGraphics.h"
#include "Mx/Scene/MxSceneManager.h"
#include "Mx/Engine/MxPlatform.h"
#include "MxApplicationBase.h"
#include "Mx/Animation/MxAnimationManager.h"

namespace Mix {
    MixEngine::MixEngine(int _argc, char** _argv) {
        for (int i = 0; i < _argc; ++i)
            mCommandLines.emplace_back(_argv[i]);
    }

    void MixEngine::requestQuit() {
        Platform::PushQuitEvent();
    }

    void MixEngine::setFPSLimit(uint32_t _limit) {
        mFPSLimit = _limit;
        if (mFPSLimit > 0)
            mFrameStep = 1 / mFPSLimit;
    }

    MixEngine::~MixEngine() {
        //mModuleHolder.get<Audio::Core>()->release();
        mModuleHolder.clear();
        Platform::ShutDown();
    }

    void MixEngine::loadModule() {
        SDL_Rect rect;
        SDL_GetDisplayBounds(0, &rect);

        mModuleHolder.add<Window>("Mix Engine Demo", Vector2i{ rect.w * 2.f / 3, rect.h * 2.f / 3 }, WindowFlag::Vulkan | WindowFlag::Shown)->load();
        mModuleHolder.add<Input>()->load();
        mModuleHolder.add<Audio::Core>()->load();
        mModuleHolder.add<Physics::World>()->load();
        mModuleHolder.add<Coroutine::CoroMgr>();
        mModuleHolder.add<Graphics>()->load();
        mModuleHolder.add<GUI>()->load();
        mModuleHolder.add<ResourceLoader>()->load();
        mModuleHolder.add<SceneObjectManager>()->load();
        mModuleHolder.add<SceneManager>()->load();
        mModuleHolder.add<AnimationManager>()->load();

        std::string n = mApp->getAppName();
        Version v = mApp->getAppVersion();
        std::string title = Utils::StringFormat("%1% V %2%.%3%.%4%",
                                                mApp->getAppName(),
                                                v.getMajor(), v.getMinor(), v.getPatch());
        Window::Get()->setTitle(title);

        mApp->onModuleLoaded();
    }

    void MixEngine::initModule() {
        auto modules = mModuleHolder.getAllOrdered();
        for (auto m : modules)
            m->init();

        mApp->onModuleInitialized();
    }

    int MixEngine::execute(std::shared_ptr<ApplicationBase> _app) {
        if (_app == nullptr) {
            throw std::runtime_error("No application specified.");
        }
        mApp = std::move(_app);

        Time::Awake();
        Platform::Initialize();
        Platform::QuitEvent.connect(std::bind(&MixEngine::onQuitRequested, this));

        mApp->startUp(getCommandLines());

        try {
            loadModule();
            initModule();

            loadMainScene();

            while (!mQuit) {
                /*awake();
                init();*/
                Platform::Update();
                Time::Tick();
                if (mRunning) {
                    // Limit FPS if limit was set.
                    if (mFPSLimit > 0) {
                        float currentTime = Time::TotalTime();
                        float nextFrameTime = mLastFrameTime + mFrameStep;
                        while (nextFrameTime > currentTime) {
                            uint32_t waitTime = (nextFrameTime - currentTime) * 1000;

                            // If waiting for longer, sleep
                            if (waitTime >= 2000) {
                                Platform::Sleep(waitTime);
                                currentTime = Time::TotalTime();
                            }
                        }

                        mLastFrameTime = currentTime;
                    }

                    // Calculate fps
                    static auto startTp = Time::RealTime();
                    if (++mFrameCount > mFrameSampleRate) {
                        mFramePerSecond = mFrameCount / (Time::RealTime() - startTp);
                        startTp = Time::RealTime();
                        mFrameCount = 0u;
                    }
                    Window::Get()->setTitle(std::to_string(mFramePerSecond));

                    auto start = Time::RealTime();
                    update();
                    mAnalysisTime.update = Time::RealTime() - start;

                    start = Time::RealTime();
                    lateUpdate();
                    mAnalysisTime.lateUpdate = Time::RealTime() - start;

                    start = Time::RealTime();
                    render();
                    mAnalysisTime.render = Time::RealTime() - start;

                    start = Time::RealTime();
                    postRender();
                    mAnalysisTime.postRender = Time::RealTime() - start;

                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    void MixEngine::loadMainScene() {
        auto sceneManager = mModuleHolder.get<SceneManager>();

        auto scene = sceneManager->createScene("MainScene");
        auto filler = std::make_shared<DefaultSceneFiller>();
        scene->setFiller(filler);
        sceneManager->loadScene(scene->getIndex());
        sceneManager->setActiveScene(scene);

        mApp->onMainSceneCreated();
    }

    void MixEngine::update() {
        mApp->onUpdate();
        mModuleHolder.get<Physics::World>()->sync(Time::FixedDeltaTime(), Time::SmoothingFactor());
        mModuleHolder.get<Coroutine::CoroMgr>()->update(); // todo: wrong order!
        mModuleHolder.get<SceneManager>()->sceneUpdate();

        for (auto i = 0u; i < Time::sFixedClampedSteps; ++i) {
            mApp->onFixedUpdate();
            fixedUpdate();
        }
    }

    void MixEngine::fixedUpdate() {
        mApp->onFixedUpdate();
        mModuleHolder.get<Physics::World>()->step(Time::FixedDeltaTime());
        mModuleHolder.get<SceneManager>()->sceneFixedUpdate();

#ifdef MX_ENABLE_PHYSICS_DEBUG_DRAW_
        mModuleHolder.get<Physics::World>()->pushDrawData();
#endif
    }

    void MixEngine::lateUpdate() {
        mApp->onLateUpdate();
        mModuleHolder.get<SceneManager>()->sceneLateUpdate();
        mModuleHolder.get<SceneObjectManager>()->lateUpdate();
        mModuleHolder.get<Audio::Core>()->lateUpdate();
    }

    void MixEngine::onQuitRequested() {
        if (mApp->onQuitRequested())
            quit();
    }

    void MixEngine::render() {

        mApp->onRender();


#ifdef MX_ENABLE_PHYSICS_DEBUG_DRAW_
        mModuleHolder.get<Physics::World>()->render();
#endif
        mModuleHolder.get<GUI>()->beginGUI();
        mApp->onGUI();
        mModuleHolder.get<GUI>()->endGUI();
        mModuleHolder.get<GUI>()->update();

        FrameSceneInfo frameSceneInfo = mModuleHolder.get<SceneManager>()->getActiveScene()->_getFrameSceneInfo();

        FrameRenderInfo frameRenderInfo;
        frameRenderInfo.sceneInfo = &frameSceneInfo;

        frameRenderInfo.animInfo = mModuleHolder.get<AnimationManager>()->update(frameSceneInfo);

        mModuleHolder.get<Graphics>()->update();
        mModuleHolder.get<Graphics>()->render(frameRenderInfo);
    }

    void MixEngine::postRender() {
        mApp->onPostRender();
        mModuleHolder.get<SceneManager>()->scenePostRender();
        mModuleHolder.get<SceneObjectManager>()->postRender();
        mModuleHolder.get<Input>()->nextFrame();
    }

}
