#pragma once

#include "CellNavigator.hpp"
#include "pages/ConveyorPage.hpp"
#include "pages/OverviewPage.hpp"
#include "pages/RobotPage.hpp"
#include "pages/SafetyPage.hpp"
#include "pages/SensorsPage.hpp"
#include "../sim/PLCManager.hpp"
#include <elements.hpp>

namespace tsim::ui
{
    using namespace cycfi::elements;

    class MainWindow
    {
      public:
        MainWindow(app& _app)
          : m_app(_app)
          , m_window(_app.name(), window::standard, { 0, 0, 1280, 750 })
          , m_view(m_window)
          , m_navigator([this](size_t index) { this->show_page(index); })
          , m_conveyor_sim(std::make_shared<sim::ConveyorSimulator>())
          , m_plc_manager(m_conveyor_sim)
        {
            m_window.on_close = [this]() { m_app.stop(); };
            
            setup_content();
            
            m_plc_manager.start();
            m_timer = m_view.post(std::chrono::milliseconds(16), [this]() { this->on_timer(); });
        }

      private:
        void setup_content()
        {
            m_conveyor_page = std::make_shared<ConveyorPage>();

            m_deck = share(deck(
                align_left_top(make_overview_page()),
                align_left_top(make_robot_page()),
                align_left_top(hold(m_conveyor_page->get_content())),
                align_left_top(make_sensors_page()),
                align_left_top(make_safety_page())
            ));

            auto sidebar = hsize(200, m_navigator.make_sidebar());
            auto content_area = 
                layer(
                    align_left_top(hold(m_deck)),
                    box(rgba(35, 35, 37, 255))
                );

            m_view.content(
                min_size({ 1280, 750 }, 
                    htile(sidebar, content_area)
                )
            );
        }

        void on_timer()
        {
            float dt = 0.016f;
            m_conveyor_sim->step(dt);

            m_conveyor_page->update(
                m_conveyor_sim->get_control(), 
                m_conveyor_sim->get_status(),
                m_conveyor_sim->get_actual_velocity(),
                m_conveyor_sim->get_item_count()
            );
            
            m_view.refresh();
            m_timer = m_view.post(std::chrono::milliseconds(16), [this]() { this->on_timer(); });
        }

        void show_page(size_t index)
        {
            m_deck->select(index);
            m_view.refresh();
        }

        app& m_app;
        window m_window;
        view m_view;
        CellNavigator m_navigator;
        std::shared_ptr<deck_element> m_deck;

        std::shared_ptr<sim::ConveyorSimulator> m_conveyor_sim;
        sim::PLCManager m_plc_manager;
        std::shared_ptr<ConveyorPage> m_conveyor_page;
        view::steady_timer_ptr m_timer;
    };
}
