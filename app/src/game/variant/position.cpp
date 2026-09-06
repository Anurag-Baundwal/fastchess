#include <game/variant/position.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace fastchess::rules {
namespace {
int square(std::string_view s) {
    if (s.size() != 2 || s[0] < 'a' || s[0] > 'h' || s[1] < '1' || s[1] > '8') return -1;
    return (s[1] - '1') * 8 + s[0] - 'a';
}
std::string squareName(int s) { return {char('a' + s % 8), char('1' + s / 8)}; }
char lower(char c) { return char(std::tolower(static_cast<unsigned char>(c))); }
bool whitePiece(char c) { return c >= 'A' && c <= 'Z'; }
bool number(std::string_view s, int& value, int minimum) {
    if (s.empty()) return false;
    int result = 0;
    for (char c : s) {
        if (c < '0' || c > '9' || result > (std::numeric_limits<int>::max() - (c - '0')) / 10) return false;
        result = result * 10 + c - '0';
    }
    if (result < minimum || result > 1000000000) return false;
    value = result;
    return true;
}
}

DuckPosition::DuckPosition() { setFen(chess::constants::STARTPOS); }

bool DuckPosition::setFen(std::string_view fen) {
    std::istringstream input{std::string(fen)};
    std::vector<std::string> fields;
    for (std::string s; input >> s;) fields.push_back(s);
    if (fields.size() != 4 && fields.size() != 6) return false;
    std::array<char, 64> board{};
    int rank = 7, file = 0, duck = -1, white_kings = 0, black_kings = 0;
    for (char c : fields[0]) {
        if (c == '/') {
            if (file != 8 || rank == 0) return false;
            --rank;
            file = 0;
        } else if (c >= '1' && c <= '8') {
            file += c - '0';
            if (file > 8) return false;
        } else {
            if (file >= 8 || std::string_view("PNBRQKpnbrqk*").find(c) == std::string_view::npos) return false;
            const int sq = rank * 8 + file++;
            if (c == '*') {
                if (duck != -1) return false;
                duck = sq;
            } else {
                if (lower(c) == 'p' && (rank == 0 || rank == 7)) return false;
                board[sq] = c;
                white_kings += c == 'K';
                black_kings += c == 'k';
            }
        }
    }
    if (rank != 0 || file != 8 || white_kings > 1 || black_kings > 1 || white_kings + black_kings == 0) return false;
    if (fields[1] != "w" && fields[1] != "b") return false;
    int rights = 0;
    if (fields[2] != "-") {
        if (fields[2].empty()) return false;
        for (char c : fields[2]) {
            const auto bit = std::string_view("KQkq").find(c);
            if (bit == std::string_view::npos || (rights & (1 << bit))) return false;
            rights |= 1 << bit;
        }
    }
    if ((rights & 1) && (board[4] != 'K' || board[7] != 'R')) return false;
    if ((rights & 2) && (board[4] != 'K' || board[0] != 'R')) return false;
    if ((rights & 4) && (board[60] != 'k' || board[63] != 'r')) return false;
    if ((rights & 8) && (board[60] != 'k' || board[56] != 'r')) return false;
    const bool white = fields[1] == "w";
    int ep = -1, halfmove = 0, fullmove = 1;
    if (fields[3] != "-") {
        ep = square(fields[3]);
        if (ep < 0 || ep / 8 != (white ? 5 : 2) || board[ep] ||
            board[ep + (white ? -8 : 8)] != (white ? 'p' : 'P') || board[ep + (white ? 8 : -8)]) return false;
    }
    if (fields.size() == 6 && (!number(fields[4], halfmove, 0) || !number(fields[5], fullmove, 1))) return false;
    squares_ = board;
    duck_ = duck;
    white_ = white;
    rights_ = rights;
    ep_ = ep;
    halfmove_ = halfmove;
    fullmove_ = fullmove;
    history_.clear();
    history_.push_back(repetitionKey());
    return true;
}

bool DuckPosition::setEpd(std::string_view epd) {
    std::istringstream input{std::string(epd)};
    std::string placement, side, rights, ep;
    if (!(input >> placement >> side >> rights >> ep)) return false;
    int halfmove = 0, fullmove = 1;
    std::string operation;
    while (std::getline(input, operation, ';')) {
        std::istringstream op(operation);
        std::string key, value, extra;
        op >> key;
        if (key == "hmvc" || key == "fmvn") {
            if (!(op >> value) || (op >> extra)) return false;
            if (!number(value, key == "hmvc" ? halfmove : fullmove, key == "hmvc" ? 0 : 1)) return false;
        }
    }
    return setFen(placement + " " + side + " " + rights + " " + ep + " " + std::to_string(halfmove) + " " + std::to_string(fullmove));
}

