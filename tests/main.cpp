#include <gtest/gtest.h>
#include "log4cxx/logger.h"
#include "log4cxx/propertyconfigurator.h"
#include "log4cxx/basicconfigurator.h"
#include "log4cxx/helpers/exception.h"

using namespace log4cxx;
using namespace log4cxx::helpers;

LoggerPtr logger(log4cxx::Logger::getLogger("fabreq"));


int main(int argc, char **argv) {
    int rtn;
    ::testing::InitGoogleTest(&argc, argv); 

    if (argc > 1)
        PropertyConfigurator::configure(argv[1]);
    else
        PropertyConfigurator::configure("/home/gerard/visualcode/fabreq/tests/logger.cfg");
//        BasicConfigurator::configure();

    LOG4CXX_INFO(logger, "Entering application.");
    rtn = RUN_ALL_TESTS();
    LOG4CXX_INFO(logger, "Exiting application.");

    return rtn;
}