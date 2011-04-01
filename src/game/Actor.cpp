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

#include "Actor.h"
#include "Map.h"
#include "globals.h"
#include <list>
#include <cmath>

Actor::Actor(float x, float y, float w, float h, bool staticBody) {
	width = w;
	height = h;
// 	setPos(0, 0);
	pos_x = x;
	pos_y = y;
	destroyed = false;
	actors.push_back(this);
	setDrawOffset(0, 0);
	collideable = true;
	hasImage = false;
	hidden = false;
	currentLevel = 1;
	experienceValue = 0;
	body = NULL;
	shape = NULL;
	grounded = 0;
	actorName = "Unnamed Actor";
	shapeLayers = CP_ALL_LAYERS;
	this->staticBody = staticBody;
	defaultVelocityFunc = cpBodyUpdateVelocity;
	resetPhysics();
}

Actor::~Actor() {
	destroyPhysics();
}

void Actor::setPlaceholder(sf::Color c, float w, float h, float xoff, float yoff) {
	width = (int)w;
	height = (int)h;
	sprite.SetColor(c);
	sprite.SetScale((float)width, (float)height);
	sprite.SetCenter(xoff, yoff);
}

void Actor::setVelocityFunc(cpBodyVelocityFunc f) {
	if(body)
		body->velocity_func = f;
	defaultVelocityFunc = f;
}

void Actor::setPos(float px, float py) {
	pos_x = px;
	pos_y = py;
}

void Actor::setDrawOffset(int ox, int oy) {
	xDrawOffset = ox;
	yDrawOffset = oy;
	sprite.SetCenter((float)ox, (float)oy);
}

// returns true if the actor collided with a map tile
bool Actor::move(float &mx, float &my) {
  return game_map->move(*this, mx, my);
}

void Actor::getPos(float &px, float &py) {
	px = pos_x;
	py = pos_y;
}

void Actor::setSize(int w, int h) {
	height = h;
	width = w;
	resetPhysics();
}

void Actor::getSize(int &w, int &h) {
	h = height;
	w = width;
}

/*
void Actor::getBoundingBox(float &x1, float &y1, float &x2, float &y2) {
	x1 = pos_x - xOrigin;
	y1 = pos_y - yOrigin;
	x2 = x1 + width;
	y2 = y1 + height;
}

bool Actor::isColliding(Actor * otherActor) {
	float x1, y1, x2, y2;
	float ox1, oy1, ox2, oy2;
	getBoundingBox(x1, y1, x2, y2);
	otherActor->getBoundingBox(ox1, oy1, ox2, oy2);

	if (x2 < ox1 || ox2 < x1 || y2 < oy1 || oy2 < y1)
		return false;
	return true;
}
*/

void Actor::draw() {
	if(!hidden && hasImage) {
		if(body && body->a) {
			sprite.SetRotation(-rad2deg(body->a));
		} else {
			sprite.SetRotation(0);
		}
		
		sprite.SetPosition(
			0.5f + (int)(pos_x - game_map->cam_x),
			0.5f + (int)(pos_y - game_map->cam_y));
		App->Draw(sprite);
		
		/*
		if(debugMode)
        {
						float px = sprite.GetPosition().x;
						float py = sprite.GetPosition().y;
						
            float bbx1, bby1, bbx2, bby2;
            bbx1 = px - xOrigin;
            bby1 = py - yOrigin;
            bbx2 = bbx1 + width;
            bby2 = bby1 + height;
            App->Draw(sf::Shape::Rectangle(bbx1, bby1, bbx2, bby2,
                                           sf::Color(0, 0, 0, 0), 1.0f, sf::Color(255, 0, 0)));
						
						// Draw a crosshair at the actor's position
						App->Draw(sf::Shape::Line(px-4, py, px+4, py, 1.0f, 
																					 sf::Color(0, 0, 255)));
						App->Draw(sf::Shape::Line(px, py-4, px, py+4, 1.0f,
																					 sf::Color(0, 0, 255)));
        }
    */
	}
}

void Actor::destroy() {
	onDestroy();
	game_map->actorDestroyed(this);
	destroyed = true;
}

