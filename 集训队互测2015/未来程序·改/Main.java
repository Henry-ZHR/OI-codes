import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Queue;
import java.util.Scanner;
import java.util.Set;
import java.util.Stack;

// Make UOJ happy
public class Main {
    public static void main(String[] args) {
        final Scanner scanner = new Scanner(System.in);
        final int n = scanner.nextInt();
        final int[] a = new int[n];
        for (int i = 0; i < n; ++i) {
            a[i] = scanner.nextInt();
        }
        scanner.nextLine();
        final ArrayList<String> lines = new ArrayList<>();
        while (scanner.hasNext()) {
            lines.add(scanner.nextLine());
        }
        Interpreter.interpret(lines.toArray(new String[0]), a);
    }
}

final class Variables {
    private final Variables parentVars;
    private final Map<String, Integer> scopedVars = new HashMap<>();
    private final Set<String> scopedNames = new HashSet<>();

    public Variables() {
        parentVars = null;
    }

    public Variables(Variables other) {
        parentVars = other;
    }

    private static String getOriginalName(String name) {
        final int p = name.indexOf('[');
        return p == -1 ? name : name.substring(0, p);
    }

    public int get(String name) {
        if (scopedNames.contains(getOriginalName(name)) || parentVars == null) {
            return scopedVars.getOrDefault(name, 0);
        }
        return parentVars.get(name);
    }

    public void set(String name, int value) {
        if (scopedNames.contains(getOriginalName(name)) || parentVars == null) {
            scopedVars.put(name, value);
        } else {
            parentVars.set(name, value);
        }
    }

    public void define(String name) {
        scopedNames.add(name);
        scopedVars.put(name, 0);
    }
}

final class InputPool {
    private final Queue<Integer> pool = new LinkedList<>();

    public InputPool(int[] inputs) {
        for (int item : inputs) {
            pool.offer(item);
        }
    }

    public int get() {
        assert pool.peek() != null;
        return pool.poll();
    }
}

final class Environment {
    public final Variables variables;
    public final InputPool inputPool;
    public final Map<String, Function> functions;

    public Environment(Variables variables, InputPool inputPool,
                       Map<String, Function> functions) {
        this.variables = variables;
        this.inputPool = inputPool;
        this.functions = functions;
    }
}

final class ReturnValueException extends Throwable {
    public final int value;

    public ReturnValueException(int value) {
        this.value = value;
    }
}

class Expr {
    private static int getPriority(String operator) {
        int priority = switch (operator) {
            case "!", "++", "--" -> 12;
            case "*", "/", "%" -> 11;
            case "+", "-" -> 10;
            case "<=", ">=", "<", ">" -> 9;
            case "==", "!=" -> 8;
            case "^" -> 7;
            case "&&" -> 6;
            case "||" -> 5;
            case "=" -> 4;
            case "<<", ">>" -> 3;
            case "," -> 2;
            case ")", "]" -> 1;
            case "(", "((", "[" -> 0;
            default -> -1;
        };
        assert priority >= 0;
        return priority;
    }

    private static boolean canPop(String lastOperator, String thisOperator) {
        if (thisOperator.equals("!") || thisOperator.equals("++") || thisOperator.equals("--")) {
            return false;
        }
        return getPriority(lastOperator) > getPriority(thisOperator) ||
                (getPriority(lastOperator) == getPriority(thisOperator) && !thisOperator.equals("="));
    }

