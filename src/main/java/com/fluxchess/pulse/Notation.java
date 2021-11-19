/*
 * Copyright 2013-2021 Phokham Nonava
 *
 * Use of this source code is governed by the MIT license that can be
 * found in the LICENSE file.
 */
package com.fluxchess.pulse;

import com.fluxchess.jcpi.models.*;
import com.fluxchess.pulse.model.Castling;
import com.fluxchess.pulse.model.CastlingType;
import com.fluxchess.pulse.model.Color;
import com.fluxchess.pulse.model.Square;

import static com.fluxchess.pulse.model.Castling.*;
import static com.fluxchess.pulse.model.CastlingType.*;
import static com.fluxchess.pulse.model.Color.*;
import static com.fluxchess.pulse.model.File.*;
import static com.fluxchess.pulse.model.Piece.*;
import static com.fluxchess.pulse.model.PieceType.*;
import static com.fluxchess.pulse.model.Rank.*;
import static com.fluxchess.pulse.model.Square.NOSQUARE;

final class Notation {

	static final String STANDARDPOSITION = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

	private Notation() {
	}

	static Position toPosition(String fen) {
		try {
			return toPosition(new GenericBoard(fen));
		} catch (IllegalNotationException e) {
			throw new IllegalArgumentException(e);
		}
	}

	static String fromPosition(Position position) {
		return toGenericBoard(position).toString();
	}

	static Position toPosition(GenericBoard genericBoard) {
		Position newPosition = new Position();

		// Initialize board
		for (int square : Square.values) {
			GenericPiece genericPiece = genericBoard.getPiece(fromSquare(square));
			if (genericPiece != null) {
				int piece = toPiece(genericPiece);
				newPosition.put(piece, square);
			}
		}

		// Initialize active color
		newPosition.setActiveColor(toColor(genericBoard.getActiveColor()));

		// Initialize castling
		for (int color : Color.values) {
			for (int castlingtype : CastlingType.values) {
				GenericFile genericFile = genericBoard.getCastling(
						fromColor(color), fromCastlingType(castlingtype)
				);
				if (genericFile != null) {
					newPosition.setCastlingRight(Castling.valueOf(color, castlingtype));
				}
			}
		}

		// Initialize en passant
		if (genericBoard.getEnPassant() != null) {
			newPosition.setEnPassantSquare(toSquare(genericBoard.getEnPassant()));
		}

		// Initialize half move clock
		newPosition.setHalfmoveClock(genericBoard.getHalfMoveClock());

		// Initialize the full move number
		newPosition.setFullmoveNumber(genericBoard.getFullMoveNumber());

		return newPosition;
	}

	static GenericBoard toGenericBoard(Position position) {
		GenericBoard genericBoard = new GenericBoard();

		// Set board
		for (int square : Square.values) {
			if (position.board[square] != NOPIECE) {
				genericBoard.setPiece(fromPiece(position.board[square]), fromSquare(square));
			}
		}

		// Set castling
		if ((position.castlingRights & WHITE_KINGSIDE) != NOCASTLING) {
			genericBoard.setCastling(
					fromColor(WHITE), fromCastlingType(KINGSIDE),
					fromFile(h)
			);
		}
		if ((position.castlingRights & WHITE_QUEENSIDE) != NOCASTLING) {
			genericBoard.setCastling(
					fromColor(WHITE), fromCastlingType(QUEENSIDE),
					fromFile(a)
			);
		}
		if ((position.castlingRights & BLACK_KINGSIDE) != NOCASTLING) {
			genericBoard.setCastling(
					fromColor(BLACK), fromCastlingType(KINGSIDE),
					fromFile(h)
			);
		}
		if ((position.castlingRights & BLACK_QUEENSIDE) != NOCASTLING) {
			genericBoard.setCastling(
					fromColor(BLACK), fromCastlingType(QUEENSIDE),
					fromFile(a)
			);
		}

		// Set en passant
		if (position.enPassantSquare != NOSQUARE) {
			genericBoard.setEnPassant(fromSquare(position.enPassantSquare));
		}

		// Set active color
		genericBoard.setActiveColor(fromColor(position.activeColor));

		// Set half move clock
		genericBoard.setHalfMoveClock(position.halfmoveClock);

		// Set full move number
		genericBoard.setFullMoveNumber(position.getFullmoveNumber());

		return genericBoard;
	}

	static int toColor(GenericColor genericColor) {
		switch (genericColor) {
			case WHITE:
				return WHITE;
			case BLACK:
				return BLACK;
			default:
				throw new IllegalArgumentException();
		}
	}

	static GenericColor fromColor(int color) {
		switch (color) {
			case WHITE:
				return GenericColor.WHITE;
			case BLACK:
				return GenericColor.BLACK;
			case NOCOLOR:
			default:
				throw new IllegalArgumentException();
		}
	}

