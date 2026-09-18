#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

constexpr int EMPTY = 0;
constexpr int WP = 1, WN = 2, WB = 3, WR = 4, WQ = 5, WK = 6;
constexpr int BP = -1, BN = -2, BB = -3, BR = -4, BQ = -5, BK = -6;
constexpr int INF = 1000000, MATE = 900000;

struct Move {
    int from = -1, to = -1, promotion = 0;
    bool enPassant = false, castle = false;
};

struct Board {
    array<int, 128> squares{};
    bool white = true;
    int castling = 15; // white KQ, black kq
    int ep = -1;
    int halfmove = 0, fullmove = 1;

    static bool validStep(int from, int to, int delta) {
        if (to < 0 || to >= 128 || (to & 8)) return false;
        int fileDistance = abs((to & 7) - (from & 7));
        int rankDistance = abs((to >> 4) - (from >> 4));
        if (abs(delta) == 1) return rankDistance == 0 && fileDistance == 1;
        if (abs(delta) == 16) return fileDistance == 0 && rankDistance == 1;
        if (abs(delta) == 15 || abs(delta) == 17) return fileDistance == 1 && rankDistance == 1;
        return (fileDistance == 1 && rankDistance == 2) || (fileDistance == 2 && rankDistance == 1);
    }

    static bool validRay(int from, int to, int delta) {
        if (to < 0 || to >= 128 || (to & 8)) return false;
        int fileDistance = abs((to & 7) - (from & 7));
        int rankDistance = abs((to >> 4) - (from >> 4));
        if (abs(delta) == 1) return rankDistance == 0;
        if (abs(delta) == 16) return fileDistance == 0;
        return fileDistance == rankDistance;
    }

    void start() {
        setFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    }

    bool setFen(const string& fen) {
        squares.fill(EMPTY);
        istringstream in(fen);
        string placement, side, rights, epSquare;
        if (!(in >> placement >> side >> rights >> epSquare >> halfmove >> fullmove)) return false;
        int rank = 7, file = 0;
        for (char c : placement) {
            if (c == '/') { --rank; file = 0; continue; }
            if (isdigit(static_cast<unsigned char>(c))) { file += c - '0'; continue; }
            if (rank < 0 || file > 7) return false;
            squares[rank * 16 + file++] = pieceFromChar(c);
        }
        white = side == "w";
        castling = 0;
        if (rights.find('K') != string::npos) castling |= 1;
        if (rights.find('Q') != string::npos) castling |= 2;
        if (rights.find('k') != string::npos) castling |= 4;
        if (rights.find('q') != string::npos) castling |= 8;
        ep = epSquare == "-" ? -1 : parseSquare(epSquare);
        return true;
    }

    int kingSquare(bool side) const {
        int king = side ? WK : BK;
        for (int sq = 0; sq < 128; ++sq)
            if (!(sq & 8) && squares[sq] == king) return sq;
        return -1;
    }

    bool attacked(int sq, bool byWhite) const {
        const int pawn = byWhite ? WP : BP;
        int pawnRank = byWhite ? sq - 16 : sq + 16;
        if (pawnRank >= 0 && !(pawnRank & 8) && ((validStep(sq, pawnRank - 1, -17) && squares[pawnRank - 1] == pawn) || (validStep(sq, pawnRank + 1, -15) && squares[pawnRank + 1] == pawn))) return true;
        const int knight = byWhite ? WN : BN;
        for (int d : {-33, -31, -18, -14, 14, 18, 31, 33}) {
            int t = sq + d; if (validStep(sq, t, d) && squares[t] == knight) return true;
        }
        const int bishop = byWhite ? WB : BB, rook = byWhite ? WR : BR, queen = byWhite ? WQ : BQ;
        for (int d : {-17, -15, 15, 17}) {
            for (int t = sq + d; validRay(sq, t, d); t += d) { if (squares[t]) { if (squares[t] == bishop || squares[t] == queen) return true; break; } }
        }
        for (int d : {-16, -1, 1, 16}) {
            for (int t = sq + d; validRay(sq, t, d); t += d) { if (squares[t]) { if (squares[t] == rook || squares[t] == queen) return true; break; } }
        }
        const int king = byWhite ? WK : BK;
        for (int d : {-17, -16, -15, -1, 1, 15, 16, 17}) {
            int t = sq + d; if (validStep(sq, t, d) && squares[t] == king) return true;
        }
        return false;
    }

