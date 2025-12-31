#pragma once

#include <elements.hpp>

namespace tsim::ui
{
    using namespace cycfi::elements;

    inline auto make_conveyor_page()
    {
        return margin({ 20, 20, 20, 20 },
                      vtile(align_left(heading("Conveyor Control")),
                            margin_top(10, label("Conveyor Speed: 0.5 m/s")),
                            margin_top(10, htile(button("Start"), margin_left(10, button("Stop")))),
                            margin_top(20, label("Item Count: 142"))));
    }
}
