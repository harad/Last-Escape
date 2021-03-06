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

/**
 *  Map
 */

#include "Map.h"

#include "tinyxml/tinyxml.h"

#include "Collectible.h"
#include "Enemy.h"
#include "EnemyWalker.h"
#include "EnemyCrawler.h"
#include "EnemyFlyer.h"
#include "EnemyCentipede.h"
#include "BossSpider.h"
#include "Particles.h"
#include "Player.h"
#include "Teleport.h"
#include "StartPoint.h"
#include "SpawnPoint.h"
#include "ExitPoint.h"
#include "Player.h"
#include "Actor.h"
#include <cstdlib>

Map::Map(string mapName) {

	physSpace = NULL;

	loadTileset("tileset.png");
	loaded = false;
	
	tile_sprite.SetImage(tileset);
	tile_sprite.FlipY(true);
	
	/*
	for (int i=0; i<VIEW_TILES_X; i++) {
		for (int j=0; j<VIEW_TILES_Y; j++) {
			tile_sprites[i][j].SetImage(tileset);
		}
	}
	*/

	// prep the rects for each tile
	for (int i=0; i<TILE_COUNT-1; i++) {

		// assumes tileset is 512px wide
		tile_rects[i+1].Left = (i % 16) * TILE_SIZE;
		tile_rects[i+1].Top = (i / 16) * TILE_SIZE;
		tile_rects[i+1].Right = tile_rects[i+1].Left + TILE_SIZE;
		tile_rects[i+1].Bottom = tile_rects[i+1].Top + TILE_SIZE;
	}

	cam_x = 0;
	cam_y = 0;
	cameraFollow = NULL;

	loadMap(mapName);
}

void Map::loadTileset(string filename) {

	tileset.LoadFromFile(("images/" + filename).c_str());
	tileset.SetSmooth(false);
}


