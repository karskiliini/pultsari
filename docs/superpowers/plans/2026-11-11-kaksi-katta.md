# Kaksi kättä Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the counter inventory with two hands + one pocket + belt + back. Make the
vodka bottle a 6-sip life meter shown as ASCII arms beside the map. Sleeves tear with damage,
and coats restore health.

**Architecture:** A new slot model (`include/hands.hpp`, `src/hands.cpp`) owns all carrying rules
behind a small API (`count/give/take/freeHands`). Every `++inv.X` / `--inv.X` / `inv.X > 0` site
migrates to that API. Arm pictures are composed in C++ (`src/armart.cpp`) from a sleeve layer
(by health) and a hand layer (by held item). PrinterWeb sends the finished rows. The page draws
them in the left panel and a new right panel.

**Tech Stack:** C++14 (g++, `make`), the repo's own `TEST()`/`CHECK()` framework in
`tests/unit_tests.cpp`, the vanilla JS page `web/game.js` + `web/index.html`, and the Python game
test `tests/test_game.py`.

**Spec:** `docs/superpowers/specs/2026-11-11-kaksi-katta-design.md`. The art drafts are in
`docs/superpowers/specs/2026-11-11-kaksi-katta-art.md`.

## Global Constraints

- All work goes on `pultsari2` (merge this worktree branch into it; never to master).
- Browser game runs only on port 3000 (`npm start`). If 3000 is taken, stop the old server first.
- Slots: left hand 1, right hand 1, pocket 1 small item, belt = pistol only (needs `hasBelt`), back = AK only.
- Vodka bottle = 6 sips. Each `q` sip: fill −1, `updatepromilles(+1)`, `feedHunger(1)`. Beer and Jaguar are drunk in one go.
- Rounds live in the gun: Makarov 8, AK 30. One spare magazine (`Lipas`) in the pocket = one full reload.
- Pistol needs one free hand to fire. AK needs both hands free.
- Propusk takes no slot and never falls out.
- Money and kopecks stay as `stats`, not items.
- Art is plain ASCII, each picture exactly 14 columns × 22 rows.
- Sleeve stage by health: ≥8 → 0, 5–7 → 1, 2–4 → 2, 0–1 → 3 (bloody).
- Game text is Finnish. The copy lines given in the spec are used verbatim.
- Don't make the inventory any broader than this (spec non-goal).

## Review Focus

- Hands full while walking onto an item: the item must stay on the map (`interact` returns false), not vanish. Tested in Task 2.
- Corpse loot / shop with no free slot: nothing is lost silently. The shop refuses and keeps the money; loot drops to the ground. Tested in Task 2.
- Throwing or drinking the last vodka sip in the same turn as a bottle thief steals: no underflow or crash on an empty hand. Tested in Task 3 (`take` on empty is false).
- The mirrored picture (bottle in the right hand) must stay exactly 14 columns wide. Tested in Task 5.
- The pocket drops while the pocket is empty: no item is spawned, and no delayed message. Tested in Task 4.

---

### Task 1: Slot model

**Files:**
- Create: `include/hands.hpp`, `src/hands.cpp`
- Test: `tests/unit_tests.cpp` (new section `// ---- hands ----`)

**Interfaces:**
- Produces:
```cpp
enum class Thing : uint8_t { None, Kalja, Lonkka, Vodka, Tyhja, Lenkki, Kala, Omppo, Bansku,
    Ketjut, Veitsi, Kivi, Pamppu, Bootsit, Granata, Pistooli, Rynkky, Lipas };
constexpr uint8_t VODKA_SIPS = 6, MAKAROV_ROUNDS = 8, AK_ROUNDS = 30;
struct Held { Thing what = Thing::None; uint8_t fill = 0; uint8_t rounds = 0; std::string pissSeller;
              bool empty() const { return what == Thing::None; } };
bool isSmall(Thing t);            // Lipas, Granata, Veitsi
bool isDrink(Thing t);            // Kalja, Lonkka, Vodka
bool isWeapon(Thing t);           // Pistooli, Rynkky
const char* thingName(Thing t);   // "VODKA", "TYHJÄ PULLO", "MAKAROV", ...
// in struct Inventory (include/inventory.hpp, replacing the counters):
Held left, right, pocket, belt, back; bool hasBelt = false; uint32_t propusk = 0;
uint32_t count(Thing t) const;            // across all five slots
bool give(Thing t, const std::string& pissSeller = "");  // false = no room, nothing changed
bool take(Thing t);                       // removes one, false if none
Held* find(Thing t);                      // first slot holding t (hands before pocket/belt/back)
int freeHands() const;
bool give(const Held& h);                 // keeps fill/rounds (used for dropping/picking a used bottle)
```
- `give` placement: Pistooli → belt if `hasBelt && belt.empty()`, else a hand. Rynkky → back if empty,
  else false. Lipas → pocket first, else a hand. Drinks → left first, then right. Weapons → right
  first, then left. Others → any free hand. A small thing goes to the pocket when the hands are full
  and the pocket is empty. A new Vodka gets `fill = VODKA_SIPS`, a new Pistooli `rounds = MAKAROV_ROUNDS`,
  a new Rynkky `rounds = AK_ROUNDS`.
