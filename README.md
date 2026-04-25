SIGMA4⁴ is a chess-playing robot made with VEX robotics and C++.
Its software is a chess engine developed independently using Minimax, Tapered Piece-Square Table Evaluation, AlphaBeta Pruning, Null-Move Pruning, Zobrist Hashing Transposition Table, MVV-LVA, Repetition Check, Quiescence Search, and Dual Board.
The file ```chess_engine.cpp``` contains the entire code for the chess engine.
The file ```robot.cpp``` contains the code for the chess engine and the robot movement.

The chess engine can operate independently without the physical robot:
- To play on the console, press [Enter] after launching the .exe
- To play with the UCI protocol, input ```uci``` and press [Enter] after launching.
