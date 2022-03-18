#ifdef ONLINE_JUDGE
#define NDEBUG
#endif

#include <cassert>
#include <cstdlib>

#include <initializer_list>
#include <iostream>
#include <queue>
#include <tuple>
#include <utility>
#include <vector>

class World {
private:
  int n, hm, hr, hc, hx, hy;
  std::vector<std::vector<int>> coldness;               // 冷冻度
  std::vector<std::vector<std::vector<bool>>> hasBlock; // 是否有冰砖
  int blockCnt;                                         // 当前冰砖数

  static constexpr int delta1[8][2] = {{-1, 0}, {-1, -1}, {0, -1},
                                       {1, -1}, {1, 0},   {1, 1},
                                       {0, 1},  {-1, 1}}; // 平面，八连通
  static constexpr int delta2[6][3] = {{-1, 0, 0}, {1, 0, 0},
                                       {0, -1, 0}, {0, 1, 0},
                                       {0, 0, -1}, {0, 0, 1}}; // 立体

  bool inRange(int r, int c, int h) const {
    return r >= 0 && r < n && c >= 0 && c < n && h >= 0 && h <= hm;
  }

  // 判断 (r, c, h) 周围是否有可依附的冰砖
  bool hasBlockAround(int r, int c, int h) const {
    for (int i = 0; i < 6; ++i) {
      const int _r = r + delta2[i][0];
      const int _c = c + delta2[i][1];
      const int _h = h + delta2[i][2];
      if (inRange(_r, _c, _h) && hasBlock[_r][_c][_h])
        return true;
    }
    return false;
  }

  // 当前已放置冰砖个数
  int countInFieldBlocks() const {
    int cnt = 0;
    for (int i = 0; i < n; ++i) {
      for (int j = 0; j < n; ++j) {
        for (int k = 0; k <= hm; ++k) {
          if (hasBlock[i][j][k])
            ++cnt;
        }
      }
    }
    return cnt;
  }

  // 移除所有悬空的冰砖
  void removeDanglingIceBlocks() {
    std::vector<std::vector<std::vector<bool>>> hasBlockNew(
        n, std::vector<std::vector<bool>>(n, std::vector<bool>(hm + 1, false)));
    std::queue<std::tuple<int, int, int>> queue;
    for (int i = 0; i < n; ++i) {
      for (int j = 0; j < n; ++j) {
        if (hasBlock[i][j][0]) {
          queue.emplace(i, j, 0);
          hasBlock[i][j][0] = false;
        }
      }
    }
    while (!queue.empty()) {
      const auto [r, c, h] = queue.front();
      queue.pop();
      hasBlockNew[r][c][h] = true;
      for (int i = 0; i < 6; ++i) {
        const int _r = r + delta2[i][0];
        const int _c = c + delta2[i][1];
        const int _h = h + delta2[i][2];
        if (inRange(_r, _c, _h) && hasBlock[_r][_c][_h]) {
          queue.emplace(_r, _c, _h);
          hasBlock[_r][_c][_h] = false;
        }
      }
    }
    hasBlock = std::move(hasBlockNew);
  }

  // 计算屋顶所在高度
  int getRoofHeight() const {
    for (int i = hm - 1; i >= 0; --i) {
      for (int j = hr; j < hr + hx; ++j) {
        if (hasBlock[j][hc][i] || hasBlock[j][hc + hy - 1][i])
          return i + 1;
      }
      for (int j = hc; j < hc + hy; ++j) {
        if (hasBlock[hr][j][i] || hasBlock[hr + hx - 1][j][i])
          return i + 1;
      }
    }
    return 0;
  }

  // [r1, r2] 行，[c1, c2] 列，高度 [h1, h2]  冰砖数
  int countBlocksInRange(int r1, int r2, int c1, int c2, int h1, int h2) {
    int cnt = 0;
    assert(0 <= r1 && r1 <= r2 && r2 < n);
    assert(0 <= c1 && c1 <= c2 && c2 < n);
    assert(0 <= h1 && h1 <= h2 && h2 <= hm);
    for (int i = r1; i <= r2; ++i) {
      for (int j = c1; j <= c2; ++j) {
        for (int k = h1; k <= h2; ++k) {
          if (hasBlock[i][j][k])
            ++cnt;
        }
      }
    }
    return cnt;
  }

