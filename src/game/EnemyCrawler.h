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

#pragma once
#include "EnemyPatroller.h"

class EnemyCrawler : public EnemyPatroller
{
public:
	EnemyCrawler(float x, float y);
	virtual ~EnemyCrawler();
	/*
	virtual ~EnemyCrawler();
	virtual void update(float dt);
	virtual void draw();
	virtual void die();
	//virtual void doDamage(float damage);
	virtual void onAnimationComplete(std::string anim);
	//virtual void resetPhysics();
	//virtual void destroyPhysics();
	virtual void onBumperCollide(int facing);
protected:
	float walk_speed;
	Bumper* leftBumper;
	Bumper* rightBumper;
	*/
}; 
