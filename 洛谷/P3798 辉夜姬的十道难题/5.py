def judge(a: list[int]) -> bool:
    for i in range(1, len(a)):
        if a[i - 1] == a[i]:
            return True


def calc(a: list[int]) -> int:
    result = 0
    while judge(a):
        b = []
        i = 0
        while i < len(a):
            if i < len(a) - 1 and a[i] == a[i + 1]:
                b.append(a[i] * 2)
                result += a[i] * 2
                i += 2
            else:
                b.append(a[i])
                i += 1
        a = b
    return result


vals = [1, 2, 4, 8, 16, 32, 64, 128, 256]
G = dict()


def dfs(x: int, a: list[int]):
    G[(x, a.count(2), a.count(4))] = max(
        G.get((x, a.count(2), a.count(4)), 0), calc([x] + a))
    if len(a) == 6:
        return
    dfs(x, a + [2])
    dfs(x, a + [4])


for i in vals:
    dfs(i, [])

print(G)

F = dict()
for i in range(1, 234):
    _i = (i & (~i + 1)) * 2
    # if _i == 1 or _i > 16:
    #    continue
    _F = F.copy()
    for j in range(234):
        for l in range(7):
            if j + l > 233:
                break
            for m in range(7):
                if l + m > 6:
                    break
                for k in range(67):
                    if k + m > 66:
                        break
                    F[(j + l, k + m)] = max(F.get((j + l, k + m), 0),
                                            _F.get((j, k), 0) + G[(_i, l, m)])
    print(i)
    # print(F)
print(F)
