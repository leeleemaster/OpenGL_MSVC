#include "core/BuildInfo.h"

#include <iostream>

int main()
{
    std::cout << dentalviz::projectName() << ' ' << dentalviz::projectVersion()
              << " bootstrap\n";
    return 0;
}