void Map::loadMap(string filename) {
	loaded = false;
	ifstream infile;
	string line;
	string starts_with;
	string section;
	string key;
	string val;
	string cur_layer;
	int width;
	int height;
	this->initPhysics();
	game_map = this;
	//unsigned int comma; // Unreferenced local variable

	currentFilename = filename;

	for (int i=0; i<MAP_TILES_X; i++) {
		for (int j=0; j<MAP_TILES_Y; j++) {
			background[i][j] = 0;
			foreground[i][j] = 0;
			fringe[i][j] = 0;
			collision[i][j] = false;
		}
	}
	clear();

	TiXmlDocument doc;

	if(filename == "") {
		return;
	}
	else if (!doc.LoadFile(("maps/" + filename).c_str()))
	{
		printf("failed to open map\n");
		return;
	}

	TiXmlElement* root = doc.RootElement();
	root->QueryIntAttribute("width", &width);
	root->QueryIntAttribute("height", &height);

	
	for (TiXmlNode* child = root->FirstChild(); child; child = child->NextSibling())
	{
		std::string childName = child->Value();
		if (childName == "layer")
		{
			typedef int tiles_t[MAP_TILES_X][MAP_TILES_Y];
			tiles_t* tiles;

			std::string layerName = ((TiXmlElement*)child)->Attribute("name");
			if (layerName == "background")
				tiles = &background;
			else if (layerName == "fringe")
				tiles = &fringe;
			else if (layerName == "foreground")
				tiles = &foreground;
			else if (layerName == "collision")
				tiles = &collision;
			else if (layerName == "danger")
				tiles = &danger;
			else
				continue;

			const char* text = ((TiXmlElement*)child->FirstChild())->GetText();
			if (!text)
				continue;

			const char* start = text;
			for (int j = 0; j < height; j++) {
				for (int i = 0; i < width; i++) {
					const char* end = strchr(start, ',');
					if (!end)
						end = start + strlen(start);
					(*tiles)[i][j] = atoi(std::string(start, end).c_str());
					start = end+1;
				}
			}
		}
		else if (childName == "objectgroup")
		{
			const char* defaultType = NULL;

			// Look for a 'type' property as default for the whole group
			TiXmlElement* prop = TiXmlHandle(child).FirstChild("properties").FirstChild("property").ToElement();
			if (prop && strcmp(prop->Attribute("name"), "type") == 0)
				defaultType = prop->Attribute("value");

			for (TiXmlNode* object = child->FirstChild(); object; object = object->NextSibling())
			{
				// Skip everything except <object>
				if (strcmp(object->Value(), "object") != 0)
					continue;

				std::string type;
				if (((TiXmlElement*)object)->Attribute("type"))
				{
					type = ((TiXmlElement*)object)->Attribute("type");
				}
				else
				{
					if (defaultType)
						type = defaultType;
					else
						continue;
				}

				int w = 0, h = 0;
				((TiXmlElement*)object)->QueryIntAttribute("width", &w);
				((TiXmlElement*)object)->QueryIntAttribute("height", &h);
				const char* name = ((TiXmlElement*)object)->Attribute("name");

				float x = 0, y = 0;
				int temp_x, temp_y;
				((TiXmlElement*)object)->QueryIntAttribute("x", &temp_x);
				((TiXmlElement*)object)->QueryIntAttribute("y", &temp_y);
				
				// Convert to chipmunk coords
				cpVect pos = sfml2cp(sf::Vector2f(temp_x, temp_y));
				x = pos.x;
				y = pos.y;

				Actor* actor;
				if (type == "pill") {
					actor = new CollectiblePill((float)x, (float)y);
				} else if (type == "weaponupgrade") {
					actor = new CollectibleWeaponUpgrade((float)x, (float)y);
				} else if (type == "armor") {
					actor = new CollectibleArmor((float)x, (float)y);
				} else if (type == "smoke") {
					actor = new ParticleEmitter((float)x, (float)y);
				} else if (type == "walker") {
					actor = new EnemyWalker((float)x, (float)y);
				} else if (type == "crawler") {
					actor = new EnemyCrawler((float)x, (float)y);
				} else if (type == "flyer") {
					actor = new EnemyFlyer((float)x, (float)y);
				} else if (type == "centipede") {
					actor = new EnemyCentipede((float)x, (float)y);
				} else if (type == "spider") {
					actor = new BossSpider((float)x, (float)y);
				} else if (type == "teleportenter") {
					actor = new TeleportEnter((float)x, (float)y, w, h, name);
				} else if (type == "teleportexit") {
					actor = new TeleportExit((float)x, (float)y, name);
				} else if (type == "start") {
					actor = new StartPoint((float)x, (float)y);
				} else if (type == "spawn") {
					actor = new SpawnPoint((float)x, (float)y);
				} else if (type == "exit") {
					actor = new ExitPoint((float)x, (float)y, w, h);
					if (debugMode)
						cout << "Exit point\n";
					TiXmlElement* prop = TiXmlHandle(object).FirstChild("properties").FirstChild("property").ToElement();

					if(prop != NULL) {
						std::string mapname;
						std::string attrname = ((TiXmlElement*)prop)->Attribute("name");
						if(attrname == "map")
							mapname = ((TiXmlElement*)prop)->Attribute("value");
						if (!mapname.empty())
							dynamic_cast<ExitPoint *>(actor)->setMap(mapname);
					}
				} else {
					printf("unrecognised object type %s\n", type.c_str());
					continue;
				}
			}
		}
		else if(childName == "properties")
		{
			for (TiXmlNode* prop = child->FirstChild(); prop; prop = prop->NextSibling())
			{
				std::string propname = ((TiXmlElement*)prop)->Attribute("name");
				std::string propval = ((TiXmlElement*)prop)->Attribute("value");
				if(propname == "landscape") {
					landscapeImg.LoadFromFile("images/landscapes/" + propval);
					landscapeImg.SetSmooth(false);
					landscape.SetImage(landscapeImg);
				} else if(propname == "music" && enableMusic) {
                                        backgroundMusic.Stop();
                                        backgroundMusic.OpenFromFile("audio/" + propval);
					backgroundMusic.SetLoop(true);
					backgroundMusic.Play();
				}
			}
		}
	}

	this->setupPhysics();

	if(g_player == NULL) {
		g_player = new Player(0, 0);
	}
	g_player->init();
	cameraFollow = g_player;
	
	loaded = true;
}

void Map::setNextMap(string filename) {
	nextMap = filename;
}

void Map::loadNextMap() {
	if(nextMap != "") {
		loadMap(nextMap);
		nextMap = "";
	}
}

void Map::initPhysics()
{
	if(physSpace) {
		//cpSpaceFreeChildren(physSpace);
		cpSpaceFree(physSpace);
	}
	cpResetShapeIdCounter();

	physSpace = cpSpaceNew();
	physSpace->iterations = 10;
	physSpace->damping = 0.9f;
	physSpace->gravity = cpv(0, -1500);

}