  enum class DoorState {
    NO_DOOR,
    HAS_DOOR,
    HAS_CORNER_DOOR,
    HAS_HALF_DOOR,
    HAS_HALF_CORNER_DOOR
  };

  struct DoorInfo {
    DoorState doorState;
    int cornersNeedFix;
  };

  // 计算门在对应位置时，需要修复的四角块数
  // @param r,c 门的位置
  // @param h1,h2 要修复的高度范围
  int countNeedFixCornerForDoor(int r, int c, int h1, int h2) {
    int cnt = 0;
    for (int i : {hr, hr + hx - 1}) {
      for (int j : {hc, hc + hy - 1}) {
        assert(r != i || c != j);
        if (std::abs(r - i) + std::abs(c - j) == 1)
          cnt += h2 - h1 + 1 - countBlocksInRange(i, i, j, j, h1, h2);
      }
    }
    return cnt;
  }

  // 修复四角
  void fixCornerForDoor(int r, int c, int h1, int h2) {
    for (int i : {hr, hr + hx - 1}) {
      for (int j : {hc, hc + hy - 1}) {
        assert(r != i || c != j);
        if (std::abs(r - i) + std::abs(c - j) == 1) {
          for (int k = h1; k <= h2; ++k)
            hasBlock[i][j][k] = true;
        }
      }
    }
  }

  // 计算门的状态
  // 如果需要修复四角，顺便修复
  DoorInfo getDoorState() {
    { // 完整门
      for (int i = hr + 2; i <= hr + hx - 3; ++i) {
        for (int j : {hc, hc + hy - 1}) {
          if (!hasBlock[i][j][0] && !hasBlock[i][j][1])
            return {DoorState::HAS_DOOR, 0};
        }
      }
      for (int i = hc + 2; i <= hc + hy - 3; ++i) {
        for (int j : {hr, hr + hx - 1}) {
          if (!hasBlock[j][i][0] && !hasBlock[j][i][1])
            return {DoorState::HAS_DOOR, 0};
        }
      }
    }
    { // 完整角落门
      int r, c, cnt = -1;
      const auto check = [this, &r, &c, &cnt](int x, int y) {
        const int t = countNeedFixCornerForDoor(x, y, 0, 1);
        if (cnt == -1 || t < cnt) {
          cnt = t;
          r = x;
          c = y;
        }
      };
      for (int i : {hr + 1, hr + hx - 2}) {
        for (int j : {hc, hc + hy - 1}) {
          if (!hasBlock[i][j][0] && !hasBlock[i][j][1])
            check(i, j);
        }
      }
      for (int i : {hr, hr + hx - 1}) {
        for (int j : {hc + 1, hc + hy - 2}) {
          if (!hasBlock[i][j][0] && !hasBlock[i][j][1])
            check(i, j);
        }
      }
      if (cnt != -1) {
        fixCornerForDoor(r, c, 0, 2);
        return {DoorState::HAS_CORNER_DOOR, cnt};
      }
    }
    { // 不完整门
      for (int i = hr + 2; i <= hr + hx - 3; ++i) {
        for (int j : {hc, hc + hy - 1}) {
          if (!hasBlock[i][j][0] || !hasBlock[i][j][1])
            return {DoorState::HAS_HALF_DOOR, 0};
        }
      }
      for (int i = hc + 2; i <= hc + hy - 3; ++i) {
        for (int j : {hr, hr + hx - 1}) {
          if (!hasBlock[j][i][0] || !hasBlock[j][i][1])
            return {DoorState::HAS_HALF_DOOR, 0};
        }
      }
    }
    { // 不完整角落门
      int r, c, cnt = -1;
      const auto check = [this, &r, &c, &cnt](int x, int y) {
        const int t = countNeedFixCornerForDoor(
            x, y, !hasBlock[x][y][0] ? 0 : 1, !hasBlock[x][y][0] ? 0 : 1);
        if (cnt == -1 || t < cnt) {
          cnt = t;
          r = x;
          c = y;
        }
      };
      for (int i : {hr + 1, hr + hx - 2}) {
        for (int j : {hc, hc + hy - 1}) {
          if (!hasBlock[i][j][0] || !hasBlock[i][j][1])
            check(i, j);
        }
      }
      for (int i : {hr, hr + hx - 1}) {
        for (int j : {hc + 1, hc + hy - 2}) {
          if (!hasBlock[i][j][0] || !hasBlock[i][j][1])
            check(i, j);
        }
      }
      if (cnt != -1 && cnt <= 1) {
        fixCornerForDoor(r, c, !hasBlock[r][c][0] ? 0 : 1,
                         !hasBlock[r][c][0] ? 0 : 1);
        return {DoorState::HAS_HALF_CORNER_DOOR, cnt};
      }
    }
    return {DoorState::NO_DOOR, 0};
  }

public:
  World(int n, int hm, int hr, int hc, int hx, int hy)
      : n(n), hm(hm), hr(hr), hc(hc), hx(hx), hy(hy),
        coldness(n, std::vector<int>(n, 0)),
        hasBlock(n, std::vector<std::vector<bool>>(
                        n, std::vector<bool>(hm + 1, false))),
        blockCnt(0) {
    assert(n >= 4 && n <= 16);
    assert(hm >= 5 && hm <= 20);
    assert(hr >= 0 && hr < n);
    assert(hc >= 0 && hc < n);
    assert(hx > 0 && hr + hx - 1 < n);
    assert(hy > 0 && hc + hy - 1 < n);
  }

