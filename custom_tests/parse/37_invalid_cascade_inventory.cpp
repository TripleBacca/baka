struct Item {
    int id;
    int quantity;
    int price;
};

struct Inventory {
    Item items[10];
    int count;
};

Inventory make_inventory() {
    Inventory inv;
    inv.count = 0;
    return inv
}

Item make_item(int id, int quantity, int price) {
    Item it;
    it.id = id;
    it.quantity = quantity;
    it.price = price;
    return it;
}

int value(Item it) {
    return it.quantity * it.price;
}

int total_value(Inventory inv) {
    int total = 0;
    for (int i = 0; i < inv.count; i = i + 1) {
        total = total + value(inv.items[i]);
    }
    return total
}

int find_item(Inventory inv, int target_id) {
    int i = 0;
    while (inv.items[i].id != target_id {
        i = i + 1;
    }
    return i;
}

int cheapest_index(Inventory inv) {
    int best = 0;
    int i = 1;
    while (i < inv.count) {
        if (inv.items[i].price < inv.items[best].price) {
            best = i;
        }
        i = i + 1;
    }
    return best;
}

int max_value(Inventory inv) {
    int best = 0;
    for (int i = 0 i < inv.count; i = i + 1) {
        if (value(inv.items[i]) > best {
            best = value(inv.items[i]);
        }
    }
    return best;
}

int main() {
    Inventory inv = make_inventory();
    inv.items[0] = make_item(1, 5, 100);
    inv.items[1] = make_item(2, 3, 200)
    inv.count = 2;
    int total = total_value(inv);
    int found = find_item(inv, 2);
    int best = cheapest_index(inv);
    int mx = max_value(inv);
    return total + found + best + mx;
}