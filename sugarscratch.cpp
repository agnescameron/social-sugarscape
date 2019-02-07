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

// for convenience
using json = nlohmann::json;
using namespace std;

int random(int min, int max) {
  return min + rand() % (max - min);
}

float currentSent;

class World;

typedef struct Glance {
  int x, y;
  int sugar;
  int spice;
  bool occupied;
} Glance;

class Bug {
  friend class World;
  private:
    World *world;
    int x, y, id;
    int nextX, nextY;
    int sugar;
    int spice;
    float metabolism;

    Glance *look(int dx, int dy);
  public:
    Bug(World *_world, int _id, int _x, int _y, int _sugar, int _spice, float _metabolism) {
      world = _world;
      id = _id;
      x = _x;
      y = _y;
      sugar = _sugar;
      spice = _spice;
      metabolism = _metabolism;
    };
    void update(json &bugStep);
    void think();
    void trade();    
};

class World {
  private:
    const int MIN_SUGAR = 1;
    const int MAX_SUGAR = 6;
    const int MIN_SPICE = 0;
    const int MAX_SPICE = 3;    
    const int width, height;
    vector <vector<int> > cells;
    vector<Bug*> bugs;
  public:
    int clk = 0;    
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
      }

      // Add the bugs in random locations
      for (int i = 0; i < bugCount; i++) {
        int x, y;
        int id=i;
        int sugar = 0;
        int spice = 0;
        float metabolism = 0.5;

        do {
          x = random(0, width);
          y = random(0, height);
        } while (occupied(x, y));

        bugs.push_back(new Bug(this, id, x, y, sugar, spice, metabolism));
      }
    }

    void update(json &bugStep);
    bool occupied(int x, int y);
    int getSugar(int x, int y);
    int getSpice(int x, int y);
    void regrowSugar();
    bool inBounds(int x, int y);
    int eat(int x, int y);
    bool gather(int x, int y);
    void printSugar();
    void printSpice();    
    void printAppetites();
};


int callback(void *data, int argc, char **argv, char **azColName){
   int i;
   fprintf(stderr, "%s: ", (const char*)data);
   
   for(i = 0; i<argc; i++){
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   
   currentSent = atof(argv[0]);
   printf("\n");
   return 0;
}

int getSentiment(int row) {
   sqlite3 *db;
   char *zErrMsg = 0;
   int rc;
   char sql[256];
   const char* data = "Callback function called";

   int sentScore = 5;

   rc = sqlite3_open("ishaan/feels.sqlite", &db);

   if( rc ) {
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return(0);
   } else {
      fprintf(stderr, "Opened database successfully\n");
   }

/* Create SQL statement */
   sprintf(sql, "SELECT sentiment from tweets LIMIT 1 OFFSET %d", row);

   /* Execute SQL statement */
   rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);
   
   if( rc != SQLITE_OK ) {
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
   } else {
      fprintf(stdout, "Operation done successfully\n");
   }

   sqlite3_close(db);
   return sentScore;
}

int World::eat(int x, int y) {
  if (!inBounds(x, y)) {
    // Nothing to eat in the wastes
    return 0;
  }

  int i = y * width + x;

  int sugar = cells[0][i];
  cells[0][i] = 0;

  return sugar;
}

bool World::gather(int x, int y) {
  if (!inBounds(x, y)) {
    // Nothing to eat in the wastes
    return 0;
  }

  int i = y * width + x;
  int spice = cells[1][i];

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

void World::regrowSugar(){
  //regenerate sugar randomly for each cell
  getSentiment(clk);
  int regrowth;
  if(currentSent>0){
  regrowth = ceil((float)9*currentSent);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int i = y * height + x;
      if (cells[0][i] == 0) {
          int sugar = (rand() % regrowth);
          if(sugar > 9){
            cells[0][i] = 9;
          }
          else cells[0][i] = sugar;
      }
    }
  }
}
}

int World::getSugar(int x, int y) {
  if (!inBounds(x, y)) {
    // Only wastes lie beyond
    return false;
  }

  return cells[0][y * width + x];
}

int World::getSpice(int x, int y) {
  if (!inBounds(x, y)) {
    // Only wastes lie beyond
    return false;
  }

  return cells[1][y * width + x];
}

void World::update(json &bugStep) {

  regrowSugar();

  // Update the bugs in a random order
  vector<Bug*> shuffledBugs = vector<Bug*>(bugs);
  random_shuffle(shuffledBugs.begin(), shuffledBugs.end());

  // Let everyone figure out there next move without the world changing
  for (auto bug : shuffledBugs) {
    bug->think();
  }

  // Let everyone figure out their next move without the world changing
  for (auto bug : shuffledBugs) {
    bug->trade();
  }

  // Let everyone affect the world
  for (auto bug : shuffledBugs) {
    bug->update(bugStep);
  }
}

