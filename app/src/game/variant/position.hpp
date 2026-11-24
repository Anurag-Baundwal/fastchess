#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <chess.hpp>
#include <types/enums.hpp>

namespace fastchess::rules {

struct VariantMove {
    int from = -1;
    int to = -1;
    int duck = -1;
    char promotion = 0;
};

class KingCapturePosition {
   public:
    explicit KingCapturePosition(VariantType variant);
    bool setSecretQueen(chess::Color side, std::string_view square);
    bool setFen(std::string_view fen);
    bool setEpd(std::string_view epd);
    std::string getFen(bool counters = true) const;
    std::string getEpd() const;
    chess::Color sideToMove() const { return white_ ? chess::Color::WHITE : chess::Color::BLACK; }
    int fullMoveNumber() const { return fullmove_; }
    int halfMoveClock() const { return halfmove_; }
    static std::optional<VariantMove> parseTurn(std::string_view move);
    static std::string toUci(const VariantMove& move);
    bool isLegal(const VariantMove& move) const;
    bool isLegal(std::string_view move) const;
    void play(std::string_view move);
    std::string notation(std::string_view move, NotationType type) const;
    std::string parseNotation(std::string_view move) const;
    std::pair<chess::GameResultReason, chess::GameResult> isGameOver() const;

   private:
    int castlingIndex(const VariantMove& move) const;
    bool pieceLegal(const VariantMove& move) const;
    bool pawnMove(const VariantMove& move) const;
    void applyPiece(const VariantMove& move);
    bool kingCaptured(const VariantMove& move) const;
    bool hasMove() const;
    std::string repetitionKey() const;
    std::string san(const VariantMove& move, bool long_notation) const;
    std::array<char, 64> squares_{};
    bool white_ = true;
    int duck_ = -1;
    int ep_ = -1;
    int rights_ = 0;
    std::array<int, 4> rooks_{7, 0, 63, 56};
    bool chess960_ = false;
    bool secret_queen_ = false;
    std::array<int, 2> queens_{-1, -1};
    int halfmove_ = 0;
    int fullmove_ = 1;
    std::vector<std::string> history_;
};

class Position {
   public:
    explicit Position(VariantType variant = VariantType::STANDARD) : variant_(variant) {
        chess_.set960(variant == VariantType::FRC);
        if (isKingCapture()) capture_.emplace(variant);
    }
    bool isDuck() const { return variant_ == VariantType::DUCK || variant_ == VariantType::DUCK960; }
    bool isSecretQueen() const { return variant_ == VariantType::SECRETQUEEN; }
    bool isKingCapture() const { return isDuck() || isSecretQueen(); }
    bool setSecretQueen(chess::Color side, std::string_view square) {
        return isSecretQueen() && capture_->setSecretQueen(side, square);
    }
    bool setFen(std::string_view fen) { return isKingCapture() ? capture_->setFen(fen) : chess_.setFen(fen); }
    bool setEpd(std::string_view epd) { return isKingCapture() ? capture_->setEpd(epd) : chess_.setEpd(epd); }
    std::string getFen() const { return isKingCapture() ? capture_->getFen() : chess_.getFen(); }
    std::string getEpd() const { return isKingCapture() ? capture_->getEpd() : chess_.getEpd(); }
    chess::Color sideToMove() const { return isKingCapture() ? capture_->sideToMove() : chess_.sideToMove(); }
    int fullMoveNumber() const { return isKingCapture() ? capture_->fullMoveNumber() : chess_.fullMoveNumber(); }
    int halfMoveClock() const { return isKingCapture() ? capture_->halfMoveClock() : chess_.halfMoveClock(); }
    auto isGameOver() const { return isKingCapture() ? capture_->isGameOver() : chess_.isGameOver(); }
    const chess::Board& orthodox() const { return chess_; }
    chess::Board& orthodox() { return chess_; }
    bool isLegal(std::string_view move) const;
    void play(std::string_view move);
    std::string notation(std::string_view move, NotationType type) const;
    std::string parseNotation(std::string_view move) const;

   private:
    VariantType variant_;
    chess::Board chess_;
    std::optional<KingCapturePosition> capture_;
};

}