	static GenericChessman fromPieceType(int piecetype) {
		switch (piecetype) {
			case PAWN:
				return GenericChessman.PAWN;
			case KNIGHT:
				return GenericChessman.KNIGHT;
			case BISHOP:
				return GenericChessman.BISHOP;
			case ROOK:
				return GenericChessman.ROOK;
			case QUEEN:
				return GenericChessman.QUEEN;
			case KING:
				return GenericChessman.KING;
			case NOPIECETYPE:
			default:
				throw new IllegalArgumentException();
		}
	}

	static int toPiece(GenericPiece genericPiece) {
		switch (genericPiece) {
			case WHITEPAWN:
				return WHITE_PAWN;
			case WHITEKNIGHT:
				return WHITE_KNIGHT;
			case WHITEBISHOP:
				return WHITE_BISHOP;
			case WHITEROOK:
				return WHITE_ROOK;
			case WHITEQUEEN:
				return WHITE_QUEEN;
			case WHITEKING:
				return WHITE_KING;
			case BLACKPAWN:
				return BLACK_PAWN;
			case BLACKKNIGHT:
				return BLACK_KNIGHT;
			case BLACKBISHOP:
				return BLACK_BISHOP;
			case BLACKROOK:
				return BLACK_ROOK;
			case BLACKQUEEN:
				return BLACK_QUEEN;
			case BLACKKING:
				return BLACK_KING;
			default:
				throw new IllegalArgumentException();
		}
	}

	static GenericPiece fromPiece(int piece) {
		switch (piece) {
			case WHITE_PAWN:
				return GenericPiece.WHITEPAWN;
			case WHITE_KNIGHT:
				return GenericPiece.WHITEKNIGHT;
			case WHITE_BISHOP:
				return GenericPiece.WHITEBISHOP;
			case WHITE_ROOK:
				return GenericPiece.WHITEROOK;
			case WHITE_QUEEN:
				return GenericPiece.WHITEQUEEN;
			case WHITE_KING:
				return GenericPiece.WHITEKING;
			case BLACK_PAWN:
				return GenericPiece.BLACKPAWN;
			case BLACK_KNIGHT:
				return GenericPiece.BLACKKNIGHT;
			case BLACK_BISHOP:
				return GenericPiece.BLACKBISHOP;
			case BLACK_ROOK:
				return GenericPiece.BLACKROOK;
			case BLACK_QUEEN:
				return GenericPiece.BLACKQUEEN;
			case BLACK_KING:
				return GenericPiece.BLACKKING;
			case NOPIECE:
			default:
				throw new IllegalArgumentException();
		}
	}

	static GenericCastling fromCastlingType(int castlingtype) {
		switch (castlingtype) {
			case KINGSIDE:
				return GenericCastling.KINGSIDE;
			case QUEENSIDE:
				return GenericCastling.QUEENSIDE;
			case NOCASTLINGTYPE:
			default:
				throw new IllegalArgumentException();
		}
	}

	static int toFile(GenericFile genericFile) {
		switch (genericFile) {
			case Fa:
				return a;
			case Fb:
				return b;
			case Fc:
				return c;
			case Fd:
				return d;
			case Fe:
				return e;
			case Ff:
				return f;
			case Fg:
				return g;
			case Fh:
				return h;
			default:
				throw new IllegalArgumentException();
		}
	}

	static GenericFile fromFile(int file) {
		switch (file) {
			case a:
				return GenericFile.Fa;
			case b:
				return GenericFile.Fb;
			case c:
				return GenericFile.Fc;
			case d:
				return GenericFile.Fd;
			case e:
				return GenericFile.Fe;
			case f:
				return GenericFile.Ff;
			case g:
				return GenericFile.Fg;
			case h:
				return GenericFile.Fh;
			case NOFILE:
			default:
				throw new IllegalArgumentException();
		}
	}

	static int toRank(GenericRank genericRank) {
		switch (genericRank) {
			case R1:
				return r1;
			case R2:
				return r2;
			case R3:
				return r3;
			case R4:
				return r4;
			case R5:
				return r5;
			case R6:
				return r6;
			case R7:
				return r7;
			case R8:
				return r8;
			default:
				throw new IllegalArgumentException();
		}
	}

	static GenericRank fromRank(int rank) {
		switch (rank) {
			case r1:
				return GenericRank.R1;
			case r2:
				return GenericRank.R2;
			case r3:
				return GenericRank.R3;
			case r4:
				return GenericRank.R4;
			case r5:
				return GenericRank.R5;
			case r6:
				return GenericRank.R6;
			case r7:
				return GenericRank.R7;
			case r8:
				return GenericRank.R8;
			case NORANK:
			default:
				throw new IllegalArgumentException();
		}
	}

	static int toSquare(GenericPosition genericPosition) {
		return toRank(genericPosition.rank) * 16 + toFile(genericPosition.file);
	}

	static GenericPosition fromSquare(int square) {
		return GenericPosition.valueOf(fromFile(Square.getFile(square)), fromRank(Square.getRank(square)));
	}
}
