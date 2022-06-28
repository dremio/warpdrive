///
/// Copyright (C) 2020-2022 Dremio Corporation
///
/// See “license.txt” for license information.
///
/// Module:			main.cc
///
/// Description:		This module contains routines for building the Arrow Flight SQL Driver.

#include <memory>
#include <flight_sql/flight_sql_driver.h>

/**
 * @brief Create a Driver object
 */
std::shared_ptr<driver::odbcabstraction::Driver> CreateDriver() {
    auto driver = std::shared_ptr<driver::odbcabstraction::Driver>(new driver::flight_sql::FlightSqlDriver());
    driver->SetVersion(WARPDRIVE_BUILD_VERSION);
    return driver;
}
