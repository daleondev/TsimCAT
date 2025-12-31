#pragma once

#include "../../model/Robot.hpp"
#include <elements.hpp>
#include <string>
#include <memory>
#include <vector>

namespace tsim::ui
{
    using namespace cycfi::elements;
    using namespace tsim::model;

    class RobotPage
    {
    public:
        using on_error_toggle = std::function<void(bool)>;
        using on_area_toggle = std::function<void(int, bool)>;

        RobotPage()
        {
            // 1. Initialize State Widgets
            m_enabled_deck = share(deck(label("DISABLED").font_color(colors::red), label("ENABLED").font_color(colors::green)));
            m_motion_deck = share(deck(label("IDLE").font_color(colors::dim_gray), label("MOVING").font_color(colors::gold)));
            m_home_deck = share(deck(label("NOT HOME").font_color(colors::dim_gray), label("HOME").font_color(colors::green)));
            m_job_val = share(label("0"));
            m_job_feedback_val = share(label("0"));
            m_part_type_val = share(label("0"));
            m_part_type_echo_val = share(label("0"));
            m_error_deck = share(deck(label("NONE").font_color(colors::green), label("ERROR").font_color(colors::red)));

            m_brake_test_deck = share(deck(label("FAIL").font_color(colors::red), label("OK").font_color(colors::green)));
            m_mastering_deck = share(deck(label("FAIL").font_color(colors::red), label("OK").font_color(colors::green)));
            
            m_mode_deck = share(deck(
                label("NONE").font_color(colors::dim_gray),
                label("T1").font_color(colors::gold),
                label("T2").font_color(colors::orange),
                label("AUT").font_color(colors::green),
                label("EXT").font_color(colors::dodger_blue)
            ));

            m_move_enable_deck = share(deck(label("OFF").font_color(colors::dim_gray), label("ON").font_color(colors::green)));
            m_reset_deck = share(deck(label("OFF").font_color(colors::dim_gray), label("ON").font_color(colors::green)));

            for (int i = 0; i < 8; ++i) {
                m_area_plc_decks[i] = share(deck(label("OCCUPIED").font_color(colors::red), label("FREE").font_color(colors::green)));
                
                // check_box returns a toggle_button proxy. We need to store it as a basic_button or element.
                // To access .value(), basic_toggle_button is appropriate.
                m_area_robot_manual[i] = share(check_box(""));
                m_area_robot_manual[i]->value(true); // Default to free
                m_area_robot_manual[i]->on_click = [this, i](bool state) {
                    if (on_area_manual) on_area_manual(i, state);
                };
            }

            // 2. Helper lambda for rows
            auto info_row = [](std::string label_text, std::shared_ptr<element> val_element) {
                return share(margin_bottom(5, 
                    htile(
                        hsize(180, align_right(label(label_text))),
                        margin_left(10, hold(val_element))
                    )
                ));
            };

            // 3. Construct Control Group
            auto control_group = share(group("PLC Interface", 
                margin({20, 30, 20, 10},
                    vtile(
                        hold(info_row("Command Job ID:", m_job_val)),
                        hold(info_row("Part Type:", m_part_type_val)),
                        hold(info_row("Move Enable:", m_move_enable_deck)), 
                        hold(info_row("Reset:", m_reset_deck))        
                    )
                ), 0.8f
            ));

            // 4. Construct Status Group
            auto status_group = share(group("Robot Status", 
                margin({20, 30, 20, 10},
                    vtile(
                        hold(info_row("Status:", m_enabled_deck)),
                        hold(info_row("Mode:", m_mode_deck)),
                        hold(info_row("Motion:", m_motion_deck)),
                        hold(info_row("Position:", m_home_deck)),
                        hold(info_row("Feedback Job ID:", m_job_feedback_val)),
                        hold(info_row("Part Type Echo:", m_part_type_echo_val)),
                        hold(info_row("Brake Test:", m_brake_test_deck)),
                        hold(info_row("Mastering:", m_mastering_deck)),
                        hold(info_row("Error State:", m_error_deck))
                    )
                ), 0.8f
            ));

            // 5. Construct Area Group
            auto area_vtile = std::make_shared<vtile_composite>();
            area_vtile->push_back(share(margin_bottom(5, htile(hsize(80, empty()), margin_left(10, hsize(100, label("PLC"))), margin_left(10, hsize(100, label("Robot")))))));
            
            for(int i=0; i<8; ++i) {
                auto row = htile(
                    hsize(80, align_right(label("Area " + std::to_string(i+1) + ":"))),
                    margin_left(10, hsize(100, hold(m_area_plc_decks[i]))),
                    margin_left(10, hsize(100, hold(m_area_robot_manual[i])))
                );
                area_vtile->push_back(share(margin_bottom(2, row)));
            }

            auto area_group = share(group("Area Handshake (Mutex)", 
                margin({20, 30, 20, 10}, hold(area_vtile)), 0.8f
            ));

            // 6. Fault Injection
            auto error_cb = share(check_box("Simulate Robot Fault"));
            error_cb->on_click = [this](bool state) { if (on_error) on_error(state); };
            auto fault_group = share(group("Fault Injection", margin({20,10,20,10}, align_left(hold(error_cb))), 0.8f));

            // 7. Main Content Layout
            m_content = share(
                margin(
                    {30, 30, 30, 30},
                    vtile(
                        align_left(heading("Robot System")),
                        margin_top(30, htile(
                            hstretch(1.0, vtile(
                                hold(control_group), 
                                margin_top(20, hold(fault_group))
                            )),
                            margin_left(20, hstretch(1.0, hold(status_group))),
                            margin_left(20, hstretch(1.0, hold(area_group)))
                        )),
                        vstretch(1.0, empty()) 
                    )
                )
            );
        }

