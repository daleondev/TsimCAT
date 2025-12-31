#pragma once

#include "../../model/Conveyor.hpp"
#include <elements.hpp>
#include <string>
#include <memory>

namespace tsim::ui
{
    using namespace cycfi::elements;
    using namespace tsim::model;

    class ConveyorPage
    {
    public:
        using on_error_toggle = std::function<void(bool)>;

        ConveyorPage()
        {
            m_run_deck = share(deck(
                label("OFF").font_color(colors::dim_gray),
                label("ON").font_color(colors::green)
            ));

            m_rev_deck = share(deck(
                label("FALSE").font_color(colors::dim_gray),
                label("TRUE").font_color(colors::gold)
            ));

            m_running_deck = share(deck(
                label("STOPPED").font_color(colors::red),
                label("RUNNING").font_color(colors::green)
            ));

            m_actual_vel_val = share(label("0.00 m/s"));
            m_item_count_val = share(label("0"));
            
            m_error_deck = share(deck(
                label("NONE").font_color(colors::green),
                label("ERROR").font_color(colors::red)
            ));

            auto make_info_row = [](std::string label_text, auto val_element) {
                return margin_bottom(10, 
                    htile(
                        hsize(180, align_right(label(label_text))),
                        margin_left(10, hold(val_element))
                    )
                );
            };

            auto control_group = group("PLC Commands (Digital)", 
                margin({20, 30, 20, 10},
                    vtile(
                        make_info_row("Run (bRun):", m_run_deck),
                        make_info_row("Reverse (bReverse):", m_rev_deck)
                    )
                ), 0.8f
            );

            auto status_group = group("Internal Simulation State", 
                margin({20, 30, 20, 10},
                    vtile(
                        make_info_row("Running:", m_running_deck),
                        make_info_row("Actual Velocity:", m_actual_vel_val),
                        make_info_row("Item Count:", m_item_count_val),
                        make_info_row("Error State:", m_error_deck)
                    )
                ), 0.8f
            );

            auto error_cb = share(check_box("Simulate Component Error"));
            error_cb->on_click = [this](bool state) {
                if (on_error) on_error(state);
            };

            auto fault_group = group("Fault Injection",
                margin({20, 10, 20, 10},
                    align_left(hold(error_cb))
                ), 0.8f
            );

            m_content = share(
                margin(
                    {30, 30, 30, 30},
                    vtile(
                        align_left(heading("Conveyor System")),
                        margin_top(30, htile(
                            hstretch(1.0, control_group),
                            margin_left(20, hstretch(1.0, status_group))
                        )),
                        margin_top(20, fault_group),
                        vstretch(1.0, empty()) 
                    )
                )
            );
        }

        element_ptr get_content() { return m_content; }

        void update(const ConveyorControl& ctrl, const ConveyorStatus& status, float actualVelocity, uint32_t itemCount)
        {
            m_run_deck->select(ctrl.bRun ? 1 : 0);
            m_rev_deck->select(ctrl.bReverse ? 1 : 0);
            m_running_deck->select(status.bRunning ? 1 : 0);
            m_error_deck->select(status.bError ? 1 : 0);

            char buf[32];
            std::snprintf(buf, sizeof(buf), "%.2f m/s", actualVelocity);
            m_actual_vel_val->set_text(buf);
            m_item_count_val->set_text(std::to_string(itemCount));
        }

        on_error_toggle on_error;

    private:
        std::shared_ptr<deck_element> m_run_deck;
        std::shared_ptr<deck_element> m_rev_deck;
        std::shared_ptr<deck_element> m_running_deck;
        std::shared_ptr<basic_label> m_actual_vel_val;
        std::shared_ptr<basic_label> m_item_count_val;
        std::shared_ptr<deck_element> m_error_deck;
        element_ptr m_content;
    };
}
