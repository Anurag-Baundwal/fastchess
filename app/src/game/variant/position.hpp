#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <chess.hpp>
#include <types/enums.hpp>

namespace fastchess::rules {

struct DuckTurn {
    int from = -1;
    int to = -1;
    int duck = -1;
    char promotion = 0;
};

class DuckPosition {
   public:
    DuckPosition();
    bool setFen(std::string_view fen);
    bool setEpd(std::string_view epd);
    std::string getFen(bool counters = true) const;
    std::string getEpd() const;
    chess::Color sideToMove() const { return white_ ? chess::Color::WHITE : chess::Color::BLACK; }
    int fullMoveNumber() const { return fullmove_; }
    int halfMoveClock() const { return halfmove_; }
    static std::optional<DuckTurn> parseTurn(std::string_view move);
    static std::string toUci(const DuckTurn& move);
    bool isLegal(const DuckTurn& move) const;
    bool isLegal(std::string_view move) const;
    void play(std::string_view move);
    std::string notation(std::string_view move, NotationType type) const;
    std::string parseNotation(std::string_view move) const;
    std::pair<chess::GameResultReason, chess::GameResult> isGameOver() const;

   private:
    bool pieceLegal(const DuckTurn& move) const;
    void applyPiece(const DuckTurn& move);
    bool kingCaptured(const DuckTurn& move) const;
    bool hasMove() const;
    std::string repetitionKey() const;
    std::string san(const DuckTurn& move, bool long_notation) const;
    std::array<char, 64> squares_{};
    bool white_ = true;
    int duck_ = -1;
    int ep_ = -1;
    int rights_ = 0;
    int halfmove_ = 0;
    int fullmove_ = 1;
    std::vector<std::string> history_;
};

class Position {
   public:
    explicit Position(VariantType variant = VariantType::STANDARD) : variant_(variant) {
        chess_.set960(variant == VariantType::FRC);
        if (isDuck()) duck_.emplace();
    }
    bool isDuck() const { return variant_ == VariantType::DUCK; }
    bool setFen(std::string_view fen) { return isDuck() ? duck_->setFen(fen) : chess_.setFen(fen); }
    bool setEpd(std::string_view epd) { return isDuck() ? duck_->setEpd(epd) : chess_.setEpd(epd); }
    std::string getFen() const { return isDuck() ? duck_->getFen() : chess_.getFen(); }
    std::string getEpd() const { return isDuck() ? duck_->getEpd() : chess_.getEpd(); }
    chess::Color sideToMove() const { return isDuck() ? duck_->sideToMove() : chess_.sideToMove(); }
    int fullMoveNumber() const { return isDuck() ? duck_->fullMoveNumber() : chess_.fullMoveNumber(); }
    int halfMoveClock() const { return isDuck() ? duck_->halfMoveClock() : chess_.halfMoveClock(); }
    auto isGameOver() const { return isDuck() ? duck_->isGameOver() : chess_.isGameOver(); }
    const chess::Board& orthodox() const { return chess_; }
    chess::Board& orthodox() { return chess_; }
    bool isLegal(std::string_view move) const;
    void play(std::string_view move);
    std::string notation(std::string_view move, NotationType type) const;
    std::string parseNotation(std::string_view move) const;

   private:
    VariantType variant_;
    chess::Board chess_;
    std::optional<DuckPosition> duck_;
};

}
