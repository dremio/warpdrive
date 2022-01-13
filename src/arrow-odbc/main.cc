#include <memory>
/*-------
 * Module:			main.cc
 *
 * Description:		This module contains routines for building the Arrow Flight SQL Driver.
 *
 * Comments:		See "readme.txt" for copyright and license information.
 *-------
 */
#include <flight_sql/flight_sql_driver.h>

/**
 * @brief Create a Driver object
 */
std::shared_ptr<driver::odbcabstraction::Driver> CreateDriver() {
    return std::shared_ptr<driver::odbcabstraction::Driver>(new driver::flight_sql::FlightSqlDriver());
}