    bool inCheck(bool side) const {
        int king = kingSquare(side);
        return king >= 0 && attacked(king, !side);
    }

    string positionKey() const {
        string key;
        key.reserve(140);
        for (int sq = 0; sq < 128; ++sq) if (!(sq & 8)) key += char(squares[sq] + 7);
        key += white ? 'w' : 'b';
        key += char(castling);
        key += char(ep + 1);
        return key;
    }

    void addPawnMoves(vector<Move>& moves, int from) const {
        bool side = squares[from] > 0;
        int dir = side ? 16 : -16, rank = from >> 4, to = from + dir;
        if (validStep(from, to, dir) && squares[to] == EMPTY) {
            addPawnMove(moves, from, to, side);
            int startRank = side ? 1 : 6, jump = from + 2 * dir;
            if (rank == startRank && squares[jump] == EMPTY) moves.push_back({from, jump});
        }
        for (int delta : {dir - 1, dir + 1}) {
            to = from + delta;
            if (!validStep(from, to, delta)) continue;
            if (squares[to] != EMPTY && ((squares[to] > 0) != side)) addPawnMove(moves, from, to, side);
            if (to == ep) { Move m{from, to}; m.enPassant = true; moves.push_back(m); }
        }
    }

    void addPawnMove(vector<Move>& moves, int from, int to, bool side) const {
        int rank = to >> 4;
        if (rank == (side ? 7 : 0)) for (int p : {side ? WQ : BQ, side ? WR : BR, side ? WB : BB, side ? WN : BN}) moves.push_back({from, to, p});
        else moves.push_back({from, to});
    }

    vector<Move> pseudoMoves() const {
        vector<Move> moves;
        for (int from = 0; from < 128; ++from) {
            if (from & 8) { ++from; continue; }
            int piece = squares[from];
            if (!piece || (piece > 0) != white) continue;
            int type = abs(piece);
            if (type == 1) { addPawnMoves(moves, from); continue; }
            if (type == 2) {
                for (int d : {-33, -31, -18, -14, 14, 18, 31, 33}) { int to = from + d; if (validStep(from, to, d) && (!squares[to] || (squares[to] > 0) != white)) moves.push_back({from, to}); }
            } else if (type == 6) {
                for (int d : {-17, -16, -15, -1, 1, 15, 16, 17}) { int to = from + d; if (validStep(from, to, d) && (!squares[to] || (squares[to] > 0) != white)) moves.push_back({from, to}); }
                int home = white ? 0 : 7, king = white ? WK : BK;
                if (from == home * 16 + 4 && !inCheck(white)) {
                    int kingRight = white ? 1 : 4, queenRight = white ? 2 : 8;
                    if ((castling & kingRight) && squares[from + 1] == EMPTY && squares[from + 2] == EMPTY && !attacked(from + 1, !white) && !attacked(from + 2, !white)) moves.push_back({from, from + 2, 0, false, true});
                    if ((castling & queenRight) && squares[from - 1] == EMPTY && squares[from - 2] == EMPTY && squares[from - 3] == EMPTY && !attacked(from - 1, !white) && !attacked(from - 2, !white)) moves.push_back({from, from - 2, 0, false, true});
                }
                (void)king;
            } else {
                const bool sliding = type == 3 || type == 4 || type == 5;
                const int* dirs = nullptr; int count = 0;
                static const int bishopDirs[] = {-17, -15, 15, 17};
                static const int rookDirs[] = {-16, -1, 1, 16};
                static const int queenDirs[] = {-17, -16, -15, -1, 1, 15, 16, 17};
                if (type == 3) { dirs = bishopDirs; count = 4; } else if (type == 4) { dirs = rookDirs; count = 4; } else { dirs = queenDirs; count = 8; }
                for (int i = 0; i < count; ++i) for (int to = from + dirs[i]; validRay(from, to, dirs[i]); to += dirs[i]) { if (!squares[to]) moves.push_back({from, to}); else { if ((squares[to] > 0) != white) moves.push_back({from, to}); break; } }
                (void)sliding;
            }
        }
        return moves;
    }

    vector<Move> legalMoves() const {
        vector<Move> legal;
        for (const Move& move : pseudoMoves()) { Board next = *this; next.make(move); if (!next.inCheck(white)) legal.push_back(move); }
        return legal;
    }