    private static Expr parseArithmeticExpr(List<String> _tokens) {
        if (_tokens.isEmpty()) {
            return new EmptyExpr();
        }
        final Stack<String> operator = new Stack<>();
        final Stack<Expr> operands = new Stack<>();
        boolean lastIsOperand = false;
        List<String> tokens = new ArrayList<>(_tokens);
        tokens.add(0, "(");
        tokens.add(")");
        for (String token : tokens) {
            if (token.equals("cin")) {
                operands.push(new InputOperation());
                lastIsOperand = true;
                continue;
            }
            if (token.equals("cout")) {
                operands.push(new OutputOperation());
                lastIsOperand = true;
                continue;
            }
            if (token.equals("endl")) {
                operands.push(new Endl());
                lastIsOperand = true;
                continue;
            }
            if (token.matches("[0-9a-zA-Z_]+")) {
                operands.push(new SingleVariable(token));
                lastIsOperand = true;
                continue;
            }
            if (!lastIsOperand) {
                switch (token) {
                    case "+" -> token = "++";
                    case "-" -> token = "--";
                }
            } else {
                if (token.equals("(")) {
                    token = "((";
                }
            }
            while (!token.equals("(") && !token.equals("((") && !token.equals("[") && !operator.empty() && canPop(operator.peek(), token)) {
                switch (operator.peek()) {
                    case "!", "++", "--" -> {
                        final Expr expr = operands.pop();
                        operands.push(new UnaryOperation(operator.peek(), expr));
                    }
                    case "=" -> {
                        final Expr expr = operands.pop();
                        operands.push(new AssignOperation((SingleVariable) operands.pop(), expr));
                    }
                    case ">>" -> {
                        final SingleVariable var = (SingleVariable) operands.pop();
                        ((InputOperation) operands.peek()).addVar(var);
                    }
                    case "<<" -> {
                        final Expr expr = operands.pop();
                        ((OutputOperation) operands.peek()).addVar(expr);
                    }
                    case "," -> {
                        final Expr rhs = operands.pop();
                        Expr lhs = operands.pop();
                        if (lhs instanceof ArgList) {
                            ((ArgList) lhs).addArg(rhs);
                        } else {
                            final ArgList argList = new ArgList();
                            argList.addArg(lhs);
                            argList.addArg(rhs);
                            lhs = argList;
                        }
                        operands.push(lhs);
                    }
                    default -> {
                        final Expr rhs = operands.pop();
                        final Expr lhs = operands.pop();
                        operands.push(new BinaryOperation(operator.peek(), lhs, rhs));
                    }
                }
                operator.pop();
            }
            switch (token) {
                case ")" -> {
                    if (!lastIsOperand) {
                        operands.push(new EmptyExpr());
                    }
                    switch (operator.pop()) {
                        case "(" -> {
                        }
                        case "((" -> {
                            ArgList argList;
                            if (operands.peek() instanceof ArgList) {
                                argList = (ArgList) operands.pop();
                            } else {
                                argList = new ArgList();
                                if (!(operands.peek() instanceof EmptyExpr)) {
                                    argList.addArg(operands.peek());
                                }
                                operands.pop();
                            }
                            final String name = ((SingleVariable) operands.peek()).getOriginalName();
                            operands.pop();
                            operands.push(new FunctionCall(name, argList.args));
                        }
                        default -> {
                            assert false;
                        }
                    }
                    lastIsOperand = true;
                }
                case "]" -> {
                    assert operator.peek().equals("[");
                    final Expr expr = operands.pop();
                    ((SingleVariable) operands.peek()).addIndex(expr);
                    operator.pop();
                    lastIsOperand = true;
                }
                default -> {
                    operator.push(token);
                    lastIsOperand = false;
                }
            }
        }
        assert operator.isEmpty();
        assert operands.size() == 1;
        return operands.peek();
    }

    private static int findStatementEnd(List<String> tokens, int start) {
        if (tokens.get(start).equals("{")) {
            int cnt = 0;
            for (int i = start; i < tokens.size(); ++i) {
                switch (tokens.get(i)) {
                    case "{" -> ++cnt;
                    case "}" -> {
                        --cnt;
                        if (cnt == 0) {
                            return i;
                        }
                    }
                }
            }
        } else {
            switch (tokens.get(start)) {
                case "if", "for", "while" -> {
                    return findStatementEnd(tokens, findClosingBracket(tokens, start + 1) + 1);
                }
            }
            for (int i = start; i < tokens.size(); ++i) {
                if (tokens.get(i).equals(";")) {
                    return i;
                }
            }
        }
        throw new RuntimeException("failed to find statement end");
    }

    // find ')'
    private static int findClosingBracket(List<String> tokens, int index) {
        int cnt = 0;
        assert tokens.get(index).equals("(");
        for (int i = index; i < tokens.size(); ++i) {
            switch (tokens.
                    get(i)) {
                case "(" -> ++cnt;
                case ")" -> {
                    --cnt;
                    if (cnt == 0) {
                        return i;
                    }
                }
            }
        }
        throw new RuntimeException("failed to find closing bracket");
    }

