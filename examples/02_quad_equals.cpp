// Quad equals: `a ==== b` is true when two objects have the same *runtime* class,
// whatever their declared types; `a !=== b` is the opposite. It works on objects,
// references and pointers to classes.

class Animal {
public:
    int legs;
    Animal() { legs = 4; }
};

class Dog : public Animal { };
class Cat : public Animal { };

void describe(Animal& a, Animal& b) {
    if (a ==== b) {
        printf("same kind of animal\n");
    } else {
        printf("different kinds of animal\n");
    }
}

int main() {
    Dog rex;
    Dog fido;
    Cat tom;

    describe(rex, fido);   // both Dogs, though seen as Animal&
    describe(rex, tom);    // a Dog and a Cat

    Animal* pet = &tom;
    printf("pet is a dog? %d\n", pet ==== rex);
    pet = &fido;
    printf("pet is a dog now? %d\n", pet ==== rex);
    printf("rex !=== tom: %d\n", rex !=== tom);

    Animal copy = rex;     // copying into an Animal makes a plain Animal
    printf("copy is still a dog? %d\n", copy ==== rex);
    return 0;
}
