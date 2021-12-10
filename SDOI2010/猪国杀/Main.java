import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;
import java.util.Scanner;

class CardsHeap {
    private final LinkedList<Character> cards;

    public CardsHeap(List<Character> cards) {
        this.cards = new LinkedList<>(cards);
    }

    public char getCard() {
        char card = cards.getFirst();
        if (cards.size() > 1) {
            cards.removeFirst();
        }
        return card;
    }
}

class Global {
    private Global() {
    }

    public static CardsHeap cardsHeap;

    public static ArrayList<Pig> pigs = new ArrayList<>();

    private static void outputCards() {
        for (Pig pig : pigs) {
            if (pig.isDead())
                System.out.println("DEAD");
            else {
                ArrayList<String> cards = new ArrayList<>();
                for (char card : pig.cards)
                    cards.add(String.valueOf(card));
                System.out.println(String.join(" ", cards));
            }
        }
    }

    public static void checkGameState() {
        for (Pig pig : pigs) {
            if (pig.publicType == 'M' && pig.isDead()) {
                System.out.println("FP");
                outputCards();
                System.exit(0);
            }
        }
        boolean hasFPig = false;
        for (Pig pig : pigs) {
            if (pig instanceof FPig && !pig.isDead()) {
                hasFPig = true;
                break;
            }
        }
        if (!hasFPig) {
            System.out.println("MP");
            outputCards();
            System.exit(0);
        }
    }
}

abstract class Pig {
    public static final int MAX_HP = 4;

    protected int id; // 编号（1..n）
    protected int hp = MAX_HP; // 血量

    // 公开的身份（'U'未知，'M'主，'Z'忠，'F'反，'f'类反）
    protected char publicType;

    public LinkedList<Character> cards; // 手牌

    protected boolean hasLiannu; // 装备

    protected Pig prev, next; // 按顺序上一只和下一只

    public Pig(int id, List<Character> cards) {
        this.id = id;
        this.cards = new LinkedList<>(cards);
    }

    public void setPrevAndNext(Pig prev, Pig next) {
        this.prev = prev;
        this.next = next;
    }

    protected boolean isSame(Pig other) {
        return this.id == other.id;
    }

    protected Pig getNextAlive() {
        for (Pig pig = next; !pig.isSame(this); pig = pig.next) {
            if (pig.isDead())
                continue;
            return pig;
        }
        return null;
    }

    protected boolean isDead() {
        assert hp >= 0;
        return hp == 0;
    }

    protected void decreaseHp(Pig source) {
        --this.hp;
        if (isDead()) {
            if (cards.removeFirstOccurrence('P')) {
                hp = 1;
            }
        }
    }

    // 被无懈可击掉了返回 true，否则 false
    protected boolean askWuxie(Pig target) {
        if (handleWuxieQuery(this, target))
            return !askWuxie(null);
        for (Pig pig = next; !pig.isSame(this); pig = pig.next) {
            if (pig.isDead())
                continue;
            if (pig.handleWuxieQuery(this, target))
                return !pig.askWuxie(null);
        }
        return false;
    }

    protected abstract boolean handleWuxieQuery(Pig source, Pig target);

    protected boolean useTao() {
        if (this.hp < MAX_HP) {
            ++this.hp;
            return true;
        }
        return false;
    }

    protected abstract boolean useSha();

    protected abstract boolean useJuedou();

    protected void useNanzhu() {
        for (Pig pig = next; !pig.isSame(this); pig = pig.next) {
            if (pig.isDead())
                continue;
            if (!askWuxie(pig))
                pig.handleNanzhu(this);
        }
    }

    protected void useWanjian() {
        for (Pig pig = next; !pig.isSame(this); pig = pig.next) {
            if (pig.isDead())
                continue;
            if (!askWuxie(pig))
                pig.handleWanjian(this);
        }
    }

    protected void handleSha(Pig source) {
        if (!cards.removeFirstOccurrence('D')) {
            decreaseHp(source);
        }
    }

    protected void handleNanzhu(Pig source) {
        if (!cards.removeFirstOccurrence('K')) {
            decreaseHp(source);
        }
    }