    public static Expr parse(List<String> tokens) {
        ExprList list = new ExprList();
        for (int i = 0; i < tokens.size(); ++i) {
            switch (tokens.get(i)) {
                case "return" -> {
                    int p = findStatementEnd(tokens, i + 1);
                    list.addExpr(new ReturnStatement(parseArithmeticExpr(tokens.subList(i + 1, p))));
                    i = p;
                }
                case "for" -> {
                    assert tokens.get(i + 1).equals("(");
                    int p = findClosingBracket(tokens, i + 1);
                    assert tokens.get(p).equals(")");
                    List<Integer> pos = new ArrayList<>();
                    for (int j = i + 1; j < p; ++j) {
                        if (tokens.get(j).equals(";")) {
                            pos.add(j);
                        }
                    }
                    assert pos.size() == 2;
                    List<Expr> exprs = new ArrayList<>();
                    exprs.add(parse(tokens.subList(i + 2, pos.get(0) + 1)));
                    exprs.add(parseArithmeticExpr(tokens.subList(pos.get(0) + 1, pos.get(1))));
                    exprs.add(parseArithmeticExpr(tokens.subList(pos.get(1) + 1, p)));
                    i = p + 1;
                    p = findStatementEnd(tokens, i);
                    exprs.add(parse(tokens.subList(i, p + 1)));
                    list.addExpr(new ForStatement(exprs.get(0), exprs.get(1), exprs.get(2), exprs.get(3)));
                    i = p;
                }
                case "while" -> {
                    assert tokens.get(i + 1).equals("(");
                    int p = findClosingBracket(tokens, i + 1);
                    assert tokens.get(p).equals(")");
                    List<Expr> exprs = new ArrayList<>();
                    exprs.add(parseArithmeticExpr(tokens.subList(i + 2, p)));
                    i = p + 1;
                    p = findStatementEnd(tokens, i);
                    exprs.add(parse(tokens.subList(i, p + 1)));
                    list.addExpr(new WhileStatement(exprs.get(0), exprs.get(1)));
                    i = p;
                }
                case "if" -> {
                    assert tokens.get(i + 1).equals("(");
                    int p = findClosingBracket(tokens, i + 1);
                    assert tokens.get(p).equals(")");
                    List<Expr> exprs = new ArrayList<>();
                    exprs.add(parseArithmeticExpr(tokens.subList(i + 2, p)));
                    i = p + 1;
                    p = findStatementEnd(tokens, i);
                    exprs.add(parse(tokens.subList(i, p + 1)));
                    i = p;
                    if (i + 1 < tokens.size() && tokens.get(i + 1).equals("else")) {
                        p = findStatementEnd(tokens, i + 2);
                        exprs.add(parse(tokens.subList(i + 2, p + 1)));
                        list.addExpr(new IfElseStatement(exprs.get(0), exprs.get(1), exprs.get(2)));
                        i = p;
                    } else {
                        list.addExpr(new IfElseStatement(exprs.get(0), exprs.get(1)));
                    }
                }
                case "{" -> {
                    int p = findStatementEnd(tokens, i);
                    list.addExpr(new ExprWrapper(parse(tokens.subList(i + 1, p))));
                    i = p;
                }
                case "int" -> {
                    List<DefineOperation> defines = new ArrayList<>();
                    for (++i; i < tokens.size(); ++i) {
                        if (tokens.get(i).equals(";")) {
                            break;
                        }
                        switch (tokens.get(i)) {
                            case "[" -> {
                                while (!tokens.get(i).equals("]")) {
                                    ++i;
                                }
                            }
                            case "," -> {
                            }
                            default -> defines.add(new DefineOperation(tokens.get(i)));
                        }
                    }
                    for (DefineOperation opt : defines) {
                        list.addExpr(opt);
                    }
                }
                default -> {
                    int p = findStatementEnd(tokens, i);
                    assert tokens.get(p).equals(";");
                    list.addExpr(parseArithmeticExpr(tokens.subList(i, p)));
                    i = p;
                }
            }
        }
        return list;
    }

