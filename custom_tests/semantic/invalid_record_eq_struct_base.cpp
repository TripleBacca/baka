struct SB {
    int x;
};

struct SD : SB {
    int y;
};

int main() {
    SD d;
    SB& b = d;
    return d == b;
}
