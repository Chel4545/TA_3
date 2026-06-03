#include "ast.hpp"
#include "httplib.h"

#include <iostream>
#include <stdexcept>
#include <utility>
#include <limits>

RobotClient::RobotClient(std::string host, int port)
        : host(std::move(host)), port(port) {
}

std::string RobotClient::commandToString(CommandKind command) {
    switch (command) {
        case CommandKind::Top:
            return "top";

        case CommandKind::Bottom:
            return "bottom";

        case CommandKind::Left:
            return "left";

        case CommandKind::Right:
            return "right";
    }

    throw std::runtime_error("Unknown robot command");
}

void RobotClient::move(CommandKind command) {
    std::string direction = commandToString(command);
    std::string path = "/move/" + direction;

    httplib::Client client(host, port);

    client.set_connection_timeout(2, 0);
    client.set_read_timeout(2, 0);
    client.set_write_timeout(2, 0);

    auto response = client.Post(path.c_str());

    if (!response) {
        throw std::runtime_error("Cannot connect to robot game server");
    }

    if (response->status != 200) {
        throw std::runtime_error("Robot game server returned HTTP status ");
    }

    std::string& body = response->body;

    bool ok = body.find("\"ok\":true") != std::string::npos ||
              body.find("\"ok\": true") != std::string::npos;

    if (!ok) {
        throw std::runtime_error("Robot move failed. Direction: " + direction + ". Server response: " + body);
    }

    std::cout << "MOVE " << direction << " -> " << body << std::endl;
}

std::string RobotClient::xray() {
    httplib::Client client(host, port);

    client.set_connection_timeout(2, 0);
    client.set_read_timeout(2, 0);
    client.set_write_timeout(2, 0);

    auto response = client.Get("/xray");

    if (!response) {
        throw std::runtime_error(
                "Cannot connect to robot game server at http://" + host + ":" + std::to_string(port)
        );
    }

    if (response->status != 200) {
        throw std::runtime_error(
                "Robot game server returned HTTP status " + std::to_string(response->status)
        );
    }

    const std::string& body = response->body;

    std::cout << "XRAY -> " << body << std::endl;

    return body;
}

//переменные

Value Value::makeSigned(long long value) {
    Value result{};
    result.type = ValueType::Signed;
    result.signedValue = value;
    result.unsignedValue = 0;
    return result;
}


Value Value::makeUnsigned(unsigned long long value) {
    Value result{};
    result.type = ValueType::Unsigned;
    result.signedValue = 0;
    result.unsignedValue = value;
    return result;
}

Value Value::convertTo(const Value& value, ValueType targetType) {
    if (value.type == targetType) {
        return value;
    }

    if (targetType == ValueType::Signed) {
        if (value.unsignedValue > static_cast<unsigned long long>(std::numeric_limits<long long>::max())) {
            throw std::runtime_error("Cannot convert unsigned to signed: value is too large");
        }

        return Value::makeSigned(static_cast<long long>(value.unsignedValue));
    }

    if (targetType == ValueType::Unsigned) {
        if (value.signedValue < 0) {
            throw std::runtime_error("Cannot convert negative signed value to unsigned");
        }

        return Value::makeUnsigned(static_cast<unsigned long long>(value.signedValue));
    }

    throw std::runtime_error("Unknown target type");
}


std::string Value::toString() const {
    if (type == ValueType::Signed) {
        return "signed(" + std::to_string(signedValue) + ")";
    }

    return "unsigned(" + std::to_string(unsignedValue) + ")";
}

// Variables

RuntimeContext::RuntimeContext(RobotClient& robotClient, const Program& program)
        : robotClient(robotClient), program(program) {
}


RobotClient& RuntimeContext::robot() {
    return robotClient;
}

void RuntimeContext::callFunction(const std::string& name, const std::vector<Value>& arguments) {
    program.callFunction(name,arguments,  *this);
}

void RuntimeContext::pushFrame() {
    frames.emplace_back();
}


void RuntimeContext::popFrame() {
    if (frames.empty()) {
        throw std::runtime_error("Call stack is empty");
    }

    frames.pop_back();
}

std::unordered_map<std::string, Variable>& RuntimeContext::currentFrame() {
    if (frames.empty()) {
        throw std::runtime_error("No active function frame");
    }

    return frames.back();
}

const std::unordered_map<std::string, Variable>& RuntimeContext::currentFrame() const {
    if (frames.empty()) {
        throw std::runtime_error("No active function frame");
    }

    return frames.back();
}


void RuntimeContext::declareVariable(
        const std::string& name,
        ValueType type,
        bool isConst,
        const Value& value
) {

    auto& variables = currentFrame();

    if (variables.contains(name)) {
        throw std::runtime_error("Variable already declared: " + name);
    }

    Value convertedValue = Value::convertTo(value, type);

    variables[name] = Variable{
            type,
            isConst,
            convertedValue
    };

    std::cout << "DECLARE " << name << " <- " << convertedValue.toString();

    if (isConst) {
        std::cout << " const";
    }

    std::cout << std::endl;
}