bool Map::setupPhysics()
{
	// Possibly reset the physics.
	/*
	for (list<Actor*>::iterator it = actors.begin(); it != actors.end(); ++it) {
		Actor * actor = *it;
		actor->destroyPhysics();
	}
	*/


	// Ah, come on...
	/*
	for (list<Actor*>::iterator it = actors.begin(); it != actors.end(); ++it) {
		Actor * actor = *it;
		//actor->resetPhysics();
		
		if(actor->isPlayer()) {
			Player* p = dynamic_cast<Player*>(actor);
			//p->resetPhysics();
			p->init();
		}
	}
	*/
	
	// Vertical Pass
	/* This only accounts for on and off collision tiles now, but would be easy
	 * enough to expand to working with diagonals as well, if we do a diagonal
	 * pass and account for multiple tile types.
	 */
	for (int i=0; i<MAP_TILES_X - 1; i++) {
		bool prev_different = false;
		cpVect p1, p2;
		
		for (int j=0; j<MAP_TILES_Y; j++) {
			if(collision[i][MAP_TILES_Y - 1 - j] == collision[i+1][MAP_TILES_Y - 1 - j]) {
				if(prev_different) {
					//p2 = sfml2cp(sf::Vector2f(TILE_SIZE * (i + 1), TILE_SIZE * j - 1));
					p2 = cpv(TILE_SIZE * (i + 1), TILE_SIZE * j - 1);
					prev_different = false;
					createSegment(p1, p2, false);
				}
			} else {
				if(!prev_different) {
					//p1 = sfml2cp(sf::Vector2f(TILE_SIZE * (i + 1), TILE_SIZE * j + 1));
					p1 = cpv(TILE_SIZE * (i + 1), TILE_SIZE * j + 1);
					prev_different = true;
				}
			}
		}
		
		if(prev_different) {
			//p2 = sfml2cp(sf::Vector2f(TILE_SIZE * (i + 1), TILE_SIZE * MAP_TILES_Y - 1));
			p2 = cpv(TILE_SIZE * (i + 1), TILE_SIZE * MAP_TILES_Y - 1);
			createSegment(p1, p2, false);
		}
		prev_different = false;
	}
	
	// Horizontal pass
	
	//TODO: Only set ground for top borders.
	
	for (int j=0; j<MAP_TILES_Y - 1; j++) {
		bool prev_different = false;
		cpVect p1, p2;
		for (int i=0; i<MAP_TILES_X; i++) {	
			if(collision[i][MAP_TILES_Y - 1 - j] == collision[i][MAP_TILES_Y - j - 2]) {
				if(prev_different) {
					//p2 = sfml2cp(sf::Vector2f(TILE_SIZE * i - 1, TILE_SIZE * (j + 1)));
					p2 = cpv(TILE_SIZE * i - 1, TILE_SIZE * (j + 1));
					prev_different = false;
					createSegment(p1, p2, true);
				}
			} else {
				if(!prev_different) {
					//p1 = sfml2cp(sf::Vector2f(TILE_SIZE * i + 1, TILE_SIZE * (j + 1)));
					p1 = cpv(TILE_SIZE * i + 1, TILE_SIZE * (j + 1));
					prev_different = true;
				}
			}
		}
			
		if(prev_different) {
			//p2 = sfml2cp(sf::Vector2f(TILE_SIZE * MAP_TILES_X - 1, TILE_SIZE * (j + 1)));
			p2 = cpv(TILE_SIZE * MAP_TILES_X - 1, TILE_SIZE * (j + 1));
			createSegment(p1, p2, true);
		}
		prev_different = false;
	}
	
	//TODO: Diagonal pass once we have diagonal tiles
	
	
	// Set up collision handlers
	cpSpaceAddCollisionHandler(
		physSpace, 
		PhysicsType::Player, PhysicsType::Item, //types of objects
		map_begin_collide, // callback on initial collision
		NULL, // any time the shapes are touching
		NULL, // after the collision has been processed
		NULL, // after the shapes separate
		NULL // data pointer
	);
	
	cpSpaceAddCollisionHandler(
		physSpace, 
		PhysicsType::Player, PhysicsType::Enemy, //types of objects
		map_begin_collide, // callback on initial collision
		map_colliding, // any time the shapes are touching
		NULL, // after the collision has been processed
		map_end_collide, // after the shapes separate
		NULL // data pointer
	);
	
	cpSpaceAddCollisionHandler(
		physSpace, 
		PhysicsType::Player, PhysicsType::Sensor, //types of objects
		map_begin_collide, // callback on initial collision
		map_colliding, // any time the shapes are touching
		NULL, // after the collision has been processed
		map_end_collide, // after the shapes separate
		NULL // data pointer
	);
	
	cpSpaceAddCollisionHandler(
		physSpace, 
		PhysicsType::PlayerBullet, PhysicsType::Enemy, //types of objects
		map_begin_collide, // callback on initial collision
		map_colliding, // any time the shapes are touching
		NULL, // after the collision has been processed
		map_end_collide, // after the shapes separate
		NULL // data pointer
	);
	
	cpSpaceAddCollisionHandler(
		physSpace, 
		PhysicsType::Player, PhysicsType::Ground, //types of objects
		map_begin_ground_collide, // callback on initial collision
		NULL, // any time the shapes are touching
		NULL, // after the collision has been processed
		map_end_ground_collide, // after the shapes separate
		NULL // data pointer
	);

	cpSpaceAddCollisionHandler(
		physSpace, 
		PhysicsType::Enemy, PhysicsType::Ground, //types of objects
		map_begin_ground_collide, // callback on initial collision
		NULL, // any time the shapes are touching
		NULL, // after the collision has been processed
		map_end_ground_collide, // after the shapes separate
		NULL // data pointer
	);
	
	cpSpaceAddCollisionHandler(
		physSpace, 
		PhysicsType::Neutral, PhysicsType::Ground, //types of objects
		map_begin_ground_collide, // callback on initial collision
		NULL, // any time the shapes are touching
		NULL, // after the collision has been processed
		map_end_ground_collide, // after the shapes separate
		NULL // data pointer
	);
	
	cpSpaceAddCollisionHandler(
		physSpace, 
		PhysicsType::PlayerBullet, PhysicsType::Ground, //types of objects
		map_begin_ground_collide, // callback on initial collision
		NULL, // any time the shapes are touching
		NULL, // after the collision has been processed
		map_end_ground_collide, // after the shapes separate
		NULL // data pointer
	);
	
	cpSpaceAddCollisionHandler(
		physSpace, 
		PhysicsType::PlayerBullet, PhysicsType::Wall, //types of objects
		map_begin_ground_collide, // callback on initial collision
		NULL, // any time the shapes are touching
		NULL, // after the collision has been processed
		map_end_ground_collide, // after the shapes separate
		NULL // data pointer
	);
	
	
	// BUMPERS
	cpSpaceAddCollisionHandler(
		physSpace, 
		PhysicsType::Bumper, PhysicsType::Wall, //types of objects
		map_bumper_begin_collide, // callback on initial collision
		map_bumper_colliding, // any time the shapes are touching
		NULL, // after the collision has been processed
		map_bumper_end_collide, // after the shapes separate
		NULL // data pointer
	);
	
	cpSpaceAddCollisionHandler(
		physSpace, 
		PhysicsType::Bumper, PhysicsType::Enemy, //types of objects
		map_bumper_begin_collide, // callback on initial collision
		map_bumper_colliding, // any time the shapes are touching
		NULL, // after the collision has been processed
		map_bumper_end_collide, // after the shapes separate
		NULL // data pointer
	);
		
	cpSpaceAddCollisionHandler(
		physSpace, 
		PhysicsType::Bumper, PhysicsType::Ground, //types of objects
		map_bumper_begin_ground_collide, // callback on initial collision
		NULL, // any time the shapes are touching
		NULL, // after the collision has been processed
		map_bumper_end_ground_collide, // after the shapes separate
		NULL // data pointer
	);	
	
	return true;
}