    void make(const Move& move) {
        int piece = squares[move.from], captured = squares[move.to];
        squares[move.from] = EMPTY;
        if (move.enPassant) squares[move.to + (piece > 0 ? -16 : 16)] = EMPTY;
        squares[move.to] = move.promotion ? move.promotion : piece;
        if (abs(piece) == 6) {
            if (piece > 0) castling &= ~3; else castling &= ~12;
            if (move.castle) { int rookFrom = move.to > move.from ? move.from + 3 : move.from - 4, rookTo = move.to > move.from ? move.from + 1 : move.from - 1; squares[rookTo] = squares[rookFrom]; squares[rookFrom] = EMPTY; }
        }
        if (piece == WR && move.from == 0) castling &= ~2;
        if (piece == WR && move.from == 7) castling &= ~1;
        if (piece == BR && move.from == 112) castling &= ~8;
        if (piece == BR && move.from == 119) castling &= ~4;
        if (captured == WR && move.to == 0) castling &= ~2;
        if (captured == WR && move.to == 7) castling &= ~1;
        if (captured == BR && move.to == 112) castling &= ~8;
        if (captured == BR && move.to == 119) castling &= ~4;
        ep = (abs(piece) == 1 && abs(move.to - move.from) == 32) ? (move.from + move.to) / 2 : -1;
        halfmove = (abs(piece) == 1 || captured || move.enPassant) ? 0 : halfmove + 1;
        if (!white) ++fullmove;
        white = !white;
    }

    int evaluate() const {
        static const int values[] = {0, 100, 320, 330, 500, 900, 20000};
        int score = 0;
        int whiteBishops = 0, blackBishops = 0;
        for (int sq = 0; sq < 128; ++sq) if (!(sq & 8) && squares[sq]) {
            int piece = squares[sq], type = abs(piece), sign = piece > 0 ? 1 : -1;
            int rank = sq >> 4, file = sq & 7;
            score += sign * values[type];
            if (type == 3) piece > 0 ? ++whiteBishops : ++blackBishops;
            if (type == 1) {
                int advance = piece > 0 ? rank : 7 - rank;
                score += sign * advance * 7;
                if (file >= 2 && file <= 5) score += sign * 10;
                if (advance >= 5) score += sign * advance * 10;
            } else if (type == 2 || type == 3 || type == 4 || type == 5) {
                int centerDistance = abs(file - 3) + abs(rank - 3);
                score += sign * (14 - centerDistance * 3);
            }
        }
        if (whiteBishops >= 2) score += 25;
        if (blackBishops >= 2) score -= 25;
        int mobility = static_cast<int>(pseudoMoves().size());
        score += (white ? 1 : -1) * mobility * 2;
        if (inCheck(!white)) score += 35;
        return white ? score : -score;
    }

    static int pieceFromChar(char c) { const string chars = "PNBRQKpnbrqk"; size_t p = chars.find(c); return p == string::npos ? EMPTY : (p < 6 ? static_cast<int>(p + 1) : -static_cast<int>(p - 5)); }
    static int parseSquare(const string& s) { return s.size() == 2 && s[0] >= 'a' && s[0] <= 'h' && s[1] >= '1' && s[1] <= '8' ? (s[1] - '1') * 16 + s[0] - 'a' : -1; }
};

string squareName(int sq) { string s; s += char('a' + (sq & 7)); s += char('1' + (sq >> 4)); return s; }
string moveName(const Move& m) { string s = squareName(m.from) + squareName(m.to); if (m.promotion) s += char(tolower(" PNBRQK"[abs(m.promotion)])); return s; }

char pieceChar(int piece) {
    static const string pieces = ".PNBRQK";
    if (!piece) return '.';
    char result = pieces[abs(piece)];
    return piece > 0 ? result : static_cast<char>(tolower(result));
}

void printBoard(const Board& board) {
    cout << "\n  +-----------------+\n";
    for (int rank = 7; rank >= 0; --rank) {
        cout << rank + 1 << " | ";
        for (int file = 0; file < 8; ++file) cout << pieceChar(board.squares[rank * 16 + file]) << ' ';
        cout << "|\n";
    }
    cout << "  +-----------------+\n    a b c d e f g h\n";
}

