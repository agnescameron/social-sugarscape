// Based on https://github.com/agnescameron/sugarscape

#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <vector>
#include <sqlite3.h>
#include <iostream> 
#include <math.h>
#include "json.hpp"
#include <fstream>
#include <valarray>

// for convenience
using json = nlohmann::json;
using namespace std;

int random(int min, int max) {
  return min + rand() % (max - min);
}

float currentSent;
int databaseSize;

class World;

typedef struct Glance {
  int x, y;
  float sugar;
  float spice;
  bool occupied;
  float metabolicRatio;
} Glance;

class Bug {
  friend class World;
  private:
    World *world;
    int x, y, id;
    int nextX, nextY;
    float sugar;
    float spice;
    int timeToLive;
    bool traded;
    float metabolism;

    Glance *look(int dx, int dy);
  public:
    Bug(World *_world, int _id, int _x, int _y, float _sugar, float _spice, 
      bool _traded, float _metabolism, int _timeToLive) {
      world = _world;
      id = _id;
      x = _x;
      y = _y;
      sugar = _sugar;
      spice = _spice;
      traded = _traded;
      metabolism = _metabolism;
      timeToLive = _timeToLive;
    };
    void update(json &bugStep);
    void updateLifecycle();
    float calculateRatio();
    void think();
    void trade();    
};

class World {
  private:
    const float MIN_SUGAR = 1.0;
    const float MAX_SUGAR = 6.0;
    const float MIN_SPICE = 0.0;
    const float MAX_SPICE = 3.0;
    const int MIN_AGE = 40;
    const int MAX_AGE = 150; 
    const int width, height;
    vector <vector<int> > cells;
  public:
    vector<Bug*> bugs;
    int clk = 0;
    float totalSpiceBefore = 0;
    World(int _width, int _height, int bugCount)
      : width(_width), height(_height) {
      //set the clock
      // We need enough cells for all the bugs (max 1 bug per cell)
      assert(bugCount < (width * height));
      // set up cells as 2d vector with room for spice
      cells = vector<vector<int>> (2, vector<int>(width * height, 0));
      for (int i = 0; i < cells[0].size(); i++) {
        cells[0][i] = random(MIN_SUGAR, MAX_SUGAR);
        cells[1][i] = random(MIN_SPICE, MAX_SPICE);
        totalSpiceBefore = totalSpiceBefore + cells[1][i];
      }

      // Add the bugs in random locations
      for (int i = 0; i < bugCount; i++) {
        int x, y;
        int id=i;
        float sugar = 0;
        float spice = 0;
        bool traded = false;
        float metabolism = 0.5;
        int timeToLive = random(MIN_AGE, MAX_AGE);

        do {
          x = random(0, width);
          y = random(0, height);
        } while (occupied(x, y));

        bugs.push_back(new Bug(this, id, x, y, sugar, spice, traded, metabolism, timeToLive));
      }
    }

    void update(json &bugStep);
    bool occupied(int x, int y);
    float metabolicRatio(int x, int y);
    float getSugar(int x, int y);
    float getSpice(int x, int y);
    void regrowSugar();
    bool inBounds(int x, int y);
    int eat(int x, int y);
    bool gather(int x, int y);
    void print(int selection);
    void printAppetites();
    void calculateAppetites(int res_x, int res_y, json &appetites);
    void reincarnate(int id);
    void trackBugs(int numTracked, json &trackedBugs);
};


int callback_entries(void *data, int argc, char **argv, char **azColName){
  databaseSize = atoi(argv[0]);
  return 0;
}

