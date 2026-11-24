class Animal {
public:
    int legs;
    Animal() { legs = 4; }
    int sound() { return 0; }
    bool sameAs(Animal& other) { return *this ==== other; }
    bool sameAsPointer(Animal* other) { return this ==== other; }
};

class Dog : public Animal {
public:
    int sound() { return 1; }
};

class Cat : public Animal {
public:
    int sound() { return 2; }
};

class Kennel {
public:
    Dog resident;
};

int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    Dog rex;
    Dog fido;
    Cat tom;
    Animal plain;
    Animal& a = rex;
    Animal& b = fido;
    Animal& c = tom;
    Animal* p = &tom;
    if (n) {
        p = &rex;
    }
    Animal* q = &fido;
    Animal sliced = rex;
    Animal other;
    Dog pack[2];
    Kennel k;
    int bits = 0;
    if (rex ==== fido) bits = bits | 1;              // folded: same exact class
    if (rex !=== tom) bits = bits | 2;               // folded: different exact classes
    if (a ==== b) bits = bits | 4;                   // same dynamic class via base references
    if (a !=== c) bits = bits | 8;                   // same static type, different vptrs
    if (*p ==== rex) bits = bits | 16;               // depends on n
    if (sliced ==== plain && sliced !=== rex) bits = bits | 32;
    if (other ==== plain && pack[0] ==== pack[1] && k.resident ==== rex) bits = bits | 64;
    if (a.sameAs(b) && !a.sameAs(c)) bits = bits | 128;
    if ((p ==== q) == (n != 0)) bits = bits | 256;   // pointer operands compare the pointees
    if (p ==== *q && q ==== rex) bits = bits | 512;  // mixed pointer and object; depends on n
    if (a.sameAsPointer(q) && b.sameAsPointer(&rex)) bits = bits | 1024;
    if (rex ==== fido == true && a ==== b != c ==== b) bits = bits | 2048;  // binds tighter than == and !=
    int i = 0;
    if (pack[i++] ==== rex && i == 1) bits = bits | 4096;                    // folded, operand still runs
    return bits;
}