void RuntimeContext::assignVariable(const std::string& name, const Value& value) {
    auto& variables = currentFrame();

    auto iterator = variables.find(name);

    if (iterator == variables.end()) {
        throw std::runtime_error("Variable was not declared: " + name);
    }

    Variable& variable = iterator->second;

    if (variable.isConst) {
        throw std::runtime_error("Cannot assign to const variable: " + name);
    }

    Value convertedValue = Value::convertTo(value, variable.type);
    variable.value = convertedValue;

    std::cout << "ASSIGN " << name << " <- " << convertedValue.toString() << std::endl;
}

Value RuntimeContext::getVariable(const std::string& name) const {
    const auto& variables = currentFrame();

    auto iterator = variables.find(name);

    if (iterator == variables.end()) {
        throw std::runtime_error("Variable was not declared: " + name);
    }

    return iterator->second.value;
}

// LiteralExpression

LiteralExpression::LiteralExpression(Value value)
        : value(value) {
}

Value LiteralExpression::evaluate(RuntimeContext& context) const {
    (void) context;
    return value;
}

// VariableExpression

VariableExpression::VariableExpression(const std::string& name)
        : name(name) {
}


Value VariableExpression::evaluate(RuntimeContext& context) const {
    return context.getVariable(name);
}


BinaryExpression::BinaryExpression(BinaryOperator op, Expression* left, Expression* right)
        : op(op), left(left), right(right) {
}


BinaryExpression::~BinaryExpression() {
    delete left;
    delete right;
}

Value BinaryExpression::evaluate(RuntimeContext& context) const {
    Value leftValue = left->evaluate(context);
    Value rightValue = right->evaluate(context);

    rightValue = Value::convertTo(rightValue, leftValue.type);

    if (leftValue.type == ValueType::Signed) {
        long long lhs = leftValue.signedValue;
        long long rhs = rightValue.signedValue;

        switch (op) {
            case BinaryOperator::Add:
                return Value::makeSigned(lhs + rhs);

            case BinaryOperator::Subtract:
                return Value::makeSigned(lhs - rhs);

            case BinaryOperator::Multiply:
                return Value::makeSigned(lhs * rhs);

            case BinaryOperator::Divide:
                if (rhs == 0) {
                    throw std::runtime_error("Division by zero");
                }
                return Value::makeSigned(lhs / rhs);

            case BinaryOperator::Modulo:
                if (rhs == 0) {
                    throw std::runtime_error("Modulo by zero");
                }
                return Value::makeSigned(lhs % rhs);

            case BinaryOperator::Less:
                return Value::makeSigned(lhs < rhs ? 1 : 0);

            case BinaryOperator::Greater:
                return Value::makeSigned(lhs > rhs ? 1 : 0);

            case BinaryOperator::Equal:
                return Value::makeSigned(lhs == rhs ? 1 : 0);
        }
    }

    unsigned long long lhs = leftValue.unsignedValue;
    unsigned long long rhs = rightValue.unsignedValue;

    switch (op) {
        case BinaryOperator::Add:
            return Value::makeUnsigned(lhs + rhs);

        case BinaryOperator::Subtract:
            if (rhs > lhs) {
                throw std::runtime_error("Unsigned subtraction underflow");
            }
            return Value::makeUnsigned(lhs - rhs);

        case BinaryOperator::Multiply:
            return Value::makeUnsigned(lhs * rhs);

        case BinaryOperator::Divide:
            if (rhs == 0) {
                throw std::runtime_error("Division by zero");
            }
            return Value::makeUnsigned(lhs / rhs);

        case BinaryOperator::Modulo:
            if (rhs == 0) {
                throw std::runtime_error("Modulo by zero");
            }
            return Value::makeUnsigned(lhs % rhs);

        case BinaryOperator::Less:
            return Value::makeSigned(lhs < rhs ? 1 : 0);

        case BinaryOperator::Greater:
            return Value::makeSigned(lhs > rhs ? 1 : 0);

        case BinaryOperator::Equal:
            return Value::makeSigned(lhs == rhs ? 1 : 0);
    }

    throw std::runtime_error("Unknown binary operator");
}

UnaryMinusExpression::UnaryMinusExpression(Expression* expression)
        : expression(expression) {
}


UnaryMinusExpression::~UnaryMinusExpression() {
    delete expression;
}


Value UnaryMinusExpression::evaluate(RuntimeContext& context) const {
    Value value = expression->evaluate(context);

    if (value.type != ValueType::Signed) {
        throw std::runtime_error("Unary minus can be applied only to signed values");
    }

    return Value::makeSigned(-value.signedValue);
}

// testonce(if)

TestOnceStatement::TestOnceStatement(Expression* condition, StatementList* body)
        : condition(condition), body(*body) {
    delete body;
}


TestOnceStatement::~TestOnceStatement() {
    delete condition;

    for (Statement* statement : body) {
        delete statement;
    }
}


void TestOnceStatement::execute(RuntimeContext& context) const {
    Value conditionValue = condition->evaluate(context);

    bool conditionIsTrue = false;

    if (conditionValue.type == ValueType::Signed) {
        conditionIsTrue = conditionValue.signedValue != 0;
    } else {
        conditionIsTrue = conditionValue.unsignedValue != 0;
    }

    if (!conditionIsTrue) {
        return;
    }

    for (const Statement* statement : body) {
        statement->execute(context);
    }
}

