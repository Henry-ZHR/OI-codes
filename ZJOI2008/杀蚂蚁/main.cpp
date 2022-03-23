#ifdef ONLINE_JUDGE
#define NDEBUG
#endif

#include <cassert>
#include <climits>
#include <cmath>

#include <algorithm>
#include <iostream>
#include <numeric>
#include <utility>
#include <vector>

static_assert(sizeof(int) * CHAR_BIT >= 32);

template <typename T> constexpr T sqr(const T &x) { return x * x; }

struct Coord {
  int x, y;

  constexpr auto operator<=>(const Coord &) const = default;
};

constexpr Coord INVALID_POS = Coord{-1, -1};

struct Frac {
  int nume, deno;

  Frac(int nume = 0, int deno = 1) {
    assert(deno != 0);
    const int d = std::gcd(nume, deno);
    if (d != 0) {
      nume /= d;
      deno /= d;
    }
    if (deno < 0) {
      nume = -nume;
      deno = -deno;
    }
    this->nume = nume;
    this->deno = deno;
  }
  Frac(const Frac &other) : Frac(other.nume, other.deno) {}

  Frac &operator=(const Frac &other) {
    const auto t = other;
    nume = t.nume;
    deno = t.deno;
    return *this;
  }

  Frac operator+(const Frac &other) const {
    return Frac(nume * other.deno + other.nume * deno, deno * other.deno);
  }
  Frac &operator+=(const Frac &other) { return *this = *this + other; }
  Frac operator-(const Frac &other) const {
    return Frac(nume * other.deno - other.nume * deno, deno * other.deno);
  }
  Frac &operator-=(const Frac &other) { return *this = *this - other; }
  Frac operator*(const Frac &other) const {
    return Frac(nume * other.nume, deno * other.deno);
  }
  Frac &operator*=(const Frac &other) { return *this = *this * other; }
  Frac operator/(const Frac &other) const {
    return Frac(nume * other.deno, deno * other.nume);
  }
  Frac &operator/=(const Frac &other) { return *this = *this / other; }

  auto operator<=>(const Frac &other) const {
    assert(deno > 0 && other.deno > 0);
    return nume * other.deno <=> other.nume * deno;
  }
};

struct Line {
  int a, b, c;

  static Line getLineOfTwoPoints(Coord a, Coord b) {
    return Line{a.y - b.y, b.x - a.x, a.x * b.y - b.x * a.y};
  }

  Line getPerpendicularLineThroughPoint(Coord p) const {
    return Line{b, -a, a * p.y - b * p.x};
  }

  Frac getSqrOfDis(Coord p) const {
    return Frac(sqr(a * p.x + b * p.y + c), sqr(a) + sqr(b));
  }
};

// 距离的平方
int getSqrOfDis(Coord a, Coord b) { return sqr(a.x - b.x) + sqr(a.y - b.y); }

struct Segment {
  Coord a, b;

  Frac getSqrOfMinDis(Coord p) const {
    const Line l = Line::getLineOfTwoPoints(a, b);
    const Line l1 = l.getPerpendicularLineThroughPoint(a),
               l2 = l.getPerpendicularLineThroughPoint(b);
    assert(l1.a == l2.a && l1.b == l2.b);
    if ((l1.a * p.x + l1.b * p.y + l1.c) * (l2.a * p.x + l2.b * p.y + l2.c) >=
        0)
      return std::min(getSqrOfDis(p, a), getSqrOfDis(p, b));
    return l.getSqrOfDis(p);
  }
};

class Ant;
class Tower;
class Map;

class Ant {
private:
  Map *map;
  int age, level;
  int initHp;
  int hp;
  Coord lastPos, curPos;
  bool cake;

public:
  Ant(Map *, Coord, int);
  bool isAt(Coord) const;
  void move();
  void getCake();
  void loseHp(int);
  bool isDead() const;
  void incAge();
  bool hasCake() const;
  Coord getCoord() const;

  void output() const;
};

class Map {
private:
  struct Tower {
    Coord pos;
  };

  int n, m, damage, range, antCnt;
  std::vector<std::vector<int>> info;
  std::vector<Ant> ants;
  std::vector<Tower> towers;

  bool hasAntAt(Coord);

  void generateAnt();
  Ant *getTargetForTower(const Tower &);
  void singleAttack(const Tower &, Coord);
  void attack();
  void loseInfo();

public:
  Map(int, int, int, int);

  void addTower(int, int);

  bool isInRange(Coord) const;
  bool hasAntOrTowerAt(Coord) const;
  int getInfoAt(Coord) const;
  void leaveInfo(Coord, int);
  bool tick();

  void output() const;
};

Ant::Ant(Map *map, Coord pos, int level)
    : map(map), age(0), level(level), initHp(4 * std::pow(1.1, level)),
      lastPos(INVALID_POS), curPos(pos), cake(false) {
  hp = initHp;
}

bool Ant::isAt(Coord pos) const { return curPos == pos; }

