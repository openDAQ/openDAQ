#include "console_application.h"
#include <opendaq/instance_factory.h>

int main(int /*argc*/, const char* /*argv*/[])
{
    daq::ConsoleApplication app(daq::Instance());
    app.start();

    return 0;
}