Move parseMove(const Board& board, const string& text) {
    for (const Move& move : board.legalMoves()) if (moveName(move) == text) return move;
    return {};
}

int moveScore(const Board& board, const Move& move) {
    int score = 0;
    int captured = move.enPassant ? (board.white ? BP : WP) : board.squares[move.to];
    if (captured) score += 10000 + abs(captured) * 100 - abs(board.squares[move.from]);
    if (move.promotion) score += 12000 + abs(move.promotion) * 100;
    Board next = board;
    next.make(move);
    if (next.inCheck(next.white)) score += 9000;
    if (abs(board.squares[move.from]) == 1) score += (move.to >> 4) * (board.white ? 8 : -8);
    return score;
}

void orderMoves(const Board& board, vector<Move>& moves) {
    stable_sort(moves.begin(), moves.end(), [&board](const Move& left, const Move& right) {
        return moveScore(board, left) > moveScore(board, right);
    });
}

int repetitionCount(const vector<string>& history, const string& key) {
    return static_cast<int>(count(history.begin(), history.end(), key));
}

int quiescence(const Board& board, int alpha, int beta, int remaining) {
    vector<Move> moves = board.legalMoves();
    if (moves.empty()) return board.inCheck(board.white) ? -MATE : 0;
    int standPat = board.evaluate();
    if (remaining == 0) return standPat;
    if (!board.inCheck(board.white)) {
        if (standPat >= beta) return standPat;
        alpha = max(alpha, standPat);
    }
    orderMoves(board, moves);
    for (const Move& move : moves) {
        int captured = move.enPassant ? 1 : board.squares[move.to];
        Board next = board;
        next.make(move);
        bool forcing = board.inCheck(board.white) || captured || move.promotion || next.inCheck(next.white);
        if (!forcing) continue;
        int score = -quiescence(next, -beta, -alpha, remaining - 1);
        if (score >= beta) return score;
        alpha = max(alpha, score);
    }
    return alpha;
}

int negamax(const Board& board, int depth, int alpha, int beta, vector<string>& history) {
    vector<Move> moves = board.legalMoves();
    if (moves.empty()) return board.inCheck(board.white) ? -MATE + depth : 0;
    if (board.halfmove >= 100 || repetitionCount(history, board.positionKey()) >= 3) return 0;
    if (depth == 0) return quiescence(board, alpha, beta, 6);
    orderMoves(board, moves);
    int best = -INF;
    for (const Move& move : moves) {
        Board next = board;
        next.make(move);
        history.push_back(next.positionKey());
        int score = -negamax(next, depth - 1, -beta, -alpha, history);
        history.pop_back();
        best = max(best, score);
        alpha = max(alpha, score);
        if (alpha >= beta) break;
    }
    return best;
}

Move openingBookMove(const Board& board) {
    static unsigned variation = static_cast<unsigned>(chrono::steady_clock::now().time_since_epoch().count()) % 4;
    if (board.fullmove > 2 || board.halfmove > 1 || board.inCheck(board.white)) return {};
    int pieceCount = 0;
    for (int sq = 0; sq < 128; ++sq) if (!(sq & 8) && board.squares[sq]) ++pieceCount;
    if (pieceCount != 32) return {};

    vector<string> candidates;
    if (board.fullmove == 1 && board.white) {
        candidates = {"e2e4", "d2d4", "c2c4", "g1f3"};
    } else if (board.fullmove == 1 && !board.white) {
        if (board.squares[52] == WP) candidates = {"e7e5", "c7c5", "g8f6"};
        else if (board.squares[51] == WP) candidates = {"d7d5", "g8f6", "e7e6"};
        else if (board.squares[50] == WP) candidates = {"e7e5", "c7c5", "g8f6"};
    } else if (board.fullmove == 2 && board.white) {
        if (board.squares[52] == WP && board.squares[68] == BP) candidates = {"g1f3", "f1c4", "d2d3"};
        else if (board.squares[51] == WP && board.squares[67] == BP) candidates = {"c2c4", "g1f3", "e2e3"};
        else if (board.squares[50] == WP) candidates = {"g1f3", "d2d4", "b1c3"};
    } else if (board.fullmove == 2 && !board.white) {
        candidates = {"b8c6", "g8f6", "f8c5"};
    }
    if (candidates.empty()) return {};
    for (size_t offset = 0; offset < candidates.size(); ++offset) {
        Move move = parseMove(board, candidates[(variation + offset) % candidates.size()]);
        if (move.from >= 0) return move;
    }
    return {};
}

