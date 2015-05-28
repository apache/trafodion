1268c1268
< NESTED_JOIN
---
> HYBRID_#HASH_JOIN
1300,1302c1300
< TRAFODION_SCAN
< NESTED_JOIN
< PROBE_CACHE 1.000E+000 10 fragment_
---
> HYBRID_#HASH_JOIN
1304a1303
> TRAFODION_SCAN
1306c1305
< --- 5 row(s) selected.
---
> --- 4 row(s) selected.
1778c1777
< join_method: hash parallel_join_
---
> join_method: hash hash_join_predi
1797c1796
< 1.2
---
> 1.0
1801c1800
< 1.2
---
> 1.0
2112c2111
< 1.2
---
> 1.0
2116c2115
< 1.2
---
> 1.0
2259c2258
< NESTED_JOIN
---
> HYBRID_#HASH_JOIN
2272c2271
< NESTED_JOIN
---
> HYBRID_#HASH_JOIN
2286c2285
< NESTED_JOIN
---
> HYBRID_#HASH_JOIN
2690,2695c2689
< OPERATOR
< ----------
< 
< TUPLELIST
< 
< --- 1 row(s) selected.
---
> --- 0 row(s) selected.
2733,2738c2727
< OPERATOR
< ----------
< 
< TUPLELIST
< 
< --- 1 row(s) selected.
---
> --- 0 row(s) selected.
2763c2752
< +>where operator in ('TRAFODION_SCAN') and position('mdam_disjunct' in description) > 0
---
> +>where operator in ('FILE_SCAN') and position('mdam_disjunct' in description) > 0
2794,2799c2783
< TNAME MDAM_DISJUNCT
< ----------
< 
< C2 (CAT.MYHCUBE.CUBE2) (B = count(1 )) and ((A = 1) o
< 
< --- 1 row(s) selected.
---
> --- 0 row(s) selected.
2805c2789
< NESTED_JOIN
---
> HYBRID_#HASH_JOIN
2817,2822c2801
< TNAME MDAM_DISJUNCT
< ----------
< 
< C2 (CAT.MYHCUBE.CUBE2) (B = max(CAT.MYHCUBE.T0.A)) an
< 
< --- 1 row(s) selected.
---
> --- 0 row(s) selected.
2828c2807
< NESTED_JOIN
---
> HYBRID_#HASH_JOIN
2840,2845c2819
< TNAME MDAM_DISJUNCT
< ----------
< 
< C2 (CAT.MYHCUBE.CUBE2) (B = sum(CAT.MYHCUBE.T0.B)) an
< 
< --- 1 row(s) selected.
---
> --- 0 row(s) selected.
2851c2825
< NESTED_JOIN
---
> HYBRID_#HASH_JOIN
2863,2868c2837
< TNAME MDAM_DISJUNCT
< ----------
< 
< C2 (CAT.MYHCUBE.CUBE2) (B = 2) and ((A = 1) or (A = 3
< 
< --- 1 row(s) selected.
---
> --- 0 row(s) selected.
2874c2843
< NESTED_JOIN
---
> HYBRID_#HASH_JOIN
2888,2893c2857
< TNAME MDAM_DISJUNCT
< ----------
< 
< C2 (CAT.MYHCUBE.CUBE2) ((A = 1) or (A = 3))
< 
< --- 1 row(s) selected.
---
> --- 0 row(s) selected.
2911,2916c2875
< TNAME MDAM_DISJUNCT
< ----------
< 
< C2 (CAT.MYHCUBE.CUBE2) (B = 2) and ((A = 1) or (A = 3
< 
< --- 1 row(s) selected.
---
> --- 0 row(s) selected.
2945,2950c2904
< TNAME MDAM_DISJUNCT
< ----------
< 
< C2 (CAT.MYHCUBE.CUBE2) ((A = 1) or (A = 3))
< 
< --- 1 row(s) selected.
---
> --- 0 row(s) selected.
2968,2973c2922
< TNAME MDAM_DISJUNCT
< ----------
< 
< C2 (CAT.MYHCUBE.CUBE2) ((A = 1) or (A = 3))
< 
< --- 1 row(s) selected.
---
> --- 0 row(s) selected.
2991,2996c2940
< TNAME MDAM_DISJUNCT
< ----------
< 
< C2 (CAT.MYHCUBE.CUBE2) ((A = 1) or (A = 3))
< 
< --- 1 row(s) selected.
---
> --- 0 row(s) selected.
3014,3019c2958
< TNAME MDAM_DISJUNCT
< ----------
< 
< C2 (CAT.MYHCUBE.CUBE2) (B = 2) and ((A = 1) or (A = 3
< 
< --- 1 row(s) selected.
---
> --- 0 row(s) selected.
3039,3044c2978
< TNAME MDAM_DISJUNCT
< ----------
< 
< C2 (CAT.MYHCUBE.CUBE2) ((A = 1) or (A = 3))
< 
< --- 1 row(s) selected.
---
> --- 0 row(s) selected.
3062,3067c2996
< TNAME MDAM_DISJUNCT
< ----------
< 
< C2 (CAT.MYHCUBE.CUBE2) (B = 2) and ((A = 1) or (A = 3
< 
< --- 1 row(s) selected.
---
> --- 0 row(s) selected.
3099,3104c3028
< TNAME MDAM_DISJUNCT
< ----------
< 
< C2 (CAT.MYHCUBE.CUBE2) (B = count(1 )) and ((A = 1) o
< 
< --- 1 row(s) selected.
---
> --- 0 row(s) selected.
3110c3034
< NESTED_JOIN
---
> HYBRID_#HASH_JOIN
3122,3127c3046
< TNAME MDAM_DISJUNCT
< ----------
< 
< C2 (CAT.MYHCUBE.CUBE2) (B = max(CAT.MYHCUBE.T0.A)) an
< 
< --- 1 row(s) selected.
---
> --- 0 row(s) selected.
3133c3052
< NESTED_JOIN
---
> HYBRID_#HASH_JOIN
3145,3150c3064
< TNAME MDAM_DISJUNCT
< ----------
< 
< C2 (CAT.MYHCUBE.CUBE2) (B = sum(CAT.MYHCUBE.T0.B)) an
< 
< --- 1 row(s) selected.
---
> --- 0 row(s) selected.
3156c3070
< NESTED_JOIN
---
> HYBRID_#HASH_JOIN
3168,3173c3082
< TNAME MDAM_DISJUNCT
< ----------
< 
< C2 (CAT.MYHCUBE.CUBE2) (B = 2) and ((A = 1) or (A = 3
< 
< --- 1 row(s) selected.
---
> --- 0 row(s) selected.
3179c3088
< NESTED_JOIN
---
> HYBRID_#HASH_JOIN
3193,3198c3102
< TNAME MDAM_DISJUNCT
< ----------
< 
< C2 (CAT.MYHCUBE.CUBE2) ((A = 1) or (A = 3))
< 
< --- 1 row(s) selected.
---
> --- 0 row(s) selected.
3216,3221c3120
< TNAME MDAM_DISJUNCT
< ----------
< 
< C2 (CAT.MYHCUBE.CUBE2) (B = 2) and ((A = 1) or (A = 3
< 
< --- 1 row(s) selected.
---
> --- 0 row(s) selected.
