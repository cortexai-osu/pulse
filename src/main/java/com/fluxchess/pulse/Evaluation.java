/*
 * Copyright (C) 2013-2019 Phokham Nonava
 *
 * Use of this source code is governed by the MIT license that can be
 * found in the LICENSE file.
 */
package com.fluxchess.pulse;

import com.fluxchess.pulse.model.Piece;
import com.fluxchess.pulse.model.Square;

import static com.fluxchess.pulse.Bitboard.*;
import static com.fluxchess.pulse.model.Color.opposite;
import static com.fluxchess.pulse.model.Piece.NOPIECE;
import static com.fluxchess.pulse.model.PieceType.*;
import static com.fluxchess.pulse.model.Square.*;

final class Evaluation {

	static final int TEMPO = 1;

	private static final int MATERIAL_WEIGHT = 100;
	private static final int MOBILITY_WEIGHT = 80;
	private static final int MAX_WEIGHT = 100;

	/**
	 * Evaluates the position.
	 *
	 * @param position the position.
	 * @return the evaluation value in centipawns.
	 */
	int evaluate(Position position) {
		// Initialize
		int myColor = position.activeColor;
		int oppositeColor = opposite(myColor);
		int value = 0;

		// Evaluate material
		int materialScore = (evaluateMaterial(myColor, position) - evaluateMaterial(oppositeColor, position))
				* MATERIAL_WEIGHT / MAX_WEIGHT;
		value += materialScore;

		// Evaluate mobility
		int mobilityScore = (evaluateMobility(myColor, position) - evaluateMobility(oppositeColor, position))
				* MOBILITY_WEIGHT / MAX_WEIGHT;
		value += mobilityScore;

		// Add Tempo
		value += TEMPO;

		return value;
	}

	private int evaluateMaterial(int color, Position position) {
		int material = position.material[color];

		// Add bonus for bishop pair
		if (size(position.pieces[color][BISHOP]) >= 2) {
			material += 50;
		}

		return material;
	}

	private int evaluateMobility(int color, Position position) {
		int knightMobility = 0;
		for (long squares = position.pieces[color][KNIGHT]; squares != 0; squares = remainder(squares)) {
			int square = next(squares);
			knightMobility += evaluateMobility(position, square, knightDirections);
		}

		int bishopMobility = 0;
		for (long squares = position.pieces[color][BISHOP]; squares != 0; squares = remainder(squares)) {
			int square = next(squares);
			bishopMobility += evaluateMobility(position, square, bishopDirections);
		}

		int rookMobility = 0;
		for (long squares = position.pieces[color][ROOK]; squares != 0; squares = remainder(squares)) {
			int square = next(squares);
			rookMobility += evaluateMobility(position, square, rookDirections);
		}

		int queenMobility = 0;
		for (long squares = position.pieces[color][QUEEN]; squares != 0; squares = remainder(squares)) {
			int square = next(squares);
			queenMobility += evaluateMobility(position, square, queenDirections);
		}

		return knightMobility * 4
				+ bishopMobility * 5
				+ rookMobility * 2
				+ queenMobility;
	}

	private int evaluateMobility(Position position, int square, int[] directions) {
		int mobility = 0;
		boolean sliding = isSliding(Piece.getType(position.board[square]));

		for (int direction : directions) {
			int targetSquare = square + direction;

			while (Square.isValid(targetSquare)) {
				mobility++;

				if (sliding && position.board[targetSquare] == NOPIECE) {
					targetSquare += direction;
				} else {
					break;
				}
			}
		}

		return mobility;
	}
}
