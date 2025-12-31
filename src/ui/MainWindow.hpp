#pragma once

#include "CellNavigator.hpp"
#include "pages/OverviewPage.hpp"
#include "pages/RobotPage.hpp"
#include "pages/ConveyorPage.hpp"
#include "pages/SensorsPage.hpp"
#include "pages/SafetyPage.hpp"
#include <elements.hpp>

namespace tsim::ui
{
    using namespace cycfi::elements;

    class MainWindow
    {
    public:
        MainWindow(app& _app)
            : m_app(_app)
            , m_window(_app.name(), window::standard, {0, 0, 1280, 750})
            , m_view(m_window)
            , m_navigator([this](size_t index) { this->show_page(index); })
        {
            m_window.on_close = [this]() { m_app.stop(); };
            setup_content();
        }

    private:
        void setup_content()
        {
            m_deck = share(deck(
                make_overview_page(),
                make_robot_page(),
                make_conveyor_page(),
                make_sensors_page(),
                make_safety_page()
            ));

            auto main_layout = htile(
                hsize(200, m_navigator.make_sidebar()),
                hstretch(1.0, hold(m_deck))
            );

            auto background = box(rgba(35, 35, 37, 255));

            view_limits limits;
            limits.min = { 1280, 750 };
            limits.max = { full_extent, full_extent };

            // We combine limit (for window sizing) and stretch (for content distribution)
            m_view.content(
                limit(limits, vstretch(1.0, hstretch(1.0, main_layout))),
                background
            );
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