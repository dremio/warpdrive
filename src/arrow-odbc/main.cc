#include <memory>
/*-------
 * Module:			main.cc
 *
 * Description:		This module contains routines for building the Arrow Flight SQL Driver.
 *
 * Comments:		See "readme.txt" for copyright and license information.
 *-------
 */
#include <new_driver.h>

#include "flight_sql_driver.h"

/**
 * @brief Create a Driver object
 */
std::shared_ptr<driver::spi::Driver> CreateDriver() {
    return std::shared_ptr<driver::spi::Driver>(new driver::flight_sql::FlightSqlDriver());
}
