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
    app _app("TsimCAT");

    tsim::ui::MainWindow main_win(_app);

    struct X
    {
        int i;
        float f;
        std::mutex m;
        // int* ptr;
        // std::unique_ptr<X> ptr;
    } x;

    // x.ptr = std::make_unique<X>();
    std::println("{}", x);

    _app.run();
    return 0;
}