    public int evaluate(Environment env) throws RuntimeException, ReturnValueException {
        throw new RuntimeException("not implemented");
    }
}

final class ExprWrapper extends Expr {
    private final Expr expr;

    public ExprWrapper(Expr expr) {
        this.expr = expr;
    }

    @Override
    public int evaluate(Environment env) throws ReturnValueException {
        return expr.evaluate(new Environment(new Variables(env.variables), env.inputPool, env.functions));
    }
}

final class InputOperation extends Expr {
    private final List<SingleVariable> vars = new ArrayList<>();

    public void addVar(SingleVariable var) {
        vars.add(var);
    }

    @Override
    public int evaluate(Environment env) {
        for (SingleVariable var : vars) {
            String name = var.getEvaluatedName(env);
            env.variables.set(name, env.inputPool.get());
        }
        return 0;
    }
}

final class OutputOperation extends Expr {
    private final List<Expr> exprs = new ArrayList<>();

    public void addVar(Expr expr) {
        exprs.add(expr);
    }

    @Override
    public int evaluate(Environment env) {
        try {
            for (Expr expr : exprs) {
                if (expr instanceof Endl) {
                    System.out.println();
                } else {
                    System.out.print(expr.evaluate(env));
                }
            }
        } catch (Throwable t) {
            assert false;
        }
        return 0;
    }
}

final class Endl extends Expr {
}

final class DefineOperation extends Expr {
    private final String name;

    public DefineOperation(String name) {
        this.name = name;
    }

    @Override
    public int evaluate(Environment env) {
        env.variables.define(name);
        return 0;
    }
}

final class AssignOperation extends Expr {
    private final SingleVariable variable;
    private final Expr value;

    public AssignOperation(SingleVariable variable, Expr value) {
        this.variable = variable;
        this.value = value;
    }

    @Override
    public int evaluate(Environment env) {
        try {
            int x = value.evaluate(env);
            final String name = variable.getEvaluatedName(env);
            env.variables.set(name, x);
            return x;
        } catch (Throwable t) {
            assert false;
        }
        return 0;
    }
}

final class UnaryOperation extends Expr {
    private final String operator;
    private final Expr operand;

    public UnaryOperation(String operator, Expr operand) {
        this.operator = operator;
        this.operand = operand;
    }

    @Override
    public int evaluate(Environment env) {
        try {
            int x = operand.evaluate(env);
            return switch (operator) {
                case "!" -> x == 0 ? 1 : 0;
                case "++" -> +x;
                case "--" -> -x;
                default -> throw new RuntimeException(String.format("unknown operator %s", operator));
            };
        } catch (Throwable t) {
            assert false;
        }
        return 0;
    }
}

final class BinaryOperation extends Expr {
    private final String operator;
    private final Expr lhs, rhs;

    public BinaryOperation(String operator, Expr lhs, Expr rhs) {
        this.operator = operator;
        this.lhs = lhs;
        this.rhs = rhs;
    }

    @Override
    public int evaluate(Environment env) {
        try {
            int x = lhs.evaluate(env), y = rhs.evaluate(env);
            return switch (operator) {
                case "+" -> x + y;
                case "-" -> x - y;
                case "*" -> x * y;
                case "/" -> x / y;
                case "%" -> x % y;
                case "<=" -> x <= y ? 1 : 0;
                case ">=" -> x >= y ? 1 : 0;
                case "<" -> x < y ? 1 : 0;
                case ">" -> x > y ? 1 : 0;
                case "==" -> x == y ? 1 : 0;
                case "!=" -> x != y ? 1 : 0;
                case "^" -> ((x != 0) ^ (y != 0)) ? 1 : 0;
                case "&&" -> x != 0 && y != 0 ? 1 : 0;
                case "||" -> x != 0 || y != 0 ? 1 : 0;
                default -> throw new RuntimeException(String.format("unknown operator %s", operator));
            };
        } catch (Throwable t) {
            assert false;
        }
        return 0;
    }
}

