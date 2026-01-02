/*=============================================================================
   TsimCAT - The Simulated Control and Automation Toolkit
=============================================================================*/
#include "ui/MainWindow.hpp"
#include <elements.hpp>

#include "tlink/log/formatter.hpp"
#include <print>

using namespace cycfi::elements;

int main(int argc, char* argv[])
{
    // app _app("TsimCAT");

    // tsim::ui::MainWindow main_win(_app);

    struct Test
    {
        int i;
        float f;
        std::mutex m;
    };
    struct Test2
    {
        Test* t;
        std::unique_ptr<Test> upt = std::make_unique<Test>();
        Test* pt = new Test();
        std::mutex m2;
        std::unique_ptr<int> ptr = std::make_unique<int>();
        const char* str = "Hello";
        std::string s = "World";
        std::string_view ex = "!";
    };
    std::println("{:p}", Test2{});
    std::println("{}", Test2{});

    // _app.run();
    return 0;
}