static int map_begin_collide(cpArbiter *arb, cpSpace *space, void *data) {
	cpShape *a, *b; 
	cpArbiterGetShapes(arb, &a, &b);
	
	Actor *actor1 = (Actor *) a->data;
	Actor *actor2 = (Actor *) b->data;
	
	//cout << "Collision: " << actor1->actorName << " " << actor2->actorName << "\n"; 
	actor1->collide(*actor2);
	actor2->collide(*actor1);
	return 1;
}

static int map_colliding(cpArbiter *arb, cpSpace *space, void *data) {
	cpShape *a, *b; 
	cpArbiterGetShapes(arb, &a, &b);
	
	Actor *actor1 = (Actor *) a->data;
	Actor *actor2 = (Actor *) b->data;
	
	//cout << "Collision: " << actor1->actorName << " " << actor2->actorName << "\n"; 
	actor1->onColliding(*actor2);
	actor2->onColliding(*actor1);
	return 1;
}

static void map_end_collide(cpArbiter *arb, cpSpace *space, void *data) {
	cpShape *a, *b; 
	cpArbiterGetShapes(arb, &a, &b);
	
	Actor *actor1 = (Actor *) a->data;
	Actor *actor2 = (Actor *) b->data;
	
	//cout << "Collision: " << actor1->actorName << " " << actor2->actorName << "\n"; 
	actor1->onColliding(*actor2);
	actor2->onColliding(*actor1);
}


