# sugarscratch2

to run the code:

`$ make`  
`$ ./bin/sugarscratch`

to change the duration of the simualation, the size of the grid, or the resolution of the 'bug appetite' measurement, edit the initialisation in `main`:

```
int main(int argc, char **argv) {
  ...
  World world(50, 50, 100); <--- the world is 50x50, the number of bugs is 100
  vector<json> bugSteps;

  ...

  while (world.clk<100) { <--- running for 100 timesteps right now
  ...

	  world.calculateAppetites(6, appetites); <--- changing the number here changes the resolution of the output (e.g., use 6 if a 6x6 output, 2 if 2x2, etc etc)
  ...  
}
```

keep the world square!
