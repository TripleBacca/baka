class Shape {
public:
    int id;
    Shape() { id = 0; printf("Shape:%s ", name()); }
    ~Shape() { printf("~Shape%d:%s ", id, name()); }
    const char* name() { return "shape"; }
    int area() = 0;
    int twice() { return 2 * area(); }
};

class Square : public Shape {
public:
    int side;
    Square(int i, int s) { id = i; side = s; printf("Square:%s ", name()); }
    ~Square() { printf("~Square:%s ", name()); }
    const char* name() { return "square"; }
    int area() { return side * side; }
};

class Rect : public Shape {
public:
    int w;
    int h;
    Rect(int i, int a, int b) { id = i; w = a; h = b; }
    const char* name() { return "rect"; }
    int area() { return w * h; }
};

int total(Shape& s) { return s.twice(); }

int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    Shape* shapes[3];
    Square* sq = new(sq, 1, n + 2);
    Rect* r = new(r, 2, 3, n + 1);
    shapes[0] = sq;
    shapes[1] = r;
    shapes[2] = sq;
    printf("\n");
    int sum = 0;
    for (int i = 0; i < 3; i++) {
        sum += shapes[i]->area();
        printf("%s=%d ", shapes[i]->name(), shapes[i]->area());
    }
    printf("\nsum=%d twice=%d\n", sum, total(*r));
    delete(shapes[0]);
    printf("\n");
    delete(shapes[1]);
    printf("\n");
    return sum;
}