- Default inventory: `left = Kalja`, `right = Lenkki` (the current start kit).

- [ ] **Step 1: Write failing tests** `hands_give_fills_left_then_right_then_refuses` (3× Kalja → true, true, false; `count(Kalja)==2`), `hands_vodka_starts_with_six_sips`, `hands_pistol_goes_to_belt_only_with_belt`, `hands_second_ak_is_refused`, `hands_lipas_prefers_pocket`, `hands_small_item_overflows_to_pocket`, `hands_take_on_empty_is_false`, `hands_default_kit_is_beer_and_sausage`.
- [ ] **Step 2: Run `make tests/unit_tests && ./tests/unit_tests hands_`**. Expected: does not compile (no `hands.hpp`).
- [ ] **Step 3: Implement** `hands.hpp/.cpp` and rewrite `struct Inventory` with the members above. Keep `pissVodka` out: the seller lives in `Held::pissSeller`. Rewrite `stealable(Inventory&, int)` as `Thing stealableThing(int i)` over Kalja, Lonkka, Vodka, Lenkki, Kala, Omppo, Bansku. `stealableName` stays.
- [ ] **Step 4: Run the hands tests.** Expected: the 8 tests PASS (the rest of the build may still be broken; Task 2 fixes it).
- [ ] **Step 5: Commit** `feat: kahden käden paikkamalli`.

### Task 2: Migrate every inventory call site

**Files:**
- Modify: `src/player.cpp`, `src/item.cpp`, `src/building.cpp`, `src/persons.cpp`, `src/level.cpp`, `src/aagun.cpp`, `src/pultsari.cpp`, `src/printer.cpp`, `src/printerterm.cpp`, `src/printerweb.cpp`, `tests/unit_tests.cpp`
- Find the sites with: `grep -rnE "(inv|inventory)\.(kalja|lonkka|lenkki|ketjut|veitset|kivet|pamput|bootsit|kalat|omppo|bansku|pistooli|ammukset|granata|vodka|rynkky|pullot|pissVodka)\b" src tests`

**Interfaces:**
- Consumes: Task 1 API.
- Produces: `Thing throwNumToThing(uint32_t n)` in `hands.hpp`. It maps the old `decrementInventory`/`Item::createItem` numbers 1–14 (1 Kalja, 2 Lonkka, 3 Lenkki, 4 Ketjut, 5 Veitsi, 6 Kivi, 7 Pamppu, 8 Bootsit, 9 Kala, 10 Omppo, 11 Bansku, 13 Vodka, 14 Granata). An empty bottle thrown is `Thing::Tyhja` → spawns an empty-bottle item (reuse the thrown `Kalja` item look if there is no bottle item).