int callback(void *data, int argc, char **argv, char **azColName){
   int i;
   fprintf(stderr, "%s: ", (const char*)data);
   
   for(i = 0; i<argc; i++){
      // printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
  //change shit here!

   // cout << "sent is " << argv[3] << endl;
   currentSent = atof(argv[0]);
   //atof(argv[0]);
   //printf("\n");
   return 0;
}

int getNumEntries() {
   sqlite3 *db;
   char *zErrMsg = 0;
   int rc;
   char sql[256];
   const char* data = "Callback function called";

   rc = sqlite3_open("feelings.db", &db);

   if( rc ) {
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return(0);
   }

   sprintf(sql, "SELECT MAX(id) FROM feelings");

   rc = sqlite3_exec(db, sql, callback_entries, (void*)data, &zErrMsg);
   
   if( rc != SQLITE_OK ) {
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
   }

   sqlite3_close(db);
   return 0;
}

int getSentiment(int row) {
   sqlite3 *db;
   char *zErrMsg = 0;
   int rc;
   char sql[256];
   const char* data = "Callback function called";

   rc = sqlite3_open("feelings.db", &db);

   if( rc ) {
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return(0);
   }

/* Create SQL statement */
   sprintf(sql, "SELECT Anger, Disgust, Joy, Sadness, Emotion from feelings LIMIT 1 OFFSET %d", row);

   /* Execute SQL statement */
   rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);
   
   if( rc != SQLITE_OK ) {
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
   }

   sqlite3_close(db);
   return 0;
}

int World::eat(int x, int y) {
  if (!inBounds(x, y)) {
    // Nothing to eat in the wastes
    return 0;
  }

  int i = y * width + x;

  float sugar = cells[0][i];
  cells[0][i] = 0;

  return sugar;
}

bool World::gather(int x, int y) {
  if (!inBounds(x, y)) {
    // Nothing to eat in the wastes
    return 0;
  }

  int i = y * width + x;
  float spice = cells[1][i];

  if(spice != 0){
    cells[1][i] = spice-1;
    return true;
  }

  else
    return false;
}

bool World::inBounds(int x, int y) {
  return x >= 0 && x < width && y >= 0 && y < height;
}

bool World::occupied(int x, int y) {
  if (!inBounds(x, y)) {
    // Only death outside
    return false;
  }

  //is there a bug there already
  for (auto bug : bugs) {
    if (bug->x == x && bug->y == y) {
      return true;
    }
  }

  return false;
}


float World::metabolicRatio(int x, int y) {
  if (!inBounds(x, y)) {
    // Only death outside
    return 0.0;
  }

  //if a bug there, return
  for (auto bug : bugs) {
    if (bug->x == x && bug->y == y) {
      float ratio = bug->calculateRatio();
      return ratio;
    }
  }

  //else nothing happens 
  return 0.0;
}

void World::regrowSugar(){
  //regenerate sugar randomly for each cell
  getSentiment(clk%databaseSize);
  int regrowth;
  if(currentSent>0){
  regrowth = ceil((float)9*currentSent);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int i = y * height + x;
      if (cells[0][i] == 0) {
          float sugar = (rand() % regrowth);
          if(sugar > 9.0){
            cells[0][i] = 9.0;
          }
          else cells[0][i] = sugar;
      }
    }
  }
}
}

float World::getSugar(int x, int y) {
  if (!inBounds(x, y)) {
    // Only wastes lie beyond
    return false;
  }

  return cells[0][y * width + x];
}

float World::getSpice(int x, int y) {
  if (!inBounds(x, y)) {
    // Only wastes lie beyond
    return false;
  }

  return cells[1][y * width + x];
}

void World::update(json &bugStep) {

  //sort the world out so the friends can snack
  regrowSugar();

  // Update the bugs in a random order
  vector<Bug*> shuffledBugs = vector<Bug*>(bugs);
  random_shuffle(shuffledBugs.begin(), shuffledBugs.end());

  // Let everyone figure out their next move without the world changing
  for (auto bug : shuffledBugs) {
    bug->think();
  }

  // Let the bugs trade with one another
  for (auto bug : shuffledBugs) {
    bug->trade();
  }

  // Let everyone affect the world
  for (auto bug : shuffledBugs) {
    bug->update(bugStep);
  }

  // Let the world affect everyone (death)
  for (auto bug : shuffledBugs) {
    bug->updateLifecycle();
  }
}

void World::print(int s) {
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int i = y * height + x;

      if (occupied(x, y)) {
        printf("| ");
      } else {
        if (cells[s][i] <= 9) {
          printf("|%d", cells[s][i]);
        } else {
          printf("^");
        }
      }
    }

    printf("|\n");
  }
}

