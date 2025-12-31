#pragma once

#include <elements.hpp>

namespace tsim::ui
{
    using namespace cycfi::elements;

    inline auto make_robot_page()
    {
        return margin(
            {20, 20, 20, 20},
            vtile(
                align_left(heading("Robot Configuration")),
                margin_top(10, label("Joint 1: 45.0°")),
                margin_top(5, label("Joint 2: -10.5°")),
                margin_top(5, label("Joint 3: 90.0°")),
                margin_top(5, label("Joint 4: 0.0°")),
                margin_top(5, label("Joint 5: 15.2°")),
                margin_top(5, label("Joint 6: 0.0°")),
                margin_top(20, button("Reset to Home"))
            )
        );
    }
}