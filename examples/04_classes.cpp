// Classes: access modifiers, inheritance, and methods that are virtual by default.
// A method declared `= 0` is pure, which makes the class abstract. Heap objects use
// `new(pointer, constructor arguments)` and `delete(pointer)`.

class Shape {
private:
    int created;
public:
    Shape() { created = 1; }
    int area() = 0;                        // pure: every shape must define it
    void show() { printf("area %d\n", area()); }
};

class Rect : public Shape {
protected:
    int w;
    int h;
public:
    Rect() { w = 0; h = 0; }
    Rect(int a, int b) { w = a; h = b; }
    int area() { return w * h; }           // overrides Shape::area, no keyword needed
};

class Square : public Rect {
public:
    Square(int s) { w = s; h = s; }        // protected fields are visible here
};

int main() {
    Rect r = Rect(2, 3);
    r.show();

    Square* sq = new(sq, 4);               // heap object
    Shape* s = sq;
    s->show();                             // calls Square's area through a Shape*
    delete(s);
    return 0;
}
