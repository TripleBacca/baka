struct In {
    int a;
    double d;
};

struct P {
    char c;          // followed by padding
    int x;
    In in;
    int arr[3];      // short array: unrolled
    long big[6];     // longer array: compared in a loop
    int* p;
};

class Animal {
public:
    int legs;
    Animal() { legs = 4; }
    int sound() { return 0; }
    bool same(Animal& other) { return *this == other; }
};

class Dog : public Animal {
public:
    int tricks;
    Dog() { tricks = 0; }
    int sound() { return 1; }
};

class Cat : public Animal {
public:
    int lives;
    Cat() { lives = 9; }
};

P fill(int n) {
    P v;
    v.c = 'x';
    v.x = n;
    v.in.a = 1;
    v.in.d = 0.0;
    for (int i = 0; i < 3; i++) v.arr[i] = i;
    for (int i = 0; i < 6; i++) v.big[i] = i * 10;
    v.p = 0;
    return v;
}

int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    P a = fill(n);
    P b = fill(n);
    char* raw = (char*)&b;
    raw[1] = 77;                                  // garbage in padding after c
    b.in.d = -0.0;                                // IEEE: -0.0 == 0.0
    int bits = 0;
    if (a == b) bits = bits | 1;
    P& ref = b;
    b.big[5] = n;                                 // differs unless n == 50
    if (a != ref) bits = bits | 2;
    b.big[5] = 50;
    double zero = n - n;
    b.in.d = zero / zero;                         // NaN never equals itself
    if (!(b == b)) bits = bits | 4;
    if (fill(n) == a) bits = bits | 8;            // rvalue operand

    Dog d1;
    Dog d2;
    Cat c;
    d2.tricks = n;
    Animal& r1 = d1;
    Animal& r2 = d2;
    Animal& r3 = c;
    if (r1 == r2) bits = bits | 16;               // same dynamic class, tricks equal only for n == 0
    if (r1 != r3) bits = bits | 32;               // Dog vs Cat through Animal&
    if (d1 == r2) bits = bits | 64;               // exact Dog vs reference
    Animal plain;
    if (plain != d1) bits = bits | 128;           // base object vs derived object
    Cat cats[2];
    int i = 0;
    if (d1 != cats[i++] && i == 1) bits = bits | 256;  // unrelated: false, operand still evaluated
    Dog* heap = new(heap);
    heap->tricks = n;
    if (*heap == d2 && r2.same(*heap)) bits = bits | 512;
    delete(heap);
    return bits;
}