std::string DuckPosition::getFen(bool counters) const {
    std::string fen;
    for (int rank = 7; rank >= 0; --rank) {
        int empty = 0;
        for (int file = 0; file < 8; ++file) {
            const int sq = rank * 8 + file;
            const char piece = sq == duck_ ? '*' : squares_[sq];
            if (!piece) { ++empty; continue; }
            if (empty) { fen += char('0' + empty); empty = 0; }
            fen += piece;
        }
        if (empty) fen += char('0' + empty);
        if (rank) fen += '/';
    }
    fen += white_ ? " w " : " b ";
    if (!rights_) fen += '-';
    for (int i = 0; i < 4; ++i) if (rights_ & (1 << i)) fen += "KQkq"[i];
    fen += " " + (ep_ == -1 ? std::string("-") : squareName(ep_));
    if (counters) fen += " " + std::to_string(halfmove_) + " " + std::to_string(fullmove_);
    return fen;
}

std::string DuckPosition::getEpd() const {
    return getFen(false) + " hmvc " + std::to_string(halfmove_) + "; fmvn " + std::to_string(fullmove_) + ";";
}

std::optional<DuckTurn> DuckPosition::parseTurn(std::string_view move) {
    const auto comma = move.find(',');
    const auto piece = move.substr(0, comma);
    if (piece.size() != 4 && piece.size() != 5) return std::nullopt;
    DuckTurn turn;
    turn.from = square(piece.substr(0, 2));
    turn.to = square(piece.substr(2, 2));
    if (turn.from < 0 || turn.to < 0 || turn.from == turn.to) return std::nullopt;
    if (piece.size() == 5) {
        if (std::string_view("nbrq").find(piece[4]) == std::string_view::npos) return std::nullopt;
        turn.promotion = piece[4];
    }
    if (comma != std::string_view::npos) {
        const auto suffix = move.substr(comma + 1);
        if (suffix.size() != 4 || square(suffix.substr(0, 2)) != turn.to) return std::nullopt;
        turn.duck = square(suffix.substr(2, 2));
        if (turn.duck < 0) return std::nullopt;
    }
    return turn;
}

std::string DuckPosition::toUci(const DuckTurn& move) {
    std::string result = squareName(move.from) + squareName(move.to);
    if (move.promotion) result += move.promotion;
    if (move.duck >= 0) result += "," + squareName(move.to) + squareName(move.duck);
    return result;
}

bool DuckPosition::pieceLegal(const DuckTurn& move) const {
    const int from = move.from, to = move.to;
    if (from < 0 || from >= 64 || to < 0 || to >= 64 || from == to || to == duck_) return false;
    const char p = squares_[from], target = squares_[to];
    if (!p || whitePiece(p) != white_ || (target && whitePiece(target) == white_)) return false;
    const char type = lower(p);
    const bool promotion = type == 'p' && to / 8 == (white_ ? 7 : 0);
    if (promotion != bool(move.promotion) || (promotion && std::string_view("nbrq").find(move.promotion) == std::string_view::npos)) return false;
    const int dx = to % 8 - from % 8, dy = to / 8 - from / 8;
    const int ax = std::abs(dx), ay = std::abs(dy);
    if (type == 'p') {
        const int dir = white_ ? 1 : -1;
        if (ax == 1 && dy == dir) return target || (to == ep_ && squares_[to - 8 * dir] == (white_ ? 'p' : 'P'));
        if (dx || target) return false;
        if (dy == dir) return true;
        return dy == 2 * dir && from / 8 == (white_ ? 1 : 6) && !squares_[from + 8 * dir] && from + 8 * dir != duck_;
    }
    if (type == 'n') return ax * ay == 2;
    if (type == 'k') {
        if (std::max(ax, ay) == 1) return true;
        const int home = white_ ? 4 : 60;
        if (from != home || dy || ax != 2) return false;
        const bool kingside = dx > 0;
        const int right = (white_ ? 1 : 4) * (kingside ? 1 : 2);
        const int rook = home + (kingside ? 3 : -4);
        if (!(rights_ & right) || squares_[rook] != (white_ ? 'R' : 'r')) return false;
        const int step = kingside ? 1 : -1;
        for (int sq = home + step; sq != rook; sq += step) if (squares_[sq] || sq == duck_) return false;
        return true;
    }
    const bool diagonal = ax == ay;
    const bool straight = dx == 0 || dy == 0;
    if ((type == 'b' && !diagonal) || (type == 'r' && !straight) || (type == 'q' && !diagonal && !straight)) return false;
    const int step = (dx > 0 ? 1 : dx < 0 ? -1 : 0) + 8 * (dy > 0 ? 1 : dy < 0 ? -1 : 0);
    for (int sq = from + step; sq != to; sq += step) if (squares_[sq] || sq == duck_) return false;
    return true;
}

