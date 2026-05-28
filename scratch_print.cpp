#include <iostream>
#include <CarlaNativePlugin.h>

int main() {
    const NativePluginDescriptor* desc = carla_get_native_rack_plugin();
    if (!desc) {
        std::cerr << "Failed to get native rack plugin descriptor!" << std::endl;
        return 1;
    }
    std::cout << "audioIns: " << desc->audioIns << std::endl;
    std::cout << "audioOuts: " << desc->audioOuts << std::endl;
    std::cout << "midiIns: " << desc->midiIns << std::endl;
    std::cout << "midiOuts: " << desc->midiOuts << std::endl;
    return 0;
}
