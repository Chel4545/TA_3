#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>


class Statement;
class FunctionDecl;
class Expression;
class Program;

using StatementList = std::vector<Statement*>;
using FunctionList = std::vector<FunctionDecl*>;
using ExpressionList = std::vector<Expression*>;


enum class CommandKind {
    Top,
    Bottom,
    Left,
    Right
};

enum class ValueType {
    Signed,
    Unsigned
};


enum class BinaryOperator {
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
    Less,
    Greater,
    Equal
};


class RobotClient {
public:
    explicit RobotClient(std::string host = "127.0.0.1", int port = 5000);

    void move(CommandKind command);
    std::string xray();

private:
    std::string host;
    int port;

    static std::string commandToString(CommandKind command);
};

class Value {
public:
    ValueType type;
    long long signedValue;
    unsigned long long unsignedValue;

    static Value makeSigned(long long value);
    static Value makeUnsigned(unsigned long long value);

    static Value convertTo(const Value& value, ValueType targetType);

    std::string toString() const;
};

class Variable {
public:
    ValueType type;
    bool isConst;
    Value value;
};

class Parameter {
public:
    Parameter(ValueType type, const std::string& name);

    ValueType type;
    std::string name;
};

using ParameterList = std::vector<Parameter*>;


class RuntimeContext {
public:
    explicit RuntimeContext(RobotClient& robotClient, const Program& program);

    RobotClient& robot();

    void callFunction(const std::string& name, const std::vector<Value>& arguments);

    void pushFrame();
    void popFrame();


    void declareVariable(
            const std::string& name,
            ValueType type,
            bool isConst,
            const Value& value
    );

    void assignVariable(const std::string& name, const Value& value);
    Value getVariable(const std::string& name) const;

private:
    RobotClient& robotClient;
    const Program& program;

    std::vector<std::unordered_map<std::string, Variable>> frames;

    std::unordered_map<std::string, Variable>& currentFrame();
    const std::unordered_map<std::string, Variable>& currentFrame() const;
};




class Expression {
public:
    virtual ~Expression() = default;
    virtual Value evaluate(RuntimeContext& context) const = 0;
};

class LiteralExpression : public Expression {
public:
    explicit LiteralExpression(Value value);

    Value evaluate(RuntimeContext& context) const override;

private:
    Value value;
};

class VariableExpression : public Expression {
public:
    explicit VariableExpression(const std::string& name);

    Value evaluate(RuntimeContext& context) const override;

private:
    std::string name;
};

class BinaryExpression : public Expression {
public:
    BinaryExpression(BinaryOperator op, Expression* left, Expression* right);
    ~BinaryExpression();

    Value evaluate(RuntimeContext& context) const override;

private:
    BinaryOperator op;
    Expression* left;
    Expression* right;
};

class UnaryMinusExpression : public Expression {
public:
    explicit UnaryMinusExpression(Expression* expression);
    ~UnaryMinusExpression();

    Value evaluate(RuntimeContext& context) const override;

private:
    Expression* expression;
};




class Statement {
public:
    virtual ~Statement() = default;
    virtual void execute(RuntimeContext& context) const = 0;
};



class MoveStatement : public Statement {
public:
    explicit MoveStatement(CommandKind command);

    void execute(RuntimeContext& context) const override;

private:
    CommandKind command;
};

class XrayStatement : public Statement {
public:
    void execute(RuntimeContext& context) const override;
};

class VarDeclStatement : public Statement {
public:
    VarDeclStatement(
            ValueType type,
            const std::string& name,
            bool isConst,
            Expression* initExpression
    );

    ~VarDeclStatement();

    void execute(RuntimeContext& context) const override;

private:
    ValueType type;
    std::string name;
    bool isConst;
    Expression* initExpression;
};

class AssignmentStatement : public Statement {
public:
    AssignmentStatement(const std::string& name, Expression* expression);
    ~AssignmentStatement();

    void execute(RuntimeContext& context) const override;

private:
    std::string name;
    Expression* expression;
};

class TestOnceStatement : public Statement {
public:
    TestOnceStatement(Expression* condition, StatementList* body);
    ~TestOnceStatement();

    void execute(RuntimeContext& context) const override;

private:
    Expression* condition;
    StatementList body;
};

class TestRepStatement : public Statement {
public:
    TestRepStatement(Expression* condition, StatementList* body);
    ~TestRepStatement();

    void execute(RuntimeContext& context) const override;

private:
    Expression* condition;
    StatementList body;
};

class CallStatement : public Statement {
public:
    CallStatement(const std::string& functionName, ExpressionList* arguments);
    ~CallStatement();

    void execute(RuntimeContext& context) const override;

private:
    std::string functionName;
    ExpressionList arguments;
};

class FunctionDecl {
public:
    FunctionDecl(const std::string& name, ParameterList* parameters, StatementList* statements);
    ~FunctionDecl();

    const std::string& getName() const;

    void bindParameters(RuntimeContext& context, const std::vector<Value>& arguments) const;
    void execute(RuntimeContext& context) const;

private:
    std::string name;
    ParameterList parameters;
    StatementList statements;
};


class Program {
public:
    explicit Program(FunctionList* functions);
    ~Program() = default;

    void run(RobotClient& robotClient) const;
    void callFunction(const std::string& name, const std::vector<Value>& arguments, RuntimeContext& context) const;

private:
    std::unordered_map<std::string, std::unique_ptr<FunctionDecl>> functions;
};