typedef int Status;

enum Level {
    LOW,
    MEDIUM,
    HIGH
};

struct Vec2 {
    int x;
    int y;
};

union Variant {
    int number;
    char letter;
};

class Widget {
    int id;

    Widget() {
        id = 0;
    }

    int get_id() {
        return id;
    }
};

int global_count = 0;
static int static_bucket = 1;
const int max_widgets = 64;

Status global_status = MEDIUM;

Vec2 make_vec(int a, int b) {
    Vec2 v;
    v.x = a;
    v.y = b;
    return v;
}

Level bump(Level l) {
    return l;
}

int use_widget() {
    Widget w;
    return w.get_id();
}

int main() {
    Status s = global_status;
    Vec2 origin = make_vec(max_widgets, static_bucket);
    Variant v;
    v.number = 7;
    int total = origin.x + origin.y + s + v.number + use_widget() + global_count;
    return total;
}