Move chooseMove(const Board& board, int maxDepth, int moveTimeMs, const vector<string>& rootHistory = {}) {
    Move book = openingBookMove(board);
    if (book.from >= 0) return book;
    Move best{}; auto start = chrono::steady_clock::now();
    vector<string> history = rootHistory;
    if (history.empty()) history.push_back(board.positionKey());
    for (int depth = 1; depth <= maxDepth; ++depth) {
        int score = -INF; Move depthBest{};
        vector<Move> moves = board.legalMoves();
        orderMoves(board, moves);
        for (const Move& move : moves) {
            Board next = board; next.make(move);
            int value = 0;
            history.push_back(next.positionKey());
            value = -negamax(next, depth - 1, -INF, INF, history);
            history.pop_back();
            if (value > score) { score = value; depthBest = move; }
            if (chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start).count() >= moveTimeMs) return best.from >= 0 ? best : depthBest;
        }
        best = depthBest;
        if (score > MATE - 1000) break;
    }
    return best;
}

#ifdef _WIN32
struct GuiState {
    Board board;
    vector<string> history;
    int dragSquare = -1;
    int depth = 3;
    string message = "Drag a white piece to make a move.";
};

int guiSquareFromPoint(int x, int y) {
    if (x < 0 || y < 0 || x >= 640 || y >= 640) return -1;
    return (7 - y / 80) * 16 + x / 80;
}

int guiMaterial(const Board& board, bool white) {
    static const int values[] = {0, 100, 320, 330, 500, 900, 20000};
    int score = 0;
    for (int sq = 0; sq < 128; ++sq) if (!(sq & 8) && board.squares[sq] && (board.squares[sq] > 0) == white) score += values[abs(board.squares[sq])];
    return score;
}

int guiMobility(const Board& board, bool white) {
    Board copy = board;
    copy.white = white;
    return static_cast<int>(copy.legalMoves().size());
}

void guiText(HDC dc, int x, int y, const string& text, int size = 18) {
    HFONT font = CreateFontA(size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_SWISS, "Segoe UI");
    HFONT old = static_cast<HFONT>(SelectObject(dc, font));
    SetBkMode(dc, TRANSPARENT);
    TextOutA(dc, x, y, text.c_str(), static_cast<int>(text.size()));
    SelectObject(dc, old);
    DeleteObject(font);
}

void drawGui(HWND hwnd, HDC dc, const GuiState& state) {
    RECT client;
    GetClientRect(hwnd, &client);
    HBRUSH background = CreateSolidBrush(RGB(238, 241, 236));
    FillRect(dc, &client, background);
    DeleteObject(background);
    static const COLORREF light = RGB(236, 221, 190), dark = RGB(117, 150, 112);
    static const char* pieces = ".PNBRQK";
    for (int rank = 7; rank >= 0; --rank) for (int file = 0; file < 8; ++file) {
        int x = file * 80, y = (7 - rank) * 80;
        HBRUSH squareBrush = CreateSolidBrush((file + rank) % 2 ? dark : light);
        RECT square{x, y, x + 80, y + 80};
        FillRect(dc, &square, squareBrush);
        DeleteObject(squareBrush);
        if (state.dragSquare == rank * 16 + file) {
            HPEN pen = CreatePen(PS_SOLID, 5, RGB(220, 180, 40));
            HGDIOBJ oldPen = SelectObject(dc, pen);
            HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
            Rectangle(dc, x + 3, y + 3, x + 77, y + 77);
            SelectObject(dc, oldBrush);
            SelectObject(dc, oldPen);
            DeleteObject(pen);
        }
        int piece = state.board.squares[rank * 16 + file];
        if (piece) {
            string label(1, piece > 0 ? pieces[abs(piece)] : static_cast<char>(tolower(pieces[abs(piece)])));
            SetTextColor(dc, piece > 0 ? RGB(28, 28, 28) : RGB(245, 245, 245));
            guiText(dc, x + 27, y + 18, label, 42);
        }
    }
    SetTextColor(dc, RGB(35, 43, 39));
    int whiteMaterial = guiMaterial(state.board, true), blackMaterial = guiMaterial(state.board, false);
    int whiteMobility = guiMobility(state.board, true), blackMobility = guiMobility(state.board, false);
    int materialDifference = whiteMaterial - blackMaterial;
    string advantage = materialDifference > 0 ? "White advantage" : materialDifference < 0 ? "Black advantage" : "Material even";
    guiText(dc, 675, 50, "PocketCpp", 28);
    guiText(dc, 675, 105, "Position difference", 20);
    guiText(dc, 675, 145, "White material: " + to_string(whiteMaterial), 17);
    guiText(dc, 675, 175, "Black material: " + to_string(blackMaterial), 17);
    guiText(dc, 675, 220, "Material: " + advantage, 17);
    guiText(dc, 675, 265, "White mobility: " + to_string(whiteMobility), 17);
    guiText(dc, 675, 295, "Black mobility: " + to_string(blackMobility), 17);
    guiText(dc, 675, 340, string("White king: ") + (state.board.inCheck(true) ? "in check" : "safe"), 17);
    guiText(dc, 675, 370, string("Black king: ") + (state.board.inCheck(false) ? "in check" : "safe"), 17);
    guiText(dc, 675, 430, state.message, 16);
    guiText(dc, 675, 490, "Drag pieces to play", 16);
    guiText(dc, 675, 520, "Esc closes the game", 16);
}

