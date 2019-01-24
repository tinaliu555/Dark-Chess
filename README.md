# DarkChess

Platform: Linux/Windows

[AI Game]
Main Search Algorithm: NegaScout + Chance Search
Transposition Table: Using ZobristKey encoding with 2^64 hash table and 2^28 hash key
Search Order: Move Ordering
Dynamic Depth Search: Aspiration search + Quiescent search &
Static Exchange Evaluation (SEE) + Conditional depth extension
Knowledge heuristics: Material Value + Dynamic Pawn Value + Dynamic King Value
End game heuristics: Attack Value + Binding opponent + Avoid Tie

[Competition]
3rd place of The 12th NTU CSIE CUP of Computer Chinese Dark Chess Competition
