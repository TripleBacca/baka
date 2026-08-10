#include <bits/stdc++.h>

using namespace std;

class TrieNode
{
public:
	map<char, int> children;
};


class Trie
{
public:

	vector<TrieNode> nodes;

	Trie(vector<string> &words)
	{
		nodes.emplace_back(); // root node

		for (string &word : words)
		{
			insert(word);
		}
	}

	void insert(const string &word)
	{
		int currentNode = 0; // start at root

		for (char c : word)
		{
			if (!nodes[currentNode].children.contains(c))
			{
				nodes.emplace_back();
				nodes[currentNode].children[c] = nodes.size() - 1;
			}
			currentNode = nodes[currentNode].children[c];
		}
	}
};

int main()
{
	vector<string> words = {
		"bool", "break", "case", "char", "class", "const", "continue", "delete", "do",
		"double", "else", "enum", "false", "float", "for", "goto", "if", "int", "long",
		"mutable", "namespace", "new", "return", "short", "signed", "sizeof", "static",
		"struct", "switch", "this", "true", "typedef", "union", "unsigned", "using",
		"void", "while",
		"~", "-", "--", "++", "*", "/", "%", "&", "|", "^", "<<", ">>", "&&", "||",
		"==", "!=", "<", ">", "<=", ">=", "+", "!", "=", "+=", "-=", "*=", "/=", "%=",
		"&=", "|=", "^=", "<<=", ">>=", ":", "::", ",", "?", ".", "->", "...",
		"(", ")", "[", "]", "{", "}", ";",
		"nullptr"
	};
	Trie trie(words);

	return 0;
}