// testrep(while)

TestRepStatement::TestRepStatement(Expression* condition, StatementList* body)
        : condition(condition), body(*body) {
    delete body;
}


TestRepStatement::~TestRepStatement() {
    delete condition;

    for (Statement* statement : body) {
        delete statement;
    }
}


void TestRepStatement::execute(RuntimeContext& context) const {
    while (true) {
        Value conditionValue = condition->evaluate(context);

        bool conditionIsTrue = false;

        if (conditionValue.type == ValueType::Signed) {
            conditionIsTrue = conditionValue.signedValue != 0;
        } else {
            conditionIsTrue = conditionValue.unsignedValue != 0;
        }

        if (!conditionIsTrue) {
            break;
        }

        for (const Statement* statement : body) {
            statement->execute(context);
        }
    }
}

// Statements

MoveStatement::MoveStatement(CommandKind command) : command(command) {}

void MoveStatement::execute(RuntimeContext& context) const {
    context.robot().move(command);
}

void XrayStatement::execute(RuntimeContext& context) const {
    context.robot().xray();
}

VarDeclStatement::VarDeclStatement(
        ValueType type,
        const std::string& name,
        bool isConst,
        Expression* initExpression
) : type(type), name(name), isConst(isConst), initExpression(initExpression) {}

VarDeclStatement::~VarDeclStatement() {
    delete initExpression;
}

void VarDeclStatement::execute(RuntimeContext& context) const {
    if (isConst && initExpression == nullptr) {
        throw std::runtime_error("Const variable must be initialized: " + name);
    }

    Value initialValue = type == ValueType::Signed
                         ? Value::makeSigned(0)
                         : Value::makeUnsigned(0);

    if (initExpression != nullptr) {
        initialValue = initExpression->evaluate(context);
    }

    context.declareVariable(name, type, isConst, initialValue);
}

AssignmentStatement::AssignmentStatement(const std::string& name, Expression* expression)
        : name(name), expression(expression) {
}

AssignmentStatement::~AssignmentStatement() {
    delete expression;
}


void AssignmentStatement::execute(RuntimeContext& context) const {
    Value value = expression->evaluate(context);
    context.assignVariable(name, value);
}

CallStatement::CallStatement(const std::string& functionName, ExpressionList* arguments)
        : functionName(functionName), arguments(*arguments) {
    delete arguments;
}


CallStatement::~CallStatement() {
    for (Expression* argument : arguments) {
        delete argument;
    }
}


void CallStatement::execute(RuntimeContext& context) const {
    std::vector<Value> evaluatedArguments;

    for (const Expression* argument : arguments) {
        evaluatedArguments.push_back(argument->evaluate(context));
    }

    context.callFunction(functionName, evaluatedArguments);
}

// prog

FunctionDecl::FunctionDecl(
        const std::string& name,
        ParameterList* parameters,
        StatementList* statements
) : name(name), parameters(*parameters), statements(*statements) {
    delete parameters;
    delete statements;
}

FunctionDecl::~FunctionDecl() {
    for (Parameter* parameter : parameters) {
        delete parameter;
    }

    for (Statement* statement : statements) {
        delete statement;
    }
}


const std::string& FunctionDecl::getName() const {
    return name;
}

void FunctionDecl::bindParameters(RuntimeContext& context, const std::vector<Value>& arguments) const {
    if (arguments.size() != parameters.size()) {
        throw std::runtime_error(
                "Function '" + name + "' expects " +
                std::to_string(parameters.size()) +
                " arguments, got " +
                std::to_string(arguments.size())
        );
    }

    for (std::size_t i = 0; i < parameters.size(); ++i) {
        Parameter* parameter = parameters[i];

        context.declareVariable(
                parameter->name,
                parameter->type,
                false,
                arguments[i]
        );
    }
}

void FunctionDecl::execute(RuntimeContext& context) const {
    for (const Statement* statement : statements) {
        statement->execute(context);
    }
}


Program::Program(FunctionList* functionList) {
    for (FunctionDecl* function : *functionList) {
        const std::string& name = function->getName();

        if (functions.contains(name)) {
            delete function;
            delete functionList;
            throw std::runtime_error("Duplicate function declaration: " + name);
        }

        functions[name] = std::unique_ptr<FunctionDecl>(function);
    }

    delete functionList;
}


void Program::run(RobotClient& robotClient) const {
    RuntimeContext context(robotClient, *this);
    callFunction("start", {}, context);
}

void Program::callFunction(
        const std::string& name,
        const std::vector<Value>& arguments,
        RuntimeContext& context
) const {
    auto function = functions.find(name);

    if (function == functions.end()) {
        throw std::runtime_error("Function was not declared: " + name);
    }

    context.pushFrame();

    try {
        function->second->bindParameters(context, arguments);
        function->second->execute(context);
    } catch (...) {
        context.popFrame();
        throw;
    }

    context.popFrame();
}

Parameter::Parameter(ValueType type, const std::string& name)
        : type(type), name(name) {
}