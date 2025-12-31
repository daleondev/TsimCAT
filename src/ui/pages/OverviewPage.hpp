#pragma once

#include <elements.hpp>

namespace tsim::ui
{
    using namespace cycfi::elements;

    inline auto make_overview_page()
    {
        return margin(
            {20, 20, 20, 20},
            vtile(
                align_left(heading("Robot Cell Overview")),
                margin_top(10, label("System Status: Online")),
                margin_top(10, label("Active Process: Pick and Place Operation")),
                margin_top(20, align_center_middle(label("Cell Visualization Placeholder")))
            )
        );
    }
}