// Bumper collisions
static int map_bumper_begin_collide(cpArbiter *arb, cpSpace *space, void *data) {
	cpShape *a, *b; 
	cpArbiterGetShapes(arb, &a, &b);
	
	Bumper *bumper = (Bumper *) a->data;
	Actor *actor = bumper->actor;
	
	//cout << "Bumper collision: " << actor->actorName << "\n"; 
	actor->onBumperCollide(bumper->facing_direction);
	return 1;
}

static int map_bumper_colliding(cpArbiter *arb, cpSpace *space, void *data) {
	cpShape *a, *b; 
	cpArbiterGetShapes(arb, &a, &b);
	
	Bumper *bumper = (Bumper *) a->data;
	Actor *actor = bumper->actor;
	
	//cout << "Collision: " << actor1->actorName << " " << actor2->actorName << "\n"; 
	actor->onBumperColliding(bumper->facing_direction);
	return 1;
}

static void map_bumper_end_collide(cpArbiter *arb, cpSpace *space, void *data) {
	cpShape *a, *b; 
	cpArbiterGetShapes(arb, &a, &b);
	
	Bumper *bumper = (Bumper *) a->data;
	Actor *actor = bumper->actor;
	
	//cout << "Collision: " << actor1->actorName << " " << actor2->actorName << "\n"; 
	actor->onBumperEndCollide(bumper->facing_direction);
}


// Ground collisions
static int map_begin_ground_collide(cpArbiter *arb, cpSpace *space, void *data) {
	cpShape *a, *b; 
	cpArbiterGetShapes(arb, &a, &b);
	
	Actor *actor1 = (Actor *) a->data;
	
	//cout << "Ground collision: " << actor1->actorName << " " << actor1->grounded << "\n"; 
	actor1->collideGround();
	return 1;
}

static void map_end_ground_collide(cpArbiter *arb, cpSpace *space, void *data) {
	cpShape *a, *b; 
	cpArbiterGetShapes(arb, &a, &b);
	
	Actor *actor1 = (Actor *) a->data;
	
	//cout << "End ground collision: " << actor1->actorName << " " << actor1->grounded << "\n"; 
	actor1->leaveGround();
}

// Bumper ground collisions
static int map_bumper_begin_ground_collide(cpArbiter *arb, cpSpace *space, void *data) {
	cpShape *a, *b; 
	cpArbiterGetShapes(arb, &a, &b);
	
	Bumper *bumper = (Bumper *) a->data;
	
	//cout << "Bumper ground collision: " << bumper->actor->actorName << "\n"; 
	bumper->collideGround();
	return 1;
}

static void map_bumper_end_ground_collide(cpArbiter *arb, cpSpace *space, void *data) {
	cpShape *a, *b; 
	cpArbiterGetShapes(arb, &a, &b);
	
	Bumper *bumper = (Bumper *) a->data;
	
	//cout << "End ground collision: " << actor1->actorName << " " << actor1->grounded << "\n"; 
	bumper->leaveGround();
}

bool Map::isLoaded() {
	return loaded;
}