//takes segments of the grid and calculates the 'appetites'
//of the bugs in that space
void World::calculateAppetites(int res_x, int res_y, json &appetites) {
  int seg_x = floor(width/res_x);
  int seg_y = floor(height/res_y);
  int segCells = seg_x*seg_y;
  valarray <int> segVec(res_x*res_y);

  //for each segment in the grid, get and average the resources
  for(int i=0; i<res_x*res_y; i++){
    int segSum = 0;
    for(int j=0; j<segCells; j++){
      segSum = segSum + cells[0][j+i*segCells];
    }
    segVec[i] = segSum;
  }

  //now, normalise for 0-255
  // int maxSeg = segVec.max();
  // int minSeg = segVec.min(); 
  segVec = segVec-segVec.min();
  segVec = segVec*(255/segVec.max());
  float avSeg = segVec.sum()/segVec.size();
  int range = segVec.max() - segVec.min(); 
  cout << range << "  " << avSeg << endl;

  appetites = segVec;
}

void World::trackBugs(int numTracked, json &trackedBugs){
  valarray <float> sugar(numTracked);

  //iterate over the bugs vector
  //for the bug with id X, write that id into
  //tracked bugs in the right place
  
  for(int i = 0; i<numTracked; i++){
    for (auto bug : bugs) {
      if (bug->id == i) {
        sugar[i] = bug->sugar;
      }
    }
  }

  trackedBugs = sugar;
}

void World::reincarnate(int id) {
  // delete bug from list
  auto remove = remove_if(bugs.begin(), bugs.end(), [&](const Bug *bug) {
    return (bug->id == id);
  });
  
  bugs.erase(remove, bugs.end());

  // generate a new bug in a random unoccupied square with same id
  int x, y;
  float sugar = 0;
  float spice = 0;
  bool traded = false;
  float metabolism = 0.5;
  int timeToLive = random(MIN_AGE, MAX_AGE);

  do {
    x = random(0, width);
    y = random(0, height);
  } while (occupied(x, y));

  bugs.push_back(new Bug(this, id, x, y, sugar, spice, traded, metabolism, timeToLive));

}

//death function
void Bug::updateLifecycle() {
  // printf("not dead yet");
  timeToLive = timeToLive-1;
  if(timeToLive == 0 || sugar == 0.0){
    world->reincarnate(id);
  }
}


Glance *Bug::look(int dx, int dy) {
  int targetX = x + dx;
  int targetY = y + dy;

  return new Glance {
    .x = targetX,
    .y = targetY,
    .sugar = world->getSugar(targetX, targetY),
    .spice = world->getSpice(targetX, targetY),        
    .occupied = world->occupied(targetX, targetY),
    .metabolicRatio = world->metabolicRatio(targetX, targetY),
  };
}

float Bug::calculateRatio(){
  float ratio;

  if (sugar == 0)
    ratio = 0.0;

  else
    ratio = sqrt(spice*metabolism/sugar);
  
  return ratio;
}


void Bug::think() {
  Glance *up = look(0, -1);
  Glance *down = look(0, 1);
  Glance *left = look(-1, 0);
  Glance *right = look(1, 0);
  Glance *twoUp = look(0, -2);
  Glance *twoDown = look(0, 2);
  Glance *twoLeft = look(-2, 0);
  Glance *twoRight = look(2, 0);

  vector<Glance*> sight = { up, down, left, right, twoUp, twoDown, twoLeft, twoRight };
  random_shuffle(sight.begin(), sight.end());

  // Filter out the occupied cells
  auto remove = remove_if(sight.begin(), sight.end(), [](const Glance *a) {
    return a->occupied;
  }); 
  sight.erase(remove, sight.end());

  if (sight.size() == 0) {
    // There's nowhere to move
    return;
  }

  // Sort the cells we looked at by decreasing sugar value
  sort(sight.begin(), sight.end(), [](const Glance *a, const Glance *b) {
    return a->sugar > b->sugar;
  });

  // Plan to move to the cell with the highest sugar value
  Glance *target = sight.front();
  nextX = target->x;
  nextY = target->y;

  for ( int i = 0; i < sight.size(); i++ ) 
  {       
    delete sight[i];    
  }    
  sight.clear(); 

}