bool Actor::isGrounded() {
	//return game_map->isGrounded(*this);
	return(grounded > 0);
}

bool Actor::isDestroyed() {
	return destroyed;
}

bool Actor::isDying() {
	return dying;
}

/*
void Actor::checkCollisions() {
	list<Actor*>::iterator it2 = actors.begin();

	for (; it2 != actors.end(); ++it2)
	{
		// Don't collide with self. :)
		if (*it2 != this && !isDestroyed() && !(*it2)->isDestroyed() &&
			(*it2)->canCollide() && isColliding(*it2))
		{
			collide(**it2);
		}
	}
}
*/

void Actor::die() {
	destroy();
}

bool Actor::canCollide() {
	return collideable;
}

void Actor::setCanCollide(bool col) {
	collideable = col;
	if(col) {
		shape->layers = shapeLayers;
	} else {
		shape->layers = 0;
	}
}

void Actor::goToGround() {
	while(!isGrounded() && pos_y < 32 * 256) {
		float mov_x = 0;
		float mov_y = 15;
		move(mov_x, mov_y);
	}
}

void Actor::collideGround() {
	grounded++;
}

void Actor::leaveGround() {
	grounded--;
}

void Actor::doUpdate(float dt) {
	update(dt);
	if(body && game_map) {
		sf::Vector2f pos = game_map->cp2sfml(body->p);
		setPos(pos.x, pos.y+height/2);
	}
}

void Actor::setLevel(int newLevel) {
	currentLevel = newLevel;
}

int Actor::getLevel() {
	return currentLevel;
}

void Actor::incrementLevel() {
	currentLevel++;
	onLevelUp(currentLevel);
}

int Actor::getExperienceValue() {
	return experienceValue;
}
	
void Actor::setExperienceValue(int exp) {
	experienceValue = exp;
}
	
void Actor::resetPhysics()
{
	// No map -> no physics
	if(!game_map || !game_map->physSpace) {
		return;
	}

	destroyPhysics();
	
	if(!staticBody) {
		body = cpSpaceAddBody(game_map->physSpace, cpBodyNew(10.0f, INFINITY));
		body->p = game_map->sfml2cp(sf::Vector2f(pos_x, pos_y - height/2));

		shape = cpSpaceAddShape(game_map->physSpace, cpBoxShapeNew(body, width, height));
		shape->e = 0.0f; shape->u = 2.0f;
	} else {
		
		cpVect verts[] = {
			cpv(-width/2, -height/2),			
			cpv(-width/2, height/2),			
			cpv(width/2, height/2),
			cpv(width/2, -height/2)
		};
		
		shape = cpSpaceAddShape(game_map->physSpace, cpPolyShapeNew(&game_map->physSpace->staticBody, 4, verts, game_map->sfml2cp(sf::Vector2f(pos_x, pos_y))));
	}
	
	shape->data = (void *) this;
}

void Actor::destroyPhysics() {
	if(body) {
		if(shape) 
			cpSpaceRemoveShape(game_map->physSpace, shape);
		
		cpSpaceRemoveBody(game_map->physSpace, body);
		
		if(shape)
			cpShapeFree(shape);
		
		cpBodyFree(body);

	} else if(shape) {
		// Static things like collectibles have no body.
		cpSpaceRemoveShape(game_map->physSpace, shape);
		cpShapeFree(shape);		
	}
	shape = NULL;
	body = NULL;
}

void Actor::setShapeLayers(cpLayers l) {
	shapeLayers = l;
	if(collideable) shape->layers = shapeLayers;
}

void Actor::freeze() {
	if(body) {
		//body->v = cpv(0, 0);
		body->velocity_func = no_gravity_stop;
	}
}

void Actor::unFreeze() {
	if(body) {
		body->velocity_func = defaultVelocityFunc;
	}
}

void no_gravity(struct cpBody *body, cpVect gravity, cpFloat damping, cpFloat dt) {
	cpBodyUpdateVelocity(body, cpv(0, 0), 1, dt);
}

void no_gravity_stop(struct cpBody *body, cpVect gravity, cpFloat damping, cpFloat dt) {
	cpBodyUpdateVelocity(body, cpv(0, 0), .1, dt);
}