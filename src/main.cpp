#include <Geode/Geode.hpp>
#include <Geode/modify/CCDirector.hpp>
#include <Windows.h>

using namespace geode::prelude;

bool wasOut = false;
bool mutedByMod = false;
double oldInterval = 1.0 / 60.0;

bool gameIsOut() {
    auto window = GetForegroundWindow();
    if (!window) return true;

    DWORD pid = 0;
    GetWindowThreadProcessId(window, &pid);
    return pid != GetCurrentProcessId();
}

void turnStuffOn() {
    auto* director = cocos2d::CCDirector::sharedDirector();
    if (!director) return;

    oldInterval = director->getAnimationInterval();

    if (Mod::get()->getSettingValue<bool>("limit-fps-when-unfocused")) {
        auto fps = Mod::get()->getSettingValue<int64_t>("unfocused-fps");
        if (fps < 1) fps = 1;
        director->setAnimationInterval(1.0 / static_cast<double>(fps));
    }

    if (Mod::get()->getSettingValue<bool>("mute-audio-when-unfocused")) {
        if (auto* audio = FMODAudioEngine::sharedEngine()) {
            audio->pauseAllAudio();
            mutedByMod = true;
        }
    }
}

void turnStuffOff() {
    if (auto* director = cocos2d::CCDirector::sharedDirector()) {
        director->setAnimationInterval(oldInterval);
    }

    if (mutedByMod) {
        if (auto* audio = FMODAudioEngine::sharedEngine()) {
            audio->resumeAllAudio();
        }
        mutedByMod = false;
    }
}

class $modify(TabbedOutDirector, cocos2d::CCDirector) {
    void drawScene() {
        auto out = gameIsOut();

        if (out && !wasOut) {
            turnStuffOn();
        }
        else if (!out && wasOut) {
            turnStuffOff();
        }

        wasOut = out;
        cocos2d::CCDirector::drawScene();
    }
};