final class ReturnStatement extends Expr {
    private final Expr value;

    public ReturnStatement(Expr expr) {
        value = expr;
    }

    @Override
    public int evaluate(Environment env) throws ReturnValueException {
        throw new ReturnValueException(value.evaluate(env));
    }
}

final class ForStatement extends Expr {
    private final Expr init, cond, loop, statement;

    public ForStatement(Expr init, Expr cond, Expr loop, Expr statement) {
        this.init = init;
        this.cond = cond;
        this.loop = loop;
        this.statement = statement;
    }

    @Override
    public int evaluate(Environment env) throws ReturnValueException {
        for (init.evaluate(env); cond instanceof EmptyExpr || cond.evaluate(env) != 0; loop.evaluate(env)) {
            statement.evaluate(env);
        }
        return 0;
    }
}

final class WhileStatement extends Expr {
    private final Expr cond, statement;

    public WhileStatement(Expr cond, Expr statement) {
        this.cond = cond;
        this.statement = statement;
    }

    @Override
    public int evaluate(Environment env) throws ReturnValueException {
        while (cond.evaluate(env) != 0) {
            statement.evaluate(env);
        }
        return 0;
    }
}

final class IfElseStatement extends Expr {
    private final Expr cond, statement1, statement2;

    public IfElseStatement(Expr cond, Expr statement1, Expr statement2) {
        this.cond = cond;
        this.statement1 = statement1;
        this.statement2 = statement2;
    }

    public IfElseStatement(Expr cond, Expr statement1) {
        this(cond, statement1, new EmptyExpr());
    }

    @Override
    public int evaluate(Environment env) throws ReturnValueException {
        if (cond.evaluate(env) != 0) {
            statement1.evaluate(env);
        } else {
            statement2.evaluate(env);
        }
        return 0;
    }
}

final class EmptyExpr extends Expr {
    @Override
    public int evaluate(Environment env) {
        return 0;
    }
}

final class ExprList extends Expr {
    private final List<Expr> exprs = new ArrayList<>();

    public void addExpr(Expr expr) {
        exprs.add(expr);
    }

    @Override
    public int evaluate(Environment env) throws RuntimeException, ReturnValueException {
        for (Expr expr : exprs) {
            expr.evaluate(env);
        }
        return 0;
    }
}

final class SingleVariable extends Expr {
    private final String name;
    private final List<Expr> indexes = new ArrayList<>();

    public SingleVariable(String name) {
        this.name = name;
    }

    public void addIndex(Expr expr) {
        indexes.add(expr);
    }

    public String getOriginalName() {
        assert indexes.isEmpty();
        return name;
    }

    public String getEvaluatedName(Environment env) {
        if (Character.isDigit(name.charAt(0))) {
            assert name.matches("[0-9]+");
            assert indexes.isEmpty();
            return name;
        }
        final StringBuilder sb = new StringBuilder(name);
        for (Expr expr : indexes) {
            try {
                sb.append('[').append(expr.evaluate(env)).append(']');
            } catch (Throwable t) {
                assert false;
            }
        }
        return sb.toString();
    }

    @Override
    public int evaluate(Environment env) {
        if (Character.isDigit(name.charAt(0))) {
            assert name.matches("[0-9]+");
            assert indexes.isEmpty();
            return Integer.parseInt(name);
        }
        return env.variables.get(getEvaluatedName(env));
    }
}

final class FunctionCall extends Expr {
    private final String name;
    private final List<Expr> args;

    public FunctionCall(String name, List<Expr> args) {
        this.name = name;
        this.args = args;
    }

    @Override
    public int evaluate(Environment env) {
        if (name.equals("putchar")) {
            assert args.size() == 1;
            int x = 0;
            try {
                x = args.get(0).evaluate(env);
            } catch (Throwable t) {
                assert false;
            }
            System.out.print((char) x);
            return x;
        }
        int[] argValues = new int[args.size()];
        for (int i = 0; i < args.size(); ++i) {
            try {
                argValues[i] = args.get(i).evaluate(env);
            } catch (Throwable t) {
                assert false;
            }
        }
        return env.functions.get(name).evaluate(
                new Environment(new Variables(env.variables), env.inputPool, env.functions),
                argValues
        );
    }
}

