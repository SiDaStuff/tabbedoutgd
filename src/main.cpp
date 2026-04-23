#include <Geode/Geode.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/modify/AppDelegate.hpp>
#include <Windows.h>

using namespace geode::prelude;

namespace {
    class TabbedOutState {
    public:
        static TabbedOutState& get() {
            static TabbedOutState state;
            return state;
        }

        void setAppActive(bool active) {
            m_appActive = active;
            this->refresh();
        }

        void refresh() {
            bool shouldBeUnfocused = this->isReallyUnfocused();

            if (shouldBeUnfocused == m_isUnfocused) {
                if (m_isUnfocused) {
                    this->syncWhileUnfocused();
                }
                return;
            }

            m_isUnfocused = shouldBeUnfocused;

            if (m_isUnfocused) {
                this->syncWhileUnfocused();
            }
            else {
                this->restoreEverything();
            }
        }

    private:
        bool m_appActive = true;
        bool m_isUnfocused = false;
        bool m_changedFps = false;
        bool m_mutedAudio = false;
        double m_oldInterval = 1.0 / 60.0;
        int64_t m_appliedUnfocusedFps = 0;
        float m_oldMusicVolume = 1.f;
        float m_oldSfxVolume = 1.f;

        bool isReallyUnfocused() const {
            auto foregroundWindow = GetForegroundWindow();
            if (!foregroundWindow) {
                return true;
            }

            DWORD foregroundPid = 0;
            GetWindowThreadProcessId(foregroundWindow, &foregroundPid);

            auto sameProcess = foregroundPid == GetCurrentProcessId();
            auto iconified = IsIconic(foregroundWindow) != 0;

            return !m_appActive || !sameProcess || iconified;
        }

        void syncWhileUnfocused() {
            this->syncFpsLimit();
            this->syncAudioMute();
        }

        void syncFpsLimit() {
            auto* director = cocos2d::CCDirector::sharedDirector();
            auto* gameManager = GameManager::sharedState();

            if (!director) {
                return;
            }

            bool shouldLimitFps =
                Mod::get()->getSettingValue<bool>("limit-fps-when-unfocused") &&
                gameManager &&
                !gameManager->m_vsyncEnabled;

            if (!shouldLimitFps) {
                if (m_changedFps) {
                    director->setAnimationInterval(m_oldInterval);
                    m_changedFps = false;
                    m_appliedUnfocusedFps = 0;
                }
                return;
            }

            auto fps = Mod::get()->getSettingValue<int64_t>("unfocused-fps");
            if (fps < 1) {
                fps = 1;
            }

            if (m_changedFps && m_appliedUnfocusedFps == fps) {
                return;
            }

            if (m_changedFps) {
                director->setAnimationInterval(m_oldInterval);
            }

            m_oldInterval = director->getAnimationInterval();
            director->setAnimationInterval(1.0 / static_cast<double>(fps));
            m_changedFps = true;
            m_appliedUnfocusedFps = fps;
        }

        void syncAudioMute() {
            auto* audio = FMODAudioEngine::sharedEngine();

            if (audio) {
                if (m_mutedAudio) {
                    audio->setBackgroundMusicVolume(m_oldMusicVolume);
                    audio->setEffectsVolume(m_oldSfxVolume);
                    m_mutedAudio = false;
                }

                if (Mod::get()->getSettingValue<bool>("mute-audio-when-unfocused")) {
                    m_oldMusicVolume = audio->getBackgroundMusicVolume();
                    m_oldSfxVolume = audio->getEffectsVolume();
                    audio->setBackgroundMusicVolume(0.f);
                    audio->setEffectsVolume(0.f);
                    m_mutedAudio = true;
                }
            }
        }

        void restoreEverything() {
            auto* director = cocos2d::CCDirector::sharedDirector();
            auto* audio = FMODAudioEngine::sharedEngine();

            if (director && m_changedFps) {
                director->setAnimationInterval(m_oldInterval);
                m_changedFps = false;
                m_appliedUnfocusedFps = 0;
            }

            if (audio && m_mutedAudio) {
                audio->setBackgroundMusicVolume(m_oldMusicVolume);
                audio->setEffectsVolume(m_oldSfxVolume);
                m_mutedAudio = false;
            }
        }
    };

    class FocusWatcher : public cocos2d::CCObject {
    public:
        void update(float) {
            TabbedOutState::get().refresh();
        }
    };

    FocusWatcher* getFocusWatcher() {
        static auto* watcher = new FocusWatcher();
        return watcher;
    }
}

$execute {
    auto* scheduler = cocos2d::CCDirector::sharedDirector()->getScheduler();
    scheduler->scheduleUpdateForTarget(getFocusWatcher(), 0, false);

    listenForSettingChanges<bool>("limit-fps-when-unfocused", [](bool) {
        TabbedOutState::get().refresh();
    });

    listenForSettingChanges<int64_t>("unfocused-fps", [](int64_t) {
        TabbedOutState::get().refresh();
    });

    listenForSettingChanges<bool>("mute-audio-when-unfocused", [](bool) {
        TabbedOutState::get().refresh();
    });
}

class $modify(TabbedOutAppDelegate, AppDelegate) {
    void applicationWillResignActive() {
        AppDelegate::applicationWillResignActive();
        TabbedOutState::get().setAppActive(false);
    }

    void applicationWillBecomeActive() {
        AppDelegate::applicationWillBecomeActive();
        TabbedOutState::get().setAppActive(true);
    }

    void applicationDidEnterBackground() {
        AppDelegate::applicationDidEnterBackground();
        TabbedOutState::get().setAppActive(false);
    }

    void applicationWillEnterForeground() {
        AppDelegate::applicationWillEnterForeground();
        TabbedOutState::get().setAppActive(true);
    }
};
