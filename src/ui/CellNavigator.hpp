#pragma once

#include <elements.hpp>
#include <vector>
#include <string>
#include <memory>

namespace tsim::ui
{
    using namespace cycfi::elements;

    /**
     * CellNavigator provides a sidebar with buttons to switch between
     * different views of the robot cell.
     */
    class CellNavigator
    {
    public:
        using on_change_view = std::function<void(size_t)>;

        explicit CellNavigator(on_change_view cb)
            : m_on_change_view(std::move(cb))
        {
        }

        auto make_sidebar()
        {
            auto make_nav_button = [this](std::string label, size_t index) {
                // Create the button first
                auto btn_raw = share(toggle_button(std::move(label), 1.0f));
                
                // Set the callback
                btn_raw->on_click = [this, index](bool state) {
                    if (state) {
                        this->select_view(index);
                    } else {
                        if (m_current_index == index) {
                            if (auto b_ptr = std::dynamic_pointer_cast<basic_button>(m_buttons[index])) {
                                b_ptr->value(true);
                            }
                        }
                    }
                };

                // Store the button for state management
                m_buttons.push_back(btn_raw);

                // Wrap in hstretch and margin for layout
                return margin({10, 5, 10, 5}, hstretch(1.0, hold(btn_raw)));
            };

            auto sidebar_content = vtile(
                make_nav_button("Overview", 0),
                make_nav_button("Robot", 1),
                make_nav_button("Conveyor", 2),
                make_nav_button("Sensors", 3),
                make_nav_button("Safety", 4),
                vstretch(1.0, empty()) // Fill remaining vertical space
            );

            // Initial selection
            if (!m_buttons.empty()) {
                if (auto b = std::dynamic_pointer_cast<basic_button>(m_buttons[0])) {
                    b->value(true);
                }
            }

            return layer(
                vstretch(1.0, sidebar_content),
                panel{} // Background for sidebar
            );
        }

        void select_view(size_t index)
        {
            if (index >= m_buttons.size() || index == m_current_index)
                return;

            // Unselect previous
            if (m_current_index < m_buttons.size()) {
                if (auto b = std::dynamic_pointer_cast<basic_button>(m_buttons[m_current_index])) {
                    b->value(false);
                }
            }

            m_current_index = index;
            if (auto b = std::dynamic_pointer_cast<basic_button>(m_buttons[m_current_index])) {
                b->value(true);
            }

            if (m_on_change_view) {
                m_on_change_view(index);
            }
        }

    private:
        on_change_view m_on_change_view;
        std::vector<std::shared_ptr<basic_button>> m_buttons;
        size_t m_current_index = 0;
    };
}
