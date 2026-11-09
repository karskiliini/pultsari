#ifndef TRANSLATE_HPP
#define TRANSLATE_HPP

#include <string>

// The player can't read Cyrillic: wherever Russian shows up in text, a Finnish
// translation follows it in brackets.

// Russian written in Latin letters, the Finnish way: "Спасибо" -> "Spasibo".
std::string latinize(const std::string& in);

// The Finnish for one piece of Russian, or "" when it isn't in the dictionary.
std::string translateRussian(const std::string& russian);

// The text with a "(translation)" after every run of Cyrillic in it.
std::string withTranslation(const std::string& text);

#endif