    protected void handleWanjian(Pig source) {
        if (!cards.removeFirstOccurrence('D')) {
            decreaseHp(source);
        }
    }

    protected abstract void handleJuedou(Pig source);

    public void work() {
        if (isDead())
            return;
        cards.addLast(Global.cardsHeap.getCard());
        cards.addLast(Global.cardsHeap.getCard());
        boolean usedKill = false;
        while (!isDead()) {
            boolean used = false;
            for (int index = 0; index < cards.size(); ++index) {
                switch (cards.get(index)) {
                    case 'P':
                        if (useTao()) {
                            cards.remove(index);
                            --index;
                        }
                        break;
                    case 'K':
                        if ((!usedKill || hasLiannu)) {
                            cards.remove(index);
                            --index;
                            if (!useSha()) {
                                ++index;
                                cards.add(index, 'K');
                            } else {
                                used = true;
                                usedKill = true;
                            }
                        }
                        break;
                    case 'D':
                        break;
                    case 'F':
                        cards.remove(index);
                        --index;
                        if (!useJuedou()) {
                            ++index;
                            cards.add(index, 'F');
                        } else {
                            used = true;
                        }
                        break;
                    case 'N':
                        cards.remove(index);
                        --index;
                        useNanzhu();
                        used = true;
                        break;
                    case 'W':
                        cards.remove(index);
                        --index;
                        useWanjian();
                        used = true;
                        break;
                    case 'J':
                        break;
                    case 'Z':
                        cards.remove(index);
                        --index;
                        hasLiannu = true;
                        used = true;
                        break;
                }
                if (used)
                    break;
            }
            if (!used)
                break;
        }
    }
}

// 主猪
class MPig extends Pig {
    MPig(int id, List<Character> cards) {
        super(id, cards);
        this.publicType = 'M';
    }

    @Override
    protected void decreaseHp(Pig source) {
        super.decreaseHp(source);
        if (isDead())
            Global.checkGameState();
    }

    @Override
    protected boolean handleWuxieQuery(Pig source, Pig target) {
        if (target != null) {
            if (target.publicType == 'M' || target.publicType == 'Z')
                return cards.removeFirstOccurrence('J');
        } else {
            if (source.publicType == 'F')
                return cards.removeFirstOccurrence('J');
        }
        return false;
    }

    @Override
    protected boolean useSha() {
        Pig nextPig = getNextAlive();
        if (nextPig != null && (nextPig.publicType == 'F' || nextPig.publicType == 'f')) {
            nextPig.handleSha(this);
            return true;
        }
        return false;
    }

    @Override
    protected boolean useJuedou() {
        for (Pig pig = next; !pig.isSame(this); pig = pig.next) {
            if (pig.isDead())
                continue;
            if (pig.publicType == 'F' || pig.publicType == 'f') {
                if (!askWuxie(pig))
                    pig.handleJuedou(this);
                return true;
            }
        }
        return false;
    }

    @Override
    protected void handleNanzhu(Pig source) {
        if (!cards.removeFirstOccurrence('K')) {
            decreaseHp(source);
            if (source.publicType == 'U') {
                source.publicType = 'f';
            }
        }
    }

    @Override
    protected void handleWanjian(Pig source) {
        if (!cards.removeFirstOccurrence('D')) {
            decreaseHp(source);
            if (source.publicType == 'U') {
                source.publicType = 'f';
            }
        }
    }

    @Override
    protected void handleJuedou(Pig source) {
        if (cards.removeFirstOccurrence('K')) {
            source.handleJuedou(this);
        } else {
            decreaseHp(source);
        }
    }
}

// 忠猪
class ZPig extends Pig {
    ZPig(int id, List<Character> cards) {
        super(id, cards);
        this.publicType = 'U';
    }

    @Override
    protected void decreaseHp(Pig source) {
        super.decreaseHp(source);
        if (isDead() && source.publicType == 'M') {
            source.cards.clear();
            source.hasLiannu = false;
        }
    }

    @Override
    protected boolean handleWuxieQuery(Pig source, Pig target) {
        if (target != null) {
            if (target.publicType == 'M' || target.publicType == 'Z') {
                if (cards.removeFirstOccurrence('J')) {
                    publicType = 'Z';
                    return true;
                }
            }
        } else {
            if (source.publicType == 'F') {
                if (cards.removeFirstOccurrence('J')) {
                    publicType = 'Z';
                    return true;
                }
            }
        }
        return false;
    }

