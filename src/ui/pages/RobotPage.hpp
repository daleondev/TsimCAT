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
                align_left(heading("Robot System")),
                margin_top(10, label("Status: Offline")),
                vstretch(1.0, empty())
            )
        );
    }
}