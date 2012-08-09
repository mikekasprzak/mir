/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef MIR_LOGGER_H_
#define MIR_LOGGER_H_

#include <iosfwd>

namespace mir
{
namespace client
{
class Logger
{
public:
    virtual std::ostream& error() = 0;
protected:
    Logger() {}
    virtual ~Logger() {}
};

class ConsoleLogger : public Logger
{
public:
    virtual std::ostream& error();
};

}
}

#endif /* MIR_LOGGER_H_ */