void Ant::move() {
  static constexpr int delta[4][2] = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};

  static const auto getNewCoord = [](Coord coord, int d) {
    assert(d >= 0 && d <= 3);
    return Coord{coord.x + delta[d][0], coord.y + delta[d][1]};
  };

  map->leaveInfo(curPos, cake ? 5 : 2);
  std::vector<int> validDirections;
  for (int i = 0; i < 4; ++i) {
    const Coord pos = getNewCoord(curPos, i);
    if (map->isInRange(pos) && !map->hasAntOrTowerAt(pos) && pos != lastPos)
      validDirections.push_back(i);
  }
  if (validDirections.empty()) {
    lastPos = curPos;
    return;
  }
  int maxInfo = 0;
  for (const auto &d : validDirections)
    maxInfo = std::max(maxInfo, map->getInfoAt(getNewCoord(curPos, d)));
  int direction = -1;
  for (const auto &d : validDirections) {
    if (map->getInfoAt(getNewCoord(curPos, d)) == maxInfo) {
      direction = d;
      break;
    }
  }
  assert(direction != -1);
  if (age % 5 == 4) {
    direction = (direction + 3) % 4;
    while (std::find(validDirections.begin(), validDirections.end(),
                     direction) == validDirections.end())
      direction = (direction + 3) % 4;
    // auto p = getNewCoord(curPos, direction);
    // while (!map->isInRange(p) || map->hasAntOrTowerAt(p) || p == lastPos) {
    //   direction = (direction + 3) % 4;
    //   p = getNewCoord(curPos, direction);
    // }
  }
  lastPos = curPos;
  curPos = getNewCoord(curPos, direction);
}

void Ant::getCake() {
  assert(!cake);
  cake = true;
  hp = std::min(hp + initHp / 2, initHp);
}

void Ant::loseHp(int v) { hp -= v; }

bool Ant::isDead() const { return hp < 0; }

void Ant::incAge() { ++age; }

bool Ant::hasCake() const { return cake; }

Coord Ant::getCoord() const { return curPos; }

void Ant::output() const {
  assert(!isDead());
  std::cout << age << ' ' << level << ' ' << hp << ' ' << curPos.x << ' '
            << curPos.y << '\n';
}

Map::Map(int n, int m, int damage, int range)
    : n(n), m(m), damage(damage), range(range), antCnt(0),
      info(n + 1, std::vector<int>(m + 1, 0)) {}

bool Map::hasAntAt(Coord pos) {
  for (const Ant &ant : ants) {
    if (ant.isAt(pos))
      return true;
  }
  return false;
}

void Map::generateAnt() {
  if (ants.size() >= 6 || hasAntAt({0, 0}))
    return;
  ants.emplace_back(this, Coord{0, 0}, antCnt / 6 + 1);
  ++antCnt;
}

Ant *Map::getTargetForTower(const Tower &tower) {
  Ant *target = nullptr;
  for (Ant &ant : ants) {
    if (getSqrOfDis(ant.getCoord(), tower.pos) > range * range)
      continue;
    if (ant.hasCake())
      return &ant;
    if (target == nullptr) {
      target = &ant;
      continue;
    }
    if (getSqrOfDis(ant.getCoord(), tower.pos) <
        getSqrOfDis(target->getCoord(), tower.pos))
      target = &ant;
  }
  return target;
}

void Map::singleAttack(const Tower &tower, Coord pos) {
  Segment l{tower.pos, pos};
  for (Ant &ant : ants) {
    if (l.getSqrOfMinDis(ant.getCoord()) <= Frac(1, 4))
      ant.loseHp(damage);
  }
}

void Map::attack() {
  std::vector<Ant *> targets;
  for (const auto &tower : towers)
    targets.push_back(getTargetForTower(tower));
  for (int i = 0, k = towers.size(); i < k; ++i) {
    if (targets[i] != nullptr)
      singleAttack(towers[i], targets[i]->getCoord());
  }
  for (auto iter = ants.begin(); iter != ants.end();) {
    if (iter->isDead())
      iter = ants.erase(iter);
    else
      ++iter;
  }
}

void Map::loseInfo() {
  for (int i = 0; i <= n; ++i) {
    for (int j = 0; j <= m; ++j) {
      if (info[i][j] > 0)
        --info[i][j];
    }
  }
}

void Map::addTower(int x, int y) {
  assert(isInRange(Coord{x, y}));
  towers.push_back({Coord{x, y}});
}

bool Map::isInRange(Coord coord) const {
  return coord.x >= 0 && coord.x <= n && coord.y >= 0 && coord.y <= m;
}

bool Map::hasAntOrTowerAt(Coord coord) const {
  assert(isInRange(coord));
  for (const auto &ant : ants) {
    if (ant.isAt(coord))
      return true;
  }
  for (const auto &tower : towers) {
    if (tower.pos == coord)
      return true;
  }
  return false;
}

int Map::getInfoAt(Coord coord) const {
  assert(isInRange(coord));
  return info[coord.x][coord.y];
}

void Map::leaveInfo(Coord coord, int v) {
  assert(isInRange(coord));
  info[coord.x][coord.y] += v;
}

// @return 游戏是否结束
bool Map::tick() {
  generateAnt();
  for (Ant &ant : ants)
    ant.move();
  bool cakeNotTaken = true;
  for (Ant &ant : ants) {
    if (ant.hasCake()) {
      cakeNotTaken = false;
      break;
    }
  }
  if (cakeNotTaken) {
    for (Ant &ant : ants) {
      if (ant.isAt(Coord{n, m})) {
        for (const auto &a : ants)
          assert(!a.hasCake());
        ant.getCake();
      }
    }
  }
  attack();
  for (const Ant &ant : ants) {
    if (ant.hasCake() && ant.isAt(Coord{0, 0}))
      return true;
  }
  loseInfo();
  for (Ant &ant : ants)
    ant.incAge();
  return false;
}

void Map::output() const {
  std::cout << ants.size() << '\n';
  for (const Ant &ant : ants)
    ant.output();
}

int main() {
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);
  int n, m, s, d, r;
  std::cin >> n >> m >> s >> d >> r;
  Map map(n, m, d, r);
  while (s--) {
    int x, y;
    std::cin >> x >> y;
    map.addTower(x, y);
  }
  int t;
  std::cin >> t;
  for (int i = 1; i <= t; ++i) {
    if (map.tick()) {
      std::cout << "Game over after " << i << " seconds\n";
      map.output();
      return 0;
    }
  }
  std::cout << "The game is going on\n";
  map.output();
  return 0;
}