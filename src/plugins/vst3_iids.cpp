/* vst3_iids.cpp
 * Provides missing VST3 SDK symbol definitions needed for linking
 */

#include <pluginterfaces/base/funknown.h>

namespace Steinberg {
    // Define FUnknown::iid - required for EventList::queryInterface
    const FUID FUnknown::iid(0x00000000, 0x00000000, 0x00000000, 0x00000000);
}

