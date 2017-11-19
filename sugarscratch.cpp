// Based on https://github.com/agnescameron/sugarscape

#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <vector>

using namespace std;

int random(int min, int max) {
  return min + rand() % (max - min);
}

class World;

typedef struct Glance {
  int x, y;
  int sugar;
  bool occupied;
} Glance;
    
class Bug {
  friend class World;
  private:
    World *world;
    int x, y;
    int nextX, nextY;

    Glance *look(int dx, int dy);
  public:
    Bug(World *_world, int _x, int _y) {
      world = _world;
      x = _x;
      y = _y;
    };
    void update();
    void think();
};

class World {
  private:
    const int MIN_SUGAR = 1;
    const int MAX_SUGAR = 5;
    const int width, height;
    vector<int> cells;
    vector<Bug*> bugs;
  public:
    World(int _width, int _height, int bugCount)
      : width(_width), height(_height) {
      // We need enough cells for all the bugs (max 1 bug per cell)
      assert(bugCount < (width * height));

      // Put some sugar in the cells
      cells = vector<int>(width * height, 0);
      for (int i = 0; i < cells.size(); i++) {
        cells[i] = random(MIN_SUGAR, MAX_SUGAR);
      }

      // Add the bugs in random locations
      for (int i = 0; i < bugCount; i++) {
        int x, y;

        do {
          x = random(0, width);
          y = random(0, height);
        } while (occupied(x, y));

        bugs.push_back(new Bug(this, x, y));
      }
    }

    void update();
    bool occupied(int x, int y);
    int getSugar(int x, int y);
    bool inBounds(int x, int y);
    int eat(int x, int y);
    void print();
};

int World::eat(int x, int y) {
  if (!inBounds(x, y)) {
    // Nothing to eat in the wastes
    return 0;
  }

  int i = y * width + x;

  int sugar = cells[i];
  cells[i] = 0;

  return sugar;
}

bool World::inBounds(int x, int y) {
  return x >= 0 && x < width && y >= 0 && y < height;
}

bool World::occupied(int x, int y) {
  if (!inBounds(x, y)) {
    // Only death outside
    return false;
  }

  for (auto bug : bugs) {
    if (bug->x == x && bug->y == y) {
      return true;
    }
  }

  return false;
}

int World::getSugar(int x, int y) {
  if (!inBounds(x, y)) {
    // Only wastes lie beyond
    return false;
  }

  return cells[y * width + x];
}

void World::update() {
  // Update the bugs in a random order

  vector<Bug*> shuffledBugs = vector<Bug*>(bugs);
  random_shuffle(shuffledBugs.begin(), shuffledBugs.end());

  // Let everyone figure out there next move without the world changing
  for (auto bug : shuffledBugs) {
    bug->think();
  }

  // Let everyone affect the world
  for (auto bug : shuffledBugs) {
    bug->update();
  }
}

void World::print() {
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int i = y * height + x;

      if (occupied(x, y)) {
        printf(" ");
      } else {
        if (cells[i] <= 9) {
          printf("%d", cells[i]);
        } else {
          printf("^");
        }
      }
    }

    printf("\n");
  }
}

Glance *Bug::look(int dx, int dy) {
  int targetX = x + dx;
  int targetY = y + dy;

  return new Glance {
    .x = targetX,
    .y = targetY,
    .sugar = world->getSugar(targetX, targetY),
    .occupied = world->occupied(targetX, targetY),
  };
}

void Bug::think() {
  Glance *up = look(0, -1);
  Glance *down = look(0, 1);
  Glance *left = look(-1, 0);
  Glance *right = look(1, 0);

  vector<Glance*> sight = { up, down, left, right };

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

void Bug::update() {
  x = nextX;
  y = nextY;

  world->eat(x, y);
}

int main(int argc, char **argv) {
  // Seed PRNG
  srand(time(NULL));

  World world(16, 16, 10);

  while (true) {
    world.update();

    printf("\e[2J");
    world.print();

    usleep(1000 * 1000);
  }
}