final class ArgList extends Expr {
    public final List<Expr> args = new ArrayList<>();

    public void addArg(Expr expr) {
        args.add(expr);
    }
}

class Function {
    private final String name;
    private final List<String> argNames = new ArrayList<>();
    private final Expr expr;

    public Function(List<String> tokens) {
        assert "int".equals(tokens.get(0));
        assert "(".equals(tokens.get(2));
        name = tokens.get(1);
        int p = 3;
        while (!tokens.get(p).equals(")")) {
            ++p;
        }
        assert p == 3 || p % 3 == 2;
        for (int i = 3; i < p; i += 3) {
            assert tokens.get(i).equals("int");
            argNames.add(tokens.get(i + 1));
            assert i + 2 == p || tokens.get(i + 2).equals(",");
        }
        assert tokens.get(p + 1).equals("{");
        assert tokens.get(tokens.size() - 1).equals("}");
        expr = Expr.parse(tokens.subList(p + 2, tokens.size() - 1));
    }

    public String getName() {
        return name;
    }

    private void setVariablesFromArgs(Environment env, int[] args) {
        assert argNames.size() == args.length;
        for (int i = 0; i < args.length; ++i) {
            env.variables.define(argNames.get(i));
            env.variables.set(argNames.get(i), args[i]);
        }
    }

    public int evaluate(Environment env, int[] args) {
        setVariablesFromArgs(env, args);
        try {
            expr.evaluate(env);
        } catch (ReturnValueException e) {
            return e.value;
        }
        return 0;
    }
}

final class Program {
    private final Map<String, Function> functions = new HashMap<>();

    public Program(List<String> tokens) {
        for (int i = 0; i < tokens.size(); ++i) {
            assert tokens.get(i).equals("int");
            // Try to parse function
            if (tokens.get(i + 2).equals("(")) {
                int cnt = 0, p = -1;
                for (int j = i + 3; j < tokens.size(); ++j) {
                    if (tokens.get(j).equals("{")) {
                        ++cnt;
                        continue;
                    }
                    if (tokens.get(j).equals("}")) {
                        --cnt;
                        if (cnt == 0) {
                            p = j;
                            break;
                        }
                    }
                }
                assert p != -1;
                Function function = new Function(tokens.subList(i, p + 1));
                functions.put(function.getName(), function);
                i = p;
            } else {
                // Ignore all variable definitions
                while (!tokens.get(i).equals(";")) {
                    ++i;
                }
            }
        }
    }

    void execute(int[] inputs) {
        final Environment env = new Environment(new Variables(), new InputPool(inputs), functions);
        functions.get("main").evaluate(env, new int[0]);
    }
}

final class Interpreter {
    private static boolean canBeSameToken(String last, char current) {
        return ((last.equals("<") || last.equals(">") || last.equals("=") || last.equals("!")) && current == '=') ||
                (last.equals("&") && current == '&') ||
                (last.equals("|") && current == '|') ||
                (last.equals("<") && current == '<') ||
                (last.equals(">") && current == '>') ||
                ((last + current).matches("[0-9a-zA-Z_]{2,}"));
    }

    public static void interpret(String[] lines, int[] inputs) {
        assert "#include<iostream>".equals(lines[0]);
        assert "#include<cstdio>".equals(lines[1]);
        assert "using namespace std;".equals(lines[2]);
        final StringBuilder sb = new StringBuilder();
        for (int i = 3; i < lines.length; ++i) {
            sb.append(lines[i]).append(' ');
        }
        String currentToken = "";
        final ArrayList<String> tokens = new ArrayList<>();
        for (int i = 0; i < sb.length(); ++i) {
            if (!canBeSameToken(currentToken, sb.charAt(i))) {
                if (!currentToken.isBlank()) {
                    tokens.add(currentToken);
                }
                currentToken = String.valueOf(sb.charAt(i));
            } else {
                currentToken += sb.charAt(i);
            }
        }
        if (!currentToken.isBlank()) {
            tokens.add(currentToken);
        }
        final Program program = new Program(tokens);
        program.execute(inputs);
    }
}