void Bug::trade() {
  Glance *up = look(0, -1);
  Glance *down = look(0, 1);
  Glance *left = look(-1, 0);
  Glance *right = look(1, 0);
  Glance *twoUp = look(0, -2);
  Glance *twoDown = look(0, 2);
  Glance *twoLeft = look(-2, 0);
  Glance *twoRight = look(2, 0);

  vector<Glance*> sight = { up, down, left, right, twoUp, twoDown, twoLeft, twoRight };
  random_shuffle(sight.begin(), sight.end());

  // Filter out the non-occupied cells
  auto remove = remove_if(sight.begin(), sight.end(), [](const Glance *a) {
    return !(a->occupied);
  }); 

  float myRatio = calculateRatio();

  sight.erase(remove, sight.end());

  if (sight.size() == 0) {
    // There's nobody to trade with
    return;
  }

  // Sort the cells we looked at by decreasing metabolic ratio value
  sort(sight.begin(), sight.end(), [](const Glance *a, const Glance *b) {
    return a->metabolicRatio > b->metabolicRatio;
  });

  //this is the guy we might want to trade with
  float neighborRatio = sight[0]->metabolicRatio;

  if(myRatio <= neighborRatio){
    return;
  }

  float price = sqrt(myRatio*neighborRatio);

  if(isnan(price)) return; //none of that!

  for (auto bug : world->bugs) { 
    if (bug->x == sight[0]->x && bug->y == sight[0]->y) {
      //no trading twice!
      if(bug->traded == true){
        return;
      }
      else {
        sugar = sugar + 1;
        spice = spice - price;
        traded = true;
        bug->sugar = bug->sugar - 1;
        bug->spice = bug->spice + price;
        bug->traded = true;      
      }
    }
  }

  float totalSugar = 0;
  float totalSpice = 0;
  for (auto bug : world->bugs) {
    totalSugar = totalSugar + bug->sugar;
    totalSpice = totalSpice + bug->spice;
  }

  for ( int i = 0; i < sight.size(); i++ ) 
  {       
    delete sight[i];    
  }    
  sight.clear();   
}


void Bug::update(json &bugStep) {
  json bug ={
    {"hexID", id},
    {"moveX", nextX-x},
    {"moveY", nextY-y},
    {"sugar", sugar},
    {"spice", spice},    
  };
  bugStep.push_back(bug);

  x = nextX;
  y = nextY;

  int foundSugar = world->eat(x, y);
  if (world->gather(x, y)){
    spice = spice + 1;
  }
  sugar = sugar + foundSugar - sugar*metabolism;
  traded = false;
}


int main(int argc, char **argv) {
  // Seed PRNG
  srand(time(NULL));
  World world(50, 50, 100);
  getNumEntries();
  //for json output
  int res_x = 6;
  int res_y = 8;  
  vector<json> bugTracker;
  //outputs 2x2 file
  vector<json> bugAppetites;

  //initialise output file for JSON
  ofstream bugTrackerJson;
  ofstream bugAppetitesJson;
  bugTrackerJson.open("bugTracker.json");
  bugAppetitesJson.open("bugApp-6-8.json");

  while (world.clk<100) {
  json bugs, appetites, trackedBugs;
  world.update(bugs);
  // printf("\e[2J");
  //0 for sugar, 1 for spice
  world.print(0);
  world.calculateAppetites(res_x, res_y, appetites);
  world.trackBugs(res_x*res_y, trackedBugs);
  bugTracker.push_back(trackedBugs);
  bugAppetites.push_back(appetites);
  usleep(90 * 1000);

  //increment the timer
  world.clk++;
  // cout << world.clk << endl;
  }

  bugTrackerJson << bugTracker << endl;
  bugAppetitesJson << bugAppetites << endl;

}