    @Override
    protected boolean useSha() {
        Pig nextPig = getNextAlive();
        if (nextPig != null && nextPig.publicType == 'F') {
            nextPig.handleSha(this);
            publicType = 'Z';
            return true;
        }
        return false;
    }

    @Override
    protected boolean useJuedou() {
        for (Pig pig = next; !pig.isSame(this); pig = pig.next) {
            if (pig.isDead())
                continue;
            if (pig.publicType == 'F') {
                publicType = 'Z';
                if (!askWuxie(pig))
                    pig.handleJuedou(this);
                return true;
            }
        }
        return false;
    }

    @Override
    protected void handleJuedou(Pig source) {
        if (source.publicType == 'M') {
            decreaseHp(source);
            return;
        }
        if (cards.removeFirstOccurrence('K')) {
            source.handleJuedou(this);
        } else {
            decreaseHp(source);
        }
    }
}

// 反猪
class FPig extends Pig {
    public FPig(int id, List<Character> cards) {
        super(id, cards);
        this.publicType = 'U';
    }

    @Override
    protected void decreaseHp(Pig source) {
        super.decreaseHp(source);
        if (isDead()) {
            Global.checkGameState();
            source.cards.add(Global.cardsHeap.getCard());
            source.cards.add(Global.cardsHeap.getCard());
            source.cards.add(Global.cardsHeap.getCard());
        }
    }

    @Override
    protected boolean handleWuxieQuery(Pig source, Pig target) {
        if (target != null) {
            if (target.publicType == 'F') {
                if (cards.removeFirstOccurrence('J')) {
                    publicType = 'F';
                    return true;
                }
            }
        } else {
            if (source.publicType == 'M' || source.publicType == 'Z') {
                if (cards.removeFirstOccurrence('J')) {
                    publicType = 'F';
                    return true;
                }
            }
        }
        return false;
    }

    @Override
    protected boolean useSha() {
        Pig nextPig = getNextAlive();
        if (nextPig != null && (nextPig.publicType == 'M' || nextPig.publicType == 'Z')) {
            nextPig.handleSha(this);
            publicType = 'F';
            return true;
        }
        return false;
    }

    @Override
    protected boolean useJuedou() {
        for (Pig pig = next; !pig.isSame(this); pig = pig.next) {
            if (pig.publicType == 'M') {
                publicType = 'F';
                if (!askWuxie(pig))
                    pig.handleJuedou(this);
                return true;
            }
        }
        assert false;
        return false;
    }

    @Override
    protected void handleJuedou(Pig source) {
        if (cards.removeFirstOccurrence('K')) {
            source.handleJuedou(this);
        } else {
            decreaseHp(source);
        }
    }
}

public class Main {
    public static void main(String[] args) {
        Scanner scanner = new Scanner(System.in);
        int n = scanner.nextInt(), m = scanner.nextInt();
        for (int i = 0; i < n; ++i) {
            String type = scanner.next();
            ArrayList<Character> cards = new ArrayList<>();
            for (int j = 0; j < 4; ++j) {
                cards.add(scanner.next().charAt(0));
            }
            switch (type) {
                case "MP":
                    Global.pigs.add(new MPig(i, cards));
                    break;
                case "ZP":
                    Global.pigs.add(new ZPig(i, cards));
                    break;
                case "FP":
                    Global.pigs.add(new FPig(i, cards));
                    break;
            }
        }
        assert (Global.pigs.size() == n);
        for (int i = 0; i < n; ++i) {
            Global.pigs.get(i).setPrevAndNext(Global.pigs.get((i + n - 1) % n), Global.pigs.get((i + 1) % n));
        }
        ArrayList<Character> cards = new ArrayList<>();
        for (int i = 0; i < m; ++i) {
            cards.add(scanner.next().charAt(0));
        }
        Global.cardsHeap = new CardsHeap(cards);
        while (true) {
            for (int i = 0; i < n; ++i) {
                Global.pigs.get(i).work();
            }
        }
    }
}