void World::printSugar() {
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int i = y * height + x;

      if (occupied(x, y)) {
        printf("| ");
      } else {
        if (cells[0][i] <= 9) {
          printf("|%d", cells[0][i]);
        } else {
          printf("^");
        }
      }
    }

    printf("|\n");
  }
}

void World::printSpice() {
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int i = y * height + x;

      if (occupied(x, y)) {
        printf("| ");
      } else {
        if (cells[1][i] <= 9) {
          printf("|%d", cells[1][i]);
        } else {
          printf("^");
        }
      }
    }

    printf("|\n");
  }
}


void getTiles(int sugar, string &tile1, string &tile2){
  if(sugar <= -100){
    tile1 = " ";
    tile2 = " ";
  }
  if(sugar > -100 && sugar <= -50){
    tile1 = " ";
    tile2 = "░";
  }
  if(sugar > -50 && sugar <= 0){
    tile1 = "░";
    tile2 = "▒";       
  }
  if(sugar > 0 && sugar <= 50){
    tile1 = "▒";
    tile2 = "▓";       
  }
  if(sugar > 50 && sugar <= 100){
    tile1 = "▓";
    tile2 = "█";         
  }
  if(sugar > 100){
    tile1 = "█";
    tile2 = "█";         
  }

}

void getPattern(int sugar, string &pattern){
  if(sugar < 2){
    pattern = "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
  }
  else if(sugar < 4){
    pattern = "00000000000000000000000000000000000001111110000000000111111000000000011111100000000001111110000000000000000000000000000000000000";
  }
  else if(sugar < 6){
    pattern = "00000000000000000000011111100000000011000011000000001101101100000000110110110000000011000011000000000111111000000000000000000000";
  }
  else if(sugar < 8){
    pattern = "00001111111100000010100000010100010100011000101010100011110001011010001111000101010100011000101000101000000101000000111111110000";
  }
  else{
    pattern = "11111111111111111110110000110111110010011001001111010110011010111101011001101011110010011001001111101100001101111111111111111111";
  }
}

//this is specific to the 16-bug system
void World::printAppetites() {
string stateArr[16*4][8*4];
string tempArr[16][8];

for(int y=0; y<4; y++){
  for(int x=0; x<4; x++){
      //calculate i,j values for square
      // assign tiles to (i,j) elements in array
      int sugar = bugs[x+y*4]->sugar;
      string tile1;
      string tile2;
      string pattern;

      getTiles(sugar, tile1, tile2);
      getPattern(sugar, pattern);

      for(int j=0; j<8; j++){
        for(int i=0; i<16; i++){
          if(pattern[i + j*16] == '0'){
            tempArr[i][j] = tile1;
            stateArr[(x*16)+i][(y*8)+j] = tile1;
          }
          else{
            tempArr[i][j] = tile2;
            stateArr[(x*16)+i][(y*8)+j] = tile2;
          }
        }
      }
    }
  }
  //print array here:
  for(int y=0; y<8*4; y++){
      for(int x=0; x<16*4; x++){
        cout << stateArr[x][y];
    }
    cout << endl;
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
  };
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
  remove_if(sight.begin(), sight.end(), [](const Glance *a) {
    return a->occupied;
  }); 

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
}

void Bug::trade() {
  Glance *up = look(0, -1);
  Glance *down = look(0, 1);
  Glance *left = look(-1, 0);
  Glance *right = look(1, 0);

  vector<Glance*> sight = { up, down, left, right};
  random_shuffle(sight.begin(), sight.end());

  // Filter out the non-occupied cells
  remove_if(sight.begin(), sight.end(), [](const Glance *a) {
    return !(a->occupied);
  }); 

  if (sight.size() == 0) {
    // There's nowhere to move
    return;
  }

  // Sort the cells we looked at by decreasing sugar value
  sort(sight.begin(), sight.end(), [](const Glance *a, const Glance *b) {
    return a->spice > b->spice;
  });

}


void Bug::update(json &bugStep) {
  json bug ={
    {"hexID", id},
    {"moveX", nextX-x},
    {"moveY", nextY-y},
    {"sugar", sugar},
  };
  bugStep.push_back(bug);

  x = nextX;
  y = nextY;

  int foundSugar = world->eat(x, y);
  if (world->gather(x, y)){
    spice = spice + 1;
  }
  sugar = sugar + foundSugar - round(float(sugar)*metabolism);
}


int main(int argc, char **argv) {
  // Seed PRNG
  srand(time(NULL));
  World world(50, 50, 16);
  vector<json> bugSteps;
  //initialise output file for JSON
  ofstream jsonFile;
  jsonFile.open("bugs.json");

  while (world.clk<1000) {

  json bugs;
  world.update(bugs);
  bugSteps.push_back(bugs);
  printf("\e[2J");
  // world.printSugar();
  world.printSpice();
  //world.printAppetites();
  usleep(90 * 1000);
  //increment the timer
  world.clk++;
  cout << world.clk << endl;
  }

  jsonFile << bugSteps << endl;

}
