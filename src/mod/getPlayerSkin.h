#pragma once

#include "ll/api/mod/NativeMod.h"

namespace get_skin {

class getSkin {
public:
    static getSkin& getInstance();

    getSkin() : mSelf(*ll::mod::NativeMod::current()) {}

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

    bool load();
    bool enable();
    bool disable();

private:
    ll::mod::NativeMod& mSelf;
};

} // namespace get_skin