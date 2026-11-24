class Shape {
public:
    int area() = 0;
    int twice() { return 2 * area(); }
};

class Square : public Shape {
public:
    int side;
    Square() { side = 3; }
    int area() { return side * side; }
};

class Unit : public Square {
public:
    int area(int scale) { return scale; }
};

int measure(Shape& shape) {
    return shape.area();
}

int main() {
    Square square;
    Unit unit;
    Shape* shapes[2] = {&square, &unit};
    Square* heap = new(heap);
    Shape* base = heap;
    delete(base);
    return measure(square) + shapes[1]->twice() + unit.area(4);
}
