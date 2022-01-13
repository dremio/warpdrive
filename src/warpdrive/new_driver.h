/* File:			new_driver.h
 *
 * Description:		Entry point for creating a new instance of driver::odbcabstraction::Driver objects.
 *
 * Comments:		See "readme.txt" for copyright and license information.
 *
 */

#ifndef __NEW_DRIVER_H__
#define __NEW_DRIVER_H__

#include <memory>

namespace driver
{
namespace odbcabstraction
{
  class Driver;
}
}

/*
 * Create a new instance of the Driver object.
 */
std::shared_ptr<driver::odbcabstraction::Driver> CreateDriver();

#endif
