
echo Section 3
echo Testing basic commands
echo a
echo b
echo c
pwd
echo Testing variable substitution
a=hello
b=$a
echo $b world
b=
echo empty b $b

echo Testing return codes
if false ; then
	echo bad1
elif true ; then
	echo good1
	b=hello
	echo b is $b
elif false ; then
	echo bad2
else
	echo bad3
fi

echo Testing for_loop
for x in a b c ; do
	echo x is $x
done

