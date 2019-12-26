# sugarscratch2

dependencies:
you'll need to install [this library](https://github.com/nlohmann/json) (nlohmann:json).

to run the code:

`$ make`  
`$ ./bin/sugarscratch`

to change the duration of the simualation, the size of the grid, or the resolution of the 'bug appetite' measurement, edit the initialisation in `main`:

```
int main(int argc, char **argv) {
  ...
  World world(50, 50, 100); <--- the world is 50x50, the number of bugs is 100
  int res_x = 6 <--- changing the number here changes the resolution of the output (x-dimension)
  int res_y = 8 <--- (y-dimension)
  vector<json> bugSteps;

  ...

  while (world.clk<100) { <--- running for 100 timesteps right now
  ...

	  world.calculateAppetites(res_x, res_y, appetites);
  ...  
}
```

keep the world square!