Rules applied while migrating:
- `++inv.X` → `inv.give(Thing::X)`. Where it is a pickup (`Item::interact`), return the result so a full-handed player leaves the item on the map. Message: `"Kädet ovat täynnä. Juo tai heitä jotain pois."`
- Shops (`building.cpp` buy menus): check room first. If there is none, show the same message and do not charge.
- Multi-item loot (`player.cpp` robbers' car ~825, item.cpp finds, building.cpp break-ins): each piece that `give` refuses becomes a map item next to the player (`level->addItem(Item::createItem(...))`).
- `inv.ammukset += n` → `inv.give(Thing::Lipas)` once per find (one magazine regardless of n).
- `--inv.pullot` / deposit hatch (`building.cpp:739`): returns every `Tyhja` in the hands at once. Each pays the existing 20 kopecks.
- Bottle thieves (`level.cpp:1443-1495`): steal hand items via `stealableThing`. "Bottles" = the `Tyhja`/drink items in the hands.
- The empty vodka/beer bottle stays in the hand as `Thing::Tyhja` (replaces `++inv.pullot`).
- `Divari::getEnterMsg` "nothing to buy": check `count()` of Bootsit/Pamppu/Ketjut/Veitsi.
- Overlay lists (`overlayItems`, printerterm side list, printerweb `inv`) list slots: `VASEN`, `OIKEA`, `TASKU`, `VYÖ` (only with `hasBelt`), `SELKÄ`, each with `thingName` (vodka adds ` n/6`, guns add rounds).

- [ ] **Step 1: Update the existing tests** that read counters (shops, gun shop, bottles 1425–1495, etc.) to the new API with the same intent, e.g. `CHECK(w.player.inventory.count(Thing::Pistooli) == guns + 1)`. Add `pickup_with_full_hands_leaves_item_on_map`, `shop_refuses_and_keeps_money_when_hands_full`, `robber_loot_that_does_not_fit_drops_to_ground`, `deposit_hatch_takes_all_empty_bottles_in_hands`.
- [ ] **Step 2: Migrate the call sites** per the rules above until `make` builds with no warnings from the touched files.
- [ ] **Step 3: Run `make test`.** Expected: all unit tests and `tests/test_game.py` PASS.
- [ ] **Step 4: Commit** `refactor: inventaario käsien kautta`.

### Task 3: Drinking, eating, throwing from hands

**Files:**
- Modify: `src/player.cpp` (`Player::drink`, `Player::eat`, `decrementInventory`, `throwItem`), `src/pultsari.cpp` (`useInventoryItem`), `src/input.cpp` (hand choice)
- Create: `include/quips.hpp`, `src/quips.cpp`
- Test: `tests/unit_tests.cpp`

**Interfaces:**
- Produces: `std::string pickQuip(const std::vector<std::string>& lines)` and the lists `BEER_LINES`, `VODKA_SIP_LINES[6]` (index = fill after the sip: 5…0), `HINT_HANDS_FULL`, `HINT_AK_HANDS`, `HINT_PISTOL_BOTTLE`. Copy them verbatim from the spec's "Vinkit ja kommentit" section. `BEER_LINES[0]` = `"Olutpullo meni kierteellä alas."`
- Produces: `Held* Player::drinkTarget()`. It picks a vodka with `fill > 0` first, then the left-hand drink, then the right-hand drink.
- Produces: `InputNS::Input::getHand()` → `'v'`/`'o'` (left/right) or `0` for cancel. It is used only when both hands hold something.

- [ ] **Step 1: Write failing tests** `vodka_sip_lowers_fill_by_one_and_adds_promille` (fill 6→5, promilles +1), `six_sips_leave_empty_bottle_in_hand` (after 6 `drink()`: `left.what == Thing::Tyhja`), `beer_is_drunk_in_one_go` (Kalja → Tyhja, promilles +2, message is one of BEER_LINES), `piss_vodka_reveals_on_first_sip` (Held with pissSeller → damage 2, bottle becomes Tyhja), `drink_with_no_drink_says_nothing_to_drink`, `eat_uses_food_in_hand`.
- [ ] **Step 2: Run** `./tests/unit_tests sip` etc. Expected: FAIL.
- [ ] **Step 3: Implement.** `q` → `Player::drink` uses `drinkTarget()`. Remove the "Kumpaakos horaat" 1/2 prompt. `e` eats hand food. `t`/space throws from the only filled hand, or asks `getHand()` when both are filled. The inventory overlay "use" row maps to the same functions.
- [ ] **Step 4: Run `make test`.** Expected: PASS.
- [ ] **Step 5: Commit** `feat: votkan huikat ja juonti kädestä`.

### Task 4: Weapons, belt, magazines, pocket drop, coats

**Files:**
- Modify: `src/player.cpp` (`shootAt`, `updateCrawling`, corpse interaction ~878), `src/pultsari.cpp` (pistol raise ~1289, AK aim ~1275, Esc ~1361), `src/vehicle.cpp:160`, `src/building.cpp` (Divari sells a used coat), `src/item.cpp` + `include/item.hpp` (new `ETakki`, class `Takki`, glyph `T`; belt item `EVyo`, class `Vyo`, glyph `=`), `src/pultsari.cpp` level spawns, `include/player.hpp`
- Test: `tests/unit_tests.cpp`

**Interfaces:**
- Produces on `Player`: `bool drawPistol()` (moves the pistol belt→free hand; false + `HINT_PISTOL_BOTTLE` if no free hand), `void holsterPistol()`, `bool shoulderAk()` (false + `HINT_AK_HANDS` unless `freeHands()==2`), `bool reload()` (uses the pocket `Lipas`), `void maybeDropPocket()`, `uint32_t pocketNoticeTurns = 0`, `void wearCoat(uint32_t hp)` and `bool stripCorpseCoat(Person* corpse)`.
- `Person` gets `bool coatTaken = false`.
- Constants: `POCKET_DROP_PERCENT = 25`, `POCKET_NOTICE_TURNS = 4`, `USED_COAT_HP = 3`, used coat price 30 ₽.

Rules:
- `shootAt` uses the held gun's `rounds` (`Ei ammuksia!` at 0, auto-`reload()` if a Lipas is in the pocket).
- The soldier corpse drops `Vyo` with 30 % (in `dropItem` for `sotilas`). Picking it sets `hasBelt = true` and moves a hand pistol onto it.
- A fall (`updateCrawling` start, bus fall) calls `maybeDropPocket()`: with 25 % the pocket item is placed on the map at the player's coord, `pocketNoticeTurns = 4`. `updateTurn` counts it down and then shows `"Hetkinen... tasku tuntuu kevyeltä!"`.
- `Takki` pickup → `wearCoat(INITIAL_HP)` (capped by `Disease::maxHealth`), consumed. Message `"Uusi takki! Vain vähän verta kauluksessa."`.
- Walking onto a corpse (health 0, `!coatTaken`) asks `"Riisutko takin? (k/e)"`. On `k`: `wearCoat(INITIAL_HP)`, `coatTaken = true`. Every living person with `canSeeTarget(player->coord)` gets `target = player`, `alerted = true` (as persons.cpp:453) and says `"Ruumiinryöstäjä!"` or `"Paskiainen!"`.
- Divari menu gains `Käytetty takki 30₽` → `wearCoat(health + USED_COAT_HP)` capped at INITIAL_HP. The sleeve stage cannot be better than 1 after a used coat (`Player::usedCoat = true`, cleared by a new coat).

- [ ] **Step 1: Write failing tests** `pistol_needs_a_free_hand`, `belt_frees_the_hand_until_drawn`, `ak_needs_both_hands_empty_and_hints`, `makarov_has_eight_rounds_and_reloads_from_pocket`, `fall_can_drop_pocket_item_and_notices_later` (force the roll: expose `maybeDropPocket(bool force)`), `empty_pocket_drop_spawns_nothing`, `coat_restores_full_health`, `stripping_corpse_coat_angers_witnesses`, `divari_used_coat_gives_three_hp`.
- [ ] **Step 2: Run them.** Expected: FAIL.
- [ ] **Step 3: Implement** per the rules.
- [ ] **Step 4: Run `make test`.** Expected: PASS.
- [ ] **Step 5: Commit** `feat: vyö, lippaat, takit ja taskusta putoaminen`.

### Task 5: Arm pictures

**Files:**
- Create: `include/armart.hpp`, `src/armart.cpp`
- Test: `tests/unit_tests.cpp`

**Interfaces:**
- Produces:
```cpp
constexpr int ARM_W = 14, ARM_H = 22;
int sleeveStage(uint32_t health, bool usedCoat);   // 0..3 per Global Constraints; usedCoat → max(stage,1)
std::vector<std::string> armPicture(bool leftArm, const Held& held, int stage);  // ARM_H rows of ARM_W
std::string mirrorRow(const std::string& row);     // reverse + swap / \ ( ) < > [ ] { } ` '
bool armRowBloody(int stage, int row);             // rows the web colours red at stage 3
```
- Layers: rows 0–8 are the sleeve, 4 variants (stage 0–3) drawn for the LEFT arm. The right arm is `mirrorRow`. Rows 9–21 are the hand, with variants for empty, bottle (Vodka/Kalja/Lonkka/Tyhja), pistol, AK, other (fist + `thingName` centred on row 21). Hand art is drawn for its natural side (bottle left, pistol right) and mirrored when held in the other hand.
- Start from the grok drafts in `docs/superpowers/specs/2026-11-11-kaksi-katta-art.md`. LEFT_BOTTLE liquid sits on rows 16–21 (1-based), inside columns 3–8. Redraw the pistol as a recognisable Makarov pointing down-inward. Unify both sleeves into one torn jacket, style from `art/mainos-pullolla.jpg`. Stage 2 shows a bare forearm below a torn edge. Stage 3 has almost no sleeve plus `'` `,` `.` blood specks on the hand.
- The bottle fill: of the 6 liquid rows, the top `6 - fill` are spaces inside the glass. The topmost remaining row uses `~`, the rest `#` (the VODKA label row keeps its letters while it is filled).

- [ ] **Step 1: Write failing tests** `arm_pictures_are_14_by_22` (every variant × stage × side, incl. mirrored), `vodka_fill_three_shows_three_liquid_rows`, `empty_bottle_has_no_liquid`, `sleeve_stage_thresholds` (10→0, 8→0, 7→1, 5→1, 4→2, 2→2, 1→3, 0→3; used coat at 10 → 1), `mirror_swaps_slashes`.
- [ ] **Step 2: Run.** Expected: FAIL.
- [ ] **Step 3: Draw and implement.** Look at every variant in a terminal (`./tests/unit_tests arm_dump` printing all pictures is fine as a throwaway helper, not committed).
- [ ] **Step 4: Run `make test`.** Expected: PASS.
- [ ] **Step 5: Commit** `feat: käsivarsikuvat`.

### Task 6: Show the arms in the browser and the terminal

**Files:**
- Modify: `src/printerweb.cpp` (`print`), `web/index.html`, `web/game.js` (`updateUi`, layout width calc at ~135), `src/printerterm.cpp`, `src/printerncurses.cpp`
- Test: `tests/unit_tests.cpp`, `tests/test_game.py`

**Interfaces:**
- Consumes: Task 5 `armPicture`, `sleeveStage`, `armRowBloody`.
- Produces: in the web frame JSON `"arms":[[22 strings],[22 strings]],"blood":[[rows…],[rows…]]` sent only when the pictures change (keep the last sent strings in `PrinterWeb`), and `"slots":[["TASKU","LIPAS"],["VYÖ","MAKAROV 8"],["SELKÄ","AK 30"]]` replacing `inv`.

- [ ] **Step 1: Write failing test** `web_frame_sends_arms_once_per_change` (two `print()` calls with the same state → `"arms"` appears only in the first; after a sip it appears again). Capture `webSend` the way the existing printerweb tests do.
- [ ] **Step 2: Implement.** Web: `#side` shows `<pre id="armL">` under the slot list. A new `<aside id="sideR"><pre id="armR"></pre></aside>` goes right of `#main`. `pre` uses the map font. Bloody rows are wrapped in `<span class="blood">` (`color: var(--red)`). Hide `#sideR` with `#side` under `max-width: 1000px`, and subtract its width in the canvas width calc. Terminal (`printerterm.cpp`): print the slot list in the left column instead of the counter list. Add the arm pictures as a right column only if the terminal is at least `13 + VIEWPORT_W + 2 + 15` columns wide, otherwise skip them. ncurses: slot list text only.
- [ ] **Step 3: Run `make test`**, then `npm start` (port 3000) and check in the browser: start kit shows beer left / sausage right; buy vodka, `q` six times → the liquid drops a row each time; take damage → sleeves tear; Makarov shows in the right hand.
- [ ] **Step 4: Commit** `feat: kädet näkyvät kartan vierellä`.

### Task 7: Passers-by comment on the drunk

**Files:**
- Modify: `src/mask.cpp` (`Mask::gazeDirection`: remember the stared person), `include/mask.hpp`, `src/player.cpp` (`Player::stagger`)
- Test: `tests/unit_tests.cpp`

**Interfaces:**
- Produces: `const Person* Mask::staredAt = nullptr` (set to `nearest` in `gazeDirection`).

Rules:
- Each turn, if `promilles >= 15` and `staredAt` is a living non-player within 6 cells, with 10 %: `staredAt->say("Sehän katsoo kuin ryssä rahaa.")`. At most once per 40 turns per level.
- In `Player::stagger`, with 15 %: the nearest living person within 8 cells who can see the player says `"Sehän pyörii kuin ryssä Anttilassa."`.

- [ ] **Step 1: Write failing tests** `drunk_stare_gets_a_comment` (srand-fixed loop of 200 turns at 20 ‰ next to one person → that person said the line at least once), `sober_stare_gets_no_comment`, `stagger_comment_from_onlooker`.
- [ ] **Step 2: Run.** Expected: FAIL.
- [ ] **Step 3: Implement.**
- [ ] **Step 4: Run `make test`.** Expected: PASS.
- [ ] **Step 5: Commit** `feat: ohikulkijat kommentoivat juoppoa`.

### Task 8: Shoplifting and the fanatical seller

**Files:**
- Modify: `include/persontype.hpp` (add `myyja`), `include/persons.hpp` + `src/persons.cpp` (class `Myyja`), `src/building.cpp` (`Kauppa::getInteractMsg`, `Kauppa::interact`), `include/building.hpp` (`bool sellerHostile = false` on `Kauppa`), `src/level.cpp` (`nameOf` → "Myyjä"), `src/printer.cpp` (colour for `myyja`)
- Test: `tests/unit_tests.cpp`

**Interfaces:**
- Consumes: Task 1 `give`, Task 3 `HINT_HANDS_FULL`.
- Produces: `enum class SellerWeapon { Nitoja, Veitsi, Pistooli }` and `SellerWeapon rollSellerWeapon()` (75 / 20 / 5 %). `class Myyja : public Person` has a `SellerWeapon weapon` and chases with `target = player`, `alerted = true` every turn until dead or the level changes. Damage per hit: Nitoja 1, Veitsi 2. Pistooli shoots like a militia gun when in line of sight, 2 dmg.
- Constants: `SHOPLIFT_NOTICE_PERCENT = 70`.

Rules:
- Menu lines end with `  v+nro) varasta`. Key `v` then a digit takes that item via `give` without paying. If there is no room → `HINT_HANDS_FULL`, nothing happens.
- With 70 %: the shop's `sellerHostile = true`, a `Myyja` is spawned on the shop door tile and says one of `"VARAS! Seis!"`, `"Minun kauppani! Minun!"`. The message names the weapon, e.g. `"Myyjä ryntää perääsi nitoja kädessä!"`.
- `sellerHostile` shop: `getEnterMsg` returns false with `"Kauppa on kiinni. Myyjä jahtaa sinua jossain."`. If the seller is dead: `"Kauppa on hiljainen. Kukaan ei myy mitään."`.

- [ ] **Step 1: Write failing tests** `shoplift_puts_item_in_hand_without_paying` (force notice off), `shoplift_with_full_hands_takes_nothing`, `noticed_shoplift_spawns_armed_seller_who_chases` (force notice on → a `myyja` Person exists with `alerted`), `seller_weapon_odds` (10 000 rolls: Nitoja 70–80 %, Veitsi 15–25 %, Pistooli 2–8 %), `hostile_shop_is_closed`. Expose the notice roll as a `bool force` parameter or seedable roll.
- [ ] **Step 2: Run.** Expected: FAIL.
- [ ] **Step 3: Implement.**
- [ ] **Step 4: Run `make test`.** Expected: PASS.
- [ ] **Step 5: Commit** `feat: kaupasta varastaminen ja fanaattinen myyjä`.

### Task 9: Merge and run

- [ ] **Step 1:** `git fetch origin pultsari2 && git rebase origin/pultsari2`, then `make test`. Expected: all PASS.
- [ ] **Step 2:** Ask the user before pushing to `pultsari2`.
- [ ] **Step 3:** After the push: in `/Users/marski/work/pultsari` run `git pull --ff-only`, `make`, and restart the port 3000 server.
