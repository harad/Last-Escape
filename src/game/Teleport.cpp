/*
 *  This file is part of Last Escape.
 *
 *  Last Escape is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Last Escape is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Last Escape.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Teleport.h"
#include "Player.h"

TeleportEnter::TeleportEnter(float x, float y, float w, float h, std::string teleName)
:Sensor(x, y, w, h)
{
	name = teleName;
}

void TeleportEnter::collide(Actor& otherActor) {
	if (otherActor.isPlayer())
	{
		for (list<Actor*>::iterator it = actors.begin(); it != actors.end(); ++it)
		{
			if ((*it)->isTeleportExit() && ((TeleportExit*)*it)->getName() == name)
			{
				// TODO: Fix this mofo, once we make an Actor::Teleport function.
				/*
				otherActor.setPos((*it)->pos_x, (*it)->pos_y);
				// Make the player fly up a bit
				((Player&)otherActor).speed_x = 0;
				((Player&)otherActor).speed_y = -500;
				*/
				break;
			}
		}
	}
}

TeleportExit::TeleportExit(float x, float y, std::string teleName)
:Sensor(x, y, 32.0f, 32.0f)
{
	name = teleName;
}
