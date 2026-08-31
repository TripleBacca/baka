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
    acc.balance = acc.balance + amount
    return acc.balance;
}

Money withdraw(Account acc, Money amount) {
    if (amount > acc.balance {
        amount = acc.balance;
    }
    acc.balance = acc.balance - amount;
    return amount;
}

Money interest(Money value, int rate) {
    return (value > 50) ? value * rate / 100 value
}

Money clamp(Money value, Money lo, Money hi) {
    if (value < lo {
        return lo
    }
    return hi;
}

int main() {
    Account a = open_account(1);
    Account b = open_account(2);
    deposit(a, 100);
    deposit(b, 50)
    Money taken = withdraw(a, 30);
    Money gain = interest(taken, 5);
    int total = a.balance + b.balance + taken + gain;
    while (total < 0) {
        break
    }
    goto settle
    return total;
}