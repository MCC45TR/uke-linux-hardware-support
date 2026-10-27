// SPDX-License-Identifier: MIT
// Parse the shipped ALSA data and its merged control namespace without a device.
#include <alsa/asoundlib.h>
#include <cassert>
#include <cstdio>

int main(int argc, char **argv)
{
    assert(argc > 1);
    for (int i = 1; i < argc; ++i) {
        snd_config_t *config = nullptr;
        snd_input_t *input = nullptr;
        assert(snd_config_top(&config) == 0);
        assert(snd_input_stdio_open(&input, argv[i], "r") == 0);
        const int result = snd_config_load(config, input);
        if (result < 0)
            std::fprintf(stderr, "%s: %s\n", argv[i], snd_strerror(result));
        assert(result == 0);
        snd_input_close(input);
        snd_config_delete(config);
    }
    return 0;
}
