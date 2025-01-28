
echo Testing '&&'
echo hello && true && echo world && false && echo bad

echo Testing '||'
echo hello || echo world
true || echo bad
false || echo world
false || echo good || echo bad

echo Testing combinations of '&&' and '||'
echo hello || echo bad && echo world || echo bad2

echo Testing return codes
if true || false ; then echo good ; else echo bad ; fi
if false || true ; then echo good ; else echo bad ; fi
if true || true ; then echo good ; else echo bad ; fi
if false || false ; then echo bad ; else echo good ; fi
if true && false ; then echo bad ; else echo good ; fi
if false && true ; then echo bad ; else echo good ; fi
if true && true ; then echo good ; else echo bad ; fi
if false && false ; then echo bad ; else echo good ; fi
if true && false || true && false || true ; then echo good ; else echo bad ; fi
if true && false || true || false && true ; then echo good ; else echo bad ; fi

