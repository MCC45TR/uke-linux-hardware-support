// SPDX-License-Identifier: GPL-3.0-or-later
// Exercise the actual generated protobuf parser, without a sensor connection.
#include <cassert>
#include <cstdint>
#include "ssc-sensor-proximity.pb-c.h"

int main()
{
    // Required accuracy=3, with the repeated data field legally absent.
    const std::uint8_t payload[] = {0x10, 0x03};
    auto *message = ssc_proximity_response_davinci__unpack(nullptr, sizeof(payload), payload);
    assert(message && message->accuracy == 3 && message->n_data == 0 && message->data == nullptr);
    ssc_proximity_response_davinci__free_unpacked(message, nullptr);
}
