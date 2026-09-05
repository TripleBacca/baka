
void example_function(int param_a, int param_b) {
  int array[5];

  array[0] = param_a;
  array[1] = param_b;
  array[2] = array[0] + array[1];

  int calculated = ((array[0] * (array[1] + 2)) - (array[2] / 3));

  {
    int inner_scoped_var = 100;
    calculated = calculated + inner_scoped_var;
  }

  ;
  ;
}

struct Config {
  int id;
  int values[3];
};

int main() {
  Config cfg = {1, {10, 20, 30}};
  example_function(cfg.id, cfg.values[0]);
  return 0;
}