LRESULT CALLBACK guiWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    GuiState* state = reinterpret_cast<GuiState*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        auto* create = reinterpret_cast<CREATESTRUCTA*>(lParam);
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
        return TRUE;
    }
    if (!state) return DefWindowProcA(hwnd, message, wParam, lParam);
    if (message == WM_PAINT) {
        PAINTSTRUCT paint;
        HDC dc = BeginPaint(hwnd, &paint);
        drawGui(hwnd, dc, *state);
        EndPaint(hwnd, &paint);
        return 0;
    }
    if (message == WM_LBUTTONDOWN) {
        int x = static_cast<int>(static_cast<short>(LOWORD(lParam)));
        int y = static_cast<int>(static_cast<short>(HIWORD(lParam)));
        int square = guiSquareFromPoint(x, y);
        if (state->dragSquare >= 0 && square >= 0) {
            PostMessageA(hwnd, WM_LBUTTONUP, wParam, lParam);
        } else if (square >= 0 && state->board.white && state->board.squares[square] > 0) {
            state->dragSquare = square;
            SetCapture(hwnd);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    }
    if (message == WM_LBUTTONUP && state->dragSquare >= 0) {
        int x = static_cast<int>(static_cast<short>(LOWORD(lParam)));
        int y = static_cast<int>(static_cast<short>(HIWORD(lParam)));
        int target = guiSquareFromPoint(x, y);
        Move selected{};
        for (const Move& move : state->board.legalMoves()) if (move.from == state->dragSquare && move.to == target) { selected = move; break; }
        ReleaseCapture();
        state->dragSquare = -1;
        if (selected.from < 0) state->message = "Illegal move. Try another destination.";
        else {
            state->board.make(selected);
            state->history.push_back(state->board.positionKey());
            state->message = "Bot is thinking...";
            InvalidateRect(hwnd, nullptr, FALSE);
            UpdateWindow(hwnd);
            Move reply = chooseMove(state->board, state->depth, 1000, state->history);
            if (reply.from >= 0) { state->board.make(reply); state->history.push_back(state->board.positionKey()); state->message = "Bot played " + moveName(reply); }
            else state->message = "Game over.";
        }
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }
    if (message == WM_KEYDOWN && wParam == VK_ESCAPE) { DestroyWindow(hwnd); return 0; }
    if (message == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcA(hwnd, message, wParam, lParam);
}

int runGui() {
    GuiState state;
    state.board.start();
    state.history.push_back(state.board.positionKey());
    HINSTANCE instance = GetModuleHandleA(nullptr);
    const char* className = "PocketCppBoard";
    WNDCLASSA windowClass{};
    windowClass.hInstance = instance;
    windowClass.lpfnWndProc = guiWindowProc;
    windowClass.lpszClassName = className;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    RegisterClassA(&windowClass);
    HWND window = CreateWindowExA(0, className, "PocketCpp - Play Against the Engine", WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 980, 700, nullptr, nullptr, instance, &state);
    ShowWindow(window, SW_SHOW);
    UpdateWindow(window);
    MSG message;
    while (GetMessageA(&message, nullptr, 0, 0) > 0) { TranslateMessage(&message); DispatchMessageA(&message); }
    return 0;
}
#endif

void selfPlay(int maxPlies, int depth) {
    Board board;
    board.start();
    vector<string> history{board.positionKey()};
    printBoard(board);
    for (int ply = 0; ply < maxPlies; ++ply) {
        vector<Move> moves = board.legalMoves();
        if (moves.empty()) break;
        Move move = chooseMove(board, depth, 1000, history);
        if (move.from < 0) break;
        cout << (board.white ? "White" : "Black") << " " << moveName(move) << '\n';
        board.make(move);
        history.push_back(board.positionKey());
        printBoard(board);
    }
    if (board.legalMoves().empty()) cout << (board.inCheck(board.white) ? "checkmate\n" : "stalemate\n");
}

void humanPlay(int depth) {
    Board board;
    board.start();
    vector<string> history{board.positionKey()};
    printBoard(board);
    cout << "You are White. Enter moves like e2e4, or type quit.\n";
    string text;
    while (true) {
        vector<Move> legal = board.legalMoves();
        if (legal.empty()) { cout << (board.inCheck(board.white) ? "checkmate\n" : "stalemate\n"); return; }
        cout << "Your move: " << flush;
        if (!getline(cin, text) || text == "quit") return;
        Move humanMove = parseMove(board, text);
        if (humanMove.from < 0) { cout << "Invalid move. Use coordinate notation such as e2e4.\n"; continue; }
        board.make(humanMove);
        history.push_back(board.positionKey());
        printBoard(board);
        legal = board.legalMoves();
        if (legal.empty()) { cout << (board.inCheck(board.white) ? "You win by checkmate.\n" : "Draw by stalemate.\n"); return; }
        Move reply = chooseMove(board, depth, 1000, history);
        if (reply.from < 0) { cout << "The bot has no legal move.\n"; return; }
        cout << "Bot: " << moveName(reply) << '\n';
        board.make(reply);
        history.push_back(board.positionKey());
        printBoard(board);
    }
}

int main(int argc, char** argv) {
    ios::sync_with_stdio(false); cin.tie(nullptr);
    if (argc >= 2 && string(argv[1]) == "--selfplay") {
        int plies = argc >= 3 ? max(1, atoi(argv[2])) : 40;
        int depth = argc >= 4 ? max(1, atoi(argv[3])) : 3;
        selfPlay(plies, depth);
        return 0;
    }
    if (argc >= 2 && string(argv[1]) == "--play") {
        int depth = argc >= 3 ? max(1, atoi(argv[2])) : 3;
        humanPlay(depth);
        return 0;
    }
#ifdef _WIN32
    if (argc >= 2 && string(argv[1]) == "--gui") return runGui();
#endif
    Board board; board.start(); vector<string> history{board.positionKey()}; string line;
    while (getline(cin, line)) {
        istringstream in(line); string command; in >> command;
        if (command == "uci") { cout << "id name PocketCpp\nid author OpenAI\noption name MoveTime type spin default 100 min 1 max 10000\nuciok\n" << flush; }
        else if (command == "isready") cout << "readyok\n" << flush;
        else if (command == "ucinewgame") { board.start(); history = {board.positionKey()}; }
        else if (command == "position") {
            string token; in >> token;
            if (token == "startpos") board.start();
            else if (token == "fen") { string fen, part; for (int i = 0; i < 6 && in >> part; ++i) { if (i) fen += ' '; fen += part; } board.setFen(fen); }
            history = {board.positionKey()};
            if (in >> token && token == "moves") while (in >> token) { Move move = parseMove(board, token); if (move.from >= 0) { board.make(move); history.push_back(board.positionKey()); } }
        } else if (command == "go") {
            int depth = 5, moveTime = 100;
            string token; while (in >> token) { if (token == "depth") in >> depth; else if (token == "movetime") in >> moveTime; else if (token == "wtime" || token == "btime") { int time; in >> time; moveTime = max(10, time / 30); } }
            Move best = chooseMove(board, min(depth, 8), moveTime, history); cout << "bestmove " << (best.from >= 0 ? moveName(best) : "0000") << '\n' << flush;
        } else if (command == "quit") break;
    }
    return 0;
}