void Map::createSegment(cpVect &p1, cpVect &p2, bool ground) {
	if(debugMode) cout << "new segment (" << p1.x << ", " << p1.y << ") to (" << p2.x << ", " << p2.y << ")\n";
	cpShape * seg = cpSegmentShapeNew(&physSpace->staticBody, p1, p2, 1);
	seg->e = 0.0f;
	seg->u = 1.0f;
	seg->layers = PhysicsLayer::Map|PhysicsLayer::EnemyBullet;
	if(ground)
		seg->collision_type = PhysicsType::Ground;
	else
		seg->collision_type = PhysicsType::Wall;
	
	cpSpaceAddShape(physSpace, seg);
}

// Convert SFML 0,0 = top left csys and y down
// to cp 0,0 = bottom left csys and y up.
cpVect Map::sfml2cp(const sf::Vector2f& v) const
{
	return cpv(v.x, MAP_TILES_Y*TILE_SIZE - v.y);
}

// Convert cp 0,0 = bottom left csys and y up
// to SFML 0,0 = top left csys and y down.
sf::Vector2f Map::cp2sfml(const cpVect& v) const
{
	return sf::Vector2f(v.x, MAP_TILES_Y*TILE_SIZE - v.y);
}

void Map::setCameraFollow(Actor * actor) {
	cameraFollow = actor;
}

bool Map::isOnInstantdeath(Actor &actor)
{
	//return danger[(int)actor.pos_x/TILE_SIZE][(int)actor.pos_y/TILE_SIZE] != 0;
	return false;
}

/*
bool Map::isSolid(int x, int y) {
	return collision[x/TILE_SIZE][y/TILE_SIZE] != 0;
}
*/

void Map::renderBackground() {
	int cam_x, cam_y;
	if(cameraFollow != NULL && cameraFollow->body) {	
		gameView.SetCenter(cameraFollow->body->p.x, cameraFollow->body->p.y);
	}

	
	// which tile is at the bottom left corner of the screen?
	//cout << cam_tile_x << ", " << cam_tile_y << "\n";
	/*
	// how far offset is this tile (and each subsequent tiles)?
	int cam_off_x = (int)(gameView.GetRect().GetWidth()) % TILE_SIZE;
	int cam_off_y = (int)(gameView.GetRect().GetHeight()) % TILE_SIZE;

	// apply camera
	for (int i=0; i<VIEW_TILES_X; i++) {
		for (int j=0; j<VIEW_TILES_Y; j++) {
		+ cam_off_y	tile_sprites[i][j].SetPosition((cam_tile_x + i)*TILE_SIZE, (cam_tile_y + j)*TILE_SIZE);
		}
	}

	// render background
	for (int i=0; i<VIEW_TILES_X; i++) {
		if (cam_tile_x + i < 0 || cam_tile_x + i >= MAP_TILES_X) continue;
		for (int j=0; j<VIEW_TILES_Y; j++) {
			if (MAP_TILES_Y - cam_tile_y - j - 1 < 0 || MAP_TILES_Y - cam_tile_y - j - 1 >= MAP_TILES_Y) continue;
			if (!background[cam_tile_x + i][cam_tile_y + j]) continue;
			tile_sprites[i][j].SetSubRect(tile_rects[background[cam_tile_x + i][MAP_TILES_Y - cam_tile_y - j - 1]]);
			App->Draw(tile_sprites[i][j]);
		}
	}

	// render fringe
	for (int i=0; i<VIEW_TILES_X; i++) {
		if (cam_tile_x + i < 0 || cam_tile_x + i >= MAP_TILES_X) continue;
		for (int j=0; j<VIEW_TILES_Y; j++) {
			if (MAP_TILES_Y - cam_tile_y - j - 1 < 0 || MAP_TILES_Y - cam_tile_y - j - 1 >= MAP_TILES_Y) continue;
			if (!fringe[cam_tile_x + i][cam_tile_y + j]) continue;
			tile_sprites[i][j].SetSubRect(tile_rects[fringe[cam_tile_x + i][MAP_TILES_Y - cam_tile_y - j - 1]]);
			App->Draw(tile_sprites[i][j]);
		}
	}
	*/

	sf::FloatRect rect = gameView.GetRect();
	
	int cam_tile_x = rect.Left / TILE_SIZE;
	int cam_tile_y = rect.Bottom / TILE_SIZE;
	cam_tile_y = MAP_TILES_Y - cam_tile_y;
	
	int tile_w = rect.GetWidth() / TILE_SIZE + 2;
	int tile_h = rect.GetHeight() / TILE_SIZE + 2;
	
	for(int i = max(cam_tile_x, 0); i < min(cam_tile_x + tile_w, MAP_TILES_X); i++) {
		for(int j = max(cam_tile_y, 0); j < min(cam_tile_y + tile_h, MAP_TILES_Y); j++) {
			tile_sprite.SetPosition(i * TILE_SIZE, (MAP_TILES_Y - j - 1) * TILE_SIZE);
			
			tile_sprite.SetSubRect(tile_rects[background[i][j]]);
			App->Draw(tile_sprite);
			tile_sprite.SetSubRect(tile_rects[fringe[i][j]]);
			App->Draw(tile_sprite);
		}
	}
}

