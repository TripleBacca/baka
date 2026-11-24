struct In {
    int a;
    double d;
};

struct S {
    int x;
    In in;
    int arr[3];
};

class Animal {
public:
    int legs;
};

class Dog : public Animal {
public:
    int tricks;
};

class Cat : public Animal {
public:
    int lives;
};

S make() {
    S s;
    return s;
}

int main() {
    S a;
    S b;
    S& r = b;
    const S& c = a;
    Dog d;
    Cat k;
    Animal& ra = d;
    int same = (a == b) + (a != r) + (r == c) + (make() == a);
    int classes = (d == ra) + (ra != d) + (d == k) + (k != ra);
    return same + classes;
}
