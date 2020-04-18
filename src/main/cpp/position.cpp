/*
 * Copyright (C) 2013-2019 Phokham Nonava
 *
 * Use of this source code is governed by the MIT license that can be
 * found in the LICENSE file.
 */

#include "position.h"
#include "move.h"

#include <algorithm>
#include <sstream>

namespace pulse {

// Initialize the zobrist keys
Position::Zobrist::Zobrist() {
	for (auto piece : piece::values) {
		for (int i = 0; i < Square::VALUES_LENGTH; i++) {
			board[piece][i] = next();
		}
	}

	castlingRights[castling::WHITE_KINGSIDE] = next();
	castlingRights[castling::WHITE_QUEENSIDE] = next();
	castlingRights[castling::BLACK_KINGSIDE] = next();
	castlingRights[castling::BLACK_QUEENSIDE] = next();
	castlingRights[castling::WHITE_KINGSIDE | castling::WHITE_QUEENSIDE] =
			castlingRights[castling::WHITE_KINGSIDE] ^ castlingRights[castling::WHITE_QUEENSIDE];
	castlingRights[castling::BLACK_KINGSIDE | castling::BLACK_QUEENSIDE] =
			castlingRights[castling::BLACK_KINGSIDE] ^ castlingRights[castling::BLACK_QUEENSIDE];

	for (int i = 0; i < Square::VALUES_LENGTH; i++) {
		enPassantSquare[i] = next();
	}

	activeColor = next();
}

Position::Zobrist& Position::Zobrist::instance() {
	static Zobrist* instance = new Zobrist();
	return *instance;
}

uint64_t Position::Zobrist::next() {
	std::array<uint64_t, 16> bytes;
	for (int i = 0; i < 16; i++) {
		bytes[i] = generator();
	}

	uint64_t hash = 0;
	for (int i = 0; i < 16; i++) {
		hash ^= bytes[i] << ((i * 8) % 64);
	}

	return hash;
}

Position::Position()
		: zobrist(Zobrist::instance()) {
	board.fill(+piece::NOPIECE);
}

Position::Position(const Position& position)
		: Position() {
	this->board = position.board;
	this->pieces = position.pieces;

	this->material = position.material;

	this->castlingRights = position.castlingRights;
	this->enPassantSquare = position.enPassantSquare;
	this->activeColor = position.activeColor;
	this->halfmoveClock = position.halfmoveClock;

	this->zobristKey = position.zobristKey;

	this->halfmoveNumber = position.halfmoveNumber;

	this->statesSize = 0;
}

Position& Position::operator=(const Position& position) {
	this->board = position.board;
	this->pieces = position.pieces;

	this->material = position.material;

	this->castlingRights = position.castlingRights;
	this->enPassantSquare = position.enPassantSquare;
	this->activeColor = position.activeColor;
	this->halfmoveClock = position.halfmoveClock;

	this->zobristKey = position.zobristKey;

	this->halfmoveNumber = position.halfmoveNumber;

	this->statesSize = 0;

	return *this;
}

bool Position::operator==(const Position& position) const {
	return this->board == position.board
		   && this->pieces == position.pieces

		   && this->material == position.material

		   && this->castlingRights == position.castlingRights
		   && this->enPassantSquare == position.enPassantSquare
		   && this->activeColor == position.activeColor
		   && this->halfmoveClock == position.halfmoveClock

		   && this->zobristKey == position.zobristKey

		   && this->halfmoveNumber == position.halfmoveNumber;
}

bool Position::operator!=(const Position& position) const {
	return !(*this == position);
}

void Position::setActiveColor(int activeColor) {
	if (this->activeColor != activeColor) {
		this->activeColor = activeColor;
		zobristKey ^= zobrist.activeColor;
	}
}

void Position::setCastlingRight(int castling) {
	if ((castlingRights & castling) == castling::NOCASTLING) {
		castlingRights |= castling;
		zobristKey ^= zobrist.castlingRights[castling];
	}
}

void Position::setEnPassantSquare(int enPassantSquare) {
	if (this->enPassantSquare != Square::NOSQUARE) {
		zobristKey ^= zobrist.enPassantSquare[this->enPassantSquare];
	}
	if (enPassantSquare != Square::NOSQUARE) {
		zobristKey ^= zobrist.enPassantSquare[enPassantSquare];
	}
	this->enPassantSquare = enPassantSquare;
}

void Position::setHalfmoveClock(int halfmoveClock) {
	this->halfmoveClock = halfmoveClock;
}

int Position::getFullmoveNumber() const {
	return halfmoveNumber / 2;
}

void Position::setFullmoveNumber(int fullmoveNumber) {
	halfmoveNumber = fullmoveNumber * 2;
	if (activeColor == color::BLACK) {
		halfmoveNumber++;
	}
}

bool Position::isRepetition() {
	// Search back until the last halfmoveClock reset
	int j = std::max(0, statesSize - halfmoveClock);
	for (int i = statesSize - 2; i >= j; i -= 2) {
		if (zobristKey == states[i].zobristKey) {
			return true;
		}
	}

	return false;
}

bool Position::hasInsufficientMaterial() {
	// If there is only one minor left, we are unable to checkmate
	return bitboard::size(pieces[color::WHITE][PieceType::PAWN]) == 0
		   && bitboard::size(pieces[color::BLACK][PieceType::PAWN]) == 0
		   && bitboard::size(pieces[color::WHITE][PieceType::ROOK]) == 0
		   && bitboard::size(pieces[color::BLACK][PieceType::ROOK]) == 0
		   && bitboard::size(pieces[color::WHITE][PieceType::QUEEN]) == 0
		   && bitboard::size(pieces[color::BLACK][PieceType::QUEEN]) == 0
		   && (bitboard::size(pieces[color::WHITE][PieceType::KNIGHT]) +
			   bitboard::size(pieces[color::WHITE][PieceType::BISHOP]) <= 1)
		   && (bitboard::size(pieces[color::BLACK][PieceType::KNIGHT]) +
			   bitboard::size(pieces[color::BLACK][PieceType::BISHOP]) <= 1);
}

/**
 * Puts a piece at the square. We need to update our board and the appropriate
 * piece type list.
 *
 * @param piece  the Piece.
 * @param square the Square.
 */
void Position::put(int piece, int square) {
	int piecetype = piece::getType(piece);
	int color = piece::getColor(piece);

	board[square] = piece;
	pieces[color][piecetype] = bitboard::add(square, pieces[color][piecetype]);
	material[color] += PieceType::getValue(piecetype);

	zobristKey ^= zobrist.board[piece][square];
}

/**
 * Removes a piece from the square. We need to update our board and the
 * appropriate piece type list.
 *
 * @param square the Square.
 * @return the Piece which was removed.
 */
int Position::remove(int square) {
	int piece = board[square];

	int piecetype = piece::getType(piece);
	int color = piece::getColor(piece);

	board[square] = piece::NOPIECE;
	pieces[color][piecetype] = bitboard::remove(square, pieces[color][piecetype]);
	material[color] -= PieceType::getValue(piecetype);

	zobristKey ^= zobrist.board[piece][square];

	return piece;
}

void Position::makeMove(int move) {
	// Save state
	State& entry = states[statesSize];
	entry.zobristKey = zobristKey;
	entry.castlingRights = castlingRights;
	entry.enPassantSquare = enPassantSquare;
	entry.halfmoveClock = halfmoveClock;

	statesSize++;

	// Get variables
	int type = move::getType(move);
	int originSquare = move::getOriginSquare(move);
	int targetSquare = move::getTargetSquare(move);
	int originPiece = move::getOriginPiece(move);
	int originColor = piece::getColor(originPiece);
	int targetPiece = move::getTargetPiece(move);

	// Remove target piece and update castling rights
	if (targetPiece != piece::NOPIECE) {
		int captureSquare = targetSquare;
		if (type == movetype::ENPASSANT) {
			captureSquare += (originColor == color::WHITE ? Square::S : Square::N);
		}
		remove(captureSquare);

		clearCastling(captureSquare);
	}

	// Move piece
	remove(originSquare);
	if (type == movetype::PAWNPROMOTION) {
		put(piece::valueOf(originColor, move::getPromotion(move)), targetSquare);
	} else {
		put(originPiece, targetSquare);
	}

	// Move rook and update castling rights
	if (type == movetype::CASTLING) {
		int rookOriginSquare;
		int rookTargetSquare;
		switch (targetSquare) {
			case Square::g1:
				rookOriginSquare = Square::h1;
				rookTargetSquare = Square::f1;
				break;
			case Square::c1:
				rookOriginSquare = Square::a1;
				rookTargetSquare = Square::d1;
				break;
			case Square::g8:
				rookOriginSquare = Square::h8;
				rookTargetSquare = Square::f8;
				break;
			case Square::c8:
				rookOriginSquare = Square::a8;
				rookTargetSquare = Square::d8;
				break;
			default:
				throw std::exception();
		}

		int rookPiece = remove(rookOriginSquare);
		put(rookPiece, rookTargetSquare);
	}

	// Update castling
	clearCastling(originSquare);

	// Update enPassantSquare
	if (enPassantSquare != Square::NOSQUARE) {
		zobristKey ^= zobrist.enPassantSquare[enPassantSquare];
	}
	if (type == movetype::PAWNDOUBLE) {
		enPassantSquare = targetSquare + (originColor == color::WHITE ? Square::S : Square::N);
		zobristKey ^= zobrist.enPassantSquare[enPassantSquare];
	} else {
		enPassantSquare = Square::NOSQUARE;
	}

	// Update activeColor
	activeColor = color::opposite(activeColor);
	zobristKey ^= zobrist.activeColor;

	// Update halfmoveClock
	if (piece::getType(originPiece) == PieceType::PAWN || targetPiece != piece::NOPIECE) {
		halfmoveClock = 0;
	} else {
		halfmoveClock++;
	}

	// Update fullMoveNumber
	halfmoveNumber++;
}

void Position::undoMove(int move) {
	// Get variables
	int type = move::getType(move);
	int originSquare = move::getOriginSquare(move);
	int targetSquare = move::getTargetSquare(move);
	int originPiece = move::getOriginPiece(move);
	int originColor = piece::getColor(originPiece);
	int targetPiece = move::getTargetPiece(move);

	// Update fullMoveNumber
	halfmoveNumber--;

	// Update activeColor
	activeColor = color::opposite(activeColor);

	// Undo move rook
	if (type == movetype::CASTLING) {
		int rookOriginSquare;
		int rookTargetSquare;
		switch (targetSquare) {
			case Square::g1:
				rookOriginSquare = Square::h1;
				rookTargetSquare = Square::f1;
				break;
			case Square::c1:
				rookOriginSquare = Square::a1;
				rookTargetSquare = Square::d1;
				break;
			case Square::g8:
				rookOriginSquare = Square::h8;
				rookTargetSquare = Square::f8;
				break;
			case Square::c8:
				rookOriginSquare = Square::a8;
				rookTargetSquare = Square::d8;
				break;
			default:
				throw std::exception();
		}

		int rookPiece = remove(rookTargetSquare);
		put(rookPiece, rookOriginSquare);
	}

	// Undo move piece
	remove(targetSquare);
	put(originPiece, originSquare);

	// Restore target piece
	if (targetPiece != piece::NOPIECE) {
		int captureSquare = targetSquare;
		if (type == movetype::ENPASSANT) {
			captureSquare += (originColor == color::WHITE ? Square::S : Square::N);
		}
		put(targetPiece, captureSquare);
	}

	// Restore state
	statesSize--;

	State& entry = states[statesSize];
	halfmoveClock = entry.halfmoveClock;
	enPassantSquare = entry.enPassantSquare;
	castlingRights = entry.castlingRights;
	zobristKey = entry.zobristKey;
}

void Position::clearCastling(int square) {
	int newCastlingRights = castlingRights;

	switch (square) {
		case Square::a1:
			newCastlingRights &= ~castling::WHITE_QUEENSIDE;
			break;
		case Square::a8:
			newCastlingRights &= ~castling::BLACK_QUEENSIDE;
			break;
		case Square::h1:
			newCastlingRights &= ~castling::WHITE_KINGSIDE;
			break;
		case Square::h8:
			newCastlingRights &= ~castling::BLACK_KINGSIDE;
			break;
		case Square::e1:
			newCastlingRights &= ~(castling::WHITE_KINGSIDE | castling::WHITE_QUEENSIDE);
			break;
		case Square::e8:
			newCastlingRights &= ~(castling::BLACK_KINGSIDE | castling::BLACK_QUEENSIDE);
			break;
		default:
			return;
	}

	if (newCastlingRights != castlingRights) {
		castlingRights = newCastlingRights;
		zobristKey ^= zobrist.castlingRights[newCastlingRights ^ castlingRights];
	}
}

bool Position::isCheck() {
	// Check whether our king is attacked by any opponent piece
	return isAttacked(bitboard::next(pieces[activeColor][PieceType::KING]), color::opposite(activeColor));
}

bool Position::isCheck(int color) {
	// Check whether the king for color is attacked by any opponent piece
	return isAttacked(bitboard::next(pieces[color][PieceType::KING]), color::opposite(color));
}

/**
 * Returns whether the targetSquare is attacked by any piece from the
 * attackerColor. We will backtrack from the targetSquare to find the piece.
 *
 * @param targetSquare  the target Square.
 * @param attackerColor the attacker Color.
 * @return whether the targetSquare is attacked.
 */
bool Position::isAttacked(int targetSquare, int attackerColor) {
	// Pawn attacks
	int pawnPiece = piece::valueOf(attackerColor, PieceType::PAWN);
	for (unsigned int i = 1; i < Square::pawnDirections[attackerColor].size(); i++) {
		int attackerSquare = targetSquare - Square::pawnDirections[attackerColor][i];
		if (Square::isValid(attackerSquare)) {
			int attackerPawn = board[attackerSquare];

			if (attackerPawn == pawnPiece) {
				return true;
			}
		}
	}

	return isAttacked(targetSquare,
			piece::valueOf(attackerColor, PieceType::KNIGHT),
			Square::knightDirections)

		   // The queen moves like a bishop, so check both piece types
		   || isAttacked(targetSquare,
			piece::valueOf(attackerColor, PieceType::BISHOP),
			piece::valueOf(attackerColor, PieceType::QUEEN),
			Square::bishopDirections)

		   // The queen moves like a rook, so check both piece types
		   || isAttacked(targetSquare,
			piece::valueOf(attackerColor, PieceType::ROOK),
			piece::valueOf(attackerColor, PieceType::QUEEN),
			Square::rookDirections)

		   || isAttacked(targetSquare,
			piece::valueOf(attackerColor, PieceType::KING),
			Square::kingDirections);
}

/**
 * Returns whether the targetSquare is attacked by a non-sliding piece.
 */
bool Position::isAttacked(int targetSquare, int attackerPiece, const std::vector<int>& directions) {
	for (auto direction : directions) {
		int attackerSquare = targetSquare + direction;

		if (Square::isValid(attackerSquare) && board[attackerSquare] == attackerPiece) {
			return true;
		}
	}

	return false;
}

/**
 * Returns whether the targetSquare is attacked by a sliding piece.
 */
bool Position::isAttacked(int targetSquare, int attackerPiece, int queenPiece, const std::vector<int>& directions) {
	for (auto direction : directions) {
		int attackerSquare = targetSquare + direction;

		while (Square::isValid(attackerSquare)) {
			int piece = board[attackerSquare];

			if (piece::isValid(piece)) {
				if (piece == attackerPiece || piece == queenPiece) {
					return true;
				}

				break;
			} else {
				attackerSquare += direction;
			}
		}
	}

	return false;
}
}
