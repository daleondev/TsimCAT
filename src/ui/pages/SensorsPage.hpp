#pragma once

#include <elements.hpp>

namespace tsim::ui
{
    using namespace cycfi::elements;

    inline auto make_sensors_page()
    {
        return margin(
            {20, 20, 20, 20},
            vtile(
                align_left(heading("Sensor Diagnostics")),
                margin_top(10, label("Proximity Sensor 1: Active")),
                margin_top(5, label("Optical Sensor 2: Clear")),
                margin_top(5, label("Pressure Sensor 3: 4.2 bar"))
            )
        );
    }
}
