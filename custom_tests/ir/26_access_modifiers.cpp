// Access control: private state behind public methods, protected members used by a
// derived class, private inheritance used internally, and struct/class defaults.
class Account {
    int balance;
    int fee() { return 1; }

public:
    Account(int initial) { balance = initial; }
    void deposit(int amount) { balance = balance + amount - fee(); }
    int get() { return balance; }
};

class Shape {
protected:
    int sides;
    int scaled(int k) { return sides * k; }

public:
    Shape() { sides = 0; }
};

class Square : public Shape {
public:
    Square() { sides = 4; }
    int perimeterFactor(int k) { return scaled(k); }
};

class Stack : private Shape {
    int items[4];

public:
    int count;
    Stack() { count = 0; sides = 9; }
    void push(int v) { items[count] = v; count = count + 1; }
    int top() { return items[count - 1] + sides; }
};

struct Pair {
    int first;

private:
    int hidden;

public:
    void setHidden(int v) { hidden = v; }
    int sum() { return first + hidden; }
};

int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    Account account = Account(n);
    account.deposit(10);
    Square square = Square();
    Stack stack = Stack();
    stack.push(n);
    stack.push(n * 2);
    Pair pair;
    pair.first = 3;
    pair.setHidden(n);
    return account.get() + square.perimeterFactor(n) + stack.top() + stack.count + pair.sum();
}
