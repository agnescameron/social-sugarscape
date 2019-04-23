# sugarscratch2

to run the code:

`$ make`  
`$ ./bin/sugarscratch`

to change the duration of the simualation, or the size of the grid, edit the initialisation in `main`:

```
int main(int argc, char **argv) {
  ...
  World world(50, 50, 100); <--- the world is 50x50, the number of bugs is 100
  vector<json> bugSteps;

  ...

  while (world.clk<100) { <--- running for 100 timesteps right now
  ...
  }
```

keep the world square!
