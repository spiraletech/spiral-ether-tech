#pragma once

#include <vector>

#include "crystal/AUMCrystal.hpp"

class CrystalGrid {
public:
    void attach(AUMCrystal& crystal)
    {
        crystals_.push_back(&crystal);
    }

    // Failure containment is a grid law: one crystal may fail emergence
    // without preventing unrelated Hakui systems from booting.
    void emergeAll();
    void sustain(float dt);
    void compressAll();

private:
    std::vector<AUMCrystal*> crystals_;
};
