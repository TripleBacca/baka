int counter;
int seeds[4] = {3, 1, 4, 1};
double scale = 2.5;
float ratio = 0.75f;
long huge = 1099511627776L;
char letters[6] = "abcde";
const char* label = "global label";
int* alias = &seeds[2];
int (*ops[2])(int v);

class Config {
    int id;
    double factor;
    const char* name;
};

Config config = {7, 1.5, "cfg"};
Config configs[2] = {{1, 0.5, "one"}, {2, 0.25, "two"}};
const int limit = 5;

int twice(int v) {
    return v * 2;
}

int thrice(int v) {
    return v * 3;
}

int next() {
    static int state = 10;
    static double fraction = 0.5;
    state += 3;
    fraction *= 2;
    counter++;
    return state + (int)fraction;
}

int shadow(int v) {
    int counter = v * 2;
    {
        int counter = v * 3;
        v += counter;
    }
    return v + counter;
}

int main(int n) {
    ops[0] = twice;
    ops[1] = thrice;
    int a = next();
    int b = next();
    int c = next();
    *alias += n;
    seeds[0] = ops[1](seeds[0]) + ops[0](seeds[1]);
    letters[2] = 'X';
    printf("%d %d %d %d %f %f %ld\n", a, b, c, counter, scale, (double)ratio, huge);
    printf("%s %s %c %d %d %d\n", label, letters, label[7], seeds[0], seeds[2], limit);
    printf("%d %f %s %d %f %s\n", config.id, config.factor, config.name, configs[1].id, configs[1].factor,
           configs[0].name);
    return a + b + c + counter + shadow(5) + (int)(huge >> 40) + seeds[0] + seeds[2] + configs[1].id;
}