bool DuckPosition::kingCaptured(const DuckTurn& move) const { return lower(squares_[move.to]) == 'k'; }

void DuckPosition::applyPiece(const DuckTurn& move) {
    const char piece = squares_[move.from], target = squares_[move.to];
    const bool pawn = lower(piece) == 'p';
    const int dir = white_ ? 8 : -8;
    if (pawn && move.to == ep_ && !target && move.from % 8 != move.to % 8) squares_[move.to - dir] = 0;
    if (lower(piece) == 'k') {
        rights_ &= white_ ? ~3 : ~12;
        if (std::abs(move.to - move.from) == 2) {
            const int rook = move.from + (move.to > move.from ? 3 : -4);
            squares_[(move.from + move.to) / 2] = squares_[rook];
            squares_[rook] = 0;
        }
    }
    for (int i = 0; i < 4; ++i) {
        const int rook = std::array<int, 4>{7, 0, 63, 56}[i];
        if (move.from == rook || move.to == rook) rights_ &= ~(1 << i);
    }
    if (lower(target) == 'k') rights_ &= white_ ? ~12 : ~3;
    squares_[move.from] = 0;
    squares_[move.to] = move.promotion ? (white_ ? char(std::toupper(move.promotion)) : move.promotion) : piece;
    ep_ = pawn && std::abs(move.to - move.from) == 16 ? (move.to + move.from) / 2 : -1;
    halfmove_ = pawn || target ? 0 : halfmove_ + 1;
}

bool DuckPosition::isLegal(const DuckTurn& move) const {
    if (!pieceLegal(move)) return false;
    if (std::find(squares_.begin(), squares_.end(), 'K') == squares_.end() ||
        std::find(squares_.begin(), squares_.end(), 'k') == squares_.end()) return false;
    if (move.duck == -1) return kingCaptured(move);
    if (move.duck < 0 || move.duck >= 64 || move.duck == duck_) return false;
    if (move.duck == move.from) return true;
    if (move.duck == move.to) return false;
    if (lower(squares_[move.from]) == 'k' && std::abs(move.to - move.from) == 2) {
        if (move.duck == (move.from + move.to) / 2) return false;
        if (move.duck == move.from + (move.to > move.from ? 3 : -4)) return true;
    }
    if (lower(squares_[move.from]) == 'p' && move.to == ep_ && !squares_[move.to] &&
        move.from % 8 != move.to % 8 && move.duck == move.to + (white_ ? -8 : 8)) return true;
    return !squares_[move.duck];
}

bool DuckPosition::isLegal(std::string_view move) const {
    const auto parsed = parseTurn(move);
    return parsed && isLegal(*parsed);
}

void DuckPosition::play(std::string_view move) {
    const auto parsed = parseTurn(move);
    if (!parsed || !isLegal(*parsed)) throw std::invalid_argument("Illegal Duck Chess turn: " + std::string(move));
    applyPiece(*parsed);
    if (parsed->duck >= 0) duck_ = parsed->duck;
    fullmove_ += !white_;
    white_ = !white_;
    history_.push_back(repetitionKey());
}

std::string DuckPosition::repetitionKey() const {
    auto fen = getFen(false);
    bool possible_ep = false;
    if (ep_ >= 0 && ep_ != duck_) {
        for (int from = 0; from < 64; ++from) {
            if (lower(squares_[from]) == 'p' && pieceLegal({from, ep_, -1, 0})) { possible_ep = true; break; }
        }
    }
    if (!possible_ep) fen.replace(fen.find_last_of(' ') + 1, std::string::npos, "-");
    return fen;
}

bool DuckPosition::hasMove() const {
    for (int from = 0; from < 64; ++from) {
        if (!squares_[from] || whitePiece(squares_[from]) != white_) continue;
        for (int to = 0; to < 64; ++to) {
            DuckTurn move{from, to, -1, 0};
            if (lower(squares_[from]) == 'p' && to / 8 == (white_ ? 7 : 0)) move.promotion = 'q';
            if (pieceLegal(move)) return true;
        }
    }
    return false;
}

std::pair<chess::GameResultReason, chess::GameResult> DuckPosition::isGameOver() const {
    using Reason = chess::GameResultReason;
    using Result = chess::GameResult;
    if (std::find(squares_.begin(), squares_.end(), white_ ? 'K' : 'k') == squares_.end()) return {Reason::CHECKMATE, Result::LOSE};
    if (std::find(squares_.begin(), squares_.end(), white_ ? 'k' : 'K') == squares_.end()) return {Reason::CHECKMATE, Result::WIN};
    if (!hasMove()) return {Reason::STALEMATE, Result::WIN};
    if (halfmove_ >= 100) return {Reason::FIFTY_MOVE_RULE, Result::DRAW};
    if (std::count(history_.begin(), history_.end(), repetitionKey()) >= 3) return {Reason::THREEFOLD_REPETITION, Result::DRAW};
    return {Reason::NONE, Result::NONE};
}

