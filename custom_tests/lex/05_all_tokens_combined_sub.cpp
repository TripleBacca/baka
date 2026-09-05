
namespace ComprehensiveTest {

typedef unsigned long int ulong_t;

using IntegerType = int;

enum Status { STATUS_OK, STATUS_ERR };

union VariantData {
  int i;
  double d;
  char c;
};

struct Node {
  int val;
  Node *next;
};

class TokenDemonstrator {
private:
  signed short short_val;
  float float_val;
  double double_val;
  char char_val;
  bool bool_val;

public:
  static const int BUFFER_LIMIT = 256;

  TokenDemonstrator()
      : short_val(0), float_val(0.0f), double_val(3.1415), char_val('A'),
        bool_val(true) {}

  TokenDemonstrator *get_instance() { return this; }

  void log_messages(int count, ...) { return; }
};
} // namespace ComprehensiveTest

int main() {
  bool flag_t = true;
  bool flag_f = false;

  int decimal_num = 42;
  double pi_val = 3.14;
  char newline_char = '\n';
  const char *message = "Baka Tokenizer Test String";

  void *null_ptr = nullptr;

  int *dynamic_array = new int[5];
  delete[] dynamic_array;

  int type_size = sizeof(ComprehensiveTest::TokenDemonstrator);

  int a = 15;
  int b = 4;
  int c = 0;

  int tilde_res = ~a;
  int neg_res = -a;
  int plus_res = +a;
  ++a;
  a++;
  --b;
  b--;
  bool not_res = !flag_t;

  c = (a * b) / ((a % b) + 1);
  c = (a & b) | (a ^ b);
  c = (a << 1) >> 1;

  bool cond =
      ((a < b) || (a > b)) && ((a <= b) || (a >= b)) && (a == b) && (a != b);

  c = 10;
  c += 1;
  c -= 1;
  c *= 2;
  c /= 2;
  c %= 5;
  c &= 0xFF;
  c |= 0x01;
  c ^= 0x02;
  c <<= 1;
  c >>= 1;

  ComprehensiveTest::Node node_instance;
  node_instance.val = 100;
  node_instance.next = nullptr;

  ComprehensiveTest::Node *node_ptr = &node_instance;
  int accessed_via_arrow = node_ptr->val;

  int ternary_val = (c > 0) ? c : 0;
  int comma_expr = (a = 1, b = 2, a + b);

  if (cond) {
    c = 1;
  } else {
    c = 2;
  }

  while (c < 10) {
    c += 1;
    if (c == 5) {
      continue;
    }
    if (c == 8) {
      break;
    }
  }

  do {
    c -= 1;
  } while (c > 5);

  for (int i = 0; i < 3; i++) {
    c += i;
  }

  switch (c) {
  case 5:
    c = 10;
    break;
  case 10:
    c = 20;
    break;
  }

  goto exit_point;

exit_point:;
  return 0;
}