  // ICE_BARRAGE R C D S
  void shot(int r, int c, int d, int s) {
    assert(r >= 0 && r < n);
    assert(c >= 0 && c < n);
    assert(s >= 0 && s <= n); // 题面锅了
    assert(d >= 0 && d <= 7);
    int cnt = 0;
    for (int i = 0; i <= s; ++i) {
      if (!inRange(r, c, 0))
        break;
      if (hasBlock[r][c][0])
        break;
      if (coldness[r][c] < 4) {
        ++coldness[r][c];
        ++cnt;
      }
      r += delta1[d][0];
      c += delta1[d][1];
    }
    std::cout << "CIRNO FREEZED " << cnt << " BLOCK(S)\n";
  }

  // MAKE_ICE_BLOCK
  void makeIceBlock() {
    const int oldBlockCnt = blockCnt;
    for (int i = 0; i < n; ++i) {
      for (int j = 0; j < n; ++j) {
        if (coldness[i][j] == 4) {
          coldness[i][j] = 0;
          ++blockCnt;
        }
      }
    }
    std::cout << "CIRNO MADE " << blockCnt - oldBlockCnt
              << " ICE BLOCK(S),NOW SHE HAS " << blockCnt << " ICE BLOCK(S)\n";
  }

  // PUT_ICE_BLOCK R C H
  void putIceBlock(int r, int c, int h) {
    assert(r >= 0 && r < n);
    assert(c >= 0 && c < n);
    assert(h >= 0 && h < hm);
    if (!blockCnt) {
      std::cout << "CIRNO HAS NO ICE_BLOCK\n";
      return;
    }
    if (hasBlock[r][c][h] || (h > 0 && !hasBlockAround(r, c, h))) {
      std::cout << "BAKA CIRNO,CAN'T PUT HERE\n";
      return;
    }
    --blockCnt;
    hasBlock[r][c][h] = true;
    if (h == 0)
      coldness[r][c] = 0;
    if (r < hr || r > hr + hx - 1 || c < hc || c > hc + hy - 1) {
      std::cout << "CIRNO MISSED THE PLACE\n";
      return;
    }
    if (r >= hr + 1 && r <= hr + hx - 2 && c >= hc + 1 && c <= hc + hy - 2) {
      std::cout << "CIRNO PUT AN ICE_BLOCK INSIDE THE HOUSE\n";
      return;
    }
    std::cout << "CIRNO SUCCESSFULLY PUT AN ICE_BLOCK,NOW SHE HAS " << blockCnt
              << " ICE_BLOCK(S)\n";
  }

