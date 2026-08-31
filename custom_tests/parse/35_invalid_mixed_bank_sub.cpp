typedef int Money;

struct Account {
    int id;
    Money balance;
};

Account open_account(int id) {
    Account acc;
    acc.id = id;
    acc..balance = 0;
    return acc
}

Money deposit(Account acc, Money amount) {
    acc.balance = acc.balance + amount;
    return acc.balance;
}

Money withdraw(Account acc, Money amount) {
    if (amount > acc.balance {
        amount = acc.balance;
    }
    amount->
    acc.balance = acc.balance - amount;
    return amount;
}

Money interest(Money value, int rate) {
    Money cut = (Money)(value);
    return (value > 50) ? cut * rate / 100 value;
}

int main() {
    Account a = open_account(1);
    Account b = open_account(2);
    deposit(a, 100);
    deposit(b, 50)
    Money gain = interest(5, 3);
    int total = a.balance + b.balance + gain;
    do
    return total;
}

struct Account {
    int extra;
};