void Map::actorDestroyed(Actor * actor) {
	if(actor == cameraFollow) {
		cameraFollow = NULL;
	}
}

// and fringe
void Map::renderForeground() {
	/*
	// which tile is at the topleft corner of the screen?
	int cam_tile_x = cam_x / TILE_SIZE;
	int cam_tile_y = cam_y / TILE_SIZE;

	// how far offset is this tile (and each subsequent tiles)?
	int cam_off_x = cam_x % TILE_SIZE;
	int cam_off_y = cam_y % TILE_SIZE;

	// apply camera
	for (int i=0; i<VIEW_TILES_X; i++) {
		for (int j=0; j<VIEW_TILES_Y; j++) {
			tile_sprites[i][j].SetPosition(i*32 - cam_off_x + 0.5f, j*32 - cam_off_y + 0.5f);
		}
	}

	// render foreground
	for (int i=0; i<VIEW_TILES_X; i++) {
		if (cam_tile_x + i < 0 || cam_tile_x + i >= MAP_TILES_X) continue;
		for (int j=0; j<VIEW_TILES_Y; j++) {
			if (cam_tile_y + j < 0 || cam_tile_y + j >= MAP_TILES_Y) continue;
			if (!foreground[cam_tile_x + i][cam_tile_y + j]) continue;
			tile_sprites[i][j].SetSubRect(tile_rects[foreground[cam_tile_x + i][cam_tile_y + j]]);
			App->Draw(tile_sprites[i][j]);
		}
	}
	*/
	
	sf::FloatRect rect = gameView.GetRect();
	
	int cam_tile_x = rect.Left / TILE_SIZE;
	int cam_tile_y = rect.Bottom / TILE_SIZE;
	cam_tile_y = MAP_TILES_Y - cam_tile_y;
	
	int tile_w = rect.GetWidth() / TILE_SIZE + 2;
	int tile_h = rect.GetHeight() / TILE_SIZE + 2;
	
	for(int i = max(cam_tile_x, 0); i < min(cam_tile_x + tile_w, MAP_TILES_X); i++) {
		for(int j = max(cam_tile_y, 0); j < min(cam_tile_y + tile_h, MAP_TILES_Y); j++) {
			tile_sprite.SetPosition(i * TILE_SIZE, (MAP_TILES_Y - j - 1) * TILE_SIZE);
			
			tile_sprite.SetSubRect(tile_rects[foreground[i][j]]);
			App->Draw(tile_sprite);
		}
	}
}

void Map::renderLandscape() {
	// Do nothing if no landscape was specified
	if (!landscapeImg.GetWidth())
		return;
	
	sf::Vector2f cam = gameView.GetCenter();
	cam_x = cam.x;
	cam_y = MAP_TILES_Y * TILE_SIZE - cam.y;
	
	// Draw it four times, aka repeating in X and Y
	sf::Vector2f topleft(-(float)(cam_x/10 % landscapeImg.GetWidth()), -(float)(cam_y/10 % landscapeImg.GetHeight()));
	landscape.SetPosition(topleft);
	App->Draw(landscape);
	landscape.SetPosition(topleft.x + landscapeImg.GetWidth(), topleft.y);
	App->Draw(landscape);
	landscape.SetPosition(topleft.x, topleft.y + landscapeImg.GetHeight());
	App->Draw(landscape);
	landscape.SetPosition(topleft.x + landscapeImg.GetWidth(), topleft.y + landscapeImg.GetHeight());
	App->Draw(landscape);
}

void Map::clear() {
	for (list<Actor*>::iterator it = actors.begin(); it != actors.end(); ++it) {
		Actor * actor = *it;
		if(!actor->isPlayer()) actor->destroy();
	}
}

Map::~Map() {
	clear();
}