  // REMOVE_ICE_BLOCK R C H
  void removeIceBlock(int r, int c, int h) {
    assert(r >= 0 && r < n);
    assert(c >= 0 && c < n);
    assert(h >= 0 && h <= hm);
    if (!hasBlock[r][c][h]) {
      std::cout << "BAKA CIRNO,THERE IS NO ICE_BLOCK\n";
      return;
    }
    hasBlock[r][c][h] = false;
    const int oldCnt = countInFieldBlocks();
    removeDanglingIceBlocks();
    ++blockCnt;
    const int cnt = countInFieldBlocks();
    if (cnt < oldCnt)
      std::cout << "CIRNO REMOVED AN ICE_BLOCK,AND " << oldCnt - cnt
                << " BLOCK(S) ARE BROKEN\n";
    else
      std::cout << "CIRNO REMOVED AN ICE_BLOCK\n";
  }

  // MAKE_ROOF
  void makeRoof() {
    const int h = getRoofHeight(); // 屋顶高度

    { // 冰砖是否足够
      const int c = countBlocksInRange(hr, hr + hx - 1, hc, hc + hy - 1, h, h);
      if (hx * hy - c > blockCnt) {
        std::cout << "SORRY CIRNO,NOT ENOUGH ICE_BLOCK(S) TO MAKE ROOF\n";
        return;
      }
    }
    { // 空间是否足够
      const int validSpace = (hx - 2) * (hy - 2) * h;
      if (h < 2 || validSpace < 2) {
        std::cout << "SORRY CIRNO,HOUSE IS TOO SMALL\n";
        return;
      }
    }
    { // 建屋顶
      for (int i = hr; i < hr + hx; ++i) {
        for (int j = hc; j < hc + hy; ++j) {
          if (!hasBlock[i][j][h]) {
            --blockCnt;
            hasBlock[i][j][h] = true;
          }
        }
      }
    }
    bool perfect = true;
    { // 移除错误放置的冰砖
      const int c = countInFieldBlocks();
      int k1 = 0, k2 = 0;
      for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
          for (int k = 0; k <= hm; ++k) {
            if (!hasBlock[i][j][k])
              continue;
            if (i > hr && i < hr + hx - 1 && j > hc && j < hc + hy - 1 &&
                k < h) {
              // 内部
              ++k1;
              hasBlock[i][j][k] = false;
            }
            if (i < hr || i > hr + hx - 1 || j < hc || j > hc + hy - 1 ||
                k > h) {
              // 外部
              ++k2;
              hasBlock[i][j][k] = false;
            }
          }
        }
      }
      std::cout << k1 << " ICE_BLOCK(S) INSIDE THE HOUSE NEED TO BE REMOVED\n";
      std::cout << k2 << " ICE_BLOCK(S) OUTSIDE THE HOUSE NEED TO BE REMOVED\n";
      if (k1 > 0 || k2 > 0) {
        perfect = false;
        removeDanglingIceBlocks();
      }
      if (!hasBlock[hr][hc][h]) {
        std::cout << "SORRY CIRNO,HOUSE IS BROKEN WHEN REMOVING BLOCKS\n";
        return;
      }
      blockCnt += c - countInFieldBlocks();
    }
    bool wallNeedFix = false, hasDoor = false;
    { // 修补墙壁残缺 & 开门
      int c = 0;
      const auto [doorState, k] = getDoorState();
      c += k;
      for (int i = hr + 1; i <= hr + hx - 2; ++i) {
        for (int j : {hc, hc + hy - 1}) {
          for (int k = 0; k < h; ++k) {
            if (!hasBlock[i][j][k])
              ++c;
          }
        }
      }
      for (int i : {hr, hr + hx - 1}) {
        for (int j = hc + 1; j <= hc + hy - 2; ++j) {
          for (int k = 0; k < h; ++k) {
            if (!hasBlock[i][j][k])
              ++c;
          }
        }
      }
      switch (doorState) {
      case DoorState::HAS_DOOR:
      case DoorState::HAS_CORNER_DOOR:
        c -= 2;
        break;
      case DoorState::HAS_HALF_DOOR:
      case DoorState::HAS_HALF_CORNER_DOOR:
        --c;
        break;
      case DoorState::NO_DOOR:
        break; // Make GCC happy
      }
      if (c > blockCnt) {
        std::cout << "SORRY CIRNO,NOT ENOUGH ICE_BLOCKS TO FIX THE WALL\n";
        return;
      }
      if (c > 0)
        wallNeedFix = true;
      blockCnt -= c;
      // 题面没有说清楚“开一个门”是否回收冰砖
      // 试验结果：不回收
      if (doorState == DoorState::HAS_DOOR ||
          doorState == DoorState::HAS_CORNER_DOOR)
        hasDoor = true;
      if ((doorState != DoorState::HAS_DOOR &&
           doorState != DoorState::HAS_CORNER_DOOR) ||
          c > 0)
        perfect = false;
      switch (doorState) {
      case DoorState::NO_DOOR:
        blockCnt += 2;
        break;
      case DoorState::HAS_HALF_DOOR:
      case DoorState::HAS_HALF_CORNER_DOOR:
        ++blockCnt;
        break;
      case DoorState::HAS_DOOR:
      case DoorState::HAS_CORNER_DOOR:
        break; // Make GCC happy again
      }
    }
    std::cout << "GOOD JOB CIRNO,SUCCESSFULLY BUILT THE HOUSE\n";
    std::cout << (hasDoor ? "DOOR IS OK\n" : "HOUSE HAS NO DOOR\n");
    std::cout << (wallNeedFix ? "WALL NEED TO BE FIXED\n" : "WALL IS OK\n");
    { // 修复四角
      int c = 0;
      for (auto [i, j] : std::initializer_list<std::pair<int, int>>{
               {hr, hc},
               {hr, hc + hy - 1},
               {hr + hx - 1, hc},
               {hr + hx - 1, hc + hy - 1}}) {
        for (int k = 0; k < h; ++k) {
          if (!hasBlock[i][j][k])
            ++c;
        }
      }
      if (c > 0) {
        std::cout << "CORNER NEED TO BE FIXED\n";
        blockCnt = std::max(0, blockCnt - c);
        perfect = false;
      } else
        std::cout << "CORNER IS OK\n";
    }
    std::cout << "CIRNO FINALLY HAS " << blockCnt << " ICE_BLOCK(S)\n";
    { // 完美判定
      if (perfect) {
        perfect = false;
        for (int i : {hr + (hx - 1) / 2, hr + hx / 2}) {
          for (int j : {hc, hc + hy - 1}) {
            if (!hasBlock[i][j][0])
              perfect = true;
          }
        }
        for (int i : {hr, hr + hx - 1}) {
          for (int j : {hc + (hy - 1) / 2, hc + hy / 2}) {
            if (!hasBlock[i][j][0])
              perfect = true;
          }
        }
        if (perfect)
          std::cout << "CIRNO IS PERFECT!\n";
      }
    }
  }
};

int main() {
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);
  int n, hm, hr, hc, hx, hy;
  std::cin >> n >> hm >> hr >> hc >> hx >> hy;
  World world(n, hm, hr, hc, hx, hy);
  int m;
  std::cin >> m;
  assert(m >= 10 && m <= 1000);
  while (m--) {
    std::string opt;
    std::cin >> opt;
    if (opt == "ICE_BARRAGE") {
      int r, c, d, s;
      std::cin >> r >> c >> d >> s;
      world.shot(r, c, d, s);
      continue;
    }
    if (opt == "MAKE_ICE_BLOCK") {
      world.makeIceBlock();
      continue;
    }
    if (opt == "PUT_ICE_BLOCK") {
      int r, c, h;
      std::cin >> r >> c >> h;
      world.putIceBlock(r, c, h);
      continue;
    }
    if (opt == "REMOVE_ICE_BLOCK") {
      int r, c, h;
      std::cin >> r >> c >> h;
      world.removeIceBlock(r, c, h);
      continue;
    }
    if (opt == "MAKE_ROOF") {
      assert(m == 0);
      world.makeRoof();
      break;
    }
    assert(false);
  }
  return 0;
}