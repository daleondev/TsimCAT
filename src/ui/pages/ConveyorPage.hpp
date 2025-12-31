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
        ConveyorPage()
        {
            m_run_val = share(label("OFF").font_color(colors::dim_gray));
            m_rev_val = share(label("FALSE").font_color(colors::dim_gray));

            m_running_val = share(label("STOPPED").font_color(colors::red));
            m_actual_vel_val = share(label("0.00 m/s"));
            m_item_count_val = share(label("0"));
            m_error_val = share(label("NONE").font_color(colors::green));

            m_content = share(
                margin(
                    {30, 30, 30, 30},
                    vtile(
                        align_left(heading("Conveyor System")),
                        margin_top(30, htile(
                            hstretch(1.0, hold(make_control_group())),
                            margin_left(20, hstretch(1.0, hold(make_status_group())))
                        )),
                        vstretch(1.0, empty()) 
                    )
                )
            );
        }

        auto get_content() { return m_content; }

        void update(const ConveyorControl& ctrl, const ConveyorStatus& status, float actualVelocity, uint32_t itemCount)
        {
            m_run_val->set_text(ctrl.bRun ? "ON" : "OFF");
            m_rev_val->set_text(ctrl.bReverse ? "TRUE" : "FALSE");
            
            m_running_val->set_text(status.bRunning ? "RUNNING" : "STOPPED");

            char buf[32];
            std::snprintf(buf, sizeof(buf), "%.2f m/s", actualVelocity);
            m_actual_vel_val->set_text(buf);

            m_item_count_val->set_text(std::to_string(itemCount));

            m_error_val->set_text(status.bError ? "ERROR" : "NONE");
        }

    private:
        auto info_row(std::string label_text, std::shared_ptr<basic_label> val_element)
        {
            return margin_bottom(10, 
                htile(
                    hsize(180, align_right(label(label_text))),
                    margin_left(10, hold(val_element))
                )
            );
        }

        auto make_control_group() -> std::shared_ptr<element>
        {
            return share(group("PLC Commands (Digital)", 
                margin({20, 30, 20, 10},
                    vtile(
                        info_row("Run (bRun):", m_run_val),
                        info_row("Reverse (bReverse):", m_rev_val)
                    )
                ), 0.8f
            ));
        }

        auto make_status_group() -> std::shared_ptr<element>
        {
            return share(group("Internal Simulation State", 
                margin({20, 30, 20, 10},
                    vtile(
                        info_row("Running:", m_running_val),
                        info_row("Actual Velocity:", m_actual_vel_val),
                        info_row("Item Count:", m_item_count_val),
                        info_row("Error State:", m_error_val)
                    )
                ), 0.8f
            ));
        }

        std::shared_ptr<basic_label> m_run_val;
        std::shared_ptr<basic_label> m_rev_val;

        std::shared_ptr<basic_label> m_running_val;
        std::shared_ptr<basic_label> m_actual_vel_val;
        std::shared_ptr<basic_label> m_item_count_val;
        std::shared_ptr<basic_label> m_error_val;

        element_ptr m_content;
    };
}