std::string DuckPosition::san(const DuckTurn& move, bool long_notation) const {
    const char type = lower(squares_[move.from]);
    std::string result;
    if (type == 'k' && std::abs(move.from - move.to) == 2) {
        result = move.to > move.from ? "O-O" : "O-O-O";
    } else {
        const bool capture = squares_[move.to] || (type == 'p' && move.from % 8 != move.to % 8);
        if (type != 'p') result += char(std::toupper(type));
        if (long_notation) result += squareName(move.from);
        else if (type == 'p') { if (capture) result += char('a' + move.from % 8); }
        else {
            bool ambiguous = false, same_file = false, same_rank = false;
            for (int from = 0; from < 64; ++from) {
                if (from == move.from || squares_[from] != squares_[move.from]) continue;
                auto other = move;
                other.from = from;
                if (!pieceLegal(other)) continue;
                ambiguous = true;
                same_file |= from % 8 == move.from % 8;
                same_rank |= from / 8 == move.from / 8;
            }
            if (ambiguous) {
                if (!same_file || same_rank) result += char('a' + move.from % 8);
                if (same_file) result += char('1' + move.from / 8);
            }
        }
        if (capture) result += 'x';
        else if (long_notation) result += '-';
        result += squareName(move.to);
        if (move.promotion) { result += '='; result += char(std::toupper(move.promotion)); }
    }
    if (move.duck >= 0) result += "@" + squareName(move.duck);
    return result;
}

std::string DuckPosition::notation(std::string_view move, NotationType type) const {
    const auto parsed = parseTurn(move);
    if (!parsed || !isLegal(*parsed)) throw std::invalid_argument("Invalid Duck Chess move notation");
    return type == NotationType::UCI ? toUci(*parsed) : san(*parsed, type == NotationType::LAN);
}

std::string DuckPosition::parseNotation(std::string_view text) const {
    std::string move(text);
    while (!move.empty() && (move.back() == '!' || move.back() == '?')) move.pop_back();
    if (isLegal(move)) return move;
    std::replace(move.begin(), move.end(), '0', 'O');
    int duck = -1;
    const auto at = move.find('@');
    if (at != std::string::npos) {
        duck = square(std::string_view(move).substr(at + 1));
        if (duck < 0) throw std::invalid_argument("Invalid duck square in PGN");
    }
    std::optional<DuckTurn> found;
    for (int from = 0; from < 64; ++from) {
        if (!squares_[from] || whitePiece(squares_[from]) != white_) continue;
        for (int to = 0; to < 64; ++to) {
            const bool promotion = lower(squares_[from]) == 'p' && to / 8 == (white_ ? 7 : 0);
            for (char p : promotion ? std::string("nbrq") : std::string(1, '\0')) {
                DuckTurn turn{from, to, duck, p};
                if (!isLegal(turn)) continue;
                if (san(turn, false) != move && san(turn, true) != move) continue;
                if (found) throw std::invalid_argument("Ambiguous Duck Chess PGN move");
                found = turn;
            }
        }
    }
    if (!found) throw std::invalid_argument("Invalid Duck Chess PGN move: " + move);
    return toUci(*found);
}

bool Position::isLegal(std::string_view move) const {
    if (isDuck()) return duck_->isLegal(move);
    if (!chess::uci::isUciMove(move)) return false;
    const auto parsed = chess::uci::uciToMove(chess_, move);
    chess::Movelist moves;
    chess::movegen::legalmoves(moves, chess_);
    return std::find(moves.begin(), moves.end(), parsed) != moves.end();
}

void Position::play(std::string_view move) {
    if (isDuck()) duck_->play(move);
    else chess_.makeMove<true>(chess::uci::uciToMove(chess_, move));
}

std::string Position::notation(std::string_view move, NotationType type) const {
    if (isDuck()) return duck_->notation(move, type);
    const auto parsed = chess::uci::uciToMove(chess_, move);
    if (type == NotationType::SAN) return chess::uci::moveToSan(chess_, parsed);
    if (type == NotationType::LAN) return chess::uci::moveToLan(chess_, parsed);
    return std::string(move);
}

std::string Position::parseNotation(std::string_view move) const {
    if (isDuck()) return duck_->parseNotation(move);
    return chess::uci::moveToUci(chess::uci::parseSan(chess_, move), chess_.chess960());
}

}
