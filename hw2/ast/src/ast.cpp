#include "ast/ast.hpp"

namespace funny::ast {

const char* to_string(Type type) {
    switch (type) {
        case Type::Int: return "int";
        case Type::IntArray: return "int[]";
    }
    return "?";
}

const char* to_string(UnaryOp op) {
    switch (op) {
        case UnaryOp::Neg: return "neg";
        case UnaryOp::Not: return "not";
    }
    return "?";
}

const char* to_string(BinaryOp op) {
    switch (op) {
        case BinaryOp::Add: return "+";
        case BinaryOp::Sub: return "-";
        case BinaryOp::Mul: return "*";
        case BinaryOp::Div: return "/";
        case BinaryOp::Eq: return "==";
        case BinaryOp::Ne: return "!=";
        case BinaryOp::Lt: return "<";
        case BinaryOp::Le: return "<=";
        case BinaryOp::Gt: return ">";
        case BinaryOp::Ge: return ">=";
        case BinaryOp::And: return "and";
        case BinaryOp::Or: return "or";
        case BinaryOp::Implies: return "->";
    }
    return "?";
}

const char* to_string(QuantifierKind kind) {
    switch (kind) {
        case QuantifierKind::Forall: return "forall";
        case QuantifierKind::Exists: return "exists";
    }
    return "?";
}

}  // namespace funny::ast