#pragma once

#include <elements.hpp>

namespace tsim::ui
{
    using namespace cycfi::elements;

    inline auto make_safety_page()
    {
        return margin(
            {20, 20, 20, 20},
            vtile(
                align_left(heading("Safety System")),
                margin_top(10, label("Emergency Stop: Released")),
                margin_top(5, label("Light Curtain: Clear")),
                margin_top(5, label("Safety Gate: Locked"))
            )
        );
    }
}
