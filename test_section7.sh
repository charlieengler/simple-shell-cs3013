
echo Section 7 pipe streams
echo hello | grep h
cat /usr/share/dict/words | grep ^z.*o$

echo Arbitrarily deep pipes
find /usr/include -type f -print0 | xargs -0 grep -l uintptr_t | wc --lines

echo The most common letters are
cat /usr/share/dict/words | grep -o . | sort | uniq -c | sort -n -r | head

echo Pipe stream return codes as predicates
if cat test_section5_b.sh | grep -q hello ; then
	echo this is broken
elif cat test_section5_c.sh | grep -q hello ; then
	echo return code works properly
else
	echo this is broken
fi

