#pragma once

#include "CellNavigator.hpp"
#include "pages/ConveyorPage.hpp"
#include "pages/OverviewPage.hpp"
#include "pages/RobotPage.hpp"
#include "pages/SafetyPage.hpp"
#include "pages/SensorsPage.hpp"
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
        {
            m_window.on_close = [this]() { m_app.stop(); };
            setup_content();
        }

      private:
        void setup_content()
        {
            // align_left_top(subject) is a proxy that returns:
            // min = subject.min
            // max = full_extent (IMPORTANT!)
            // This prevents the deck (which takes the MINIMUM of its children's MAX limits)
            // from being restricted by a small page.

            m_deck = share(deck(align_left_top(make_overview_page()),
                                align_left_top(make_robot_page()),
                                align_left_top(make_conveyor_page()),
                                align_left_top(make_sensors_page()),
                                align_left_top(make_safety_page())));

            auto sidebar = hsize(200, m_navigator.make_sidebar());

            // The content area needs to stretch to fill the rest of the htile.
            // We also wrap it in a background layer.
            auto content_area = layer(hstretch(1.0, vstretch(1.0, hold(m_deck))), box(rgba(35, 35, 37, 255)));

            m_view.content(min_size({ 1280, 750 }, htile(sidebar, content_area)));
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
    };
}
