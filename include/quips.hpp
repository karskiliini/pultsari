#ifndef QUIPS_HPP
#define QUIPS_HPP

#include <string>
#include <vector>

// one line of the list, picked at random so it does not repeat every time
std::string pickQuip(const std::vector<std::string>& lines);

extern const std::vector<std::string> BEER_LINES;
// index = the bottle's fill after the sip: 5 ... 0
extern const std::string VODKA_SIP_LINES[6];

extern const std::string HINT_HANDS_FULL;
extern const std::string HINT_AK_HANDS;
extern const std::string HINT_PISTOL_BOTTLE;
extern const std::string KICK_BONA_MSG;

#endif
