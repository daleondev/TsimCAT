/*=============================================================================
   TsimCAT - The Simulated Control and Automation Toolkit
=============================================================================*/
#include "ui/MainWindow.hpp"
#include <elements.hpp>

using namespace cycfi::elements;

int main(int argc, char* argv[])
{
    app _app("TsimCAT");
    tsim::ui::MainWindow main_win(_app);
    _app.run();
    return 0;
}