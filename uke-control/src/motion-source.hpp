// SPDX-License-Identifier: MIT
#pragma once
#include "motion-sample.hpp"
#include <functional>
#include <memory>
#include <systemd/sd-event.h>

namespace uke::motion {
struct Source {
    virtual ~Source()=default;
    virtual bool start()=0;
    virtual void stop()=0;
    virtual Snapshot current()const=0;
};
std::unique_ptr<Source> iio_source(sd_event*,std::function<void(Snapshot)>);
bool recover_iio();
}