        element_ptr get_content() { return m_content; }

        void update(const RobotControl& ctrl, const RobotStatus& status)
        {
            m_enabled_deck->select(status.bEnabled ? 1 : 0);
            m_motion_deck->select(status.bInMotion ? 1 : 0);
            m_home_deck->select(status.bInHome ? 1 : 0);
            m_job_val->set_text(std::to_string(ctrl.nJobId));
            m_job_feedback_val->set_text(std::to_string(status.nJobIdFeedback));
            m_part_type_val->set_text(std::to_string(ctrl.nPartType));
            m_part_type_echo_val->set_text(std::to_string(status.nPartTypeMirrored));
            m_error_deck->select(status.bError ? 1 : 0);
            
            m_brake_test_deck->select(status.bBrakeTestOk ? 1 : 0);
            m_mastering_deck->select(status.bMasteringOk ? 1 : 0);

            if (status.bInT1) m_mode_deck->select(1);
            else if (status.bInT2) m_mode_deck->select(2);
            else if (status.bInAut) m_mode_deck->select(3);
            else if (status.bInExt) m_mode_deck->select(4);
            else m_mode_deck->select(0);

            for (int i = 0; i < 8; ++i) {
                m_area_plc_decks[i]->select((ctrl.nAreaFree_PLC & (1 << i)) ? 1 : 0);
                
                // Only update the UI state from simulation if the user isn't interacting
                // But for display purposes, we can just force it to match simulation state
                m_area_robot_manual[i]->value(((status.nAreaFree_Robot >> i) & 1) ? true : false);
            }
        }

        on_error_toggle on_error;
        on_area_toggle on_area_manual;

    private:
        std::shared_ptr<deck_element> m_enabled_deck;
        std::shared_ptr<deck_element> m_motion_deck;
        std::shared_ptr<deck_element> m_home_deck;
        std::shared_ptr<basic_label> m_job_val;
        std::shared_ptr<basic_label> m_job_feedback_val;
        std::shared_ptr<basic_label> m_part_type_val;
        std::shared_ptr<basic_label> m_part_type_echo_val;
        std::shared_ptr<deck_element> m_error_deck;
        std::shared_ptr<deck_element> m_brake_test_deck;
        std::shared_ptr<deck_element> m_mastering_deck;
        std::shared_ptr<deck_element> m_mode_deck;
        std::shared_ptr<deck_element> m_move_enable_deck;
        std::shared_ptr<deck_element> m_reset_deck;
        std::array<std::shared_ptr<deck_element>, 8> m_area_plc_decks;
        std::array<std::shared_ptr<basic_toggle_button>, 8> m_area_robot_manual;
        element_ptr m_content;
    };
}